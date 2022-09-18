/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ident	"@(#)uts-x86:mem/getcpages.c	1.3"
#ident	"$Header: $"

/*
 * VM - physical page management.
 */

#include <util/types.h>
#include <util/param.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#include <util/sysmacros.h>
#include <util/inline.h>
#include <mem/page.h>
#include <mem/tuneable.h>
#include <mem/vmmac.h>
#include <mem/vmsystm.h>
#include <proc/proc.h>
#include <proc/user.h>


/*
 *	Gets physically contiguous pages.
 *	      Maintained for backwards compatibility.
 */

caddr_t
getcpages(npgs, nosleep)
	int npgs;		/* in clicks */
	int nosleep;		/* wait for pages iff nosleep == 0 */
{
	register page_t	*pp;

	if (npgs <= 0) {
#ifdef DEBUG
		cmn_err(CE_NOTE,"getcpages: request for %d pages\n",npgs);
#endif
		return(NULL);
	}

	if ((availrmem - npgs) < tune.t_minarmem) {
		nomemmsg("getcpages", npgs, 1, 0);
		return(NULL);
	}
	else	availrmem -= npgs;

	/*
	 *	Release 3.2 feature maintained, e.g., first try out without
	 *	sleeping for the pages, etc.
	 */

	if ((pp = page_get(ctob(npgs), P_PHYSCONTIG|P_DMA)) != (page_t *) NULL)
		goto found_contig;

	if (u.u_procp->p_pid == 0) {
		availrmem += npgs;
		nomemmsg("getcpages", npgs, 1, 0);
		return(NULL);
	}
	if (npgs > 1 && nosleep) {
		availrmem += npgs;
		nomemmsg("getcpages", npgs, 1, 0);
		return(NULL);
	}
	cmn_err(CE_NOTE,"!getcpages - waiting for %d contiguous pages",npgs);

	/*
	 *	Now try sleeping for the pages - assured to get it !!!
	 */
	pp = page_get(ctob(npgs), P_CANWAIT | P_PHYSCONTIG | P_DMA);

found_contig:
	if ((pp == (page_t *) NULL) || (pp < pages) || (pp + (npgs - 1) >= epages))
		cmn_err(CE_PANIC,"getcpages: Invalid pp = %x npgs = %d\n",pp, npgs);
#ifdef DEBUG
	{
		register int	i;
		register page_t	*check_pp, *next_pp;

		for (check_pp = next_pp = pp, i = 0;
			i < (npgs - 1);
			i++, next_pp = next_pp->p_next, check_pp++)
			if (check_pp != next_pp)
				cmn_err(CE_PANIC,"getcpages: not contiguous pages: npgs %d pageno %d pp %x checkpp %x nextpp %x\n",
					npgs, i, pp, check_pp, next_pp);
		bzero((caddr_t)phystokv(ctob(page_pptonum(pp))), ctob(npgs));
	}
#endif

	/*
	 *	Usually these pages are used for DMA - and their keep count is
	 *	already bumped up - so they will NOT be stolen/swapped.
	 *	Since we are returning a kernel virtual address, the virtual map
	 *	for the vtop translation is also thus never lost.
	 *	These pages are locked down in memory - until freepage() is invoked.
	 */

	pages_pp_kernel += npgs;	/* bump up pages kept for kernel */

	return ((caddr_t)phystokv(ctob(page_pptonum(pp))));
}

/*
 *	Frees a single page - pages allocated by getcpages().
 */
freepage(pfn)
	register u_int pfn;
{
	register page_t	*pp;

	pp = page_numtopp(pfn);			/* Must succeed */
	ASSERT(pp >= pages && pp < epages);	/* and hence this too */

	availrmem++;
	pages_pp_kernel--;

	page_rele(pp);				/* and then page_abort() */
}
