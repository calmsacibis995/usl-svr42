/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)rfsetup:cmd/rfsetup/rfsetup.c	1.8.11.3"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/rfsetup/rfsetup.c,v 1.1 91/02/28 19:28:38 ccs Exp $"
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stropts.h>
#include <nserve.h>
#include <sys/rf_cirmgr.h>
#include <sys/types.h>
#include <sys/rf_sys.h>
#include <pn.h>
#include <sys/hetero.h>
#include <sys/conf.h>
#include <sys/stat.h>

#include <stdlib.h>
#include <unistd.h>
#include <tiuser.h>


#define DEBUG_FILE "/var/adm/net/servers/rfs/rfs.log"
#define MAX_DB_SIZE 100000

static	pid_t	Mypid;
static	int	op;
static	char	canonbuf[20];
static	int	flags = 0;

static	FILE	*Debugfp = stderr;
static	int	null_fflush  (FILE *);
static	int	null_fprintf (FILE *, const char *, ...);
static	int	(*do_fflush) (FILE *)			= null_fflush;
static	int	(*do_fprintf)(FILE *, const char *, ...)= null_fprintf;
#define	FFLUSH	(*do_fflush)
#define	FPRINTF	(*do_fprintf)
static	void	logrequest();
static	void	logopcode();
static	void	logsetup();
static	void	gensetup();

static	void	d_exit();
static	int	banter();
static	int	get_opcode();
static	int	poptli();

extern	int	rfsys();

extern	int	errno;
extern	int	t_errno;
extern	ndata_t	ndata;
extern	pntab_t sw_tab[];

extern	int	rf_rcv();	/*libns*/
extern	int	negotiate();	/*libns*/
extern	int	setlog();	/*libns*/
extern	int	fcanon();	/*libns*/
extern	int	tcanon();	/*libns*/
extern	int	uidmap();	/*libns*/

extern	int	t_sync();	/*tli*/
extern	int	t_snd();	/*tli*/
extern	int	t_snddis();	/*tli*/

static	pnhdr_t pnhdr;

main(argc, argv)
	int		argc;
	char		**argv;
{
	int		fd;
	int		sfd;
	char		ns_pipe[256];
	char		chr;
	struct gdpmisc	gdpmisc;
	extern char	*optarg;

	fd = 0;			/* transport endpoint */

	(void) t_sync(fd);

	gdpmisc.hetero	=
	gdpmisc.version =
	gdpmisc.ngroups_max = 0;

	gensetup();
	
	while ((chr = getopt(argc,argv,"l:L")) != EOF){
		switch(chr) {
			case 'l':
				setlog(optarg);
				/* FALLTHROUGH */
			case 'L':
				logsetup();
				break;
		}
		switch(chr) {
			case 'L':
				logrequest();
				break;
		}
	}

	if (banter(fd) < 0)
		d_exit(fd);
	switch (op) {
	case RF_NS:
	/*
	 * Pass fd to name server process and exit.
	 * if pass fails, close stream and exit.
	 */
		/* select TP-dependent pipe */
		(void) sprintf(ns_pipe, TPNS_PIPE, getenv("NLSPROVIDER"));
		if ((sfd = open(ns_pipe, O_RDWR)) < 0) {
			FPRINTF(Debugfp,
			   "(%5d) cannot open ns channel, errno=%d\n",
			   Mypid, errno);
			FFLUSH (Debugfp);
			d_exit(fd);
		}
		(void) poptli(fd);
		if (ioctl(fd, I_PUSH, "tirdwr") < 0) {
			FPRINTF(Debugfp,
			   "(%5d) can't push TIRDWR, errno=%d\n",
			   Mypid,errno);
			FFLUSH (Debugfp);
			d_exit(fd);
		}
		if (ioctl(sfd, I_SENDFD, fd) < 0) {
			FPRINTF(Debugfp,
			   "(%5d) can't I_SENDFD fd to ns, errno=%d\n",
			   Mypid,errno);
			FFLUSH (Debugfp);
			d_exit(fd);
		}
		exit(0);
		/* NOTREACHED */
	case RF_RF:
	/* read token of client, along with rest of negotiation data
	 * here we would also read specific du opcode and switch
	 * (i.e., case MNT)
	 */
		if ((gdpmisc.version =
		    negotiate(fd, NULL, SERVER, &gdpmisc.ngroups_max,
				&gdpmisc.aclmax, &gdpmisc.mac
			)) < 0 ) {
			FPRINTF(Debugfp,"(%5d) negotiations failed\n", Mypid);
			FFLUSH (Debugfp);
			d_exit(fd);
		}
		gdpmisc.hetero = ndata.n_hetero;
		ndata.n_token.t_id = SERVER;
		(void) poptli(fd);
		if (rfsys(RF_FWFD, fd, &ndata.n_token, &gdpmisc) <0) {
			FPRINTF(Debugfp,
			   "(%5d) can't do FWFD, errno=%d\n",Mypid,errno);
			FFLUSH (Debugfp);
			d_exit(fd);
		}
		uidmap(0, (char *)NULL, (char *)NULL, &ndata.n_netname[0], 0);
		uidmap(1, (char *)NULL, (char *)NULL, &ndata.n_netname[0], 0);
		exit(0);
	default:
		FPRINTF(Debugfp,"(%5d) invalid opcode %d\n",Mypid,op);
		FFLUSH (Debugfp);
		exit(1);
	} /* end switch */
	/* NOTREACHED*/
}

int
banter(fd)
	int	fd;
{
	int	len;
	pnhdr_t	rpnhdr;

	/*
	 * get header from client - 
	 *
	 * format of header is	4 char opcode
	 *			canon long hi version
	 *			canon long lo version
	 */

	if ((len =rf_rcv(fd, canonbuf, CANON_CLEN, &flags)) != CANON_CLEN) {
		FPRINTF(Debugfp,
			"(%5d) t_rcv opcode failed, t_errno=%d, errno=%d\n",
			Mypid,t_errno,errno);
		if (len != CANON_CLEN)
			FPRINTF(Debugfp, "(%5d) t_rcv only returned %d bytes\n",
				Mypid, len);
		FFLUSH (Debugfp);
		return -1;
	}
	if (fcanon(CANONSTR, canonbuf, &pnhdr) == 0) {
		FPRINTF(Debugfp,"(%5d) fcanon returned 0\n",Mypid);
		FFLUSH (Debugfp);
		return -1;
	}
	/* check on version mapping */
	if((pnhdr.pn_lo > HI_VER) || (pnhdr.pn_hi < LO_VER )) {
		FPRINTF(Debugfp,
		   "(%5d) bad version local hi=%d lo=%d, rem hi=%d,lo=%d\n",
		   Mypid,HI_VER,LO_VER,pnhdr.pn_lo,pnhdr.pn_hi);
		FFLUSH (Debugfp);
		rpnhdr.pn_hi = -1;
	} else  {
		if(pnhdr.pn_hi < HI_VER)
			rpnhdr.pn_lo = rpnhdr.pn_hi = pnhdr.pn_hi;
		else
			rpnhdr.pn_lo = rpnhdr.pn_hi = HI_VER;
	}
	op = get_opcode(&pnhdr);
	logopcode();
	if (op < 0) {
		FPRINTF(Debugfp, "(%5d) invalid opcode\n",Mypid);
		FFLUSH (Debugfp);
		return -1;
	}
	/* send back version we are talking in */
	(void)strncpy(&rpnhdr.pn_op[0],sw_tab[RF_AK].sw_opcode,OPCODLEN);
	if ((len = tcanon(CANONSTR,&rpnhdr, canonbuf)) == 0) {
		FPRINTF(Debugfp,"(%5d) version fcanon returned 0\n",Mypid);
		FFLUSH (Debugfp);
		return -1;
	}
	if (t_snd(fd, canonbuf, len, 0) != len) {
		FPRINTF(Debugfp,
		    "(%5d) response t_snd failed, t_errno=%d, errno=%d\n",
		    Mypid,t_errno,errno);
		FFLUSH (Debugfp);
		return -1;
	}
	return rpnhdr.pn_hi;
}

int
get_opcode(ptr)
	pnhdr_t *ptr;
{
	int i;

	/* check on version mapping */
	for (i = 0; i < NUMSWENT; i++){
		if (strcmp(ptr->pn_op, sw_tab[i].sw_opcode) == 0)
			break;
	}
	if (i >= NUMSWENT)
		return -1;
	return sw_tab[i].sw_idx;
}

static void
d_exit(fd)
int fd;
{
	char modname[FMNAMESZ];

	FPRINTF(Debugfp,"(%5d) exit\n",Mypid);
	FFLUSH (Debugfp);
	if (ioctl(fd, I_LOOK, modname) >= 0) {
		if (strcmp(modname, TIMOD) == 0)
			(void) t_snddis(fd);
	}
	exit(2);
	/* NOTREACHED */
}

static void
logrequest()
{
	char	*addr = getenv("NLSADDR");
	char	*provider = getenv("NLSPROVIDER");

	FPRINTF(Debugfp,"(%5d) Rcvd req from addr=%s, provider=%s\n",
		Mypid,(addr)?addr:"UNK",(provider)?provider:"UNK");
	FFLUSH (Debugfp);
}

static void
logopcode()
{
	char	*opcode;
	char	errval[BUFSIZ];

	switch (op) {
	case RF_NS:
		opcode="RFS name service";
		break;
	case RF_RF:
		opcode="RFS remote mount";
		break;
	default:
		opcode=errval;
		(void)sprintf(opcode,"UNKNOWN(%d)",op);
		break;
	}
	FPRINTF(Debugfp,"(%5d) request = %s\n",Mypid,opcode);
	FFLUSH (Debugfp);
}

static void
gensetup()
{
	Mypid = getpid();
}

static void
logsetup()
{
	struct stat	sbuf;
	char		*openflag;
	extern	int	(*do_fflush) (FILE *);
	extern	int	(*do_fprintf)(FILE *, const char *, ...);

	openflag =	stat(DEBUG_FILE,&sbuf) == 0	/* got file info */
		&&	sbuf.st_size <= MAX_DB_SIZE	/* not too big */
		?	"a"	/* open for append (below) */
		:	"w"	/* open for create or truncate (below)*/
		;

	/* redefine stderr to log file */
	if (freopen(DEBUG_FILE, openflag, stderr) == NULL) {
		/*
		 * logging via FPRINTF will be no-ops (per initialization)
		 * logging via fprintf will fail (silently) as well
		 */
		(void)fclose(stderr);
		return;
	}
	/* make real the logging functions behind FPRINTF & FFLUSH */
	do_fflush  = fflush;
	do_fprintf = fprintf;
}

/* dummy functions for FPRINTF and FFLUSH sans logging */
/* ARGSUSED */
static null_fflush( FILE *fp)
{
	return 0;
}
/* ARGSUSED */
static	int	null_fprintf (FILE * fp, const char *format, ...)
{
	return 0;
}

static int
poptli(fd)
int fd;
{
	char modname[FMNAMESZ+1];

	if (ioctl(fd, I_LOOK, modname) >= 0) {
		if (strcmp(modname, TIMOD) == 0) {
			if (ioctl(fd, I_POP, 0) < 0) {
				FPRINTF(Debugfp,
					"(%5d) can't pop TIMOD, errno=%d\n",
					Mypid,errno);
				FFLUSH (Debugfp);
			}
		}
	}
	return 0;
}
