#ident	"@(#)xpr:devices/terminfo/image_bits.c	1.4"
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

#include "xpr_term.h"

#define	FIX_NOTHING	0
#define	FIX_INVERT_TEXT	1
#define FIX_SWAP_COLORS	2

static unsigned char	*set_mask,
			*dot_bit[2];

static int		offset,
			*dot_byte,
			bytes_per_unit;

static void		general_copy_bits(),
			(*copy_bits)(),
			init_color();

static int		fix_colors();

static XColor		*program_colors;

static int		program_ncolors;

/**
 ** image_to_bits()
 **/

void			image_to_bits (image, x, y, width, height, wscale, hscale)
	XImage			*image;
	int			x,
				y,
				width,
				height,
				wscale,	/* not used */
				hscale;
{
	static unsigned char	**bytes;

	char			*pd;

	int			data_offset,
				units,
				plane,
				plane_area,
				nplanes	= image->depth;


	/*
	 * If we can program the colors the printer will use,
	 * do this now.
	 *
	 * Note: Doing this here means it is repeated if the
	 * user gives a -s option. However, (1) -s is not likely
	 * to be used often (it was added for problem printers that
	 * handle to much data), and (2) we have to do it after
	 * getting the printer into graphics mode so that the
	 * color def'n works properly (e.g. HP printer).
	 */
	if (bitype == 3 && OKAY(initc)) {
		XColor			*pc	= program_colors;

		int			nc	= program_ncolors;


		for (; nc--; pc++)
#define X_TO_TI(X)	(int)(((long)(X) * 1000) / 65536)

			init_color (
				pc->pixel,
				X_TO_TI(pc->red),
				X_TO_TI(pc->green),
				X_TO_TI(pc->blue)
			);
	}


	/*
	 * Construct and put out bit rows for laser printer type
	 * devices.
	 *
	 * For these devices, "porder" specifies the order of the
	 * bits in each unit of image data. "npins" gives the size
	 * of the unit (in bits).
	 */


	/*
	 * Each "bytes[i]" will hold a row of bit units.
	 */
	units = (width + npins - 1) / npins;
	bytes = (unsigned char **)malloc(nplanes * sizeof(unsigned char **));
	for (plane = 0; plane < nplanes; plane++)
		bytes[plane] = umalloc(units * bytes_per_unit);

	/*
	 * We only care about the delta height;
	 */
	height -= y;

	/*
	 * Take care of the lion's share of the x-offset by an offset
	 * in the data pointer. The residual offset has to be done with
	 * bit shifts.
	 */
	data_offset = (x / (image->bitmap_unit * 8)) * image->bitmap_unit;
	x -= data_offset * 8;
	width -= data_offset * 8;

	plane_area = PLANE_AREA(image);

	for (
		pd = image->data + data_offset + y * image->bytes_per_line;
		height;
		pd += image->bytes_per_line, height--
	) {
		/*
		 * Initialize the output bytes with the constant
		 * mask of on-bits.
		 */
		for (plane = 0; plane < nplanes; plane++) {
			register int		b,
						u,
						_units	= units,
						_bytes_per_unit
							= bytes_per_unit;

			register unsigned char	*pb	= bytes[plane];


			for (u = 0; u < _units; u++)
				for (b = 0; b < _bytes_per_unit; b++)
					*pb++ = set_mask[b];
		}

		/*
		 * Copy the bits to their place(s) in the output byte
		 * stream.
		 *
		 * MORE: IS THE FOLLOWING RIGHT FOR *ALL* PRINTERS?
		 * We want to put out the bit planes starting with
		 * the LSB, but the planes are ordered MSBFirst.
		 * Thus the odd calculation for the plane address.
		 */
		for (plane = 0; plane < nplanes; plane++)
			(*copy_bits) (
				bytes[plane],
				pd + plane_area * (nplanes - plane - 1),
				x,
				width,
				1
			);

		/*
		 * Add in the constant offset to each byte.
		 */
		if (offset) for (plane = 0; plane < nplanes; plane++) {
			register unsigned char	*pb	= bytes[plane];

			register int		_x,
						b,
						_width	= width,
						_offset	= offset,
						_bytes_per_unit
							= bytes_per_unit;


			for (_x = x; _x < _width; _x++)
				for (b = 0; b < _bytes_per_unit; b++)
					*pb++ += _offset;
		}

		/*
		 * Put out the packed bits.
		 */
		{
			int			_hscale = hscale;


			while (_hscale--) {
			    for (plane = 0; plane < nplanes; plane++)
				output_units (
					0,
					bytes[plane],
					bytes_per_unit,
					units,
					1,
					0
				);
			    output_nel ();
			}
		}
	}

	for (plane = 0; plane < nplanes; plane++)
		free ((char *)bytes[plane]);
	free ((char *)bytes);

	return;
}

/**
 ** init_image_to_bits()
 **/

/*
 * This routine sets up any local variables needed by the
 * "image_to_bits()" routine, and establishes the image data structure
 * needed (i.e. XYBitmap, XYPixmap, depth, bit and byte order).
 *
 * WARNING: Currently the "image_bitblt()" routine only works
 * with the XYBitmap format; the ZPixmap format with depth of 1;
 * or the XYPixmap format, any depth (but copying into the last plane).
 * ALSO only with a ``native'' format, with WORDSIZE bits per unit.
 * If you need other formats you will have to update the "image_bitblt()"
 * routine.
 */

void			init_image_to_bits (
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
	int			nplanes;


	offset = parse_porder(
		&set_mask,
		dot_bit,
		&dot_byte,
		&bytes_per_unit
	);

#if	defined(BE_EFFICIENT)

	/*
	 * Most devices of this type will take simple bit arrangements,
	 * e.g. 8-bit bytes, most significant bit 1st for the HP Laserjet.
	 * These we can handle efficiently, unless we have to scale
	 * horizontally. We don't know about the scale yet, so the
	 * following setup is conditional and will be verified in the
	 * "image_to_bits()" routine.
	 */

	...

#else

	if (bitype != 3) {

		*p_dst_ncolors = 0;
		*p_dst_colors = 0;
		dst_image->format = XYBitmap;
		dst_image->depth = 1;

	} else if (bitype == 3 && src_ncolors <= colors) {

		/*
		 * We use the same colors as the source (if possible)
		 * in hopes that the pixel-conversion routines can
		 * work quickly. Later (in the "ti_map()" routine),
		 * we put out the control sequences necessary to program
		 * the printer with these colors. HOWEVER, we
		 * want a pixel value of 0 to be white and a pixel value
		 * of 1 to be black (because of the damned header/footer
		 * text). If the source colors don't have this, we have
		 * to switch them.
		 */

		program_ncolors = *p_dst_ncolors = src_ncolors;

		switch (fix_colors(src_ncolors, src_colors, p_dst_colors)) {

		case FIX_INVERT_TEXT:
			*p_invert_text = 1;
			goto There;

		case FIX_NOTHING:
			*p_invert_text = 0;
There:			free ((char *)*p_dst_colors);
			*p_dst_colors = src_colors;
			break;

		case FIX_SWAP_COLORS:
			*p_invert_text = 0;
			break;

		}

		program_colors = *p_dst_colors;

		if (isbitmap(src_image))
			nplanes = 1;
		else {
			/*
			 * Compute the number of planes necessary to
			 * encode the number of SOURCE COLORS. This keeps
			 * us from putting out more planes than necessary
			 * to reproduce the source image. Note, though,
			 * that the Terminfo entries *may* have to
			 * compensate for missing planes, if the printer
			 * requires a fixed number of planes.
			 *
			 * MORE: Does the above matter? Maybe we need
			 * a new Terminfo cap. that says #planes is fixed?
			 */
			for (nplanes = 1; nplanes < 32; nplanes++)
				if ((1 << nplanes) >= src_ncolors)
					break;
		}

		dst_image->format = XYPixmap;
		dst_image->depth = nplanes;

	} else {

		/*
		 * MORE: The source has more colors than we can reproduce
		 * with this printer. However, it is a color-programmable
		 * printer, so we ought to be able to do better than the
		 * following.
		 *
		 * WE ASSUME WE ARE USING AN HP PAINTJET PRINTER:
		 *
		 * The following table is from the manual for the
		 * HP PaintJet printer. We need RGB values scaled
		 * to 65535, while these are scaled to (90,88,85)
		 * (i.e. each of R, G, and B are scaled slightly
		 * differently).
		 *
		 * WE WILL USE ONLY THE FIRST "colors" VALUES.
		 */
		static XColor		PaintJetColors[] = {

#define HP_TO_X(HP,LO,HI) (int)(((long)(HP-LO)*65535)/(HI-LO))
#define R(X)	HP_TO_X(X, 4, 90)
#define G(X)	HP_TO_X(X, 4, 88)
#define B(X)	HP_TO_X(X, 6, 85)


			{ 0,	 R(4),  G(4),  B(6) },	/* black */
			{ 1,	R(90), G(88), B(85) },	/* white */
			{ 2,	R(53),  G(8), B(14) },	/* red */
			{ 3,	 R(3), G(26), B(22) },	/* green */
			{ 4,	R(89), G(83), B(13) },	/* yellow */
			{ 5,	 R(4),  G(4), B(29) },	/* blue */
			{ 6,	R(53),  G(5), B(25) },	/* magenta */
			{ 7,	 R(2), G(22), B(64) },	/* cyan */
			{ 8,	R(72), G(41), B(13) },	/* orange */
			{ 9,	R(12),  G(6), B(24) },	/* purple */
			{ 10,	R(12),  G(8), B(10) },	/* brown */
			{ 11,	R(15), G(16), B(18) },	/* dark gray */
			{ 12,	R(43), G(43), B(45) },	/* light gray */
			{ 13,	R(52),  G(6), B(19) },	/* pink */
			{ 14,	 R(3), G(10), B(46) },	/* light blue */
			{ 15,	R(89), G(87), B(31) },	/* light yellow */
		};


		program_ncolors = *p_dst_ncolors = colors;
		program_colors = *p_dst_colors = PaintJetColors;

		/*
		 * Compute the number of planes necessary to encode
		 * the number of colors THE PRINTER CAN HANDLE.
		 * This differs from the similar computation for the
		 * "src_ncolors <= colors" case, so watch out!
		 */
		for (nplanes = 0; nplanes < 32; nplanes++)
			if ((1 << nplanes) >= colors)
				break;
/*		nplanes++;	*/

		dst_image->format = XYPixmap;
		dst_image->depth = nplanes;

	}

	copy_bits = general_copy_bits;

	dst_image->bits_per_pixel = 1; /* not depth! except for ZPixmap */

	dst_image->byte_order = endian();
	dst_image->bitmap_bit_order = LSBFirst;
	dst_image->bitmap_unit = dst_image->bitmap_pad = WORDSIZE;

#endif

	return;
}

/**
 ** general_copy_bits()
 **/

static void		general_copy_bits (bytes, data, x, width, wscale)
	unsigned char		*bytes;
	char			*data;
	int			x,
				width,
				wscale;
{
	/*
	 * "data" must point to a Word aligned position
	 * for this to work. "pd" is incremented by
	 * left-shifting a single bit in "m", starting
	 * at 1 and shifting until the bit is shifted
	 * out ("mask" becomes zero); when the bit is
	 * shifted out, "pd" is incremented. "mask",
	 * not surprisingly, is the mask used to select
	 * the bit/pixel.
	 */
	register Word		*pd		= (Word *)data,
				mask		= 0x1 << x;

	register unsigned char	*pb		= bytes,
				ink_mask;

	register int		_x,
				dot		= 0,
				rep,
				_npins		= npins,
				_width		= width,
				_bytes_per_unit	= bytes_per_unit;


	for (_x = x; _x < _width; _x++) {
		ink_mask = dot_bit[(*pd & mask) != 0][dot];
		for (rep = wscale; rep--; ) {
			pb[dot_byte[dot]] |= ink_mask;
			if (++dot >= _npins) {
				dot = 0;
				pb += _bytes_per_unit;
			}
		}
		if (!(mask <<= 1)) {
			pd++;
			mask = 0x1;
		}
	}

	return;
}

/**
 ** fix_colors() - FIX COLORMAP SO THAT PIXEL VALUE 1/0 == BLACK/WHITE
 **/

static int		fix_colors (src_ncolors, src_colors, p_dst_colors)
	int			src_ncolors;
	XColor			*src_colors,
				**p_dst_colors;
{
	XColor			*pwhite,
				*pblack;

	int			color;


	/*
	 * Our job: To make sure that the pixel values 1 and 0 give
	 * the colors black and white, respectively. This is needed
	 * to support the header/trailer feature: these text images
	 * are bitmaps that assume that 1/0 give black and white.
	 * Note: If these color values are simply reversed, we can
	 * take care of the problem elsewhere. Here we take care of
	 * a really mixed up source color pallete.
	 */


	*p_dst_colors = (XColor *)malloc(src_ncolors * sizeof(XColor));
	for (color = 0; color < src_ncolors; color++)
		(*p_dst_colors)[color] = src_colors[color];

	/*
	 * Find the ``white'' and ``black'' colors. These
	 * are really the most and least intense colors, respectively.
	 */
	pwhite = &(*p_dst_colors)[0];
	for (color = 1; color < src_ncolors; color++)
		if (INTENSITY(pwhite) < INTENSITY(&(*p_dst_colors)[color]))
			pwhite = &(*p_dst_colors)[color];
	pblack = &(*p_dst_colors)[0];
	for (color = 1; color < src_ncolors; color++)
		if (INTENSITY(pblack) > INTENSITY(&(*p_dst_colors)[color]))
			pblack = &(*p_dst_colors)[color];

	/*
	 * Three possibilities:
	 *
	 *	FIX_NOTHING	  ok, black and white are pixels 1 and 0
	 *	FIX_INVERT_TEXT	  ok, except black and white reversed
	 *	FIX_SWAP_COLORS	  not ok, swapped black/white with others
	 */

	if (pwhite->pixel == 0 && pblack->pixel == 1)
		return (FIX_NOTHING);

	if (pwhite->pixel == 1 && pblack->pixel == 0)
		return (FIX_INVERT_TEXT);

	if (pwhite->pixel != 0)
		for (color = 0; color < src_ncolors; color++)
			if ((*p_dst_colors)[color].pixel == 0) {
				(*p_dst_colors)[color].pixel = pwhite->pixel;
				pwhite->pixel = 0;
				break;
			}

	if (pblack->pixel != 1)
		for (color = 0; color < src_ncolors; color++)
			if ((*p_dst_colors)[color].pixel == 1) {
				(*p_dst_colors)[color].pixel = pblack->pixel;
				pblack->pixel = 1;
				break;
			}

	return (FIX_SWAP_COLORS);
}

/**
 ** init_color()
 **/

/*
 * This code has been largely snarfed from the Curses library,
 * although much has been stripped out.
 */

static void	_rgb_to_hls();
static float	MAX();
static float	MIN();

static void
init_color(color, r, g, b)
register short color, r, g, b;
{
    /* if any of the last 3 arguments is out of 0 - 1000 range,     */
    /* adjust them accordingly					    */

    if (r > 1000)	r = 1000;
    if (g > 1000)	g = 1000;
    if (b > 1000)	b = 1000;
    if (r < 0)		r = 0;
    if (g < 0)		g = 0;
    if (b < 0)		b = 0;

    /* for terminals that can define individual colors (Tek model)  */
    /* send an escape sequence to define that color                 */

    if (initc)
    {
	if (hls)
	{
            int      h, s, l;
	    _rgb_to_hls ((float)r, (float)g, (float)b, &h, &l, &s);
            putp (tparm (initc, color, h, l, s));
	}
	else
            putp (tparm (initc, color, r, g, b));
    }
    return;
}

static void
_rgb_to_hls(r, g, b, hh, ll, ss)
register float r, g, b;
int  *hh, *ll, *ss;
{
    register float rc, gc, bc, h, l, s;
    double   _max, _min;

    r /= 1000;  g /= 1000;  b /= 1000;

    _max = MAX (r, g, b);
    _min = MIN (r, g, b);

    /* calculate lightness  */

    l = (_max + _min) / 2;

    /* calculate saturation */

    if (_max == _min)
    {
        s = 0;
        h = 0;
    }
    else
    {
        if (l < 0.5)
            s = (_max - _min) / (_max + _min);
        else
            s = (_max - _min) / (2 - _max - _min);

        /* calculate hue   */

        rc = (_max - r) / (_max - _min);
        gc = (_max - g) / (_max - _min);
        bc = (_max - b) / (_max - _min);

        if (r == _max)
                h = bc - gc;
        else if (g == _max)
                h = 2 + rc - bc;
        else /* if (b == _max) */
                h = 4 + gc - rc;

        h = h * 60;
        if (h < 0.0)
            h = h + 360;

        /* until here we have converted into HSL.  Now, to convert into */
        /* Tektronix HLS, add 120 to h					*/

	h = ((int)(h+120))%360;
    }
    *hh = (int) h;
    *ss = (int) (s * 100);
    *ll = (int) (l * 100);
}

static float
MAX (a, b, c)
register float a, b, c;
{
	if ( a>= b)
	     if (a >= c)
		 return (a);
	     else return (c);
        else if (c >=b)
		return (c);
	     else return (b);
}

static float
MIN (a, b, c)
register float a, b, c;
{
	if ( a> b)
	     if (b > c)
		 return (c);
	     else return (b);
        else if (a <c)
		return (a);
	     else return (c);
}
