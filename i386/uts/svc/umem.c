/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:svc/umem.c	1.1"
#ident	"$Header: $"

/* umem.c - User memory allocation */

/* Enhanced Application Compatibility Support */

#include <proc/user.h>
#include <proc/proc.h>
#include <proc/reg.h>
#include <svc/trap.h>
#include <svc/umem.h>
#include <mem/seg.h>
#include <mem/seg_vn.h>

/*
** Macro to align `sp' to size of the largest type we're likely to use (ie.
** "char *" not "double").
*/
#define ALIGN_SP(sp)	((unsigned)(sp) & ~(sizeof(char *) - 1))


/*
** Allocate memory in user space for use by routines in "os/sco.c" and
** "os/isc.c" (or anyone who needs it).  There is an assumption that
** any memory allocated will be freed before the return to user context.
** This will keep users from seeing anomalies appear in their address
** space if a new segment has to be used.
**
** The value returned in `where' should be passed to free when the
** corresponding address is deallocated.
*/
_VOID *
umem_alloc(size, where)
	size_t	size;
	int	*where;
{
	_VOID		*addr = NULL;
	caddr_t		cur_sp = (caddr_t)u.u_ar0[UESP];
	struct proc	*p = u.u_procp;
	stack_t		*asp = &u.u_sigaltstack;

	/*
	** Allocate on stack if:
	** 	1. The current stack pointer is in range of the normal stack
	**	   (ie. no alternate or user-defined stack is currently being
	**	    used).
	**	OR
	**	2a. The current stack pointer is in range of the alternate
	**	    stack.
	**	AND
	**	2b. There is enough room on the alternate stack (after
	**	    alignment).
	*/
	if (cur_sp <= p->p_stkbase && cur_sp >= p->p_stkbase - p->p_stksize
	    ||
	    cur_sp >= asp->ss_sp && cur_sp < asp->ss_sp + asp->ss_size &&
	    ALIGN_SP(cur_sp) - size >= (unsigned)asp->ss_sp) {
		/*
		** Assume users stack goes from high to low.  u.u_ar0[UESP]
		** points to the users stack pointer.  This is lowest
		** address in the stack used by the process. Point to the
		** next lowest word aligned address that can fully contain a
		** new word plus the size (eg. if addr is xxxxxxx7 next word
		** ptr is at xxxxxxx0)
		*/
		addr = (_VOID *)(cur_sp - size);
		addr = (_VOID *)ALIGN_SP(addr);

		*where = UMEM_STACK;
	} else {
		/*
		** Either we're not executing on the user or alternate
		** stack, or there's not enough room on the alternate stack.
		** So, we let the system pick the address.
		*/
		map_addr(&addr, size, (off_t)0, 1);
		if(addr != NULL &&
		   as_map(u.u_procp->p_as, addr, size,segvn_create,zfod_argsp))
			addr = NULL;	/* as_map() failed */
		else
			*where = UMEM_NEWSEG;
	}

	return addr;
}


/*
** Free space allocated by umem_alloc().  `size' should be the size requested
** of umem_alloc(), and `where' should be the corresponding value returned by
** umem_alloc();
*/
void
umem_free(addr, size, where)
	_VOID	*addr;
	size_t	size;
	int	where;
{
	/*
	** We only do any real work if we need to unmap a segment.  If the
	** space was allocated on the user's stack, there's nothing to be
	** done.
	*/
	if (where == UMEM_NEWSEG && addr != NULL)
		(void)as_unmap(u.u_procp->p_as, addr, size);
}

/* End Enhanced Application Compatibility Support */
