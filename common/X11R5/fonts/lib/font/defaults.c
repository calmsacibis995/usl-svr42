/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5fontlib:font/defaults.c	1.1"
/*

 * $XConsortium: defaults.c,v 1.1 91/05/10 14:46:27 keith Exp $
 *
 * Copyright 1990 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Keith Packard, MIT X Consortium
 */

#include    <X11/X.h>
#include    <X11/Xproto.h>
#include    <server/include/servermd.h>

#ifndef DEFAULT_BIT_ORDER
#ifdef BITMAP_BIT_ORDER
#define DEFAULT_BIT_ORDER BITMAP_BIT_ORDER
#else
#define DEFAULT_BIT_ORDER MSBFirst
#endif
#endif

#ifndef DEFAULT_BYTE_ORDER
#ifdef IMAGE_BYTE_ORDER
#define DEFAULT_BYTE_ORDER IMAGE_BYTE_ORDER
#else
#define DEFAULT_BYTE_ORDER MSBFirst
#endif
#endif

#ifndef DEFAULT_GLYPH_PAD
#ifdef GLYPHPADBYTES
#define DEFAULT_GLYPH_PAD GLYPHPADBYTES
#else
#define DEFAULT_GLYPH_PAD 4
#endif
#endif

#ifndef DEFAULT_SCAN_UNIT
#define DEFAULT_SCAN_UNIT 1
#endif

/*
#ifndef DEFAULT_SCAN_UNIT
#define DEFAULT_SCAN_UNIT  32
#endif
*/
FontDefaultFormat (bit, byte, glyph, scan)
    int	    *bit, *byte, *glyph, *scan;
{
    *bit = DEFAULT_BIT_ORDER;
    *byte = DEFAULT_BYTE_ORDER;
    *glyph = DEFAULT_GLYPH_PAD;
    *scan = DEFAULT_SCAN_UNIT;
}
