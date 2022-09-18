/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*          copyright       "%c%"   */

#ifndef _IO_PRF_PRF_H    /* wrapper symbol for kernel use */
#define _IO_PRF_PRF_H    /* subject to change without notice */

#ident	"@(#)uts-comm:io/prf/prf.h	1.3"
#ident	"$Header: $"

/*
 *	prf ioctl commands
 */
#define PRF_ENGINE	1	/* return number of processors		*/
#define PRF_STAT	2	/* return 0 or 1, is prf enabled?	*/
#define PRF_MAX		3	/* return number of text symbols loaded	*/
#define PRF_NAMESZ	4	/* return size of symbol names		*/
#define PRF_SIZE	5	/* return total size of prf data	*/
#define PRF_ENABLE	6	/* enable profiling			*/
#define PRF_DISABLE	7	/* disable profiling			*/
#define PRF_LOAD	8	/* load addresses, lock in modules	*/
#define PRF_UNLOAD	9	/* unlock modules, unload addresses	*/

/*
 *	struct mprf - lists symbol addresses and offsets into name table
 *		When reading /dev/prf, the first mprf has the number of
 *		loaded text symbols in mprf_addr, and the size of the 
 *		name table in mprf_offset.
 */
struct mprf {
	unsigned long mprf_addr;	/* text address			*/
	unsigned long mprf_offset;	/* offset into symbol name tbl	*/
};

#endif
