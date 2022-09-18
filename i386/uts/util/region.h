/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_REGION_H	/* wrapper symbol for kernel use */
#define _UTIL_REGION_H	/* subject to change without notice */

/* This header file, region.h is kept for source compatibility for those
 * pre-SVR4.0 applications that may include it, but are not regions-based.
 * SVR4.0 memory management is implemented using VM and will not support
 * any application which depends upon regions.
 */

#ident	"@(#)uts-x86:util/region.h	1.2"
#ident	"$Header: $"

#endif	/* _UTIL_REGION_H */
