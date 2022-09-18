/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/v256fill.c	1.2"

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

#ifdef    V256FILL_DEBUG
#define NDEBUG     1
#include <assert.h>
#endif    /* V256FILL_DEBUG */



BYTE v256_pat_fg;
BYTE v256_pat_bg;
extern    SIBool    v256_tile_setup();
extern    SIBool    v256_big_stpl_setup();

extern    unsigned long v256_expand[];

/*
 *     Function prototypes
 */
void
v256_do_fast_hline_fill( DDXPointRec pt1, DDXPointRec pt2, int h);

/*
 * If we're tiling but have a rasterop of GXset or GXclear, we don't really
 * tile, we just draw with 0 or 1.  We do this by temporarily setting things
 * up as if it's doing a solid fill.  This variable is used to restore
 * the real setting.
 */
static  int  saved_fill_mode;


/*
 * Tile and stipple patterns get set up in a global array that is already
 * alligned as needed based on the pattern's origin.  The pattern is set up
 * as if it's origin were 0, 0 (the upper left corner of the screen.
 * This way it's simple for us to pattern a region because we don't have to
 * do any allignment calculations at fill time.   We also build up the
 * pattern to be exactly 16 bits wide, which makes the code here simpler.
 * The pattern is set up at GS download time.
 *
 * Patterns are 16 bits wide.
 */



/*
 *    v256_fill_rect(cnt, prect)    -- draw a series of filled rectangles.
 *                The current fill style, foreground and 
 *                background colors, and current ROP are used.
 *
 *    Input:
 *        int    cnt        -- number of rectangles to fill
 *        SIRectP    prect        -- pointer to list of rectangles
 */
SIBool
v256_fill_rect(cnt, prect)
register int         cnt;
register SIRectP    prect;
{
	extern int v256_start_fill();
	extern void v256_finish_fill();
	extern int v256_big_stpl_rect();
	extern int v256_fill_lines();

    DBENTRY("vga_fill_rect()");
    /*
     *     Set up stipple
     */
    if (v256_start_fill() == SI_FAIL)
    {
        printf("v256_fill_rect: start_fill failed\n");
        return(SI_FAIL);
    }

    if ((v256_gs->fill_mode == SGFillStipple) && (v256_gs->big_stpl))
    {
        /*
         *    special case the big stipples
         */
        while (--cnt >= 0)
        {
            v256_big_stpl_rect(prect++);
        }
    }
    else 
    {
        while (--cnt >= 0) 
        {
            /*
             *    Valid region -> call the fill lines function
             */
            if ((prect->lr.x > prect->ul.x) &&
                (prect->lr.y > prect->ul.y))
            {
                v256_fill_lines(prect->ul.x, prect->lr.x-1,
                        prect->ul.y,
                        prect->lr.y - prect->ul.y);
            }
            prect++;
        }
    }

    /*
     * Wind up
     */
    v256_finish_fill();

    return(SI_SUCCEED);
}



/*
 *    v256_fill_spans(count, pts, widths)    -- fill a series of scanlines.
 *                The current fill style, foreground and 
 *                background colors, and current ROP are used.
 *
 *    Input:
 *        int        count        -- number of scanlines to fill
 *        SIPointP     pts        -- list of start points.
 *        int        *widths        -- list of widths to fill
 */
SIBool
v256_fill_spans(count, pts, widths)
register int    count;
register SIPointP pts;
register int    *widths;
{
    DDXPointRec pt1, pt2;
    extern void v256_hline_stpl( DDXPointRec pt1, 
								 DDXPointRec pt2, 
								 int        ycnt);
	extern void v256_big_hline_stpl();
	extern int 	v256_start_fill();
	extern void v256_finish_fill();
	extern void 	v256_line_horiz();
	extern void 	v256_hline_tile();


    DBENTRY("vga_fill_spans()");

    if (v256_gs->mode == GXnoop)
    {
        return(SI_SUCCEED);
    }

    if (v256_start_fill() == SI_FAIL)
    {
        return(SI_FAIL);
    }

    while (count--) 
    {
        if (!*widths) 
        {
            widths++;
            pts++;
            continue;
        }
        pt1.x = pts->x;
        pt2.x = pts->x + *widths++ - 1;
        pt1.y = pt2.y = pts->y;
        pts++;

        if ((pt1.x > v256_clip_x2) || (pt2.x < v256_clip_x1) ||
            (pt1.y > v256_clip_y2) || (pt1.y < v256_clip_y1))
        {
            continue;
        }

        if (pt1.x < v256_clip_x1) pt1.x = v256_clip_x1;
        if (pt2.x > v256_clip_x2) pt2.x = v256_clip_x2;

        switch(v256_gs->fill_mode) 
        {
        case SGFillSolidFG:
        case SGFillSolidBG:
            v256_line_horiz(pt1, pt2, 1);
            break;
        case SGFillStipple:
            if (v256_gs->big_stpl)
            {
                v256_big_hline_stpl(pt1.x, pt1.y,
                            pt2.x-pt1.x+1, 1);
            }
            else
            {
                v256_hline_stpl(pt1, pt2, 1);
            }
            break;
        case SGFillTile:
            v256_hline_tile(pt1, pt2, 1);
            break;
        }
    }

    v256_finish_fill();

    return(SI_SUCCEED);
}


/*
 *    v256_start_fill()    -- do all the setup before a fill operation
 *
 *        since this function is called before any fill operation, 
 *         setup the lookuparrays cfb8StippleAnd and cfb8StippleXor
 *        for stippling operations here.
 */
int
v256_start_fill()
{
	extern int v256_pat_setup();
    register int i, j;
    register BYTE *s, *p;
    SIbitmapP bmap;
    BYTE      *pat;
    int       height;

    saved_fill_mode = -1;

    /*
     * check if stipple mode  and call the appropriate function to setup
     * cfb8StippleAnd and cfb8StippleXor arrays
     */
    if (v256_gs->fill_mode == SGFillStipple) 
    {

        if (v256_gs->stp_mode == SGOPQStipple)
        {
            cfb8CheckOpaqueStipple(    v256_gs->mode, 
                                    v256_gs->fg, 
                                    v256_gs->bg, 
                                    v256_gs->pmask);
        }
        else if ( v256_gs->stp_mode == SGStipple )
        {
            cfb8CheckStipple (    v256_gs->mode,
                                v256_gs->fg, 
                                v256_gs->pmask);
        }
        else 
        {
            printf("unknown stipple mode\n");
            return (SI_FAIL);
        }

        if(!v256_gs->stpl_valid) 
        {
            if (v256_gs->big_stpl) 
            {
                if (!v256_gs->raw_stipple.Bptr) 
                {
                    if (v256_big_stpl_setup() == SI_FAIL)
                    {
                        return(SI_FAIL);
                    }
                    v256_gs->raw_stipple.Bptr = (SIArray)
                                v256_gs->big_stpl;
                }
                return(SI_TRUE);
            }
                
            bmap = &(v256_gs->raw_stipple);
            pat = v256_gs->stpl;
            height = bmap->Bheight;
            if (height > V256_PAT_H) 
            {
                v256_gs->stpl_h = -1;
                return(SI_FAIL);
            }
            v256_gs->stpl_h = height;
            if (v256_pat_setup(v256_gs->raw_stpl_data, pat, 
                       bmap->Bwidth, height, 
                       bmap->BorgX, bmap->BorgY) == -1)
            {
                v256_gs->stpl_h = -1;
            }
            v256_cur_pat = v256_gs->stpl;
            if ((v256_cur_pat_h = v256_gs->stpl_h) == -1)
            {
                return(SI_FAIL);
            }
            v256_gs->stpl_valid = 1;
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

    }
    else if (v256_gs->fill_mode == SGFillTile) 
    {
        if (!v256_gs->tile_valid) 
        {
            if (v256_tile_setup() == SI_FAIL)
            {
                return(SI_FAIL);
            }
            v256_gs->tile_valid = 1;
        }

        if ((v256_gs->mode == GXclear) || (v256_gs->mode == GXset) ||
            (v256_gs->mode == GXinvert)) 
        {
            saved_fill_mode = v256_gs->fill_mode;
            v256_gs->fill_mode = SGFillSolidFG;
        }

        /*
         * If we need to invert the tile, do it now, then invalidate
         * it when we're done filling.
         */
        if (v256_invertsrc) 
        {
            p = s = v256_gs->raw_tile_data;
            j = ((v256_gs->raw_tile.Bwidth + 3) & ~3) * 
                v256_gs->raw_tile.Bheight;
            for (i = 0; i < j; i++)
            {
                *p++ = ~(*s++);
            }
        }
    }

    return(SI_SUCCEED);
}



/*
 *    v256_finish_fill()    -- do the last little cleanup after having
 *                filled something.
 */
void
v256_finish_fill()
{
    /*
     * Restore the fill mode if we had to save it away previously
     */
    if (saved_fill_mode != -1)
    {
        v256_gs->fill_mode = saved_fill_mode;
    }

    /*
     * If we were tile filling and had to invert the tile, it's not
     * valid anymore.
     */
    if ((v256_gs->fill_mode == SGFillTile) && v256_invertsrc)
    {
        v256_gs->tile_valid = 0;
    }
}


/*
 *    v256_fill_lines(xl, xr, y, h)    -- fill in lines
 *
 *    Input:
 *        int        xl    -- left edge of line to fill
 *        int        xr    -- right edge of line to fill
 *        int        y    -- y position of line to fill
 *        int        h    -- number of lines to draw
 *
 *    1.  for solid fills we have whacked code from x386 server, which provides
 *        two assembly routines for filling using GXcopy and GXxor.The other
 *        cases go through our old organisation.
 *    2.    for stipple mode filling, we have adopted the method used by SUN cfb
 *        server. The stipple arrays cfb8StippleAnd and cfb8StippleXor are 
 *        setup during startfill time. 
 */

int
v256_fill_lines(xl, xr, y, h)
register int xl, xr, y, h;
{
    DDXPointRec     pt1, pt2;
    int             y2;


    /*
     *     number of rectangles within one vga page and the rectangles 
     *     themselves
     */
    int            num_subrects;
    VgaRegion    subRectangles[MAX_VGA_PAGES * 3];


    /*
     * temporary variable
     */
    int            i;

    extern    int    stipple_cfb_style(    int    xleft,
                                    int    xright,
                                    int    ytop,
                                    int    height);

	extern void 	v256_line_horiz();
	extern void 	v256_hline_tile();

    /*
     * assembly functions from x386 public domain server
     */
    extern unsigned char * fastFillSolidGXcopy();
    extern unsigned char * fastFillSolidGXxor();


    if (v256_gs->mode == GXnoop)
    {
        return(SI_SUCCEED);
    }
    
    y2 = y + h - 1;

    /*
     * Clip points
     */
    if ((xl > v256_clip_x2) || (xr < v256_clip_x1) ||
        (y > v256_clip_y2) || (y2 < v256_clip_y1)) 
    {
        return(SI_SUCCEED);        /* out of bounds */
    }

    /*
     * Clip box
     */
    if (xl < v256_clip_x1) xl = v256_clip_x1;
    if (xr > v256_clip_x2) xr = v256_clip_x2;
    if (y  < v256_clip_y1) y  = v256_clip_y1;
    if (y2 > v256_clip_y2) y2 = v256_clip_y2;

    pt1.x = xl;
    pt1.y = y;
    pt2.x = xr;
    pt2.y = y;
    h = y2 - y + 1;

#ifdef V256FILL_DEBUG
printf("v256_fillrect**\n");
#endif

    switch(v256_gs->fill_mode) 
    {
        case SGFillSolidFG:
        case SGFillSolidBG:
            /*
             * added fast solid fill routines from x386 code
             * special case alu cases copy and xor
             * we have assembly code to do the work
             */
            if ( (v256_function == V256_COPY) || (v256_function == V256_XOR))
            {
                extern unsigned long v256_expand[];

                unsigned char * (* func)();
                unsigned long mask = v256_expand[v256_gs->pmask];
				/*
				 * foreground color replicated in a long  for filling functions
				 * in assembly (x386 code )
				 */
                unsigned long expanded_color = v256_expand[v256_src & mask];

                /*
                 * point to the appropriate function
                 * copy or xor
                 */
                if ( v256_function == V256_COPY)
                {
                    func = fastFillSolidGXcopy;
                }
                else
                {
                    func = fastFillSolidGXxor;
                }


                /*
                 * split the source regions into smaller rectangles that lie
                 * in one vga page -- to minimise the calls to selectpage
                 */

                v256_split_request(pt1.x,pt1.y,pt2.x,(pt2.y+h-1),
                                    &num_subrects, subRectangles);


                for ( i = 0; i < num_subrects; i++)
                {
                    /* 
                     * select the page in which the screen rectangle lies
                     */

#ifdef    V256FILL_DEBUG
                assert ( 
                    (subRectangles[i].x < 1152) & 
                    ( subRectangles[i].y < 900 ) &
                    ((subRectangles[i].x + subRectangles[i].width - 1) < 1152) &
                    ((subRectangles[i].y + subRectangles[i].height - 1) < 900) &
                    (subRectangles[i].x >= 0) & 
                    ( subRectangles[i].y >= 0) &
                    ((subRectangles[i].x + subRectangles[i].width - 1) > 0) &
                    ((subRectangles[i].y + subRectangles[i].height - 1) > 0) 
                );
#endif /* V256FILL_DEBUG */
                    selectpage(OFFSET(subRectangles[i].x,subRectangles[i].y));
                    if (subRectangles[i].height > 1)
                    {
                        func (subRectangles[i].region_p,
                            expanded_color,
                            subRectangles[i].height,
                            subRectangles[i].width,
                            subRectangles[i].width,
                            v256_slbytes-subRectangles[i].width);

                    }
                    else if (subRectangles[i].height == 1)
                    {
                        func ( subRectangles[i].region_p,
                            expanded_color,
                            1,
                            subRectangles[i].width,
                            0,
                            0);
                    }
                    else if (subRectangles[i].height < 1)
                    {
                        return (SI_SUCCEED);
                    }
                }
            }
            else
            {
                /*
                 * not copy or xor operation , general case
                 */

                v256_line_horiz(pt1, pt2, h);
            }
            break;
                
        case SGFillStipple:
                /*
                 *    do stippling MIT's cfb style
                 */
                stipple_cfb_style(pt1.x,pt2.x,pt2.y,h);
                break;

        case SGFillTile:
                v256_hline_tile(pt1, pt2, h);
            break;
    }
    return(SI_SUCCEED);
}
