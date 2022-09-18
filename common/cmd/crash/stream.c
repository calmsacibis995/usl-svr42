/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:common/cmd/crash/stream.c	1.15.23.3"
#ident	"$Header: stream.c 1.1 91/07/23 $"

/*
 * This file contains code for the crash functions:  stream, queue, 
 * strstat, linkblk, qrun.
 */

#include <sys/param.h>
#include <a.out.h>
#include <stdio.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/var.h>
#include <sys/proc.h>
#include <sys/fstyp.h>
#include <sys/fsid.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/list.h>
#include <sys/rf_messg.h>
#include <sys/rf_comm.h>
#include <sys/vfs.h>
#define _KERNEL
#include <sys/poll.h>
#undef _KERNEL
#include <sys/stream.h>
#define DEBUG
#include <sys/strsubr.h>
#undef DEBUG
#include <sys/strstat.h>
#include <sys/stropts.h>
#include "crash.h"

int	_streams_crash;
static struct syment *Strinfop, *Strst, *Qhead, *_streams_crashp;
extern struct syment *Vnode, *Streams;
struct dbinfo *prdblk();
struct dbinfo *realprdblk();
struct mbinfo *prmess();
struct mbinfo *realprmess();
struct shinfo *prstream();
struct linkinfo *prlinkblk();


/* get arguments for stream function */
int
getstream()
{
	int all = 0;
	int full = 0;
	int phys = 0;
	long addr = -1;
	int c;
	register int i;
	struct shinfo *sp;
	struct strinfo strinfo[NDYNAMIC];
 	char *heading = " ADDRESS      WRQ     IOCB    VNODE  PUSHCNT  RERR/WERR FLAG\n";

	optind = 1;
	while((c = getopt(argcnt,args,"efpw:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'f' :	full = 1;
					break;
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	if (!Strinfop)
		if(!(Strinfop = symsrch("Strinfo")))
			error("Strinfo not found in symbol table\n");
	readmem((long)Strinfop->n_value,1,-1,(char *)&strinfo,
		sizeof(strinfo),"Strinfo");
	fprintf(fp,"STREAM TABLE SIZE = %d\n",strinfo[DYN_STREAM].sd_cnt);
	if(!full)
		fprintf(fp,"%s",heading);
	if(args[optind]) {
		all = 1;
		do {
			if((addr = strcon(args[optind++], 'h')) == -1)
				error("\n");
			(void) prstream(all,full,addr,phys,heading);
		}while(args[++optind]);
	}
	else {
		sp = (struct shinfo *) strinfo[DYN_STREAM].sd_head;
		while(sp) {
			sp = prstream(all,full,sp,phys,heading);
		}
	}
}

/* print streams table */
struct shinfo *
prstream(all,full,addr,phys,heading)
int all,full,phys;
long addr;
char *heading;
{
	struct shinfo strm;
	register struct stdata *stp;
	struct strevent evbuf;
	struct strevent *next;
	struct polldat *pdp;
	struct polldat pdat;

	readmem(addr,1,-1,(char *)&strm,sizeof(strm),"streams table slot");
	stp = (struct stdata *) &strm;
	if (!stp->sd_wrq && !all) 
		return(NULL);
	if(full)
		fprintf(fp,"%s",heading);
	fprintf(fp,"%8x %8x %8x %8x %6d %d/%d",addr,stp->sd_wrq,stp->sd_strtab,
	    stp->sd_vnode,stp->sd_pushcnt,stp->sd_rerror,stp->sd_werror);
	fprintf(fp,"%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
		((stp->sd_flag & IOCWAIT) ? "iocw " : ""),
		((stp->sd_flag & RSLEEP) ? "rslp " : ""),
		((stp->sd_flag & WSLEEP) ? "wslp " : ""),
		((stp->sd_flag & STRPRI) ? "pri " : ""),
		((stp->sd_flag & STRHUP) ? "hup " : ""),
		((stp->sd_flag & STWOPEN) ? "stwo " : ""),
		((stp->sd_flag & STPLEX) ? "plex " : ""),
		((stp->sd_flag & STRISTTY) ? "istty " : ""),
		((stp->sd_flag & RMSGDIS) ? "mdis " : ""),
		((stp->sd_flag & RMSGNODIS) ? "mnds " : ""),
		((stp->sd_flag & STRDERR) ? "rerr " : ""),
		((stp->sd_flag & STWRERR) ? "werr " : ""),
		((stp->sd_flag & STRTIME) ? "sttm " : ""),
		((stp->sd_flag & STR2TIME) ? "s2tm " : ""),
		((stp->sd_flag & STR3TIME) ? "s3tm " : ""),
		((stp->sd_flag & STRCLOSE) ? "clos " : ""),
		((stp->sd_flag & SNDMREAD) ? "mrd " : ""),
		((stp->sd_flag & OLDNDELAY) ? "ondel " : ""),
		((stp->sd_flag & RDBUFWAIT) ? "rdbfw " : ""),
		((stp->sd_flag & SNDZERO) ? "sndz " : ""),
		((stp->sd_flag & STRTOSTOP) ? "tstp " : ""),
		((stp->sd_flag & RDPROTDAT) ? "pdat " : ""),
		((stp->sd_flag & RDPROTDIS) ? "pdis " : ""),
		((stp->sd_flag & STRMOUNT) ? "mnt " : ""),
		((stp->sd_flag & STRDELIM) ? "delim " : ""),
		((stp->sd_flag & STRSIGPIPE) ? "spip " : ""));

	if(full) {
		fprintf(fp,"\t     SID     PGID   IOCBLK    IOCID  IOCWAIT\n");
		fprintf(fp,"\t%8d %8d %8x %8d %8d\n",
			readpid(stp->sd_sidp),
			readpid(stp->sd_pgidp),
			stp->sd_iocblk,stp->sd_iocid,stp->sd_iocwait);
		fprintf(fp,"\t  WOFF     MARK CLOSTIME\n");
		fprintf(fp,"\t%6d %8x %8d\n",stp->sd_wroff,stp->sd_mark,
		    stp->sd_closetime);
		fprintf(fp,"\tSIGFLAGS:  %s%s%s%s%s%s%s%s%s\n",
			((stp->sd_sigflags & S_INPUT) ? " input" : ""),
			((stp->sd_sigflags & S_HIPRI) ? " hipri" : ""),
			((stp->sd_sigflags & S_OUTPUT) ? " output" : ""),
			((stp->sd_sigflags & S_RDNORM) ? " rdnorm" : ""),
			((stp->sd_sigflags & S_RDBAND) ? " rdband" : ""),
			((stp->sd_sigflags & S_WRBAND) ? " wrband" : ""),
			((stp->sd_sigflags & S_ERROR) ? " err" : ""),
			((stp->sd_sigflags & S_HANGUP) ? " hup" : ""),
			((stp->sd_sigflags & S_MSG) ? " msg" : ""));
		fprintf(fp,"\tSIGLIST:\n");
		next = stp->sd_siglist;
		while(next) {
			readmem((long)next,1,-1,(char *)&evbuf,
				sizeof evbuf,"stream event buffer");
			fprintf(fp,"\t\tPROC:  %3d   %s%s%s%s%s%s%s%s%s\n",
				proc_to_slot((long)evbuf.se_procp),
				((evbuf.se_events & S_INPUT) ? " input" : ""),
				((evbuf.se_events & S_HIPRI) ? " hipri" : ""),
				((evbuf.se_events & S_OUTPUT) ? " output" : ""),
				((evbuf.se_events & S_RDNORM) ? " rdnorm" : ""),
				((evbuf.se_events & S_RDBAND) ? " rdband" : ""),
				((evbuf.se_events & S_WRBAND) ? " wrband" : ""),
				((evbuf.se_events & S_ERROR) ? " err" : ""),
				((evbuf.se_events & S_HANGUP) ? " hup" : ""),
				((evbuf.se_events & S_MSG) ? " msg" : ""));
			next = evbuf.se_next;	
		}
		fprintf(fp,"\tPOLLFLAGS:  %s%s%s%s%s%s\n",
			((stp->sd_pollist.ph_events & POLLIN) ? " in" : ""),
			((stp->sd_pollist.ph_events & POLLPRI) ? " pri" : ""),
			((stp->sd_pollist.ph_events & POLLOUT) ? " out" : ""),
			((stp->sd_pollist.ph_events & POLLRDNORM) ? " rdnorm" : ""),
			((stp->sd_pollist.ph_events & POLLRDBAND) ? " rdband" : ""),
			((stp->sd_pollist.ph_events & POLLWRBAND) ? " wrband" : ""));
		fprintf(fp,"\tPOLLIST:\n");
		pdp = stp->sd_pollist.ph_list;
		while(pdp) {
			readmem((long)pdp,1,-1,(char *)&pdat,
				sizeof pdat,"poll data buffer");
			fprintf(fp,"\t\tFUNC:  %#.8x   ARG:  %#.8x   %s%s%s%s%s%s\n",
				pdat.pd_fn, pdat.pd_arg,
				((pdat.pd_events & POLLIN) ? " in" : ""),
				((pdat.pd_events & POLLPRI) ? " pri" : ""),
				((pdat.pd_events & POLLOUT) ? " out" : ""),
				((pdat.pd_events & POLLRDNORM) ? " rdnorm" : ""),
				((pdat.pd_events & POLLRDBAND) ? " rdband" : ""),
				((pdat.pd_events & POLLWRBAND) ? " wrband" : ""));
			pdp = pdat.pd_next;
		}
		fprintf(fp,"\n");
	}
	return(strm.sh_next);
}

char *qheading = " QUEADDR     INFO     NEXT     LINK      PTR     RCNT FLAG\n";

/* get arguments for queue function */
int
getqueue()
{
	int all = 0;
	int phys = 0;
	long addr = -1;
	int c;
	register int i;
	struct queinfo *qip;
	struct queinfo queinfo;
	struct strinfo strinfo[NDYNAMIC];
	int full = 0;

	optind = 1;
	while((c = getopt(argcnt,args,"efpw:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			case 'f' :	full = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	if (!Strinfop)
		if(!(Strinfop = symsrch("Strinfo")))
			error("Strinfo not found in symbol table\n");
	readmem((long)Strinfop->n_value,1,-1,(char *)&strinfo,
		sizeof(strinfo),"Strinfo");
	fprintf(fp,"QUEUE TABLE SIZE = %d\n",strinfo[DYN_QUEUE].sd_cnt);
	if(args[optind]) {
		all = 1;
		if (!full)
			fprintf(fp, qheading);
		do {
			if((addr = strcon(args[optind++], 'h')) == -1)
				error("\n");
			prqueue(all,addr,NULL,NULL,NULL,phys,full);
		}while(args[optind]);
	}
	else {
		if (!full)
			fprintf(fp, qheading);
		qip = (struct queinfo *) strinfo[DYN_QUEUE].sd_head;
		while(qip) {
			readmem((long)qip,1,-1,&queinfo,sizeof(queinfo),"queue");
#ifdef _STYPES
			prqueue(all,qip,queinfo.qu_rqueue.q_eq,&queinfo.qu_rqueue,&queinfo.qu_requeue,phys,full);
			prqueue(all,(long)qip+sizeof(queue_t),queinfo.qu_wqueue.q_eq,&queinfo.qu_wqueue,&queinfo.qu_wequeue,phys,full);
#else
			prqueue(all,qip,NULL,&queinfo.qu_rqueue,NULL,phys,full);
			prqueue(all,(long)qip+sizeof(queue_t),NULL,&queinfo.qu_wqueue,NULL,phys,full);
#endif
			qip = queinfo.qu_next;
		}
	}
}


/* print queue table */
int
prqueue(all,qaddr,eqaddr,uqaddr,ueqaddr,phys,full)
int all,phys,full;
long qaddr,eqaddr;
long uqaddr,ueqaddr;
{
	register queue_t *qp;
	queue_t q;

#ifdef _STYPES
	register struct equeue *eqp;
	struct equeue eq;
#endif


	if (uqaddr) {
		qp = (queue_t *)uqaddr;
	} else {
		readmem((long)qaddr,1,-1,&q, sizeof(q),"queue");
		qp = &q;
	}
	if (!(qp->q_flag & QUSE) && !all)
		return;

#ifdef _STYPES
	if (ueqaddr) {
		eqp = (struct equeue *)ueqaddr;
	} else {
		readmem((long)qp->q_eq,1,-1,&eq, sizeof(eq),"equeue");
		eqp = &eq;
	}
#endif

	if (full)
		fprintf(fp,qheading);
        fprintf(fp,"%8x ",qaddr);
	fprintf(fp,"%8x ",qp->q_qinfo);
	if (qp->q_next)
		fprintf(fp,"%8x ",qp->q_next);
	else fprintf(fp,"       - ");

#ifdef _STYPES
	if (eqp->eq_link)
		fprintf(fp,"%8x ",eqp->eq_link);
#else
	if (qp->q_link)
		fprintf(fp,"%8x ",qp->q_link);
#endif

	else fprintf(fp,"       - ");
	fprintf(fp,"%8x",qp->q_ptr);
	fprintf(fp," %8d ",qp->q_count);
	fprintf(fp,"%s%s%s%s%s%s%s%s\n",
		((qp->q_flag & QENAB) ? "en " : ""),
		((qp->q_flag & QWANTR) ? "wr " : ""),
		((qp->q_flag & QWANTW) ? "ww " : ""),
		((qp->q_flag & QFULL) ? "fl " : ""),
		((qp->q_flag & QREADR) ? "rr " : ""),
		((qp->q_flag & QUSE) ? "us " : ""),
		((qp->q_flag & QNOENB) ? "ne " : ""),
		((qp->q_flag & QOLD) ? "ol " : ""));
	if (!full)
		return;
	fprintf(fp,"\t    HEAD     TAIL     MINP     MAXP     HIWT     LOWT BAND BANDADDR\n");
	if (qp->q_first)
		fprintf(fp,"\t%8x ",qp->q_first);
	else fprintf(fp,"\t       - ");
	if (qp->q_last)
		fprintf(fp,"%8x ",qp->q_last);
	else fprintf(fp,"       - ");
	fprintf(fp,"%8d %8d %8d %8d ",
		qp->q_minpsz,
		qp->q_maxpsz,
		qp->q_hiwat,
		qp->q_lowat);
	fprintf(fp," %3d %8x\n\n",

#ifdef _STYPES
		eqp->eq_nband, eqp->eq_bandp);
#else
		qp->q_nband, qp->q_bandp);
#endif
}

/* get arguments for qrun function */
int
getqrun()
{
	int c;

	if(!Qhead)
		if(!(Qhead = symsrch("qhead")))
			error("qhead not found in symbol table\n");
	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	prqrun();
}

/* print qrun information */
int
prqrun()
{
	queue_t que, *q;

#ifdef _STYPES
	struct equeue eq;
#endif

	readmem((long)Qhead->n_value,1,-1,(char *)&q,sizeof(q),"qhead");
	fprintf(fp,"Queue slots scheduled for service: ");
	while (q) {
		fprintf(fp,"%8x ",q);
		readmem((long)q,1,-1,(char *)&que,
			sizeof que,"scanning queue list");

#ifdef _STYPES
		readmem((long)que.q_eq,1,-1,(char *)&eq,
			sizeof eq,"equeue");
		q = eq.eq_link;
#else
		q = que.q_link;
#endif

	}
	fprintf(fp,"\n");
}


/* initialization for namelist symbols */
int
streaminit()
{
	static int strinit = 0;

	if(strinit)
		return;
/*
 *	if(!Mbfree)
 *		if(!(Mbfree = symsrch("mbfreelist")))
 *			error("mbfreelist not found in symbol table\n");
 *	if(!Dbfree)
 *		if(!(Dbfree = symsrch("dbfreelist")))
 *			error("dbfreelist not found in symbol table\n");
 */
	if(!Strinfop)
		if(!(Strinfop = symsrch("Strinfo")))
			error("Strinfo not found in symbol table\n");
	if(!Strst)
		if(!(Strst = symsrch("strst")))
			error("strst not found in symbol table\n");
	if(!Qhead)
		if(!(Qhead = symsrch("qhead")))
			error("qhead not found in symbol table\n");

	if(!_streams_crashp)
		if (!(_streams_crashp = symsrch("_streams_crash")))
			error("_streams_crash not found in symbol table\n");

	strinit = 1;
}


/* get arguments for strstat function */
int
getstrstat()
{
	int c;

	streaminit();
	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	prstrstat();
}

/* print stream statistics */
int
prstrstat()
{
	queue_t que, *q;
	struct strstat strst;
	struct strinfo strinfo[NDYNAMIC];
	int qruncnt;

	int	m_conf, m_alloc, m_free, m_total, m_max, m_fail;

#ifdef _STYPES
	struct equeue eq;
#endif

	qruncnt = 0;

	fprintf(fp,"ITEM                  CONFIG    ALLOC    FREE         TOTAL     MAX    FAIL\n");

	readmem((long)Qhead->n_value,1,-1,(char *)&q, sizeof(q),"qhead");
	while (q) {
		qruncnt++;
		readmem((long)q,1,-1,(char *)&que,sizeof(que),"queue run list");

#ifdef _STYPES
		readmem((long)que.q_eq,1,-1,(char *)&eq,sizeof(eq),"equeue");
		q = eq.eq_link;
#else
		q = que.q_link;
#endif

	}

	readmem((long)Strinfop->n_value,1,-1,(char *)&strinfo,
		sizeof(strinfo),"Strinfo");
	
	readmem((long)Strst->n_value,1,-1, &strst, sizeof strst,"strst");

	fprintf(fp,"streams                 %4d     %4d    %4d    %10d    %4d    %4d\n",
		strinfo[DYN_STREAM].sd_cnt, strst.stream.use, strinfo[DYN_STREAM].sd_cnt - strst.stream.use, strst.stream.total, strst.stream.max, strst.stream.fail);
	fprintf(fp,"queues                  %4d     %4d    %4d    %10d    %4d    %4d\n",
		strinfo[DYN_QUEUE].sd_cnt, strst.queue.use, strinfo[DYN_QUEUE].sd_cnt - strst.queue.use, strst.queue.total, strst.queue.max, strst.queue.fail);

	m_conf = strinfo[DYN_MSGBLOCK].sd_cnt + strinfo[DYN_MDBBLOCK].sd_cnt;
	m_alloc = strst.msgblock.use + strst.mdbblock.use;
	m_free = m_conf - m_alloc;
	m_total = strst.msgblock.total + strst.mdbblock.total;
	m_max = strst.msgblock.max + strst.mdbblock.max;
	m_fail = strst.msgblock.fail + strst.mdbblock.fail;
	fprintf(fp,"message blocks          %4d     %4d    %4d    %10d    %4d    %4d\n",
m_conf, m_alloc, m_free, m_total, m_max, m_fail);
	fprintf(fp,"data blocks             %4d     %4d    %4d    %10d    %4d    %4d\n",
		strinfo[DYN_MDBBLOCK].sd_cnt, strst.mdbblock.use, strinfo[DYN_MDBBLOCK].sd_cnt - strst.mdbblock.use, strst.mdbblock.total, strst.mdbblock.max, strst.mdbblock.fail);
	fprintf(fp,"link blocks             %4d     %4d    %4d    %10d    %4d    %4d\n",
		strinfo[DYN_LINKBLK].sd_cnt, strst.linkblk.use, strinfo[DYN_LINKBLK].sd_cnt - strst.linkblk.use, strst.linkblk.total, strst.linkblk.max, strst.linkblk.fail);
	fprintf(fp,"stream events           %4d     %4d    %4d    %10d    %4d    %4d\n",
		strinfo[DYN_STREVENT].sd_cnt, strst.strevent.use, strinfo[DYN_STREVENT].sd_cnt - strst.strevent.use, strst.strevent.total, strst.strevent.max, strst.strevent.fail);
	fprintf(fp,"\nCount of scheduled queues:%4d\n", qruncnt);
}


/* get arguments for linkblk function */
int
getlinkblk()
{
	int all = 0;
	int phys = 0;
	long addr = -1;
	int c;
	register int i;
	struct linkinfo *lp;
	struct strinfo strinfo[NDYNAMIC];

	optind = 1;
	while((c = getopt(argcnt,args,"epw:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	if (!Strinfop)
		if(!(Strinfop = symsrch("Strinfo")))
			error("Strinfo not found in symbol table\n");
	readmem((long)Strinfop->n_value,1,-1,(char *)&strinfo,
		sizeof(strinfo),"Strinfo");
	fprintf(fp,"LINKBLK TABLE SIZE = %d\n",strinfo[DYN_LINKBLK].sd_cnt);
	fprintf(fp,"LBLKADDR     QTOP     QBOT FILEADDR    MUXID\n");
	if(args[optind]) {
		all = 1;
		do {
			if((addr = strcon(args[optind++], 'h')) == -1)
				error("\n");
			(void) prlinkblk(all,addr,phys);
		}while(args[optind]);
	}
	else {
		lp = (struct linkinfo *) strinfo[DYN_LINKBLK].sd_head;
		while(lp) {
			lp = prlinkblk(all,lp,phys);
		}
	}
}

/* print linkblk table */
struct linkinfo *
prlinkblk(all,addr,phys)
int all,phys;
long addr;
{
	struct linkinfo linkbuf;
	struct linkinfo *lp;

	readmem(addr,1,-1,(char *)&linkbuf,sizeof(linkbuf),"linkblk table");
	lp = &linkbuf;
	if(!lp->li_lblk.l_qbot && !all)
		return;
	fprintf(fp,"%8x", addr);
	fprintf(fp," %8x",lp->li_lblk.l_qtop);
	fprintf(fp," %8x",lp->li_lblk.l_qbot);
	fprintf(fp," %8x",lp->li_fpdown);
	if (lp->li_lblk.l_qbot)
		fprintf(fp," %8d\n",lp->li_lblk.l_index);
	else
		fprintf(fp,"        -\n");
	return(lp->li_next);
}
