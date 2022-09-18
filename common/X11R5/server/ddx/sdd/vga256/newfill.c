/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/newfill.c	1.1"

/*
 *	Copyright (c) 1991, 1992 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/*
 * File History :
 *
 * File : 
 *	newfill.c
 *
 * Description : 
 * 	This file contains code for doing stippling operations , in the cfb 
 * 	style. i.e., at graphics download time the reduced rop will be 
 *	identified and at the startfill time the stipple lookup arrays 
 * 	will be setup. For each line of the stipple an routine
 * 	will be called which will use these precomputed values and do
 *	the stippling for all cases of Gxfunction. This we hope will reduce
 *	drastically the amount of code and also simplify things.
 *	The code in this file is used for doing what has been catagorized as
 *	small stipples in cfbgc.c
 *
 *	21 Dec 91
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

#ifdef	V256FILL_DEBUG
#define NDEBUG 1
#endif	/* V256FILL_DEBUG */
#include <assert.h>


unsigned long cfb8StippleMasks[16] = {
    0x00000000, 0x000000ff, 0x0000ff00, 0x0000ffff,
    0x00ff0000, 0x00ff00ff, 0x00ffff00, 0x00ffffff,
    0xff000000, 0xff0000ff, 0xff00ff00, 0xff00ffff,
    0xffff0000, 0xffff00ff, 0xffffff00, 0xffffffff,
};

/*
 * vars to avoid doing things for a second time
 * initialize one of them to make sure that first time succeeds
 */
int 			cfb8StippleMode = -1; 
int				cfb8StippleAlu; 
int				cfb8StippleRRop;
unsigned long   cfb8StippleFg; 
unsigned long	cfb8StippleBg; 
unsigned long	cfb8StipplePm;

/*
 * the heart of the cfb style of stippling. 
 * the and and xor arrays
 */
unsigned long   cfb8StippleAnd[16]; 
unsigned long	cfb8StippleXor[16];

/*
 *	start and end masks for masking relevant pixels in the beginning and
 * 	end of the scanline, used by v256stipple.s
 */
unsigned long 	v256_stipple_start_mask[] = 
	{ 0xFFFFFFFF,0xFFFFFF00,0xFFFF0000,0xFF000000 };
unsigned long 	v256_stipple_end_mask[] = 
	{ 0x000000FF,0x0000FFFF,0x00FFFFFF,0xFFFFFFFF };

/*
 *	Function : 
 *  generalised_stippling_routine:
 *      int     xl  -- left edge of line to fill
 *      int     xr  -- right edge of line to fill
 *      int     y   -- y position of line to fill
 *      int     h   -- number of lines to draw
 *
 *	This function breaks up the destination region into rectangles so that
 *	each rectangle lies completely in one vga segment. Then for each line 
 *	of the source stipple the function v256_do_stippleline is called 
 *	which stipples the line of source stipple in the appropriate places in 
 * 	the rectangle that is passed to it. It assumes that select page has 
 *	been done.
 */

int
stipple_cfb_style(int xl,int xr,int y,int h)
{
	/*
	 * extern function prototype definition
	 */
	extern void	v256_do_stipple_line(
					BITS16	*src_bits_p,
					int		stplwidth,
					int		cur_pat_h,
					BYTE 	*dst_rect_ptr,
					int		x_top_left,
					int		x_bottom_right,
					int		numlines
					);

	/*
	 * for breaking destination into rectangles that lie wholly in one vga
	 * segment
	 */
    int         num_subrects;
    VgaRegion   subRectangles[MAX_VGA_PAGES * 3];
    int         i;

	/*
	 * source stipple descriptors
	 */
	BITS16 	*src_bits_p;
	int		current_pat_y;
	int		j;


	/*
	 * trivial case ? 
	 */
	 if ( h <= 0 )
	 {
		return (SI_SUCCEED);
	 }

	/*
	 * split the source regions into smaller rectangles that lie
	 * in one vga page -- to minimise the calls to selectpage
	 */
	v256_split_request(xl,y,xr,(y+h-1),&num_subrects, subRectangles);


	for ( i = 0; i < num_subrects; i++)
	{
		/*
		 *  Top left and bottom right of rect to be stippled
		 */
		int		x_top_left,y_top_left;
		int		x_bottom_right;

		/*
		 * destination width and height
		 */
		int		dst_rect_width;
		int		dst_rect_height;

		/*
		 * destination rectangle pointer
		 */
		BYTE *	dst_rect_ptr;

		/*
		 * extra lines ???? -> for numlines computation
		 * and associated vars for drawing these extra lines
		 */
		int		extra_lines; 
		int		extra_start;
		int		extra_end;
		int		current_line_no;
		int		exact_stipples;

		/*
		 * destination rectangle bounds
		 */
		dst_rect_width = subRectangles[i].width;
		dst_rect_height = subRectangles[i].height;
		x_top_left = subRectangles[i].x;
		y_top_left = subRectangles[i].y;
		x_bottom_right = subRectangles[i].x + dst_rect_width - 1 ;
		dst_rect_ptr = v256_fb + 
						(OFFSET(x_top_left, y_top_left) & VIDEO_PAGE_MASK);


		/*
		 * number of extra lines in the destination to stipple after
		 * integral number of source stipples have been placed one below
		 * the other in the destination rectangle
		 */
		extra_lines = dst_rect_height % v256_cur_pat_h;
		exact_stipples = dst_rect_height / v256_cur_pat_h;

		/*
		 * select the page in which the screen rectangle lies
		 */
		selectpage( OFFSET(x_top_left, y_top_left) ) ;

		/*
		 * line number of source stipple that corresponds to first line of dst
		 */
		current_pat_y = y_top_left % v256_cur_pat_h;

        /*
         *  source pointer for stipple bits
         */
        src_bits_p = (BITS16 *)v256_cur_pat + current_pat_y ;

        /*
         * stipple all lines of the source stipple onto destination
         */
        for ( j = 0,
				extra_start = current_pat_y,
				extra_end = current_pat_y + extra_lines,
				current_line_no = current_pat_y;
			 ((j < v256_cur_pat_h) && (j < dst_rect_height)); 
			 j++,current_line_no++)
        {
			/*
			 * number of times the current stpl line will
			 * appear in the destination region
			 */
			int		numlines;

			/*
			 * compute numlines
			 */
			numlines = exact_stipples;
			if(	(extra_lines) && 
				(current_line_no < extra_end) &&
				(current_line_no >= extra_start))
			{
				numlines++;
			}


			/*
			 * enough fooling around - do the drawing 
			 */

			v256_do_stipple_line(
								src_bits_p,
			/*stpl width*/		(int)16,
								(int)(v256_cur_pat_h),
								(BYTE *)dst_rect_ptr,
								x_top_left,
								x_bottom_right,
								(int)numlines
								);

            /*
             * advance to next line in source and dest pixmaps
             */
            src_bits_p++;
			dst_rect_ptr += v256_slbytes;

            /*
             * check for overflow
             */
            if ( ++current_pat_y >= v256_cur_pat_h)
            {
                current_pat_y = 0;
                src_bits_p = (BITS16 *) v256_cur_pat;
            }
		}	

	} 	/* for ( i = 0; i < num_subrects; i++)	*/
	return (SI_SUCCEED);
}

/*
 * this is from MIT's cfb server
 */
int
cfb8SetStipple (alu, fg, planemask)
int     alu;
unsigned long   fg, planemask;
{
    unsigned long   and, xor, rrop;
    int s;
    unsigned long   c;

    cfb8StippleMode = SGStipple;
    cfb8StippleAlu = alu;
    cfb8StippleFg = fg & PMSK;
    rrop = cfbReduceRasterOp (alu, fg, planemask, &and, &xor);
    cfb8StippleRRop = rrop;
    /*
     * create the appropriate pixel-fill bits for current
     * foreground
     */
    for (s = 0; s < 16; s++)
    {
		c = cfb8StippleMasks[s];
		cfb8StippleAnd[s] = and | ~c;
		cfb8StippleXor[s] = xor & c;
    }
}

/*
 * this is from MIT's cfb server
 */
int
cfb8SetOpaqueStipple (alu, fg, bg, planemask)
int     alu;
unsigned long   fg, bg, planemask;
{
    unsigned long   andfg, xorfg, andbg, xorbg, rropfg, rropbg;
    int s;
    unsigned long   c;

    cfb8StippleMode = SGOPQStipple;
    cfb8StippleAlu = alu;
    cfb8StippleFg = fg & PMSK;
    cfb8StippleBg = bg & PMSK;
    cfb8StipplePm = planemask & PMSK;
    rropfg = cfbReduceRasterOp (alu, cfb8StippleFg, cfb8StipplePm, 
								&andfg, &xorfg);
    rropbg = cfbReduceRasterOp (alu, cfb8StippleBg, cfb8StipplePm, 
								&andbg, &xorbg);
    if (rropfg == rropbg)
		cfb8StippleRRop = rropfg;
    else
		cfb8StippleRRop = GXset;
    /*
     * create the appropriate pixel-fill bits for current
     * foreground
     */
    for (s = 0; s < 16; s++)
    {
		c = cfb8StippleMasks[s];
		cfb8StippleAnd[s] = (andfg | ~c) & (andbg | c);
		cfb8StippleXor[s] = (xorfg & c) | (xorbg & ~c);
    }
}

/*
 *	This function takes one stipple line and one rectangle on screen and
 *	stipples the source line into all the lines of the destination where 
 *	the source stipple line will fit. It assumes that select page has 
 *	already been done. This works for stipple widths which are a power of 2
 *	only since this is used for stippling what we have catagorized as 
 *	small stipples
 */

#define 	MAX_SRC_NIBBLES		512
void
v256_do_stipple_line(
					BITS16	*src_bits_p,
					int		stplwidth,
					int		cur_pat_h,
					BYTE 	*dst_rect_ptr,
					int		x_top_left,
					int		x_bottom_right,
					int		numlines
)
{
	/*
	 * startmask and endmask for the first and last long words
	 */
	unsigned long	startmask,endmask;

	/*
	 * number of middle words
	 */
	int				nlwmiddle;

	/*
	 * 	destination stride : no of bytes to increment to get to a line on the
	 * 	screen which will take the same line of source stipple
	 */
	int				dst_stride;

	/*
	 * the array of nibbles in the source stipple
	 * This var is to be eliminated later and the cfb macros are to be 
	 * used
	 * total nibbles in source stipple, 
	 * nibble corresponding to start x location,
	 * current nibble of stipple 
	 */
	int				src_nibbles[MAX_SRC_NIBBLES];
	int				max_src_nibbles;
	unsigned int	start_src_nibble;
	unsigned int	current_src_nibble;

	/*
	 * temporary variables
	 */
	int				i,j,k;

	/*
	 * floor of pixel address to nearest longword boundary
	 */
	int				aligned_start,aligned_end;

	/*
	 * pointer to current longword in the destination
	 */
	unsigned long	*current_dst_p; 

	/*
	 * compute the src_nibbles array
	 */
	max_src_nibbles = stplwidth/4; 
	for ( i = 0,j = 0; i < max_src_nibbles ; i += 2,j++)
	{
		BYTE 	*tmp;  
		
		tmp = (BYTE *)src_bits_p + j;
		src_nibbles[i] = *tmp & 0x0F; 
		src_nibbles[i+1] = (unsigned)(*tmp & 0xF0) >> (unsigned)4; 
	}

	/*
	 * decrement max_src_nibbles because the same variable will be
	 * used later to do a modulo opereation( value mod max_src_nibbles)
	 */
	max_src_nibbles--; 

	/*
	 * compute startmask,endmask and middlewords
	 */
	startmask = v256_stipple_start_mask[x_top_left & 0x03];
	endmask = v256_stipple_end_mask[x_bottom_right & 0x03];

	/*
	 * compute the number of middlewords
	 */
	aligned_start = x_top_left & ~0x03;
	aligned_end = x_bottom_right & ~0x03;
	switch(aligned_end - aligned_start)
	{
		/*
		 * only one nibble to stipple
		 * convention : no startmask, only endmask
		 */
		case 0:
				endmask &= startmask; 
				startmask = 0;
				nlwmiddle = 0;
				break;

		/*
		 * two nibbles to stipple , no middle bytes
		 * leave startmask and endmask alone
		 */
		case 4:
				nlwmiddle = 0;
				break;

		/*
		 * complete case with startmask,endmask and some middle bytes
		 * leave startmask and endmask alone
		 */
		default:
				nlwmiddle = (unsigned)(aligned_end - aligned_start - 4) 
									>> 2;
				break;
	}


	/*
	 * compute the source nibble corresponding to first pixel of destination
	 * compute the destination stride
	 */
	start_src_nibble = (unsigned)((stplwidth - 1) & x_top_left) >>(unsigned) 2;
	dst_stride = cur_pat_h * v256_slbytes;

	/*
	 * trim the destination pointer to previous longword
	 */
	j = (int)dst_rect_ptr;
	j &= ~0x03;
	dst_rect_ptr = (BYTE *)j;

	if (v256_gs->stp_mode == SGStipple)
	{
		/*
		 * transparent stippling
		 */
		for ( i = 0; i < numlines; i++)
		{
			/*
			 * assign the vars that are modified in the loop
			 */
			current_dst_p = (unsigned long *)dst_rect_ptr;
			current_src_nibble = start_src_nibble;
			j = nlwmiddle;
			
			if ( cfb8StippleRRop != GXcopy )
			{
				if (startmask)
				{
					k = src_nibbles[current_src_nibble++];
					*current_dst_p = 
							MaskRRopPixels(*current_dst_p, k, startmask);
					current_dst_p++;
					current_src_nibble &= max_src_nibbles;
				}

				/*
				 * middle long words
				 */
				while (j--)
				{
					k = src_nibbles[current_src_nibble++];
					RRopFourBits(current_dst_p, k);
					current_dst_p++;
					current_src_nibble &= max_src_nibbles;
				}

				/*
				 * follow convention : endmask always present
				 */
				k = src_nibbles[current_src_nibble];
				*current_dst_p = 
							MaskRRopPixels(*current_dst_p, k, endmask);
			}
			else
			{
				unsigned long		mask;
				/*
				 * GXcopy operation
				 */
				if (startmask)
				{
					k = src_nibbles[current_src_nibble++];
					mask = cfb8PixelMasks[k];
					*current_dst_p = 
							(*current_dst_p & ~(mask & startmask)) |
							(V256Xor & (mask & startmask));
					current_dst_p++;
					current_src_nibble &= max_src_nibbles;
				}

				/*
				 * middle long words
				 */
				while (j--)
				{
					k = src_nibbles[current_src_nibble++];
					WriteFourBits (current_dst_p,V256Xor,k)
					current_dst_p++;
					current_src_nibble &= max_src_nibbles;
				}

				/*
				 * follow convention : endmask always present
				 */
				k = src_nibbles[current_src_nibble];
				mask = cfb8PixelMasks[k];
				*current_dst_p = 
						(*current_dst_p & ~(mask & endmask)) |
						(V256Xor & (mask & endmask));
			}

			/*
			 * go to the line of the destination which will take the same
			 * source stipple line
			 */
			dst_rect_ptr += dst_stride;
		}
	}
	else
	{
		/*
		 * opaque stippling
		 */

		assert(numlines >= 0);

		for ( i = 0; i < numlines; i++)
		{
			/*
			 * assign the vars that are modified in the loop
			 */
			current_dst_p = (unsigned long *)dst_rect_ptr;
			current_src_nibble = start_src_nibble;
			j = nlwmiddle;
			
			if ( cfb8StippleRRop != GXcopy )
			{
				if (startmask)
				{
					k = src_nibbles[current_src_nibble++];
					*current_dst_p = 
						MaskRRopPixels(*current_dst_p, k, startmask);
					current_dst_p++;
					current_src_nibble &= max_src_nibbles;
				}

				/*
				 * middle long words
				 */
				while (j--)
				{
					k = src_nibbles[current_src_nibble++];
					RRopFourBits(current_dst_p, k);
					current_dst_p++;
					current_src_nibble &= max_src_nibbles;
				}

				/*
				 * follow convention : endmask always present
				 */
				k = src_nibbles[current_src_nibble];
				*current_dst_p = 
						MaskRRopPixels(*current_dst_p, k, endmask);
			}
			else
			{
				/*
				 * GXcopy operation
				 */
				if (startmask)
				{
					k = src_nibbles[current_src_nibble++];
					*current_dst_p = *current_dst_p & ~startmask |
						GetFourPixels (k) & startmask;
					current_dst_p++;
					current_src_nibble &= max_src_nibbles;
				}

				/*
				 * middle long words
				 */
				while (j--)
				{
					k = src_nibbles[current_src_nibble++];
					*current_dst_p = GetFourPixels(k);
					current_dst_p++;
					current_src_nibble &= max_src_nibbles;
				}

				/*
				 * follow convention : endmask always present
				 */
				k = src_nibbles[current_src_nibble];
				*current_dst_p = *current_dst_p & ~endmask |
								GetFourPixels (k) & endmask;
			}

			/*
			 * go to the line of the destination which will take the same
			 * source stipple line
			 */
			dst_rect_ptr += dst_stride;
		}
	}
}
