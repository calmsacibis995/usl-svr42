#ident	"@(#)xpr:devices/terminfo/text.c	1.2"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "Xlib.h"
#include "Xutil.h"

#include "xpr.h"

#include "xpr_term.h"
#include "text.h"

static void		image_string();

static _Font		*get_font(),
			*curfont;

static int		strwidth();

/**
 ** pick_best_font() - PICK FONT CLOSEST TO DESIRED POINT SIZE
 **/

void			pick_best_font (ptsize, d_scale)
	int			ptsize;
	double			d_scale;
{
	_Fontref		*pfr,
				*pfr_closest	= 0;

				/*
				 * There are about 72 points per inch.
				 */
	double			size_inches	= (double)ptsize / 72,
				dist_closest	= 999999,
				dist;


	for (pfr = fontref; pfr->font_p; pfr++) {
		dist = size_inches - d_scale * pfr->font_p->height;
		if (dist < 0)
			dist = -dist;
		if (!pfr_closest || dist < dist_closest) {
			pfr_closest = pfr;
			dist_closest = dist;
		}
	}


	curfont = pfr_closest->font_p;
	_XInitImageFuncPtrs (curfont->image);


/*
 * MORE: Fix the fonts so this is no longer necessary.
 */
	{
		_Font			*fnt;

		static XImage		img;

		extern char		*Malloc();


		curfont->image->byte_order = endian();

		fnt = (_Font *)Malloc(
		    sizeof(_Font) + (curfont->n + 1) * sizeof(_Fontchar)
		);
		*fnt = *curfont;
		memcpy (
			fnt->info,
			curfont->info,
			(curfont->n + 2) * sizeof(_Fontchar)
		);

		*(fnt->image = &img) = *(curfont->image);
		fnt->image->data = Calloc(IMAGE_SIZE(fnt->image), 1);
		fnt->image->byte_order = endian();
		fnt->image->bitmap_bit_order = LSBFirst;
		swap_copy (curfont->image, fnt->image);

		curfont = fnt;
	}

	return;
}

/**
 ** text_to_image()
 **/

XImage			*text_to_image (text)
	char			*text;
{
	XImage			*pi;


	if (!text || !*text)
		return (0);

	pi = (XImage *)Malloc(sizeof(XImage));

	pi->width = strwidth(curfont, text);
	pi->height = curfont->height;
	pi->xoffset = 0;

	pi->format = XYBitmap;
	pi->depth = pi->bits_per_pixel = 1;

	pi->byte_order = endian();
	pi->bitmap_unit = pi->bitmap_pad = WORDSIZE;
	pi->bitmap_bit_order = LSBFirst;

	pi->bytes_per_line = ROUNDUP(pi->width, pi->bitmap_pad) / 8;
	pi->data = Malloc(pi->height * pi->bytes_per_line);

	_XInitImageFuncPtrs (pi);

	image_string (curfont, text, pi, 0, 0);

	return (pi);
}

/**
 ** image_string() - DRAW CHARACTER STRING IN A BITMAP
 **/

static void		image_string (pf, text, pi, x, y)
	_Font			*pf;
	char			*text;
	XImage			*pi;
	int			x,
				y;
{
	register unsigned int	c;

	register _Fontchar	*pc;


	for (; (c = (unsigned int)*text++); x += pc->width) {
		if (c > pf->n || !(pc = &(pf->info[c]))->width)
			break;
		image_bitblt (
			pf->image,
			pc->x,
			0,
#if	defined(SHOULD_BE_THIS)
			(pc + 1)->x - pc->x,
#else
			pc->width,
#endif
			pf->height,
			pi,
			x + pc->left,
			y
		);
	}
	return;
}

/**
 ** strwidth() - COMPUTE WIDTH OF TEXT IMAGE
 **/

static int		strwidth (pf, text)
	_Font			*pf;
	char			*text;
{
	register int		width	= 0;

	register _Fontchar	*info	= pf->info;


	while (*text)
#define USE_REAL_WIDTH	/* */
#if	defined(USE_REAL_WIDTH)
		width += info[*text++].width;
#else
	{
		width += (info[*text + 1].x - info[*text].x);
		text++;
	}
#endif
	return (width);
}
