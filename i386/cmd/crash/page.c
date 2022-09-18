/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:i386/cmd/crash/page.c	1.1.2.5"
#ident "$Header: page.c 1.2 91/09/04 $"

/*
 * This file contains code for the crash functions: page, as, and ptbl.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <a.out.h>
#include <stdio.h>
#include <signal.h>
#include <sys/sysmacros.h>
#include <sys/fs/s5dir.h>
#include <sys/tss.h>
#include <sys/immu.h>
#include <sys/vnode.h>
#include <sys/vmparam.h>
#include <vm/vm_hat.h>
#include <vm/hat.h>
#include <vm/seg.h>
#include <vm/as.h>
#include <vm/page.h>
#include <sys/user.h>
#include <sys/var.h>
#include <sys/proc.h>
#include "crash.h"

/* namelist symbol pointers */
extern struct syment *V;		/* ptr to var structure */

extern u_long vtop();
extern pte_t *kpd_start;

void prsegs();

#define MAX_PFN	0xFFFFF

/*
 * On pagepool algorithms see "mem/vm_page.c"
 */
static struct pp_chunk  pagepool[MAX_MEM_CHUNK];/* Pagepool chunks	*/
static struct pp_chunk *pp_first;		/* First chunk in list	*/

static int
load_pagepool()
{
	struct syment    *Pagepool;
	struct pp_chunk **chunkpp;
	int i;

	if(!(Pagepool = symsrch("pagepool")))
		error("pagepool not found in symbol table\n");

	readmem((long)Pagepool->n_value, 1, -1,
		(char *)pagepool, MAX_MEM_CHUNK * sizeof(struct pp_chunk),
		"page pool table");

	if(debugmode > 0)
		dump_pagepool();

	/*
	 * Setup the ordered list
	 */
	for(i = 0; i<MAX_MEM_CHUNK; i++){
		if(pagepool[i].pp_epage == NULL &&
		   pagepool[i].pp_page  == NULL)
			break;

		chunkpp = &pp_first;
		while(*chunkpp != NULL){
			if((*chunkpp)->pp_epage - (*chunkpp)->pp_page >
			   pagepool[i].pp_epage - pagepool[i].pp_page)
					break;

			chunkpp = &(*chunkpp)->pp_next;
		}

		pagepool[i].pp_next = *chunkpp;
		*chunkpp = &pagepool[i];
	}

	if(debugmode > 0)
		dump_pagepool();

	return(0);
}

static int
dump_pagepool()
{
	struct pp_chunk *chp;
	int i;

	fprintf(fp,"PAGEPOOL	   addr      pfn     epfn     page    epage     next\n");
	for(i=0, chp = pagepool; i<MAX_MEM_CHUNK; i++, chp++){
		fprintf(fp,"Pagepool[%2d]:%c %08x %8d %8d %08x %08x %08x\n",
			i,chp == pp_first ? '*' : ' ',chp,
			chp->pp_pfn, chp->pp_epfn,
			chp->pp_page,chp->pp_epage,
			chp->pp_next);
	}

	return(0);
}

u_int
page_pptonum(pp)
	page_t	*pp;
{
	struct pp_chunk *chp;

	if(pp_first == NULL && load_pagepool() < 0)
		return((u_int)-1);

	chp = pp_first;

	while(pp < chp->pp_page || pp >= chp->pp_epage){
#if MAX_MEM_CHUNK > 1
		if((chp = chp->pp_next) == NULL)
#endif
			return((u_int)-1);
	}

	return((u_int)((pp-chp->pp_page)*(PAGESIZE/MMU_PAGESIZE))+chp->pp_pfn);
}

page_t *
page_numtopp(pfn)
	u_int	pfn;
{
	struct pp_chunk *chp;

	if(pp_first == NULL && load_pagepool() < 0)
		return((page_t *)NULL);

	chp = pp_first;

	while(pfn < chp->pp_pfn || pfn >= chp->pp_epfn){
#if MAX_MEM_CHUNK > 1
		if((chp = chp->pp_next) == NULL)
#endif
			return((page_t *)NULL);
	}

	return(&chp->pp_page[(pfn - chp->pp_pfn) / (PAGESIZE/MMU_PAGESIZE)]);
}

/* get arguments for page function */
int
getpage()
{
	u_int pfn = (u_int)-1;
	u_int all = 0;
	u_int phys = 0;
	u_long addr = -1;
	u_long arg1 = -1;
	u_long arg2 = -1;
	int c;

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

	fprintf(fp,"       PFN  KEEP       VNODE        HASH        PREV      VPPREV  FLAGS\n");
	fprintf(fp,"        PP   NIO      OFFSET                    NEXT      VPNEXT\n");
	if(args[optind]) {
		all = 1;
		do {
			getargs((int)MAX_PFN+1,&arg1,&arg2);
			if(arg1 == -1) 
				continue;
			if(arg2 != -1)
				for(pfn = arg1; pfn <= (u_int)arg2; pfn++)
					prpage(all,pfn,phys,addr);
			else {
				if(arg1 >= 0 && arg1 <= MAX_PFN)
					pfn = arg1;
				else
					addr = arg1;
				prpage(all,pfn,phys,addr);
			}
			pfn = addr = arg1 = arg2 = -1;
		} while(args[++optind]);
	} else
		for(pfn = 0; pfn <= MAX_PFN; pfn++)
			prpage(all,pfn,phys,addr);
}

/* print page structure table */
int
prpage(all,pfn,phys,addr)
u_int all,pfn,phys;
u_long addr;
{
	struct page pagebuf;
	u_int next, prev;
	u_int vpnext, vpprev;
	u_int hash;

	if (!Virtmode)
		phys = 1;
	if (addr == (u_long)-1) {
		if ((addr = (u_long)page_numtopp(pfn)) == 0)
			return;
		phys = 0;
	} else if (pfn == (u_int)-1 && !phys)
		pfn = page_pptonum((page_t *)addr);

	readmem(addr,!phys,-1,
		(char *)&pagebuf,sizeof pagebuf,"page structure table");

	/* check page flags */
	if ((*((ushort *)&pagebuf) == 0) && !all)
		return;

	if (pfn == (u_int)-1)
		fprintf(fp,"         -");
	else
		fprintf(fp,"     %5x",pfn);

	fprintf(fp,"  %4d  0x%08x  ",
		pagebuf.p_keepcnt,
		pagebuf.p_vnode);

	/* calculate page structure entry number of pointers */

	hash = page_pptonum(pagebuf.p_hash);
	if (hash == (u_int)-1)
		fprintf(fp,"0x%08x  ", pagebuf.p_hash);
	else
		fprintf(fp,"     %5x  ",hash);

	prev = page_pptonum(pagebuf.p_prev);
	if (prev == (u_int)-1)
		fprintf(fp,"0x%08x  ", pagebuf.p_prev);
	else
		fprintf(fp,"     %5x  ",prev);

	vpprev = page_pptonum(pagebuf.p_vpprev);
	if (vpprev == (u_int)-1)
		fprintf(fp,"0x%08x  ", pagebuf.p_vpprev);
	else
		fprintf(fp,"     %5x  ",vpprev);

	fprintf(fp,"%s%s%s%s%s%s%s%s%s%s\n",
		pagebuf.p_lock    ? "lock "    : "",
		pagebuf.p_want    ? "want "    : "",
		pagebuf.p_free    ? "free "    : "",
		pagebuf.p_intrans ? "intrans " : "",
		pagebuf.p_gone    ? "gone "    : "",
		pagebuf.p_mod     ? "mod "     : "",
		pagebuf.p_ref     ? "ref "     : "",
		pagebuf.p_pagein  ? "pagein "  : "",
		pagebuf.p_nc      ? "nc "      : "",
		pagebuf.p_age     ? "age "     : "");

	/* second line */

	fprintf(fp,"0x%08x  %4d    %8d              ",
		addr,
		pagebuf.p_nio,
		pagebuf.p_offset);

	next = page_pptonum(pagebuf.p_next);
	if (next == (u_int)-1)
		fprintf(fp,"0x%08x  ", pagebuf.p_next);
	else
		fprintf(fp,"     %5x  ",next);

	vpnext = page_pptonum(pagebuf.p_vpnext);
	if (vpnext == (u_int)-1)
		fprintf(fp,"0x%08x  \n", pagebuf.p_vpnext);
	else
		fprintf(fp,"     %5x  \n",vpnext);
}

/* get arguments for ptbl function */
int
getptbl()
{
	int proc = Procslot;
	int all = 0;
	int phys = 0;
	u_long addr = -1;
	int c;
	struct proc prbuf;
	struct as asbuf;
	int count = 1;

	optind = 1;
	while((c = getopt(argcnt,args,"epw:s:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'p' :	phys = 1;
					break;
			case 's' :	proc = setproc();
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	procntry(proc,&prbuf);
	readmem((long)prbuf.p_as, 1, -1, (char *)&asbuf, sizeof asbuf, "as");
	if (args[optind]) {
		if ((addr = (u_long)strcon(args[optind++],'h')) == (u_long)-1)
			error("\n");
		if (args[optind]) 
			if ((count = strcon(args[optind],'d')) == -1)
				error("\n");
		prptbl(all,phys,addr,count,(u_long)-1,proc);
	}
	else
		prptbls(all,proc,prbuf.p_as,&asbuf.a_hat);
}

/* print all of a proc's page tables */
int
prptbls(all,proc,as,hatp)
int all;
u_int proc;
struct as *as;
struct hat *hatp;
{
	u_long	ptapaddr;
	hatpt_t	ptapbuf;
	u_long	pt_addr, base;

	fprintf(fp, "Page Tables for Process %d\n", proc);

	if ((ptapaddr = (u_long)hatp->hat_pts) == 0)
		return;

	do {
		readmem(ptapaddr, 1, proc, (char *)&ptapbuf, sizeof(ptapbuf),
			"hatpt structure");

		fprintf(fp,
		"\nHATPT 0x%08x: virt 0x%08x pde 0x%08x aec %d locks %d\n\n",
			ptapaddr,
			base = (ptapbuf.hatpt_pdtep - kpd_start) << PTNUMSHFT,
			ptapbuf.hatpt_pde.pg_pte,
			ptapbuf.hatpt_aec,
			ptapbuf.hatpt_locks);

		if (ptapbuf.hatpt_as != as) {
			fprintf(fp, "WARNING - hatpt was not pointing to the correct as struct: 0x%8x\n",
				ptapbuf.hatpt_as);
			fprintf(fp, "          hatpt list traversal aborted.\n");
			break;
		}

		/* locate page table */
		pt_addr = pfntophys(ptapbuf.hatpt_pde.pgm.pg_pfn);
		prptbl(all, 1, pt_addr, NPGPT, base, proc);
	} while ((ptapaddr = (u_long)ptapbuf.hatpt_forw) != (u_long)hatp->hat_pts);
}

/* print page table */
int
prptbl(all,phys,addr,count,base,proc)
int all,phys;
u_int count,proc;
u_long addr,base;
{
	pte_t	ptebuf;
	u_int	i;

	if (count > NPGPT)
		count = NPGPT;

	if (base != (u_long)-1)
		fprintf(fp, "SLOT     VADDR    PFN   TAG   FLAGS\n");
	else
		fprintf(fp, "SLOT    PFN   TAG   FLAGS\n");

	for (i = 0; i < count; i++) {
		readmem(addr,!phys,proc,(char *)&ptebuf,sizeof(ptebuf),
			"page table");
		addr += sizeof(ptebuf);
		if (ptebuf.pgm.pg_pfn == 0 && !all)
			continue;
		fprintf(fp, "%4u", i);
		if (base != (u_long)-1)
			fprintf(fp, "  %08x", base + pfntophys(i));
		fprintf(fp, " %6x   %3x   %s%s%s%s%s\n",
			ptebuf.pgm.pg_pfn,
			ptebuf.pgm.pg_tag,
			ptebuf.pgm.pg_ref   ? "ref "   : "",	
			ptebuf.pgm.pg_rw    ? "w "     : "",	
			ptebuf.pgm.pg_us    ? "us "    : "",	
			ptebuf.pgm.pg_mod   ? "mod "   : "",	
			ptebuf.pgm.pg_v     ? "v "     : "");	
	}
}

/* get arguments for as function */
int
getas()
{
	struct var varbuf;
	int slot = -1;
	int proc = -1;
	int full = 0;
	int all = 0;
	int phys = 0;
	u_long addr = -1;
	u_long arg1 = -1;
	u_long arg2 = -1;
	int c;
	char *heading = "PROC  KEEP        SEGS    SEGLAST  MEM_CLAIM     HAT_PTS HAT_PTLAST    FLAGS\n";

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

	if (!full)
		fprintf(fp,"%s",heading);

	if(args[optind]) {
		do {
			getargs(vbuf.v_proc,&arg1,&arg2);
			if(arg1 == -1) 
				continue;
			if(arg2 != -1)
				for(proc = arg1; proc <= arg2; proc++)
					pras(all,full,proc,phys,addr,heading);
			else {
				if(arg1 >= 0 && arg1 < vbuf.v_proc)
					proc = arg1;
				else
					addr = arg1;
				pras(all,full,proc,phys,addr,heading);
			}
			addr = proc = arg1 = arg2 = -1;
		} while(args[++optind]);
	} else if (all) {
		for(proc = 0; proc < vbuf.v_proc; proc++) 
			pras(all,full,proc,phys,addr,heading);
	} else
		pras(all,full,Procslot,phys,addr,heading);
}


/* print address space structure */
int
pras(all,full,slot,phys,addr,heading)
int all,full,slot,phys,addr;
char *heading;
{
	struct proc prbuf, *procaddr;
	struct as asbuf;
	proc_t *slot_to_proc();

	if(addr == -1)
		procaddr = slot_to_proc(slot);
	else
		procaddr = (struct proc *) addr;

	if (procaddr) {
		readmem((long)procaddr,1, -1,(char *)&prbuf,sizeof prbuf,
		    "proc table");
	} else {
		return;
	}

	if (full)
		fprintf(fp,"\n%s",heading);

	fprintf(fp, "%4d  ", slot);

	if (prbuf.p_as == NULL) {
		fprintf(fp, "- no address space.\n");
		return;
	}

	readmem((long)(prbuf.p_as),1,-1,
		(char *)&asbuf,sizeof asbuf,"as structure");

	fprintf(fp,"%4d  0x%08x 0x%08x  %9d  0x%08x 0x%08x  %s%s\n",
		asbuf.a_keepcnt,
		asbuf.a_segs,
		asbuf.a_seglast,
		asbuf.a_rss,
		asbuf.a_hat.hat_pts,
		asbuf.a_hat.hat_ptlast,
		(asbuf.a_lock == 0) ? "" : "lock " ,
		(asbuf.a_want == 0) ? "" : "want " );

	if (full) { 
		prsegs(prbuf.p_as, (struct as *)&asbuf, phys);
	}
}


/* print list of seg structures */
void
prsegs(as, asbuf, phys)
	struct as *as, *asbuf;
	u_long phys;
{
	struct seg *seg, *sseg;
	struct seg  segbuf;
	struct syment *sp;
	extern char * strtbl;
	extern struct syment *findsym();

	sseg = seg = asbuf->a_segs;

	if (seg == NULL)
		return;

	fprintf(fp, "      LOCK        BASE     SIZE        NEXT       PREV          OPS        DATA\n");

	do {
		readmem(seg, 1, -1, (char *)&segbuf, sizeof segbuf,
			"seg structure");
		fprintf(fp, "      %4d  0x%08x %8d  0x%08x 0x%08x ",
			segbuf.s_lock,
			segbuf.s_base,
			segbuf.s_size,
			segbuf.s_next,
			segbuf.s_prev);

		/* Try to find a symbolic name for the sops vector. If
		 * can't find one print the hex address.
		 */
		sp = findsym((unsigned long)segbuf.s_ops);
		if ((!sp) || ((unsigned long)segbuf.s_ops != sp->n_value))
			fprintf(fp,"0x%08x  ", segbuf.s_ops);
			fprintf(fp, "%12.12s  ", (char *)sp->n_offset);

		fprintf(fp,"0x%08x\n", segbuf.s_data);

		if (segbuf.s_as != as) {
			fprintf(fp, "WARNING - seg was not pointing to the correct as struct: 0x%8x\n",
				segbuf.s_as);
			fprintf(fp, "          seg list traversal aborted.\n");
			return;
		}
	} while((seg = segbuf.s_next) != sseg);
}
