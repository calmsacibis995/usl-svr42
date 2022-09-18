/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olbitmaps:bitmaps/panmask.h	1.1"
#endif

#define panmask_width 18
#define panmask_height 14
#define panmask_x_hot 1
#define panmask_y_hot 12
static unsigned char panmask_bits[] = {
   0xc0, 0xf8, 0x03, 0xe0, 0xfd, 0x03, 0xe0, 0xff, 0x03, 0xf0, 0xff, 0x03,
   0xf0, 0x0f, 0x00, 0xf8, 0x1f, 0x00, 0xf8, 0x3f, 0x00, 0xfc, 0x3f, 0x00,
   0xfc, 0x1f, 0x00, 0xfe, 0x07, 0x00, 0xfe, 0x01, 0x00, 0x7f, 0x00, 0x00,
   0x1f, 0x00, 0x00, 0x07, 0x00, 0x00};
