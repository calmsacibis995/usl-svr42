#ident	"@(#)xpr:colormap.c	1.2"
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

#define TEXTURE_LIST		_4x4
#define TEXTURE_LIST_SIZE	17

#define COLORMAP(PC) \
	( \
		rgbmap.red * (PC)->red \
	      + rgbmap.green * (PC)->green \
	      + rgbmap.blue * (PC)->blue \
	)

struct RGBmap		rgbmap	= {
	.30, .59, .11
};

/*
 * Define the following if the color map should be reduced to just
 * the colors used in the image. This may improve the black and white
 * reproduction, but since it requires examing each pixel of the image
 * the execution time is increased.
 */
/* #define REDUCE_COLORMAP */

#if	defined(REDUCE_COLORMAP)
static void		reduce_colormap();
#endif

/**
 ** color_to_color() - COMPUTE COLOR MAP FROM TWO COLOR TABLES
 **/

long			*color_to_color (
				src_image,
				src_ncolors,
				src_colors,
				dst_image,
				dst_ncolors,
				dst_colors
			)
	XImage			*src_image,
				*dst_image;
	int			src_ncolors,
				dst_ncolors;
	XColor			*src_colors,
				*dst_colors;
{
	long			*ret;

	int			j,
				k;


/*
 * MORE: We need to understand better the relationship
 * between "ncolors" and "depth". It would seem that
 * "ncolors" should be 2 ** depth, that is,
 *
 *	depth = log(ncolors)	(log base 2)
 *
 * We assume the above is correct for the purpose of
 * generating the color-to-texture map. Other parts of
 * the code should verify this, or make allowances for
 * it not being true.
 */

	ret = (long *)Malloc(src_ncolors * sizeof(long));

#define C(K)		(&src_colors[K])
#define CBAR(J)		(&dst_colors[J])

	for (k = 0; k < src_ncolors; k++) {
		int			n	= 0;

		long			best	= DISTANCE(CBAR(n), C(k)),
					dist;


		for (j = 1; j < dst_ncolors; j++) {
			dist = DISTANCE(CBAR(j), C(k));
			if (dist < best) {
				n = j;
				best = dist;
			}
		}

		ret[C(k)->pixel] = CBAR(n)->pixel;
	}
			
	return (ret);
}

/**
 ** color_to_bw() - COMPUTE BLACK AND WHITE MAP FROM COLOR TABLE
 **/

char			**color_to_bw (image, ncolors, colors)
	XImage			*image;
	int			ncolors;
	XColor			*colors;
{
	char			**ret;

	int			n,
				nunique,
				ntextures;


/*
 * MORE: We need to understand better the relationship
 * between "ncolors" and "depth". It would seem that
 * "ncolors" should be 2 ** depth, that is,
 *
 *	depth = log(ncolors)	(log base 2)
 *
 * We assume the above is correct for the purpose of
 * generating the color-to-texture map. Other parts of
 * the code should verify this, or make allowances for
 * it not being true.
 */

	/*
	 * "Calloc" instead of "Malloc", because we use the return
	 * list in a temporary fashion when reducing the colors to
	 * a small enough set to map onto the textures.
	 */
	ret = (char **)Calloc(ncolors, sizeof(char *));

#if	defined(REDUCE_COLORMAP)
	/*
	 * Reduce the list of colors to include only those that show
	 * up in the image. Do this AFTER the "Calloc" above, because
	 * the returned array will be indexed by pixel value, which
	 * is most likely to vary from 0 to the original "ncolors".
	 * From here on, however, we don't need the original "ncolors"
	 * value any more.
	 */
	reduce_colormap (image, &ncolors, &colors);
#endif

	qsort ((char *)colors, ncolors, sizeof(XColor), color_cmp);

	/*
	 * If we have more colors than textures, double-up colors
	 * on the same texture. This is done in a loop that quits
	 * when enough have been doubled-up. We mark doubled-up
	 * colors (only one in the set) by a 1 in the map list.
	 *
	 * A pair of colors is doubled-up if they are the closest in
	 * intensity of the colors remaining. Because of the sorting
	 * we did above, this pair will either adjoin, or be separated
	 * by colors already doubled-up with one in the pair.
	 */

#define DELTA(A,B)	COLORMAP(&colors[A]) - COLORMAP(&colors[B])

	nunique = ncolors;
	ntextures = TEXTURE_LIST_SIZE;
	while (nunique > ntextures) {
		register int		dbl = colors[ncolors - 1].pixel,
					j;

		register long		delta_best = DELTA(dbl, 0),
					delta;


		for (n = 0; n < ncolors; n++) {

			/*
			 * Skip to nearest color that has not been
			 * doubled-up yet.
			 */
			for (j = n + 1; j < ncolors; j++)
				if (!ret[colors[j].pixel])
					break;

			if (
				j < ncolors
			     && (delta = DELTA(j, n)) < delta_best
			) {
				delta_best = delta;
				dbl = colors[j].pixel;
			}

		}
		ret[dbl] = (char *)1;
		nunique--;
	}

	/*
	 * Spread the textures over the colors, if there are less
	 * (unique) colors than textures. (The following code will
	 * still work when there are the same number of colors as
	 * textures.)
	 */
	{
		/*
		 * Confused about the arithmetic for "skip" and
		 * "residual"? These values are how many textures to skip
		 * between colors (plus 1), and how many additional
		 * textures to skip (1 per color), respectively. We have
		 * a ``fence-post'' problem here--imagine the solution
		 * as follows: The first texture should be applied to
		 * the first color, the last texture to the last color,
		 * and the rest of the textures applied evenly across
		 * the rest of the colors. Or put another way, we can
		 * apply the last texture to the last color, and apply
		 * one texture per remaining color with a uniform number
		 * of textures skipped AFTER EACH REMAINING COLOR.
		 * I.e. 1 texture (the last) is applied to 1 color (the
		 * last) and the other "(ntextures - 1)" textures are
		 * doled out to the other "(nunique - 1)" colors.
		 */
		register int		skip	=
					  (ntextures - 1) / (nunique - 1),
					residual=
					  (ntextures - 1) % (nunique - 1),
					low_n	= 0,
					high_n	= ncolors - 1;

		register char		**cur_texture,
					**low_texture =
					  TEXTURE_LIST,
					**high_texture=
					  TEXTURE_LIST + ntextures - 1;


		/*
		 * This loop counts through the colors by alternating
		 * between the ends of the color list, gradually moving
		 * in to the middle color(s). The texture to assign
		 * to a color is also alternated between the ends of the
		 * texture list, but some textures are skipped. The skip
		 * is as uniform as possible.
		 */
		for (
			n = low_n, cur_texture = low_texture;
			low_n <= high_n;
			/* find ``increment'' in body of loop */
		) {
			/*
			 * If this color is marked, it is one to
			 * double-up on the same texture as the previous
			 * color. This means that there were more
			 * colors than textures so that the stuff with
			 * "skip" and "residual" doesn't apply.
			 */
			register int		bump_texture	=
						ret[colors[n].pixel] == 0;


			ret[colors[n].pixel] = *cur_texture;

			if (n == low_n) {
				n = high_n;
				low_n++;
			} else {
				n = low_n;
				high_n--;
			}

			if (cur_texture == low_texture) {
				cur_texture = high_texture;
				if (bump_texture) {
					low_texture += skip;
					if (residual-- > 0)
						low_texture++;
				}
			} else {
				cur_texture = low_texture;
				if (bump_texture) {
					high_texture -= skip;
					if (residual-- > 0)
						high_texture--;
				}
			}
		}
	}

	return (ret);
}

/**
 ** reduce_colormap() - REDUCE COLOR MAP TO COLORS USED IN IMAGE
 **/

#if	defined(REDUCE_COLORMAP)

static void		reduce_colormap (image, pncolors, pcolors)
	XImage			*image;
	int			*pncolors;
	XColor			**pcolors;
{
	long			*used	=
				(long *)Calloc(*pncolors, sizeof(long));

	int			ncolors_used,
				x,
				y,
				n,
				j;

	XColor			*colors_used;


	/*
	 * MORE: We count the number of times each color is used,
	 * but don't use this except to see IF the color is used.
	 * Later we might use this information to provide a better
	 * map.
	 */
	for (y = 0; y < image->height; y++)
		for (x = 0; x < image->width; x++)
			used[XGetPixel(image, x, y)]++;

	ncolors_used = 0;
	for (n = 0; n < *pncolors; n++)
		if (used[n])
			ncolors_used++;

	colors_used = (XColor *)Malloc(ncolors_used * sizeof(XColor));
	for (j = n = 0; n < *pncolors; n++)
		if (used[(*pcolors)[n].pixel])
			colors_used[j++] = (*pcolors)[n];

	if (*pncolors != ncolors_used) {
		*pncolors = ncolors_used;
		*pcolors = colors_used;
	}

	free ((char *)used);

	return;
}

#endif

/**
 ** color_cmp() - COMPARE TWO COLORS FOR ORDER
 **/

int			color_cmp (pa, pb)
	XColor			*pa,
				*pb;
{
	long			Ma	= COLORMAP(pa),
				Mb	= COLORMAP(pb);


	if (Ma < Mb)
		return (-1);
	else if (Ma == Mb)
		return (0);
	else
		return (1);
}

/**
 ** RGBdistance() - COMPUTE DISTANCE BETWEEN TWO COLORS
 **/

long			RGBdistance (pa, pb)
	XColor			*pa,
				*pb;
{
	double			delta_red,
				delta_green,
				delta_blue;

	long			ret;

	extern double		sqrt();


	delta_red   = (double)pa->red   - (double)pb->red;
	delta_green = (double)pa->green - (double)pb->green;
	delta_blue  = (double)pa->blue  - (double)pb->blue;

	ret = (long)sqrt(
		  delta_red * delta_red
		+ delta_green * delta_green
		+ delta_blue * delta_blue
	);

	return (ret);
}
