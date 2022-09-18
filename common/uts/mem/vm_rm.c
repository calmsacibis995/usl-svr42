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

#ident	"@(#)uts-comm:mem/vm_rm.c	1.2.3.2"
#ident	"$Header: $"

/*
 * VM - resource manager.
 */

#include <mem/as.h>
#include <mem/page.h>
#include <mem/seg.h>
#include <proc/proc.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <util/cmn_err.h>
#include <util/param.h>
#include <util/types.h>

/*
 * This routine is called when we couldn't allocate an anon slot.
 * For now, we simply print out a message and kill off the process
 * that happened to have gotten burned.
 *
 * XXX - swap reservation needs lots of work so this only happens in
 * "nice" places or we need to have a method to allow for recovery.
 */

void
rm_outofanon()
{
	struct proc *p;

	p = u.u_procp;
	cmn_err(CE_WARN, "Sorry, pid %d (%s) was killed due to lack of swap space\n",
	    p->p_pid, u.u_comm);
	/*
	 * To ensure no looping, mark process locked in core
	 * (as though done by user) after killing it, so that
	 * no one will try to swap it out.
	 */
	psignal(p, SIGKILL);
	p->p_flag |= SLOCK;
}

/*
 * Routine to be called from the hat layer on
 * fatal exhaustion of some mapping resource.
 * Unused if the implementation is immune from such problems.
 */

void
rm_outofhat()
{
	cmn_err(CE_PANIC, "out of mapping resources\n");
	/*NOTREACHED*/
}

/*
 * Yield the size of an address space.
 */

size_t
rm_assize(as)
	register struct as *as;
{
	return (as == (struct as *)NULL? 0 : as->a_size);
}

/*
 * The following routines are available as macros for performance reasons.
 * If DEBUG is off, the macro versions defined in rm.h are used.
 */

#ifdef DEBUG

/*
 * Allocate physical pages for the kernel virtual address range
 * [addr, addr+len).  Flags as for page_get(), typically P_CANWAIT.
 */

/*ARGSUSED*/
page_t *
rm_allocpage(seg, addr, len, flags)
	struct seg *seg;
	addr_t addr;
	u_int len;
	u_int flags;
{
	return page_get(len, flags);
}

/*ARGSUSED*/
page_t *
rm_allocpage_aligned(seg, addr, len, align_mask, align_val, flags)
	struct seg *seg;
	addr_t addr;
	u_int len;
	u_int align_mask, align_val;
	u_int flags;
{
	return page_get_aligned(len, align_mask, align_val, flags);
}

/*
 * Yield the memory claim requirement for an address space.
 */

size_t
rm_asrss(as)
	register struct as *as;
{
	return (as == NULL? 0 : as->a_rss);
}

#endif	/* DEBUG */
