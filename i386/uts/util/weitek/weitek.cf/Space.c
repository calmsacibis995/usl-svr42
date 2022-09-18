/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:util/weitek/weitek.cf/Space.c	1.3"
#ident	"$Header: $"

#include <config.h>
#include <sys/weitek.h>

/*  All bit fields are #define'd in weitek.h.
 *  must always have WFPB17, and WFPB24.
 *  can have one of WFPR? plus one of WFPRI?,
 *  and to turn off a particular exception, may have any of 
 *  the exception bit masks.
 */
unsigned long   weitek_cfg = WFPRN | WFPRIZ | WFPB24 | WFPPM | WFPUM | WFPB17;

unsigned long	weitek_paddr = 0xC0000000;	/* chip physical address */

int	weitek_pt = -1;				/* No weitek page table as yet */
