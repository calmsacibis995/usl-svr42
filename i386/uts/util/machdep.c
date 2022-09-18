/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

#ident	"@(#)uts-x86:util/machdep.c	1.11"
#ident	"$Header: $"

#include <fs/buf.h>
#include <fs/file.h>
#include <fs/fstyp.h>
#include <fs/procfs/prsystm.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <mem/hat.h>
#include <mem/immu.h>
#include <mem/page.h>
#include <mem/seg.h>
#include <mem/seg_vn.h>
#include <mem/vmparam.h>
#include <proc/acct.h>
#include <proc/cred.h>
#include <proc/exec.h>
#include <proc/proc.h>
#include <proc/reg.h>
#include <proc/siginfo.h>
#include <proc/signal.h>
#include <proc/tss.h>
#include <proc/ucontext.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/trap.h>
#include <svc/time.h>
#include <svc/utsname.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/fp.h>
#include <util/map.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/var.h>

#ifdef VPIX
#include <vpix/v86.h>
#endif

#ifdef WEITEK
#include <util/weitek.h>
#endif

#ifdef VPIX
/*      The following flag is used to indicate whether the process
**      is a dual mode process (user mode 386 task OR virtual 8086
**      mode task).
*/

extern char	v86procflag;
#endif

/*
 * Allocate 'size' units from the given map,
 * returning the base of an area which starts on a "mask" boundary.
 * That is, which has all of the bits in "mask" off in the starting
 * address.  Mask is assumed to be (2**N - 1).
 * Algorithm is first-fit.
 */
int
ptmalloc(mp, size, mask)
	struct map *mp;
	int size;
	int mask;
{
	register int a, b;
	register int gap, tail;
	register struct map *bp;

	ASSERT(size >= 0);
	for (bp = mapstart(mp); bp->m_size; bp++) {
		if (bp->m_size >= size) {
			a = bp->m_addr;
			b = (a+mask) & ~mask;
			gap = b - a;
			if (bp->m_size < (gap + size))
				continue;
			if (gap != 0) {
				tail = bp->m_size - size - gap;
				bp->m_size = gap;
				if (tail) 
					rmfree(mp, tail, bp->m_addr+gap+size);
			} else {
				bp->m_addr += size;
				if ((bp->m_size -= size) == 0) {
					do {
						bp++;
						(bp-1)->m_addr = bp->m_addr;
					} while ((bp-1)->m_size = bp->m_size);
					mapsize(mp)++;
				}
			}
			ASSERT(bp->m_size < (unsigned) 0x80000000);
			return b;
		}
	}
	return 0;
}

/*
 * 1. Map tss of the new process by JTSS
 * 2. Mark JTSS an available TSS
 * 3. Map ldt of the new process by LDTSEL descriptor
 */
mapnewtss(p)
struct proc *p;
{
 	user_t	*nu;
 	extern struct seg_desc gdt[];

#ifdef VPIX
 	register v86_t	*v86p;
 	char utssflag;
#endif

 	nu = PTOU(p);

 	/* Set up the new ldt as it will appear in the new
 	 * process.  Be careful not to use ldt based selectors
 	 * until after the context switch.
 	 */
	gdt[seltoi(LDTSEL)] = nu->u_ldt_desc;

	/*
 	 * set up size of the TEMPORARY new tss and mark it available
 	 */
#ifdef VPIX
 	/*
 	**  If outgoing process is a dual-mode process, remember which
 	**  task he was in when he entered the kernel.
 	*/
 	v86p = (v86_t *)(u.u_procp->p_v86);	/* Get ptr to v86_t struct */
 	if (v86p)                       /* Save TR for v86 process */
 	    v86p->vp_oldtr = (sel_t)get_tr() & (sel_t)0x0FFF8;

 	/*
 	**  Always set EM bit in CR0 if there is emulation, since if the
 	**  previous task was a dual-mode process in V86 mode, we may have
 	**  unset it to fool it into thinking there's no emulation.
 	*/
 	if (fp_kind == FP_SW)
 		setem();

 	/*
 	**  If incoming process is a dual-mode process then:
 	**    a)  If he entered the kernel in Virtual 86 mode, we must
 	**        map his XTSS, not his U-block TSS, into the window.
 	**        The XTSS descriptor must be marked AVAILABLE because
 	**        we will switch into it in misc.s:swtch(). The U-block
 	**        TSS is already marked AVAILABLE.
 	**    b)  If he entered the kernel in protected mode we treat
 	**        him like everyone else, but the XTSS descriptor must
 	**        be marked BUSY, because the ECT will IRET into it.
 	**  We can tell the difference by looking at the TSS selector we
 	**  saved when we switched him out (see immediately above).
	*/

 	utssflag = 1;     /* Assume no XTSS to map in                    */
	v86p = (v86_t *)(p->p_v86);	/* Get ptr to v86_t struct */
 	if (v86p)
 	{   gdt[seltoi(XTSSSEL)] = v86p->vp_xtss_desc;
 	    v86procflag = 1;
 	    if (v86p->vp_oldtr == XTSSSEL) {
		gdt[seltoi(JTSSSEL)] = v86p->vp_xtss_desc;
 		/*
 		 * Make sure the xtss has interrupts disabled, in case
 		 * we were swapped out
 		 */
 		v86p->vp_xtss->xt_tss.t_eflags &= ~PS_IE;
 		utssflag = 0;
 	    }
 	    else
 		setdscracc1(&gdt[seltoi(XTSSSEL)], TSS3_KBACC1);
 	} else
	    v86procflag = 0;	/* No user-mode idt switch for new proc */
 	if (utssflag)
#endif
	{
		gdt[seltoi(JTSSSEL)] = nu->u_tss_desc;

		ASSERT((((struct tss386 *)((char *)nu + (uint)nu->u_tss - UVUBLK))
			->t_eflags & PS_IE) == 0);
	}
}
 
setdscrbase(dscr, base)
struct dscr *dscr;
unsigned int base;
{
 	dscr->a_base0015 = (ushort)base;
 	dscr->a_base1623 = (base>>16)&0x000000FF;
 	dscr->a_base2431 = (base>>24)&0x000000FF;
}

setdscracc1(dscr, acc1)
struct dscr *dscr;
unsigned int	acc1;
{
 	dscr->a_acc0007 = (unsigned char)acc1;
}

setdscracc2(dscr, acc2)
struct dscr *dscr;
unsigned int	acc2;
{
 	dscr->a_acc0811 = acc2;
}
 
unsigned int
getdscrbase(dscr)
struct dscr *dscr;
{
 	register unsigned int base;
 
 	base = (dscr->a_base2431 << 24);
 	base = (base | (dscr->a_base1623 << 16));
 	base = (base | dscr->a_base0015);
 	return base;
}


/* dfgetuserTSS - Get the user TSS pointer from back link for double fault */
 
struct tss386 *
dfgetuserTSS()
{
 	extern struct seg_desc gdt[];
 	extern struct tss386 dftss;
 	struct seg_desc *tss_dscr;

 	tss_dscr = &gdt[seltoi(dftss.t_link)];  /* Get user TSS descriptor */
	return (struct tss386 *)getdscrbase(tss_dscr);
}

#if	DEBUG
/*
 *	db_resume()  --  debugging routine called at the end of resume()
 */
db_resume()
{
 	unsigned int getdscrlimit();
 
 	ASSERT(getdscrlimit(LDTSEL) ==
 				(u.u_ldtlimit+1)*sizeof(struct seg_desc) - 1);
}
#endif

/* return limit (in bytes) of a descriptor which may be in LDT or GDT */
unsigned int
getdscrlimit(seg)
ushort seg;
{
 	extern struct seg_desc gdt[];
 	register struct dscr *dp;
 	register unsigned int limit;

 	if(!(SEL_LDT & seg))
 		dp = (struct dscr *)&gdt[seltoi(seg)];
 	else {
 		seg = seltoi(seg);
 		if(seg >= u.u_ldtlimit)
 			return 0;
 		dp = (struct dscr *)(u.u_procp->p_ldt) + seg;
	}

 	/*	separated because compiler doesn't seem to handle
 		shifting of char bitfields quite right...
 	*/
 	limit = (unsigned int)dp->a_lim1619;
 	limit <<= 16;
 	limit |= (unsigned int)dp->a_lim0015;
 
 	if(dp->a_acc0811 & GRANBIT)
 		return (limit<<BPCSHIFT) | (NBPC-1);
 	return limit;
}

unsigned int
getdscraddr(seg)
unsigned short seg;
{
 	extern struct seg_desc gdt[];
 	register struct dscr *dp;
 	register unsigned addr;

 	if(!(SEL_LDT & seg))
 		dp = (struct dscr *)&gdt[seltoi(seg)];
 	else {
 		seg = seltoi(seg);
 		if(seg >= u.u_ldtlimit)
 			return 0;
 		dp = (struct dscr *)(u.u_procp->p_ldt) + seg;
 	}
 
 	addr = dp->a_base0015;
 	addr += dp->a_base1623 << 16;
 	addr += dp->a_base2431 << 24;
 
 	return addr;
}

/*
 * dhalt
 *	Halt devices.
 *
 * Called late in system shutdown to allow devices to stop cleanly
 * AFTER interrupts are shut off.  Interrupts may be turned on
 * again (e.g., the a.t.&t. console driver needs interrupts on to
 * recognize the ctrl-alt-del sequence) so the drivers should make
 * sure no interrupt is pending from their peripheral.
 */

dhalt()
{
 	extern int (*io_halt[])(); 
 	register int i;
 
 	for(i = 0; io_halt[i]; i++)
 		(*io_halt[i])();
}

/*
 * Adjust the time to UNIX based time.
 * This routine must run at IPL15.
 */

void
clkset(oldtime)
	register time_t	oldtime;
{
	extern void settime();

	settime(oldtime);
}

/*
 * Stop the clock.
 * This routine must run at IPL15.
 * hrt (common) references this routine.
 */

clkreld()
{
}

/*
** Can user write virtual address va?
** This routine is necessary because user addresses are being accessed
** from the kernel, and therefore the hardware won't enforce user permissions.
** Returns 0 unless an address-fault would occur.
** 386 specific: called from ucopy.c during copyout()/copyin().
**
** We have to do an explicit write fault here, since, on the 386, kernel
** writes to read-only pages don't generate faults.
**
** The user address validity check is done by VALID_USR_RANGE
*/

int
userwrite(va, nbytes)
register caddr_t va;
int nbytes;
{
	register pte_t		*ppte;
	register caddr_t	addr;
	register caddr_t	eaddr;
	k_siginfo_t	info;
	u_int		error;


	/*
	 * Account for the case where address range can cross the page boundary.
	 * Thus truncate addr to the begining of the page so that it will be 
	 * within the range of of eaddr.
	 */

	addr = (caddr_t)((long)va & PAGEMASK);
	eaddr = va + nbytes;

	/* 
	 * info.si_signo is used as the return value from 
	 * usrxmemflt
	 */
	info.si_signo = 0;

	/*
	 *	Optimization: If user pages are valid, user accessible and writeable,
	 *	then no need to break user copy-on-write.
	 */
	ppte = 0;
	while (addr < eaddr) {
		if (((ulong)ppte & PTMASK) == 0) {
			if (! PG_ISVALID(ppte = vatopdte(addr))) {
				if (usrxmemflt((PF_ERR_PAGE|PF_ERR_WRITE), 
						addr, &info))
					return(1);

			}
			ppte = vatopte(addr, ppte);
		}


		/* 
		 * For 80386 B1 stepping Errata #9 - page fault code unreliable
		 * Since kernel accesses only trap if page not present, we
		 * force a page-not-present and a page write fault to break
		 * a copy on write.
		 */
		if ((ppte->pg_pte & (PG_V | PTE_RW)) != (PG_V | PTE_RW)) {
			if (usrxmemflt((PF_ERR_PAGE|PF_ERR_WRITE), addr, 
					    &info))
				return(1);
		}

		addr += PAGESIZE;
		nbytes -= PAGESIZE;
		ppte++;
	}
	return 0;
}
/*
 * Checks to see if the I/O described in the argument buffer
 * violates any architecture-dependent bus constraints.
 * Should return -1 if so.  For legal requests the routine
 * typically returns 0, but some implementations might
 * profitably return a more useful result.
 *
 * In the 386 implementation buscheck is a no-op; we consider
 * all requests to be legal.
 */

/* ARGUSED */
int
buscheck(bp)
	struct buf *bp;
{
	return 0;
}

STATIC long mapin_count = 0;

/*
 * Map the data referred to by the buffer 'bp' into the kernel
 * at kernel virtual address 'addr'.  
 */

static void
bp_map(bp, addr)
	register struct buf *bp;
	caddr_t addr;
{
	register struct page *pp;
	register int npf;
	register pte_t	*ppte;
	register pte_t	*pdte;

	npf = btoc(bp->b_bufsize + ((int)bp->b_un.b_addr & PAGEOFFSET));

	if (bp->b_flags & B_PAGEIO) {
		pp = bp->b_pages;
		ASSERT(pp != NULL);
		while (npf--) {
			pdte = vatopdte((caddr_t)addr);
			ppte = vatopte((caddr_t)addr, pdte);
			ppte->pg_pte = (u_int)mkpte(PG_V, page_pptonum(pp));
			pp = pp->p_next;
			addr += PAGESIZE;
		}
		flushtlb();
	} else 
		cmn_err(CE_PANIC, "bp_map - non B_PAGEIO");
}

/*
 * Called to convert 'bp' for paged I/O to a kernel addressable location.
 * We allocate virtual space from the sptmap and then use bp_map to do
 * most of the real work.
 */

void
bp_mapin(bp)
	register struct buf *bp;
{
	int npf, o;
	caddr_t kaddr;

	mapin_count++;

	if ((bp->b_flags & (B_PAGEIO | B_PHYS)) == 0 ||
	    (bp->b_flags & B_REMAPPED) != 0)
		return;		/* no pageio or already mapped in */

	/*
	 * Check that the buffer is not marked as doing both
	 * paged I/O and physical I/O.
	 */
	if ((bp->b_flags & (B_PAGEIO | B_PHYS)) == (B_PAGEIO | B_PHYS))
		cmn_err(CE_PANIC, "bp_mapin");

	o = (int)bp->b_un.b_addr & PAGEOFFSET;
	npf = btoc(bp->b_bufsize + o);

	if ((npf <= 1) && (bp->b_flags & B_PAGEIO)) {
		register struct page *pp;

		pp = bp->b_pages;
		bp->b_un.b_addr += (u_int)pfntokv(page_pptonum(pp));
		return;
	}

	/*
	 * Allocate kernel virtual space for remapping.
	 */
	while ((kaddr = (caddr_t)rmalloc(sptmap, npf)) == 0) {
		mapwant(sptmap)++;
		(void) sleep((caddr_t)sptmap, PSWP);
	}
	kaddr = (caddr_t)((int)kaddr << PAGESHIFT);

	/* map the bp into the virtual space we just allocated */
	bp_map(bp, kaddr);

	bp->b_flags |= B_REMAPPED;
	bp->b_un.b_addr = kaddr + o;
}

/*
 * bp_mapout will release all the resources associated with a bp_mapin call.
 */
void
bp_mapout(bp)
	register struct buf *bp;
{
	register int npf, saved_npf;
	register pte_t *ppte;
	register pte_t *pdte;
	register struct page *pp;
	caddr_t addr, saved_addr;

	mapin_count--;

	if (bp->b_flags & B_REMAPPED) {
		pp = bp->b_pages;
		npf = btoc(bp->b_bufsize + ((u_int)bp->b_un.b_addr & PAGEOFFSET));
		saved_npf = npf;
		saved_addr = addr = (caddr_t)((u_int)bp->b_un.b_addr & PAGEMASK);
		while (npf--) {
			ASSERT(pp != NULL);
			pdte = vatopdte((caddr_t)addr);
			ppte = vatopte((caddr_t)addr, pdte);
			ppte->pg_pte = 0;
			addr += PAGESIZE;
			pp = pp->p_next;
		}
		flushtlb();
		saved_addr = (caddr_t)((u_int)saved_addr >> PAGESHIFT);
		rmfree(sptmap, saved_npf, (u_long)saved_addr);
		bp->b_un.b_addr = (caddr_t)((u_int)bp->b_un.b_addr & PAGEOFFSET);
		bp->b_flags &= ~B_REMAPPED;
	}
}
