/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/v256spreq.c	1.3"

/*
 *	Copyright (c) 1992 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/*
 *	FILE : v256spreq.c
 *
 *	MODULE : vga256
 *
 *	DESCRIPTION :
 *
 *	NOTES :
 *
 *	CAVEATS :
 *		This function assumes that one scan line is always less than
 *	64 K in size.
 *	
 *	SEE ALSO :
 *	
 *	HISTORY :
 *
 */

/*
#define		SPLIT_REQ_DEBUG 	1
*/
#ifdef		SPLIT_REQ_DEBUG	
#include	<stdio.h>
#define		NDEBUG		1
#endif	/* SPLIT_REQ_DEBUG */

#include	<assert.h>

/*
 *	Include the required typedefs
 */
#include	<sys/types.h>
#include	<sys/kd.h>
#include	"sidep.h"
#include	"v256.h"
#include	"vtio.h"
#include 	"v256spreq.h"


/*
 *	The #bytes per scan line will vary according to the mode
 */
#define	VGA_SCREEN_LENGTH	v256_slbytes

#ifndef	VGA_PAGE_SIZE
#error	VGA_PAGE_SIZE is undefined
#endif	/* VGA_PAGE_SIZE */


#define	PAGE_INFO(n,linesize) \
	{ \
		( (VGA_PAGE_SIZE * (n)) % (linesize) ),\
		( (VGA_PAGE_SIZE * (n)) / (linesize) ),\
		( VGA_PAGE_SIZE * (n) )	\
	}


#define	TABLE_SIZE(table)	\
	(sizeof(table)/sizeof(table[0]))

/*
 *	The table for standard vga 640 x 480
 */

static const struct _v256_page_info v256_640x480_page_table[] =
{
	PAGE_INFO(0,640),
	PAGE_INFO(1,640),
	PAGE_INFO(2,640),
	PAGE_INFO(3,640),
	PAGE_INFO(4,640),
	PAGE_INFO(5,640),
	PAGE_INFO(6,640),
	PAGE_INFO(7,640),
	PAGE_INFO(8,640),
	PAGE_INFO(9,640),
	PAGE_INFO(10,640),
	PAGE_INFO(11,640),
	PAGE_INFO(12,640),
	PAGE_INFO(13,640),
	PAGE_INFO(14,640),
	PAGE_INFO(15,640),
	PAGE_INFO(16,640),
	PAGE_INFO(17,640),
	PAGE_INFO(18,640),
	PAGE_INFO(19,640),
	PAGE_INFO(20,640)
};

/*
 *	The table for 800 x 600
 */

static const struct _v256_page_info v256_800x600_page_table[] =
{
	PAGE_INFO(0,800),
	PAGE_INFO(1,800),
	PAGE_INFO(2,800),
	PAGE_INFO(3,800),
	PAGE_INFO(4,800),
	PAGE_INFO(5,800),
	PAGE_INFO(6,800),
	PAGE_INFO(7,800),
	PAGE_INFO(8,800),
	PAGE_INFO(9,800),
	PAGE_INFO(10,800),
	PAGE_INFO(11,800),
	PAGE_INFO(12,800),
	PAGE_INFO(13,800),
	PAGE_INFO(14,800),
	PAGE_INFO(15,800),
	PAGE_INFO(16,800),
	PAGE_INFO(17,800),
	PAGE_INFO(18,800),
	PAGE_INFO(19,800),
	PAGE_INFO(20,800)
};

static const struct _v256_page_info v256_1024x768_page_table[] =
{
	PAGE_INFO(0,1024),
	PAGE_INFO(1,1024),
	PAGE_INFO(2,1024),
	PAGE_INFO(3,1024),
	PAGE_INFO(4,1024),
	PAGE_INFO(5,1024),
	PAGE_INFO(6,1024),
	PAGE_INFO(7,1024),
	PAGE_INFO(8,1024),
	PAGE_INFO(9,1024),
	PAGE_INFO(10,1024),
	PAGE_INFO(11,1024),
	PAGE_INFO(12,1024),
	PAGE_INFO(13,1024),
	PAGE_INFO(14,1024),
	PAGE_INFO(15,1024),
	PAGE_INFO(16,1024),
	PAGE_INFO(17,1024),
	PAGE_INFO(18,1024),
	PAGE_INFO(19,1024),
	PAGE_INFO(20,1024)
};

/*
 * 	default : 800x600
 */
const struct	_v256_page_info *v256_page_table_info_p =
									v256_800x600_page_table;

/*
 *	Function : v256_setup_splitter
 *		Adjusts the page table pointer to the correct mode.
 *	Returns : 1 if successful, 0 if a bad mode was passed in
 */

int
v256_setup_splitter(int	x_res,  int	y_res, int vga_page_size)
{
	int	return_code;

	/*
	 *	We don't support non 64k pages yet
	 */
	if ( vga_page_size != 64 * 1024 & vga_page_size != 128 * 1024 )
	{
		return 0;
	}
	
	switch ( x_res )
	{
		case 640 :
			v256_page_table_info_p = v256_640x480_page_table;
			return_code = 1;
		break;

		case 800 :
			v256_page_table_info_p = v256_800x600_page_table;
			return_code = 1;
		break;

		case 1024 :
			v256_page_table_info_p = v256_1024x768_page_table;
			return_code = 1;
		break;
			
		default : 
			/*
			 *	Force a nice core dump if the return code is ignored
			 */
			v256_page_table_info_p = 0;
			return_code = 0;
		break;
	}

	return return_code;
}
/*
 *	Function : v256_split_request
 *		Given the top left and bottom right co-ordinates of a screen region
 *	this function will return the component rectangles such that each returned
 *	rectangle lies in one VGA page.
 */


int
v256_split_request(
	int	x_top_left, 
	int	y_top_left,
	int	x_bottom_right,
	int	y_bottom_right,
	int	*n_rects_p,
	VgaRegion *rects_p
	)
{
	int	x_left = x_top_left;
	int	y_region_start = y_top_left;
	int	x_right = x_bottom_right;
	
	const struct	_v256_page_info	*info_p = v256_page_table_info_p;
	const struct	_v256_page_info	*next_info_p;

	long	region_start = y_top_left * VGA_SCREEN_LENGTH + x_top_left;
	long	region_end = 
			y_bottom_right * VGA_SCREEN_LENGTH + x_bottom_right;

#ifdef	SPLIT_REQ_DEBUG
	VgaRegion	*orig_region_p = rects_p;
#endif	/* SPLIT_REQ_DEBUG */

	/*
	 *	reset count of rectangles passed up
	 */
	*n_rects_p = 0;

#ifdef	SPLIT_REQ_DEBUG
	fprintf(stderr, "\
v256_split_request :\n\
	x_top_left= %d, \n\
	y_top_left= %d,\n\
	x_bottom_right= %d,\n\
	y_bottom_right= %d\n",
		x_top_left, 
		y_top_left,
		x_bottom_right,
		y_bottom_right
	);

	/*
	 *	check that we have a valid region here
	 */
	assert(y_top_left <= y_bottom_right);
	assert(x_top_left <= x_bottom_right);

#else
	if (y_top_left > y_bottom_right || x_top_left > x_bottom_right)
	{
		return 0;
	}
#endif /* SPLIT_REQ_DEBUG */

	/*
	 *	skip till we find the page the top left corner lies in
	 */
	while ( info_p->page_offset <= region_start )
	{
		next_info_p = ++info_p; 
	}

	/*
	 *	step back once
	 */
	info_p --;



	/*
	 *	main loop
	 */
	while (next_info_p->page_offset <= region_end)
	{
		/*
		 *	We will be generating at least one region here
		 *	check and see which case it is in
		 */
		
		/*
		 *	CASE 1: x_next_page < x_left
		 *	The bottom scanline is actually in the next page
		 */
		if ( next_info_p->x_start < x_left )
		{
			rects_p->x = x_left;
			rects_p->y = y_region_start;
			rects_p->width = x_right - x_left + 1;
			rects_p->height = next_info_p->y_start - 
						y_region_start; 
			y_region_start = next_info_p->y_start;

			rects_p->region_p = (v256_fb + 
									(OFFSET(rects_p->x, rects_p->y) & VIDEO_PAGE_MASK));
			(*n_rects_p)++;
			rects_p ++;
		}

		/*
		 *	CASE 2: x_next_page > x_right
		 *	The bottom scanline is wholly in the current page
		 */
		 else if ( next_info_p->x_start > x_right )
		 {
			rects_p->x = x_left;
			rects_p->y = y_region_start;
			rects_p->width = x_right - x_left + 1;
			rects_p->height =  next_info_p->y_start - 
						y_region_start + 1;

			y_region_start = next_info_p->y_start + 1;

			rects_p->region_p = (v256_fb + 
									(OFFSET(rects_p->x, rects_p->y) & VIDEO_PAGE_MASK));
			(*n_rects_p) ++;
			rects_p ++;
		 }

		/*
		 *	CASE 3: x_next_page falls within the rectangle
		 *	We need to split the rectangle into 1 rect + 2 lines
		 */
		else if ( next_info_p->x_start >= x_left && 
				next_info_p->x_start <= x_right )
		{
			
			assert(next_info_p->y_start >= y_region_start );

			/*
			 *	The first big rectangle
			 */

			if ( next_info_p->y_start != y_region_start )
			{
			    rects_p->x = x_left;
			    rects_p->y = y_region_start;
			    rects_p->width = x_right - x_left + 1;
			    rects_p->height = next_info_p->y_start - 
						    y_region_start; 

				rects_p->region_p = (v256_fb + 
										(OFFSET(rects_p->x, rects_p->y) & VIDEO_PAGE_MASK));
			    (*n_rects_p) ++;
			    rects_p ++;
			}
			
			/*
			 *	a single width line excluding page beginning
			 */
			rects_p->x = x_left;
			rects_p->y = next_info_p->y_start;
			rects_p->width = next_info_p->x_start - x_left;
			rects_p->height =  1;

			rects_p->region_p = (v256_fb + 
									(OFFSET(rects_p->x, rects_p->y) & VIDEO_PAGE_MASK));
			/*
			 * we could have a zero width rectangle : ignore
			 */
			if ( rects_p->width > 0 )
			{
				(*n_rects_p) ++;
				rects_p ++;
			}
			
			/*
			 *	single scan line inclusive of the page
			 *	beginning.
			 */

			rects_p->x = next_info_p->x_start;
			rects_p->y = next_info_p->y_start;
			rects_p->width = x_right - next_info_p->x_start + 1;
			rects_p->height = 1;

			rects_p->region_p = (v256_fb + 
									(OFFSET(rects_p->x, rects_p->y) & VIDEO_PAGE_MASK));
			(*n_rects_p) ++;
			rects_p ++;


			/*
			 *	update the region start
			 */
			y_region_start = next_info_p->y_start + 1;

		}

		/*
		 *	CASE 4: Something is rotten in the state of the function
		 */
		else 
		{
			/*
			 *	should NEVER reach here
			 */
			assert(0);
		}

		info_p ++;
		next_info_p = info_p + 1;
	}

	 /*
	  *	Last rectangle : 
	  *	No complications as it HAS to lie in one VGA page.
	  */
	if ( info_p->page_offset <= region_end && y_bottom_right >= y_region_start )
	{
	    rects_p->x = x_left;
	    rects_p->y = y_region_start;
	    rects_p->width = x_right - x_left + 1;
	    rects_p->height = y_bottom_right - y_region_start + 1;

		rects_p->region_p = (v256_fb + 
								(OFFSET(rects_p->x, rects_p->y) & VIDEO_PAGE_MASK));
	    (*n_rects_p) ++;
	}
#ifdef	SPLIT_REQ_DEBUG
	{
		int 	i = *n_rects_p;
		VgaRegion *r_p = orig_region_p;

		for (i=0; i < *n_rects_p; i++,r_p++ )
		{
			fprintf(stderr, "\t\t** rect #%d\n", i);
			fprintf(stderr, "\t\tx= %d, y= %d, width= %d, height= %d,ptr=%d\n", 
				r_p->x,r_p->y,r_p->width,r_p->height,r_p->region_p);
		}
	}
#endif	/* SPLIT_REQ_DEBUG */
	return	1;
}


#ifdef 	SPLIT_REQ_DEBUG_1

void
main(void)
{
	int	x1, y1, x2, y2;
	struct	_vga_rect_region	rects[50];
	int	nrects;
	int	i;
/*
 *	read in x1, y1, x2, y2 from standard input and call the 
 *	splitter. Print split results.
 */
	printf("? \n");
	while ( scanf("%d %d %d %d\n", &x1, &y1, &x2, &y2 ) == 4 )
	{
		v256_split_request(x1, y1, x2, y2, &nrects, rects);
		for( i = 0; i < nrects; i ++)
		{
			printf("x = %d, y = %d\n", rects[i].x, rects[i].y );
			printf("width = %d, height = %d\n", rects[i].width, rects[i].height );
		}
		printf("? \n");
	}

}

#endif	/* SPLIT_REQ_DEBUG_1 */
