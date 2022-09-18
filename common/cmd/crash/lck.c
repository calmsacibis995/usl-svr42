/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Header: lck.c 1.1 91/07/23 $"

/*
 * This file contains code for the crash function lck.
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/var.h>
#include <a.out.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/fs/s5inode.h>
#include <sys/flock.h>
#include <sys/fsinode.h>
#include "crash.h"

static struct syment *Flckinfo,*Sleeplcks;	/* namelist symbol */
extern struct syment *S_s5fshead;			/* pointers */
extern struct syment *S_sfs_fshead;			/* pointers */
extern struct syment *S_vxfs_fshead;			/* pointers */

struct procid {			/* effective and sys ids */
	pid_t epid;
	long sysid;
	int valid;
};
struct procid *procptr;		/* pointer to procid structure */
extern char *malloc();

/* get effective and sys ids into table */
int
getprocid()
{
	struct proc *prp, prbuf;
	struct pid pid;
	static int lckinit = 0;
	register i;
	proc_t *slot_to_proc();

	if(lckinit == 0) {
		procptr = (struct procid *)malloc((unsigned)
			(sizeof (struct procid) * vbuf.v_proc));
		lckinit = 1;
	}

	for (i = 0; i < vbuf.v_proc; i++) {
		prp = slot_to_proc(i);
		if (prp == NULL)
			procptr[i].valid = 0;
		else {
			readmem((long)prp,1, -1,(char *)&prbuf,sizeof (proc_t),
				"proc table");
			readmem((long)prbuf.p_pidp,1,-1,
				(char *)&pid,sizeof(struct pid), "pid table");
			procptr[i].epid = prbuf.p_epid;
			procptr[i].sysid = prbuf.p_sysid;
			procptr[i].valid = 1;
		}
	}
}

/* find process with same id and sys id */
int
findproc(pid,sysid)
pid_t pid;
short sysid;
{
	int slot;

	for (slot = 0; slot < vbuf.v_proc; slot++) 
		if ((procptr[slot].valid) &&
		    (procptr[slot].epid == pid) &&
		    (procptr[slot].sysid == sysid))
			return slot;
	return(-1);
}


/* get arguments for lck function */
int
getlcks()
{
	int slot = -1;
	int phys = 0;
	long addr = -1;
	long arg1 = -1;
	long arg2 = -1;
	int c;
	struct flckinfo infobuf;

	if(!Flckinfo)
		if(!(Flckinfo = symsrch("flckinfo")))
			error("flckinfo not found in symbol table\n");
	if(!Sleeplcks)
		if(!(Sleeplcks = symsrch("sleeplcks")))
			error("sleeplcks not found in symbol table\n");
	optind = 1;
	while((c = getopt(argcnt,args,"epw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	getprocid();
	readmem((long)Flckinfo->n_value,1,-1,(char *)&infobuf,
		sizeof infobuf,"flckinfo table");
	fprintf(fp,"\nAdministrative Info:\n");
	fprintf(fp,"Currently_in_use  Total_used\n");
	fprintf(fp,"     %5d             %5d\n\n",
		infobuf.reccnt,
		infobuf.rectot);
	if(args[optind]) {
		fprintf(fp,"TYP WHENCE      START        LEN\n\tPROC     EPID    SYSID WAIT     PREV     NEXT\n\n");
			do {
				if((addr = strcon(args[optind], 'h')) == -1)
					error("\n");
				prlcks(phys,addr);
			}while(args[++optind]);
	} else
		prilcks();
}


/* print lock information relative to s5inodes (default) */
int
prilcks()
{
	struct	filock	*slptr,fibuf;
	struct flckinfo info;
	long iptr;
	int active = 0;
	int free = 0;
	int sleep = 0;
	int next,prev;
	long addr;
	int i;
	extern struct filock *s5getifilock();
	extern struct filock *sfsgetifilock();
	extern struct filock *vxfsgetifilock();

	fprintf(fp,"Active Locks:\n");
	fprintf(fp,"      INO         TYP WHENCE      START        LEN\n\tPROC     EPID    SYSID WAIT     PREV     NEXT\n");


	GETDSYM(s5fshead,B_FALSE);
	if(S_s5fshead->n_value != 0)
		active += practlcks(S_s5fshead,s5getifilock,"s5");

	GETDSYM(sfs_fshead,B_FALSE);
	if(S_sfs_fshead->n_value != 0)
		active += practlcks(S_sfs_fshead,sfsgetifilock,"ufs/sfs");

	GETDSYM(vxfs_fshead,B_FALSE);
	if(S_vxfs_fshead->n_value != 0)
		active += vpractlcks(S_vxfs_fshead,vxfsgetifilock,"vxfs");


	fprintf(fp,"\nSleep  Locks:\n");
	fprintf(fp,"TYP WHENCE      START        LEN\n\tLPROC     EPID    SYSID BPROC     EPID    SYSID     PREV     NEXT\n");
	readmem((long)Sleeplcks->n_value,1,-1,(char *)&slptr,
		sizeof slptr,"sleep lock information table");
	while (slptr) {
		readmem((long)slptr,1,-1,(char *)&fibuf,sizeof fibuf,
			"sleep lock information table slot");
		++sleep;
		if(fibuf.set.l_type == F_RDLCK) 
			fprintf(fp," r  ");
		else if(fibuf.set.l_type == F_WRLCK) 
			fprintf(fp," w  ");
		else fprintf(fp," ?  ");
		fprintf(fp,"%6d %10ld %10ld\n\t%5d %8d %8d %5d %8d %8d %8x %8x\n\n",
			fibuf.set.l_whence,
			fibuf.set.l_start,
			fibuf.set.l_len,
			findproc(fibuf.set.l_pid,fibuf.set.l_sysid),
			fibuf.set.l_pid,
			fibuf.set.l_sysid,
			findproc(fibuf.stat.blk.pid,fibuf.stat.blk.sysid),
			fibuf.stat.blk.pid,
			fibuf.stat.blk.sysid,
			fibuf.prev,
			fibuf.next);
		slptr = fibuf.next;
	}

	fprintf(fp,"\nSummary From Actual Lists:\n");
	fprintf(fp," TOTAL    ACTIVE  SLEEP\n");
	fprintf(fp," %4d    %4d    %4d\n",
		active+sleep,
		active,
		sleep);
}    

int
prlcks(phys,addr)
int phys;
long addr;
{
	struct filock fibuf;

	readmem(addr,!phys,-1,(char *)&fibuf,sizeof fibuf,"frlock");
	fprintf(fp," %c%c%c",
	(fibuf.set.l_type == F_RDLCK) ? 'r' : ' ',
	(fibuf.set.l_type == F_WRLCK) ? 'w' : ' ',
	(fibuf.set.l_type == F_UNLCK) ? 'u' : ' ');
	fprintf(fp,"%6d %10ld %10ld\n\t%4d %8d %8d %4x %8x %8x\n",
		fibuf.set.l_whence,
		fibuf.set.l_start,
		fibuf.set.l_len,
		findproc(fibuf.set.l_pid,fibuf.set.l_sysid),
		fibuf.set.l_pid,
		fibuf.set.l_sysid,
		fibuf.stat.wakeflg,
		fibuf.prev,
		fibuf.next);
}

int
practlcks(sfsh,getifilock,itype)
struct syment *sfsh;
struct filock *getifilock();
char *itype;
{
	struct idata *fsidata;
	struct idata idata;
	unsigned long addr;
	struct filock *actptr;
	struct filock fibuf;
	int i;
	int active = 0;
	struct fshead fsh;

	readmem(sfsh->n_value,1,-1,(char *)&fsh, 
		sizeof fsh, "inode table head");
	fsidata = (struct idata *)
	    (sfsh->n_value + ((int)&fsh.f_idata - (int)&fsh));

	/* Structure Copy */
	idata = fsh.f_idata;

	while(idata.id_next != fsidata) {
		addr = (long)idata.id_next;
		readmem((long)addr,1,-1,(char *)&idata, sizeof idata,
		    "inode idata");
		addr += sizeof idata;
		for(i = 0; i < idata.id_total; i++) {

			actptr = getifilock(addr);
			addr += fsh.f_isize; 
	
			while(actptr) {
				readmem((long)actptr,1,-1,(char *)&fibuf,
					sizeof fibuf,"filock information");
				++active;
				fprintf(fp,"%x(%-7s) ",addr,itype);
				if(fibuf.set.l_type == F_RDLCK) 
					fprintf(fp," r  ");
				else if(fibuf.set.l_type == F_WRLCK) 
					fprintf(fp," w  ");
				else fprintf(fp," ?  ");
				fprintf(fp,"%6d %10ld %10ld\n\t%4d %8d %8d %4x %8x %8x\n\n",
					fibuf.set.l_whence,
					fibuf.set.l_start,
					fibuf.set.l_len,
					findproc(fibuf.set.l_pid,fibuf.set.l_sysid),
					fibuf.set.l_pid,
					fibuf.set.l_sysid,
					fibuf.stat.wakeflg,
					fibuf.prev,
					fibuf.next);
				actptr = fibuf.next;
	
			}
		}
	}
	return(active);
}

int
vpractlcks(sfsh,getifilock,itype)
struct syment *sfsh;
struct filock *getifilock();
char *itype;
{
	struct filock *actptr;
	struct filock fibuf;
	struct fshead fsh;
	caddr_t *vxfs_iptrs;
	int i;
	int active = 0;

	readmem(sfsh->n_value,1,-1,(char *)&fsh, 
		sizeof fsh, "vxfs inode table head");
	vxfs_iptrs = (caddr_t *)malloc(sizeof (caddr_t *) * fsh.f_max);
	if (vxfs_iptrs == NULL) {
		fprintf(fp, "Could not allocate space for vxfs inode pointers\n");
		return;
	}
	readmem((long)(fsh.f_freelist),1,-1,(char *)vxfs_iptrs,
		sizeof (caddr_t *) * fsh.f_max, "vxfs inode pointers");
	for (i = 0; i < fsh.f_curr; i++) {
		actptr = getifilock(*(vxfs_iptrs + i));
		while(actptr) {
			readmem((long)actptr,1,-1,(char *)&fibuf,
				sizeof fibuf,"filock information");
			++active;
			fprintf(fp,"%x(%-7s) ",*(vxfs_iptrs + i),itype);
			if(fibuf.set.l_type == F_RDLCK) 
				fprintf(fp," r  ");
			else if(fibuf.set.l_type == F_WRLCK) 
				fprintf(fp," w  ");
			else fprintf(fp," ?  ");
			fprintf(fp,"%6d %10ld %10ld\n\t%4d %8d %8d %4x %8x %8x\n\n",
				fibuf.set.l_whence,
				fibuf.set.l_start,
				fibuf.set.l_len,
				findproc(fibuf.set.l_pid,fibuf.set.l_sysid),
				fibuf.set.l_pid,
				fibuf.set.l_sysid,
				fibuf.stat.wakeflg,
				fibuf.prev,
				fibuf.next);
			actptr = fibuf.next;
		}
	}
	free(vxfs_iptrs);
	return(active);
}

