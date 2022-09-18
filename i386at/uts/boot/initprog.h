/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _BOOT_INITPROG_H	/* wrapper symbol for kernel use */
#define _BOOT_INITPROG_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:boot/initprog.h	1.4"
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

#ifndef _BOOT_BOOTLINK_H
#include <boot/bootlink.h>
#endif	/* _BOOT_BOOTLINK_H */

#define printf	(bfp->b_printf)
#define strcpy	(bfp->b_strcpy)
#define strncpy	(bfp->b_strncpy)
#define strcat	(bfp->b_strcat)
#define strlen	(bfp->b_strlen)
#define memcpy	(bfp->b_memcpy)
#define memcmp	(bfp->b_memcmp)
#define memset	(bfp->b_memset)
#define getchar	(bfp->b_getchar)
#define putchar	(bfp->b_putchar)
#define bgets	(bfp->b_bgets)
#define ischar	(bfp->b_ischar)
#define doint	(bfp->b_doint)
#define goany	(bfp->b_goany)
#define CMOSread	(bfp->b_CMOSread)
#define shomem	(bfp->b_shomem)
#define memwrap	(bfp->b_memwrap)

/*
 *	The following must be declared and set by the initprog 
 *	program when it is called from boot.
 */
extern	struct	bootfuncs	*bfp;
extern	struct	bootenv		*btep;
#define BTEP_INFO (btep->bootinfo)

#endif	/* _BOOT_INITPROG_H */
