/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:proc/grow.c	1.5"
#ident	"$Header: $"

#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/file.h>
#include <fs/vnode.h>
#include <mem/as.h>
#include <mem/seg_vn.h>
#include <mem/vmparam.h>
#include <proc/lock.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/reg.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

extern int do386b1;

struct brka {
	caddr_t	nva;
};

/*
 * Increase or decrease the size of a process's data space.
 */

/* ARGSUSED */
int
brk(uap, rvp)
	struct brka *uap;
	rval_t *rvp;
{
	register struct proc *p;
	register caddr_t nva;	/* new break address */
	register caddr_t ova;	/* old break address */
	register size_t nsize;	/* new break size */
#ifdef ASYNCIO
	caddr_t  ab, ae;
#endif /* ASYNC IO */

	p = u.u_procp;
	nva = uap->nva;

        if (nva < p->p_brkbase
	 || nva > (caddr_t) UVSHM
	 || (p->p_stkbase > u.u_exdata.ux_datorg
          && nva >= (caddr_t)u.u_sub)
	 || ((nsize = nva - p->p_brkbase) > p->p_brksize
          && nsize > u.u_rlimit[RLIMIT_DATA].rlim_cur)) {
		/*
		 * Return an error if the size argument supplied was
		 * negative, the new address is beyond the range of
		 * the shared memory address, or the additional memory 
		 * results in the user exceeding the process limit.
		 */
		return ENOMEM;
	}
	nva = (caddr_t)roundup((u_int)nva, PAGESIZE);
	ova = (caddr_t)roundup((u_int)(p->p_brkbase+p->p_brksize), PAGESIZE);

	if (nva > ova) {

		int error;
		int lckflag = 0;

		if (u.u_lock & (DATLOCK | PROCLOCK)) {
			if (p->p_as->a_paglck == 0) {
				 p->p_as->a_paglck = 1;
				 lckflag = 1;
			}
		}

		/*
		 * Add new zfod mapping to extend data segment
		 */

		error = as_map(p->p_as, ova, (u_int)(nva - ova), 
		 	       segvn_create, zfod_argsp); 

		if (lckflag)
			p->p_as->a_paglck = 0;

		if (error)
			return error;

	} else if (ova > nva) {

		/*
		 * Can't shrink the data space if there are any outstanding
		 * async I/O operations.
		 */

		if (p->p_aiocount)
			return EINVAL;
#ifdef ASYNCIO
		/* Disallow to shrink memory used for raw disk async I/Os */ 

		ab = u.u_raioaddr;
		ae = ab + u.u_raiosize;
	
		if ((ab <= nva && nva < ae) ||
		    (nva <= ab && ab < ova) || (nva <= ae && ae < ova))
				return EBUSY;
#endif ASYNCIO

		/*
		 * Unmap the range of pages requested.
		 */
		(void) as_unmap(p->p_as, nva, (u_int)(ova - nva));
	}

	p->p_brksize = nsize;
	return 0;
}

/*
 * Grow the stack to include the SP.  Return 1 if successful, 0 otherwise.
 * This routine is machine dependent.
 */
int
grow(sp)
	int *sp;
{
	register int	si;
	register struct proc *p = u.u_procp;
	register int	ssize;
	register int 	lckflag = 0;


	if (btoct((caddr_t)sp) < (btoct(u.u_ar0[UESP]) -1))
		return 0;

	/*
	 * Calculate the size to grow by subtracting the current
	 * base address of the stack from the specified address
	 * minus the size of the current stack plus the stack
	 * increment (SINCR).
	 */
	ssize = btoc(p->p_stksize);
	si = btoc((u_int)p->p_stkbase - (u_int)sp) - ssize + SINCR;

	if (si == 0 ) {
		/* bad stack size */
		return 0;
	}
	if (si > 0) {
		/* 
		 * Subtract SINCR to allow use of the entire RLIMIT_STACK
		 * as specified in the SVID.
		 */
		if (ctob(ssize + si - SINCR) > u.u_rlimit[RLIMIT_STACK].rlim_cur) {
			/* new stack size exceeds process limit */
			return 0;
		}

                /* 80386 B1 stepping workaround (Errata #10) --
                   Since dirty pages in the range 1-15 can cause I/O problems,
                   don't allow the stack to grow below page 16 */

                if (do386b1 && btoct((u_int)u.u_sub) - si < 16) {
                        if (btoct((u_int)u.u_sub) - si + SINCR >= 16) {
                                if ((si = btoct((u_int)u.u_sub) - 16) == 0)
                                        return 0;
                        } else
                              	return 0;
		}
		if (u.u_lock & (DATLOCK | PROCLOCK)) {
			if (p->p_as->a_paglck == 0) {
				 p->p_as->a_paglck = 1;
				 lckflag = 1;
			}
		}
                if (as_map(p->p_as, (ulong)(p->p_stkbase - ctob(ssize + si) 
			+ sizeof(int)), (u_int)ctob(si), segvn_create, 
			zfod_argsp) != 0) {
			if (lckflag)
				p->p_as->a_paglck = 0;
			return (0);
		}

		if (lckflag)
			p->p_as->a_paglck = 0;
	} else {
		/*
		 * Release mapping to shrink stack segment.
		 */
                (void) as_unmap(p->p_as, p->p_stkbase - ctob(ssize) 
			+ sizeof(int), (u_int)ctob(-si));
	}

	p->p_stksize += ctob(si);
	u.u_sub = (ulong)(p->p_stkbase - p->p_stksize + sizeof(int));

	return 1;

}

struct mmapa {
	caddr_t	addr;
	size_t	len;
	int	prot;
	int	flags;
	int	fd;
	off_t	pos;
};

/*
 * Map-in memory of the specified type.
 */
int
mmap(uap, rvp)
	register struct mmapa *uap;
	rval_t *rvp;
{
	register struct vnode *vp;
	caddr_t addr;
	register size_t len;
	struct as *as = u.u_procp->p_as;
	struct file *fp;
	u_int prot, maxprot;
	u_int type;
	register u_int flags;
	register int error;

	flags = uap->flags;
	type = flags & MAP_TYPE;

	if ((flags & ~(MAP_SHARED | MAP_PRIVATE | MAP_FIXED 
		| MAP_NORESERVE	/* not implemented, but don't fail here */
		/* | MAP_RENAME */	/* not yet implemented, let user know */
		)) != 0) {

		return EINVAL;
	}

        if (type != MAP_PRIVATE && type != MAP_SHARED) {
		return EINVAL;
	}

	len = uap->len;
	addr = uap->addr;

	/*
	 * Check for bad lengths and file position.
	 * We let the VOP_MAP routine check for negative lengths
	 * since on some vnode types this might be appropriate.
	 */
	if ((uap->pos & PAGEOFFSET) != 0) {
		return EINVAL;
	}

	if (error = getf(uap->fd, &fp))
		return error;

	vp = fp->f_vnode;

	maxprot = PROT_ALL;		/* start out allowing all accesses */
	prot = uap->prot | PROT_USER;

	if (type == MAP_SHARED && (fp->f_flag & FWRITE) == 0) {
		/* no write access allowed */
		maxprot &= ~PROT_WRITE;
	}

	/* On the 386 write privileges imply read privileges */
	if (prot & PROT_WRITE)
		prot |= PROT_READ;

	/*
	 * XXX - Do we also adjust maxprot based on protections
	 * of the vnode?  E.g. if no execute permission is given
	 * on the vnode for the current user, maxprot probably
	 * should disallow PROT_EXEC also?  This is different
	 * from the write access as this would be a per vnode
	 * test as opposed to a per fd test for writability.
	 */

	/*
	 * Verify that the specified protections are not greater than
	 * the maximum allowable protections.  Also test to make sure
	 * that the file descriptor does allows for read access since
	 * "write only" mappings are hard to do since normally we do
	 * the read from the file before the page can be written.
	 */
	if (((maxprot & prot) != prot) || (fp->f_flag & FREAD) == 0) {
		return EACCES;
	}

	/*
	 * If the user specified an address, do some simple checks here
	 */
	if ((flags & MAP_FIXED) != 0) {
		/*
		 * Use the user address.  First verify that
		 * the address to be used is page aligned.
		 * Then make some simple bounds checks.
		 */
		if (((int)addr & PAGEOFFSET) != 0) {
			return EINVAL;
		}
		if (VALID_USR_RANGE(addr, len) == 0) {
			return ENOMEM;
		}
	}

	/*
	 * Ok, now let the vnode map routine do its thing to set things up.
	 */
	if (error = VOP_MAP(vp, uap->pos, as, &addr, len, prot, maxprot, flags,
	    fp->f_cred))
		return error;

	rvp->r_val1 = (int)addr;	/* return starting address */
	return 0;
}

struct munmapa {
	caddr_t	addr;
	size_t	len;
};

/*
 * Unmap a memory range.
 */

/* ARGSUSED */
int
munmap(uap, rvp)
	struct munmapa *uap;
	rval_t *rvp;
{
	register caddr_t addr = uap->addr;
	register size_t len = uap->len;
	register struct proc *p = u.u_procp;

	if (((int)addr & PAGEOFFSET) != 0) {
		return EINVAL;
	}

	if (VALID_USR_RANGE(addr, len) == 0) {
		return EINVAL;
	}

	if (as_unmap(p->p_as, addr, len) != 0) {
		return EINVAL;
	}

	return 0;
}

struct mprotecta {
	caddr_t	addr;
	size_t	len;
	int	prot;
};

/*
 * Set the permission on a specified address range.
 */

/* ARGSUSED */
int
mprotect(uap, rvp)
	struct mprotecta *uap;
	rval_t *rvp;
{
	register caddr_t addr = uap->addr;
	register size_t len = uap->len;
	register u_int prot = uap->prot | PROT_USER;
	register int error;

	if (((int)addr & PAGEOFFSET) != 0) {
		return EINVAL;
	}

	if (VALID_USR_RANGE(addr, len) == 0) {
		return ENOMEM;
	}

	/* On the 386 write privileges imply read privileges */
	if (prot & PROT_WRITE)
		prot |= PROT_READ;

	error = as_setprot(u.u_procp->p_as, addr, len, prot);
	return error;
}

#define	MC_CACHE	128			/* internal result buffer */
#define	MC_QUANTUM	(MC_CACHE * PAGESIZE)	/* addresses covered in loop */

struct mincorea {
	caddr_t addr;
	size_t	len;
	char	*vec;
};

/*
 * Return the number of in-core pages in the specified
 * address range for the current process.
 */

/* ARGSUSED */
int
mincore(uap, rvp)
	struct mincorea *uap;
	rval_t *rvp;
{
	register caddr_t addr = uap->addr;
	register caddr_t ea;			/* end address of loop */
	register struct as *as;			/* address space */
	u_int rl;				/* inner result length */
	char vec[MC_CACHE];			/* local vector cache */
	register int error;
	size_t	len = uap->len;

	/*
	 * Validate form of address parameters.
	 */
	if (((int)addr & PAGEOFFSET) != 0) {
		return EINVAL;
	}

	if (VALID_USR_RANGE(addr, len) == 0) {
		return ENOMEM;
	}

	/*
	 * privilege required on a system running MAC
	 * to avoid a potential covert channel.
	 */
	if (mac_installed && pm_denied(u.u_cred, P_PLOCK))
		return EPERM;

	/*
	 * Loop over subranges of interval [addr : addr + len), recovering
	 * results internally and then copying them out to caller.  Subrange
	 * is based on the size of MC_CACHE, defined above.
	 */
	as = u.u_procp->p_as;
	for (ea = addr + len; addr < ea; addr += MC_QUANTUM) {
		error = as_incore(as, addr, 
		    (u_int)MIN(MC_QUANTUM, ea - addr), vec, &rl);
		if (rl != 0) {
			rl = (rl + PAGESIZE - 1) / PAGESIZE;
			if ((error = copyout(vec, uap->vec, rl)) != 0)
				return EFAULT;
			uap->vec += rl;
		}
		if (error != 0) {
			return ENOMEM;
		}
	}
	return 0;
}

/*
 * The madvise system call was never implemented.
 * The code is listed, however, as a model for future development.
 */

#if 0

struct madvisea {
	caddr_t	addr;
	size_t len;
	int behav;
};

/* ARGSUSED */
int
madvise(uap, rvp)
	register struct madvisea *uap;
	rval_t *rvp;
{
	faultcode_t fc;
	register caddr_t addr = uap->addr;
	register size_t len = uap->len;
	register struct as *as = u.u_procp->p_as;
	register int error = 0;

	if (VALID_USR_RANGE(addr, len) == 0) {
		return(ENOMEM);
	}

	switch (uap->behav) {

	case MADV_NORMAL:
		/*
		 * Should inform segment driver to go back to read-ahead
		 * and no free behind the current address on faults.
		 */
		break;

	case MADV_RANDOM:
		/*
		 * Should inform segment driver to have no read-ahead
		 * and no free behind the current address on faults.
		 */
		break;

	case MADV_SEQUENTIAL:
		/*
		 * Should inform segment driver to do read-ahead and
		 * to free behind the current address on faults.
		 */
		break;

	case MADV_WILLNEED:
		fc = as_faulta(as, addr, (u_int)len);
		if (fc) {
			if (FC_CODE(fc) == FC_OBJERR)
				error = FC_ERRNO(fc);
			else
				error = EINVAL;
		}
		break;

	case MADV_DONTNEED:
		/*
		 * For now, don't need is turned into an as_ctl(MC_SYNC)
		 * operation flagged for async invalidate.
		 */
		error = as_ctl(as, addr, len, MC_SYNC, 0,
	            (caddr_t) (MS_ASYNC | MS_INVALIDATE),
		    	(u_long *)NULL, (size_t)NULL);
		break;

	default:
		/* unknown behavior value */
		error = EINVAL;
		break;

	}
	return error;
}
#endif
