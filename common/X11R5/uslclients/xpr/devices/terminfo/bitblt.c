#ident	"@(#)xpr:devices/terminfo/bitblt.c	1.2"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "stdio.h"

#include "Xlib.h"
#include "Xutil.h"

#include "xpr.h"

#include "xpr_term.h"

static void		pixmap_bitblt(),
			bitmap_bitblt();

/**
 ** image_bitblt()
 **/

void			image_bitblt (
				src,
				x,
				y,
				width,
				height,
				dst,
				to_x,
				to_y
			)
	XImage			*src;
	int			x,
				y,
				width,
				height;
	XImage			*dst;
	int			to_x,
				to_y;
{
	/*
	 * WARNING: This is a crude approximation of a bitblt operator:
	 * 	- Only the copy (store) operation is supported.
	 *	- No checks are made to ensure that the destination
	 *	  bitmap has enough space (i.e., no clipping).
	 *	- No optimization for special cases.
	 *
	 * ALSO: The routine supports XYBitmap, ZPixmap of depth 1,
	 * and XYPixmap of any depth. Other formats are simulated
	 * using pixel-at-a-time routines; besides being slow the
	 * simulation may not do what is desired with color images.
	 */

	if (
		(isbitmap(src) || src->format == XYPixmap)
	     && (isbitmap(dst) || dst->format == XYPixmap)
	)
		bitmap_bitblt (src, x, y, width, height, dst, to_x, to_y);
	else	
		pixmap_bitblt (src, x, y, width, height, dst, to_x, to_y);

	return;
}

/**
 ** pixmap_bitblt() - BITBLT FOR HARD CASES
 **/

static void		pixmap_bitblt (
				src,
				x,
				y,
				width,
				height,
				dst,
				to_x,
				to_y
			)
	XImage			*src;
	int			x,
				y,
				width,
				height;
	XImage			*dst;
	int			to_x,
				to_y;
{
	static int		complained_yet	= 0;


	if (!complained_yet) {
		fprintf (
			stderr,
"xpr: Warning: Bitblt operator can't handle internal data structure.\n"
		);
		complained_yet = 1;
	}

	for (; height--; y++, to_y++) {
		int			_width	= width,
					_to_x	= to_x,
					_x	= x;


		for (; _width--; _to_x++, _x++)
			XPutPixel (dst, _to_x, to_y, XGetPixel(src, _x, y));
	}

	return;
}

/**
 ** bitmap_bitblt() - BITBLT FOR EASY CASES
 **/

static void		_bitblt();

static void		bitmap_bitblt (
				src,
				x,
				y,
				width,
				height,
				dst,
				to_x,
				to_y
			)
	XImage			*src;
	int			x,
				y,
				width,
				height;
	XImage			*dst;
	int			to_x,
				to_y;
{
	int			src_offset,
				dst_offset,
				nplanes,
				src_plane_area,
				dst_plane_area;

	char			*src_p,
				*dst_p;


	/*
	 * This is a VERY special case routine!
	 */
	if (
		src == dst
	     || src->bitmap_unit != WORDSIZE
	     || src->bitmap_pad != WORDSIZE
	     || dst->bitmap_unit != WORDSIZE
	     || dst->bitmap_pad != WORDSIZE
	     || src->byte_order != endian()
	     || dst->byte_order != endian()
	     || src->bitmap_bit_order != LSBFirst
	     || dst->bitmap_bit_order != LSBFirst
	) {
		pixmap_bitblt (src, x, y, width, height, dst, to_x, to_y);
		return;
	}


	/*
	 * Take care of the lion's share of each x,y shift using
	 * offsets in the data pointers. The residual offsets have to be
	 * done with bit shifts.
	 */

	src_offset = (x / WORDSIZE) * WORDSIZE;
	x -= src_offset;
	src_p = src->data + (src_offset / 8) + y * src->bytes_per_line;

	dst_offset = (to_x / WORDSIZE) * WORDSIZE;
	to_x -= dst_offset;
	dst_p = dst->data + (dst_offset / 8) + to_y * dst->bytes_per_line;


	/*
	 * Copy the lower planes only.
	 */
	nplanes = min(src->depth, dst->depth);


	src_plane_area = PLANE_AREA(src);
	dst_plane_area = PLANE_AREA(dst);

	for (
		src_p += (src->depth - nplanes) * src_plane_area,
			dst_p += (dst->depth - nplanes) * dst_plane_area;
		nplanes--;
		src_p += src_plane_area,
			dst_p += dst_plane_area
	)
		_bitblt (
			src_p,
			(8 * src->bytes_per_line) / WORDSIZE,
			x,
			width,
			height,
			dst_p,
			(8 * dst->bytes_per_line) / WORDSIZE,
			to_x
		);

	return;
}

static void		_bitblt (
				src_p,
				src_wpl,
				x,
				width,
				height,
				dst_p,
				dst_wpl,
				to_x
			)
	Word			*src_p;
	int			src_wpl,
				x,
				width,
				height;
	Word			*dst_p;
	int			dst_wpl,
				to_x;
{
	for (; height--; src_p += src_wpl, dst_p += dst_wpl) {
		register Word		*_src_p		= (Word *)src_p,
					src_mask	= 0x1 << x,
					*_dst_p		= (Word *)dst_p,
					dst_mask	= 0x1 << to_x;

		register int		_width		= width;


		while (_width--) {
			if (*_src_p & src_mask)
				*_dst_p |= dst_mask;
			else
				*_dst_p &= ~dst_mask;
			if (!(src_mask <<= 1)) {
				_src_p++;
				src_mask = 0x1;
			}
			if (!(dst_mask <<= 1)) {
				_dst_p++;
				dst_mask = 0x1;
			}
		}
	}

	return;
}
