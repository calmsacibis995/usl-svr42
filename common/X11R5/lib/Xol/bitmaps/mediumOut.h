/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olbitmaps:bitmaps/mediumOut.h	1.2"
#endif

#define mediumOut_width 9
#define mediumOut_height 8

#ifdef INACTIVE_CURSOR
static unsigned char mediumOut_bits[] = {
   0x00, 0x00,
   0x00, 0x00,
   0x00, 0x00,
   0x10, 0x00, 
   0x28, 0x00, 
   0x54, 0x00, 
   0x28, 0x00, 
   0x10, 0x00
};
#else
static unsigned char mediumOut_bits[] = {
   0x00, 0x00, 
   0x00, 0x00, 
   0x00, 0x00, 
   0x00, 0x00, 
   0x00, 0x00, 
   0x00, 0x00, 
   0x00, 0x00, 
   0x00, 0x00, 
};
#endif
