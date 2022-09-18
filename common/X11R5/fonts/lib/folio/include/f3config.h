/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:include/f3config.h	1.1"
/*
**    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
**   
**                   PROPRIETARY NOTICE (Combined) 
**   
**            This source code is unpublished proprietary 
**            information constituting, or derived under 
**            license from AT&T's UNIX(r) System V. 
**   
**                       Copyright Notice 
**   
**            Notice of copyright on this source code 
**            product does not indicate publication. 
**   
**    Copyright (C) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
**    Copyright (C) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T
**   
**                      All rights reserved. 
**   
**    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
**   
**                    RESTRICTED RIGHTS LEGEND: 
**    Use, duplication, or disclosure by the Government is subject 
**    to restrictions as set forth in subparagraph (c)(1)(ii) of 
**    the Rights in Technical Data and Computer Software clause at 
**    DFARS 52.227-7013 and in similar clauses in the FAR and NASA 
**    FAR Supplement. 
**   
**    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
*/


#ifndef _SH_CONFIG_
#define _SH_CONFIG_

/*
    This file defines a number of types and constants appropriate
    to a specific CPU, Frame-buffer, operating-system and compiler
    combination.

    When porting X11-NeWS to your system, you may have to change
    these types and constants.

    Two "standard" combinations are provided, and one of these
    can be chosen by defining:

    #define STD_BIGEND 1	:standard set for big-endian machines,

    or:

    #define STD_LITTLEEND 1	:standard set for little-endian machines.

    Instead of using these standard sets, you could define a set
    appropriate to your system.

*/

#ifdef DEBUG
#define FAST
#else
#define FAST register
#endif

/*  choose the standard-bigendian set */
#define STD_BIGEND 1

#define _LITTLEENDIAN	1	/* Least significant bit first */
#define _BIGENDIAN	2	/* Most significant bit first */

/*
 * Sgn*		-- for use as signed numbers
 * Unsgn*	-- for use as unsigned numbers and bits
 */

#ifdef STD_BIGEND
/*
 * 32 bit ints and longs, 16 bit shorts, 8 bit chars.   Big endian cpu.
 */
typedef long		Sgn32;
typedef short		Sgn16;
typedef char		Sgn8;

typedef unsigned long	Unsgn32;
typedef unsigned short	Unsgn16;
typedef unsigned char	Unsgn8;

#define CPU_BYTE_ORDER	_BIGENDIAN
#define CPU_BIT_ORDER	_BIGENDIAN
#endif

#ifdef STD_LITTLEEND
/*
 * 32 bit ints and longs, 16 bit shorts, 8 bit chars.   Little endian cpu.
 */
typedef long		Sgn32;
typedef short		Sgn16;
typedef char		Sgn8;

typedef unsigned long	Unsgn32;
typedef unsigned short	Unsgn16;
typedef unsigned char	Unsgn8;

#define CPU_BYTE_ORDER	_LITTLEENDIAN
#define CPU_BIT_ORDER	_LITTLEENDIAN
#endif

/*
 * If your display has a different memory layout than your cpu, 
 * these defines will need to be changed accordingly.
 */
#define	FB_BYTE_ORDER	CPU_BYTE_ORDER
/*
### 12/8/89 kumar: BIT_ORDER FOR VGA SHOUD BE _BIGENDIAN (also for color vga)
*/
#ifdef VGA
#define	FB_BIT_ORDER	_BIGENDIAN
#else
#define	FB_BIT_ORDER	CPU_BIT_ORDER
#endif

#ifdef SYSV

#define MAX_FDS 256	/* seems to be  missing -- nara 890202 */

#ifdef SYSVR3
/* 
 * AT&T System V.3xx specifics 
 */

#ifndef EWOULDBLOCK
#define EWOULDBLOCK EAGAIN
#endif

/* misc. bsd functions.  You"ll run into problems when you try
   to define any of the following, with miscellaneous errors in
   compiling parts of the server and clients.

   bcmp, bcopy, bzero, random, srandom, index, rindex
*/  
#endif /* SYSVR3 */
#endif /*SYSV*/

#ifndef	CAT
#ifdef __STDC__
#define	CAT(a,b) a ## b
#else
#undef	IDENTIFIER
#define	IDENTIFIER(x) x
#define	CAT(a,b) IDENTIFIER(a)b
#endif
#endif	/* CAT */


#endif /* _SH_CONFIG_ */

