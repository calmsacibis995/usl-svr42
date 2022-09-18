/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

#ifndef machineItype_h
#define machineItype_h
#ident	"@(#)debugger:inc/i386/Itype.Mach.h	1.2"

#include "fpemu.h"
/* for extended precision floating point we use a 
 * floating point emulation package to get around
 * lack of support for long double in cfront 1.2
 */
typedef char		Ichar;	/* signed target char */
typedef	char		Iint1;	/* signed 1 byte int */
typedef	short		Iint2;	/* signed 2 byte int */
typedef	long		Iint4;	/* signed 4 byte int */

typedef unsigned char	Iuchar;	/* unsigned target char */
typedef	unsigned char	Iuint1;	/* unsigned 1 byte int */
typedef	unsigned short	Iuint2;	/* unsigned 2 byte int */
typedef	unsigned long	Iuint4;	/* unsigned 4 byte int */

typedef float		Isfloat; /* ANSI single prec. floating pt */
typedef double		Idfloat; /* ANSI double precision floating pt */
typedef fp_x_t		Ixfloat; /* extended precision floating pt */

#include "Iaddr.h"
/*typedef unsigned long Iaddr;	holds a target address */
typedef unsigned long	Ibase;	/* holds a target segment base */
typedef unsigned long	Ioffset; /* holds a target segment base */

#endif

