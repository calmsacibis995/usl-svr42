/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olbitmaps:bitmaps/smallOut.h	1.2"
#endif

#define smallOut_width 5
#define smallOut_height 5

#ifdef INACTIVE_CURSOR
static unsigned char smallOut_bits[] = {
   0x00,
   0x00,
   0x04, 
   0x0a, 
   0x04};
#else
static unsigned char smallOut_bits[] = {
   0x00,
   0x00,
   0x00,
   0x00,
   0x00
   };
#endif
