/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_BITMASKS_H	/* wrapper symbol for kernel use */
#define _UTIL_BITMASKS_H	/* subject to change without notice */

#ident	"@(#)uts-x86:util/bitmasks.h	1.3"
#ident	"$Header: $"

/*
 * setmask[N] has the low order N bits set.  For example,
 * setmask[5] == 0x1F.
 */

extern int setmask[];

/*
 * sbittab[N] has the Nth bit set.  For example,
 * sbittab[5] == 0x20.
 */

extern int sbittab[];

/*
 * cbittab[N] has all bits on, except for the Nth bit which is off.
 * For example, cbittab[5] == 0xFFFFFFDF.
 */

extern int cbittab[];

#endif	/* _UTIL_BITMASKS_H */
