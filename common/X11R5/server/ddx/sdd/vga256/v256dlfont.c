/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/v256dlfont.c	1.2"

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

/*
#define V256DLFONT_DEBUG
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

extern BYTE v256_pat_fg, v256_pat_bg;
extern BYTE v256_startmask[];
extern BYTE v256_endmask[];

static int left_x, right_x, top_y, bottom_y;
BITS32    *v256_font_vector;
int v256_blocksize;
int v256_chars_per_block;

/*
 * For downloaded fonts, we just copy the font data into an internal 
 * area, leaving the padding in place.  The end result is significantly
 * faster text painting because we eliminate extra function calls, tests, 
 * etc that the SBI must do for it's general cases.
 */


/*
 *    v256_check_dlfont(num, info)     -- Check to see if we can download
 *                    a new font.
 *
 *    Input:
 *        int        num    -- index of the font to be downloaded
 *        SIFontInfoP    info    -- basic info about font
 */
SIBool
v256_check_dlfont(
    int        num,
    SIFontInfoP    info
    )
{
    int height;

    DBENTRY("v256_check_dlfont()");

    if ((info->SFflag & SFTerminalFont) == 0)
    {
        return(SI_FAIL);
    }

    if ((info->SFnumglyph < 0) || (info->SFnumglyph > V256_NUMDLGLYPHS))
    {
        return(SI_FAIL);
    }

    if ((info->SFmax.SFwidth < 1) || (info->SFmax.SFwidth > 25))
    {
        return(SI_FAIL);
    }
        
    height = info->SFmax.SFascent + info->SFmax.SFdescent;

    if ((height < 1) || (height > V256_DL_FONT_H))
    {
        return(SI_FAIL);
    }
    
    return(SI_SUCCEED);

}


/*
 *    v256_dl_font(num, info, glyphs)     -- download the glyphs for a font
 *
 *    Input:
 *        int        num    -- the index for the downloaded font
 *        SIFontInfoP    info    -- basic info about font
 *        SIGlyphP    glyphs    -- the glyphs themselves
 */

SIBool
v256_dl_font(
    int        num,
    SIFontInfoP    info,
    SIGlyphP    glyphs
    )
{
    register BITS32    *font_dst, *font_src;
    register int    i, chars;
    int        h;

    DBENTRY("v256_dl_font()");

    v256_fonts[num].w = info->SFmax.SFwidth;
    v256_fonts[num].h = h = info->SFmax.SFascent + info->SFmax.SFdescent;
    v256_fonts[num].ascent = info->SFmax.SFascent;
    chars = info->SFnumglyph;

    if (!(v256_fonts[num].glyphs = (BYTE *) malloc(chars * 4 * h)))
    {
        return(SI_FAIL);
    }

    font_dst = (BITS32 *) v256_fonts[num].glyphs;

    for (; chars; chars--, glyphs++) 
    {
        i = h;                    /* number of lines */
        font_src = (BITS32 *)glyphs->SFglyph.Bptr;

        while (i--)
        {
            *font_dst++ = *font_src++;
        }
    }

    return(SI_SUCCEED);
}


/*
 *    v256_stpl_font(num, x_start, y, cnt_start, glyphs_start, type)
 *                -- stipple glyphs in a downloaded font.
 *
 *    Input:
 *        int    num    -- font index to stipple from
 *        int    x_start    -- x position of baseline to stipple to
 *        int     y    -- y position of baseline to stipple to
 *        int    cnt_start    -- number of glyphs to stipple
 *        BITS16    *glyphs    -- list of glyph indices to stipple
 *        int    opaque    -- Opaque or regular stipple (if non-zero)
 */
SIBool
v256_stpl_font(
    int    num,
    int    x_start,
    int    y,
    int    cnt_start,
    BITS16    *glyphs,
    int    opaque
    )
{
	extern void v256_stplfill();

    register int x;
    int    w, sy;
    int    char_height, ht;
    int    step_size, cnt;

    BYTE     *psrc, *src;
    int    pdst, dst;
    BYTE    mask1, mask2;
    int    saved_function;

	/*
	 * save the forcetype and set a flag if the cfbstipple arrays are 
	 * destroyed due to nonzero forcetype
	 */
    int    forcetype;
    SIBool    destroyed_globals = SI_FALSE;


    DBENTRY("v256_stpl_font()");

    forcetype = opaque;

    saved_function = v256_function;
#ifdef V256DLFONT_DEBUG
    printf("v256stplfont (%d), saved_stp_mode (%d) ",opaque, v256_gs->stp_mode);
#endif

    if (opaque == SGOPQStipple ||
        (opaque == 0 && v256_gs->stp_mode == SGOPQStipple)) 
    {
        /*
         * opaque stippling
         */

        opaque = 1;

        v256_function = V256_COPY;
        v256_pat_fg = v256_gs->fg;
        v256_pat_bg = v256_gs->bg;

		/*
		 * if forcetype is set then set the cfbstipple arrays  and also
		 * a flag for subsequent restoration
		 */
        if (forcetype)
        {
#ifdef V256DLFONT_DEBUG
    printf("check opaque ");
#endif
            cfb8CheckOpaqueStipple( GXcopy,
                                    v256_gs->fg,
                                    v256_gs->bg,
                                    v256_gs->pmask);
            destroyed_globals = SI_TRUE;
        } 
    } 
    else 
    {
          /*
         * transparent stippling
         */
        opaque = 0;

        if ( forcetype )
        {
#ifdef V256DLFONT_DEBUG
    printf("check transparent ");
#endif
            cfb8CheckStipple (  v256_gs->mode,
                                v256_gs->fg,
                                v256_gs->pmask);
            destroyed_globals = SI_TRUE;
        }

        if (v256_invertsrc)
        {
            v256_pat_fg = v256_gs->fg ^ v256_gs->pmask;
        }
        else
        {
            v256_pat_fg = v256_gs->fg;
        }

        switch (v256_gs->mode) 
        {
        case GXset:
        case GXinvert:
            v256_pat_fg = v256_gs->pmask;
            break;
        case GXclear:
            v256_pat_fg = 0;
            break;
        }
    }

    y -= v256_fonts[num].ascent;
    char_height = v256_fonts[num].h;
    w = v256_fonts[num].w;
    v256_font_vector = (BITS32 *)v256_fonts[num].glyphs;

    /*
     * Figure out all the clipping bounds
     */
    sy = 0;
    left_x   = x = x_start;
    right_x  = x_start + (w * cnt_start);
    top_y    = y;
    bottom_y = y + char_height - 1;

    if ((left_x > v256_clip_x2) || (right_x <= v256_clip_x1) ||
        (top_y > v256_clip_y2) || (bottom_y < v256_clip_y1))  
    {
        goto done;
    }

    if (right_x > v256_clip_x2)
    {
        right_x = v256_clip_x2 + 1;
    }

    if (top_y < v256_clip_y1) 
    {
        sy = v256_clip_y1 - top_y;
        top_y = v256_clip_y1;
    }
    if (bottom_y > v256_clip_y2)
    {
        bottom_y = v256_clip_y2;
    }

    ht = bottom_y - top_y + 1;

    v256_chars_per_block = (cnt_start + 7) & ~7;
    if (char_height * (w * v256_chars_per_block + 7) >= 8 * MAXSCANLINE) 
    {
        v256_chars_per_block = ((8*MAXSCANLINE)/(char_height*w)) & ~7;
    }
    step_size = v256_chars_per_block * w;
    v256_blocksize = step_size >> 3;
    src = v256_slbuf + v256_blocksize * sy;
    psrc = src;
    mask1 = 0xFF;

    if (left_x < v256_clip_x1) 
    {
        while (x < v256_clip_x1) 
        {
            x += w;
            glyphs++;
        }
        left_x = x;
        if (x != v256_clip_x1) 
        {
            x -= w;
            glyphs--;
            left_x = v256_clip_x1 - x;
            mask1 = v256_startmask[left_x & 0x07];
            psrc += (left_x >> 3);
            left_x = x + (left_x & ~0x07);
        }
        if (right_x <= x + w) 
        {
            v256_blocksize = 4;
            w = right_x - x;
            mask2 = v256_endmask[(w-1) & 0x07];
            cnt = psrc - src;
            psrc = (BYTE *)&(v256_font_vector[(*glyphs)*char_height+sy]);
            psrc += cnt;
            cnt = (right_x - x + 7) >> 3 - cnt;
            pdst = left_x + v256_slbytes * top_y;
            v256_stplfill(psrc, pdst, cnt, mask1, mask2, ht, opaque);
            goto done;
        }
    }
    dst = v256_slbytes * top_y;
    pdst = left_x + dst;
    dst += x;

    if (right_x <= x + w) 
    {
        v256_blocksize = 4;
        w = right_x - x;
        mask2 = v256_endmask[(w-1) & 0x07];
        psrc = (BYTE *) &(v256_font_vector[(*glyphs)*char_height + sy]);
        cnt = (w + 7) >> 3;
        v256_stplfill(psrc, pdst, cnt, mask1, mask2, ht, opaque);
        goto done;
    }

    while (right_x - x > step_size) 
    {
        v256_stippletext(glyphs, v256_chars_per_block, char_height, w);
        cnt = step_size >> 3;
        v256_stplfill(psrc, pdst, cnt, mask1, 0xFF, ht, opaque);
        glyphs += v256_chars_per_block;
        dst += step_size;
        x += step_size;
        left_x = x;
        mask1 = 0xFF;
        psrc = src;
        pdst = dst;
         
    }
    cnt = right_x - x;
    mask2 = v256_endmask[(cnt-1) & 0x07];
    v256_stippletext(glyphs, (cnt + w - 1)/w, char_height, w);
    cnt = (cnt + 7) >> 3;
    v256_stplfill(psrc, pdst, cnt, mask1, mask2, ht, opaque);


done:
    v256_function = saved_function;

	/*
	 * restore the cfbstipple arrays in case they were destroyed 
	 */
    if(destroyed_globals == SI_TRUE)
    {
        if(v256_gs->stp_mode == SGOPQStipple)
        {
#ifdef V256DLFONT_DEBUG
    printf("restoring opaque\n");
#endif
            cfb8CheckOpaqueStipple( v256_gs->mode,
                                    v256_gs->fg,
                                    v256_gs->bg,
                                    v256_gs->pmask);

        }
        else if(v256_gs->stp_mode == SGStipple)
        {
#ifdef V256DLFONT_DEBUG
    printf("restoring transparent\n");
#endif
            cfb8CheckStipple (  v256_gs->mode,
                                v256_gs->fg,
                                v256_gs->pmask);
        }
    }
    return(SI_SUCCEED);
}



/*
 *    v256_font_free(num)    -- Free data structures associated with a
 *                downloaded font.
 *
 *    Input:
 *        int    num    -- index of font
 */
SIBool
v256_font_free(
    int    num
    )
{
    DBENTRY("v256_font_free()");

    free(v256_fonts[num].glyphs);

    return(SI_SUCCEED);
}
