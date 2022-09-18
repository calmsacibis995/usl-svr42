/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:i386/cmd/crash/u.c	1.1.3.5"
#ident "$Header: u.c 1.1 91/07/23 $"

/*
 * This file contains code for the crash functions:  user, pcb, stack,
 * trace, and kfp.
 */

#include <sys/param.h>
#include <a.out.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/fs/s5dir.h>
#include <sys/immu.h>
#include <sys/tss.h>
#include <sys/seg.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <sys/var.h>
#include <sys/acct.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/lock.h>
#include <sys/signal.h>
#include <sys/cred.h>
#include "crash.h"
#include <sys/vmparam.h>
#include <sys/sysi86.h>

#define DECR	4
#define UBADDR UVUBLK
#define UREG(x) ((long*)(ubp->u_stack))[((long)(&ubp->u_ar0[x]) - UBADDR)/sizeof(long)]
long tmp1;

#define min(a,b) (a>b ? b:a)

#define	DATE_FMT	"%a %b %e %H:%M:%S %Y\n"
/*
 *	%a	abbreviated weekday name
 *	%b	abbreviated month name
 *	%e	day of month
 *	%H	hour
 *	%M	minute
 *	%S	second
 *	%Y	year
 */

extern struct user *ubp;		/* ublock pointer */
extern int active;			/* active system flag */
struct proc procbuf;			/* proc entry buffer */
static unsigned long Kfp = 0;		/* kernel frame pointer */
static char	time_buf[50];		/* holds date and time string */
extern	char	*strtbl ;		/* pointer to string table */
unsigned long *stk_bptr;		/* stack pointer */
extern	struct	syment	*File,
	*Vnode, *Curproc, *Panic, *V;	/* namelist symbol pointers */
extern struct	syment	*findsym();
extern char *malloc();
void free();
unsigned long *temp;

char *rlimits[] = {
	"cpu time",
	"file size",
	"swap size",
	"stack size",
	"coredump size",
	"file descriptors",
	"address space"
};

/* read ublock into buffer */
int
getublock(slot)
int slot;
{
	return _getublock(slot,USIZE*NBPC,ubp);
}

int
_getublock(slot, size, buf)
int slot;
long size;
struct user *buf;
{
	int 	i,cnt;
	struct proc *procp;
	pte_t	ubptbl[MAXUSIZE];
	long length;
	proc_t *slot_to_proc();

	if(slot == -1) 
		slot = getcurproc();
	if(slot >= vbuf.v_proc || slot < 0) {
		prerrmes("%d out of range\n",slot);
		return(-1);
	}

	procp = slot_to_proc(slot);
	if (procp == NULL) {
		prerrmes("%d is not a valid process\n",slot);
		return(-1);
	}
	readmem((unsigned long)procp,1,slot,(char *)&procbuf,sizeof procbuf,
		"process table");
	if (procbuf.p_stat == SZOMB) {
		prerrmes("%d is a zombie process\n",slot);
		return(-1);
	}
	if(active)
	{
		if(sysi86(RDUBLK, slot_to_pid(slot), (char *)buf, (USIZE*NBPC))==-1)
			return(-1);
		else
			return(0);
	}
	/* examine sysdump and U-Block was swapped-out */
	else if(!(procbuf.p_flag & SULOAD)) {
		prerrmes("%d was swapped-out\n", slot);
		return(-1);
	} else {
	if (procbuf.p_ubptbl == 0) {
		prerrmes("proc %d ubptbl is 0, but not swapped\n", slot);
		return(-1);
	}
	readmem(procbuf.p_ubptbl,1,-1,ubptbl,sizeof(ubptbl),
			"ublock page table entries");
	i=0;
	for(cnt=0; cnt < size; cnt += NBPP, i++) {
		/* seek from begining of memory to ith page of uarea */
		if (!ubptbl[i].pgm.pg_v) error("ublock not present\n");
		readmem(ctob(ubptbl[i].pgm.pg_pfn),0,0,
			(char *)buf+cnt,min(NBPP,size-cnt),"ublock");
	}
	}
	return(0);
}

/* allocate buffer for stack */
unsigned
setbf(top, bottom, slot)
unsigned long top;
unsigned long bottom;
int slot;
{
	unsigned range;
	char *bptr;
	long remainder;
	long nbyte;
	unsigned long paddr;


	if (bottom > top) 
		error("Top of stack value less than bottom of stack\n");
	range = (unsigned)(top - bottom);
	if((stk_bptr = (unsigned long *)malloc(range)) == NULL)
		error("Insufficient memory available for stack buffering.\n");
	
	bottom = top - range;
	bptr = (char *)stk_bptr;
	do {
		remainder = ((bottom + NBPP) & ~((long)NBPP -1)) - bottom;
		nbyte = min(remainder, top-bottom);
		if((paddr = vtop(bottom,slot)) == -1) {
			free((char *)stk_bptr);
			stk_bptr = NULL;
			error("The stack lower bound, %x, is an invalid address\nThe saved stack frame pointer is %x\n",bottom,top);
		}
		readmem(paddr,0,0,bptr,nbyte,"stack");
		bptr += nbyte;
		bottom += nbyte;
	} while (bottom < top);
	return(range);
}

/* get arguments for user function */
int
getuser()
{
	int slot = Procslot;
	int full = 0;
	int all = 0;
	unsigned long arg1 = -1;
	unsigned long arg2 = -1;
	int c;

	optind = 1;
	while((c = getopt(argcnt,args,"efw:")) !=EOF) {
		switch(c) {
			case 'f' :	full = 1;
					break;
			case 'e' :	all = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) {
		do {
			getargs(vbuf.v_proc,&arg1,&arg2);
			if(arg1 == -1)
				continue;
			if(arg2 != -1)
				for(slot = arg1; slot <= arg2; slot++)
					pruser(full,slot);
			else pruser(full,arg1);
			slot = arg1 = arg2 = -1;
		}while(args[++optind]);
	}
	else if (all) {
		for(slot =0; slot < vbuf.v_proc; slot++)
			pruser(full,slot);
	}
	else pruser(full,slot);
}

/* print ublock */
int
pruser(full,slot)
int full,slot;
{
	register  int  i,j;
	unsigned offset;

	if(getublock(slot) == -1)
		return;
	if(slot == -1)
		slot = getcurproc();
	fprintf(fp,"PER PROCESS USER AREA FOR PROCESS %d\n",slot);

	fprintf(fp,"PROCESS MISC:\n");
	fprintf(fp,"\tcommand: %s,", ubp->u_comm);
	fprintf(fp," psargs: %s\n", ubp->u_psargs);
	fprintf(fp,"\tproc slot: %d", proc_to_slot(ubp->u_procp));
	cftime(time_buf, DATE_FMT, &ubp->u_start);
	fprintf(fp,"\tstart: %s", time_buf);
	fprintf(fp,"\tmem: %x, type: %s%s\n",
		ubp->u_mem,
		ubp->u_acflag & AFORK ? "fork" : "exec",
		ubp->u_acflag & ASU ? " su-user" : "");
	fprintf(fp,"proc/text lock:%s%s%s%s\n",
		ubp->u_lock & TXTLOCK ? " txtlock" : "",
		ubp->u_lock & DATLOCK ? " datlock" : "",
		ubp->u_lock & PROCLOCK ? " proclock" : "",
		ubp->u_lock & (PROCLOCK|TXTLOCK|DATLOCK) ? "" : " none");
	if(ubp->u_cdir)
		fprintf(fp,"\tvnode of current directory: %8x",ubp->u_cdir);
	else fprintf(fp," - ,");
	if(ubp->u_rdir)
		fprintf(fp,", vnode of root directory: %8x,",ubp->u_rdir);
	fprintf(fp,"\nOPEN FILES AND POFILE FLAGS:\n");
	for(i = 0, j = 0; i < ubp->u_nofiles; i++){
		struct ufchunk uf;
		struct ufchunk *ufp;

		if ((i % NFPCHUNK) == 0) {
			if (i == 0) {
				ufp = &ubp->u_flist;
			} else {
				readmem((long)ufp->uf_next,1,-1,(char *)&uf,
					sizeof uf,"user file array");
				ufp = &uf;
			}
		}
		if(ufp->uf_ofile[i % NFPCHUNK] != 0) {
			if ((j++ % 2) == 0)
				fprintf(fp,"\n");
			fprintf(fp,"\t[%d]: F %#.8x, %x\t",i,
			    ufp->uf_ofile[i%NFPCHUNK],ufp->uf_pofile[i%NFPCHUNK]);
		}
	}
	fprintf(fp,"\n");
	fprintf(fp,"FILE I/O:\n\tu_base: %8x,",ubp->u_base);
	fprintf(fp," file offset: %d, bytes: %d,\n",
		ubp->u_offset,
		ubp->u_count);
	fprintf(fp,"\tsegment: %s,", ubp->u_segflg == 0 ? "data" :
		(ubp->u_segflg == 1 ? "sys" : "text"));
	fprintf(fp," cmask: %4.4o\n", ubp->u_cmask);
	fprintf(fp,"RESOURCE LIMITS:\n");
	for (i = 0; i < RLIM_NLIMITS; i++) {
		if (rlimits[i] == 0)
			continue;
		fprintf(fp,"\t%s: ", rlimits[i]);
		if (ubp->u_rlimit[i].rlim_cur == RLIM_INFINITY)
			fprintf(fp,"unlimited/");
		else
			fprintf(fp,"%d/", ubp->u_rlimit[i].rlim_cur);
		if (ubp->u_rlimit[i].rlim_max == RLIM_INFINITY)
			fprintf(fp,"unlimited\n");
		else
			fprintf(fp,"%d\n", ubp->u_rlimit[i].rlim_max);
	}
	fprintf(fp,"\tfile mode(s):");	
	fprintf(fp,"%s%s%s%s%s%s%s%s\n",
		ubp->u_fmode & FREAD ? " read" : "",
		ubp->u_fmode & FWRITE ? " write" : "",
		ubp->u_fmode & FAPPEND ? " append" : "",
		ubp->u_fmode & FSYNC ? " sync" : "",
		ubp->u_fmode & FCREAT ? " creat" : "",
		ubp->u_fmode & FTRUNC ? " trunc" : "",
		ubp->u_fmode & FEXCL ? " excl" : "",
		ubp->u_fmode & FNDELAY ? " ndelay" : "");
	fprintf(fp,"SIGNAL DISPOSITION:");
	for (i = 0; i < MAXSIG; i++) {
		if(!(i & 3))
			fprintf(fp,"\n\t");
		fprintf(fp,"%4d: ", i+1);
		if((int)ubp->u_signal[i] == 0 || (int)ubp->u_signal[i] == 1)
			fprintf(fp,"%8s",(int)ubp->u_signal[i] ? "ignore" : "default");
		else fprintf(fp,"%-8x",(int)ubp->u_signal[i]);
	}
	if(full) {
		fprintf(fp,"\tfc_flags %x, fc_errno %d, fc_func %x\n",
			ubp->u_fault_catch.fc_flags,
			ubp->u_fault_catch.fc_errno,
			ubp->u_fault_catch.fc_func);
		fprintf(fp,"\tnshmseg: %d, bsize: %d, qsav: %x, error: %d\n",
			ubp->u_nshmseg,
			ubp->u_bsize,
			ubp->u_qsav,
			ubp->u_error);
		fprintf(fp,"\tap: %x, u_r: %x, pbsize: %d\n",
			ubp->u_ap,
			ubp->u_rval1,
			ubp->u_pbsize);
		fprintf(fp,"\tpboff: %d,",ubp->u_pboff);
		fprintf(fp," rablock: %x, errcnt: %d\n",
			ubp->u_rablock,
			ubp->u_errcnt);
		fprintf(fp," tsize: %x, dsize: %x, ssize: %x\n",
			ubp->u_tsize,
			ubp->u_dsize,
			ubp->u_ssize);
		fprintf(fp,"\targ[0]: %x, arg[1]: %x, arg[2]: %x\n",
			ubp->u_arg[0],
			ubp->u_arg[1],
			ubp->u_arg[2]);
		fprintf(fp,"\targ[3]: %x, arg[4]: %x, arg[5]: %x\n",
			ubp->u_arg[3],
			ubp->u_arg[4],
			ubp->u_arg[5]);	
		fprintf(fp,"\tar0: %x, ticks: %x\n",
			ubp->u_ar0,
			ubp->u_ticks);

		fprintf(fp,"\tpr_base: %x, pr_size: %d, pr_off: %x, pr_scale: %d\n",
			ubp->u_prof.pr_base,
			ubp->u_prof.pr_size,
			ubp->u_prof.pr_off,
			ubp->u_prof.pr_scale);
		fprintf(fp,"\tior: %x, iow: %x, iosw: %x, ioch: %x\n",
			ubp->u_ior,
			ubp->u_iow,
			ubp->u_iosw,
			ubp->u_ioch);
		fprintf(fp, "\tsysabort: %d, systrap: %d\n",
			ubp->u_sysabort,
			ubp->u_systrap);
		fprintf(fp, "\tentrymask:");
		for (i = 0; i < sizeof(k_sysset_t)/sizeof(long); i++)
			fprintf(fp, " %08x", ubp->u_entrymask.word[i]);
		fprintf(fp, "\n");
		fprintf(fp, "\texitmask:");
		for (i = 0; i < sizeof(k_sysset_t)/sizeof(long); i++)
			fprintf(fp, " %08x", ubp->u_exitmask.word[i]);
		fprintf(fp, "\n");
		fprintf(fp,"\n\tEXDATA:\n");
		fprintf(fp,"\tvp: ");
		if(ubp->u_exdata.vp)
			fprintf(fp," %8x,",ubp->u_exdata.vp);
		else fprintf(fp," - , ");
		fprintf(fp,"tsize: %x, dsize: %x, bsize: %x, lsize: %x\n",
			ubp->u_exdata.ux_tsize,
			ubp->u_exdata.ux_dsize,
			ubp->u_exdata.ux_bsize,
			ubp->u_exdata.ux_lsize);
		fprintf(fp,"\tmagic#: %o, toffset: %x, doffset: %x, loffset: %x\n",
			ubp->u_exdata.ux_mag,
			ubp->u_exdata.ux_toffset,
			ubp->u_exdata.ux_doffset,
			ubp->u_exdata.ux_loffset);
		fprintf(fp,"\ttxtorg: %x, datorg: %x, entloc: %x, nshlibs: %d\n",
			ubp->u_exdata.ux_txtorg,
			ubp->u_exdata.ux_datorg,
			ubp->u_exdata.ux_entloc,
			ubp->u_exdata.ux_nshlibs);
		fprintf(fp,"\texecsz: %x\n",ubp->u_execsz);
		fprintf(fp,"\n\tRFS:\n");
		fprintf(fp,"\tsyscall: %d\n",ubp->u_syscall);
		fprintf(fp,"\n\tSIGNAL MASK:");
		for (i = 0; i < MAXSIG; i++) {
			if(!(i & 3))
				fprintf(fp,"\n\t");
			fprintf(fp,"%4d: %-8x ",i+1, (int)ubp->u_sigmask[i]);
		}
		fprintf(fp,"\n\tsigonstack: %x, sigflag: %x, oldmask: %x\n",
			ubp->u_sigonstack,
			ubp->u_sigflag,
			ubp->u_sigoldmask);
		fprintf(fp,"\taltflags: %s %s, altsp: %x, altsize: %x\n",
			ubp->u_sigaltstack.ss_flags&SS_DISABLE ? "disabl" : "",
			ubp->u_sigaltstack.ss_flags&SS_ONSTACK ? "onstak" : "",
			ubp->u_sigaltstack.ss_sp,
			ubp->u_sigaltstack.ss_size);
	}
	fprintf(fp,"\n");
}

/* get arguments for pcb function */
int
getpcb()
{
	int proc = Procslot;
	int phys = 0;
	char type = 'n';
	unsigned long addr = -1;
	int c;
	struct syment *sp;

	optind = 1;
	while((c = getopt(argcnt,args,"iukpw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			case 'i' :	type = 'i';
					break;
			case 'u' :	type = 'u';
					break;
			case 'k' :	type = 'k';
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(type == 'i') {
		if(!args[optind])
			longjmp(syn,0);
		if(*args[optind] == '(')  {
			if((addr = eval(++args[optind])) == -1)
				error("\n");
		}
		else if(sp = symsrch(args[optind])) 
			addr = (unsigned long)sp->n_value;
		else if(isasymbol(args[optind]))
			error("%s not found in symbol table\n",args[optind]);
		else if((addr = strcon(args[optind],'h')) == -1)
				error("\n");
		pripcb(phys,addr);
		}
	else {
		if(args[optind]) {
			if((proc = strcon(args[optind],'d')) == -1)
				error("\n");
			if((proc > vbuf.v_proc) || (proc < 0))
				error("%d out of range\n",proc);
			prpcb(proc,type);
		}
		else prpcb(proc,type);
	}
}


/* print user, kernel, or active pcb */
/* The kernel pcb is the 386 task state segment (pointed to by u.u_tss)
   There is no user pcb in the 386. The user task state is saved on the
   kernel stack, instead. This area is taken as "user pcb". */
   
int prpcb(proc,type)
int proc;
char type;
{
	struct tss386 tss;
	struct syment *sym;

	if(getublock(proc) == -1)
		return;
	switch(type) {
		case 'n' :
		case 'k' :
			if (active && ((proc== -1) || (proc == getcurproc())))
				error ("This is current process on active system\n");
			readmem(ubp->u_tss,1,proc,&tss,sizeof(tss),"TSS");
			test_tss(tss.t_esp0,UVUBLK+KSTKSZ,"esp0");
			test_tss(tss.t_esp1,UVUBLK+KSTKSZ,"esp1");
			test_tss(tss.t_esp2,UVUBLK+KSTKSZ,"esp2");
			test_tss(tss.t_ss0,KDSSEL,"ss0");
			test_tss(tss.t_ss1,KDSSEL,"ss1");
			test_tss(tss.t_ss2,KDSSEL,"ss2");
			if(!(sym = symsrch("kpd0"))) 
				error("kpd0 not found in symbol table\n");
			else if (tss.t_cr3 != ((sym->n_value-KVSBASE)|0x80000000))
				test_tss(tss.t_cr3,sym->n_value-KVSBASE,"cr3");
			test_tss(tss.t_ldt,LDTSEL,"ldt");
			printreg(tss.t_eax,tss.t_ebx,tss.t_ecx,tss.t_edx,
				 tss.t_esp,tss.t_ebp,tss.t_esi,tss.t_edi,
				 tss.t_eflags,tss.t_eip,tss.t_cs ,tss.t_ss,
				 tss.t_ds ,tss.t_es ,tss.t_fs ,tss.t_gs);
			break;
		case 'u' :
			if (procbuf.p_flag & SSYS)
				error ("This is a system process\n");
			/* u_ar0 points to location in kernel stack */
			fprintf(fp,"ERR=%d, TRAPNO=%d\n",
				UREG(ERR),UREG(TRAPNO));
			printreg(UREG(EAX),UREG(EBX), UREG(ECX), UREG(EDX),
				UREG(UESP),UREG(EBP), UREG(ESI), UREG(EDI),
				UREG(EFL), UREG(EIP), UREG( CS), UREG( SS),
				UREG( DS), UREG( ES), UREG( FS), UREG( GS));
			break;
		default  : longjmp(syn,0);
			   break;
	}
}

test_tss(actual,expected,regname)
unsigned long int actual,expected;
char *regname;
{
	if (actual != expected) fprintf(fp,
		"Field u_t%s in tss has strange value %08x, expected %08x\n",
		regname,actual,expected);
}

printreg(eax,ebx,ecx,edx,esp,ebp,esi,edi,efl,eip,cs,ss,ds,es,fs,gs)
unsigned int eax,ebx,ecx,edx,esp,ebp,esi,edi,efl,eip,cs,ss,ds,es,fs,gs;
{
	fprintf(fp,"cs:eip=%04x:%08x Flags=%03x\n",cs&0xffff,eip,efl&0x3ffff);
	fprintf(fp,
		"ds = %04x   es = %04x   fs = %04x   gs = %04x",
		ds&0xffff,es&0xffff,fs&0xffff,gs&0xffff);
	if (ss != -1) fprintf(fp,"   ss = %04x",ss&0xffff);
	fprintf(fp,"\nesi= %08x   edi= %08x   ebp= %08x   esp= %08x\n",
		esi,edi,ebp,esp);
	fprintf(fp,"eax= %08x   ebx= %08x   ecx= %08x   edx= %08x\n",
		eax,ebx,ecx,edx);
}


/* print interrupt pcb */
int
pripcb(phys,addr)
int phys;
unsigned long addr;
{
/*
	struct tss386 tss;
	readmem(addr,phys,-1,&tss,sizeof(tss),"TSS");
	fprintf(fp,"ss:esp [0] = %04x:%08x\n",tss.t_ss0,tss.t_esp0);
	fprintf(fp,"ss:esp [1] = %04x:%08x\n",tss.t_ss1,tss.t_esp1);
	fprintf(fp,"ss:esp [2] = %04x:%08x\n",tss.t_ss2,tss.t_esp2);
	fprintf(fp,"cr3 = %08x\n",tss.t_cr3);
	fprintf(fp,"ldt =     %04x\n",tss.t_ldt);
	printreg(tss.t_eax,tss.t_ebx,tss.t_ecx,tss.t_edx,
		 tss.t_esp,tss.t_ebp,tss.t_esi,tss.t_edi,
		 tss.t_eflags,tss.t_eip,tss.t_cs ,tss.t_ss,
		 tss.t_ds ,tss.t_es ,tss.t_fs ,tss.t_gs);
*/
	int	regs[19];
	readmem(addr,phys,-1,regs,sizeof(regs),"Register Set");
	printf("ERR=%d, TRAPNO=%d\n",
		regs[ERR],regs[TRAPNO]);
	printreg(regs[EAX],regs[EBX], regs[ECX], regs[EDX],
		regs[ESP], regs[EBP], regs[ESI], regs[EDI],
		regs[EFL], regs[EIP], regs[ CS], -1,
		regs[ DS], regs[ ES], regs[ FS], regs[ GS]);
}

/* get arguments for stack function */
int
getstack()
{
	int proc = Procslot;
	int phys = 0;
	char type = 'k';
	unsigned long addr = -1;
	int c;
	struct syment *sp;

	optind = 1;
	while((c = getopt(argcnt,args,"iukpw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			case 'i' :	type = 'i';
					break;
			case 'u' :	type = 'u';
					break;
			case 'k' :	type = 'k';
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(type == 'i') {
		if(!args[optind])
			longjmp(syn,0);
		if(*args[optind] == '(') {
			if((addr = eval(++args[optind])) == -1)
				error("\n");
		}
		else if(sp = symsrch(args[optind])) 
			addr = sp->n_value;
		else if(isasymbol(args[optind]))
			error("%s not found in symbol table\n",args[optind]);
			else if((addr = strcon(args[optind],'h')) == -1)
				error("\n");
		pristk(phys,addr);
	}
	else {
		if(args[optind]) {
			if((proc = strcon(args[optind],'d')) == -1)
				error("\n");
			if((proc > vbuf.v_proc) || (proc < 0))
				error("%d out of range\n",proc);
			if(type == 'u')
				prustk(proc);
			else prkstk(proc);
		}
		else if(type == 'u')
			prustk(proc);
		else prkstk(proc);
	}
}

/* print kernel stack */
int
prkstk(proc)
int proc;
{
	int panicstr;
	unsigned long stkfp,stklo,stkhi;
	struct tss386 tss;

	if(getublock(proc) == -1)
		return;
	if (active && ((proc== -1) || (proc == getcurproc())))
		error ("This is current process on active system\n");
	readmem(ubp->u_tss,1,proc,&tss,sizeof(tss),"TSS");
	if ((tss.t_esp<UBADDR) || (tss.t_esp>UBADDR+KSTKSZ))
		error("kernel stack not valid\n");
	stklo = tss.t_esp;
	stkfp = tss.t_ebp;
	stkhi = UBADDR+KSTKSZ;
	prkstack(stkfp,stklo,stkhi,proc);
}


/* print user stack */
int
prustk(proc)
int proc;
{
	int	panicstr;
	unsigned long	stkfp,stklo,stkhi ;
	if(getublock(proc) == -1)
		return;
	if (procbuf.p_flag & SSYS)
		error ("This is a system process\n");
	stkfp = UREG(EBP);
	stklo = UREG(UESP);
	stkhi = UVSTACK+sizeof(int);
	if ((stklo>stkhi) || (stkfp>stkhi)) error("user registers corrupted\n");
	prstack(stkfp,stklo,stkhi,proc);
}

/* print interrupt stack */
int
pristk(phys,addr)
int phys;
unsigned long addr;
{
	error("The iAPX386 has no interrupt stack\n");
}

/* dump stack */
int
prstack(stkfp,stklo,stkhi,slot)
unsigned long stkfp,stklo,stkhi; 
int slot;
{
	unsigned dmpcnt;
	unsigned long *stkptr;
	int prcnt;

	fprintf(fp,"FP: %x\n",stkfp);
	fprintf(fp,"LOWER BOUND: %x\n",stklo) ;
	
	if ( stkfp < stklo)
		error("upper bound < lower bound, unable to process stack\n") ;
	dmpcnt = setbf(stkhi, stklo, slot);
	stklo = stkhi - dmpcnt ;
	stkptr = (unsigned long *)(stk_bptr);

	prcnt = 0;
	for(; dmpcnt != 0; stkptr++, dmpcnt -= DECR)
	{
		if((prcnt++ % 4) == 0){
			fprintf(fp,"\n%8.8x: ",
				(int)(((long)stkptr - (long)stk_bptr)+stklo));
		}
		fprintf(fp,"  %8.8x", *stkptr);
	}
	free((char *)stk_bptr);
	stk_bptr = NULL;

	fprintf(fp,"\n\nSTACK FRAME:\n");
	fprintf(fp,"	ARGN ... ARG1  EIP'  EBP'  (REGS)  LOCAL1 ...\n");
	fprintf(fp,"	FP (=EBP) ------------^\n");
}

/* dump stack */
int
prkstack(stkfp,stklo,stkhi,slot)
unsigned long stkfp,stklo,stkhi;
int slot;
{
	unsigned dmpcnt;
	unsigned long *stkptr;
	int prcnt;
	proc_t *procp;
	proc_t *slot_to_proc();

	fprintf(fp,"FP: %x\n",stkfp);
	fprintf(fp,"LOWER BOUND: %x\n",stklo) ;
	
	if ( stkfp < stklo)
		error("upper bound < lower bound, unable to process stack\n") ;
	

	if(active) {
		stk_bptr = (unsigned long *)malloc(NBPP*USIZE);
		procp = slot_to_proc(slot);
		if (procp == NULL) {
			prerrmes("%d is not a valid process\n",slot);
			return(-1);
		}
		readmem((long)procp, 1, slot, (char *)&procbuf, sizeof procbuf,
			"process table");
		sysi86(RDUBLK, slot_to_pid(slot), (char *)stk_bptr, NBPP*USIZE);
		stkptr = (unsigned long *)((long)stk_bptr +(stklo - UBADDR));
		dmpcnt = stkhi - stklo;
		temp = stkptr;
	} else {
		dmpcnt = setbf(stkhi, stklo, slot);
		stklo = stkhi - dmpcnt;
		stkptr = (unsigned long *)(stk_bptr);
	}
	prcnt = 0;
	for(; dmpcnt != 0; stkptr++, dmpcnt -= DECR)
	{
		if((prcnt++ % 4) == 0){
		if(active)
			fprintf(fp,"\n%8.8x: ",
				(int)(((long)stkptr - (long)temp)+stklo));
		else
			fprintf(fp,"\n%8.8x: ",
				(int)(((long)stkptr - (long)stk_bptr)+stklo));
		}
		fprintf(fp,"  %8.8x", *stkptr);
	}

	free((char *)stk_bptr); 
	stk_bptr = NULL;

	fprintf(fp,"\n\nSTACK FRAME:\n");
	fprintf(fp,"	ARGN ... ARG1  EIP'  EBP'  (REGS)  LOCAL1 ...\n");
	fprintf(fp,"	FP (=EBP) ------------^\n");
}

/* get arguments for trace function */
int
gettrace()
{
	int proc = Procslot;
	int phys = 0;
	int all = 0;
	int kfpset = 0;
	char type = 'k';
	unsigned long addr = -1;
	unsigned long arg1 = -1;
	unsigned long arg2 = -1;
	int c;
	unsigned lastproc;
	struct syment *sp;

	optind = 1;
	while((c = getopt(argcnt,args,"ierpw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			case 'e' :	all = 1;
					break;
			case 'r' :	kfpset = 1;
					break;
			case 'i' :	type = 'i';
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(type == 'i') {
		if(!args[optind])
			longjmp(syn,0);
		if(*args[optind] == '(') {
			if((addr = eval(++args[optind])) == -1)
				error("\n");
		}
		else if(sp = symsrch(args[optind])) 
			addr = sp->n_value;
		else if(isasymbol(args[optind]))
			error("%s not found in symbol table\n",args[optind]);
			else if((addr = strcon(args[optind],'h')) == -1)
				error("\n");
		pritrace(phys,addr,kfpset,proc);
	}
	else {
		if(args[optind]) {
			do {
				getargs(vbuf.v_proc,&arg1,&arg2);
				if(arg1 == -1)
					continue;
				if(arg2 != -1)
					for(proc = arg1; proc <= arg2; proc++)
						prktrace(proc,kfpset);
				else
					prktrace(arg1,kfpset);
				proc = arg1 = arg2 = -1;
			} while(args[++optind]);
		} else if(all) {
			for(proc =0; proc < vbuf.v_proc; proc++) 
				prktrace(proc,kfpset);
		} else
			prktrace(proc,kfpset);
	}
}

/* print kernel trace */
int
prktrace(proc,kfpset)
int proc,kfpset;
{
	int panicstr;
	unsigned long	stklo, stkhi, pcbaddr;
	unsigned long	savefp,savesp,saveap,savepc;
	struct syment *symsrch();
	unsigned range;
	struct pcb *ptr;
	proc_t *procp;
	proc_t *slot_to_proc();
	struct tss386 tss;
	struct user *big_ub;

	fprintf(fp,"STACK TRACE FOR PROCESS %d:\n",proc);

	/* Get entire ublock; we need extra information */
	if ((big_ub = (struct user *)malloc(NBPP*USIZE)) == NULL) {
		prerrmes("Insufficient memory for copy of ublock.\n");
		return;
	}
	if (_getublock(proc, NBPP*USIZE, big_ub) == -1)
		return;
	if (active && ((proc== -1) || (proc == getcurproc()))) {
		prerrmes("This is current process on active system\n");
		return;
	}
	/* Can't call readmem, because readmem calls vtop
		readmem(big_ub->u_tss,1,proc,&tss,sizeof(tss),"TSS"); */
	/* Instead, get tss from ublock */
	tss = *(struct tss386 *)((long)big_ub + ((long)big_ub->u_tss - UBADDR));
	if ((tss.t_esp<UBADDR) || (tss.t_esp>UBADDR+KSTKSZ)) {
		prerrmes("kernel stack not valid\n");
		return;
	}
	savesp = tss.t_esp;
	savefp = tss.t_ebp;
	savepc = tss.t_eip;
	saveap = 0; 
	stklo = UVUBLK;
	stkhi = UBADDR+KSTKSZ;
	if (stkhi < stklo) {
		prerrmes("upper bound < lower bound\n");
		return;
	}
	stk_bptr = (unsigned long *)big_ub;
	if(kfpset) {
		if(Kfp)
			savefp = Kfp;
		else {
			prerrmes("stack frame pointer not saved\n");
			return;
		}
	}
	puttrace(stklo,savefp,savesp,saveap,savepc,kfpset,stkhi);
	free((char *)big_ub);
	stk_bptr = NULL;
}

/* print interrupt trace */
int
pritrace(phys,addr,kfpset,proc)
int phys,kfpset,proc;
unsigned long addr;
{
	error("The iAPX386 has no interrupt stack\n");
}

invalkfp() { error("Invalid kfp\n"); }

/* dump trace */
int
puttrace(stklo,sfp,ssp,sap,spc,kfpset,stkhi)
unsigned long stklo,sfp,ssp,sap,spc,stkhi;
int kfpset;
{
#define RET	stk_bptr[(sfp - stklo) / sizeof (long) + 1]
#define OAP	0
#define OSP	(sfp + 2*sizeof (long))
#define OFP	stk_bptr[(sfp - stklo) / sizeof (long)    ]
#define HEAD	"STKADDR   FRAMEPTR  FUNCTION  POSSIBLE ARGUMENTS\n"
#define FRMT	"%8.8x  %8.8x "
#define INSTACK(fp,val)	((val) < stkhi && (val) > (fp))
#define FREG(reg)	stk_bptr[(sfp - stklo) / sizeof (long) + (reg)]

	extern short N_TEXT;
	int noaptr = 0;
	
	signal(SIGSEGV,(void (*)())invalkfp);
	if(kfpset) {
		spc = RET;
		ssp = OSP;
		sap = OAP;
		sfp = OFP;
		fprintf(fp,"SET FRAMEPTR = %x\n\n",sfp);
	}
	fprintf(fp,HEAD);
	
	while (1) {
		if (INSTACK(sfp, OFP)) {
			noaptr = nframe(stklo, ssp, sfp, sap, spc);
			spc = RET;
			ssp = OSP;
			sap = OAP;
			sfp = OFP;
		} else {
			if (!INSTACK(sfp, FREG(ESP))) {
				nframe(stklo, ssp, sfp, sap, spc);
				break;
			}
			iframe(stklo, sfp, spc);
			if (!INSTACK(sfp, FREG(EBP)))
				break;
			spc = FREG(EIP);
			ssp = OSP;
			sfp = FREG(EBP);
		}
	}
	signal(SIGSEGV,SIG_DFL);
}

static void
prfuncname(addr)
unsigned long addr;
{
	struct syment *func_nm;

	if (addr == 0 || (func_nm = findsym(addr)) == 0)
		fprintf(fp, " %8.8x", addr);
	else {
		fprintf(fp, " %-8.8s", (char *) func_nm->n_offset);
	}
}

/* print a normal stack frame */
nframe(stklo, ssp, sfp, sap, spc)
unsigned long stklo, ssp, sfp, sap, spc;
{
	unsigned long *argp;
	int narg;

	fprintf(fp, FRMT, ssp, sfp, sap);
	prfuncname(spc);
	fprintf(fp, " (");
	argp = &stk_bptr[(sfp-stklo)/sizeof(long) + 2];
	for (narg = 0; narg < 4; narg++) {
		if (argp >= &stk_bptr[(OFP-stklo)/sizeof(long)])
			break;
		if (narg > 0)
			putc(',', fp);
		fprintf(fp, "%x", *argp++);
	}
	fprintf(fp, ")\n");
	return 0;
}

/* print an interrupt (or trap) stack frame */
iframe(stklo, sfp, spc)
unsigned long stklo, sfp, spc;
{
	static struct syment *CmnTrap, *CmnInt;
	struct syment *func;

	if (!CmnTrap) {
		if (!(CmnTrap = symsrch("cmntrap")))
			error("cmntrap not found in symbol table\n");
		if (!(CmnInt = symsrch("cmnint")))
			error("cmnint not found in symbol table\n");
	}
	func = findsym(spc);
	if (func == CmnTrap)
		fprintf(fp, "Trap %-4lx", FREG(TRAPNO));
	else if (func == CmnInt)
		fprintf(fp, "Int %-5lx", FREG(TRAPNO));
	else
		fprintf(fp, "         ");
	fprintf(fp, " %8.8x ", sfp);
	prfuncname(spc);
	fprintf(fp, " from %8.8x", FREG(EIP));
	if (KADDR(FREG(EIP))) {
		fprintf(fp, " in");
		prfuncname(FREG(EIP));
	}
	fprintf(fp, "\n  ax:%8lx cx:%8lx dx:%8lx bx:%8lx fl: %8lx ds:%4lx fs:%4lx\n",
		FREG(EAX), FREG(ECX), FREG(EDX), FREG(EBX), FREG(EFL),
		FREG(DS) & 0xFFFF, FREG(FS) & 0xFFFF);
	fprintf(fp, "  sp:%8lx bp:%8lx si:%8lx di:%8lx err:%8lx es:%4lx gs:%4lx\n",
		FREG(ESP), FREG(EBP), FREG(ESI), FREG(EDI), FREG(ERR),
		FREG(ES) & 0xFFFF, FREG(GS) & 0xFFFF);
}

/* get arguments for kfp function */
int
getkfp()
{
	int c;
	struct syment *sp;
	int reset = 0;
	int proc = Procslot;
	long value;

	optind = 1;
	while((c = getopt(argcnt,args,"w:s:r")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 's' :	proc = setproc();
					break;
			case 'r' :	reset = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) {
		if(*args[optind] == '(') {
			if((value = eval(++args[optind])) == -1)
				error("\n");
			prkfp(value,proc,reset);
		}
		else if(sp = symsrch(args[optind])) 
			prkfp(sp->n_value,proc,reset);
		else if(isasymbol(args[optind]))
			error("%s not found in symbol table\n",args[optind]);
		else {
			if((value = strcon(args[optind],'h')) == -1)
				error("\n");
			prkfp(value,proc,reset);
		}
	}
	else prkfp(-1,proc,reset);
}

/* print kfp */
int
prkfp(value,proc,reset)
long value;
int proc,reset;
{
	int panicstr;
	struct tss386 tss;
	
	if(value != -1)
		Kfp = value;
	else if(reset) {
		if(getublock(proc) == -1)
			return;
		if (active && ((proc== -1) || (proc == getcurproc())))
			error ("This is current process on active system\n");
		readmem(ubp->u_tss,1,proc,&tss,sizeof(tss),"TSS");
		Kfp = tss.t_ebp;
	}
	fprintf(fp,"kfp: %8.8x\n", Kfp);
}
