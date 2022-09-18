/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/si/simisc.c	1.3"

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

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved
******************************************************************/
/* $XConsortium: mfbmisc.c,v 5.0 89/06/09 15:06:49 keith Exp $ */

#include "X.h"
#include "misc.h"
#include "cursor.h"
#include "screenint.h"

#include "si.h"		/* SI */
#include "sidep.h"	/* SI */

/*ARGSUSED*/
void
siQueryBestSize(class, pwidth, pheight)
int class;
short *pwidth;
short *pheight;
{
    unsigned width, test;
    /* SI: START */
    int set = 0;

    switch(class)
    {
      case CursorShape:
	  *pwidth = si_bestcursorwidth;
	  *pheight = si_bestcursorheight;
	  return;
      case TileShape:
 	  if ((*pwidth > si_GetInfoVal(SItilewidth)) || *pwidth==0) {
		*pwidth = si_GetInfoVal(SItilewidth);
		set++;
	  }
 	  if ((*pheight > si_GetInfoVal(SItileheight)) || *pheight==0) {
		*pheight = si_GetInfoVal(SItileheight);
		set++;
	  }
	  if (set)
		return;
	  break;
      case StippleShape:
 	  if ((*pwidth > si_GetInfoVal(SIstipplewidth)) || *pwidth==0) {
		*pwidth = si_GetInfoVal(SIstipplewidth);
		set++;
	  }
 	  if ((*pheight > si_GetInfoVal(SIstippleheight)) || *pheight==0) {
		*pheight = si_GetInfoVal(SIstippleheight);
		set++;
	  }
	  if (set)
		return;
	  break;
    }
    /* SI: END */

	  width = *pwidth;
	  /* Return the closes power of two not less than what they gave me */
	  test = 0x80000000;
	  /* Find the highest 1 bit in the width given */
	  while(!(test & width))
	     test >>= 1;
	  /* If their number is greater than that, bump up to the next
	   *  power of two */
	  if((test - 1) & width)
	     test <<= 1;
	  *pwidth = test;
	  /* We don't care what height they use */
}

int
ReduceRop(alu, src)
    register unsigned char alu;
    register Pixel src;
{
    int rop;
    if (src == 0)	/* src is black */
    {
	switch(alu)
	{
	  case GXclear:
	    rop = RROP_BLACK;
	    break;
	  case GXand:
	    rop = RROP_BLACK;
	    break;
	  case GXandReverse:
	    rop = RROP_BLACK;
	    break;
	  case GXcopy:
	    rop = RROP_BLACK;
	    break;
	  case GXandInverted:
	    rop = RROP_NOP;
	    break;
	  case GXnoop:
	    rop = RROP_NOP;
	    break;
	  case GXxor:
	    rop = RROP_NOP;
	    break;
	  case GXor:
	    rop = RROP_NOP;
	    break;
	  case GXnor:
	    rop = RROP_INVERT;
	    break;
	  case GXequiv:
	    rop = RROP_INVERT;
	    break;
	  case GXinvert:
	    rop = RROP_INVERT;
	    break;
	  case GXorReverse:
	    rop = RROP_INVERT;
	    break;
	  case GXcopyInverted:
	    rop = RROP_WHITE;
	    break;
	  case GXorInverted:
	    rop = RROP_WHITE;
	    break;
	  case GXnand:
	    rop = RROP_WHITE;
	    break;
	  case GXset:
	    rop = RROP_WHITE;
	    break;
	}
    }
    else /* src is white */
    {
	switch(alu)
	{
	  case GXclear:
	    rop = RROP_BLACK;
	    break;
	  case GXand:
	    rop = RROP_NOP;
	    break;
	  case GXandReverse:
	    rop = RROP_INVERT;
	    break;
	  case GXcopy:
	    rop = RROP_WHITE;
	    break;
	  case GXandInverted:
	    rop = RROP_BLACK;
	    break;
	  case GXnoop:
	    rop = RROP_NOP;
	    break;
	  case GXxor:
	    rop = RROP_INVERT;
	    break;
	  case GXor:
	    rop = RROP_WHITE;
	    break;
	  case GXnor:
	    rop = RROP_BLACK;
	    break;
	  case GXequiv:
	    rop = RROP_NOP;
	    break;
	  case GXinvert:
	    rop = RROP_INVERT;
	    break;
	  case GXorReverse:
	    rop = RROP_WHITE;
	    break;
	  case GXcopyInverted:
	    rop = RROP_BLACK;
	    break;
	  case GXorInverted:
	    rop = RROP_NOP;
	    break;
	  case GXnand:
	    rop = RROP_INVERT;
	    break;
	  case GXset:
	    rop = RROP_WHITE;
	    break;
	}
    }
    return rop;
}

