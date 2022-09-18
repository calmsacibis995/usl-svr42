/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:fs/procfs/prmachdep.c	1.8"
#ident	"$Header: $"

#include <mem/as.h>
#include <mem/faultcatch.h>
#include <mem/immu.h>
#include <mem/page.h>
#include <mem/vm_hat.h>
#include <mem/vmparam.h>
#include <proc/proc.h>
#include <proc/reg.h>
#include <proc/regset.h>
#include <proc/seg.h>
#include <proc/tss.h>
#include <proc/user.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/debugreg.h>
#include <util/fp.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/spl.h>
#include <util/sysmacros.h>
#include <util/types.h>

#ifdef WEITEK
#include <util/weitek.h>
#endif

/* for PRMPT used in macro page_numtopp() */
#ifndef DEBUG
#include <svc/systm.h>
#endif

#define map_uaddr(up, addr)	((caddr_t)up + ((u_long)addr - UVUBLK))

/*
 * Tell ID/TP what the name of this filesystem is, since it's different
 * from the module name.
 */
char prextmodname[] = "proc";



/*
 * Map a target process's u-block in and out.  prumap() makes it addressable
 * (if necessary) and returns a pointer to it.
 */
struct user *
prumap(p)
	register proc_t *p;
{
	/*
	 * With paged u-blocks, this is easy: we just use the address kept in
	 * the proc-table entry.
	 */
	ub_lock(p);		/* honor ublock locking protocol */
	return ((user_t *) p->p_segu);
}

/*
 * With paged u-blocks, there's nothing to do in order to unmap.
 */
void
prunmap(p)
	proc_t *p;
{
	ub_rele(p);		/* honor ublock locking protocol */
}

/*
 * Return general registers.  The u-block must already have been mapped
 * in (via prumap()) by the caller, who supplies its address.
 */
void
prgetregs(up, rp)
	register user_t *up;
	register gregset_t rp;
{
	register greg_t *uregs;

	if (up->u_ar0 == NULL) {  /* system process - no user regs */
		bzero((caddr_t)rp, sizeof(gregset_t));
		return;
	}

	uregs = (greg_t *) map_uaddr(up, up->u_ar0);

	bcopy((caddr_t)uregs, (caddr_t)rp, sizeof(gregset_t));
}

/*
 * Set general registers.  The u-block must already have been mapped
 * in (via prumap()) by the caller, who supplies its address.
 */
void
prsetregs(up, rp)
	register user_t *up;
	register gregset_t rp;
{
	register greg_t *uregs;
	register greg_t	saved_efl;

	if (up->u_ar0 == NULL) {  /* system process - no user regs */
		return;
	}

	uregs = (greg_t *) map_uaddr(up, up->u_ar0);
	saved_efl = uregs[EFL];
	bcopy((caddr_t)rp, (caddr_t)uregs, sizeof(gregset_t));

	/* protect system bits in %efl from user modification */
	uregs[EFL] = (uregs[EFL] & ~PS_SYSMASK) | (saved_efl & PS_SYSMASK);

	/* set TI bit in user CS for LDT access (XXX - is this necessary?) */
	uregs[CS] |= SEL_LDT;
}

/*
 * Return the value of the PC from the supplied register set.
 */
greg_t
prgetpc(rp)
	gregset_t rp;
{
	return rp[EIP];
}

/*
 * Is there floating-point support?
 */
int
prhasfp()
{
	extern char fp_kind;
#ifdef WEITEK
	extern char weitek_kind;

	return (fp_kind != FP_NO) || (weitek_kind != WEITEK_NO);
#else
	return (fp_kind != FP_NO);
#endif
}

/*
 * Get floating-point registers.  The u-block is mapped in here (not by
 * the caller).
 */
int
prgetfpregs(p, fp)
	register proc_t *p;
	register fpregset_t *fp;
{
	user_t *up = prumap(p);

	/*
	 * If already dumped core, then state is already saved in the ublock.
	 * core() saves the state in the ublock.
	 * For proc specific ioctl, we cannot guarantee the exact saved state,
	 * we return the state that was in the ublock at this moment.
	 */

	CATCH_FAULTS(CATCH_SEGU_FAULT) {
		if (fp_kind != FP_NO) {
			bcopy((caddr_t) &up->u_fps.u_fpstate,
			      (caddr_t) &fp->fp_reg_set,
			      sizeof fp->fp_reg_set);
		}
#ifdef WEITEK
		if (weitek_kind != WEITEK_NO) {
			bcopy((caddr_t) up->u_weitek_reg,
			      (caddr_t) fp->f_wregs,
			      sizeof fp->f_wregs);
		}
#endif
	}

	prunmap(p);

	return END_CATCH();

}

/*
 * Set floating-point registers.  The u-block is mapped in here (not by
 * the caller).
 */
int
prsetfpregs(p, fp)
	proc_t *p;
	register fpregset_t *fp;
{
	user_t *up = prumap(p);

	CATCH_FAULTS(CATCH_SEGU_FAULT) {
		if (fp_kind != FP_NO) {
			bcopy((caddr_t) &fp->fp_reg_set,
			      (caddr_t) &up->u_fps.u_fpstate,
			      sizeof fp->fp_reg_set);
		}
#ifdef WEITEK
		if (weitek_kind & WEITEK_HW) {
			bcopy((caddr_t) fp->f_wregs,
			      (caddr_t) up->u_weitek_reg,
			      sizeof fp->f_wregs);
		}
#endif
	}

	prunmap(p);

	return END_CATCH();
}

/*
 * Get debug registers.  The u-block is mapped in here (not by
 * the caller).
 */
int
prgetdbregs(p, db)
	register proc_t *p;
	register dbregset_t *db;
{
	user_t *up = prumap(p);

	CATCH_FAULTS(CATCH_SEGU_FAULT) {
		bcopy((caddr_t) up->u_debugreg, (caddr_t) db->debugreg,
						sizeof db->debugreg);
	}

	prunmap(p);

	return END_CATCH();
}

/*
 * Set debug registers.  The u-block is mapped in here (not by
 * the caller).
 */
int
prsetdbregs(p, db)
	proc_t *p;
	register dbregset_t *db;
{
	user_t *up = prumap(p);

	db->debugreg[DR_CONTROL] &= ~(DR_GLOBAL_SLOWDOWN |
				      DR_CONTROL_RESERVED |
				      DR_GLOBAL_ENABLE_MASK);

	CATCH_FAULTS(CATCH_SEGU_FAULT) {
		bcopy((caddr_t) db->debugreg, (caddr_t) up->u_debugreg,
						sizeof db->debugreg);

		up->u_debugon = (db->debugreg[DR_CONTROL] &
				 (DR_LOCAL_SLOWDOWN|DR_LOCAL_ENABLE_MASK));
	}

	prunmap(p);

	return END_CATCH();
}

/*
 * Return the "addr" field for pr_addr in prpsinfo_t.
 */
caddr_t
prgetpsaddr(p)
	register proc_t *p;
{
	return (caddr_t)p->p_ubptbl;
}

/*
 * Set the PSW to single-step the process.
 */
/* ARGSUSED */
void
prstep(p, up)
	proc_t *p;
	register user_t *up;
{
	if (up->u_ar0 == NULL) {  /* system process - no user regs */
		return;
	}

	((flags_t *)map_uaddr(up, &up->u_ar0[EFL]))->fl_tf = 1;
}

/*
 * Set the EIP to the specified virtual address.
 */
/* ARGSUSED */
void
prsvaddr(p, up, vaddr)
	proc_t *p;
	register user_t *up;
	caddr_t vaddr;
{
 	if (up->u_ar0 == NULL) {  /* system process - no user regs */
		return;
	}

	*((caddr_t *)map_uaddr(up, &up->u_ar0[EIP])) = vaddr;
}

/*
 * Map address "addr" in process "p" into a kernel virtual address.
 * The memory is guaranteed to be resident and locked down.
 */
/* ARGSUSED */
caddr_t
prmapin(p, addr, writing)
	proc_t *p;
	caddr_t addr;
	int writing;
{
	paddr_t paddr;
	extern paddr_t vtop();
	/*
	 * On the 386 this is easy: physical addresses are mapped into
	 * their equivalent virtual addresses.
	 * We call phystokv in case the i/o is asynchronous
	 */
	if ((paddr = vtop(addr, p)) == NULL)
		return NULL;
	return (caddr_t)phystokv(paddr);
}

/*
 * Unmap address "addr" in process "p"; inverse of prmapin().
 */
/* ARGSUSED */
void
prmapout(p, addr, vaddr, writing)
	proc_t *p;
	caddr_t addr;
	caddr_t vaddr;
	int writing;
{
	/*
	 * Nothing to do on the 386.
	 */
}

/*
 * Short-cut fast mapping-in of a process's page: if the page is already
 * resident and copy-on-write processing is not required, return a pointer
 * to the page.  This is a performance hack which may not be meaningful or
 * easy to implement on all systems, in which case this routine should
 * simply return NULL.
 *
 * The 386 version is a modified version of vtop() (from vm_hat.c).  It
 * computes the physical address of the page and returns it as a virtual
 * address.
 */

extern pte_t kpd0[];

caddr_t
prfastmapin(p, addr, writing)
	register proc_t *p;
	caddr_t addr;
	int writing;
{

	pte_t *ptp, *pdtep;
	paddr_t retval;
	struct	hat    *hatp;
	hatpt_t	*ptap, *eptap;
	int mcndx;
	int mcnum;
	hatpgt_t *pt;
	int s;

	s = splhi();
	ASSERT( !(KADDR((unsigned)addr)) && !(UVUBLK <= (unsigned)addr) );

	/*
	 * Here addr is in the context of some process other than
	 * curproc.  Since there is no per proc page directory, we
	 * scan its hatpts and the associated page tables.
	 */

	pdtep = kpd0 + ptnum(addr);
	if (p->p_as == (struct as *) NULL) {
#ifdef DEBUG
		cmn_err(CE_WARN, "process pid=%x, address space is null!\n",
			p->p_pid);
#endif
		splx(s);
		return(NULL);
	}
	hatp = &(p->p_as->a_hat);
	
	/*
	 * Should we take advantage of hat_ptlast here?
	 * We should not take advantage of hat_ptlast because it may
	 * point a higher virtual addresses.
	 * We could make the check but it is not worth the
	 * additional cpu cycles.
	 */

	ptap = eptap = hatp->hat_pts;
	if (ptap == (hatpt_t *)NULL) {
		ASSERT(hatp->hat_pts == (hatpt_t *)NULL);
			splx(s);
			return(NULL);
	} else {
		do {
			ASSERT(ptap && ptap->hatpt_aec);
			ASSERT(ptap->hatpt_pde.pg_pte && ptap->hatpt_pdtep);
			if (pdtep > ptap->hatpt_pdtep) {
				ptap = ptap->hatpt_forw;
				continue;
			}
			else
				break;
		} while (ptap != eptap);
		
		if (pdtep == ptap->hatpt_pdtep) { 
			hatp->hat_ptlast = ptap;
			pt = (hatpgt_t *) ptetokv(ptap->hatpt_pde.pg_pte);
			mcnum = HATMCNO(addr);
			mcndx = HATMCNDX(addr);
			ptp = pt->hat_pgtc[mcnum].hat_pte;
			ptp += mcndx;
		}
		else {
			splx(s);
			return(NULL);
		}
	}
	if (!PG_ISVALID(ptp) || (writing && !ptp->pgm.pg_rw)) {
		splx(s);
		return(NULL);
	}
	retval = phystokv(ctob(ptp->pgm.pg_pfn) + PAGOFF(addr));
	ASSERT(page_numtopp(ptp->pgm.pg_pfn));
	PAGE_HOLD(page_numtopp(ptp->pgm.pg_pfn));
done:
	splx(s);
	return((caddr_t)retval);
}

/*
 * Inverse of prfastmapin().
 */
/* ARGSUSED */
void
prfastmapout(p, addr, vaddr, writing)
	proc_t *p;
	caddr_t addr;
	caddr_t vaddr;
	int writing;
{
	ASSERT((unsigned)vaddr >= KVBASE);
	PAGE_RELE(page_numtopp(phystopfn(kvtophys(vaddr))));
}
