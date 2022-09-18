#ident	"@(#)xpr:devices/terminfo/image_cells.c	1.3"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "string.h"

#include "Xlib.h"

#include "xpr.h"

#include "xpr_term.h"

static unsigned char	*bytes,
			*set_mask,
			*pin_bit[2];

static int		offset,
			rheight,
			*pin_byte,
			bytes_per_cell,
			*npreal;

/**
 ** image_to_cells()
 **/

void			image_to_cells (image, x, y, width, height, wscale, hscale)
	XImage			*image;
	int			x,
				y,
				width,
				height,
				wscale,
				hscale;	/* ignored */
{
	int			data_offset,
				data_skip,
				residual,
				depth,
				pass,
				plane_area;

	char			*pd;


	/*
	 * Construct and put out rows of ``cells'' for Type 1 and
	 * Type 2 devices (Type == "bimodel").
	 *
	 * For these devices, "porder" specifies the order of the
	 * ``pins'' in the ``print-head'', relative to the bits
	 * in each byte of image data. "npins" specifies the size
	 * of the print-head, in pins.
	 */


	/*
	 * "bytes" will hold a row of cells.
	 */
	bytes = umalloc(width * bytes_per_cell);


	/*
	 * We only care about the delta height;
	 */
	height -= y;

	/*
	 * Calculate the number of pins that will get real data
	 * at the bottom of the image. For multi-pass devices, the
	 * value may differ for each pass.
	 */
	residual = height - (height / rheight) * rheight;
	if (!residual)
		for (pass = 0; pass < bitwin; pass++)
			npreal[pass] = npins;
	else
		for (pass = 0; pass < bitwin; pass++)
			npreal[pass] = (residual - pass) / bitwin;

	/*
	 * Take care of the lion's share of the x-offset by an offset
	 * in the data pointer. The residual offset has to be done with
	 * bit shifts.
	 */
	data_offset = (x / WORDSIZE) * sizeof(Word);
	x -= data_offset * 8;

	data_skip = bitwin * image->bytes_per_line;

	plane_area = PLANE_AREA(image);

	/*
	 * Step through the image one cell row at a time.
	 * Start with the first bit-plane that has a color.
	 * (the number of planes is a multiple of 4, so may
	 * be more than the number of colors. Include black
	 * as a ``color''!)
	 */
	for (
		pd = image->data
			+ data_offset
			+ y * image->bytes_per_line
			+ plane_area * (image->depth - (colors + 1));
		height > 0;  /* may never become exactly == 0 */
		pd += rheight * image->bytes_per_line, height -= rheight
	) {
	    /*
	     * Map the image rectangle that is "rheight" tall and
	     * "width" wide. The upper left corner of this rectangle
	     * moves down "rheight" the next time we get here.
	     * The height of the rectangle may be reduced at the
	     * bottom of the image, if there are not enough rows left.
	     *
	     * "bitwin" (bit-image intertwining factor) gives the
	     * number of sparse rectangles that are overlayed. Each
	     * sparse rectangle has only "npins" rows filled, with
	     * "bitwin - 1" empty rows between each filled row. Each
	     * subsequent overlay of a sparse rectangle is moved down
	     * by the device 1/"bitwin" of the distance between pins.
	     * This allows the device higher vertical resolution
	     * than the inter-pin spacing allows in one pass.
	     */
	    for (pass = 0; pass < bitwin; pass++) {
		register unsigned char	*pb;

		register int		_x,
					_width	= width;

		char			*_pd;

		int			_npins	=
				(height >= rheight)? npins : npreal[pass],
					pin;


		/*
		 * For colors built up from more than one pass,
		 * we need to print light colors before darker colors.
		 * This avoids staining the ribbon. We built the pixel
		 * values such that a darker color is in a less
		 * significant bit than a lighter color (e.g. black is
		 * the 0th bit). HOWEVER, the bits are presented in
		 * MSBFirst format as we step forward through the
		 * bit-planes. Thus, walking forward through the planes
		 * accesses colors in the right order for printing,
		 * we just have to be careful how we number the color.
		 * Thus the reverse order on "depth".
		 */
		for (
			_pd = pd, depth = colors;
			depth >= 0;
			_pd += plane_area, depth--
		) {
		    char		    *__pd;

		    int			    empty_row = 1;


		    /*
		     * Initialize the output bytes with the constant
		     * mask of on-bits.
		     */
		    {
			register int		b;


			pb = bytes;
			for (_x = x; _x < _width; _x++)
				for (b = 0; b < bytes_per_cell; b++)
					*pb++ = set_mask[b];
		    }

		    /*
		     * Map a sparse rectangle.
		     *
		     * Note: This compound loop could be turned
		     * inside-out (loop across the page then down
		     * the pins) but the C code appears more efficient
		     * this way. Here we have to compute the mask
		     * inside the inner loop, so each mask is computed
		     * "npins" times. Turning the loop inside-out
		     * would have it computed once, but then "ink",
		     * "noink" and the byte offset (from "pin_byte[]")
		     * would have to be fetched inside the inner loop.
		     */
		    for (
			pin = 0, __pd = _pd + pass * image->bytes_per_line;
			pin < _npins;
			pin++, __pd += data_skip
		    ) {
			/*
			 * "__pd" must point to a Word aligned position
			 * for this to work. "___pd" is incremented by
			 * left-shifting a single bit in "m", starting
			 * at 1 and shifting until the bit is shifted
			 * out ("mask" becomes zero); when the bit is
			 * shifted out, "___pd" is incremented. "mask",
			 * not surprisingly, is the mask used to select
			 * the bit/pixel.
			 *
			 * "empty_row" is cleared if we think we need to
			 * apply ink in this row. We take a fast hack
			 * approach and check an entire set of bits
			 * in one shot. THIS MAY BE WRONG if stray bits
			 * (ones not to be printed) are in the word,
			 * but the consequence is not major.
			 */
			register Word		*___pd	= (Word *)__pd,
						mask	= 0x1 << x;

			register unsigned char	ink	= pin_bit[1][pin],
						noink	= pin_bit[0][pin];

			register int		_bytes_per_cell
							= bytes_per_cell;


			if (*___pd)
				empty_row = 0;
			pb = bytes + pin_byte[pin];
			for (_x = x; _x < _width; _x++) {
				*pb |= ((*___pd & mask)? ink : noink);
				pb += _bytes_per_cell;
				if (!(mask <<= 1)) {
					___pd++;
					mask = 0x1;
					if (*___pd && _x + 1 < width)
						empty_row = 0;
				}
			}
		    }

		    /*
		     * If we're at the bottom of the image there may be
		     * pin-rows left that should be cleared.
		     */
		    for (pin = _npins; pin < npins; pin++) {
			register unsigned char	noink	= pin_bit[0][pin];


			pb = bytes + pin_byte[pin];
			for (_x = x; _x < _width; _x++) {
				*pb |= noink;
				pb += bytes_per_cell;
			}
		    }

		    /*
		     * Add in the constant offset to each byte.
		     */
		    if (offset) {
			register int		_offset	= offset,
						_bytes_per_cell
							= bytes_per_cell,
						b;


			pb = bytes;
			for (_x = x; _x < _width; _x++)
				for (b = 0; b < _bytes_per_cell; b++)
					*pb++ += _offset;
		    }

		    /*
		     * Put out the cell-bytes. "output_units()" will
		     * return true if anything was put out. We don't
		     * need a ``carriage return'' if nothing was put
		     * or if this is the last depth-pass for this row.
		     */
		    if (
			output_units(
				depth,
				bytes,
				bytes_per_cell,
				width,
				wscale,
				empty_row
			)
		     && depth
		    )
			output_cr ();
		}
		output_nel ();
	    }
	}

	free ((char *)bytes);
	return;
}

/**
 ** init_image_to_cells()
 **/

/*
 * This routine sets up any local variables needed by the
 * "image_to_cells()" routine, and establishes the image data structure
 * needed (i.e. XYBitmap, XYPixmap, depth, bit and byte order).
 *
 * WARNING: Currently the "image_bitblt()" routine only works
 * with the XYBitmap format; the ZPixmap format with depth of 1;
 * or the XYPixmap format, any depth (but copying into the last plane).
 * ALSO only with a ``native'' format, with WORDSIZE bits per unit.
 * If you need other formats you will have to update the "image_bitblt()"
 * routine.
 */

void			init_image_to_cells (
				dst_image,
				p_dst_ncolors,
				p_dst_colors,
				src_image,
				src_ncolors,
				src_colors,
				color_list,
				p_invert_text
			)
	XImage			*dst_image;
	int			*p_dst_ncolors;
	XColor			**p_dst_colors;
	XImage			*src_image;
	int			src_ncolors;
	XColor			*src_colors;
	char			*color_list;
	int			*p_invert_text;
{
	*p_invert_text = 0;

	offset = parse_porder(&set_mask, pin_bit, &pin_byte, &bytes_per_cell);
	npreal = icalloc(bitwin);
	rheight = bitwin * npins;

	if (!isbitmap(src_image))
		map_colors (p_dst_ncolors, p_dst_colors, color_list);
	if (!isbitmap(src_image) && *p_dst_ncolors) {
		dst_image->format = XYPixmap;
		if (*p_dst_ncolors <= 2) {
			dst_image->depth = 1;
			colors = 0;
		} else
			/*
			 * There are "colors" primary colors plus black.
			 * Unlike on video displays, white is not made
			 * up from equal amounts of all colors, but is
			 * an absence of ink. And, black is not an absence
			 * of color, but the presence of an ink. Thus,
			 * the depth of the printed image is the total
			 * number of colors including black. We round
			 * up to a multiple of 4, since the X code only
			 * supports that for image depths.
			 */
			dst_image->depth = 4 * ((colors + 1 + 3) / 4);
	} else {
		dst_image->format = XYBitmap;
		dst_image->depth = 1;
		colors = 0;
	}

	dst_image->bits_per_pixel = dst_image->depth;

	dst_image->byte_order = endian();
	dst_image->bitmap_bit_order = LSBFirst;
	dst_image->bitmap_unit = dst_image->bitmap_pad = WORDSIZE;

	return;
}
