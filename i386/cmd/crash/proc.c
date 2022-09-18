/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:i386/cmd/crash/proc.c	1.1.1.6"
#ident	"$Header: proc.c 1.1 91/07/23 $"

/*
 * This file contains code for the crash functions:  proc, defproc, hrt.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <a.out.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/sysmacros.h>
#include <sys/fs/s5dir.h>
#include <sys/cred.h>
#include <sys/user.h>
#include <sys/var.h>
#include <vm/as.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/sysi86.h>
#include <sys/hrtcntl.h>
#include <sys/hrtsys.h>
#include <sys/priocntl.h>
#include <sys/procset.h>
#include <sys/vnode.h>
#include <sys/session.h>
#include "crash.h"
#include <priv.h>
#include <sys/secsys.h>
#include <sys/mac.h>
#include <audit.h>
#include <sys/stropts.h>

extern struct user *ubp;		/* pointer to the ublock */
extern struct syment *Curproc;	/* namelist symbol pointers */
char *proc_clk[] = {
		"CLK_STD",
		"CLK_USERVIRT",
		"CLK_PROCVIRT"
};


/* token name flag for privileges and audit events */
int tokename = 0;	

/* get arguments for proc function */
int
getproc()
{
	int slot = -1;
	int all = 0;
	int full = 0;
	int phys = 0;
	int run = 0;
	int alarm = 0;
	long addr = -1;
	long arg1 = -1;
	long arg2 = -1;
	pid_t id = -1;
	int c;
	char *heading = "SLOT ST  PID  PPID  PGID   SID   UID PRI CPU   EVENT     NAME        FLAGS\n";
	char *prprocalarm_hdg = "    CLOCK       TIME     INTERVAL    CMD    EID     PREVIOUS     NEXT    \n\n";

	optind = 1;
	tokename = 0;
	while((c = getopt(argcnt,args,"efpanrw:")) !=EOF) {
		switch(c) {
			case 'a' :	alarm = 1;
					break;
			case 'e' :	all = 1;
					break;
			case 'f' :	full = 1;
					break;
			case 'w' :	redirect();
					break;
			case 'r' :	run = 1;
					break;
			case 'p' :	phys = 1;
					break;
			case 'n' :	tokename = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	fprintf(fp,"PROC TABLE SIZE = %d\n",vbuf.v_proc);
	if(!full && !alarm)
		fprintf(fp,"%s",heading);
	if(alarm)
		fprintf(fp,"%s", prprocalarm_hdg);
	if(args[optind]) {
		all = 1;
		do {
			if(*args[optind] == '#') {
				if((id = (pid_t)strcon(++args[optind],'d')) == -1)
					error("\n");
				prproc(all,full,slot,id,tokename,phys,run,alarm,addr,heading);
			}
			else {
				getargs(vbuf.v_proc,&arg1,&arg2);
				if(arg1 == -1) 
					continue;
				if(arg2 != -1)
					for(slot = arg1; slot <= arg2; slot++)
						prproc(all,full,slot,id,tokename,phys,
							run,alarm,addr,heading);
				else {
					if((arg1 < vbuf.v_proc) && (arg1 >= 0))
						slot = arg1;
					else
						addr = arg1;
					prproc(all,full,slot,id,tokename,phys,run,alarm,
					    addr,heading);
				}
			}
			id = slot = addr = arg1 = arg2 = -1;
		}while(args[++optind]);
	}
	else{
		/*
		 * Create a new slot table to
		 * reflect the current state
		 */
		if(active)
			makeslottab();

		for(slot = 0; slot < vbuf.v_proc; slot++)
			prproc(all,full,slot,id,tokename,
				phys,run,alarm,addr,heading);
	}
}


/* print proc table */
int
prproc(all,full,slot,id,symprv,phys,run,alarm,addr,heading)
int all,full,slot,phys,run,alarm,symprv;
pid_t id;
long addr;
char *heading;
{
	char ch,*typ;
	char cp[PSCOMSIZ+1];
	struct proc procbuf, *procaddr;
	struct cred uc;
	struct sess sess;
	struct aproc ac;
	struct adtc cwd;
	register struct tm * td;
	char *evtstr, *cwdp;	
	extern char	*cnvemask();
	int i,j,cnt,seqnum;
	extern long lseek();
	timer_t *hrp;
	timer_t hrtbuf;
	char buf[40];
	int type = 0;
	void	pr_privs();
	proc_t *slot_to_proc();
	pte_t ubptbl[MAXUSIZE];

	if(id != -1) {
		for(slot = 0; ; slot++) {
			if(slot == vbuf.v_proc) {
				fprintf(fp,"%d not valid process id\n",id);
				return;
			}
			if (slot_to_pid(slot) == id) {
				procaddr = slot_to_proc(slot);
				break;
			}
		}
	} else if (slot != -1) 
		procaddr = slot_to_proc(slot);
	else for(slot = 0; ; slot++) {
		if(slot == vbuf.v_proc) {
			fprintf(fp,"%x not valid process address\n", addr);
			return;
		}
		procaddr = slot_to_proc(slot);
		if (phys) {
			if (addr==(vtop(procaddr,-1)))
				break;
		} else {
			if(addr==(long)procaddr)
				break;
		}
	}
	if (!procaddr)
		return;

	readmem((long)procaddr,!phys,-1,
		    (char *)&procbuf,sizeof procbuf,"proc table");

	if(run)
		if(!(procbuf.p_stat == SRUN || procbuf.p_stat == SONPROC))
			return;

	if (alarm) {
		while (type < 2) {
			hrp=procbuf.p_italarm[type];
			for(; hrp!=NULL; hrp=hrtbuf.hrt_next) {
				readmem((long)hrp, 1, -1, (char *)&hrtbuf,
					sizeof hrtbuf, "process alarm");
				fprintf(fp, "%s %7d %11d %7d %13x %10x\n",
					proc_clk[type + 1],
					hrtbuf.hrt_time,
					hrtbuf.hrt_int,
					hrtbuf.hrt_cmd,
					hrtbuf.hrt_prev,
					hrtbuf.hrt_next);
			}
			type++;
		}
		return;
	}

	if(full)
		fprintf(fp,"%s",heading);
	switch(procbuf.p_stat) {
	case NULL:   ch = ' '; break;
	case SSLEEP: ch = 's'; break;
	case SRUN:   ch = 'r'; break;
	case SIDL:   ch = 'i'; break;
	case SZOMB:  ch = 'z'; break;
	case SSTOP:  ch = 't'; break;
	case SONPROC:  ch = 'p'; break;
	case SXBRK:  ch = 'x'; break;
	default:     ch = '?'; break;
	}
	if(slot == -1)
		fprintf(fp,"  - ");
	else fprintf(fp,"%4d",slot);
	fprintf(fp," %c %5u %5u %5u %5u %5u  %2u %3u",
		ch,
		slot_to_pid(slot),
		procbuf.p_ppid,
		readpid(procbuf.p_pgidp),
		readsid(procbuf.p_sessp),
		procbuf.p_uid,
		procbuf.p_pri,
		procbuf.p_cpu);
	if(procbuf.p_stat == SONPROC)
		fprintf(fp,"          ");
	else fprintf(fp," %08lx ",procbuf.p_wchan);
	for(i = 0; i < PSCOMSIZ+1; i++)
		cp[i] = '\0';
	if(procbuf.p_stat == SZOMB)
		strcpy(cp,"zombie");
	else
	if(active){
		if(sysi86(RDUBLK, slot_to_pid(slot),
			 (char *)ubp,sizeof(struct user)))
			strncpy(cp, ubp->u_comm, PSCOMSIZ);
	}	
	else if(!(procbuf.p_flag & SULOAD))
		strcpy(cp, "swapped");
	else
	{
		if(procbuf.p_ubptbl == 0){
			strcpy(cp,"0?");
			goto uerr;
		}
		readmem(procbuf.p_ubptbl,1,-1,ubptbl,sizeof(ubptbl),
			"ublock page table entries");

		for(i = 0, cnt = 0; cnt < (USIZE * NBPC); cnt += NBPC, i++){
			/*
			 * Seek from beginning of memory to ith page
			 * of uarea
			 */
			if(!ubptbl[i].pgm.pg_v) {
				sprintf(cp,"0x%08x?",ubptbl[i].pgm.pg_pfn);
				goto uerr;
			}
			readmem(ctob(ubptbl[i].pgm.pg_pfn),0,0,
				(char *)ubp + cnt,NBPC, "ublock (in Proc)");
		}
		strncpy(cp,ubp->u_comm, PSCOMSIZ);
	}
	for(i = 0; i < 8 && cp[i]; i++) {
		if(cp[i] < 040 || cp[i] > 0176) {
			strcpy(cp,"unprint");
			break;
		}
	}

uerr:
	fprintf(fp,"%-14s", cp);
	fprintf(fp,"%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
		procbuf.p_flag & SLOAD ? " load" : "",
		(procbuf.p_flag & (SLOAD|SULOAD)) == SULOAD ? " uload" : "",
		procbuf.p_flag & SSYS ? " sys" : "",
		procbuf.p_flag & SLOCK ? " lock" : "",
		procbuf.p_flag & STRC ? " trc" : "",
 		procbuf.p_flag & SNWAKE ? " nwak" : "",
 		procbuf.p_flag & SPOLL ? " poll" : "",
 		procbuf.p_flag & SPRSTOP ? " prst" : "",
 		procbuf.p_flag & SPROCTR ? " prtr" : "",
 		procbuf.p_flag & SPROCIO ? " prio" : "",
 		procbuf.p_flag & SPRFORK ? " prfo" : "",
 		procbuf.p_flag & SPROPEN ? " prop" : "",
		procbuf.p_flag & SRUNLCL ? " runl" : "",
		procbuf.p_flag & SNOSTOP ? " nstp" : "",
		procbuf.p_flag & SPTRX ? " ptrx" : "",
		procbuf.p_flag & SASLEEP ? " aslp" : "",
		procbuf.p_flag & SUSWAP ? " uswp" : "",
		procbuf.p_flag & SNOWAIT ? " nowait" : "",
		procbuf.p_flag & SJCTL ? " jctl" : "",
		procbuf.p_flag & SVFORK ? " vfrk" : "",
		procbuf.p_flag & SSWLOCKS ? " swlk" : "",
		procbuf.p_flag & SXSTART ? " xstr" : "",
		procbuf.p_flag & SPSTART ? " pstr" : "");
	if(!full)
		return;

	readmem((long)procbuf.p_sessp,1,-1,(char *)&sess,sizeof sess,
		"session");
	fprintf(fp,"\tSession: ");
	fprintf(fp,"sid: %u, ctty: ", readsid(procbuf.p_sessp));
	if (sess.s_vp)
		fprintf(fp,"vnode(%x) maj(%4u) min(%5u)\n",
			sess.s_vp, getemajor(sess.s_dev), geteminor(sess.s_dev));
	else 
		fprintf(fp,"-\n");

	readmem((long)procbuf.p_cred,1,-1,(char *)&uc,sizeof uc,
		"process credentials");
	fprintf(fp,"\tProcess Credentials: ");
	fprintf(fp,"uid: %u, gid: %u, real uid: %u, real gid: %u lid: %x\n",
		uc.cr_uid,
		uc.cr_gid,
		uc.cr_ruid,
		uc.cr_rgid,
		uc.cr_lid);
	fprintf(fp, "\tProcess Privileges: ");
	pr_privs(uc, symprv);
 	fprintf(fp,"\tas: %x\n",
		procbuf.p_as);
 	fprintf(fp,"\twait code: %x, wait data: %x\n",
		procbuf.p_wcode,
		procbuf.p_wdata);
	fprintf(fp,"\tsig: %x, cursig: %d\n",
		procbuf.p_sig,
		procbuf.p_cursig);
	fprintf(fp,"\tlink: %x\tparent: %x\tchild: %x\tsibling: %x\n",
		procbuf.p_link,
		procbuf.p_parent,
		procbuf.p_child,
		procbuf.p_sibling);
	if(procbuf.p_link)
		fprintf(fp,"\tlink: %d\n", proc_to_slot(procbuf.p_link));
	fprintf(fp,"\tutime: %ld\tstime: %ld\tcutime: %ld\tcstime: %ld\n",
		procbuf.p_utime,procbuf.p_stime,procbuf.p_cutime,procbuf.p_cstime);
	fprintf(fp, "\tubptbl:  ");
	if((procbuf.p_flag & SULOAD) && procbuf.p_ubptbl){
		for(i=0, j=1; i<MAXUSIZE; i++)
			if(ubptbl[i].pgm.pg_v){
				if(!(j++ & 3))
					fprintf(fp,"\n\t");
				fprintf(fp,"%d: %8x   ",i,ubptbl[i].pg_pte);
			}
	}
	fprintf(fp,"\n\tepid: %d, sysid: %x, rlink: %x\n",
		procbuf.p_epid,
		procbuf.p_sysid,
		procbuf.p_rlink);
	fprintf(fp,"\tsrwchan: %d, trace: %x, sigmask: %x,",
		procbuf.p_srwchan,
		procbuf.p_trace,
		procbuf.p_sigmask);
	fprintf(fp,"\thold: %x\n",
		procbuf.p_hold);
	fprintf(fp, "\twhystop: %d, whatstop: %d\n",
		procbuf.p_whystop,
		procbuf.p_whatstop);
	fprintf(fp, "\talarmid: %d, alarmtime: %d\n",
		procbuf.p_alarmid,
		procbuf.p_alarmtime);

	/* print class information */

	fprintf(fp, "\tclass: %d, clfuncs: %x\n", 
 		procbuf.p_cid, procbuf.p_clfuncs);
	fprintf(fp, "\tclproc: %x\n", procbuf.p_clproc);

	/* print asyncio information */

	fprintf(fp, "\taiocount: %d, aiowcnt: %d\n",
		procbuf.p_aiocount, procbuf.p_aiowcnt);


	if(procbuf.p_aprocp == NULL) 
		fprintf(fp,"\tproc: auditing disabled - no process audit structure\n");
	else {
		readmem((long)procbuf.p_aprocp,1,-1,
		    (char *)&ac, sizeof ac,"audit process structure");
		
		readmem((long)ac.a_cwd, 1, -1, (char *)&cwd, sizeof cwd, 
			"audit current working directory structure");

		if ((cwdp = ((char *)malloc(cwd.a_len+1))) ==  NULL)
			exit(ADT_MALLOC);
		memset(cwdp,'\0',(cwd.a_len+1));
		readmem((long)cwd.a_path, 1, -1, (char *)cwdp, cwd.a_len, 
			"audit current working directory string");

		fprintf(fp,"\tCurrent Working Directory: %s\n",cwdp); 
		if (tokename) {
			if ((evtstr=(char *)prevtnam(ac.a_event))!=NULL)
				fprintf(fp,"\tEvent Number: %s\n",
					evtstr);
			else
				fprintf(fp,"\tEvent Number NULL: ERROR\n");
			seqnum=EXTRACTSEQ(ac.a_seqnum);
			fprintf(fp,"\tRecord Sequence Number: %u\n",seqnum);

			if (ac.a_flags > 0) 
				fprintf(fp,"\tFlags: %s %s\n",
				ac.a_flags & AUDITME ? "AUDITME," : "",
				ac.a_flags & AEXEMPT ? "AEXEMPT," : "");
			else
				fprintf(fp,"\tFlags: NONE\n");

			if (ac.a_time.tv_sec == 0)
				fprintf(fp,"\tStarting time of Event: 0\n");
			else {
				td = gmtime((const time_t *)&ac.a_time);
				fprintf(fp,"\tStarting time of Event: %02d/%02d/%02d%  02d:%02d:%02d GMT\n",
	                                td->tm_mon + 1,
	                                td->tm_mday,
	                                td->tm_year,
	                                td->tm_hour,
	                                td->tm_min,
                                	td->tm_sec);
			}
			if ((evtstr=(char *)cnvemask(&ac.a_procemask)) != NULL)
				fprintf(fp,"\tProcess Event Mask:\t%s\n",evtstr);
			else
				fprintf(fp,"\tProcess Event Mask:\tEVTSTR=NULL ERROR\n");
			if ((evtstr=(char *)cnvemask(&ac.a_useremask)) != NULL)
				fprintf(fp,"\tUser Event Mask:\t%s\n",evtstr);
			else
				fprintf(fp,"\tUser Event Mask:\tEVTSTR=NULL ERROR\n");
		}else  {
			fprintf(fp,"\tEvent Number: %u\n",ac.a_event);
			seqnum=EXTRACTSEQ(ac.a_seqnum);
			fprintf(fp,"\tRecord Sequence Number: %u\n",seqnum);
			fprintf(fp,"\tFlags: %u\n",ac.a_flags);
			fprintf(fp,"\tStarting time of Event: %ld\n",ac.a_time);
			fprintf(fp,"\tProcess Event Mask:\t%08lx %08lx %08lx %08lx\n",
				ac.a_procemask[0],
				ac.a_procemask[1],
				ac.a_procemask[2],
				ac.a_procemask[3]);
			fprintf(fp,"\tUser Event Mask:\t%08lx %08lx %08lx %08lx\n",
				ac.a_useremask[0],
				ac.a_useremask[1],
				ac.a_useremask[2],
				ac.a_useremask[3]);
		}
	}
	fprintf(fp,"\n");
}


/* get arguments for defproc function */
int
getdefproc()
{
	int c;
	int proc = -1;
	int reset = 0;

	optind = 1;
	while((c = getopt(argcnt,args,"cw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'c' :	reset = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) 
		if((proc = strcon(args[optind],'d')) == -1)
			error("\n");
	prdefproc(proc,reset);
}

/* print results of defproc function */
int
prdefproc(proc,reset)
int proc,reset;
{

	if(reset)
		Procslot = getcurproc();
	else if(proc > -1) {
		if((proc > vbuf.v_proc) || (proc < 0))
			error("%d out of range\n",proc);
		Procslot = proc;
	}
	fprintf(fp,"Procslot = %d\n",Procslot);
}

/* print the high resolution timers */
int
gethrt()
{
	int c;
	static struct syment *Hrt;
	timer_t hrtbuf;
	timer_t *hrp;
	extern timer_t hrt_active;
	char *prhralarm_hdg = "    PROCP       TIME     INTERVAL    CMD    EID     PREVIOUS     NEXT    \n\n";


	optind = 1;
	while((c=getopt(argcnt, args,"w:")) != EOF) {
		switch(c) {
			case 'w'  :	redirect();
					break;
			default   :	longjmp(syn,0);
		}
	}

	if (!(Hrt = symsrch("hrt_active")))
		fatal("hrt_active not found in symbol table\n");


	readmem ((long)Hrt->n_value, 1, -1, (char *)&hrtbuf,
		sizeof hrtbuf, "high resolution alarms");

	fprintf(fp, "%s", prhralarm_hdg);
	hrp=hrtbuf.hrt_next;
	for (; hrp != (timer_t *)Hrt->n_value; hrp=hrtbuf.hrt_next) {
		readmem((long)hrp, 1, -1, (char *)&hrtbuf,
			sizeof hrtbuf, "high resolution alarms");
		fprintf(fp, "%12x %7d%11d %7d %13x %10x\n",
			hrtbuf.hrt_proc,
			hrtbuf.hrt_time,
			hrtbuf.hrt_int,
			hrtbuf.hrt_cmd,
			hrtbuf.hrt_prev,
			hrtbuf.hrt_next);
	}
}

int
readsid(sessp)
	struct sess *sessp;
{
	struct sess s;

	readmem((char *)sessp,1,getcurproc(),(char *)&s,sizeof(struct sess),
		"session structure");

	return readpid(s.s_sidp);
}

int
readpid(pidp)
	struct pid *pidp;
{
	struct pid p;

	readmem((char *)pidp,1,getcurproc(),(char *)&p,sizeof(struct pid),
		"pid structure");

	return p.pid_id;
}


static	void
pr_privs(lst, symb)
cred_t	lst;
register int	symb;
{
	extern	void	prt_symbprvs();

	if (symb) {
		prt_symbprvs("\n\t\tworking: ", lst.cr_wkgpriv);
		prt_symbprvs("\t\tmaximum: ", lst.cr_maxpriv);
	}
	else {
		fprintf(fp, "working: %.8x", lst.cr_wkgpriv);
		fprintf(fp, "\tmaximum: %.8x", lst.cr_maxpriv);
		fprintf(fp, "\n");
	}
}

runq()
{
	fprintf(fp,"runq not yet implemented\n");
}
