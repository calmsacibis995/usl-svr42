#ident	"@(#)xpr:devices/terminfo/R_14.c	1.2"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "Xlib.h"

#include "xpr.h"

#include "xpr_term.h"
#include "text.h"

static Word		bits[] = {
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x0010002e,0x03fc0000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x0010002e,0x0ffe0000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00c00000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x0397003b,0x1fff8000,0x66601983,
	0x00410060,0x0c050040,0x00000000,0x038020c0,0x700207c0,
	0x67f0e030,0x00000000,0x00000070,0x07fc0080,0xff80787f,
	0xc1ff9ff8,0x1e4f8fbf,0x3f7e7cf8,0x0f007381,0xf0781f00,
	0x3c0fc006,0x4000f87b,0xe1efbe3d,0xf8f3e1e7,0xfc3c078c,
	0x00000030,0x00003000,0x3800300c,0x19803000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x0611003b,0x3c638000,
	0x6660190f,0xc1f700f8,0x0e0d8040,0x04000000,0x47c0e3f0,
	0xf8060fc3,0xe7f1f8fc,0x00000000,0x000001f8,0x1c060080,
	0x71c1fe38,0xe0e18e18,0x7fc7071e,0x1e3c3070,0x0700e1c0,
	0xe1fe1ff0,0xff0ff81f,0xcffef87b,0xe1efbe3d,0xf8f3e1e7,
	0xfe3d079c,0x00040070,0x00007000,0x7c00700c,0x1b807000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x06118fbb,
	0x73f9c000,0x66601318,0x433e0198,0x0e18c358,0x04000000,
	0x4c61e3f1,0x9c060f87,0x0ff3188e,0x00000600,0x00c0011c,
	0x380301c0,0x30e78618,0x30608609,0xe1c3030c,0x0c186030,
	0x0380e0e0,0x43870e79,0xc3871c18,0xcc663011,0xc0471c08,
	0x7061e0c4,0x0e2100b6,0x00080030,0x00003000,0x4c00300c,
	0x19803000,0x00000000,0x00000000,0x80000000,0x00000000,
	0x06118fbb,0x73fce000,0x64403318,0x07060198,0x023043f8,
	0x04000000,0x98606439,0x0c0e180e,0x08330986,0x00003e00,
	0x00f8018c,0x600101c0,0x30e70318,0x18600601,0xc0c3030c,
	0x0c18c030,0x038160f0,0x43038619,0x81c30c10,0x48623010,
	0xc0c30c10,0x38c0e180,0x1c2100a2,0x000e0030,0x00003000,
	0x40003000,0x01801000,0x00000000,0x00000000,0x80000000,
	0x00000000,0x0611802e,0xe7fc6000,0x64403218,0x06240198,
	0x0c2061f0,0x04000000,0x98306418,0x0c1e1e0c,0x00231986,
	0x0000f000,0x001e018c,0x61e881e0,0x30ee0118,0x18600603,
	0x8043030c,0x0c198030,0x03c160b0,0x4701861b,0x80c30c18,
	0x00603010,0xe0830e10,0x39807100,0x38210080,0x000e1030,
	0x0101b040,0x40303200,0x01801000,0x00080000,0x80400041,
	0x80000000,0x00000000,0x0611802e,0xc60c6000,0x6003ffde,
	0x062c01f7,0x886023f8,0x04000000,0x98306018,0x18361f1f,
	0xc023b986,0x6607c03f,0xfc07c018,0xc3388260,0x31ce0018,
	0x0c610613,0x8003030c,0x0c1b0030,0x02c160b8,0x4601c61b,
	0x00e30c1c,0x00603010,0x61838e30,0x1f003300,0x30208080,
	0x000c7c3f,0x87c3f1f1,0xf87f3f9c,0x399e11de,0xf1fe1f1f,
	0xe1f9d9f3,0xee73cef7,0x9bdcf39f,0x8611802e,0xc60c3000,
	0x6003ffcf,0x0649c1c7,0x80603358,0x04000001,0x18306010,
	0x38260f9f,0xe061f186,0x661e003f,0xfc00f018,0xc6188260,
	0x3f8c0018,0x0c7f07f3,0x0003030c,0x0c1e0030,0x02e2609c,
	0x4600c63b,0x00633c1f,0x00603010,0x71018f20,0x0e003e00,
	0x70208080,0x0000c43f,0xccc67118,0x40cf3f9c,0x399811ff,
	0xf1fe339f,0xf319f911,0x86318463,0x19d86103,0x8611803b,
	0xc60c3000,0x00006407,0xc7dbe3e2,0x00603248,0xffe00001,
	0x18306030,0x7c4601d8,0xe040f1c6,0x00380000,0x00003810,
	0x86108630,0x318c0018,0x0c610613,0x03e3ff0c,0x0c1f0030,
	0x02e2608e,0x4600c7f3,0x0063f80f,0xc0603010,0x3101df20,
	0x0e001c00,0xe020c080,0x0000c431,0xd8cc33f8,0x4084318c,
	0x19b010c6,0x10c6618c,0x3618c181,0x8631c463,0x10f07103,
	0x1c10c03f,0xce0e3000,0x0007ff81,0xe31726e6,0x00603040,
	0xffe03e01,0x18306060,0x0cc600d8,0x6041f8fe,0x003c003f,
	0xfc00f820,0x8c3087f0,0x30cc0018,0x0c600603,0x00c3030c,
	0x0c1b8030,0x02766087,0x4601c7c3,0x0063f003,0xc0603010,
	0x3a00d360,0x0f000c01,0xc0204080,0x00001c30,0xd80c33f8,
	0x40c4318c,0x19e010c2,0x18c660cc,0x3618c1e1,0x8630c833,
	0x90603207,0x1c10e000,0xce073000,0x0007ff80,0xe0362c74,
	0x00603000,0x04003e02,0x183060c0,0x06ff80d8,0x20c31876,
	0x000f803f,0xfc03e020,0x8c300ff8,0x30ee0018,0x1c600603,
	0x80c3030c,0x0c19c030,0x02746083,0x4601c603,0x00e33801,
	0xe0603010,0x3a00d3c0,0x1b800c01,0xc0206080,0x00007430,
	0xd80c3200,0x40c4318c,0x19f010c2,0x18c660cc,0x3618c0f1,
	0x8630c837,0xa070320e,0x0c11c000,0xcfff6000,0x0007ff80,
	0x60662c3c,0x00603000,0x04003e02,0x18306080,0x04ff80d8,
	0x20c30c0c,0x0001e000,0x001f0000,0x8c310818,0x30e60018,
	0x18600603,0x80c3030c,0x0c18e030,0x023c6083,0xc7018603,
	0x80c33820,0xe0603010,0x1e00e1c0,0x31c00c03,0x81202080,
	0x0000c430,0xd82c3304,0x407c318c,0x19b030c2,0x18c660cc,
	0x3618c079,0x86307835,0xa0701a0c,0x0611803b,0xefff6000,
	0x00019810,0x60444c3c,0x40603000,0x04000002,0x08606104,
	0x04060098,0x60830c1c,0x00007c00,0x007c0000,0xc472081c,
	0x30e70118,0x38604601,0xc0c3030c,0x6c187030,0x22386081,
	0xc3038601,0x81c31c20,0x60603020,0x1c0061c0,0x61c00c07,
	0x02202080,0x0000c430,0xdc6c338c,0x40d0318c,0x199830c2,
	0x18c6608c,0x3618c119,0x8630701c,0xe0981c1c,0x46118000,
	0x6060e000,0x60019818,0xe0c4ce7f,0xc0602000,0x04038034,
	0x0c6063f9,0x8c02318c,0x61830838,0x67000e00,0x00e00060,
	0x47be180c,0x71c38338,0xf0e0ce01,0xe1c7071e,0x6c183830,
	0x66186080,0xc3870e01,0xc3870e38,0xc0e03860,0x0c006180,
	0xc0e00c0f,0x06201080,0x0000ff31,0x9fcf7bf8,0xc0fc318c,
	0x199c30c2,0x18c6318e,0x67f8c119,0xa7f83018,0xc19c0c38,
	0x4611800c,0x7c63c000,0x6001301f,0xc0878fef,0x80306000,
	0x04038034,0x07c1f7f9,0xf8023f07,0xc181f9f0,0x67000200,
	0x00800060,0x70023c3e,0xff80fc7f,0xc1ffdf00,0x7fcf8fbf,
	0x787e7eff,0xcf91fbc0,0x40fe1f00,0xff0f87bf,0xc1f01fc0,
	0x0c004081,0xe1f83f0f,0xfe201080,0x0000ff3f,0x0f87f9f1,
	0xf0fe7bdf,0x1bef7def,0xbdef3f0f,0xe3f9f1f1,0xe3f82008,
	0x439e0c3f,0x86118000,0x1f3f8000,0x00000000,0x00000000,
	0x0018c000,0x00010000,0x00000000,0x00000000,0x00000000,
	0x02000000,0x00000000,0x1c0e0000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x1e000000,
	0x00000000,0x00000000,0x00000000,0x00300080,0xffc00000,
	0x00000000,0x01800000,0x18000000,0x0000000c,0x00180000,
	0x00000000,0x00001800,0x0611802e,0x0ffe0000,0x00000000,
	0x00000000,0x000d8000,0x00030000,0x00000000,0x00000000,
	0x00000000,0x06000000,0x00000000,0x07f80000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x0f800000,0x00000000,0x00000000,0x00000000,0x003c0780,
	0xffc00000,0x00000000,0x01c60000,0x78000000,0x0000000c,
	0x00180000,0x00000000,0x0000f000,0x07130003,0x03fc0000,
	0x00000000,0x00000000,0x00050000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x03e00000,0x00000000,0x00000000,0x00000000,
	0x003c0780,0x00000000,0x00000000,0x01fc0000,0x70000000,
	0x0000001f,0x007c0000,0x00000000,0x0000f000,0x0397003f,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00700000,
	0x60000000,0x0000001f,0x007c0000,0x00000000,0x00006000,
	0x00100000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x0000003f,
};

static XImage		image = {
	986,				/* width */
	22,				/* height */
	0,				/* xoffset */
	XYBitmap,			/* format */
	(char *)bits,			/* data */
	LSBFirst,			/* byte_order */
	WORDSIZE,			/* bitmap_unit */
	MSBFirst,			/* bitmap_bit_order */
	WORDSIZE,			/* bitmap_pad */
	1,				/* depth */
	ROUNDUP(986, WORDSIZE) / 8,	/* bytes_per_line */
	1,				/* bits_per_pixel */
};

struct {
	short			n;
	char			height,
				ascent;
	XImage			*image;
	_Fontchar		info[129];
}			R_14 = {
	127,
	 22,
	 16,
	&image,
	{
		{   0,  16,  16,   0,   0 },
		{   0,  16,  16,   0,   0 },
		{   0,  16,  16,   0,   0 },
		{   0,  16,  16,   0,   0 },
		{   0,  16,  16,   0,   0 },
		{   0,  16,  16,   0,   0 },
		{   0,  16,  16,   0,   0 },
		{   0,   1,  20,   0,  21 },
		{  21,   1,   1,   0,   0 },
		{  21,   1,   1,   0,   0 },
		{  21,   1,   1,   0,   0 },
		{  21,   1,   1,   0,   0 },
		{  21,   1,   1,   0,   0 },
		{  21,   1,   1,   0,   0 },
		{  21,   1,   1,   0,   0 },
		{  21,   1,   1,   0,   0 },
		{  21,   1,   1,   0,   0 },
		{  21,   1,   1,   0,   0 },
		{  21,   1,   1,   0,   0 },
		{  21,   1,   1,   0,   0 },
		{  21,   1,   1,   0,   0 },
		{  21,   1,   1,   0,   0 },
		{  21,   1,   1,   0,   0 },
		{  21,   1,   1,   0,   0 },
		{  21,   1,   1,   0,   0 },
		{  21,   1,   1,   0,   0 },
		{  21,   1,   1,   0,   0 },
		{  21,   1,   1,   0,   0 },
		{  21,   1,   1,   0,   0 },
		{  21,   1,   1,   0,   0 },
		{  21,   1,   1,   0,   0 },
		{  21,   1,   1,   0,   0 },
		{  21,   0,  22,   0,  10 },
		{  31,   3,  17,   0,   5 },
		{  36,   3,  16,   0,   8 },
		{  44,   3,  17,   0,  15 },
		{  59,   3,  17,   0,   9 },
		{  68,   3,  17,   0,  16 },
		{  84,   3,  17,   0,  15 },
		{  99,   3,  16,   0,   5 },
		{ 104,   3,  20,   0,   7 },
		{ 111,   3,  20,   0,   6 },
		{ 117,   3,  16,   0,   9 },
		{ 126,   4,  17,   0,  14 },
		{ 140,  15,  19,   0,   6 },
		{ 146,  11,  16,   0,   6 },
		{ 152,  15,  17,   0,   5 },
		{ 157,   4,  17,   0,   6 },
		{ 163,   3,  17,   0,  10 },
		{ 173,   3,  17,   0,   8 },
		{ 181,   3,  17,   0,  10 },
		{ 191,   3,  17,   0,   9 },
		{ 200,   3,  17,   0,  10 },
		{ 210,   3,  17,   0,   9 },
		{ 219,   3,  17,   0,   9 },
		{ 228,   3,  17,   0,   9 },
		{ 237,   3,  17,   0,  10 },
		{ 247,   3,  17,   0,   9 },
		{ 256,   8,  17,   0,   4 },
		{ 260,   8,  19,   0,   5 },
		{ 265,   5,  17,   0,  15 },
		{ 280,   8,  16,   0,  15 },
		{ 295,   5,  17,   0,  15 },
		{ 310,   3,  17,   0,   9 },
		{ 319,   2,  19,   0,  19 },
		{ 338,   3,  17,   0,  14 },
		{ 352,   3,  17,   0,  12 },
		{ 364,   3,  17,   0,  13 },
		{ 377,   3,  17,   0,  14 },
		{ 391,   3,  17,   0,  12 },
		{ 403,   3,  17,   0,  11 },
		{ 414,   3,  17,   0,  14 },
		{ 428,   3,  17,   0,  14 },
		{ 442,   3,  17,   0,   7 },
		{ 449,   3,  17,   0,   8 },
		{ 457,   3,  17,   0,  15 },
		{ 472,   3,  17,   0,  12 },
		{ 484,   3,  17,   0,  18 },
		{ 502,   3,  17,   0,  15 },
		{ 517,   3,  17,   0,  14 },
		{ 531,   3,  17,   0,  11 },
		{ 542,   3,  20,   0,  14 },
		{ 556,   3,  17,   0,  14 },
		{ 570,   3,  17,   0,  10 },
		{ 580,   4,  17,   0,  12 },
		{ 592,   3,  17,   0,  14 },
		{ 606,   3,  17,   0,  14 },
		{ 620,   3,  17,   0,  19 },
		{ 639,   3,  17,   0,  15 },
		{ 654,   3,  17,   0,  14 },
		{ 668,   3,  17,   0,  13 },
		{ 681,   3,  20,   0,   6 },
		{ 687,   4,  17,   0,   6 },
		{ 693,   3,  20,   0,   5 },
		{ 698,   3,  16,   0,   6 },
		{ 704,  16,  19,   0,  11 },
		{ 715,   4,  16,   0,   5 },
		{ 720,   7,  17,   0,   9 },
		{ 729,   3,  17,   0,  10 },
		{ 739,   7,  17,   0,   9 },
		{ 748,   3,  17,   0,  10 },
		{ 758,   7,  17,   0,   9 },
		{ 767,   3,  17,   0,   8 },
		{ 775,   7,  21,   0,  10 },
		{ 785,   3,  17,   0,  10 },
		{ 795,   3,  17,   0,   6 },
		{ 801,   3,  21, 255,   4 },
		{ 806,   3,  17,   0,  11 },
		{ 817,   3,  17,   0,   6 },
		{ 823,   8,  17,   0,  16 },
		{ 839,   7,  17,   0,  10 },
		{ 849,   8,  17,   0,  10 },
		{ 859,   7,  21,   0,  10 },
		{ 869,   7,  21,   0,  10 },
		{ 879,   8,  17,   0,   7 },
		{ 886,   7,  17,   0,   8 },
		{ 894,   5,  17,   0,   6 },
		{ 900,   8,  17,   0,  10 },
		{ 910,   8,  17,   0,  10 },
		{ 920,   8,  17,   0,  14 },
		{ 934,   8,  17,   0,  10 },
		{ 944,   8,  21,   0,  10 },
		{ 954,   8,  17,   0,   9 },
		{ 963,   2,  20,   0,   7 },
		{ 970,   0,  21,   0,   3 },
		{ 973,   2,  20,   0,   7 },
		{ 980,   4,  16,   0,   6 },
		{ 986,   0,   0,   0,   0 },
		{ 986,   0,   0,   0,   0 },
	}
};