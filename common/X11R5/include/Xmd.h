/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5include:Xmd.h	1.2"
/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/
#ifndef XMD_H
#define XMD_H 1
/* $XConsortium: Xmd.h,v 1.41 91/05/10 10:00:03 jap Exp $ */
/*
 *  Xmd.h: MACHINE DEPENDENT DECLARATIONS.
 */

/*
 * Special per-machine configuration flags.
 */
#ifdef CRAY
#define WORD64				/* 64-bit architecture */
#define UNSIGNEDBITFIELDS		/* bit fields do not honor sign */
#endif


/*
 * Stuff to handle large architecture machines; the constants were generated
 * on a 32-bit machine and must coorespond to the protocol.
 */
#ifdef WORD64
#define MUSTCOPY
#endif /* WORD64 */


/*
 * Definition of macro used to set constants for size of network structures;
 * machines with preprocessors that can't handle all of the sz_ symbols
 * can define this macro to be sizeof(x) if and only if their compiler doesn't
 * pad out structures (esp. the xTextElt structure which contains only two 
 * one-byte fields).  Network structures should always define sz_symbols.
 *
 * The sz_ prefix is used instead of something more descriptive so that the
 * symbols are no more than 32 characters long (which causes problems for some
 * compilers and preprocessors).
 *
 * The extra indirection in the __STDC__ case is to get macro arguments to
 * expand correctly before the concatenation, rather than afterward.
 */
#if defined(__STDC__) && !defined(UNIXCPP)
#define _SIZEOF(x) sz_##x
#define SIZEOF(x) _SIZEOF(x)
#else
#define SIZEOF(x) sz_/**/x
#endif /* if ANSI C compiler else not */

/*
 * Bitfield suffixes for the protocol structure elements, if you
 * need them.  Note that bitfields are not guarranteed to be signed
 * (or even unsigned) according to ANSI C.
 */
#ifdef WORD64
#define B32 :32
#define B16 :16
#else
#define B32
#define B16
#endif

typedef long           INT32;
typedef short          INT16;
#if defined(__STDC__) || defined(sgi) || defined(AIXV3)
typedef signed char    INT8;
#else
typedef char           INT8;
#endif

typedef unsigned long CARD32;
typedef unsigned short CARD16;
typedef unsigned char  CARD8;

typedef unsigned long		BITS32;
typedef unsigned short		BITS16;
typedef unsigned char		BYTE;

typedef unsigned char            BOOL;


/*
 * definitions for sign-extending bitfields on 64-bit architectures
 */
#if defined(WORD64) && defined(UNSIGNEDBITFIELDS)
#define cvtINT8toInt(val)   (((val) & 0x00000080) ? ((val) | 0xffffffffffffff00) : (val))
#define cvtINT16toInt(val)  (((val) & 0x00008000) ? ((val) | 0xffffffffffff0000) : (val))
#define cvtINT32toInt(val)  (((val) & 0x80000000) ? ((val) | 0xffffffff00000000) : (val))
#define cvtINT8toShort(val)  cvtINT8toInt(val)
#define cvtINT16toShort(val) cvtINT16toInt(val)
#define cvtINT32toShort(val) cvtINT32toInt(val)
#define cvtINT8toLong(val)  cvtINT8toInt(val)
#define cvtINT16toLong(val) cvtINT16toInt(val)
#define cvtINT32toLong(val) cvtINT32toInt(val)
#else
#define cvtINT8toInt(val) (val)
#define cvtINT16toInt(val) (val)
#define cvtINT32toInt(val) (val)
#define cvtINT8toShort(val) (val)
#define cvtINT16toShort(val) (val)
#define cvtINT32toShort(val) (val)
#define cvtINT8toLong(val) (val)
#define cvtINT16toLong(val) (val)
#define cvtINT32toLong(val) (val)
#endif /* WORD64 and UNSIGNEDBITFIELDS */



#ifdef MUSTCOPY
/*
 * This macro must not cast or else pointers will get aligned and be wrong
 */
#define NEXTPTR(p,t)  (((char *) p) + SIZEOF(t))
#else /* else not MUSTCOPY, this is used for 32-bit machines */
/*
 * this version should leave result of type (t *), but that should only be 
 * used when not in MUSTCOPY
 */  
#define NEXTPTR(p,t) (((t *)(p)) + 1)
#endif /* MUSTCOPY - used machines whose C structs don't line up with proto */

#endif /* XMD_H */
