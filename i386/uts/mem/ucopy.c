/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:mem/ucopy.c	1.2.2.4"
#ident	"$Header: $"

#include <mem/faultcatch.h>
#include <mem/vmparam.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/sysmacros.h>
#include <util/types.h>

/*
 * This is a collection of routines that access user addresses.
 * They provide protection against user address page fault errors using
 * CATCH_FAULTS, and validate user permissions.
 */

/*
 * Copy "cnt" bytes from (user space) source address "src"
 * to (kernel space) destination address "dst", catching specified faults.
 */
int
ucopyin(src, dst, cnt, catch_flags)
	caddr_t	src, dst;
	u_int	cnt, catch_flags;
{
	if (cnt == 0)		/* Explicit check for 0 count,		   */
		return 0;	/*   since it will fail VALID_USR_RANGE(). */
	if (!VALID_USR_RANGE(src, cnt))
		return EFAULT;
	CATCH_FAULTS(catch_flags | CATCH_UFAULT) {
		bcopy(src, dst, cnt);
	}
	return END_CATCH();
}

/*
 * An older and widely-used interface, frozen for compatibility.
 * Copy "cnt" bytes from (user space) source address "src"
 * to (kernel space) destination address "dst".
 * RFS hook for copying from remote source.
 */
int
copyin(src, dst, cnt)
	caddr_t	src, dst;
	u_int	cnt;
{
	if (RF_SERVER())
		return rcopyin(src, dst, cnt, 0);
	if (cnt == 0)		/* Explicit check for 0 count,		   */
		return 0;	/*   since it will fail VALID_USR_RANGE(). */
	if (!VALID_USR_RANGE(src, cnt))
		return -1;
	CATCH_FAULTS(CATCH_UFAULT)
		bcopy(src, dst, cnt);
	return END_CATCH() ? -1 : 0;
}

/*
 * Copy "cnt" bytes from (kernel space) source address "src"
 * to (user space) destination address "dst", catching specified faults.
 */
int
ucopyout(src, dst, cnt, catch_flags)
	caddr_t	src, dst;
	u_int	cnt, catch_flags;
{
	if (cnt == 0)		/* Explicit check for 0 count,		   */
		return 0;	/*   since it will fail VALID_USR_RANGE(). */
	if (!WRITEABLE_USR_RANGE(dst, cnt))
		return EFAULT;
	CATCH_FAULTS(catch_flags | CATCH_UFAULT) {
		bcopy(src, dst, cnt);
	}
	END_USERWRITE();
	return END_CATCH();
}

/*
 * An older and widely-used interface, frozen for compatibility.
 * Copy "cnt" bytes from (kernel space) source address "src"
 * to (user space) destination address "dst".
 * RFS hook for copying to remote destination.
 */
int
copyout(src, dst, cnt)
	caddr_t	src, dst;
	u_int	cnt;
{
	if (RF_SERVER())
		return rcopyout(src, dst, cnt, NULL);
	if (cnt == 0)		/* Explicit check for 0 count,		   */
		return 0;	/*   since it will fail VALID_USR_RANGE(). */
	if (!WRITEABLE_USR_RANGE(dst, cnt))
		return -1;
	CATCH_FAULTS(CATCH_UFAULT)
		bcopy(src, dst, cnt);
	END_USERWRITE();
	return END_CATCH() ? -1 : 0;
}

/*
 * Fetch byte from user space, local machine only.
 */
int
lfubyte(addr)
	caddr_t	addr;
{
	int	val;

	if (!VALID_USR_RANGE(addr, sizeof *addr))
		return -1;
	CATCH_FAULTS(CATCH_UFAULT)
		val = *(unsigned char *)addr;
	if (END_CATCH() != 0)
		return -1;
	return val;
}

/*
 * Fetch byte from user space, with RFS hook for remote source.
 */
int
fubyte(addr)
	caddr_t	addr;
{
	if (RF_SERVER())
		return rfubyte(addr);
	return lfubyte(addr);
}

/*
 * Fetch byte from user instruction space, with RFS hook for remote source.
 * On machines supporting separate instruction and data spaces fubyte
 * fetches from data space, while fuibyte fetches from instruction space.
 * Machines not having separate I&D define the two routines identically.
 */
int
fuibyte(addr)
	caddr_t	addr;
{
	return fubyte(addr);
}

/*
 * Fetch word from user space, local machine only.
 */
int
lfuword(addr)
	int	*addr;
{
	int	val;

	if (!VALID_USR_RANGE((caddr_t)addr, sizeof *addr))
		return -1;
	CATCH_FAULTS(CATCH_UFAULT)
		val = *addr;
	if (END_CATCH() != 0)
		return -1;
	return val;
}

/*
 * Fetch word from user space, with RFS hook for remote source.
 */
int
fuword(addr)
	int	*addr;
{
	if (RF_SERVER())
		return rfuword(addr);
	return lfuword(addr);
}

/*
 * Fetch word from user instruction space, with RFS hook for remote source.
 * On machines supporting separate instruction and data spaces fuword
 * fetches from data space, while fuiword fetches from instruction space.
 * Machines not having separate I&D define the two routines identically.
 */
int
fuiword(addr)
	int	*addr;
{
	return fuword(addr);
}

/*
 * Set byte in user space, local machine only.
 */
int
lsubyte(addr, val)
	caddr_t	addr;
	char	val;
{
	if (!WRITEABLE_USR_RANGE(addr, sizeof *addr))
		return -1;
	CATCH_FAULTS(CATCH_UFAULT)
		*addr = val;
	END_USERWRITE();
	if (END_CATCH() != 0)
		return -1;
	return 0;
}

/*
 * Set byte in user space, with RFS hook for remote destination.
 */
#if defined(__STDC__)
int
subyte(caddr_t addr, char val)
#else
int
subyte(addr, val)
	caddr_t	addr;
	char	val;
#endif
{
	if (RF_SERVER())
		return rsubyte(addr, val);
	return lsubyte(addr, val);
}

/*
 * Set byte in user instruction space, with RFS hook for remote destination.
 * On machines supporting separate instruction and data spaces subyte
 * sets in data space, while suibyte sets in instruction space.
 * Machines not having separate I&D define the two routines identically.
 */
int
suibyte(addr, val)
	caddr_t	addr;
	char	val;
{
	return subyte(addr, val);
}

/*
 * Set word in user space, local machine only.
 */
int
lsuword(addr, val)
	int	*addr;
	int	val;
{
	if (!WRITEABLE_USR_RANGE((caddr_t)addr, sizeof *addr))
		return -1;
	CATCH_FAULTS(CATCH_UFAULT)
		*addr = val;
	END_USERWRITE();
	if (END_CATCH() != 0)
		return -1;
	return 0;
}

/*
 * Set word in user space, with RFS hook for remote destination.
 */
int
suword(addr, val)
	int	*addr;
	int	val;
{
	if (RF_SERVER())
		return rsuword(addr, val);
	return lsuword(addr, val);
}

/*
 * Set word in user instruction space, with RFS hook for remote destination.
 * On machines supporting separate instruction and data spaces suword
 * sets in data space, while suiword sets in instruction space.
 * Machines not having separate I&D define the two routines identically.
 */
int
suiword(addr, val)
	int	*addr;
	int	val;
{
	return suword(addr, val);
}

/*
 * Service routine for profiling.
 * Scale user program counter value "pc" to fit the user's profiling
 * buffer and increment the appropriate bucket by the specified number
 * of ticks.  If a fault occurs, disable profiling for this process.
 */
void
addupc(pc, ticks)
	void	(*pc)();
	int	ticks;
{
	register int	offset;
	register u_short *bucket_p;

	if ((offset = (int)pc - (int)u.u_prof.pr_off) < 0)
		return;
	offset = upc_scale(offset, u.u_prof.pr_scale);
	if (offset >= u.u_prof.pr_size / sizeof (u_short))
		return;
	bucket_p = u.u_prof.pr_base + offset;
	if (!WRITEABLE_USR_RANGE((caddr_t)bucket_p, sizeof (u_short))) {
		u.u_prof.pr_scale = 0;
		return;
	}
	CATCH_FAULTS(CATCH_UFAULT) {
		*bucket_p += ticks;
	}
	END_USERWRITE();
	if (END_CATCH() != 0)
		u.u_prof.pr_scale = 0;
}

/*
 * Get a pathname from user space into a caller-supplied buffer
 * of size maxbufsize.  Returns -1 if a fault occurs, and -2
 * if the pathname is too long for the supplied buffer.
 * On success, returns the length of the pathname.
 */
int
upath(from, to, maxbufsize)
	caddr_t	from, to;
	size_t	maxbufsize;
{
	int	len;

	if (!VALID_USR_RANGE(from, 1))
		return -1;
	CATCH_FAULTS(CATCH_UFAULT) {
		len = spath(from, to, maxbufsize);
	}
	if (END_CATCH() != 0)
		return -1;
	return len;
}

/*
 * Zero a user buffer with protection against user page fault errors.
 * A fault error in any pageable user address will cause a non-zero
 * errno to be returned.
 */
int
uzero(dst, cnt)
	caddr_t	dst;
	u_int	cnt;
{
	if (!WRITEABLE_USR_RANGE(dst, cnt))
		return EFAULT;
	CATCH_FAULTS(CATCH_UFAULT)
		bzero(dst, cnt);
	END_USERWRITE();
	return END_CATCH();
}

/*
 * fc_jmpjmp -- Standard handler for catching page fault errors.
 * At startup time, u.u_fault_catch.fc_func gets set to this (see faultcatch.h).
 *
 * NOTE: This function should really be in a different file, something like
 * a pagefault.c if and when such exists.
 */
void
fc_jmpjmp()
{
	longjmp(&u.u_fault_catch.fc_jmp);
}
