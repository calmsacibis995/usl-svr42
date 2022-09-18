/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:i386/cmd/crash/vtop.c	1.1.3.3"
#ident "$Header: vtop.c 1.1 91/07/23 $"

/*
 * This file contains code for the crash functions:  vtop and mode, as well as
 * the virtual to physical offset conversion routine vtop.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <a.out.h>
#include <stdio.h>
#include <sys/sysmacros.h>
#include <sys/fs/s5dir.h>
#include <sys/immu.h>
#include <sys/vnode.h>
#include <vm/hat.h>
#include <vm/seg.h>
#include <vm/as.h>
#include <vm/page.h>
#include <sys/proc.h>
#include <sys/vmparam.h>
#include "crash.h"

extern struct syment *Curproc;

int abortflag = 0;			/* flag to abort or continue */
					/* used in vtop function */
struct proc prbuf;			/* process entry buffer */
struct as   asbuf;			/* address space buffer */
int prsid;				/* temporary variables to hold */
long prssl,prsram,prpsl;		/* values to be printed by vtop */
					/* function */
pte_t *kpd_start;

/* virtual to physical offset address translation */
paddr_t
vtop(vaddr,slot)
unsigned long vaddr;
int slot;
{
#define errtx(tx) {if(!abortflag)return(-1);abortflag=0;error(tx);}
	static pte_t *KPD;
	pte_t *kpd_entry;
	pte_t pte, *pt;
	struct as c_as;
	struct as *ppas;
	struct hat *hatp;
	struct hat c_hat;
	struct hatpt c_ptap, *start_ptap, *ptap;

	/* Get kernel page directory */
	if (!KPD) {
		struct syment *sym;
		if (!(sym = symsrch("kpd0")))
			error("kpd0 not found in symbol table\n");
		kpd_start = (pte_t *)(sym->n_value);
		KPD = (pte_t *)(sym->n_value - KVSBASE);
	}
	pt = KPD + ptnum(vaddr);
	if (debugmode>1) fprintf(stderr,"vtop(%x,%d): ",vaddr,slot);
	if KADDR(vaddr) {
		if (debugmode>1) fprintf(stderr,"kvirt  ");
	} else {
		if (debugmode>1) fprintf(stderr,"must be transformed.\n");
		procntry(slot,&prbuf);
		if (!(prbuf.p_flag&SLOAD)) error("proc is swapped out\n");
		if (vaddr >= UVUBLK && PFNUM(vaddr - UVUBLK) < MAXUSIZE) {
			if (debugmode>1) fprintf(stderr,"ublock address.\n");
			readmem(prbuf.p_ubptbl + PFNUM(vaddr - UVUBLK),1,-1,
				&pte,sizeof(pte),"ublock page table entry");
			goto got_pte;
		} else if (vaddr < MAXUVADR) {
		    kpd_entry = (pte_t *)kpd_start + ptnum(vaddr);
		    /* Read in address space structure */
		    ppas = prbuf.p_as;
		    /* system process */
		    if(ppas == 0)
			error("Invalid address in system process\n");
		    readmem(ppas, 1, slot, &c_as, sizeof(c_as), "vm as");
		    /* Get start of HAT list structures */
		    hatp = &c_as.a_hat;
		    start_ptap = ptap = hatp->hat_pts;
		    /* Scan each HAT structure */
		    do {
			if (ptap == (struct hatpt *) NULL)
				error("Page table address pointer = NULL\n");
			/* Read in Page Table aligned HAT structure */
			readmem(ptap, 1, slot, &c_ptap, sizeof(c_ptap), "hat ptap");
			/* ASSERT: HAT pointer must be in kpd0 range */
			if ((c_ptap.hatpt_pdtep < kpd_start) || ((char *)c_ptap.hatpt_pdtep > (char *)kpd_start + NBPP))
				error("Kernel hat pointer invalid\n");
			if (c_ptap.hatpt_pdtep == kpd_entry){
				/* Get physical byte address of Page Table */
				pt = (pte_t *)ctob(c_ptap.hatpt_pde.pgm.pg_pfn);
				pt += pnum(vaddr); /* offset into Page Table */
				/* Physical address read of Page Table Entry */
				readmem(pt,0,slot,&pte, sizeof(pte), "Page table entry");
				goto got_pte;
			}
			else
				/* Next Page Table aligned HAT structure */
				ptap = c_ptap.hatpt_forw;
		    } while (ptap != start_ptap);
		}
	}

	readmem(pt,0,slot,&pte,sizeof(pte),"page directory entry");
	if (!(pte.pgm.pg_v)) errtx("Page Table not in core\n");
	readmem((pte_t *)ctob(pte.pgm.pg_pfn) + pnum(vaddr),
		0,slot,&pte,sizeof(pte),"page table entry");
got_pte:
	if (!(pte.pgm.pg_v)) errtx("Page not in core\n");
	return(ctob(pte.pgm.pg_pfn)+PAGOFF(vaddr));
}

/* get arguments for vtop function */
int
getvtop()
{
	int proc = Procslot;
	struct syment *sp;
	unsigned long addr;
	int c;


	optind = 1;
	while((c = getopt(argcnt,args,"w:s:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 's' :	proc = setproc();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) {
		fprintf(fp,"VIRTUAL  PHYSICAL SECT SDT   SRAM   PDT\n");
		do {
			if(*args[optind] == '(') {
				if((addr = eval(++args[optind])) == -1)
					continue;
				prvtop(addr,proc);
			}
			else if(sp = symsrch(args[optind])) 
				prvtop((long)sp->n_value,proc);
			else if(isasymbol(args[optind]))
				error("%s not found in symbol table\n",
					args[optind]);
			else {
				if((addr = strcon(args[optind],'h')) == -1)
					continue;
				prvtop(addr,proc);
			}
		}while(args[++optind]);
	}
	else longjmp(syn,0);
}

/* print vtop information */
int
prvtop(addr,proc)
unsigned long addr;
int proc;
{
	unsigned int paddr;

	abortflag = 1;
	paddr = vtop(addr,proc) + MAINSTORE;
	fprintf(fp,"%8x %8x %4d %3d",
		addr,
		paddr,
		prsid,
		prssl);
	if(prsram == -1)
		fprintf(fp,"         ");
	else fprintf(fp," %8x",prsram);
	fprintf(fp," %3d\n",
		prpsl);
	abortflag = 0;
}


/* get arguments for mode function */
int
getmode()
{
	int c;

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) 
		prmode(args[optind]);
	else prmode("s");
}

/* print mode information */
int
prmode(mode)
char *mode;
{

	switch(*mode) {
		case 'p' :  Virtmode = 0;
			    break;
		case 'v' :  Virtmode = 1;
			    break;
		case 's' :  break;
		default  :  longjmp(syn,0);
	}
	if(Virtmode)
		fprintf(fp,"Mode = virtual\n");
	else fprintf(fp,"Mode = physical\n");
}
	
