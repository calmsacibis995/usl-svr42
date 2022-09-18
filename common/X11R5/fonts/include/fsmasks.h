/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5fontinc:include/fsmasks.h	1.1"
/* $XConsortium: fsmasks.h,v 1.2 91/05/13 16:46:16 gildea Exp $ */

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
 * @(#)fsmasks.h	4.1	91/05/02
 *
 */

/*
 * masks & values used by the font lib and the font server
 */

#ifndef _FSMASKS_H_
#define _FSMASKS_H_

/* font format macros */
#define BitmapFormatByteOrderMask       (1L << 0)
#define BitmapFormatBitOrderMask        (1L << 1)
#define BitmapFormatImageRectMask       (3L << 2)
#define BitmapFormatScanlinePadMask     (3L << 8)
#define BitmapFormatScanlineUnitMask    (3L << 12)

#define BitmapFormatByteOrderLSB        (0)
#define BitmapFormatByteOrderMSB        (1L << 0)
#define BitmapFormatBitOrderLSB         (0)
#define BitmapFormatBitOrderMSB         (1L << 1)

#define BitmapFormatImageRectMin        (0L << 2)
#define BitmapFormatImageRectMaxWidth   (1L << 2)
#define BitmapFormatImageRectMax        (2L << 2)

#define BitmapFormatScanlinePad8        (0L << 8)
#define BitmapFormatScanlinePad16       (1L << 8)
#define BitmapFormatScanlinePad32       (2L << 8)
#define BitmapFormatScanlinePad64       (3L << 8)

#define BitmapFormatScanlineUnit8       (0L << 12)
#define BitmapFormatScanlineUnit16      (1L << 12)
#define BitmapFormatScanlineUnit32      (2L << 12)
#define BitmapFormatScanlineUnit64      (3L << 12)

#define BitmapFormatMaskByte            (1L << 0)
#define BitmapFormatMaskBit             (1L << 1)
#define BitmapFormatMaskImageRectangle  (1L << 2)
#define BitmapFormatMaskScanLinePad     (1L << 3)
#define BitmapFormatMaskScanLineUnit    (1L << 4)

typedef unsigned long  fsBitmapFormat;
typedef unsigned long  fsBitmapFormatMask;

#endif	/* _FSMASKS_H_ */
