/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5fontlib:font/format.c	1.1"
/* $XConsortium: format.c,v 1.2 91/05/13 16:38:48 gildea Exp $ */

/*
 * Copyright 1990, 1991 Network Computing Devices;
 * Portions Copyright 1987 by Digital Equipment Corporation and the
 * Massachusetts Institute of Technology
 *
 * and its documentation to Members and Affiliates of the MIT X Consortium
 * any purpose and without fee is hereby granted, provided
 *
 * NETWORK COMPUTING DEVICES, DIGITAL AND MIT DISCLAIM ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *
 * @(#)format.c	4.1	91/05/02
 *
 */

#include	"FSproto.h"
#include	"font.h"

int
CheckFSFormat(format, fmask, bit, byte, scan, glyph, image)
    fsBitmapFormat format;
    fsBitmapFormatMask fmask;
    int        *bit,
               *byte,
               *scan,
               *glyph,
               *image;
{
    /* convert format to what the low levels want */
    if (fmask & BitmapFormatMaskBit) {
	*bit = format & BitmapFormatBitOrderMask;
	*bit = (*bit == BitmapFormatBitOrderMSB) ? MSBFirst : LSBFirst;
    }
    if (fmask & BitmapFormatMaskByte) {
	*byte = format & BitmapFormatByteOrderMask;
	*byte = (*byte == BitmapFormatByteOrderMSB) ? MSBFirst : LSBFirst;
    }
    if (fmask & BitmapFormatMaskScanLineUnit) {
	*scan = format & BitmapFormatScanlineUnitMask;
	/* convert byte paddings into byte counts */
	switch (*scan) {
	case BitmapFormatScanlineUnit8:
	    *scan = 1;
	    break;
	case BitmapFormatScanlineUnit16:
	    *scan = 2;
	    break;
	case BitmapFormatScanlineUnit32:
	    *scan = 4;
	    break;
	default:
	    return BadFontFormat;
	}
    }
    if (fmask & BitmapFormatMaskScanLinePad) {
	*glyph = format & BitmapFormatScanlinePadMask;
	/* convert byte paddings into byte counts */
	switch (*glyph) {
	case BitmapFormatScanlinePad8:
	    *glyph = 1;
	    break;
	case BitmapFormatScanlinePad16:
	    *glyph = 2;
	    break;
	case BitmapFormatScanlinePad32:
	    *glyph = 4;
	    break;
	default:
	    return BadFontFormat;
	}
    }
    if (fmask & BitmapFormatMaskImageRectangle) {
	*image = format & BitmapFormatImageRectMask;

	if (*image != BitmapFormatImageRectMin &&
		*image != BitmapFormatImageRectMaxWidth &&
		*image != BitmapFormatImageRectMax)
	    return BadFontFormat;
    }
    return Successful;
}
