#ident	"@(#)xpr:convert.c	1.3"
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

extern int		_XInitImageFuncPtrs();

static void		landscape(),
			portrait();

/**
 ** pix_convert() - CONVERT PIXEL IMAGE TO EASIER FORM
 **/

void			pix_convert (
				src_image,
				src_ncolors,
				src_colors,
				dst_image,
				dst_ncolors,
				dst_colors,
				rotate,
				invert,
				w_scale,
				h_scale
			)
	XImage			*src_image,
				*dst_image;
	int			src_ncolors,
				dst_ncolors;
	XColor			*src_colors,
				*dst_colors;
	int			rotate,
				invert,
				w_scale,
				h_scale;
{
	if (rotate) {
		dst_image->width = src_image->height * w_scale;
		dst_image->height = src_image->width * h_scale;
	} else {
		dst_image->width = src_image->width * w_scale;
		dst_image->height = src_image->height * h_scale;
	}
	dst_image->xoffset = 0;

	dst_image->bytes_per_line = BYTES_PER_LINE(dst_image);
	dst_image->data = Malloc(IMAGE_SIZE(dst_image));


	/*
	 * The following are identical image data structures:
	 *
	 *	- XYBitmap
	 *	- XYPixmap, depth 1
	 *	- ZPixmap, bits_per_pixel 1
	 *
	 * If each of the source and destination images are one
	 * of these structures, change the source structure to match
	 * the destination structure, to minimize wasted data conversions.
	 *
	 * Note: The last two cases still imply colors, but a binary
	 * set. Nonetheless the bits are stored the same in all three
	 * cases. HOWEVER, the mapping of source colors into textures,
	 * device colors, or gray-scales is simplified: If the mapping
	 * is such that 0 pixels map into 0 pixels and 1 into 1, then
	 * the mapping is the identity mapping; if the mapping is such
	 * that 0 pixels map into 1 pixels and vice versa, then we can
	 * achieve this by simply inverting the bits after converting.
	 */
	if (isbitmap(dst_image) && isbitmap(src_image)) {
		src_image->format = dst_image->format;
		src_image->depth = dst_image->depth;
		src_image->bits_per_pixel = dst_image->bits_per_pixel;

		/*
		 * Note: White is more intense than black, so we
		 * flip the bits if the 0th color is less intense
		 * than the 1st color (i.e. if a pixel of 0 is
		 * ``black'') AND we're mapping to a black and white
		 * printer or it's a color printer and a output pixel
		 * of 0 is more intense than an output pixel of 1.
		 */
/*
 * MORE: It is not guaranteed that elements 0 and 1 correspond to
 * pixel values 0 and 1, so the color maps should be searched!
 */
		if (
			src_ncolors >= 2
		     && color_cmp(&src_colors[0], &src_colors[1]) < 0
		     && !(
			    dst_ncolors >= 2
		         && color_cmp(&dst_colors[0], &dst_colors[1]) < 0
			)
		)
			invert = !invert;
		src_ncolors = 0;	/* skip later color mapping */
	}

	_XInitImageFuncPtrs (dst_image);

/* #define IMPROVE_IMAGE */
#if	defined(IMPROVE_IMAGE)
{
	XImage			tmp;

	/*
	 * Copy the data bytes to a temporary structure using the
	 * "portrait()" routine. The data location has room for the
	 * enlargement.
	 */
	tmp = *src_image;
	tmp.width = src_image->width * w_scale;
	tmp.height = src_image->height * h_scale;
	tmp.xoffset = 0;
	tmp.bytes_per_line = BYTES_PER_LINE(&tmp);
	tmp.data = Malloc(
		  (tmp.format == XYPixmap? tmp.depth : 1)
		* tmp.height * tmp.bytes_per_line
	);
	portrait (src_image, &tmp, 0, (XColor *)0);

	enlarge (&tmp, w_scale, h_scale);

	if (rotate)
		landscape (
			&tmp,
			src_ncolors,
			src_colors,
			dst_image,
			dst_ncolors,
			dst_colors
		);
	else
		portrait (
			&tmp,
			src_ncolors,
			src_colors,
			dst_image,
			dst_ncolors,
			dst_colors
		);

	if (invert)
		bit_invert (dst_image);
}

#else
	/*
	 * It is faster (and the code is simpler!) to first convert
	 * the image without scaling, then scale the result.
	 */

	if (rotate)
		landscape (
			src_image,
			src_ncolors,
			src_colors,
			dst_image,
			dst_ncolors,
			dst_colors
		);
	else
		portrait (
			src_image,
			src_ncolors,
			src_colors,
			dst_image,
			dst_ncolors,
			dst_colors
		);

	if (invert)
		bit_invert (dst_image);

	enlarge (dst_image, w_scale, h_scale);
#endif

	return;
}

/**
 ** landscape() - CONVERT AND ROTATE IMAGE
 **/

static void		landscape (
				src,
				src_ncolors,
				src_colors,
				dst,
				dst_ncolors,
				dst_colors
			)
	XImage			*src,
				*dst;
	int			src_ncolors,
				dst_ncolors;
	XColor			*src_colors,
				*dst_colors;
{
	int			x,
				y,
				_x;


	/*
	 * MORE: Do this faster!
	 */

	/*
	 * Note: The "dst->width" and "dst->height" values have
	 * been scaled, so be careful in using them.
	 */

	/*
	 * If the source image has color but the output device doesn't,
	 * use textures to simulate gray-scales and map the colors to
	 * them.
	 */
	if (src_ncolors && (isbitmap(dst) || !dst_ncolors)) {
		char			**texture	=
				color_to_bw(src, src_ncolors, src_colors);


		for (x = 0, _x = src->width - 1; x < src->width; x++, _x--)
			for (y = 0; y < src->height; y++)
				XPutPixel (
					dst,
					y,
					_x,
		(long)TEXTURE_BIT(texture[XGetPixel(src, x, y)], _x, y, 4)
				);
		free (texture);
		return;
	}

	/*
	 * If the output device has color, map the colors as best
	 * as possible.
	 */
	if (!isbitmap(dst) && dst_ncolors) {
		long			*lookup	= color_to_color(
						src,
						src_ncolors,
						src_colors,
						dst,
						dst_ncolors,
						dst_colors
					);


		for (x = 0, _x = src->width - 1; x < src->width; x++, _x--)
			for (y = 0; y < src->height; y++)
				XPutPixel (
					dst,
					y,
					_x,
					lookup[XGetPixel(src, x, y)]
				);
		free ((char *)lookup);
		return;
	}

	/*
	 * The source is hopeless. Just convert the pixels
	 * and see what we get.
	 */
	for (x = 0, _x = src->width - 1; x < src->width; x++, _x--)
		for (y = 0; y < src->height; y++)
			XPutPixel (dst, y, _x, XGetPixel(src, x, y));

	return;
}

/**
 ** portrait() - CONVERT AND ROTATE IMAGE
 **/

static void		portrait (
				src,
				src_ncolors,
				src_colors,
				dst,
				dst_ncolors,
				dst_colors
			)
	XImage			*src,
				*dst;
	int			src_ncolors,
				dst_ncolors;
	XColor			*src_colors,
				*dst_colors;
{
	int			x,
				y;


	/*
	 * MORE: Do this faster!
	 */

	/*
	 * Note: The "dst->width" and "dst->height" values have
	 * been scaled, so be careful in using them.
	 */

	/*
	 * If the images structures differ only in the image storage
	 * (byte order, bit order, bitmap_unit, but NOT depth)
	 * this is a piece of cake.
	 */
	if (
		src->format == dst->format
	     && src->depth == dst->depth
	     && src->bits_per_pixel == dst->bits_per_pixel
	     && src->xoffset == dst->xoffset
	     && (!dst_ncolors || dst_colors == src_colors)
	) {
		swap_copy (src, dst);
		return;
	}


	/*
	 * If the source image has color but the output device doesn't,
	 * use textures to simulate gray-scales and map the colors to
	 * them.
	 */
	if (src_ncolors && (isbitmap(dst) || !dst_ncolors)) {
		char			**texture	=
				color_to_bw(src, src_ncolors, src_colors);


		for (y = 0; y < src->height; y++)
			for (x = 0; x < src->width; x++)
				XPutPixel (
					dst,
					x,
					y,
		(long)TEXTURE_BIT(texture[XGetPixel(src, x, y)], x, y, 4)
				);
		free (texture);
		return;
	}

	/*
	 * If the output device has color, map the colors as best
	 * as possible.
	 */
	if (!isbitmap(dst) && dst_ncolors) {
		long			*lookup	= color_to_color(
						src,
						src_ncolors,
						src_colors,
						dst,
						dst_ncolors,
						dst_colors
					);


		for (y = 0; y < src->height; y++)
			for (x = 0; x < src->width; x++)
				XPutPixel (
					dst,
					x,
					y,
					lookup[XGetPixel(src, x, y)]
				);
		free ((char *)lookup);
		return;
	}

	/*
	 * The source is hopeless. Just convert the pixels
	 * and see what we get.
	 */
	for (y = 0; y < src->height; y++)
		for (x = 0; x < src->width; x++)
			XPutPixel (dst, x, y, XGetPixel(src, x, y));

	return;
}

/**
 ** convert_longs() - CONVERT TYPE long TO NATIVE FORM
 ** convert_shorts() - CONVERT TYPE short TO NATIVE FORM
 **/

void			convert_longs (pl, nl)
	unsigned long		*pl;
	int			nl;
{
	swap4 ((char *)pl, nl);
	return;
}

void			convert_shorts (ps, ns)
	unsigned short		*ps;
	int			ns;
{
	swap2 ((char *)ps, ns);
	return;
}

/**
 ** endian() - DETERMINE BYTE ORDER OF NATIVE MACHINE
 **/

int			endian ()
{
	static long		one		= 1;

#define LSB	*(unsigned char *)&one

	return (LSB? LSBFirst : MSBFirst);
}
