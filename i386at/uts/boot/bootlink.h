/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _BOOT_BOOTLINK_H	/* wrapper symbol for kernel use */
#define _BOOT_BOOTLINK_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:boot/bootlink.h	1.4"
#ident  "$Header: $"

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

/*	Structure for BIOS int call requests. Register values set by
/*	the caller and modified to the value after the int call by the
/*	int service routine. */

#ifndef _UTIL_TYPES_H
#include <util/types.h>
#endif	/* _UTIL_TYPES_H */

struct int_pb {
	ushort	intval, 	/* INT value to make int x instruction */
		ax, bx,		/* input and returned */
		cx, dx, 
		bp, es,
		si;		/* returned only */
};

/*	functions defined in boot and sharable by the
 *	initialization programs.
 */
struct bootfuncs {
	int	(*b_printf)();
	char *	(*b_strcpy)();
	char *	(*b_strncpy)();
	char *	(*b_strcat)();
	int	(*b_strlen)();
	char *	(*b_memcpy)();
	int	(*b_memcmp)();
	unchar	(*b_getchar)();
	int	(*b_putchar)();
	int	(*b_bgets)();
	int	(*b_ischar)();
	int	(*b_doint)();
	void	(*b_goany)();
	unchar	(*b_CMOSread)();
	char *	(*b_memset)();
	int	(*b_shomem)();
	int	(*b_memwrap)();
};

#endif	/* _BOOT_BOOTLINK_H */
