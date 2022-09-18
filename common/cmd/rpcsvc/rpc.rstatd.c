/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rpcsvc:rpc.rstatd.c	1.3"
#ident	"$Header: $"

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 *
 * rstat demon:  called from inet
 * Old Version was: rpc.rstatd.c 1.1 86/09/25 Copyr 1984 Sun Micro
 *
 * Version: DM 24.04.90
 */

#define NO_BOOTTIME     /* delete if there is a "boottime"-Symbol in    */
                        /* the kernel.                                  */
#include <signal.h>
#include <stdio.h>
#include <rpc/rpc.h>
#include <sys/ksym.h>

#include <sys/errno.h>
#include <sys/vmmeter.h>
#include <sys/time.h>
#include <sys/sysinfo.h>

#include <rpcsvc/rstat.h> 

#include <stropts.h>            /* D.M. */
#include <sys/socket.h>         /* D.M. */
#include <net/if.h>             /* D.M. */
#include <utmp.h>               /* D.M. */
#include <sys/resource.h>       /* D.M. */

#ifdef SYSLOG
#include <syslog.h>
#else
#define LOG_ERR         1
#define openlog(a,b,c)
#endif

#ifdef DEBUG
#define RPC_SVC_FG
#endif

#define bzero(s,n)      memset((s), 0, (n))
#define bcopy(a,b,c)    memcpy((b), (a), (c))
void getinfo();

#define IFSTATS       "ifstats"
#define AVENRUN       "avenrun"
#define HZ            "Hz"
#define  DKXFER       "dk_xfer"
#define SYSINFO       "sysinfo"
#define SUM           "sum"

int             fg_flag = 0;    /* foregroung Flag      */
int kmem;
int firstifnet, numintfs;       /* chain of ethernet interfaces */
int hz;                         /* clock rate of machine */
struct timeval btm;             /* boot time */

int sincelastreq = 0;           /* number of alarms since last request */
#define CLOSEDOWN        20     /* how long to wait before exiting */
#define UPDATE_INTERVAL   1     /* Number of seconds until next update */

union {
    struct stats s1;
    struct statsswtch s2;
    struct statstime s3;
} stats_u;

/* DM 24.04.90  */
#define _RPCSVC_CLOSEDOWN       120

static void closedown();
static void msgout();



void updatestat();
int stats_service();
extern int errno;

#ifndef FSCALE
#define FSCALE (1 << 8)
#endif

#define MIN(a, b) (((a) < (b)) ? (a) : (b))


#define ISTEAL(where, var, size) \
        if ((where) == 0) { \
                bzero((char *)(var), (size)); \
        } else { \
                if (lseek(kmem, (off_t)(where), 0) != (where)) { \
                        fprintf(stderr, "lseek of symbol %s failed!\n", #var); \
                        exit(1); \
                } \
                if (read(kmem, (caddr_t)(var), (size)) != (size)) { \
                        fprintf(stderr, "read of symbol %s failed!\n", #var); \
                        exit(1); \
                } \
        }

static int      _rpcpmstart;            /* Started by a port monitor    */
static int      _rpcfdtype;             /* Whether Stream or Datagram ? */
static int      _rpcsvcdirty;           /* Still serving ?              */

main(argc, argv)

int    argc;
char **argv;
{
char                    mname[FMNAMESZ + 1];
char                   *netid;
struct netconfig       *nconf = NULL;
SVCXPRT                *transp;
int                     pmclose;
extern char            *getenv();
pid_t                   pid;
int                     i;
        

if (!ioctl(0, I_LOOK, mname) &&
   (!strcmp(mname, "sockmod") || !strcmp(mname, "timod")))
    {
    _rpcpmstart = 1;
            
    if ((netid = getenv("NLSPROVIDER")) == NULL)
        {
        msgout("cannot get transport name");
        }
    else if ((nconf = getnetconfigent(netid)) == NULL)
        {
        msgout("cannot get transport info");
        }
    if (strcmp(mname, "sockmod") == 0)
        {
        if (ioctl(0, I_POP, 0) || ioctl(0, I_PUSH, "timod"))
            {
            msgout("could not get the right module");
            exit(1);
            }
        }
    pmclose = (t_getstate(0) != T_DATAXFER);

    if ((transp = svc_tli_create(0, nconf, NULL, 0, 0)) == NULL)
        {
        msgout ("cannot create server handle");
        exit(1);
        }
    if (nconf)
        freenetconfigent(nconf);
    if (!svc_reg(transp, RSTATPROG, RSTATVERS_ORIG, stats_service, 0))
        {
        msgout("unable to register (RSTATPROG, RSTATVERS_ORIG).");
        exit(1);
        }
    if (!svc_reg(transp, RSTATPROG, RSTATVERS_SWTCH, stats_service, 0)) 
        {
        msgout("unable to register (RSTATPROG, RSTATVERS_SWTCH).");
        exit(1);
        }
    if (!svc_reg(transp, RSTATPROG, RSTATVERS_TIME, stats_service, 0)) 
        {
        msgout("unable to register (RSTATPROG, RSTATVERS_SWTCH).");
        exit(1);
        }
    if (pmclose)
        {
        (void) signal(SIGALRM, closedown);
        (void) alarm(_RPCSVC_CLOSEDOWN);
        }

    setup();
    updatestat();
    svc_run();
    msgout("svc_run should never return\n");
    exit(1);
    /* NOTREACHED       */
    }
    
#ifndef RPC_SVC_FG
pid = fork();
if (pid < 0)
    {
    perror("cannot fork");
    exit(1);
    }

if (pid)
    exit(0);
for (i = 0; i < 20; i++)
    (void) close(i);
setsid();
openlog("rstat", LOG_PID, LOG_DAEMON);
#endif 

fg_flag = 1;    

if (!svc_create(stats_service, RSTATPROG, RSTATVERS_ORIG, "netpath"))
    {
    msgout("unable to create (RSTATPROG, RSTATVERS_ORIG) for netpath.");
    exit(1);
    }
if (!svc_create(stats_service, RSTATPROG, RSTATVERS_SWTCH, "netpath"))
    {
    msgout("unable to create (RSTATPROG, RSTATVERS_SWTCH) for netpath.");
    exit(1);
    }
if (!svc_create(stats_service, RSTATPROG, RSTATVERS_TIME, "netpath"))
    {
    msgout("unable to create (RSTATPROG, RSTATVERS_TIME) for netpath.");
    exit(1);
    }

setup();
updatestat();
svc_run();
msgout("svc_run should never return\n");
exit(1);
/* NOTREACHED   */
}
    

static int
stats_service(reqp, transp)
        struct svc_req  *reqp;
        SVCXPRT  *transp;
{
        int have;
        
        switch (reqp->rq_proc) {
                case RSTATPROC_STATS:
                        sincelastreq = 0;
                        if (reqp->rq_vers == RSTATVERS_ORIG) {
                                if (svc_sendreply(transp, xdr_stats,
                                    &stats_u.s1, TRUE) == FALSE) {
                                        fprintf(stderr,
                                            "err: svc_rpc_send_results");
                                        exit(1);
                                }
                                return;
                        }
                        if (reqp->rq_vers == RSTATVERS_SWTCH) {
                                if (svc_sendreply(transp, xdr_statsswtch,
                                    &stats_u.s2, TRUE) == FALSE) {
                                        fprintf(stderr,
                                            "err: svc_rpc_send_results");
                                        exit(1);
                                    }
                                return;
                        }
                        if (reqp->rq_vers == RSTATVERS_TIME) {
                                if (svc_sendreply(transp, xdr_statstime,
                                    &stats_u.s3, TRUE) == FALSE) {
                                        fprintf(stderr,
                                            "err: svc_rpc_send_results");
                                        exit(1);
                                    }
                                return;
                        }
                case RSTATPROC_HAVEDISK:
                        have = havedisk();
                        if (svc_sendreply(transp,xdr_long, &have, TRUE) == 0){
                            fprintf(stderr, "err: svc_sendreply");
                            exit(1);
                        }
                        return;
                case 0:
                        if (svc_sendreply(transp, xdr_void, 0, TRUE)
                            == FALSE) {
                                fprintf(stderr, "err: svc_rpc_send_results");
                                exit(1);
                            }
                        return;
                default: 
                        svcerr_noproc(transp);
                        return;
                }
}

void
updatestat()
{
        int             off, i;
        struct vmmeter  sum;
        struct timeval  tm;
        struct ifstats  ifstats;
        struct sysinfo  sys_info;

        signal(SIGALRM, updatestat);

        if ((sincelastreq++ >= CLOSEDOWN) && (fg_flag == 0)) {
#ifdef DEBUG
                fprintf(stderr, "about to closedown\n");
#endif
                exit(0);
        }
        getinfo(AVENRUN, stats_u.s2.avenrun, sizeof(stats_u.s2.avenrun));
        getinfo(SUM, &sum, sizeof sum);

        gettimeofday(&tm, 0);

        getinfo(SYSINFO, &sys_info, sizeof(sys_info));

        stats_u.s1.cp_time[CP_USER] = sys_info.cpu[CPU_USER];
        stats_u.s1.cp_time[CP_NICE] = sys_info.cpu[CPU_WAIT];
        stats_u.s1.cp_time[CP_SYS]  = sys_info.cpu[CPU_KERNEL];
        stats_u.s1.cp_time[CP_IDLE] = sys_info.cpu[CPU_IDLE];

        stats_u.s1.v_pgpgin = sum.v_pgpgin;
        stats_u.s1.v_pgpgout = sum.v_pgpgout;
        stats_u.s1.v_pswpin = sum.v_pswpin;
        stats_u.s1.v_pswpout = sum.v_pswpout;
        stats_u.s1.v_intr = sum.v_intr;
        stats_u.s1.v_intr -= hz*(tm.tv_sec - btm.tv_sec) +
            hz*(tm.tv_usec - btm.tv_usec)/1000000;
        stats_u.s2.v_swtch = sum.v_swtch;
#ifdef DEBUG
        fprintf(stderr, "%d %d %d %d\n", stats_u.s1.cp_time[0],
                                         stats_u.s1.cp_time[1], 
                                         stats_u.s1.cp_time[2], 
                                         stats_u.s1.cp_time[3]);
#endif

        getinfo(DKXFER, stats_u.s1.dk_xfer, sizeof(stats_u.s1.dk_xfer));
        /* Changed D.M. 29.05.90        */
        /*      X_IFNET -> X_IFSTATS    */

        stats_u.s1.if_ipackets   = 0;
        stats_u.s1.if_opackets   = 0;
        stats_u.s1.if_ierrors    = 0;
        stats_u.s1.if_oerrors    = 0;
        stats_u.s1.if_collisions = 0;

        for (off = firstifnet, i = 0; off && i < numintfs; i++) {
                ISTEAL(off, &ifstats, sizeof (ifstats));
                stats_u.s1.if_ipackets   += ifstats.ifs_ipackets;
                stats_u.s1.if_opackets   += ifstats.ifs_opackets;
                stats_u.s1.if_ierrors    += ifstats.ifs_ierrors;
                stats_u.s1.if_oerrors    += ifstats.ifs_oerrors;
                stats_u.s1.if_collisions += ifstats.ifs_collisions;
                off = (int) ifstats.ifs_next;
        }

        gettimeofday(&stats_u.s3.curtime, 0);
        alarm(UPDATE_INTERVAL);
}

static setup()
{
        int             off, *ip;
        struct ifstats  ifstats;
        struct utmp    *utmp, utmp_id;



        if ((kmem = open("/dev/kmem", 0)) < 0) {
                fprintf(stderr, "can't open kmem\n");
                exit(1);
        }

        getinfo(HZ, &hz, sizeof(hz));

#ifdef NO_BOOTTIME
        /* Changed D.M. 29.05.90                                */
        /* because there is no "bootime" Symbol in the kernel   */
        /*      seen in in.rwhod.c                              */

        utmp_id.ut_type = BOOT_TIME;               
        if ((utmp = getutid(&utmp_id)) != NULL)    
                stats_u.s2.boottime.tv_sec = utmp->ut_time;  
        endutent();                                
#else
        getinfo(BOOTTIME, &btm, sizeof(btm));
	stats_u.s2.boottime = btm;

#endif 

        /* Changed D.M. 29.05.90        */
        /*      X_IFNET -> X_IFSTATS    */

        getinfo(IFSTATS, &firstifnet, sizeof(int));

        for (numintfs = 0, off = firstifnet; off; numintfs++) {
                ISTEAL(off, &ifstats, sizeof(ifstats));
                off = (int)ifstats.ifs_next;
        }
}

/* 
 * returns true if have a disk
 */
static
havedisk()
{
        /* In SysV Rel4 there is no way to get some information */
        /* about Drives. ("I know !")                           */
        return (1);
}

static void msgout(msg)
char  *msg;
{
#ifdef RPC_SVC_FG
if(_rpcpmstart)
    syslog(LOG_ERR, msg);
else
    (void) fprintf(stderr, "%s\n",msg);
#else
syslog(LOG_ERR, msg);
#endif
}


static void closedown()
{
if (_rpcsvcdirty == 0)
    {
    extern fd_set           svc_fdset;
    static struct rlimit    rl;
    int                     i, openfd;
    struct t_info           tinfo;

    if (t_getinfo(0, &tinfo) || (tinfo.servtype == T_CLTS))
        exit(0);
    if (rl.rlim_max == 0)
        getrlimit(RLIMIT_NOFILE, &rl);
    for (i = 0, openfd = 0; i < rl.rlim_max && openfd < 2; i++)
        if (FD_ISSET(i, &svc_fdset))
            openfd++;
    if (openfd <= 1)
        exit(0);
    }
(void) alarm(_RPCSVC_CLOSEDOWN);
}

        
void
getinfo(name,buf,buflen)
char *name;
void *buf;
size_t buflen;
{

	struct mioc_rksym rks;
	rks.mirk_symname = name;
	rks.mirk_buf = buf;
	rks.mirk_buflen = buflen;
	if(ioctl(kmem,MIOC_READKSYM,&rks) != 0)
                bzero((char *)(buf), (buflen));
}
