/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:Lower.c	1.2"
/* $XConsortium: Lower.c,v 1.6 91/01/05 17:38:12 converse Exp $ */

/* 
 * Copyright 1988 by the Massachusetts Institute of Technology
 *
 *
 */

#define  XK_LATIN1
#include <X11/keysymdef.h>
#include <X11/Xmu/CharSet.h>

/*
 * ISO Latin-1 case conversion routine
 */

#if NeedFunctionPrototypes
void XmuCopyISOLatin1Lowered(char *dst, _Xconst char *src)
#else
void XmuCopyISOLatin1Lowered(dst, src)
    char *dst, *src;
#endif
{
    register unsigned char *dest, *source;

    for (dest = (unsigned char *)dst, source = (unsigned char *)src;
	 *source;
	 source++, dest++)
    {
	if ((*source >= XK_A) && (*source <= XK_Z))
	    *dest = *source + (XK_a - XK_A);
	else if ((*source >= XK_Agrave) && (*source <= XK_Odiaeresis))
	    *dest = *source + (XK_agrave - XK_Agrave);
	else if ((*source >= XK_Ooblique) && (*source <= XK_Thorn))
	    *dest = *source + (XK_oslash - XK_Ooblique);
	else
	    *dest = *source;
    }
    *dest = '\0';
}

#if NeedFunctionPrototypes
void XmuCopyISOLatin1Uppered(char *dst, _Xconst char *src)
#else
void XmuCopyISOLatin1Uppered(dst, src)
    char *dst, *src;
#endif
{
    register unsigned char *dest, *source;

    for (dest = (unsigned char *)dst, source = (unsigned char *)src;
	 *source;
	 source++, dest++)
    {
	if ((*source >= XK_a) && (*source <= XK_z))
	    *dest = *source - (XK_a - XK_A);
	else if ((*source >= XK_agrave) && (*source <= XK_odiaeresis))
	    *dest = *source - (XK_agrave - XK_Agrave);
	else if ((*source >= XK_slash) && (*source <= XK_thorn))
	    *dest = *source - (XK_oslash - XK_Ooblique);
	else
	    *dest = *source;
    }
    *dest = '\0';
}

#if NeedFunctionPrototypes
int XmuCompareISOLatin1 (_Xconst char *first, _Xconst char *second)
#else
int XmuCompareISOLatin1 (first, second)
    char *first, *second;
#endif
{
    register unsigned char *ap, *bp;

    for (ap = (unsigned char *) first, bp = (unsigned char *) second;
	 *ap && *bp; ap++, bp++) {
	register unsigned char a, b;

	if ((a = *ap) != (b = *bp)) {
	    /* try lowercasing and try again */

	    if ((a >= XK_A) && (a <= XK_Z))
	      a += (XK_a - XK_A);
	    else if ((a >= XK_Agrave) && (a <= XK_Odiaeresis))
	      a += (XK_agrave - XK_Agrave);
	    else if ((a >= XK_Ooblique) && (a <= XK_Thorn))
	      a += (XK_oslash - XK_Ooblique);

	    if ((b >= XK_A) && (b <= XK_Z))
	      b += (XK_a - XK_A);
	    else if ((b >= XK_Agrave) && (b <= XK_Odiaeresis))
	      b += (XK_agrave - XK_Agrave);
	    else if ((b >= XK_Ooblique) && (b <= XK_Thorn))
	      b += (XK_oslash - XK_Ooblique);

	    if (a != b) return (((int) a) - ((int) b));
	}
    }
    return (((int) *ap) - ((int) *bp));
}
