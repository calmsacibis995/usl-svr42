#ident	"@(#)xpr:devices/postscript/dump_image.c	1.3"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "stdio.h"
#include "string.h"

#include "Xlib.h"

#include "xpr.h"

/**
 ** dump_ps_image()
 **/

void			dump_ps_image (image, x, y, width, height)
	XImage *		image;
	int			x;
	int			y;
	int			width;
	int			height;
{
	char *			pd;

	int			data_offset;
	int			units;


	/*
	 * We only care about the delta height;
	 */
	height -= y;

	/*
	 * Take care of the lion's share of the x-offset by an offset
	 * in the data pointer. The residual offset has to be done with
	 * bit shifts.
	 */
	data_offset = (x * image->bits_per_pixel) / 8;
	x -= (data_offset * 8) / image->bits_per_pixel;
	width -= (data_offset * 8) / image->bits_per_pixel;

#define PIXELS_PER_BYTE	(8 / image->bits_per_pixel)
	units = (width + PIXELS_PER_BYTE - 1) / PIXELS_PER_BYTE;

	for (
		pd = image->data + data_offset + y * image->bytes_per_line;
		height;
		pd += image->bytes_per_line, height--
	) {
		ps_output_bytes (pd, units);
	}

	return;
}

/**
 ** init_dump_ps_image()
 **/

void			init_dump_ps_image (
				dst_image,
				p_dst_ncolors,
				p_dst_colors,
				src_image,
				src_ncolors,
				src_colors,
				color_list
			)
	XImage *		dst_image;
	int *			p_dst_ncolors;
	XColor **		p_dst_colors;
	XImage *		src_image;
	int			src_ncolors;
	XColor *		src_colors;
	char *			color_list;
{
	*p_dst_ncolors = src_ncolors;
	*p_dst_colors = src_colors;

	dst_image->format = ZPixmap;

	if (src_image->depth > 8 || src_image->bits_per_pixel > 8) {
		dst_image->depth =
		dst_image->bits_per_pixel = 8;
	} else if (src_image->format == ZPixmap) {
		dst_image->depth =
		dst_image->bits_per_pixel = src_image->bits_per_pixel;
	} else {
		dst_image->depth =
		dst_image->bits_per_pixel = src_image->depth;
	}

	dst_image->byte_order = endian();

	/*
	 * MORE: Are these necessary?
	 */
	dst_image->bitmap_bit_order = LSBFirst;
	dst_image->bitmap_unit = dst_image->bitmap_pad = WORDSIZE;

	return;
}
