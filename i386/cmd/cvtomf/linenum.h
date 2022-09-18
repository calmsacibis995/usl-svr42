/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cvtomf:linenum.h	1.1"

/*
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1989 The Santa Cruz Operation, Inc
 *		      All Rights Reserved
 */
/* Enhanced Application Compatibility Support */
#ifndef _LINENUM_H
#define _LINENUM_H


/*  There is one line number entry for every 
    "breakpointable" source line in a section.
    Line numbers are grouped on a per function
    basis; the first entry in a function grouping
    will have l_lnno = 0 and in place of physical
    address will be the symbol table index of
    the function name.
*/
struct lineno
{
	union
	{
		long	l_symndx ;	/* sym. table index of function name
						iff l_lnno == 0      */
		long	l_paddr ;	/* (physical) address of line number */
	}		l_addr ;
	unsigned short	l_lnno ;	/* line number */
} ;

#define	LINENO	struct lineno
#define	LINESZ	6	/* sizeof(LINENO) */

#endif	/* _LINENUM_H */
/* End Enhanced Application Compatibility Support */

