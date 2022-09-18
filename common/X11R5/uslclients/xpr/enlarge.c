#ident	"@(#)xpr:enlarge.c	1.2"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "memory.h"

#include "Xlib.h"
#include "Xutil.h"

#include "xpr.h"

static void		w_stretch(),
			w_stretch_bitmap(),
			w_stretch_pixmap(),
			h_stretch();

/**
 ** enlarge() - ENLARGE IMAGE
 **/

void			enlarge (image, w_scale, h_scale)
	XImage			*image;
	int			w_scale,
				h_scale;
{
	int			old_width,
				old_height;


	/*
	 * We enlarge the image IN PLACE. This means that the
	 * caller is responsible for having allocated enough
	 * data space for the final image. (That way we avoid
	 * excess memory allocation.) This also means that
	 * overlapping data moves should be avoided!
	 */

	/*
	 * Enlarging vertically is alot faster than enlarging
	 * horizontally (typically). Thus we enlarge horizontally
	 * first, to avoid enlarging duplicate lines.
	 */
	old_width = image->width / w_scale;
	old_height = image->height / h_scale;
	if (w_scale > 1)
		w_stretch (image, old_width, old_height, w_scale);

	old_width = image->width;
	if (h_scale > 1)
		h_stretch (image, old_width, old_height, h_scale);

	return;
}

/**
 ** h_stretch() - ENLARGE VERTICALLY
 **/

static void		h_stretch (image, old_width, old_height, h_scale)
	XImage			*image;
	int			old_width,
				old_height,
				h_scale;
{
	int			old_bits_per_line,
				old_nbytes,
				y,
				new_y,
				s,
				nplanes;

	char			*bit_row;


	/*
	 * Note: This routine assumes zero x-offset.
	 */

	/*
	 * The vertical expansion is done by copying each image
	 * bit-row "h_scale" times. Since this copy is done in place,
	 * we have to copy from the bottom up.
	 */

	/*
	 * The image data space may be wider than we care about:
	 * the extra space reserved for expanding in width need not
	 * be copied. Note, though, that we do copy the bytes that
	 * pad to a bitmap_unit (actually, "->bitmap_pad"), because
	 * a MSBFirst machine will have the bytes with ``real'' bits
	 * AFTER the pad bytes. (We could check for an LSBFirst machine,
	 * but the efficiency improvement isn't worth the trouble.)
	 */
	if (image->format == ZPixmap)
		old_bits_per_line = image->bits_per_pixel * old_width;
	else
		old_bits_per_line = old_width;
	old_nbytes = ROUNDUP(old_bits_per_line, image->bitmap_pad) / 8;

	nplanes = (image->format == XYPixmap? image->depth : 1);

	while (nplanes--)
		for (
			y = old_height - 1, new_y = image->height - 1;
			y >= 0;
			y--
		) {
			bit_row = getrow(image, y, nplanes);
			for (s = h_scale; s--; )
				putrow (
					image,
					bit_row,
					new_y--,
					old_nbytes,
					nplanes
				);
		}

	return;
}

/**
 ** w_stretch() - ENLARGE HORIZONTALLY
 **/

static void		w_stretch (image, old_width, old_height, w_scale)
	XImage			*image;
	int			old_width,
				old_height,
				w_scale;
{
	if (isbitmap(image))
		w_stretch_bitmap (image, old_width, old_height, w_scale);
	else
		w_stretch_pixmap (image, old_width, old_height, w_scale);
	return;
}

/**
 ** w_stretch_pixmap() - ENLARGE HORIZONTALLY (SLOW, FOR PIXMAPS)
 **/

static void		w_stretch_pixmap (image, old_width, old_height, w_scale)
	XImage			*image;
	int			old_width,
				old_height,
				w_scale;
{
	int			x,
				y;


	for (y = 0; y < old_height; y++) {
		int			_x	= image->width - 1;


		for (x = old_width - 1; x >= 0; x--) {
			long			pixel	=
						   XGetPixel(image, x, y);

			int			_scale	= w_scale;


			while (_scale--)
				XPutPixel (image, _x--, y, pixel);
		}
	}
}

/* #define FAST_WAY_WORKS	/* */
#if	defined(FAST_WAY_WORKS)

/*
 * This routine needs some work. The current code has the wrong
 * assumptions about byte order and bit order; things are not as
 * simple as described in the comments.
 */

/**
 ** w_stretch_bitmap() - ENLARGE HORIZONTALLY (FAST, BITMAPS ONLY)
 **/

static char		*calc_expansion_table();

static void		w_stretch_bitmap (image, old_width, old_height, w_scale)
	XImage			*image;
	int			old_width,
				old_height,
				w_scale;
{
	char			*pd,
				*bigger,
				*tmp_unit,
				*tmp_row;

	int			nunits,
				bytes_per_unit;

	void			(*do_bytes)()	= 0;


	/*
	 * Note: This routine assumes zero x-offset.
	 */


	/*
	 * We can magnify the image a bitmap unit at a time using a
	 * lookup table--each original unit becomes "w_scale" units,
	 * except for the last unit. The last unit may expand to less,
	 * because it may not contain a full bitmap-unit of ``real''
	 * image data.
	 */

	/*
	 * Construct the lookup table.
	 * The macro function "MAGNIFY()" expands a byte
	 * pointed to by "PSRC" into "SCALE" bytes starting at
	 * "PDST". Note that the arguments to the macro are used
	 * once, so that side effects (e.g. ++) get done once.
	 */

	bigger = calc_expansion_table(w_scale);

#define MAGNIFY(PDST,PSRC,SCALE) \
		memcpy((PDST), &bigger[*(PSRC) * SCALE], SCALE)


	/*
	 * The expansion technique requires the bits to be contiguous
	 * across bitmap unit boundaries. This occurs when the byte
	 * order and bit order are the same.
* MORE: No, the last statement is wrong.
	 */
	if (image->byte_order != image->bitmap_bit_order)
		switch (image->bitmap_unit) {
		case 32:
			do_bytes = swap4;
			break;
		case 24:
			do_bytes = swap3;
			break;
		case 16:
			do_bytes = swap2;
			break;
		case 8:
			break;
		default:
			w_stretch_pixmap (image, old_width, old_height, w_scale);
			return;
		}


	/*
	 * Calculate the number of units, including the last, partially
	 * filled one.
	 */
	nunits = (old_width + image->bitmap_unit-1) / image->bitmap_unit;

	bytes_per_unit = (image->bitmap_unit + 7) / 8;

	/*
	 * We could do this right to left, but then the code becomes
	 * tricky because of the LSBFirst to MSBFirst conversion.
	 * So we use a temporary so we can do it left to right.
	 * An added benefit of the temporary is that, by making it
	 * long enough to hold ALL the original units expanded, we
	 * don't have to worry about the end case.
	 */
	tmp_row = Malloc(nunits * bytes_per_unit);
	tmp_unit = Malloc(bytes_per_unit);

	for (pd = image->data; old_height--; pd += image->bytes_per_line){
		char			*pold	= pd,
					*pnew	= tmp_row;

		int			_nunits	= nunits;

		register int		b;


		memset ((char *)tmp_row, 0, image->bytes_per_line);
		while (_nunits--) {
			memcpy (tmp_unit, pold, bytes_per_unit);
			if (do_bytes)
				(*do_bytes) (tmp_unit, 1);
			for (b = bytes_per_unit; b; b--)
				MAGNIFY (
					pnew + b * w_scale,
					tmp_unit + b,
					w_scale
				);
			pold += bytes_per_unit;
			pnew += bytes_per_unit * w_scale;
		}
		if (do_bytes)
			(*do_bytes) (tmp_row, w_scale * nunits);
		memcpy (pd, tmp_row, image->bytes_per_line);
	}

	free (tmp_unit);
	free (tmp_row);

	return;
}

/**
 ** calc_expansion_table()
 **/

static char		*calc_expansion_table (scale)
	int			scale;
{
	static char		*table		= 0;

	static int		last_scale	= 0;

	register unsigned char	orig_mask,
				exp_mask;

	register char		*pt;

	register int		index,
				last_index	= 255;


	/*
	 * We may be called upon to expand more than one image,
	 * so save the expansion table in case it comes in handy again.
	 */

	if (last_scale == scale)
		return (table);

	last_scale = scale;
	table = Malloc(scale * 256);
	memset (table, 0, scale * 256);

#define SHIFT	if (!(exp_mask <<= 1)) { exp_mask = 0x1; pt++; } else
	exp_mask = 0x1;
	pt = table;

	for (index = 0; index <= last_index; index++)
		for (orig_mask = 0x1; orig_mask; orig_mask <<= 1) {
			register int		_scale	= scale;


			if (index & orig_mask)
				while (_scale--) {
					*pt |= exp_mask;
					SHIFT;
				}
			else
				while (_scale--)
					SHIFT;
		}

	return (table);
}

#else

/**
 ** w_stretch_bitmap() - ENLARGE HORIZONTALLY (SPECIAL BITMAPS ONLY)
 **/

static void		w_stretch_bitmap (image, old_width, old_height, w_scale)
	XImage			*image;
	int			old_width,
				old_height,
				w_scale;
{
	char			*pd;

	Word			*tmp;

	int			new_nbytes,
				nplanes;


	if (
		image->bitmap_unit != WORDSIZE
	     || image->bitmap_pad != WORDSIZE
	     || image->byte_order != endian()
	     || image->bitmap_bit_order != LSBFirst
	) {
		w_stretch_pixmap (image, old_width, old_height, w_scale);
		return;
	}


	/*
	 * Note: This routine assumes zero x-offset.
	 */


	/*
	 * We are copying in place; we could copy right to left,
	 * to avoid problems with overlapping data, but the code
	 * for that is complicated because the last "Word" doesn't
	 * necessarily expand into "w_scale" "Words". So we copy
	 * left to right, using a temporary to hold an entire row.
	 */

	new_nbytes = sizeof(Word) * ((image->width + WORDSIZE-1) / WORDSIZE);
	tmp = (Word *)Malloc(new_nbytes);

	nplanes = (image->format == XYPixmap? image->depth : 1);

	while (nplanes--)
	    for (
		pd = image->data + nplanes * PLANE_AREA(image);
		old_height--;
		pd += image->bytes_per_line
	    ) {
		register Word		*pold		= (Word *)pd,
					*pnew		= tmp,
					old_mask	= 0x1,
					new_mask	= 0x1;

		register int		_width		= old_width;


#define SHIFT(M,P)	if (!(M <<= 1)) { M = 0x1; P++; } else

		memset ((char *)tmp, 0, new_nbytes);
		while (_width--) {
			register int		_scale	= w_scale;

	
			if (*pold & old_mask)
				while (_scale--) {
					*pnew |= new_mask;
					SHIFT (new_mask, pnew);
				}
			else
				while (_scale--)
					SHIFT (new_mask, pnew);

			SHIFT (old_mask, pold);
		}
		memcpy (pd, (char *)tmp, new_nbytes);
	    }

	free ((char *)tmp);
	return;
}

#endif
