/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/v256stpl.c	1.2"

/*
 *    Copyright (c) 1991 USL
 *    All Rights Reserved 
 *
 *    THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *    The copyright notice above does not evidence any 
 *    actual or intended publication of such source code.
 *
 *    Copyrighted as an unpublished work.
 *    (c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 *    All rights reserved.
 */

#include "Xmd.h"
#include "sidep.h"
#include "miscstruct.h"
#include "sys/types.h"
#include "sys/at_ansi.h"
#include "sys/kd.h"
#include "vtio.h"
#include "v256.h"
#include "sys/inline.h"
#include "v256spreq.h"
#include "newfill.h"

extern BYTE v256_pat_fg;
extern BYTE v256_pat_bg;
extern int v256_blocksize;

BYTE v256_startmask[8] = {0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80};
BYTE v256_endmask[8] = {0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF};

unsigned long v256_expand[256] = {
    0x00000000, 0x01010101, 0x02020202, 0x03030303, 0x04040404, 0x05050505,
    0x06060606, 0x07070707, 0x08080808, 0x09090909, 0x0A0A0A0A, 0x0B0B0B0B,
    0x0C0C0C0C, 0x0D0D0D0D, 0x0E0E0E0E, 0x0F0F0F0F,

    0x10101010, 0x11111111, 0x12121212, 0x13131313, 0x14141414, 0x15151515,
    0x16161616, 0x17171717, 0x18181818, 0x19191919, 0x1A1A1A1A, 0x1B1B1B1B,
    0x1C1C1C1C, 0x1D1D1D1D, 0x1E1E1E1E, 0x1F1F1F1F,

    0x20202020, 0x21212121, 0x22222222, 0x23232323, 0x24242424, 0x25252525,
    0x26262626, 0x27272727, 0x28282828, 0x29292929, 0x2A2A2A2A, 0x2B2B2B2B,
    0x2C2C2C2C, 0x2D2D2D2D, 0x2E2E2E2E, 0x2F2F2F2F,

    0x30303030, 0x31313131, 0x32323232, 0x33333333, 0x34343434, 0x35353535,
    0x36363636, 0x37373737, 0x38383838, 0x39393939, 0x3A3A3A3A, 0x3B3B3B3B,
    0x3C3C3C3C, 0x3D3D3D3D, 0x3E3E3E3E, 0x3F3F3F3F,

    0x40404040, 0x41414141, 0x42424242, 0x43434343, 0x44444444, 0x45454545,
    0x46464646, 0x47474747, 0x48484848, 0x49494949, 0x4A4A4A4A, 0x4B4B4B4B,
    0x4C4C4C4C, 0x4D4D4D4D, 0x4E4E4E4E, 0x4F4F4F4F,

    0x50505050, 0x51515151, 0x52525252, 0x53535353, 0x54545454, 0x55555555,
    0x56565656, 0x57575757, 0x58585858, 0x59595959, 0x5A5A5A5A, 0x5B5B5B5B,
    0x5C5C5C5C, 0x5D5D5D5D, 0x5E5E5E5E, 0x5F5F5F5F,

    0x60606060, 0x61616161, 0x62626262, 0x63636363, 0x64646464, 0x65656565,
    0x66666666, 0x67676767, 0x68686868, 0x69696969, 0x6A6A6A6A, 0x6B6B6B6B,
    0x6C6C6C6C, 0x6D6D6D6D, 0x6E6E6E6E, 0x6F6F6F6F,

    0x70707070, 0x71717171, 0x72727272, 0x73737373, 0x74747474, 0x75757575,
    0x76767676, 0x77777777, 0x78787878, 0x79797979, 0x7A7A7A7A, 0x7B7B7B7B,
    0x7C7C7C7C, 0x7D7D7D7D, 0x7E7E7E7E, 0x7F7F7F7F,

    0x80808080, 0x81818181, 0x82828282, 0x83838383, 0x84848484, 0x85858585,
    0x86868686, 0x87878787, 0x88888888, 0x89898989, 0x8A8A8A8A, 0x8B8B8B8B,
    0x8C8C8C8C, 0x8D8D8D8D, 0x8E8E8E8E, 0x8F8F8F8F,

    0x90909090, 0x91919191, 0x92929292, 0x93939393, 0x94949494, 0x95959595,
    0x96969696, 0x97979797, 0x98989898, 0x99999999, 0x9A9A9A9A, 0x9B9B9B9B,
    0x9C9C9C9C, 0x9D9D9D9D, 0x9E9E9E9E, 0x9F9F9F9F,

    0xA0A0A0A0, 0xA1A1A1A1, 0xA2A2A2A2, 0xA3A3A3A3, 0xA4A4A4A4, 0xA5A5A5A5,
    0xA6A6A6A6, 0xA7A7A7A7, 0xA8A8A8A8, 0xA9A9A9A9, 0xAAAAAAAA, 0xABABABAB,
    0xACACACAC, 0xADADADAD, 0xAEAEAEAE, 0xAFAFAFAF,

    0xB0B0B0B0, 0xB1B1B1B1, 0xB2B2B2B2, 0xB3B3B3B3, 0xB4B4B4B4, 0xB5B5B5B5,
    0xB6B6B6B6, 0xB7B7B7B7, 0xB8B8B8B8, 0xB9B9B9B9, 0xBABABABA, 0xBBBBBBBB,
    0xBCBCBCBC, 0xBDBDBDBD, 0xBEBEBEBE, 0xBFBFBFBF,

    0xC0C0C0C0, 0xC1C1C1C1, 0xC2C2C2C2, 0xC3C3C3C3, 0xC4C4C4C4, 0xC5C5C5C5,
    0xC6C6C6C6, 0xC7C7C7C7, 0xC8C8C8C8, 0xC9C9C9C9, 0xCACACACA, 0xCBCBCBCB,
    0xCCCCCCCC, 0xCDCDCDCD, 0xCECECECE, 0xCFCFCFCF,

    0xD0D0D0D0, 0xD1D1D1D1, 0xD2D2D2D2, 0xD3D3D3D3, 0xD4D4D4D4, 0xD5D5D5D5,
    0xD6D6D6D6, 0xD7D7D7D7, 0xD8D8D8D8, 0xD9D9D9D9, 0xDADADADA, 0xDBDBDBDB,
    0xDCDCDCDC, 0xDDDDDDDD, 0xDEDEDEDE, 0xDFDFDFDF,

    0xE0E0E0E0, 0xE1E1E1E1, 0xE2E2E2E2, 0xE3E3E3E3, 0xE4E4E4E4, 0xE5E5E5E5,
    0xE6E6E6E6, 0xE7E7E7E7, 0xE8E8E8E8, 0xE9E9E9E9, 0xEAEAEAEA, 0xEBEBEBEB,
    0xECECECEC, 0xEDEDEDED, 0xEEEEEEEE, 0xEFEFEFEF,

    0xF0F0F0F0, 0xF1F1F1F1, 0xF2F2F2F2, 0xF3F3F3F3, 0xF4F4F4F4, 0xF5F5F5F5,
    0xF6F6F6F6, 0xF7F7F7F7, 0xF8F8F8F8, 0xF9F9F9F9, 0xFAFAFAFA, 0xFBFBFBFB,
    0xFCFCFCFC, 0xFDFDFDFD, 0xFEFEFEFE, 0xFFFFFFFF,
};

unsigned long v256_lower[256] = {
    0x00000000, 0x000000FF, 0x0000FF00, 0x0000FFFF, 0x00FF0000, 0x00FF00FF,
    0x00FFFF00, 0x00FFFFFF, 0xFF000000, 0xFF0000FF, 0xFF00FF00, 0xFF00FFFF,
    0xFFFF0000, 0xFFFF00FF, 0xFFFFFF00, 0xFFFFFFFF,
    0x00000000, 0x000000FF, 0x0000FF00, 0x0000FFFF, 0x00FF0000, 0x00FF00FF,
    0x00FFFF00, 0x00FFFFFF, 0xFF000000, 0xFF0000FF, 0xFF00FF00, 0xFF00FFFF,
    0xFFFF0000, 0xFFFF00FF, 0xFFFFFF00, 0xFFFFFFFF,
    0x00000000, 0x000000FF, 0x0000FF00, 0x0000FFFF, 0x00FF0000, 0x00FF00FF,
    0x00FFFF00, 0x00FFFFFF, 0xFF000000, 0xFF0000FF, 0xFF00FF00, 0xFF00FFFF,
    0xFFFF0000, 0xFFFF00FF, 0xFFFFFF00, 0xFFFFFFFF,
    0x00000000, 0x000000FF, 0x0000FF00, 0x0000FFFF, 0x00FF0000, 0x00FF00FF,
    0x00FFFF00, 0x00FFFFFF, 0xFF000000, 0xFF0000FF, 0xFF00FF00, 0xFF00FFFF,
    0xFFFF0000, 0xFFFF00FF, 0xFFFFFF00, 0xFFFFFFFF,

    0x00000000, 0x000000FF, 0x0000FF00, 0x0000FFFF, 0x00FF0000, 0x00FF00FF,
    0x00FFFF00, 0x00FFFFFF, 0xFF000000, 0xFF0000FF, 0xFF00FF00, 0xFF00FFFF,
    0xFFFF0000, 0xFFFF00FF, 0xFFFFFF00, 0xFFFFFFFF,
    0x00000000, 0x000000FF, 0x0000FF00, 0x0000FFFF, 0x00FF0000, 0x00FF00FF,
    0x00FFFF00, 0x00FFFFFF, 0xFF000000, 0xFF0000FF, 0xFF00FF00, 0xFF00FFFF,
    0xFFFF0000, 0xFFFF00FF, 0xFFFFFF00, 0xFFFFFFFF,
    0x00000000, 0x000000FF, 0x0000FF00, 0x0000FFFF, 0x00FF0000, 0x00FF00FF,
    0x00FFFF00, 0x00FFFFFF, 0xFF000000, 0xFF0000FF, 0xFF00FF00, 0xFF00FFFF,
    0xFFFF0000, 0xFFFF00FF, 0xFFFFFF00, 0xFFFFFFFF,
    0x00000000, 0x000000FF, 0x0000FF00, 0x0000FFFF, 0x00FF0000, 0x00FF00FF,
    0x00FFFF00, 0x00FFFFFF, 0xFF000000, 0xFF0000FF, 0xFF00FF00, 0xFF00FFFF,
    0xFFFF0000, 0xFFFF00FF, 0xFFFFFF00, 0xFFFFFFFF,

    0x00000000, 0x000000FF, 0x0000FF00, 0x0000FFFF, 0x00FF0000, 0x00FF00FF,
    0x00FFFF00, 0x00FFFFFF, 0xFF000000, 0xFF0000FF, 0xFF00FF00, 0xFF00FFFF,
    0xFFFF0000, 0xFFFF00FF, 0xFFFFFF00, 0xFFFFFFFF,
    0x00000000, 0x000000FF, 0x0000FF00, 0x0000FFFF, 0x00FF0000, 0x00FF00FF,
    0x00FFFF00, 0x00FFFFFF, 0xFF000000, 0xFF0000FF, 0xFF00FF00, 0xFF00FFFF,
    0xFFFF0000, 0xFFFF00FF, 0xFFFFFF00, 0xFFFFFFFF,
    0x00000000, 0x000000FF, 0x0000FF00, 0x0000FFFF, 0x00FF0000, 0x00FF00FF,
    0x00FFFF00, 0x00FFFFFF, 0xFF000000, 0xFF0000FF, 0xFF00FF00, 0xFF00FFFF,
    0xFFFF0000, 0xFFFF00FF, 0xFFFFFF00, 0xFFFFFFFF,
    0x00000000, 0x000000FF, 0x0000FF00, 0x0000FFFF, 0x00FF0000, 0x00FF00FF,
    0x00FFFF00, 0x00FFFFFF, 0xFF000000, 0xFF0000FF, 0xFF00FF00, 0xFF00FFFF,
    0xFFFF0000, 0xFFFF00FF, 0xFFFFFF00, 0xFFFFFFFF,

    0x00000000, 0x000000FF, 0x0000FF00, 0x0000FFFF, 0x00FF0000, 0x00FF00FF,
    0x00FFFF00, 0x00FFFFFF, 0xFF000000, 0xFF0000FF, 0xFF00FF00, 0xFF00FFFF,
    0xFFFF0000, 0xFFFF00FF, 0xFFFFFF00, 0xFFFFFFFF,
    0x00000000, 0x000000FF, 0x0000FF00, 0x0000FFFF, 0x00FF0000, 0x00FF00FF,
    0x00FFFF00, 0x00FFFFFF, 0xFF000000, 0xFF0000FF, 0xFF00FF00, 0xFF00FFFF,
    0xFFFF0000, 0xFFFF00FF, 0xFFFFFF00, 0xFFFFFFFF,
    0x00000000, 0x000000FF, 0x0000FF00, 0x0000FFFF, 0x00FF0000, 0x00FF00FF,
    0x00FFFF00, 0x00FFFFFF, 0xFF000000, 0xFF0000FF, 0xFF00FF00, 0xFF00FFFF,
    0xFFFF0000, 0xFFFF00FF, 0xFFFFFF00, 0xFFFFFFFF,
    0x00000000, 0x000000FF, 0x0000FF00, 0x0000FFFF, 0x00FF0000, 0x00FF00FF,
    0x00FFFF00, 0x00FFFFFF, 0xFF000000, 0xFF0000FF, 0xFF00FF00, 0xFF00FFFF,
    0xFFFF0000, 0xFFFF00FF, 0xFFFFFF00, 0xFFFFFFFF,
};

unsigned long v256_upper[256] = {
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    0x0000FF00, 0x0000FF00, 0x0000FF00, 0x0000FF00,
    0x0000FF00, 0x0000FF00, 0x0000FF00, 0x0000FF00,
    0x0000FF00, 0x0000FF00, 0x0000FF00, 0x0000FF00,
    0x0000FF00, 0x0000FF00, 0x0000FF00, 0x0000FF00,
    0x0000FFFF, 0x0000FFFF, 0x0000FFFF, 0x0000FFFF,
    0x0000FFFF, 0x0000FFFF, 0x0000FFFF, 0x0000FFFF,
    0x0000FFFF, 0x0000FFFF, 0x0000FFFF, 0x0000FFFF,
    0x0000FFFF, 0x0000FFFF, 0x0000FFFF, 0x0000FFFF,
    0x00FF0000, 0x00FF0000, 0x00FF0000, 0x00FF0000,
    0x00FF0000, 0x00FF0000, 0x00FF0000, 0x00FF0000,
    0x00FF0000, 0x00FF0000, 0x00FF0000, 0x00FF0000,
    0x00FF0000, 0x00FF0000, 0x00FF0000, 0x00FF0000,
    0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF,
    0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF,
    0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF,
    0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF,
    0x00FFFF00, 0x00FFFF00, 0x00FFFF00, 0x00FFFF00,
    0x00FFFF00, 0x00FFFF00, 0x00FFFF00, 0x00FFFF00,
    0x00FFFF00, 0x00FFFF00, 0x00FFFF00, 0x00FFFF00,
    0x00FFFF00, 0x00FFFF00, 0x00FFFF00, 0x00FFFF00,
    0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF,
    0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF,
    0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF,
    0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF,

    0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFF0000FF, 0xFF0000FF, 0xFF0000FF, 0xFF0000FF,
    0xFF0000FF, 0xFF0000FF, 0xFF0000FF, 0xFF0000FF,
    0xFF0000FF, 0xFF0000FF, 0xFF0000FF, 0xFF0000FF,
    0xFF0000FF, 0xFF0000FF, 0xFF0000FF, 0xFF0000FF,
    0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00,
    0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00,
    0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00,
    0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00,
    0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF,
    0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF,
    0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF,
    0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF,
    0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000,
    0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000,
    0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000,
    0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000,
    0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF,
    0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF,
    0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF,
    0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF,
    0xFFFFFF00, 0xFFFFFF00, 0xFFFFFF00, 0xFFFFFF00,
    0xFFFFFF00, 0xFFFFFF00, 0xFFFFFF00, 0xFFFFFF00,
    0xFFFFFF00, 0xFFFFFF00, 0xFFFFFF00, 0xFFFFFF00,
    0xFFFFFF00, 0xFFFFFF00, 0xFFFFFF00, 0xFFFFFF00,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
};



/*
 *    v256_ms_stplblt(src, sx, sy, dx, dy, w, h, plane, opaque) 
 *                -- Stipple the screen using the bitmap in src.
 *
 *    Input:
 *        SIbitmapP    src    -- pointer to source data        
 *        int        sx    -- X position (in pixels) of source
 *        int        sy    -- Y position (in pixels) of source
 *        int        dx    -- X position (in pixels) of destination
 *        int        dy    -- Y position (in pixels) of destination
 *        int        w    -- Width (in pixels) of area to move
 *        int        h    -- Height (in pixels) of area to move
 *        int        plane    -- which source plane
 *        int        opaque    -- Opaque/regular stipple (if non-zero)
 *
 *	calls to cfb8check(opaque)stipple have been included in case 
 *	the forcetype was set for stipple type and later restore to 
 *	original values correnponding to the previous graphics state.
 */
/* ARGSUSED */
SIBool
v256_ms_stplblt(src, sx, sy, dx, dy, w, h, plane, opaque)
SIbitmapP     src;
int        sx, sy, dx, dy, w, h, plane, opaque;
{
	extern void v256_stplfill();
	extern void v256_slstpl();

    register int srcinc, pdst, cnt;
    register BYTE *psrc, mask1, mask2;

	/*
	 * save the forcetype since the opaque variable is reused
	 */
    int            forcetype;

#ifdef V256STPL_DEBUG
printf("v256_ms_stplblt (%d) ,",opaque);
#endif

    DBENTRY("v256_ms_stplblt()");
    if ((w <= 0) || (h <= 0))
    {
        return(SI_SUCCEED);
    }

    if (src->Bdepth != 1)
    {
        return(SI_FAIL);
    }

	/*
	 * save forcetype
	 */
    forcetype = opaque;

    if (opaque == SGOPQStipple || 
        (opaque == 0 && v256_gs->stp_mode == SGOPQStipple))
    {
        opaque = 1;
    }
    else
    {
        opaque = 0;
    }
    
    if (v256_invertsrc)
    {
        v256_pat_fg = v256_gs->fg ^ v256_gs->pmask;
        v256_pat_bg = v256_gs->bg ^ v256_gs->pmask;
    }
    else
    {
        v256_pat_fg = v256_gs->fg;
        v256_pat_bg = v256_gs->bg;
    }

    switch (v256_gs->mode)
    {
    case GXset:
    case GXinvert:
        v256_pat_fg = v256_pat_bg = v256_gs->pmask;
        break;
    case GXclear:
        v256_pat_fg = v256_pat_bg = 0;
        break;
    }

    srcinc = ((src->Bwidth + 31) & ~31) >> 3;
    psrc = ((BYTE *)src->Bptr) + (srcinc * sy) + (sx >> 3);
    cnt = (w + (sx & 7) + 7) >> 3;
    pdst = dx + v256_slbytes * dy - (sx & 7);
    mask1 = v256_startmask[sx & 7];
    mask2 = v256_endmask[(sx + w - 1) & 7];

    /*
     * if we've got a startmask and we're writing to position 0, we
     * have to do it a pixel at a time to avoid writing outside our
     * video memory window.
     */
    if (mask1 && (pdst <= 0))
    {
        for (;h--; psrc+=srcinc, pdst+=v256_slbytes)
        {
            v256_slstpl(psrc, pdst, cnt, mask1, mask2, opaque);
        }
    }
    else
    {
        v256_blocksize = srcinc;
        /* 
         * check if stipple mode setting in graphics state has to 
         * be overridden if so setup new stipple arrays
         */
        if ( forcetype )
        {
            if(opaque)
            {
#ifdef V256STPL_DEBUG
    printf("check opaque");
#endif
                /*
                 * opaque stippling
                 */
                cfb8CheckOpaqueStipple( v256_gs->mode,
                                        v256_gs->fg,
                                        v256_gs->bg,
                                        v256_gs->pmask);
            }
            else 
            {
#ifdef V256STPL_DEBUG
    printf("check transparent");
#endif
                /*
                 * transparent stippling
                 */
                cfb8CheckStipple (  v256_gs->mode,
                                    v256_gs->fg,
                                    v256_gs->pmask);

            }
        }

		/* 
		 * call the stippler
		 */
        v256_stplfill(psrc, pdst, cnt, mask1, mask2, h, opaque);

        /*
         * restore stipple arrays to correspond to graphics state if 
         * required
         */
        if(forcetype)
        {
            if(v256_gs->stp_mode == SGOPQStipple)
            {
#ifdef V256STPL_DEBUG
printf("restore opaque");
#endif
                /*
                 * opaque stippling
                 */
                cfb8CheckOpaqueStipple( v256_gs->mode,
                                        v256_gs->fg,
                                        v256_gs->bg,
                                        v256_gs->pmask);
            }
            else 
            {
#ifdef V256STPL_DEBUG
printf("restore transparent");
#endif
                /*
                 * transparent stippling
                 */
                cfb8CheckStipple (  v256_gs->mode,
                                    v256_gs->fg,
                                    v256_gs->pmask);
            }
        }
    }
#ifdef V256STPL_DEBUG
printf("\n");
#endif

    return(SI_SUCCEED);
}



/*
 *    v256_hline_stpl(pt1, pt2, ycnt)    -- draw ycnt horizontal lines from 
 *                    pt1 to pt2 using the current fill
 *                    style.  For ycnt > 1, draw each 
 *                    successive line one scanline down from
 *                    the preceeding line.
 *
 *    Input:
 *        DDXPointRec    pt1    -- first point in line
 *        DDXPointRec    pt2    -- last point in line
 *        int        ycnt    -- number of lines to draw
 */
void
v256_hline_stpl(pt1, pt2, ycnt)
DDXPointRec    pt1, pt2;
int        ycnt;
{
	extern void v256_stplfill();
    int opaque, xcnt, xoffset, i, scroff, scrend, length, pat_y;
    register BITS16 *pattern, *pdst;
    BYTE startmask, endmask;

    opaque = (v256_gs->stp_mode == SGOPQStipple);
    scrend = pt2.x + v256_slbytes * (pt1.y + ycnt - 1);
    xcnt = (pt2.x >> 3) - (pt1.x >> 3) + 1;
    xoffset = (pt1.x & 0x0f) >> 3;
    startmask = v256_startmask[pt1.x & 0x7];
    endmask = v256_endmask[pt2.x & 0x7];

    /*
     * Loop through the lines in the pattern.  For each line, expand
     * the pattern into a temporary buffer big enough to stipple the
     * width needed, then stpl that buffer onto each scanline on the
     * screen that would receive that part of the pattern.
     */
    pat_y = pt1.y % v256_cur_pat_h;
    pattern = (BITS16 *)v256_cur_pat + pat_y;
    for (i = 0; i < v256_cur_pat_h; i++) {
        scroff = pt1.x + v256_slbytes * pt1.y;
        if (scroff > scrend)
            return;

        length = xcnt + xoffset;
        pdst = (BITS16 *)v256_slbuf;
        while (length > 0) {
            *pdst++ = *pattern;
            length -= 2;
        }

        while (scroff <= scrend) {
            v256_stplfill(v256_slbuf+xoffset, scroff & ~0x7, xcnt, 
                     startmask, endmask, 1, opaque);
            scroff += v256_slbytes * v256_cur_pat_h;
        }

        pt1.y++;
        pattern++;
        if (++pat_y >= v256_cur_pat_h) {
            pat_y = 0;
            pattern = (BITS16 *)v256_cur_pat;
        }
    }
}


/* 
 *    v256_slstpl(psrc, pdst, dstcnt, startmask, endmask, opaque) 
 *                    -- Stipple the screen at pdst with
 *                    the bitmap in psrc.
 *
 *    Input:
 *        BYTE    *psrc        -- pointer to bitmap
 *        int    pdst        -- offset to screen memory area
 *        int    dstcnt        -- number of bytes to copy
 *        BYTE    startmask    -- mask for first byte
 *        BYTE    endmask        -- mask for last byte
 *        int    opaque        -- 0=transparent, 1=background(opaque)
 */
void
v256_slstpl(psrc, dst, dstcnt, startmask, endmask, opaque)
BYTE        *psrc;
int        dst;
int        dstcnt;
unsigned int    startmask, endmask;
register int    opaque;
{
    register BYTE    *pdst;
    register unsigned int counter, mask;
    extern void v256_pixel_op();

    if (--dstcnt <= 0)            /* do only one byte */
        endmask = endmask & startmask;
    else {
        mask = *psrc++;
        while (startmask && (startmask & 0x01) == 0) {
            startmask >>= 1;
            mask >>= 1;
            dst++;
        }
        selectpage(dst);
        pdst = v256_fb + (dst & VIDEO_PAGE_MASK);

        if (opaque) {
            while (startmask) {        /* first byte */
                v256_pixel_op(pdst++, (mask & 0x01)? 
                          v256_pat_fg: v256_pat_bg);
                if (++dst > v256_endpage) {
                    selectpage(dst);
                    pdst -= 0x10000;
                }
                startmask >>= 1;
                mask >>= 1;
            }
        } else {
            while (startmask) {        /* first byte */
                if (mask & 0x01)
                    v256_pixel_op(pdst, v256_pat_fg);
                pdst++;
                if (++dst > v256_endpage) {
                    selectpage(dst);
                    pdst -= 0x10000;
                }
                startmask >>= 1;
                mask >>= 1;
            }
        }
    }

    if (opaque) {
        while (--dstcnt > 0) {            /* middle bytes */
            counter = 0xFF;
            mask = *psrc++;
            while (counter) {
                v256_pixel_op(pdst++, (mask & 0x01)? 
                          v256_pat_fg: v256_pat_bg);
                if (++dst > v256_endpage) {
                    selectpage(dst);
                    pdst -= 0x10000;
                }
                mask >>= 1;
                counter >>= 1;
            }
        }
    } else {
        while (--dstcnt > 0) {        /* middle bytes */
            counter = 0xFF;
            mask = *psrc++;
            while (counter) {
                if (mask & 0x01)
                    v256_pixel_op(pdst, v256_pat_fg);
                pdst++;
                if (++dst > v256_endpage) {
                    selectpage(dst);
                    pdst -= 0x10000;
                }
                mask >>= 1;
                counter >>= 1;
            }
        }
    }

    mask = *psrc;
    while (endmask && (endmask & 0x01) == 0) {
        endmask >>= 1;
        mask >>= 1;
        dst++;
    }
    selectpage(dst);
    pdst = v256_fb + (dst & VIDEO_PAGE_MASK);

    if (opaque) {
        while (endmask) {                /* last byte */
            v256_pixel_op(pdst++, (mask & 0x01)? v256_pat_fg: v256_pat_bg);
            if (++dst > v256_endpage) { 
                selectpage(dst); 
                pdst -= 0x10000; 
            }
            mask >>= 1;
            endmask >>= 1;
        }
    } else {
        while (endmask) {                /* last byte */
            if (mask & 0x01)
                v256_pixel_op(pdst, v256_pat_fg);
            pdst++;
            if (++dst > v256_endpage) { 
                selectpage(dst); 
                pdst -= 0x10000; 
            }
            mask >>= 1;
            endmask >>= 1;
        }
    }
}



/* 
 *    v256_stplfill(psrc, pdst, dstcnt, startmask, endmask, lines, opaque) 
 *                    -- Stipple the screen at pdst with
 *                    the bitmap in psrc.
 *
 *    Input:
 *        BYTE    *psrc        -- pointer to bitmap
 *        int    pdst        -- offset to screen memory area
 *        int    dstcnt        -- number of bytes to copy
 *        BYTE    startmask    -- mask for first byte
 *        BYTE    endmask        -- mask for last byte
 *        ine    lines        -- number of lines to draw
 *        int    opaque        -- 0=transparent, 1=background(opaque)
 */
void
v256_stplfill(psrc, dst, dstcnt, startmask, endmask, lines, opaque)
BYTE        *psrc;
int        dst;
int        dstcnt;
unsigned int    startmask, endmask;
int        lines;
register int    opaque;
{
    register BYTE    *pdst;
    extern void v256_transparent_stipple();
    extern void v256_opaque_stipple();
	extern void v256_slstpl();

    if (lines <= 0)
	{
        return;
	}

    selectpage(dst);

    pdst = v256_fb + (dst & VIDEO_PAGE_MASK);

    while (lines)
	{
        while (dst + (dstcnt << 3) - 1 <= v256_endpage)
		{
            if (opaque)
			{
                v256_opaque_stipple(psrc, pdst, dstcnt, 
                            startmask, endmask);
			}
            else
			{
                v256_transparent_stipple(psrc, pdst, dstcnt, 
                             startmask, endmask);
			}
            if (--lines <= 0)
			{
                return;
			}
            pdst += v256_slbytes;
            dst += v256_slbytes;
            psrc += v256_blocksize;
            if (dst > v256_endpage) 
			{
                selectpage(dst);
                pdst -= VGA_PAGE_SIZE;
            }
        }

        v256_slstpl(psrc, dst, dstcnt, startmask, endmask, opaque);

        selectpage(dst);        /* reset page pointer */
        lines--;
        pdst += v256_slbytes;
        dst += v256_slbytes;
        psrc += v256_blocksize;
        if (dst > v256_endpage)
		{
            selectpage(dst);
            pdst -= VGA_PAGE_SIZE;
        }
    }
}



void
v256_pixel_op(dst, val)
unsigned char *dst;
int val;
{
    if (v256_gs->mode == GXnoop)
        return;

    switch (v256_function) {
    case V256_COPY:
        *dst = val | (*dst & ~v256_gs->pmask);
        return;
    case V256_XOR:
        *dst ^= val;
        return;
    case V256_AND:
        *dst &= val | ~v256_gs->pmask;
        return;
    case V256_OR:
        *dst |= val;
        return;
    case V256_AND_INVERT:
        *dst = (*dst & (val | ~v256_gs->pmask)) ^ v256_gs->pmask;
        return;
    case V256_OR_INVERT:
        *dst = (*dst | val) ^ v256_gs->pmask;
        return;
    }
}



/*
 *    v256_pat_setup(src, dst, w, h, x, y)    -- Given a bitmap in src, 
 *                        pad it to exactly 16 bits
 *                        of width, align it at 0, 0
 *                        and store it in dst.  
 *
 *    Input:
 *        BYTE    *src    -- source bitmap
 *        BYTE    *dst    -- destination
 *        int    w    -- width of source bitmap
 *        int    h    -- height of source bitmap
 *        int    x    -- x origin (in bits) of source bitmap
 *        int    y    -- y origin (in bits) of source bitmap
 *
 *    Returns:
 *        0 for normal patterns
 *        -1 if the pattern has a bad width
 */
int
v256_pat_setup(src, dst, w, h, x, y)
BYTE    *src, *dst;
int    w, h, x, y;
{
    BITS16 *pat_word, *dst_word;
    BITS32 *pat_long;
    int    i, hcnt;
    int    shift;

    /*
     *  Repeat the pattern (if it's width is 1, 2, 4, or 8 bits) to fill
     *  a 32-bit word.
     */    
    pat_word = (BITS16 *)src;
    for (i = 0; i < h; i++) {
        switch (w) {
        case 16:
            break;
        case 8:
            *pat_word &= 0xff;
            *pat_word |= *pat_word << 8;
            break;

        case 1:
            *pat_word &= 0x1;
            *pat_word |= *pat_word << 1;
				/* FALLTHROUGH */
        case 2:
            *pat_word &= 0x3;
            *pat_word |= *pat_word << 2;
				/* FALLTHROUGH */
        case 4:
            *pat_word &= 0xf;
            *pat_word |= *pat_word << 4;
            *pat_word |= *pat_word << 8;
            break;
        default:
            return (-1);        /* bad pattern */
        }
        *(pat_word+1) = *pat_word;
        pat_word += 2;
    }

    /*
     * Shift the pattern as needed and store the result in the dst.
     * Note that we also may need to start in the middle if the
     * pattern is mis-aligned on the y axis.
     */
    y %= h;
    hcnt = (y? (h - y) : 0);
    pat_long = ((BITS32 *)src) + hcnt;
    dst_word = (BITS16 *)dst;
    shift = 16 - (x & 0x0F);
    for (i = 0; i < h; i++, hcnt++) {
        if (hcnt == h) {
            hcnt = 0;
            pat_long = (BITS32 *)src;
        }
        *dst_word++ = (BITS16)(*pat_long++ >> shift);
    }
    return(0);
}


/*
 *	Transparent stippler. does transparent stippling for all cases
 *	of GXfunction in SUN cfb style using the reduced rop value and the
 *	stipple arrays that have been setup.
 */	
void v256_transparent_stipple(src, dst, count, startmask, endmask)
unsigned char *src;
register unsigned long *dst;
int count;
unsigned int startmask, endmask;
{
    register unsigned long mask;
    int val;
    extern void v256_fast_xparent_stpl();

    if (v256_gs->mode == GXnoop)
        return;

	/*
	 * special case copy from the remaining stuff , and do writing avoiding
	 * memory read -- vga reads are very slow
	 */

    switch(cfb8StippleRRop) {
    case GXcopy:
            if (--count <= 0) 
            {
                endmask &= startmask;
            } 
            else 
            {
				/*
				 * startmask present ?
				 */
                val = *src++ & startmask;
                mask = (unsigned long)val;
                WriteFourBits(dst,V256Xor,(mask&0x0F))
                dst++;
                WriteFourBits(dst,V256Xor,(mask>>4))
                dst++;
            }
            if (--count > 0) 
            {
				/*
				 * middle bytes of source
				 */
                v256_fast_xparent_stpl(dst, src, count);
                dst += count + count;
                src += count;
            }

			/*
			 * endmask always present
			 */
            val = *src++ & endmask;
            mask = (unsigned long)val;
            WriteFourBits(dst,V256Xor,(mask&0x0F))
            dst++;
            WriteFourBits(dst,V256Xor,(mask>>4))
            dst++;

            break;

	 /*
	  * remaining cases of GXfunction
	  */
     default:
        if (--count <= 0) {
            endmask &= startmask;
        } else {
			/*
			 * startmask
			 */
            val = *src++ & startmask;
            mask = (unsigned long)val;
            RRopFourBits(dst,(val&0x0F));
            dst++;
            RRopFourBits(dst,(val>>4));
            dst++;
        }
		/* 
		 * middlebytes of source
		 */
        while (--count > 0) {
            val = *src++;
            mask = (unsigned long)val;
            RRopFourBits(dst,(val&0x0F));
            dst++;
            RRopFourBits(dst,(val>>4));
            dst++;
        }

		/*
		 * endmask
		 */
        val = *src++ & endmask;
        mask = (unsigned long)val;
        RRopFourBits(dst,(val&0x0F));
        dst++;
        RRopFourBits(dst,(val>>4));
        dst++;
        break;
    }
}

/*
 *	Opaque stippler. does opaque stippling for all cases
 *	of GXfunction in SUN cfb style using the reduced rop value and the
 *	stipple arrays that have been setup.
 */	
void v256_opaque_stipple(src, dst, count, startmask, endmask)
unsigned char *src;
long *dst;
int count;
unsigned int startmask, endmask;
{
    register int val;
	register unsigned long mask;
	extern void v256_fast_opaque_stpl();

    if (v256_gs->mode == GXnoop)
	{
        return;
	}

	/*
	 * special case GXcopy. 
	 */

    switch(cfb8StippleRRop) 
    {
        case GXcopy:
            if (--count <= 0) 
            {
                endmask &= startmask;
            } 
            else 
            {
				/*
				 * startmask present
				 */
                val = *src++ & startmask;
                mask = (unsigned long)val;
                *dst = *dst & ~v256_lower[startmask] |
                    GetFourPixels ((mask & 0x0F)) & v256_lower[startmask];
                dst++;
                *dst = *dst & ~v256_upper[startmask] |
                    GetFourPixels ((mask >> 4)) & v256_upper[startmask];
                dst++;

            }
			/* 
			 * middle bytes of source
			 */
            if (--count > 0) 
            {
                v256_fast_opaque_stpl(dst, src, count);
                dst += count + count;
                src += count;
            }

			/*
			 * endmask
			 */
            val = *src++ & endmask;
            mask = (unsigned long)val;
            *dst = *dst & ~v256_lower[endmask] |
                GetFourPixels ((mask & 0x0F)) & v256_lower[endmask];
            dst++;
            *dst = *dst & ~v256_upper[endmask] |
                GetFourPixels ((mask >> 4)) & v256_upper[endmask];
            dst++;
            break;

		/*
		 * other cases of GXfunction
		 */
        default :
            if (--count <= 0) 
            {
                endmask &= startmask;
            } 
            else 
            {
				/*
				 * startmask
				 */
                val = *src++ & startmask;
                mask = (unsigned long)val;
                *dst = 
                    MaskRRopPixels(*dst,(mask & 0x0F),v256_lower[startmask]);
                dst++;
                *dst = 
                    MaskRRopPixels(*dst,(mask >> 4),v256_upper[startmask]);
                dst++;
            }
			/*
			 * middle bytes of source
			 */
            while (--count > 0) 
            {
                val = *src++;
                RRopFourBits (dst , (mask & 0x0F));
                dst++;
                RRopFourBits (dst , (mask >> 4));
                dst++;
            }

			/*
			 * endmask
			 */
            val = *src++ & endmask;
            mask = (unsigned long)val;
            *dst = 
                MaskRRopPixels(*dst,(mask & 0x0F),v256_lower[endmask]);
            dst++;
            *dst = 
                MaskRRopPixels(*dst,(mask >> 4),v256_upper[endmask]);
            dst++;
            break;
    }
}

/* 
 *	routine to do transparent stippling of the middle bytes of a source
 *	stipple for the case of the GXcopy case
 * 	assumes that selectpage has been called
 *	avoids vga reads
 * 	stippling is done in the SUN cfb style
 */
void v256_fast_xparent_stpl(
register    unsigned long     *dst,
            unsigned char    *src,
            int                count
)
{
    unsigned long    k;
    int                i;

    for(i=0; count>0;count--,i++)
    {
        k = (unsigned long)src[i];
        WriteFourBits(dst,V256Xor,(k&0x0F))
        dst++;
        WriteFourBits(dst,V256Xor,(k>>4))
        dst++;
    }
}

/* 
 *	routine to do opaque stippling of the middle bytes of a source
 *	stipple for the case of the GXcopy case
 * 	assumes that selectpage has been called
 * 	stippling is done in the SUN cfb style
 */
void
v256_fast_opaque_stpl(
register unsigned long     *dst, 
BYTE             *src, 
int                count
)
{
    unsigned long    tmp;
    int                i;

    for (i = 0; count > 0; count--,i++)
    {
        tmp = (unsigned long)src[i];
        *dst = GetFourPixels((tmp & 0x0F));
        dst++;
        *dst = GetFourPixels((tmp >> 4));
        dst++;
    }
}
