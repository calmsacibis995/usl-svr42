/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/v256bitblt.c	1.3"

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
#define    MS_BITBLT_DEBUG 1
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

/*
 *    Bring in includes for VgaRegion etc
 */
#include 	"v256spreq.h"
#include 	"v256bitblt.h"


#ifdef    MS_BITBLT_DEBUG
#include    <stdio.h>
#endif    /* MS_BITBLT_DEBUG */

extern long v256_expand[];
extern void v256_moveup();
extern void v256_movedown();
extern void v256_invertup();
extern void v256_invertdown();

static int incr;
static long local_mask;

static    VgaRegion    subRectangles[MAX_VGA_PAGES * 3];

/*
 *    v256_fast_vidwrite : 
 *        Write one scan line statring from address base+dest.
 *    This is the same as 'vidwrite' except that it does not call selectpage.
 *    This means that the caller MUST ensure that the scanline passed falls in
 *    one VGA page.
 */
int
v256_fast_vidwrite( 
    register unsigned long dest,
    BYTE *src,
    register int k,
    unsigned long mask
    )
{ 
    dest &= VIDEO_PAGE_MASK; 
	/*
	 *	Special case COPY : 99.95% of the time
	 */
	if ( v256_function == V256_COPY )
	{

            if (~mask == 0) 
            { 
                v256_memcpy(v256_fb+dest, src, k); 
            } 
            else 
            { 
                v256_memcpymask(v256_fb+dest, src, k, mask); 
            } 
			return 0;
	}
	/*
	 *	All other cases
	 */
    switch (v256_function)  
    { 
 
        case V256_XOR: 
            v256_memxor(v256_fb+dest, src, k, mask); 
            break; 
 
        case V256_OR: 
            v256_memor(v256_fb+dest, src, k, mask); 
            break; 
 
        case V256_AND: 
            v256_memand(v256_fb+dest, src, k, mask); 
            break; 
 
        case V256_INVERT: 
            v256_cpyinvert(v256_fb+dest, src, k, mask); 
            break; 
 
        case V256_OR_INVERT: 
            v256_memor_i(v256_fb+dest, src, k, mask); 
            break; 
 
        case V256_AND_INVERT: 
            v256_memand_i(v256_fb+dest, src, k, mask); 
            break; 
    } 
    return 0;
}


/* 
 *    v256_ss_bitblt(sx, sy, dx, dy, w, h)    -- Moves pixels from one screen
 *                        position to another using the
 *                        ROP from the setdrawmode call.
 *
 *    Input:
 *        int    sx    -- X position (in pixels) of source
 *        int    sy    -- Y position (in pixels) of source
 *        int    dx    -- X position (in pixels) of destination
 *        int    dy    -- Y position (in pixels) of destination
 *        int    w    -- Width (in pixels) of area to move
 *        int    h    -- Height (in pixels) of area to move
 */
SIBool
v256_ss_bitblt(sx, sy, dx, dy, w, h)
int    sx, sy, dx, dy;
int    w, h;
{
    /*
     * array to read in the contents of one vga page 
     * in the worst case we will require 64KB ( sizeof(onevgapage) )
     */
    char one_vga_page[VGA_PAGE_SIZE];
    register int i, j, k;
    register BYTE *p,*q;
    DDXPointRec pt1, pt2;

    /*
     * flag to indicate if we should do a forward or backward copy
     */
    SIBool    is_forward_move;

    /* 
     * return values of split_request call stored here
     */
    int    num_subrects_src,num_subrects_dst;

    /*
     * array for storing the destination sub rectangles
     */
    VgaRegion subRectanglesDst[MAX_VGA_PAGES * 3];





    /*
     * check if we have anything to do at all
     */
    if ( (v256_gs->mode == GXnoop) || ((w == 0) || (h == 0)) )
    {
        return (SI_SUCCEED);
    }

    local_mask = v256_expand[v256_gs->pmask];

    /* 
     * split the source regions into smaller rectangles that lie
     * in one vga page -- to minimise the calls to selectpage
     */
     (void) v256_split_request(sx,sy,(sx+w-1),(sy+h-1),&num_subrects_src,
                                subRectangles );


    /*
     * compute if the source and destination regions overlap
     * and if is there a necessity to do a backward copy
     */
    j = OFFSET(sx,sy);
    k = OFFSET(dx,dy);
    if ( (j < k) && k < (j + h*v256_slbytes + w)) 
    {
        is_forward_move = SI_FALSE;
    }
    else
    {
        is_forward_move = SI_TRUE;
    }


    /* 
     * place the rectangles on the screen 
     */
    switch (v256_gs->mode)
    {
        
        /*
         * special case the rops which do not involve the src pixmap
         */
        case GXset:
        case GXinvert:
        case GXclear:
		{
			
			(void) v256_split_request(dx,dy,(dx+w-1),(dy+h-1),
				&num_subrects_dst,subRectanglesDst);
			for ( i = 0; i < num_subrects_dst; i++)
			{
				/*
				 * select the page for the write operation
				 */
				selectpage(OFFSET(subRectanglesDst[i].x,
								subRectanglesDst[i].y));
				pt1.x = subRectanglesDst[i].x;
				pt1.y = pt2.y = subRectanglesDst[i].y;
				pt2.x =  subRectanglesDst[i].x +
						 subRectanglesDst[i].width - 1;
				v256_line_horiz(pt1, pt2, subRectanglesDst[i].height);
			}
		}
        break;

        default: 
            j = num_subrects_src -1;
            while ( j >= 0)
            {
                /*
                 * set i to point to current source rectangle in the list
                 * depending on back or front move
                 */
                if ( is_forward_move == SI_TRUE)
                {
                    i = num_subrects_src -1 -j;
                }
                else /* backward copy */
                {
                    i = j;
                }
                /* 
                 * store the start pt. of destination region 
                 * corresponding to the source rectangle in variable pt1
                 */
                pt1.x = subRectangles[i].x - sx + dx;
                pt1.y = subRectangles[i].y - sy + dy;

                /* 
                 * split the dest region into sub rectangles 
                 */
                (void)v256_split_request(pt1.x,pt1.y,
                                        pt1.x + subRectangles[i].width -1,
                                        pt1.y + subRectangles[i].height -1,
                                        &num_subrects_dst,
                                        subRectanglesDst);
                
                /* 
                 * copy from source rectangle into a buffer
                 * first select page then copy
                 */
                selectpage(OFFSET(subRectangles[i].x,subRectangles[i].y));
                p = (BYTE *)one_vga_page;
                for (     k = 0,
                        q = (BYTE *)v256_fb + 
                                (OFFSET(    subRectangles[i].x,
                                        subRectangles[i].y) & VIDEO_PAGE_MASK);
                        k < subRectangles[i].height; 
                        k++,p+=v256_slbytes,q+=v256_slbytes)
                {
                    v256_vidcpy(p,q,subRectangles[i].width);

                    /*
                     * check if source is to be inverted before the rop
                     */
                    if ( v256_invertsrc )
                    {
                        register BYTE     *t1;
                        register int    t2;

                        t1 = p;
                        for ( t2 = 0; t2 < subRectangles[i].width; t2++,t1++)
                        {
                            *t1 = ~*t1;
                        }
                    }
                }
                
                /* 
                 * transfer from source buffer to destination screen
                 * with the rop performed
                 */
                for ( k = 0; k < num_subrects_dst; k++)
                {
                    /* 
                     * select the destination page
                     */
                    selectpage(OFFSET(    subRectanglesDst[k].x,
                                         subRectanglesDst[k].y));

                    /*
                     * offset of destination position in the screen 
                     */
                    q = (BYTE *) OFFSET(subRectanglesDst[k].x,
                                         subRectanglesDst[k].y);
                    /*
                     * address of start of current rectangle in the 
                     * source pixmap
                     */
                    p = (BYTE *) one_vga_page + 
                            OFFSET(    (subRectanglesDst[k].x - pt1.x),
                                    (subRectanglesDst[k].y - pt1.y));
                    /* 
                     * do the transfer
                     */
                    for(;subRectanglesDst[k].height > 0 ;
                        subRectanglesDst[k].height--,
                        p += v256_slbytes,
                        q += v256_slbytes)
                    {
                        v256_fast_vidwrite((unsigned long)q,p,
                                    subRectanglesDst[k].width,local_mask);
                    }
                } /* end of  for ( k = 0; k < num_subrects_dst; k++) */

                /* 
                 * point to the next source rectangle
                 */
                j --; 
            }/* end of while ( j >= 0) */
            break;
    }/* end of switch (v256_gs->mode) */

    return(SI_SUCCEED);
}


/*
 *    v256_ms_bitblt(src, sx, sy, dx, dy, w, h) -- Moves pixels from memory
 *                        to the screen using the ROP 
 *                        from the setdrawmode call.
 *
 *    Input:
 *        SIbitmapP    src    -- pointer to source data        
 *        int        sx    -- X position (in pixels) of source
 *        int        sy    -- Y position (in pixels) of source
 *        int        dx    -- X position (in pixels) of destination
 *        int        dy    -- Y position (in pixels) of destination
 *        int        w    -- Width (in pixels) of area to move
 *        int        h    -- Height (in pixels) of area to move
 */
SIBool
v256_ms_bitblt(src, sx, sy, dx, dy, w, h)
SIbitmapP     src;
int        sx, sy, dx, dy;
int        w, h;
{
    /*
     *    The number of split rectangles returned by split request
     */
    int        num_subrects;
    /*
     *    Bitmap used to call v256_rop
     */
    SIbitmap    dest_bitmap;

    /*
     *    Mainitains the current position in the source bitmap
     */
    int        current_src_x,    current_src_y;

    /*
     *     The constant's sx - dx and sy - dy
     */
    const    src_x_diff = sx - dx;
    const     src_y_diff = sy - dy;

    register int    dest;
    BYTE    *psrc;
    BYTE     *p, *s;
    int     srcinc, i, j;
    DDXPointRec pt1, pt2;


#ifdef    MS_BITBLT_DEBUG
    fprintf(stderr, "\
v256_ms_bitblt :\n\
    src->width= %d, src->height= %d\n\
    mode= %d\n\
    sx= %d, sy= %d, dx= %d, dy= %d\n\
    w= %d, h= %d\n",
    src->Bwidth, src->Bheight,
    v256_gs->mode,
    sx, sy, dx, dy,
    w, h
        );
#endif    /* MS_BITBLT_DEBUG */

    if (v256_gs->mode == GXnoop)
    {
        return    (SI_SUCCEED);
    }

    if ((w == 0) || (h == 0))
    {
        return(SI_SUCCEED);
    }

    if (src->Bdepth != V256_PLANES)    /* only handle screen depth */
    {
        return(SI_FAIL);
    }

    local_mask = v256_expand[v256_gs->pmask];

    /*
     *    Warning : this assumes that the src pixmap is 
     *     int boundary padded
     */
    srcinc = ((src->Bwidth + 3) & ~3);

    /* 
     *    split the request into small rectangles each within one
     *    vga page (64k)
     */
    (void) v256_split_request(dx, dy, (dx+w-1), (dy+h-1),
        &num_subrects, subRectangles );


    dest_bitmap.Btype = Z_PIXMAP;
    dest_bitmap.Bdepth = V256_PLANES;
    dest_bitmap.Bwidth = v256_slbytes;

    /*
     *    place rectangles on screen
     */
    for (i = 0; i < num_subrects; i ++)
    {
        register    int    sub_x = subRectangles[i].x;
        register    int    sub_y = subRectangles[i].y;
        register     int sub_height = subRectangles[i].height;

	/* 
	 * sx + (subRectangles[i].x - dx); 
	 */
        current_src_x = src_x_diff + sub_x; 

	/* 
	 * sy + (subRectangles[i].y - dy); 
	 */
        current_src_y = src_y_diff + sub_y; 

        /*
         * select page for drawing
         */

#ifdef    DELETE
printf("selectpage((%d,%d)->%d)\n",
subRectangles[i].x, subRectangles[i].y,
OFFSET(sub_x, sub_y));
#endif /* DELETE */

        selectpage(OFFSET(sub_x, sub_y));

        /*
         *    check for non-trivial rectangles
         */
        if (sub_height > 0 && sub_height <= V256_MIN_ROP_HEIGHT )
        {
            register char *current_src_p = 
                (char *)src->Bptr + (srcinc * current_src_y) + current_src_x;

            dest = OFFSET(sub_x,sub_y);

            switch(v256_gs->mode)
            {
                case GXset:
                case GXinvert:
                case GXclear:
                    pt1.x = sub_x;
                    pt1.y = pt2.y = sub_y;
                    pt2.x = sub_x + subRectangles[i].width - 1;
                    /*
                     * Draw block
                     */
                    v256_line_horiz(pt1, pt2, sub_height);
                    break;

                default:
                    for(;sub_height > 0;
                        sub_height--, current_src_p += srcinc,
                        dest += v256_slbytes)
                    {
                        if (v256_invertsrc)
                        {
                            s = (BYTE *)current_src_p; 
                            p = v256_tmpsl;
                            for (j = 0; j < subRectangles[i].width ; j++)
                            {
                                *p++ = ~*s++;
                            }
                            v256_fast_vidwrite(dest, v256_tmpsl, 
                                subRectangles[i].width, local_mask);
                        }
                        else
                        {
                            v256_fast_vidwrite(dest, (BYTE *)current_src_p, 
                                subRectangles[i].width, local_mask);
                        }
                    }
                    break;
            }

        }
        else if (sub_height > 20 )
        /* 
         * Call the rop
         */
        {

            dest_bitmap.Bheight = sub_height;
            dest_bitmap.Bptr = (SIArray)subRectangles[i].region_p;

            /*
             *    call v256_rop !!!
             */
            v256_rop(src, &dest_bitmap, 
                current_src_x,
                current_src_y, 
                /*
                 * dest start will always be 0,0
                 */
                0, 0,
                subRectangles[i].width,
                sub_height,
                v256_gs->mode,
                v256_gs->pmask );

        }
        else
        {
#ifdef    NDEBUG
            /*
             *    we should NEVER come here
             */

            assert(0);
#endif    /* NDEBUG */
        }
    }

    return(SI_SUCCEED);
}


/*
 *  v256_sm_bitblt(dst, sx, sy, dx, dy, w, h) -- Moves pixels from the
 *              screen to memory 
 *
 *  Input:
 *      SIbitmapP   dst -- pointer to destination buffer
 *      int     sx  -- X position (in pixels) of source
 *      int     sy  -- Y position (in pixels) of source
 *      int     dx  -- X position (in pixels) of destination
 *      int     dy  -- Y position (in pixels) of destination
 *      int     w   -- Width (in pixels) of area to move
 *      int     h   -- Height (in pixels) of area to move
 */
SIBool
v256_sm_bitblt(dst, sx, sy, dx, dy, w, h)
SIbitmapP   dst;
int     sx, sy, dx, dy;
int     w, h;
{

    /*
     * number of split rectangles in the source rectangle
     */
    int            num_subrects;

    /*
     * pointers to maintain the current position in the dst bitmap 
     */
    int            current_dst_x,current_dst_y;

    /*
     * SIbitmap structure for source ( screen ) for v256_rop call
     */
    SIbitmap    src_bitmap;

    /*
     *     The constant's dx - sx and  dy - sy
     */
    const    dst_x_diff = dx - sx ;
    const     dst_y_diff = dy - sy ;

    int            dstinc; /* width of the destination bitmap for increments */
    int            i,j;
    BYTE        *p,*s;
    DDXPointRec    pt1,pt2;
    long        source; /* offset in the vga screen of current rectangle */


    /*
     * nothing to do ?
     */
    if ( v256_gs->mode == GXnoop)
    {
        return (SI_SUCCEED);
    }

    /*
     * most trivial case, job is done
     */
    if ((w == 0) || (h == 0))
    {
        return(SI_SUCCEED);
    }

    /*
     * we handle only screen depth
     */
    if ( dst->Bdepth != V256_PLANES)
    {
        return(SI_FAIL);
    }


    local_mask = v256_expand[v256_gs->pmask];
    dstinc = ((dst->Bwidth + 3) & ~3);

    /* 
     * split the source ( screen ) rectangles into smaller rectangles 
     * such that each rectangle is within one vga page ( 64k)
     */

    (void) v256_split_request(sx,sy,sx+w-1,sy+h-1,
                        &num_subrects,subRectangles);

    
    /*
     * these fields in the SIbitmap structure do not change, put it outside
     * the for loop
     */
    src_bitmap.Btype = Z_PIXMAP;
    src_bitmap.Bdepth = V256_PLANES;
    src_bitmap.Bwidth = v256_slbytes;

    /* 
     * place the rectangles on the screen 
     */
    
    for ( i = 0; i < num_subrects; i++ )
    {
        register int sub_x = subRectangles[i].x;
        register int sub_y = subRectangles[i].y;
        register int sub_height = subRectangles[i].height;

        /*
         * compute the current position in the dst bitmap
         */
        current_dst_x = dst_x_diff + sub_x ;
        current_dst_y = dst_y_diff + sub_y ;

        /* 
         * select the page for drawing */
        selectpage(OFFSET(sub_x,sub_y));


        /*
         * check for small rectangles 
         */
        if ( sub_height > 0 && sub_height <= V256_MIN_ROP_HEIGHT)
        {
            register char  *current_dst_p =
                (char *)dst->Bptr + (dstinc * current_dst_y) + current_dst_x;

            source = (OFFSET(sub_x,sub_y)) & VIDEO_PAGE_MASK;

            switch(v256_gs->mode)
            {
            case GXset:
            case GXinvert:
            case GXclear:
                while(sub_height--)
                {
                    filler((BYTE *)current_dst_p,
                    (BYTE *)(current_dst_p+subRectangles[i].width - 1));
                }
                break;
            default :
                for(;sub_height--;
                    current_dst_p += dstinc,source += v256_slbytes)
                {
                    /*
                     * read one line from the vga memory into buffer
                     */
                    if (v256_invertsrc)
                    {
                        v256_vidcpy(v256_tmpsl, ((BYTE *)v256_fb)+source,
						subRectangles[i].width);
                        for ( j = 0; j < subRectangles[i].width; j++)
                        {
                            v256_tmpsl[j] = ~v256_tmpsl[j];
                        }
                        V256_FAST_TRANSFER(v256_tmpsl,current_dst_p,
                                subRectangles[i].width,local_mask);
                    }
                    else
                    {
                        V256_FAST_TRANSFER(((BYTE *)v256_fb + source),
                            current_dst_p,
                            subRectangles[i].width,local_mask);

                    }
                }
                break;
            } /* end of switch(v256_gs->mode) */
        }
        else if (sub_height > V256_MIN_ROP_HEIGHT )
        {
            /* 
             * call v256_rop
             */

             src_bitmap.Bheight = sub_height;
             src_bitmap.Bptr = (SIArray)subRectangles[i].region_p;

             /*
              * call v256_rop ???
              */
              v256_rop ( &src_bitmap,dst,
              0,0,
              current_dst_x,current_dst_y,
              subRectangles[i].width,sub_height,
              v256_gs->mode,
              v256_gs->pmask);
        }
        else 
        {
            /*
             * we should not reach this case 
             */ 
#ifdef    NDEBUG
             assert(0);
#endif    /* NDEBUG */
        }

    } /* end of  for ( i = 0; i < num_subrects; i++ ) */
    return(SI_SUCCEED);
}




vidwrite(dest, src, w, mask)
register unsigned long dest;
BYTE *src;
register int w;
unsigned long mask;
{
    register unsigned long pdst;
    int k;

    pdst = dest;
    selectpage(pdst);
    dest &= VIDEO_PAGE_MASK;
    while (w) {
        if (pdst+w-1 > v256_endpage)
            k = v256_endpage - pdst + 1;
        else
            k = w;

        switch (v256_function) {
        case V256_COPY:
            if (~mask == 0)
                v256_memcpy(v256_fb+dest, src, k);
            else
                v256_memcpymask(v256_fb+dest, src, k, mask);
            break;

        case V256_XOR:
            v256_memxor(v256_fb+dest, src, k, mask);
            break;

        case V256_OR:
            v256_memor(v256_fb+dest, src, k, mask);
            break;

        case V256_AND:
            v256_memand(v256_fb+dest, src, k, mask);
            break;

        case V256_INVERT:
            v256_cpyinvert(v256_fb+dest, src, k, mask);
            break;

        case V256_OR_INVERT:
            v256_memor_i(v256_fb+dest, src, k, mask);
            break;

        case V256_AND_INVERT:
            v256_memand_i(v256_fb+dest, src, k, mask);
            break;
        }
        src += k;
        w -= k;
        dest = 0;
        pdst += k;
        selectpage(pdst);
    }
    return;
}




vidread(dst, source, w)
register BYTE *dst;
register int source;
int w;
{
    register unsigned long psrc;
    int k;

    psrc = source;
    selectpage(psrc);
    source &= VIDEO_PAGE_MASK;
    while (w) {
        if (psrc+w-1 > v256_endpage)
            k = v256_endpage - psrc + 1;
        else
            k = w;
    
        v256_vidcpy(dst, v256_fb+source, k);

        dst += k;
        w -= k;
        source = 0;
        psrc += k;
        selectpage(psrc);
    }
}




backmove(j, k, h, w)
register int j, k;
int h, w;
{
    register BYTE *src, *dst;

    j += w - 1;
    k += w - 1;

    selectpage(j);
    src = v256_fb + (j & VIDEO_PAGE_MASK);
    dst = v256_fb + (k & VIDEO_PAGE_MASK);

    while (--h >= 0) {
        /*
         * if the source and destination bytes for this line are
         * all in the same page, we can do just one copy.  Otherwise,
         * we have to read the source in one operation and write
         * to the dest in another using vidread/vidwrite.
         */
        if (((j & VIDEO_PAGE_MASK) >= (w-1)) && ((k & VIDEO_PAGE_MASK) >= (w-1)) &&
            ((j & ~VIDEO_PAGE_MASK) == (k & ~VIDEO_PAGE_MASK))) {
            selectpage(j);
            v256_movedown(dst, src, w);
        }
        else {
            vidread(v256_tmpsl, j-(w-1), w);
            vidwrite(k-(w-1), v256_tmpsl, w, local_mask);
        }

        dst += incr;
        if (dst < v256_fb)
            dst += (VIDEO_PAGE_MASK+1);

        src += incr;
        if (src < v256_fb)
            src += (VIDEO_PAGE_MASK+1);

        k += incr;
        j += incr;
    }
}



fwdmove(j, k, h, w)
register int j, k;
int h, w;
{
    register BYTE *src, *dst;

    src = v256_fb + (j & VIDEO_PAGE_MASK);
    dst = v256_fb + (k & VIDEO_PAGE_MASK);

    while (--h >= 0) 
    {
        /*
         * if the source and destination bytes for this line are
         * all in the same page, we can do just one copy.  Otherwise,
         * we have to read the source in one operation and write
         * to the dest in another using vidread/vidwrite.
         */
        if ((((j&VIDEO_PAGE_MASK) + w) < (VIDEO_PAGE_MASK+1)) && (((k&VIDEO_PAGE_MASK) + w) < (VIDEO_PAGE_MASK+1)) &&
            ((j & ~VIDEO_PAGE_MASK) == (k & ~VIDEO_PAGE_MASK))) {
            selectpage(j);
            v256_moveup(dst, src, w);
        }
        else {
            vidread(v256_tmpsl, j, w);
            vidwrite(k, v256_tmpsl, w, local_mask);
        }

        dst += incr;
        if (dst >= v256_fb + (VIDEO_PAGE_MASK+1))
            dst -= (VIDEO_PAGE_MASK+1);

        src += incr;
        if (src >= v256_fb + (VIDEO_PAGE_MASK+1))
            src -= (VIDEO_PAGE_MASK+1);

        k += incr;
        j += incr;
    }
}
