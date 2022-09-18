/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/v256gs.c	1.2"

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
#include "sys/inline.h"
#include "vtio.h"
#include "v256.h"

static SIint32 psstate[] = {        /* Used in downloadstate */
    SetSGpmask,
    SetSGmode,
    SetSGstplmode,
    SetSGfillmode,
    SetSGfg,
    SetSGbg,
    SetSGtile,
    SetSGstipple,
    SetSGfillrule,
    0
};

static SIint32 pgstate[] = {        /* used in get state */
    GetSGpmask,
    GetSGmode,
    GetSGstplmode,
    GetSGfillmode,
    GetSGfg,
    GetSGbg,
    GetSGfillrule,
    0
};

/*
 *	The reduced rop computation
 */
extern	int	cfbReduceRasterOp (int	rop, unsigned long   fg, 
			unsigned long pm, unsigned long   *andp,
			unsigned long *xorp);
/*
 * VATS : declare some globals for cfb style of functioning. 
 * we need three globals to be set at gc download time in case
 * of stipple change, for the reduced rop operation the and and xor
 * values
 */
 int		V256Rrop;
 unsigned long  V256And, V256Xor;

/*
 *    v256_download_state(indx, flag, state)    -- set the graphics state
 *                        specified by indx to *state.
 *
 *    Input:
 *        int        indx    -- index into graphics states
 *        int        flag    -- mask of state elements to change
 *        SIGStateP    statep    -- pointer to new state structure
 */
SIBool
v256_download_state(indx, flag, statep)
int        indx;
int        flag;
SIGStateP    statep;
{
    SIbitmapP bmap;
    int i, w, h;
    v256_state *gs;
    int size;

    DBENTRY1("v256_download_state()");

    gs = &v256_gstates[indx];
    for (i = 0; psstate[i]; i++) {
        switch (flag & psstate[i]) {
        case SetSGpmask:
            gs->pmask = statep->SGpmask & (V256_MAXCOLOR-1);
            break;
        case SetSGmode:
            gs->mode = statep->SGmode;
            break;
        case SetSGstplmode:
            gs->stp_mode = statep->SGstplmode;
            break;
        case SetSGfillmode:
            gs->fill_mode = statep->SGfillmode;
            break;
        case SetSGfillrule:
            gs->fill_rule = statep->SGfillrule;
            break;
        case SetSGfg:
            gs->fg = statep->SGfg & gs->pmask;
            break;
        case SetSGbg:
            gs->bg = statep->SGbg & gs->pmask;
            break;
        case SetSGstipple:
            bmap = statep->SGstipple;
            gs->raw_stipple = *bmap;
            gs->raw_stipple.Bptr = NULL;
            h = gs->raw_stipple.Bheight;
            w = gs->raw_stipple.Bwidth;
            gs->stpl_valid = 0;
            if ((h > V256_PAT_H) || (w > V256_PAT_W) ||
                (w & (w-1))) {
                size = h * (((w + 31) & ~31) >> 3);
                if (gs->big_stpl)
                    free(gs->big_stpl);
                gs->big_stpl = (BYTE *)malloc(size);
                if (gs->big_stpl != NULL)
                    bcopy(bmap->Bptr, gs->big_stpl, size);
            }
            else {
                if (gs->big_stpl)
                    free(gs->big_stpl);
                gs->big_stpl = NULL;
                bcopy(statep->SGstipple->Bptr,gs->raw_stpl_data,
                      h*4);
            }
            break;
        case SetSGtile:
            bmap = statep->SGtile;
            gs->raw_tile = *bmap;
            gs->tile_valid = 0;
            size = bmap->Bheight *
                   (((bmap->Bwidth * bmap->Bdepth)+31) & ~31) / 8;
            if (gs->raw_tile_data)
                free(gs->raw_tile_data);
            if ((gs->raw_tile_data = (BYTE *)malloc(size)) != NULL)
                bcopy(bmap->Bptr, gs->raw_tile_data, size);
            break;
        default:
            break;
        }
    }

    return(SI_SUCCEED);

}



/*
 *    v256_get_state(indx, flag, state)    -- get the graphics state
 *                        specified by indx to *state.
 *
 *    Input:
 *        int        indx    -- index into graphics states
 *        int        flag    -- mask of state elements to change
 *        SIGStateP    statep    -- pointer to new state structure
 */
SIBool
v256_get_state(indx, flag, statep)
int        indx;
int        flag;
SIGStateP    statep;
{
    int i;
    v256_state *gs;

    DBENTRY1("v256_get_state()");

    gs = &v256_gstates[indx];
    for (i = 0; pgstate[i]; i++) {
        switch (flag & pgstate[i]) {
        case GetSGpmask:
            statep->SGpmask = gs->pmask & (V256_MAXCOLOR-1);
            break;
        case GetSGmode:
            statep->SGmode = gs->mode;
            break;
        case GetSGstplmode:
            statep->SGstplmode = gs->stp_mode;
            break;
        case GetSGfillmode:
            statep->SGfillmode = gs->fill_mode;
            break;
        case GetSGfillrule:
            statep->SGfillrule = gs->fill_rule;
            break;
        case GetSGfg:
            statep->SGfg = gs->fg;
            break;
        case GetSGbg:
            statep->SGbg = gs->bg;
            break;
        default:
            break;
        }
    }
    return(SI_SUCCEED);
}



/*
 *    v256_select_state(indx, flag, state)    -- set the current state
 *                        to that specified by indx.
 *
 *    Input:
 *        int        indx    -- index into graphics states
 */
SIBool
v256_select_state(indx)
int indx;
{
    DBENTRY1("v256_select_state()");

    v256_gs = &v256_gstates[indx];
    v256_cur_state = indx;

    /* 
     * now set up all the internal data structures
     * to be used with the V256 adapter based on what is in the GS.  
     */
    switch (v256_gs->fill_mode)
    {
    case SGFillSolidFG:
        v256_src = v256_gs->fg;
        break;
    case SGFillSolidBG:
        v256_src = v256_gs->bg;
        break;
    case SGFillStipple:
        v256_cur_pat = v256_gs->stpl;
        v256_cur_pat_h = v256_gs->stpl_h;
        break;
    case SGFillTile:
        break;
    }

    v256_invertsrc = SI_FALSE;
    v256_function = V256_COPY;

    switch (v256_gs->mode)
    { 
	/*
	 * set up source and dest 
	 */
        case GXclear:
            v256_src = 0;
            break;

        case GXset:
            v256_src = v256_gs->pmask;
            break;

        case GXandInverted:
            v256_src = ~v256_src & v256_gs->pmask;
            v256_invertsrc = SI_TRUE;
            /*FALLTHROUGH*/
        case GXand:
            v256_function = V256_AND;
            break;

        case GXorInverted:
            v256_src = ~v256_src & v256_gs->pmask;
            v256_invertsrc = SI_TRUE;
            /*FALLTHROUGH*/
        case GXor:
            v256_function = V256_OR;
            break;

        case GXequiv:
            v256_src = ~v256_src & v256_gs->pmask;
            v256_invertsrc = SI_TRUE;
            /*FALLTHROUGH*/
        case GXxor:
            v256_function = V256_XOR;
            break;

        case GXandReverse:
            v256_src = ~v256_src & v256_gs->pmask;
            v256_invertsrc = SI_TRUE;
            /*FALLTHROUGH*/
        case GXnor:
            v256_function = V256_OR_INVERT;
            break;

        case GXorReverse:
            v256_src = ~v256_src & v256_gs->pmask;
            v256_invertsrc = SI_TRUE;
            /*FALLTHROUGH*/
        case GXnand:
            v256_function = V256_AND_INVERT;
            break;

        case GXcopyInverted:
            v256_src = ~v256_src & v256_gs->pmask;
            v256_invertsrc = SI_TRUE;
            break;

        case GXinvert:
            v256_src = v256_gs->pmask;
            v256_function = V256_XOR;
            break;
    }

    /*
     * VATS : changes for cfb style stippling. 
     * update the V256Rrop,V256And and V256Xor values
     */  
    V256Rrop = cfbReduceRasterOp (    v256_gs->mode, 
                                    v256_gs->fg,
                                    v256_gs->pmask,
                                    &V256And, 
                                    &V256Xor);

    return(SI_SUCCEED);
}



/*
 *    v256_clip(x1, y1, x2, y2)    -- set out clipping rectangle to
 *                    the coordinates specified.
 *
 *    Input:
 *        int    x1, y1        -- upper left corner of rectangle
 *        int    x2, y2        -- bottom right corner of rectangle
 */
SIBool
v256_clip(x1, y1, x2, y2)
int x1, y1, x2, y2;
{
    DBENTRY1("v256_clip()");

    v256_clip_x1 = x1;
    v256_clip_y1 = y1;
    v256_clip_x2 = x2;
    v256_clip_y2 = y2;
    return(SI_SUCCEED);
}
