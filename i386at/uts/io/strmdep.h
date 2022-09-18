/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_STRMDEP_H	/* wrapper symbol for kernel use */
#define _IO_STRMDEP_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:io/strmdep.h	1.5"
#ident	"$Header: $"

/*
 * This file contains all machine-dependent declarations
 * in STREAMS.
 */

/*
 * Copy data from one data buffer to another.
 * The addresses must be word aligned - if not, use bcopy!
 */

	/*
	 * Porters' Note: if your CCS supports asm functions
	 * you could define strbcpy() as an asm function, here
	 */

#define	strbcpy(s, d, c)	bcopy(s, d, c)

/*
 * save the address of the calling function on the 3b2 to
 * enable tracking of who is allocating message blocks
 */

	/*
	 * Porters' Note: if your CCS supports asm functions
	 * you could define saveaddr() as an asm function, here
	 */

#define saveaddr(funcp)

/*
 * macro to check pointer alignment
 * (true if alignment is sufficient for worst case)
 */

	/*
	 * Porters' Note: the following macro may
	 * be machine dependent
	 */

#define str_aligned(X)	(((uint)(X) & (sizeof(int) - 1)) == 0)

#endif	/* _IO_STRMDEP_H */
