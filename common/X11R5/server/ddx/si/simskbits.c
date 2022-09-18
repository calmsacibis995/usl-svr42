/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/si/simskbits.c	1.2"

/*
 *	Copyright (c) 1991 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 *	All rights reserved.
 */

/************************************************************
Copyright 1987 by Sun Microsystems, Inc. Mountain View, CA.

                    All Rights Reserved
********************************************************/

/* $Header: simskbits.c,v 4.3 87/09/10 17:53:31 sun Exp $ */

/*
 * ==========================================================================
 * Converted to Color Frame Buffer by smarks@sun, April-May 1987.  The "bit 
 * numbering" in the doc below really means "byte numbering" now.
 * ==========================================================================
 */

/*
   these tables are used by several macros in the si code.

   the vax numbers everything left to right, so bit indices on the
screen match bit indices in longwords.  the pc-rt and Sun number
bits on the screen the way they would be written on paper,
(i.e. msb to the left), and so a bit index n on the screen is
bit index 32-n in a longword

   see also simskbits.h
*/
#include	<X.h>
#include	<Xmd.h>
#include	<servermd.h>

unsigned int sistarttab1[] = 
	{
	0x00000000, 0xFFFFFFFE, 0xFFFFFFFC, 0xFFFFFFF8,
	0xFFFFFFF0, 0xFFFFFFE0, 0xFFFFFFC0, 0xFFFFFF80,
	0xFFFFFF00, 0xFFFFFE00, 0xFFFFFC00, 0xFFFFF800,
	0xFFFFF000, 0xFFFFE000, 0xFFFFC000, 0xFFFF8000,
	0xFFFF0000, 0xFFFE0000, 0xFFFC0000, 0xFFF80000,
	0xFFF00000, 0xFFE00000, 0xFFC00000, 0xFF800000,
	0xFF000000, 0xFE000000, 0xFC000000, 0xF8000000,
	0xF0000000, 0xE0000000, 0xC0000000, 0x80000000,
	0x00000000,
	};

unsigned int siendtab1[] = 
	{
	0x00000000, 0x00000001, 0x00000003, 0x00000007,
	0x0000000F, 0x0000001F, 0x0000003F, 0x0000007F,
	0x000000FF, 0x000001FF, 0x000003FF, 0x000007FF,
	0x00000FFF, 0x00001FFF, 0x00003FFF, 0x00007FFF,
	0x0000FFFF, 0x0001FFFF, 0x0003FFFF, 0x0007FFFF,
	0x000FFFFF, 0x001FFFFF, 0x003FFFFF, 0x007FFFFF,
	0x00FFFFFF, 0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF,
	0x0FFFFFFF, 0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF,
	0xFFFFFFFF,
	};

unsigned int sipfillmask4[] = {
	0x00000000, 0x11111111, 0x22222222, 0x33333333,
	0x44444444, 0x55555555, 0x66666666, 0x77777777,
	0x88888888, 0x99999999, 0xAAAAAAAA, 0xBBBBBBBB,
	0xCCCCCCCC, 0xDDDDDDDD, 0xEEEEEEEE, 0xFFFFFFFF,
};

unsigned int sipfillmask2[] = {
	0x00000000,
	0x55555555,
	0xAAAAAAAA,
	0xFFFFFFFF,
};

unsigned int sipfillmask1[] = {
	0x00000000, 0xFFFFFFFF,
};

/*
 * QuartetBitsTable contains masks whose binary values are masks in the 
 * low order quartet that contain the number of bits specified in the 
 * index.  This table is used by getstipplepixels.  Quartet is a misnomer
 * as the value 4 only applies to 8 bit deep pixels.  Note that for any
 * depth, we can use the same table.
 */
unsigned int QuartetBitsTable[] = {
#if (BITMAP_BIT_ORDER == MSBFirst)
    0x00000000,
    0x80000000, 0xC0000000, 0xE0000000, 0xF0000000,
    0xF8000000, 0xFC000000, 0xFE000000, 0xFF000000,
    0xFF800000, 0xFFC00000, 0xFFE00000, 0xFFF00000,
    0xFFF80000, 0xFFFC0000, 0xFFFE0000, 0xFFFF0000,
    0xFFFF8000, 0xFFFFC000, 0xFFFFE000, 0xFFFFF000,
    0xFFFFF800, 0xFFFFFC00, 0xFFFFFE00, 0xFFFFFF00,
    0xFFFFFF80, 0xFFFFFFC0, 0xFFFFFFE0, 0xFFFFFFF0,
    0xFFFFFFF8, 0xFFFFFFFC, 0xFFFFFFFE, 0xFFFFFFFF,
#else /* (BITMAP_BIT_ORDER == LSBFirst */
    0x00000000,
    0x00000001, 0x00000003, 0x00000007, 0x0000000F,
    0x0000001F, 0x0000003F, 0x0000007F, 0x000000FF,
    0x000001FF, 0x000003FF, 0x000007FF, 0x00000FFF,
    0x00001FFF, 0x00003FFF, 0x00007FFF, 0x0000FFFF,
    0x0001FFFF, 0x0003FFFF, 0x0007FFFF, 0x000FFFFF,
    0x001FFFFF, 0x003FFFFF, 0x007FFFFF, 0x00FFFFFF,
    0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF, 0x0FFFFFFF,
    0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF,
#endif /* (BITMAP_BIT_ORDER == MSBFirst) */
};

int PPW;
int PWSH;
int PSZ;
int PMSK;

/* 
 * This is used to determine the correct value of PWSH.  It is 
 * indexed by PPW.
 */
unsigned char si_pix_to_word[33] = {
	0, 0, 1, 1, 2, 2, 2, 2, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4,
	5, 
};

#ifdef	vax
#undef	VAXBYTEORDER
#endif
