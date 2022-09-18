/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/vtio.c	1.3"

/*
 *	Copyright (c) 1991 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyright (c) 1988, 1989, 1990 AT&T
 *	All Rights Reserved 
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 *	All rights reserved.
 */

#define TSSBITMAP 1			/* so KDENABIO works... */
#define	VPIX	  1			/* so KIOCINFO works... */

#include <fcntl.h>
#include <signal.h>
#include "Xmd.h"
#include "sidep.h"
#include "sys/types.h"
#include "sys/at_ansi.h"
#include "sys/kd.h"
#include "vtio.h"
#include "sys/vt.h"
#include "sys/inline.h"
#include "vgaregs.h"
#include "v256.h"

struct v256_regs inittab256[] = { 	/* V256 register initial values */
/* Type 0, STB V256 640x480 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x06,
	/* misc */
	0xE3,
	/* CRTC */
	0x5E, 0x4F, 0x50, 0x01, 0x55, 0x9F, 0x0B, 0x3E,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xEB, 0x2D, 0xDF, 0x28, 0x00, 0xE4, 0x08, 0xC3, 0xFF,

/* Type 1, Generic Tseng Labs 640x480 256 colors */
	/* sequencer */
	0x01, 0x03, 0x0F, 0x00, 0x06,
	/* misc */
	0xE3,
	/* CRTC */
	0x5E, 0x4F, 0x50, 0x01, 0x55, 0x9F, 0x0B, 0x3E,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xEB, 0x2D, 0xDF, 0x28, 0x00, 0xE4, 0x08, 0xC3, 0xFF,

/* Type 2, STB V256 800x600 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x06,
	/* misc */
	0xEB,
	/* CRTC */
	0x7F, 0x63, 0x64, 0x02, 0x65, 0x12, 0x77, 0xF0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5E, 0x21, 0x57, 0x32, 0x00, 0x59, 0x6B, 0xC3, 0xFF,

/* Type 3, STB V256 1024x768 256 colors [ET4000 only] */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x06,
	/* misc */
	0xEF,
	/* CRTC */
	0xA1, 0x7F, 0x80, 0x04, 0x89, 0x9F, 0x26, 0xFD,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x08, 0x2A, 0xFF, 0x80, 0x00, 0x04, 0x22, 0xC3, 0xFF,

/* Type 4, STB V256 640x400 256 colors [ET4000 only] */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x06,
	/* misc */
	0x63,
	/* CRTC */
	0x5E, 0x4F, 0x50, 0x81, 0x54, 0x9F, 0xBF, 0x1F,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x9C, 0x2E, 0x8F, 0x28, 0x00, 0x96, 0xB9, 0xC3, 0xFF,

/* Type 5, Standard VGA 320x200 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0x63,
	/* CRTC */
	0x5f, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
	0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x9C, 0x2E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xa3, 0xFF,

/* Type 6, Orchid ProDesigner 800x600 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x06,
	/* misc */
	0xE7,
	/* CRTC */
	0x77, 0x63, 0x64, 0x06, 0x66, 0xaa, 0x77, 0xF0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x63, 0x22, 0x57, 0x32, 0x00, 0x5b, 0x75, 0xC3, 0xFF,

/* Type 7, Orchid Designer 800 800x600 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x06,
	/* misc */
	0xeb,
	/* CRTC */
	0x7a, 0x63, 0x64, 0x1d, 0x68, 0xbb, 0x78, 0xF0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x60, 0x24, 0x57, 0x32, 0x00, 0x5b, 0x76, 0xC3, 0xFF,

/* Type 8, Orchid VGA 800x600 256 colors  fixed freq monitor */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x06,
	/* misc */
	0xa7,
	/* CRTC */
	0x86, 0x63, 0x64, 0x15, 0x6f, 0x90, 0x60, 0x1f,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x3c, 0x11, 0x2b, 0x32, 0x00, 0x30, 0x63, 0xC3, 0xFF,

/* Type 9, Genoa VGA 800x600 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x06,
	/* misc */
	0xeb,
	/* CRTC */
	0x85, 0x63, 0x65, 0x05, 0x6d, 0x19, 0x6e, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x58, 0x0e, 0x57, 0x32, 0x00, 0x5b, 0x65, 0xC3, 0xFF,

/* Type 10, Video 7 VRAM VGA 640x400 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0x43,
	/* CRTC */
	0xc6, 0x9f, 0xa3, 0x86, 0xaa, 0x00, 0xe0, 0x10,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xce, 0x05, 0xc7, 0x50, 0x00, 0xcc, 0xdc, 0xe7, 0xFF,

/* Type 11, Video 7 VRAM VGA 640x480 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0xc3,
	/* CRTC */
	0xc3, 0x9f, 0xa2, 0x85, 0xa8, 0x00, 0x0b, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xea, 0x0c, 0xdf, 0x50, 0x00, 0xe7, 0x04, 0xe3, 0xff,

/* Type 12, Video 7 VRAM VGA 720x540 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0xc7,
	/* CRTC */
	0xe3, 0xb3, 0xb6, 0x85, 0xb8, 0x8a, 0x47, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x26, 0x08, 0x1b, 0x5a, 0x00, 0x24, 0x3f, 0xe3, 0xff,

/* Type 13, Video 7 VRAM VGA 800x600 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0xc7,
	/* CRTC */
	0xf2, 0xc8, 0xc7, 0x94, 0xcb, 0x80, 0x70, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x59, 0x0c, 0x57, 0x64, 0x00, 0x59, 0x6a, 0xe3, 0xff,

/* Type 14, Video 7 VRAM VGA 800x600 256 colors on Nanao 9070s */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0xc7,
	/* CRTC */
	0xe3, 0xc8, 0xc7, 0x85, 0xcc, 0x9f, 0x6f, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5b, 0x0d, 0x57, 0x64, 0x00, 0x5b, 0x6d, 0xe3, 0xff,

/* Type 15, ATI VGA Wonder 640x480 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0a,
	/* misc */
	0xe3,
	/* CRTC */
	0x5f, 0x4f, 0x50, 0x82, 0x55, 0x81, 0x0b, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xea, 0x0c, 0xdf, 0x28, 0x00, 0xe7, 0x04, 0xe3, 0xff,

/* Type 16, ATI VGA Wonder 800x600 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0a,
	/* misc */
	0xef,
	/* CRTC */
	0x7a, 0x63, 0x65, 0x9d, 0x69, 0x92, 0x38, 0x1f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x2d, 0x0e, 0x2c, 0x32, 0x0f, 0x30, 0x34, 0xe7, 0xff,

/* Type 17, Techmar VGA 800x600 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x06,
	/* misc */
	0xab,
	/* CRTC */
	0x7b, 0x63, 0x64, 0x1e, 0x67, 0x90, 0x6f, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x59, 0x2b, 0x57, 0x32, 0x00, 0x58, 0x6f, 0xe3, 0xFF,

/* Type 18, Paradise 640x400 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0x63,
	/* CRTC */
	0x5f, 0x4f, 0x50, 0x82, 0x53, 0x9f, 0xbf, 0x1f,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x9c, 0x0e, 0x8f, 0x50, 0x40, 0x96, 0xb9, 0xa3, 0xFF,

/* Type 19, Paradise 640x480 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0xe3,
	/* CRTC */
	0x5f, 0x4f, 0x50, 0x82, 0x53, 0x9f, 0x0b, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xea, 0x0c, 0xdf, 0x50, 0x40, 0xe7, 0x04, 0xe3, 0xFF,

/* Type 20, Video 7 1024i VGA 640x400 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0x47,
	/* CRTC */
	0xbc, 0x9f, 0xa2, 0x9e, 0xa4, 0x9d, 0xbf, 0x1f,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x9c, 0x0e, 0x8f, 0x50, 0x00, 0x96, 0xb9, 0xe3, 0xff,

/* Type 21, Video 7 1024i VGA 640x480 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0xc7,
	/* CRTC */
	0xbc, 0x9f, 0xa2, 0x9e, 0xa4, 0x9d, 0x0b, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xea, 0x0c, 0xdf, 0x50, 0x00, 0xe7, 0x04, 0xe3, 0xff,

/* Type 22, Tseng Labs ET4000 VGA 640x480 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0xe3,
	/* CRTC */
	0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xea, 0x0c, 0xdf, 0x50, 0x60, 0xe7, 0x04, 0xab, 0xFF,

/* Type 23, Tseng Labs ET4000 VGA 800x600 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0xef,
	/* CRTC */
	0x7f, 0x63, 0x64, 0x02, 0x6a, 0x1d, 0x77, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5d, 0x0f, 0x57, 0x64, 0x60, 0x5b, 0x74, 0xab, 0xff,

/* Type 24, Tseng Labs ET4000 VGA 800x600 256 colors alternate */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0xeb,
	/* CRTC */
	0x7a, 0x63, 0x64, 0x1d, 0x68, 0x9a, 0x78, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5c, 0x0f, 0x57, 0x64, 0x60, 0x5b, 0x75, 0xab, 0xff,

/* Type 25, Tseng Labs ET4000 VGA 1024x768 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0x27,
	/* CRTC */
	0x99, 0x7f, 0x7f, 0x1d, 0x83, 0x17, 0x2f, 0xf5,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x08, 0x00, 0xff, 0x80, 0x60, 0xff, 0x30, 0xab, 0xff,

/* Type 26, Tseng Labs ET4000 VGA 1024x768 256 colors non-interlaced */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0x2f,
	/* CRTC */
	0xa1, 0x7f, 0x80, 0x04, 0x89, 0x9f, 0x26, 0xfd,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x08, 0x0a, 0xff, 0x80, 0x60, 0x04, 0x22, 0xab, 0xff,

/* Type 27, Orchid Prodesigner II VGA 1024x768 256 colors interlaced */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0x2b,
	/* CRTC */
	0x99, 0x7f, 0x7f, 0x1d, 0x83, 0x17, 0x2f, 0xf5,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x08, 0x00, 0xff, 0x80, 0x60, 0xff, 0x30, 0xab, 0xff,

/* Type 28, STB VGA Extra EM16+ 1024x768 256 colors non-interlaced */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0x3f,
	/* CRTC */
	0xa1, 0x7f, 0x80, 0x04, 0x89, 0x9f, 0x2d, 0xfd,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x06, 0x0a, 0xff, 0x80, 0x60, 0x04, 0x2d, 0xab, 0xff,

/* Type 29, ATI VGA Wonder V4, V5 800x600 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0a,
	/* misc */
	0xef,
	/* CRTC */
	0x7a, 0x63, 0x65, 0x9d, 0x67, 0x92, 0x38, 0x1f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x2d, 0x0e, 0x2b, 0x32, 0x0f, 0x30, 0x34, 0xe7, 0xff,

/* Type 30, Trident VGA 8900 640x480 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0xe3,
	/* CRTC */
	0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xea, 0x0c, 0xdf, 0x28, 0x40, 0xe7, 0x04, 0xa3, 0xff,

/* Type 31, Trident VGA 8900 800x600 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0a,
	/* misc */
	0xef,
	/* CRTC */
	0x7b, 0x63, 0x64, 0x9e, 0x69, 0x8f, 0x6f, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x58, 0x0a, 0x57, 0x32, 0x40, 0x58, 0x6f, 0xa3, 0xff,

/* Type 32, Trident VGA 8900 800x600 (alternate) 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0x2b,
	/* CRTC */
	0x7e, 0x63, 0x64, 0x81, 0x6b, 0x18, 0x99, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x6e, 0x04, 0x57, 0x32, 0x40, 0x5e, 0x93, 0xa3, 0xff,

/* Type 33, Trident VGA 8900 1024x768 (interlaced) 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0x2b,
	/* CRTC */
	0x99, 0x7f, 0x81, 0x1b, 0x83, 0x10, 0x9d, 0x1f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x84, 0x06, 0x7f, 0x80, 0x40, 0x84, 0x98, 0xa3, 0xff,

/* Type 34, Trident VGA 8900 1024x768 (non-interlaced) 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0a,
	/* misc */
	0x27,
	/* CRTC */
	0xa2, 0x7f, 0x80, 0x85, 0x87, 0x90, 0x2c, 0xfd,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0f, 0x01, 0xff, 0x40, 0x40, 0x07, 0x26, 0xa3, 0xff,

/* Type 35, Trident VGA 8800 640x400 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0x6b,
	/* CRTC */
	0xb9, 0x9f, 0xa0, 0x9b, 0xa8, 0xdb, 0xbf, 0x1f,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x9f, 0x0e, 0x8f, 0x50, 0x40, 0x96, 0xb9, 0xa3, 0xff,

/* Type 36, Trident VGA 8800 640x480 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0xeb,
	/* CRTC */
	/* Original values replaced by dumped values.
	0xb9, 0x9f, 0xa0, 0x9b, 0xa8, 0xdb, 0x03, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xe7, 0x0c, 0xdf, 0x50, 0x40, 0xe7, 0xfd, 0xa3, 0xff,
	 */
	0xc3, 0x9f, 0xa1, 0x84, 0xa6, 0x0, 0x0b, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xea, 0x8c, 0xdf, 0x50, 0x40, 0xe7, 0x04, 0xa3, 0xff,

/* Type 37, Sigma VGA Legend 640x480 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0xe3,
	/* CRTC */
	0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xea, 0x0c, 0xdf, 0x50, 0x60, 0xe7, 0x04, 0xab, 0xff,

/* Type 38, Sigma VGA Legend 800x600 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0x6b,
	/* CRTC */
	0x7d, 0x63, 0x64, 0x00, 0x6c, 0x1b, 0x9a, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x6f, 0x05, 0x57, 0x64, 0x60, 0x5f, 0x95, 0xab, 0xff,

/* Type 39, Sigma VGA Legend 800x600 (alternate) 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0x2b,
	/* CRTC */
	0x7f, 0x63, 0x65, 0x9f, 0x70, 0x9d, 0x7f, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x58, 0x0c, 0x57, 0x64, 0x60, 0x58, 0x73, 0xab, 0xff,

/* Type 40, Sigma VGA Legend 1024x768 (interlaced) 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0x23,
	/* CRTC */
	0x99, 0x7f, 0x80, 0x1c, 0x81, 0x19, 0x2f, 0xfd,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x04, 0x01, 0xff, 0x80, 0x60, 0x05, 0x2a, 0xab, 0xff,

/* Type 41, Sigma VGA Legend 1024x768 (non-interlaced) 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0x37,
	/* CRTC */
	0xa1, 0x7f, 0x80, 0x04, 0x8a, 0x9f, 0x26, 0xfd,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x08, 0x0a, 0xff, 0x80, 0x60, 0x04, 0x22, 0xab, 0xff,

/* Type 42, ATI VGA Wonder+ 640x400 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0a,
	/* misc */
	0x63,
	/* CRTC */
	0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0xbf, 0x1f,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x9c, 0x0e, 0x8f, 0x28, 0x1f, 0x96, 0xb9, 0xe3, 0xff,

/* Type 43, ATI VGA Wonder+ 640x480 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0a,
	/* misc */
	0xe3,
	/* CRTC */
	0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xea, 0x0c, 0xdf, 0x28, 0x00, 0xe7, 0x04, 0xe3, 0xff,

/* Type 44, ATI VGA Wonder+ 800x600 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x8a,
	/* misc */
	0x2f,
	/* CRTC */
	0x7a, 0x63, 0x65, 0x9d, 0x67, 0x92, 0x38, 0x1f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x2d, 0x0e, 0x2b, 0x32, 0x0f, 0x32, 0x34, 0xe7, 0xff,

/* Type 45, Microlabs ET3000 800x600 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x06,
	/* misc */
	0xef,
	/* CRTC */
	0x80, 0x63, 0x64, 0x03, 0x67, 0x1c, 0x77, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x60, 0x02, 0x57, 0x32, 0x00, 0x5b, 0x75, 0xc3, 0xff,

/* Type 46, Microlabs ET4000 800x600 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0xef,
	/* CRTC */
	0x7a, 0x63, 0x64, 0x1d, 0x68, 0x9a, 0x78, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5c, 0x0e, 0x57, 0x64, 0x60, 0x5b, 0x75, 0xab, 0xff,

/* Type 47, Microlabs ET4000 alt 800x600 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0xe3,
	/* CRTC */
	0x7f, 0x63, 0x64, 0x02, 0x71, 0x1d, 0x77, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5d, 0x0f, 0x57, 0x64, 0x60, 0x5b, 0x74, 0xab, 0xff,

};

static unchar attributes[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
	0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x41, 0x00, 0x0f, 0x00, 0x00,
};

static unchar graphics[] = {	
	0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0f, 0xff,
};

/*
 * The base address of the adapters based on the type returned by KDDISPTYPE.
 * The 386/ix 2.0 changed the semantices of KDDISPTYPE sturcture. So we now
 * have to use these hard coded physical address values for the console and
 * use the values returned by KDDISPTYPE for other displays. The console is
 * identified by doing a KIOCINFO which returns ('k' << 8) for the console.
 */

static long base_addr[] = {
	0, MONO_BASE, MONO_BASE, COLOR_BASE, EGA_BASE, V256_BASE
};
	
static int
no_ext()
{
	return (SUCCESS);
}

int tli_256_init();
int tli_256_rest();
int v256_tli_selectpage();
int et4000_256_init();
int et4000_256_rest();
int v256_tli_selectpage();
int video7_256_init();
int video7_256_restore();
int v256_video7_selectpage();
int ati_256_init();
int ati_256_rest();
int v256_ati_selectpage();
int pvga1a_256_init();
int pvga1a_256_restore();
int pvga1a_256_selectpage();
int trident_256_init();
int trident_256_restore();
int trident_256_selectpage();
int legend_init();
int legend_rest();
int vt_is_ET4000;

extern	struct	at_disp_info	disp_info[];

struct	at_disp_info	disp_info[] = {	/* display info for support adapters */

	"STB256", "STDVGA", STB_6x4_256, 640, 480, NULL, 5*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[STB_6x4_256]),
	tli_256_init, tli_256_rest, v256_tli_selectpage,

	"STB256", "MULTISYNC", STB_8x6_256, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[STB_8x6_256]),
	tli_256_init, tli_256_rest, v256_tli_selectpage,

	"STB256", "MULTISYNC", TLI_1x7_256, 1024, 768, NULL, 12*64*1024, 64*1024,
	0x3D4, 1024, &(inittab256[TLI_1x7_256]),
	tli_256_init, tli_256_rest, v256_tli_selectpage,

	"STB256", "STDVGA", TLI_400_256, 640, 400, NULL, 4*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[TLI_400_256]),
	tli_256_init, tli_256_rest, v256_tli_selectpage,

	"VGA256", "STDVGA", VGA_256, 320, 200, NULL, 64*1024, 64*1024,
	0x3D4, 320, &(inittab256[VGA_256]),
	no_ext, no_ext, no_ext,

	"ORV256", "STDVGA", TLI_6x4_256, 640, 480, NULL, 5*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[TLI_6x4_256]),
	tli_256_init, tli_256_rest, v256_tli_selectpage,

	"ORV256", "MULTISYNC", ORV256_8, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[ORV256_8]),
	tli_256_init, tli_256_rest, v256_tli_selectpage,

	"ORD800256", "STDVGA", TLI_6x4_256, 640, 480, NULL, 5*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[TLI_6x4_256]),
	tli_256_init, tli_256_rest, v256_tli_selectpage,

	"ORD800256", "MULTISYNC", ORD256_8, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[ORD256_8]),
	tli_256_init, tli_256_rest, v256_tli_selectpage,

	"ORV256f", "MULTISYNC", ORF256_8, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[ORF256_8]),
	tli_256_init, tli_256_rest, v256_tli_selectpage,

	"GENOA256", "STDVGA", TLI_6x4_256, 640, 480, NULL, 5*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[TLI_6x4_256]),
	tli_256_init, tli_256_rest, v256_tli_selectpage,

	"GENOA256", "MULTISYNC", GENOA256_8, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[GENOA256_8]),
	tli_256_init, tli_256_rest, v256_tli_selectpage,

	"MICRO3256", "STDVGA", TLI_6x4_256, 640, 480, NULL, 5*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[TLI_6x4_256]),
	tli_256_init, tli_256_rest, v256_tli_selectpage,

	"MICRO3256", "MULTISYNC", MICRO3256_8, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[MICRO3256_8]),
	tli_256_init, tli_256_rest, v256_tli_selectpage,

	"V7256", "STDVGA", V7256_400, 640, 400, NULL, 4*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[V7256_400]),
	video7_256_init, video7_256_restore, v256_video7_selectpage,

	"V7256", "STDVGA", V7256_6, 640, 480, NULL, 5*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[V7256_6]),
	video7_256_init, video7_256_restore, v256_video7_selectpage,

	"V7256", "MULTISYNC", V7256_7, 720, 540, NULL, 6*64*1024, 64*1024,
	0x3D4, 720, &(inittab256[V7256_7]),
	video7_256_init, video7_256_restore, v256_video7_selectpage,

	"V7256", "MULTISYNC", V7256_8, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[V7256_8]),
	video7_256_init, video7_256_restore, v256_video7_selectpage,

	"V7256a", "MULTISYNC", V7256_8a, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[V7256_8a]),
	video7_256_init, video7_256_restore, v256_video7_selectpage,

	"FW256", "STDVGA", FW256_400, 640, 400, NULL, 4*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[V7256_400]),
	video7_256_init, video7_256_restore, v256_video7_selectpage,

	"FW256", "STDVGA", FW256_6, 640, 480, NULL, 5*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[V7256_6]),
	video7_256_init, video7_256_restore, v256_video7_selectpage,

	"FW256", "MULTISYNC", FW256_7, 720, 540, NULL, 6*64*1024, 64*1024,
	0x3D4, 720, &(inittab256[V7256_7]),
	video7_256_init, video7_256_restore, v256_video7_selectpage,

	"FW256", "MULTISYNC", FW256_8, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[V7256_8]),
	video7_256_init, video7_256_restore, v256_video7_selectpage,

	"FW256a", "MULTISYNC", FW256_8a, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[V7256_8a]),
	video7_256_init, video7_256_restore, v256_video7_selectpage,

	"1024i256", "STDVGA", V71024i256_400, 640, 400, NULL, 4*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[V71024i256_400]),
	video7_256_init, video7_256_restore, v256_video7_selectpage,

	"1024i256", "STDVGA", V71024i256_6, 640, 480, NULL, 5*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[V71024i256_6]),
	video7_256_init, video7_256_restore, v256_video7_selectpage,

	"ATI256", "STDVGA", ATI256_6, 640, 480, NULL, 5*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[ATI256_6]),
	ati_256_init, ati_256_rest, v256_ati_selectpage,

	"ATI256", "MULTISYNC", ATI256_8, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[ATI256_8]),
	ati_256_init, ati_256_rest, v256_ati_selectpage,

	"ATI2564", "STDVGA", ATI2564_6, 640, 480, NULL, 5*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[ATI256_6]),
	ati_256_init, ati_256_rest, v256_ati_selectpage,

	"ATI2564", "MULTISYNC", ATI2564_8, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[ATI2564_8]),
	ati_256_init, ati_256_rest, v256_ati_selectpage,

	"ATI2565", "STDVGA", ATI2565_6, 640, 480, NULL, 5*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[ATI256_6]),
	ati_256_init, ati_256_rest, v256_ati_selectpage,

	"ATI2565", "MULTISYNC", ATI2565_8, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[ATI2564_8]),
	ati_256_init, ati_256_rest, v256_ati_selectpage,

	"ATIPLUS256", "STDVGA", ATIPLUS256_400, 640, 400, NULL, 4*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[ATIPLUS256_400]),
	ati_256_init, ati_256_rest, v256_ati_selectpage,

	"ATIPLUS256", "STDVGA", ATIPLUS256_6, 640, 480, NULL, 5*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[ATIPLUS256_6]),
	ati_256_init, ati_256_rest, v256_ati_selectpage,

	"ATIPLUS256", "MULTISYNC", ATIPLUS256_8, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[ATIPLUS256_8]),
	ati_256_init, ati_256_rest, v256_ati_selectpage,

	"EDGE256", "STDVGA", EDGE256, 640, 400, NULL, 4*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[ATIPLUS256_400]),
	ati_256_init, ati_256_rest, v256_ati_selectpage,

	"TECMAR256", "STDVGA", TLI_6x4_256, 640, 480, NULL, 5*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[TLI_6x4_256]),
	tli_256_init, tli_256_rest, v256_tli_selectpage,

	"TECMAR256", "MULTISYNC", TEC256_8, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[TEC256_8]),
	tli_256_init, tli_256_rest, v256_tli_selectpage,

	"P256", "STDVGA", P256_400, 640, 400, NULL, 4*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[P256_400]),
	pvga1a_256_init, pvga1a_256_restore, pvga1a_256_selectpage,

	"P256", "STDVGA", P256_6, 640, 480, NULL, 5*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[P256_6]),
	pvga1a_256_init, pvga1a_256_restore, pvga1a_256_selectpage,

	"P256_1024", "STDVGA", P256_1024_400, 640, 400, NULL, 4*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[P256_400]),
	pvga1a_256_init, pvga1a_256_restore, pvga1a_256_selectpage,

	"P256_1024", "STDVGA", P256_1024_6, 640, 480, NULL, 5*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[P256_6]),
	pvga1a_256_init, pvga1a_256_restore, pvga1a_256_selectpage,

	"ET4000256", "STDVGA", ET4000256_6, 640, 480, NULL, 5*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[ET4000256_6]),
	et4000_256_init, et4000_256_rest, v256_tli_selectpage,

	"ET4000256", "MULTISYNC", ET4000256_8, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[ET4000256_8]),
	et4000_256_init, et4000_256_rest, v256_tli_selectpage,

	"ET4000256a", "MULTISYNC", ET4000256_8a, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[ET4000256_8a]),
	et4000_256_init, et4000_256_rest, v256_tli_selectpage,

	"ET4000256", "INTERLACED", ET4000256_1, 1024, 768, NULL, 12*64*1024, 64*1024,
	0x3D4, 1024, &(inittab256[ET4000256_1]),
	et4000_256_init, et4000_256_rest, v256_tli_selectpage,

	"ET4000256", "MULTISYNC", ET4000256_1ni, 1024, 768, NULL, 12*64*1024, 64*1024,
	0x3D4, 1024, &(inittab256[ET4000256_1ni]),
	et4000_256_init, et4000_256_rest, v256_tli_selectpage,

	"ORII256", "STDVGA", ET4000256_6, 640, 480, NULL, 5*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[ET4000256_6]),
	et4000_256_init, et4000_256_rest, v256_tli_selectpage,

	"ORII256", "MULTISYNC", ET4000256_8, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[ET4000256_8]),
	et4000_256_init, et4000_256_rest, v256_tli_selectpage,

	"ORII256a", "MULTISYNC", ET4000256_8a, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[ET4000256_8a]),
	et4000_256_init, et4000_256_rest, v256_tli_selectpage,

	"ORII256", "INTERLACED", ORII256_1, 1024, 768, NULL, 12*64*1024, 64*1024,
	0x3D4, 1024, &(inittab256[ORII256_1]),
	et4000_256_init, et4000_256_rest, v256_tli_selectpage,

	"ORII256", "MULTISYNC", ET4000256_1ni, 1024, 768, NULL, 12*64*1024, 64*1024,
	0x3D4, 1024, &(inittab256[ET4000256_1ni]),
	et4000_256_init, et4000_256_rest, v256_tli_selectpage,

	"STB256+", "STDVGA", ET4000256_6, 640, 480, NULL, 5*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[ET4000256_6]),
	et4000_256_init, et4000_256_rest, v256_tli_selectpage,

	"STB256+", "MULTISYNC", ET4000256_8, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[ET4000256_8]),
	et4000_256_init, et4000_256_rest, v256_tli_selectpage,

	"STB256+", "INTERLACED", STB256PLUS_1, 1024, 768, NULL, 12*64*1024, 64*1024,
	0x3D4, 1024, &(inittab256[STB256PLUS_1]),
	et4000_256_init, et4000_256_rest, v256_tli_selectpage,

	"STB256+", "MULTISYNC", ET4000256_1ni, 1024, 768, NULL, 12*64*1024, 64*1024,
	0x3D4, 1024, &(inittab256[ET4000256_1ni]),
	et4000_256_init, et4000_256_rest, v256_tli_selectpage,

	"T89256", "STDVGA", T89256_6, 640, 480, NULL, 5*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[T89256_6]),
	trident_256_init, trident_256_restore, trident_256_selectpage,

	"T89256", "MULTISYNC", T89256_8, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[T89256_8]),
	trident_256_init, trident_256_restore, trident_256_selectpage,

	"T89256a", "MULTISYNC", T89256_8a, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[T89256_8a]),
	trident_256_init, trident_256_restore, trident_256_selectpage,

	"T89256", "INTERLACED", T89256_1, 1024, 768, NULL, 12*64*1024, 64*1024,
	0x3D4, 1024, &(inittab256[T89256_1]),
	trident_256_init, trident_256_restore, trident_256_selectpage,

	"T89256", "MULTISYNC", T89256_1ni, 1024, 768, NULL, 12*64*1024, 64*1024,
	0x3D4, 1024, &(inittab256[T89256_1ni]),
	trident_256_init, trident_256_restore, trident_256_selectpage,

	"T88256", "STDVGA", T88256_400, 640, 400, NULL, 4*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[T88256_400]),
	trident_256_init, trident_256_restore, trident_256_selectpage,

	"T88256", "STDVGA", T88256_6, 640, 480, NULL, 5*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[T88256_6]),
	trident_256_init, trident_256_restore, trident_256_selectpage,

	"LEGEND256", "STDVGA", LEGEND256_6, 640, 480, NULL, 5*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[LEGEND256_6]),
	legend_init, legend_rest, v256_tli_selectpage,

	"LEGEND256", "MULTISYNC", LEGEND256_8, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[LEGEND256_8]),
	legend_init, legend_rest, v256_tli_selectpage,

	"LEGEND256a", "MULTISYNC", LEGEND256_8a, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[LEGEND256_8a]),
	legend_init, legend_rest, v256_tli_selectpage,

	"LEGEND256", "INTERLACED", LEGEND256_1, 1024, 768, NULL, 12*64*1024, 64*1024,
	0x3D4, 1024, &(inittab256[LEGEND256_1]),
	legend_init, legend_rest, v256_tli_selectpage,

	"LEGEND256", "MULTISYNC", LEGEND256_1ni, 1024, 768, NULL, 12*64*1024, 64*1024,
	0x3D4, 1024, &(inittab256[LEGEND256_1ni]),
	legend_init, legend_rest, v256_tli_selectpage,

	"MICRO4256", "STDVGA", ET4000256_6, 640, 480, NULL, 5*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[ET4000256_6]),
	et4000_256_init, et4000_256_rest, v256_tli_selectpage,

	"MICRO4256", "MULTISYNC", MICRO4256_8, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[MICRO4256_8]),
	et4000_256_init, et4000_256_rest, v256_tli_selectpage,

	"MICRO4256a", "MULTISYNC", MICRO4256_8a, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[MICRO4256_8a]),
	et4000_256_init, et4000_256_rest, v256_tli_selectpage,

	"MICRO4256", "INERLACED", ET4000256_1, 1024, 768, NULL, 12*64*1024, 64*1024,
	0x3D4, 1024, &(inittab256[ET4000256_1]),
	et4000_256_init, et4000_256_rest, v256_tli_selectpage,

	"MICRO4256", "MULTISYNC", ET4000256_1ni, 1024, 768, NULL, 12*64*1024, 64*1024,
	0x3D4, 1024, &(inittab256[ET4000256_1ni]),
	et4000_256_init, et4000_256_rest, v256_tli_selectpage,
};
int	v256_num_disp = (sizeof(disp_info) / sizeof(struct at_disp_info));

int 	v256_is_color;			/* true if on a color display */
struct	at_disp_info	vt_info;

unchar	*v256_maptbl;
unchar	v256_et3000_tbl[8] = {0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78};
unchar	v256_et4000_tbl[16]= {0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
			     0x80, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0};

static	struct	kd_memloc vt_map;
static	int	vt_fd;			/* file descriptor for the vt used */
static	unchar	*screen_buffer;		/* pointer to saved video buffer */
static	unchar	saved_misc_out;		/* need to save and restore this */
					/* because the kernel doesn't do */
					/* it right			 */

static	unchar	saved_rascas;		/* need to do this one also! */


/*
 * Table giving the information needed to initialize the V256 registers
 * This consists of the number of elements in the structure, the location of
 * the address register, and the location of the data register.
 *
 * This table is indexed by constants in <sys/kd.h>
 */
static struct reginfo	regtab[] = {
	16, 0x3b4, 0x3b5,	/* m6845init, monochrome */
	16, 0x3d4, 0x3d5,	/* m6845init, color/graphics */
	25, 0x3b4, 0x3b5,	/* v256init, monochrome */
	25, 0x3d4, 0x3d5,	/* v256init, color */
	NSEQ, 0x3c4, 0x3c5,	/* seqinit */
	NGRAPH, 0x3ce, 0x3cf,	/* graphinit */
	NATTR, 0x3c0, 0x3c0,	/* attrinit */
	NATTR, 0x3c0, 0x3c1,	/* attrinit */
};


/*
 * Initialize the virtual terminal based on the mode passed in.
 */
vt_init(fd, mode)
int	fd;
int	mode;
{
	extern char	*malloc();

	vt_fd = fd;

	if (vt_display_init(mode) == -1)	/* initialize display */
		return(-1);

	v256_slbytes = vt_info.slbytes;
	v256_endpage = 0xffff;			/* default to the page ending */
						/* at 64K.  May be changed    */
						/* by a board's selectpage    */
	

	vt_set_regs();
	if ( !(*vt_info.ext_init)(vt_info.vt_type) ) {
		/*
		 * failed to initialize; may be failed to init extended modes
		 */
		exit ();
	}

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);	/* reset sequencer */
	outb(MISC_OUT, vt_info.regs->miscreg);	/* Set misc register */
	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);	/* start sequencer */

	inb(vt_info.ad_addr + IN_STAT_1); /* turn off display */
	outb(regtab[I_ATTR].ri_address, 0);

	v256_clear();

	inb(vt_info.ad_addr + IN_STAT_1);/* turn on display */
	outb(regtab[I_ATTR].ri_address, PALETTE_ENABLE);

	/* allocate screen save buffer for VT flip now */
	if ((screen_buffer = (unchar *)malloc(vt_info.buf_size)) == NULL) {
		ErrorF("Can't Allocate Memory for Screen Save\n");
		return;
	}
}



/*
 *	vt_close()	-- unmap the current vt
 */
vt_close()
{
	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);	/* reset sequencer */
	outb(MISC_OUT, saved_misc_out);		/* Set misc register */
	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);	/* start sequencer */

	(*vt_info.ext_rest)(vt_info.vt_type);
	if (ioctl(vt_fd, KDUNMAPDISP, 0) == -1)
		ErrorF("Unable to unmap display\n");

	if (ioctl(vt_fd, KDSETMODE, KD_TEXT0) == -1)
		ErrorF("Unable to set text mode.\n");
}



/*
 * Save the current display memory
 */
vt_display_save()
{
	vt_save_mem();

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);	/* reset sequencer */
	outb(MISC_OUT, saved_misc_out);		/* Set misc register */
	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);	/* start sequencer */

	(*vt_info.ext_rest)(vt_info.vt_type);
	if (ioctl(vt_fd, KDUNMAPDISP, 0) == -1)
		return(-1);
	return(0);
}


/*
 * Restore the display registers and saved memory
 */
vt_display_restore()
{
	if ((ioctl(vt_fd, KDMAPDISP, &vt_map) == -1) ||
	    (ioctl(vt_fd, KDENABIO, 0) == -1))
		return(-1);

	saved_misc_out = inb(MISC_OUT_READ);
	vt_set_regs();
	(*vt_info.ext_init)(vt_info.vt_type);

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);	/* reset sequencer */
	outb(MISC_OUT, vt_info.regs->miscreg);	/* Set misc register */
	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);	/* start sequencer */

	vt_restore_mem();
	return(0);
}



/*
 * Save the current display memory
 */
static
vt_save_mem()
{
	int	src, limit;
	unchar	*to;

	/* screen buffer was allocated in vt_init */
	if ((to = screen_buffer) == NULL) {
		ErrorF("No memory allocated for screen save\n");
		return;
	}

	inb(vt_info.ad_addr + IN_STAT_1); 	/* turn off display */
	outb(regtab[I_ATTR].ri_address, 0);

	src = 0;
	limit = v256_slbytes * vt_info.ypix;
	while (src < limit) {
		selectpage(src);
		memcpy(to, vt_info.vt_buf, vt_info.map_size);
		to += vt_info.map_size;
		src += vt_info.map_size;
	}

	inb(vt_info.ad_addr + IN_STAT_1);
}



/*
 * Restore the saved display memory
 */
static
vt_restore_mem()
{
	int	dst, limit;
	unchar	*from;

	inb(vt_info.ad_addr + IN_STAT_1);	/* turn off display */
	outb(regtab[I_ATTR].ri_address, 0);

	from = screen_buffer;

	dst = 0;
	limit = v256_slbytes * vt_info.ypix;
	while (dst < limit) {
		selectpage(dst);
		memcpy(vt_info.vt_buf, from, vt_info.map_size);
		from += vt_info.map_size;
		dst += vt_info.map_size;
	}

	inb(vt_info.ad_addr + IN_STAT_1);	/* turn on display */
	outb(regtab[I_ATTR].ri_address, PALETTE_ENABLE);
}



/*
 * vt_set_regs()	-- Set the registers according to the mode given.
 */
static
vt_set_regs()
{
	int i;

	(void)inb(vt_info.ad_addr + IN_STAT_1); 	/* init flip-flop */
	outb(regtab[I_ATTR].ri_address, 0);		/* turn off display */
	doregs(vt_info.regs->seqtab, I_SEQ);		/* Reset, init and */
	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */

	/*
	 * Initialize CRT controller.
	 * We clear register 0x11 which enables writing to
	 * registers 0 - 7.
	 */
	if (v256_is_color) {
		out_reg(&regtab[I_EGACOLOR], 0x11, 0x2E);
		doregs(&(vt_info.regs->egatab), I_EGACOLOR);
		doregs(graphics, I_GRAPH);
	}
	else {
		out_reg(&regtab[I_EGAMONO], 0x11, 0x2E);
		doregs(&(vt_info.regs->egatab), I_EGAMONO);
		doregs(graphics, I_GRAPH);
	}
	(void)inb(vt_info.ad_addr + IN_STAT_1); 	/* init flip-flop */
	doregs(attributes, I_ATTR);			/* init attr regs */
}



/*
 * doregs(tabptr, type)	-- set V256 registers based on the type provided 
 *			and the fact that all registers of a given type are 
 *			accessed through a common I/O address with a separate 
 *			address register being used to distinguish them.
 * Input:
 *	unchar 	*tabptr		-- address of table to write/read data to/from
 *	int	type		-- type of registers to write/read
 */
static
doregs(tabptr, type)				
register unchar	*tabptr;	
int		type;
{
	register int	index, count;
	struct reginfo	*regptr;

	regptr = &regtab[type];
	count = regptr->ri_count;

	for (index = 0; index < count; index++, tabptr++) {
		out_reg(regptr, index, *tabptr);
	}
}



/*
 *	v256_color(i, r, g, b) -- set the color of pallette index i, to r, g, b.
 */
v256_color(i, r, g, b)
int i;
int r, g, b;
{
	/* wait for vertical retrace */

	outb(PEL_WRITE, i);
	outb(PEL_DATA, r);			/* Red */
	outb(PEL_DATA, g);			/* Green */
	outb(PEL_DATA, b);			/* Blue */
}
	


/*
 * vt_display_init()	-- Based on display type being used, set things up 
 *			as necessary so we'll be able to address video memory 
 *			and I/O ports.  Set the global variable vt_info.
 */
vt_display_init(type)
int type;
{
	struct kd_disparam dp;
	int disp_type;
	extern char *malloc();

	vt_info = disp_info[type];

	if (ioctl(vt_fd, KDDISPTYPE, &dp) == -1) {
		ErrorF("Unable to determine display type.\n");
		return(-1);
	}

	/*
	 * The display type has ('k' << 8) for the console. Other
	 * displays like the Sun River return other values (like
	 * ('s' << 8).
	 */
	if ((disp_type = ioctl(vt_fd, KIOCINFO, 0)) == -1) {
		ErrorF("Could not get display type.\n");
		return(-1);
	}
	disp_type >>= 8;

	/*
	 * set the display to graphics vt process mode 
	 */
	if (ioctl(vt_fd, KDSETMODE, KD_GRAPHICS) == -1) {
		ErrorF("Unable to set graphics mode.\n");
		return(-1);
	}
	
	/* 
	 * allocate an area of our virtual memory to map the physical
	 * screen address space to.  We have to have the address be on
	 * a page (4096) boundry, so we allocate extra and round up.
	 */
	if ((vt_info.vt_buf = (unchar *)malloc(vt_info.map_size+PG_SIZE)) == NULL) {
		ErrorF("Can't Allocate Memory for Screen Save\n");
		return(-1);
	}
	vt_info.vt_buf = (unchar *)(((long)vt_info.vt_buf & ~(PG_SIZE-1)) + 
			  PG_SIZE);

	/* 
	 * ask the display driver to map the screen memory and I/O ports 
	 */
	vt_map.vaddr = (char *)vt_info.vt_buf;
	vt_map.physaddr = (disp_type == 'k') ? (char *)base_addr[dp.type]
					     : (char *)dp.addr;
	vt_map.length = (long)vt_info.map_size;
	vt_map.ioflg = (long)1;

	if (ioctl(vt_fd, KDMAPDISP, &vt_map) == -1) {
		ErrorF("Unable to map display.\n");
		return(-1);
	}

	saved_misc_out = inb(MISC_OUT_READ);
	if ((saved_misc_out & IO_ADDR_SEL) == 0) {
		v256_is_color = 0;
		vt_info.ad_addr = 0x3B4;
		vt_info.regs->miscreg &= ~IO_ADDR_SEL;
	}
	else
		v256_is_color = 1;

	if ((vt_info.vt_type == ATI256_6) || (vt_info.vt_type == ATI256_8) ||
	    (vt_info.vt_type == ATI2564_6) || (vt_info.vt_type == ATI2564_8) ||
	    (vt_info.vt_type == ATI2565_6) || (vt_info.vt_type == ATI2565_8) ||
	    (vt_info.vt_type==ATIPLUS256_400)||(vt_info.vt_type==ATIPLUS256_6)||
	    (vt_info.vt_type==ATIPLUS256_8) || (vt_info.vt_type == EDGE256))
		graphics[5] = 0;

	if ((vt_info.vt_type == ATI2564_6) || (vt_info.vt_type == ATI2564_8) ||
	    (vt_info.vt_type == ATI2565_6) || (vt_info.vt_type == ATI2565_8) ||
	    (vt_info.vt_type==ATIPLUS256_400)||(vt_info.vt_type==ATIPLUS256_6)||
	    (vt_info.vt_type==ATIPLUS256_8) || (vt_info.vt_type == EDGE256))
		attributes[0x10] = 1;

	if (vt_info.vt_type == EDGE256)
		vt_info.regs->egatab.ei_offset = 0x50;

	return(0);
}



/*
 * v256_clear()		-- clear the screen.  This is not dependant on
 *			any particular board.
 */
v256_clear()
{
	int dst, limit;

	dst = 0;
	limit = v256_slbytes * vt_info.ypix;
	while (dst < limit) {
		selectpage(dst);
		memset(vt_info.vt_buf, 0, vt_info.map_size); 
		dst += vt_info.map_size;
	}
}



/*
 *	set_reg(address, index, data)	-- set an index/data I/O register
 *					pair being careful to wait between
 *					the out() instructions to allow
 *					the cycle to finish.
 *
 *	Input:
 *		int	address		-- address of register pair
 *		int	index		-- index of register to set
 *		int	data		-- data to set
 */
static
set_reg(address, index, data)
int address;
int index;
int data;
{
	outb(address, index);
	asm("	jmp	.+2");		/* flush cache */
	outb(address+1, data);
	asm("	jmp	.+2");		/* flush cache */
}

	
/*
 *	get_reg(address, index, data)	-- get a register value from an 
 *					index/data I/O register pair being 
 *					careful to wait between the I/O
 *					instructions to allow the cycle 
 *					to finish.
 *
 *	Input:
 *		int	address		-- address of register pair
 *		int	index		-- index of register to set
 *		int	*data		-- place to put data
 */
static
get_reg(address, index, data)
int address;
int index;
int *data;
{
	outb(address, index);
	asm("	jmp	.+2");		/* flush cache */
	*data = inb(address+1);
	asm("	jmp	.+2");		/* flush cache */
}
	


static unchar tli256_seq_aux; 
static unchar tli256_attr_misc; 
static unchar tli256_crt_misc;
static unchar tli256_gdc_select;
static unchar saved_crtc24_256;
/*
 *	tli_256_init(mode)	-- initialize a Tseng Labs V256 board to one
 *				of it's "extended" modes.  This takes care
 *				of non-standard V256 registers.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */

tli_256_init(mode)
int mode;
{
	static int inited = 0;
	int saved_miscattr;
	unchar	temp;

	set_reg(0x3c4, 0, SEQ_RESET);		/* reset sequencer */

	if (!inited) {
		/*
		 * What Tseng chip am I running on ???
		 */
		outb(0x3BF, 3);
		outb(0x3D8, 0xA0);
		(void)inb(0x3DA);
		outb(0x3C0, 0x36);
		saved_miscattr = inb(0x3C1);
		outb(0x3C0, 0x90);

		outb(0x3C0, 0x36);
		if (inb(0x3C1) == 0x90) {
			vt_is_ET4000 = 1;
			v256_maptbl = v256_et4000_tbl;
		} else {
			vt_is_ET4000 = 0;
			v256_maptbl = v256_et3000_tbl;
			if (mode == TLI_1x7_256)
				ErrorF("Unable to support 1024x768 resolution");
		}
		outb(0x3C0, saved_miscattr);

		tli256_gdc_select = inb(0x3CD);
		get_reg(0x3c4, 0x07, &tli256_seq_aux);
		(void)inb(vt_info.ad_addr + IN_STAT_1); /* init flip-flop */
		in_reg(&regtab[I_ATTR+1], 0x16, tli256_attr_misc);
		if (v256_is_color) {
			get_reg(0x3d4, 0x25, &tli256_crt_misc);
		}
		else {
			get_reg(0x3b4, 0x25, &tli256_crt_misc);
		}
		inited = 1;
	}

	switch(mode) {
	case ORF256_8:
		if (v256_is_color)
			set_reg(0x3d4, 0x25, 0x80);
		else
			set_reg(0x3b4, 0x25, 0x80);
		/* FALL THROUGH */
		
	case STB_8x6_256:
	case TLI_1x7_256:
	case TLI_400_256:
	case ORV256_8:
	case ORD256_8:
	case GENOA256_8:
	case MICRO3256_8:
		outb(0x3BF, 3);
		if (v256_is_color) {
			outb(0x3D8, 0xA0);
			if (vt_is_ET4000) {
				out_reg(&regtab[I_EGACOLOR], 0x13, 
					(vt_info.xpix >> 3) & 0xFF);
				out_reg(&regtab[I_EGACOLOR], 0x14, 0x60);
				out_reg(&regtab[I_EGACOLOR], 0x17, 0xAB);
				out_reg(&regtab[I_GRAPH], 0x05, 0x40);
				out_reg(&regtab[I_SEQ], 0x04, 0x0E);
			}
		}
		else {
			outb(0x3B8, 0xA0);
			if (vt_is_ET4000) {
				out_reg(&regtab[I_EGAMONO], 0x13, 
					(vt_info.xpix >> 3) & 0xFF);
				out_reg(&regtab[I_EGAMONO], 0x14, 0x60);
				out_reg(&regtab[I_EGAMONO], 0x17, 0xAB);
				out_reg(&regtab[I_GRAPH], 0x05, 0x40);
				out_reg(&regtab[I_SEQ], 0x04, 0x0E);
			}
		}

		/* 
		 * set external clock. We do some weird stuff here to 
		 * really get 16 bit writes turned on.  (set TS7 to an
		 * initial value of 8, set bit 3 of MISC_OUT,  and then 
		 * toggle CRTC24.)
		 */
		set_reg(0x3c4, 0x07, 0x48);	/* V256 reg/32K ROM */
		outb(MISC_OUT, saved_misc_out|8);/* Set misc register */

		if (vt_is_ET4000) {
			temp = ((vt_info.regs->miscreg&0x0C)==0x0C)? 0x02: 0x00;
			if (v256_is_color) {
				in_reg(&regtab[I_EGACOLOR], 0x34, saved_crtc24_256);
				in_reg(&regtab[I_EGACOLOR], 0x37, saved_rascas);
				temp |= saved_crtc24_256 & 0xFD;
				out_reg(&regtab[I_EGACOLOR], 0x34, temp);
				out_reg(&regtab[I_EGACOLOR], 0x35, 0);
				out_reg(&regtab[I_EGACOLOR], 0x37, saved_rascas&0xcf);
			} else {
				in_reg(&regtab[I_EGAMONO], 0x34, saved_crtc24_256);
				in_reg(&regtab[I_EGACOLOR], 0x37, saved_rascas);
				temp |= saved_crtc24_256 & 0xFD;
				out_reg(&regtab[I_EGAMONO], 0x34, temp);
				out_reg(&regtab[I_EGAMONO], 0x35, 0);
				out_reg(&regtab[I_EGACOLOR], 0x37, saved_rascas&0xcf);
			}
		} else {
			set_reg(0x3c4, 0, SEQ_RUN);	/* start seq */
			set_reg(0x3c4, 0, SEQ_RESET);	/* stop seq */

			if (v256_is_color) {
				get_reg(0x3d4, 0x24, &saved_crtc24_256);
				set_reg(0x3d4, 0x24, saved_crtc24_256&0xfd);
				set_reg(0x3d4, 0x24, saved_crtc24_256|2);
			} else {
				get_reg(0x3d4, 0x24, &saved_crtc24_256);
				set_reg(0x3b4, 0x24, saved_crtc24_256&~0xfd);
				set_reg(0x3b4, 0x24, saved_crtc24_256|2);
			}
		}
		outb(MISC_OUT, saved_misc_out);	/* Reset misc register */
		/* FALL THROUGH */

	case TLI_6x4_256:
	case STB_6x4_256:
	case TEC256_8:
		set_reg(0x3c4, 0x07, 0xa8);	/* V256 reg/32K ROM */

		(void)inb(vt_info.ad_addr + IN_STAT_1); /* init flip-flop */
		if (vt_is_ET4000 == 0) {
			out_reg(&regtab[I_ATTR], 0x16, 0x10);
		}
		else {
			out_reg(&regtab[I_ATTR], 0x10, 0x01);
		}
		break;
	}

	if (vt_is_ET4000 == 0) {
		if (v256_is_color)
			outb(0x3D8, 0x29);
		else
			outb(0x3B8, 0x29);
	}

	set_reg(0x3c4, 0, SEQ_RUN);		/* start seq */
}
	


/*
 *	tli_256_rest(mode)  	-- restore a Tseng Labs V256 board from one
 *			  	of it's "extended" modes.  This takes care
 *			  	of non-standard V256 registers.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
tli_256_rest(mode)
int mode;
{
	int	temp;

	set_reg(0x3c4, 0, SEQ_RESET);			/* reset sequencer */
	set_reg(0x3c4, 0x07, tli256_seq_aux);

	outb(0x3CD, tli256_gdc_select);			/* reset segment */
	(void)inb(vt_info.ad_addr + IN_STAT_1); /* init flip-flop */
	out_reg(&regtab[I_ATTR], 0x16, tli256_attr_misc);
	outb(MISC_OUT, saved_misc_out & ~IO_ADDR_SEL);
	outb(0x3BF, 0x03);
	outb(MISC_OUT, saved_misc_out);
	if (v256_is_color)
		set_reg(0x3d4, 0x25, tli256_crt_misc);
	else
		set_reg(0x3b4, 0x25, tli256_crt_misc);

	if ((mode != TLI_6x4_256) && (mode != STB_6x4_256)) {
		if (vt_is_ET4000) {
			temp = 0x34;
			if (v256_is_color) {
				outb(0x3D8, 0xA0);
				out_reg(&regtab[I_EGACOLOR],0x37, saved_rascas);
			}
			else {
				outb(0x3B8, 0xA0);
				out_reg(&regtab[I_EGAMONO], 0x37, saved_rascas);
			}
		}
		else {
			temp = 0x24;
		}
		if (v256_is_color) {
			outb(0x3D8, 0xA0);
			set_reg(0x3d4, temp, saved_crtc24_256);
		}
		else {
			outb(0x3B8, 0xA0);
			set_reg(0x3b4, temp, saved_crtc24_256);
		}
	}
	set_reg(0x3c4, 0, SEQ_RUN);		/* start sequencer */
}



/*
 * v256_tli_selectpage(j)	-- select the current page based on the
 *				byte offset passed in. 
 *
 * Input:
 *	unsigned long	j	-- byte offset into video memory
 */
v256_tli_selectpage(j)
unsigned long j;
{
	v256_page = j >> 16;
	v256_page = v256_maptbl[v256_page] | v256_page;
	outb(0x3cd, v256_page);
	v256_endpage = j | 0xffff;
}



static unchar et4000256_seq_aux; 
static unchar et4000256_gdc_select;
static unchar et4000256_attr_misc; 
static unchar saved_crtc34_256;
static unchar saved_crtc35_256;
/*
 *	et4000_256_init(mode)	-- initialize a Tseng Labs ET4000 board to 
 *				one of it's 256 color "extended" modes.  
 *				This takes care of non-standard registers.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
et4000_256_init(mode)
int mode;
{
	static int inited = 0;
	int saved_miscattr;
	unchar	temp;

	set_reg(0x3c4, 0, SEQ_RESET);		/* reset sequencer */

	outb(0x3bf, 3);				/* turn on "key" */
	if (v256_is_color)
		outb(0x3d8, 0xa0);
	else
		outb(0x3b8, 0xa0);

	if (!inited) {
		v256_maptbl = v256_et4000_tbl;
		et4000256_gdc_select = inb(0x3CD);
		get_reg(0x3c4, 0x07, &et4000256_seq_aux);
		(void)inb(vt_info.ad_addr + IN_STAT_1); /* init flip-flop */
		in_reg(&regtab[I_ATTR+1], 0x16, et4000256_attr_misc);
		if (v256_is_color) {
			get_reg(0x3d4, 0x34, &saved_crtc34_256);
			get_reg(0x3d4, 0x35, &saved_crtc35_256);
		}
		else {
			get_reg(0x3b4, 0x34, &saved_crtc34_256);
			get_reg(0x3b4, 0x35, &saved_crtc35_256);
		}
		inited = 1;
	}

	(void)inb(vt_info.ad_addr + IN_STAT_1); /* init flip-flop */
	out_reg(&regtab[I_ATTR], 0x10, 0x01);
	switch(mode) {
	case MICRO4256_8a:
		if (v256_is_color) {
			set_reg(0x3d4, 0x34, 0x0a);
		}
		else {
			set_reg(0x3b4, 0x34, 0x0a);
		}
		break;
	case ET4000256_6:
	case ET4000256_8:
	case ET4000256_8a:
		if (v256_is_color) {
			set_reg(0x3d4, 0x34, 0x08);
			set_reg(0x3d4, 0x35, 0x00);
		}
		else {
			set_reg(0x3b4, 0x34, 0x08);
			set_reg(0x3b4, 0x35, 0x00);
		}
		break;

	case STB256PLUS_1:
		if (v256_is_color) {
			set_reg(0x3d4, 0x34, 0x00);
			set_reg(0x3d4, 0x35, 0x80);
		}
		else {
			set_reg(0x3b4, 0x34, 0x00);
			set_reg(0x3b4, 0x35, 0x80);
		}
		break;

	case ORII256_1:
	case ET4000256_1:
		if (v256_is_color) {
			set_reg(0x3d4, 0x34, 0x0a);
			set_reg(0x3d4, 0x35, 0x80);
		}
		else {
			set_reg(0x3b4, 0x34, 0x0a);
			set_reg(0x3b4, 0x35, 0x80);
		}
		break;

	case ET4000256_1ni:
		(void)inb(vt_info.ad_addr + IN_STAT_1); /* init flip-flop */
		out_reg(&regtab[I_ATTR], 0x16, 0x90);
		if (v256_is_color) {
			set_reg(0x3d4, 0x34, 0x0a);
			set_reg(0x3d4, 0x35, 0x00);
		}
		else {
			set_reg(0x3b4, 0x34, 0x0a);
			set_reg(0x3b4, 0x35, 0x00);
		}
		break;
	}

	switch(mode) {
	case ET4000256_1ni:
		set_reg(0x3c4, 0x07, 0xec);
		break;
	case MICRO4256_8:
	case MICRO4256_8a:
		break;
	default:
		set_reg(0x3c4, 0x07, 0x88);
	}
	set_reg(0x3c4, 0, SEQ_RUN);		/* start seq */
}
	


/*
 *	et4000_256_rest(mode)	-- restore a Tseng Labs ET4000 board from 
 *				one of it's 256 color "extended" modes.  
 *				This takes care of non-standard registers.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
et4000_256_rest(mode)
int mode;
{
	int	temp;

	set_reg(0x3c4, 0, SEQ_RESET);			/* reset sequencer */
	set_reg(0x3c4, 0x07, et4000256_seq_aux);
	(void)inb(vt_info.ad_addr + IN_STAT_1); /* init flip-flop */
	out_reg(&regtab[I_ATTR], 0x16, et4000256_attr_misc);

	if (v256_is_color) {
		set_reg(0x3d4, 0x34, saved_crtc34_256);
		set_reg(0x3d4, 0x35, saved_crtc35_256);
	}
	else {
		set_reg(0x3b4, 0x34, saved_crtc34_256);
		set_reg(0x3b4, 0x35, saved_crtc35_256);
	}
	outb(0x3CD, et4000256_gdc_select);		/* reset segment */
	set_reg(0x3c4, 0, SEQ_RUN);			/* start sequencer */
}


static unchar bandwidth;
static unchar clock;
static unchar clock_ext;
static unchar timing;
static unchar compat;
static unchar saved_bank_sel;
static unchar page_sel;
static unchar misc_ctrl;
static unchar interlace_val;
static unchar saved_exten;
static unchar exten = 0;
static unchar bank_sel = 0;
/*
 *	video7_256_init(mode)	-- initialize a Video Seven VGA board to
 *				one of it's "extended" modes.  This takes care
 *				of non-standard VGA registers.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
video7_256_init(mode)
int mode;
{
	static int inited = 0;

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */
	out_reg(&regtab[I_SEQ], 6, 0xea);		/* enable extensions */

	if (!inited) {
		inited = 1;

		switch(mode) {
		case V71024i256_400:
		case V71024i256_6:
		case FW256_400:
		case FW256_6:
		case FW256_7:
		case FW256_8:
		case FW256_8a:
		case V7256_400:
		case V7256_6:
		case V7256_7:
		case V7256_8:
		case V7256_8a:
			in_reg(&regtab[I_SEQ], 0xfd, timing);
			in_reg(&regtab[I_SEQ], 0xa4, clock);
			in_reg(&regtab[I_SEQ], 0xf8, clock_ext);
			in_reg(&regtab[I_SEQ], 0xfc, compat);
			in_reg(&regtab[I_SEQ], 0xff, saved_exten);
			in_reg(&regtab[I_SEQ], 0xf6, saved_bank_sel);
			in_reg(&regtab[I_SEQ], 0xf9, page_sel);
			break;
		}
	}

	switch(mode) {
	case FW256_6:
	case FW256_7:
	case FW256_8:
	case FW256_8a:
		bank_sel = 0xc0;
		exten = 0x10;
		/* FALL THROUGH */

	case FW256_400:
		out_reg(&regtab[I_SEQ], 0xfd, 0x30);	/* set timing */
		out_reg(&regtab[I_SEQ], 0xa4, 0x10);	/* set clock */
		out_reg(&regtab[I_SEQ], 0xfc, 0x6c);	/* set compat */
		out_reg(&regtab[I_SEQ], 0xf6, bank_sel);/* set bank sel */
		out_reg(&regtab[I_SEQ], 0xf8, 0x00);	/* set ext. clock */
		out_reg(&regtab[I_SEQ], 0xff, exten | 5);/* set extensions */
		break;

	case V7256_6:
	case V7256_7:
	case V7256_8:
	case V7256_8a:
		bank_sel = 0xc0;
		exten = 0x10;
		/* FALL THROUGH */

	case V7256_400:
		out_reg(&regtab[I_SEQ], 0xfd, 0xa0);	/* set timing */
		out_reg(&regtab[I_SEQ], 0xa4, 0x10);	/* set clock */
		out_reg(&regtab[I_SEQ], 0xfc, 0x6c);	/* set compat */
		out_reg(&regtab[I_SEQ], 0xf6, bank_sel);/* set bank sel */
		out_reg(&regtab[I_SEQ], 0xf8, 0x00);	/* set ext. clock */
		out_reg(&regtab[I_SEQ], 0xff, exten | 5);/* set extensions */
		break;

	case V71024i256_6:
		bank_sel = 0xc0;
		exten = 0x10;
		/* FALL THROUGH */

	case V71024i256_400:
		out_reg(&regtab[I_SEQ], 0xfd, 0x30);	/* set timing */
		out_reg(&regtab[I_SEQ], 0xa4, 0x04);	/* set clock */
		out_reg(&regtab[I_SEQ], 0xfc, 0x6c);	/* set compat */
		out_reg(&regtab[I_SEQ], 0xf6, bank_sel);/* set bank sel */
		out_reg(&regtab[I_SEQ], 0xf8, 0x02);	/* set ext. clock */
		out_reg(&regtab[I_SEQ], 0xff, exten | 5);/* set extensions */
		break;
	}

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}
	


/*
 *	video7_256_restore(mode) -- restore a Video Seven VGA board from
 *				one of it's "extended" modes.  This takes care
 *				of non-standard VGA registers.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
video7_256_restore(mode)
int mode;
{
	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */
	out_reg(&regtab[I_SEQ], 6, 0xea);		/* enable extensions */

	switch(mode) {
	case V71024i256_400:
	case V71024i256_6:
	case FW256_400:
	case FW256_6:
	case FW256_7:
	case FW256_8:
	case FW256_8a:
	case V7256_400:
	case V7256_6:
	case V7256_7:
	case V7256_8:
	case V7256_8a:
		out_reg(&regtab[I_SEQ], 0xfd, timing);
		out_reg(&regtab[I_SEQ], 0xa4, clock);
		out_reg(&regtab[I_SEQ], 0xf8, clock_ext);
		out_reg(&regtab[I_SEQ], 0xfc, compat);
		out_reg(&regtab[I_SEQ], 0xff, saved_exten);
		out_reg(&regtab[I_SEQ], 0xf6, saved_bank_sel);
		out_reg(&regtab[I_SEQ], 0xf9, page_sel);
	}

	out_reg(&regtab[I_SEQ], 6, 0xae);		/* disable extensions */
	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}



/*
 * v256_video7_selectpage(j)	-- select the current page based on the
 *				byte offset passed in. 
 *
 * Input:
 *	unsigned long	j	-- byte offset into video memory
 */
v256_video7_selectpage(j)
register unsigned long j;
{
	v256_endpage = j | 0xffff;
	j >>= 16;
	if (j == v256_page)
		return;

	v256_page = j;

	outb(MISC_OUT, vt_info.regs->miscreg | ((j & 2) << 4));
	out_reg(&regtab[I_SEQ], 0xf9, j & 1);
	j &= 0x0c;
	out_reg(&regtab[I_SEQ], 0xf6, (bank_sel | j | (j >> 2)));
}



static unchar pr0a;
static unchar pr4;
static unchar pr16;
static struct reginfo *pvga_ptr;
/*
 *	pvga1a_256_init(mode)	-- initialize a Paradise VGA board to
 *				one of it's "extended" modes.  This takes care
 *				of non-standard VGA registers.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
pvga1a_256_init(mode)
int mode;
{
	static int inited = 0;

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	if (!inited) {
		inited = 1;

		out_reg(&regtab[I_GRAPH], 0xf, 0x5);	/* unlock regs */
		switch(mode) {
		case P256_1024_400:
		case P256_1024_6:
			if (v256_is_color)
				pvga_ptr = &regtab[I_EGACOLOR];
			else
				pvga_ptr = &regtab[I_EGAMONO];
	
			out_reg(pvga_ptr, 0x29, 0x85);	/* unlock regs */

			in_reg(pvga_ptr, 0x2f, pr16);
			/* FALL THROUGH */

		case P256_400:
		case P256_6:
			in_reg(&regtab[I_GRAPH], 0x09, pr0a);
			in_reg(&regtab[I_GRAPH], 0x0e, pr4);
			break;
		}
	}

	out_reg(&regtab[I_GRAPH], 0xf, 0x5);	/* unlock regs */

	switch(mode) {
	case P256_1024_400:
	case P256_1024_6:
		out_reg(pvga_ptr, 0x29, 0x85);	/* unlock regs */
		out_reg(pvga_ptr, 0x2f, 0);
		/* FALL THROUGH */

	case P256_400:
	case P256_6:
		out_reg(&regtab[I_GRAPH], 0x09, 0);
		out_reg(&regtab[I_GRAPH], 0x0e, 1);
		break;
	}
		
	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}
	


/*
 *	pvga1a_256_restore(mode) -- restore a Paradise VGA board from
 *				one of it's "extended" modes.  This takes care
 *				of non-standard VGA registers.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
pvga1a_256_restore(mode)
int mode;
{
	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	out_reg(&regtab[I_GRAPH], 0xf, 0x5);	/* unlock regs */

	switch(mode) {
	case P256_1024_400:
	case P256_1024_6:
		out_reg(pvga_ptr, 0x29, 0x85);	/* unlock regs */
		out_reg(pvga_ptr, 0x2f, pr16);
		/* FALL THROUGH */

	case P256_400:
	case P256_6:
		out_reg(&regtab[I_GRAPH], 0x09, pr0a);
		out_reg(&regtab[I_GRAPH], 0x0e, pr4);
		break;
	}

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}



/*
 * pvga1a_256_selectpage(j)	-- select the current page based on the
 *				byte offset passed in. 
 *
 * Input:
 *	unsigned long	j	-- byte offset into video memory
 */
pvga1a_256_selectpage(j)
register unsigned long j;
{
	v256_endpage = j | 0xffff;
	j >>= 16;
	if (j == v256_page)
		return;

	v256_page = j;

	out_reg(&regtab[I_GRAPH], 0x09, j << 4);
}



static unchar	ati0;
static unchar	ati_page;
static unchar	ati2;
static unchar	ati3;
static unchar	ati6;
static unchar	ati8;
static unchar	ati9;
static unchar	atie;
/*
 *	ati_256_init(mode)	-- initialize an ATI VGA Wonder board into
 *			one of it's "extended" modes.  This takes care
 *			of non-standard VGA registers.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
ati_256_init(mode)
int mode;
{
	static int inited = 0;

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	/* enable ATI ports */
	if ((ioctl(vt_fd, KDADDIO, (unsigned short) 0x1ce) == -1) ||
	    (ioctl(vt_fd, KDADDIO, (unsigned short) 0x1cf) == -1) ||
	    (ioctl(vt_fd, KDENABIO) == -1)) {
		ErrorF("Can't enable ATI extensions\n");
		return;
	}

	if (!inited) {
		outb(0x1ce, 0xbe);
		atie = inb(0x1cf);

		outb(0x1ce, 0xb9);
		ati9 = inb(0x1cf);

		outb(0x1ce, 0xb8);
		ati8 = inb(0x1cf);

		outb(0x1ce, 0xb6);
		ati6 = inb(0x1cf);

		outb(0x1ce, 0xb3);
		ati3 = inb(0x1cf);

		outb(0x1ce, 0xb2);
		ati2 = inb(0x1cf);

		outb(0x1ce, 0xb0);
		ati0 = inb(0x1cf);
		inited = 1;
	}

	ati_page = ati2;

	switch(mode) {
	case ATI256_6:
		outb(0x1ce, 0xb0);
		outb(0x1cf, 0x30);
		break;

	case ATI256_8:
		ati_page |= 0x40;
		outb(0x1ce, 0xb2);
		outb(0x1cf, ati_page);

		outb(0x1ce, 0xb0);
		outb(0x1cf, 0x38);

		outb(0x1ce, 0xb8);
		outb(0x1cf, ati8 & 0x3f);
		break;

	case ATI2565_6:
		outb(0x1ce, 0xb9);
		outb(0x1cf, ati9 | 0x2);
		/* FALL THROUGH */

	case ATI2564_6:
		outb(0x1ce, 0xb0);
		outb(0x1cf, 0x30);

		outb(0x1ce, 0xbe);
		outb(0x1cf, (atie & 0xe7) | 0x10);

		outb(0x1ce, 0xb8);
		outb(0x1cf, (ati8 & 0x7f) | 0x40);

		outb(0x1ce, 0xb3);
		outb(0x1cf, ati3 & 0xef);
		break;

	case ATI2565_8:
		outb(0x1ce, 0xb9);
		outb(0x1cf, ati9 & 0xfd);
		/* FALL THROUGH */

	case ATI2564_8:
		outb(0x1ce, 0xb0);
		outb(0x1cf, ati0 | 0x38);

		outb(0x1ce, 0xbe);
		outb(0x1cf, (atie & 0xe7) | 0x10);

		outb(0x1ce, 0xb8);
		outb(0x1cf, ati8 & 0x3f);

		outb(0x1ce, 0xb3);
		outb(0x1cf, ati3 & 0xef);
		break;

	case EDGE256:
		outb(0x1ce, 0xb0);
		outb(0x1cf, ati0 | 0x26);

		outb(0x1ce, 0xb3);
		outb(0x1cf, ati3 & 0xef);
		break;

	case ATIPLUS256_400:
	case ATIPLUS256_6:
		outb(0x1ce, 0xb0);
		outb(0x1cf, ati0 | 0x20);

		outb(0x1ce, 0xbe);
		outb(0x1cf, (atie & 0xe7) | 0x10);

		outb(0x1ce, 0xb6);
		outb(0x1cf, ati6 | 0x4);

		outb(0x1ce, 0xb8);
		outb(0x1cf, (ati8 & 0x7f) | 0x40);

		outb(0x1ce, 0xb9);
		outb(0x1cf, ati9 | 0x2);
		break;

	case ATIPLUS256_8:
		outb(0x1ce, 0xb0);
		outb(0x1cf, ati0 | 0x20);

		outb(0x1ce, 0xbe);
		outb(0x1cf, (atie & 0xe7) | 0x10);

		outb(0x1ce, 0xb6);
		outb(0x1cf, ati6 | 0x4);

		outb(0x1ce, 0xb8);
		outb(0x1cf, ati8 & 0x3f);

		outb(0x1ce, 0xb3);
		outb(0x1cf, ati3 & 0xef);

		outb(0x1ce, 0xb9);
		outb(0x1cf, ati9 & 0xfd);
		break;
	}

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}
	
	

/*
 *	ati_256_rest(mode)	-- restore an ATI VGA Wonder board from
 *			one of it's "extended" modes.  This takes care
 *			of non-standard VGA registers.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
ati_256_rest(mode)
int mode;
{
	/* disable ATI ports */
	if ((ioctl(vt_fd, KDDELIO, (unsigned short) 0x1ce) == -1) ||
	    (ioctl(vt_fd, KDDELIO, (unsigned short) 0x1cf) == -1)) {
		ErrorF("Can't disable ATI extensions\n");
		return;
	}

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	outb(0x1ce, 0xbe);
	outb(0x1cf, atie);

	outb(0x1ce, 0xb9);
	outb(0x1cf, ati9);

	outb(0x1ce, 0xb8);
	outb(0x1cf, ati8);

	outb(0x1ce, 0xb6);
	outb(0x1cf, ati6);

	outb(0x1ce, 0xb3);
	outb(0x1cf, ati3);

	outb(0x1ce, 0xb2);
	outb(0x1cf, ati2);

	outb(0x1ce, 0xb0);
	outb(0x1cf, ati0);

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}



/*
 * v256_ati_selectpage(j)	-- select the current page based on the
 *				byte offset passed in. 
 *
 * Input:
 *	unsigned long	j	-- byte offset into video memory
 */
v256_ati_selectpage(j)
register unsigned long j;
{
	v256_endpage = j | 0xffff;
	j >>= 16;
	if (j == v256_page)
		return;

	v256_page = j;

	outb(0x1ce, 0xb2);
	outb(0x1cf, (ati_page & 0xe1) | (j << 1));
}


static unchar trident_256_mode2;
static unchar trident_256_mode1_new;
static unchar trident_256_mode2_new;
static unchar trident_256_test;

/*
 *	trident_256_init(mode)	-- initialize a Trident VGA to one of
 *				it's extended modes.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
trident_256_init(mode)
int mode;
{
	static int inited = 0;
	int junk;

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	if (!inited) {
		inited = 1;
		
		in_reg(&regtab[I_SEQ], 0xb, junk);	/* 64k mode */

		if (v256_is_color) {
			in_reg(&regtab[I_EGACOLOR], 0x1e, trident_256_test);
		}
		else {
			in_reg(&regtab[I_EGAMONO], 0x1e, trident_256_test);
		}

		out_reg(&regtab[I_SEQ], 0xb, junk);	/* 128k mode */
		in_reg(&regtab[I_SEQ], 0xd, trident_256_mode2);
		in_reg(&regtab[I_SEQ], 0xb, junk);	/* 64k mode */
		in_reg(&regtab[I_SEQ], 0xe, trident_256_mode1_new);
		in_reg(&regtab[I_SEQ], 0xd, trident_256_mode2_new);
	}

	switch(mode) {
	case T88256_400:
	case T88256_6:
		if (v256_is_color) {
			out_reg(&regtab[I_EGACOLOR], 0x1e, 0x80);
		}
		else {
			out_reg(&regtab[I_EGAMONO], 0x1e, 0x80);
		}
		break;

	case T89256_6:
	case T89256_8:
		if (v256_is_color) {
			out_reg(&regtab[I_EGACOLOR], 0x1e, 0x80);
		}
		else {
			out_reg(&regtab[I_EGAMONO], 0x1e, 0x80);
		}

		out_reg(&regtab[I_SEQ], 0xb, junk);	/* 128k mode */
		out_reg(&regtab[I_SEQ], 0xd, 0x30);
		in_reg(&regtab[I_SEQ], 0xb, junk);	/* 64k mode */
		break;

	case T89256_8a:
		if (v256_is_color) {
			out_reg(&regtab[I_EGACOLOR], 0x1e, 0x80);
		}
		else {
			out_reg(&regtab[I_EGAMONO], 0x1e, 0x80);
		}

		out_reg(&regtab[I_SEQ], 0xb, junk);	/* 128k mode */
		out_reg(&regtab[I_SEQ], 0xd, 0x10);
		in_reg(&regtab[I_SEQ], 0xb, junk);	/* 64k mode */
		out_reg(&regtab[I_SEQ], 0xd, 1);
		break;

	case T89256_1:
		if (v256_is_color) {
			out_reg(&regtab[I_EGACOLOR], 0x1e, 0x84);
		}
		else {
			out_reg(&regtab[I_EGAMONO], 0x1e, 0x84);
		}

		out_reg(&regtab[I_SEQ], 0xb, junk);	/* 128k mode */
		out_reg(&regtab[I_SEQ], 0xd, 0x10);
		in_reg(&regtab[I_SEQ], 0xb, junk);	/* 64k mode */
		break;

	case T89256_1ni:
		if (v256_is_color) {
			out_reg(&regtab[I_EGACOLOR], 0x1e, 0x80);
		}
		else {
			out_reg(&regtab[I_EGAMONO], 0x1e, 0x80);
		}

		out_reg(&regtab[I_SEQ], 0xb, junk);	/* 128k mode */
		out_reg(&regtab[I_SEQ], 0xd, 0x10);
		in_reg(&regtab[I_SEQ], 0xb, junk);	/* 64k mode */
		out_reg(&regtab[I_SEQ], 0xd, 1);
		break;
	}

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}
		


/*
 *	trident_256_restore(mode)	-- restore a Trident VGA from one 
 *					of its extended modes.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
trident_256_restore(mode)
int mode;
{
	int junk;

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	in_reg(&regtab[I_SEQ], 0xb, junk);	/* 64k mode */

	if (v256_is_color) {
		out_reg(&regtab[I_EGACOLOR], 0x1e, trident_256_test);
	}
	else {
		out_reg(&regtab[I_EGAMONO], 0x1e, trident_256_test);
	}

	out_reg(&regtab[I_SEQ], 0xb, junk);	/* 128k mode */
	out_reg(&regtab[I_SEQ], 0xd, trident_256_mode2);
	in_reg(&regtab[I_SEQ], 0xb, junk);	/* 64k mode */
	trident_256_mode1_new ^= 2;		/* flip bit two around */
	out_reg(&regtab[I_SEQ], 0xe, trident_256_mode1_new);
	out_reg(&regtab[I_SEQ], 0xd, trident_256_mode2_new);
	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}



/*
 * trident_256_selectpage(j)	-- select the current page based on the
 *				byte offset passed in. 
 *
 * Input:
 *	unsigned long	j	-- byte offset into video memory
 */
trident_256_selectpage(j)
register unsigned long j;
{
	v256_endpage = j | 0xffff;
	j >>= 16;
	if (j == v256_page)
		return;

	v256_page = j;

	/*
	 * have to flip the second bit around
	 */
	out_reg(&regtab[I_SEQ], 0xe, j ^ 0x2);
}


static unchar et4000_34;
static unchar et4000_35;
static int    legend_clock30;
/*
 *	legend_init(mode)	-- initialize a Sigma VGA LEGEND to one
 *				of it's extended modes.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
legend_init(mode)
int mode;
{
	static int inited = 0;

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	if (!inited) {
		inited = 1;
		
		/* 
		 * Set "KEY" so we can get to all regs.
		 */
		outb(0x3bf, 3);
		if (v256_is_color) {
			outb(0x3d8, 0xa0);
			in_reg(&regtab[I_EGACOLOR], 0x34, et4000_34);
			in_reg(&regtab[I_EGACOLOR], 0x35, et4000_35);
		}
		else {
			outb(0x3b8, 0xa0);
			in_reg(&regtab[I_EGAMONO], 0x34, et4000_34);
			in_reg(&regtab[I_EGAMONO], 0x35, et4000_35);
		}

		v256_maptbl = v256_et4000_tbl;
		et4000256_gdc_select = inb(0x3CD);
		get_reg(0x3c4, 0x07, &et4000256_seq_aux);
		(void)inb(vt_info.ad_addr + IN_STAT_1); /* init flip-flop */
		in_reg(&regtab[I_ATTR+1], 0x16, et4000256_attr_misc);

		/*
		 * See which clock chip is in use on this board.
		 */
		legend_clock30 = et4000_34 & 0x50;
	}

	(void)inb(vt_info.ad_addr + IN_STAT_1); /* init flip-flop */
	out_reg(&regtab[I_ATTR], 0x10, 0x01);

	switch(mode) {
	case LEGEND256_8:
		if (legend_clock30)
			legend_selclk(0xd);
		else {
			legend_selclk(0x5);
			vt_info.regs->miscreg = 0x63;
		}
		break;

	case LEGEND256_8a:
		legend_selclk(0x2);
		break;

	case LEGEND256_1:
		if (v256_is_color) {
			out_reg(&regtab[I_EGACOLOR], 0x35, 0x80);
		}
		else {
			out_reg(&regtab[I_EGAMONO], 0x35, 0x80);
		}

		if (legend_clock30)
			legend_selclk(0xc);
		else {
			legend_selclk(0x3);
			vt_info.regs->miscreg = 0x2b;
		}
		break;

	case LEGEND256_1ni:
		if (legend_clock30) 
			legend_selclk(0xa);
		else {
			legend_selclk(0x5);
			vt_info.regs->miscreg = 0x3f;
		}
		set_reg(0x3c4, 0x07, 0xe8);
		(void)inb(vt_info.ad_addr + IN_STAT_1);
		out_reg(&regtab[I_ATTR], 0x16, 0x90);
		break;
	}

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}
		


/*
 *	legend_rest(mode)	-- restore a Sigma VGA LEGEND from one
 *				of its extended modes.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
legend_rest(mode)
int mode;
{
	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	outb(0x3CD, et4000256_gdc_select);		/* reset segment */
	set_reg(0x3c4, 0x07, et4000256_seq_aux);
	(void)inb(vt_info.ad_addr + IN_STAT_1); /* init flip-flop */
	out_reg(&regtab[I_ATTR], 0x16, et4000256_attr_misc);

	if (legend_clock30)
		legend_selclk(0x3);
	else
		legend_selclk(0x2);

	if (v256_is_color) {
		out_reg(&regtab[I_EGACOLOR], 0x34, et4000_34);
		out_reg(&regtab[I_EGACOLOR], 0x35, et4000_35);
	}
	else {
		out_reg(&regtab[I_EGAMONO], 0x34, et4000_34);
		out_reg(&regtab[I_EGAMONO], 0x35, et4000_35);
	}

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}
		

/*
 * 	legend_selclk(clk)	-- select all the clock based on the data
 *				passed in.
 *
 *	Input:
 *		BYTE clk	-- clock data:
 *					bit 0	= clksel
 *					bit 1	= cs2 (0 if set cs2)
 *					bit 2   = cs2 (1 if set cs2) 
 *					bit 3   = sense
 */
legend_selclk(clk)
BYTE clk;
{
	BYTE	misc_dat;

	/*
	 * Write clk sel and sense bits
	 */
	misc_dat = inb(MISC_OUT_READ) & 0xf3;
	misc_dat |= ((clk & 1) << 2) | (clk & 8);
	outb(MISC_OUT, misc_dat);
	legend_jtoggle();

	if (v256_is_color) {
		out_reg(&regtab[I_EGACOLOR], 0x34, (clk & 2));
		out_reg(&regtab[I_EGACOLOR], 0x34, ((clk & 4) >> 1));
	}
	else {
		out_reg(&regtab[I_EGAMONO], 0x34, (clk & 2));
		out_reg(&regtab[I_EGAMONO], 0x34, ((clk & 4) >> 1));
	}
}


/*
 * legend_jtoggle()	-- perform the "J-Toggle" operation on a Sigma
 *			   VGA LEGEND board.  This is used to change the
 *			   meaning of the MISC_OUT register
 *
 */
legend_jtoggle()
{
	BYTE	crtc34;

	if (v256_is_color) {
		in_reg(&regtab[I_EGACOLOR], 0x34, crtc34);
		crtc34 = (crtc34 ^ 2) & 2;
		out_reg(&regtab[I_EGACOLOR], 0x34, crtc34);
	}
	else {
		in_reg(&regtab[I_EGAMONO], 0x34, crtc34);
		crtc34 = (crtc34 ^ 2) & 2;
		out_reg(&regtab[I_EGAMONO], 0x34, crtc34);
	}
}

