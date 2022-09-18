/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga16:vga16/vgarop.h	1.1"

/*
 *	Copyright (c) 1991 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 */

/* Definitions to make rasterop suitable for either Shapes or Xwin.  This
 * particular rendition is for SI/Xwin.
 */

#ifndef _RASTEROP_
#define _RASTEROP_

#include "Xmd.h"
#include "sidep.h"
#include "miscstruct.h"
#include <sys/types.h>
#include <sys/at_ansi.h>
#include <sys/kd.h>
#include "vgaregs.h"
#include "vtio.h"
#include "vga.h"
#include <sys/inline.h>

/* The type of pSrc and pDest must be a pointer to something. */

typedef SIbitmapP RASTER;

#define SCREEN_TYPE_RASTER(r)	((r) == (SIbitmapP) 0)
#define RASTER_DEPTH(r)	(SCREEN_TYPE_RASTER(r) ? 4 : (r)->Bdepth)
#define RASTER_BITS(r)	(SCREEN_TYPE_RASTER(r) ? (unsigned *) vga_fb : \
			 (unsigned *) (r)->Bptr)
#define RASTER_BYTES_PER_LINE(r)	(SCREEN_TYPE_RASTER(r) ? \
                 vga_slbytes : (((r)->Bwidth + 0x1f) & ~0x1f) >> 3)
#define RASTER_HEIGHT(r)	((r)->Bheight)
#define RASTER_WIDTH(r)	((r)->Bwidth)	

#define ERROR_MSG(s)

#define ROP_CLEAR		GXclear
#define ROP_AND			GXand
#define ROP_AND_REVERSE		GXandReverse
#define ROP_COPY		GXcopy
#define ROP_AND_INVERSE		GXandInverted
#define ROP_NO_OP		GXnoop
#define ROP_XOR			GXxor
#define ROP_OR			GXor
#define ROP_NOR			GXnor
#define ROP_EQUIV		GXequiv
#define ROP_INVERT		GXinvert
#define ROP_OR_REVERSE		GXorReverse
#define ROP_COPY_INVERT		GXcopyInverted
#define ROP_OR_INVERSE		GXorInverted
#define ROP_NAND		GXnand
#define ROP_SET			GXset

#define USE_EGA()
#define FINISHED_WITH_EGA()

#endif
