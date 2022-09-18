/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:mem/memprobe.c	1.1.2.3"
#ident	"$Header: $"

#include <mem/faultcatch.h>
#include <proc/user.h>
#include <util/debug.h>
#include <util/types.h>

/*
 * memprobe(vaddr)
 * Assumes vaddr is a potentially valid virtual address.
 * Returns 0 if the referenced page is valid, non-zero otherwise.
 */

int
memprobe(vaddr)
	caddr_t	vaddr;
{
	volatile char dummy;

	CATCH_FAULTS(CATCH_ALL_FAULTS)
		dummy = *(char *)vaddr;
	return END_CATCH();
}
