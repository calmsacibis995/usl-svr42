#ident	"@(#)xpr:devices/postscript/map.c	1.5"
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

#include "xpr_ps.h"

static void		page_limit();
static void		begin_page();

/**
 ** ps_map()
 **/

void			ps_map (
				TERM,
				src_image,
				src_ncolors,
				src_colors,
				height,
				width,
				left,
				top,
				split,
				flags,
				header,
				trailer,
				pscale,
				pisportrait,
				color_list
			)
	char *			TERM;
	XImage *		src_image;
	int			src_ncolors;
	XColor *		src_colors;
	double			height;
	double			width;
	double			left;
	double			top;
	int			split;
	int			flags;
	char *			header;
	char *			trailer;
	int *			pscale;
	int *			pisportrait;
	char			*color_list;
{
	double			text_adjust;

	int			dot_width;
	int			dot_height;
	int			pix_width;
	int			pix_height;
	int			w_scale;
	int			h_scale;
	int			image_scale;
	int			left_margin;
	int			top_margin;
	int			row_start;
	int			split_height;
	int			split_dot_height;
	int			dst_ncolors	= 0;
	int			page;

	XImage			image1;
	XImage *		dst_image	= &image1;

	XColor *		dst_colors	= 0;


	/*
	 * Determine the orientation of the image. Unless instructed
	 * by the user, align the longest edge of the image with
	 * the longest edge of the page.
	 *
	 * Note: This code follows the letter of the original manual
	 * page, not the original code. Originally, the window
	 * height was increased for the header and trailer, before
	 * the longest edge was determined. The original manual page
	 * says ``the window is printed such that its longest side
	 * follows the long side of the paper.''
	 *
	 * MORE: Let printer rotate the image?
	 */
	if (flags & F_PORTRAIT)
		*pisportrait = 1;
	else if (flags & F_LANDSCAPE)
		*pisportrait = 0;
	else if (src_image->width == src_image->height)
		*pisportrait = 1;
	else if (src_image->width < src_image->height)
		if (PS_PAGE_WIDTH <= PS_PAGE_HEIGHT)
			*pisportrait = 1;
		else
			*pisportrait = 0;
	else
		if (PS_PAGE_WIDTH < PS_PAGE_HEIGHT)
			*pisportrait = 0;
		else
			*pisportrait = 1;


	text_adjust = 0;
	if (header)
		text_adjust += PS_TEXT_ADJUST;
	if (trailer)
		text_adjust += PS_TEXT_ADJUST;

	/*
	 * Determine the width and height of the image, as viewed
	 * upright, in inches. Make sure that the left/top margins
	 * leave enough space for the image at the desired width/height.
	 *
	 * Note: We have to leave room for the header and trailer,
	 * if present; thus the maximum space is reduced accordingly.
	 *
	 * Note: This differs from the original program, where a constant
	 * (75) was added to the incoming image size for each of the
	 * header and trailer. The problem there, though, is that when
	 * the image gets scaled, the 75 gets scaled, too, even though
	 * the printed header and trailer size doesn't change.
	 */
	if (*pisportrait) {
		page_limit (&width, &left, PS_PAGE_WIDTH);
		page_limit (&height, &top, PS_PAGE_HEIGHT - text_adjust);
	} else {
		page_limit (&width, &left, PS_PAGE_HEIGHT);
		page_limit (&height, &top, PS_PAGE_WIDTH - text_adjust);
	}

	/*
	 * Determine the maximum width and height of the printed
	 * image, measured as the image is printed, in dots.
	 * In other words, "dot_width" measures across the page,
	 * "dot_height" down the page, regardless of how the image
	 * is turned; likewise for the temporaries, "pix_width" and
	 * "pix_height".
	 *
	 * Note: The size in dots doesn't include the header and trailer.
	 */
	if (*pisportrait) {
		dot_width = PS_DPI * width;
		dot_height = PS_DPI * height;
		pix_width = src_image->width;
		pix_height = src_image->height;
	} else {
		dot_width = PS_DPI * height;
		dot_height = PS_DPI * width;
		pix_width = src_image->height;
		pix_height = src_image->width;
	}

	if (dot_width < pix_width) {
		fprintf (
			stderr,
"xpr: Warning: Image too large for width of device %s--truncating.\n",
			TERM
		);
		/*
		 * We'll truncate it later as part of the general
		 * focusing handling, but we'll keep the values the
		 * way they are for now so that we will have a way
		 * to distinguish this ``focusing'' from the user
		 * requested focusing. We need to distinguish because
		 * this ``focusing'' doesn't allow the scale and margins
		 * to override the need to get as most of the image
		 * as possible on the page.
		 */
	}
	if (dot_height < pix_height) {
		fprintf (
			stderr,
	"xpr: Warning: Image will not fit on one page; use -s option.\n"
		);
		/*
		 * Let this go because it may not matter for most
		 * devices--the image will just spill across to subsequent
		 * pages.
		 */
	}

	/*
	 * Determine the default scale. This is also the
	 * maximum scale that will still print the entire
	 * window.
	 */
	w_scale = dot_width / pix_width;
	h_scale = dot_height / pix_height;
	image_scale = min(w_scale, h_scale);
	if (image_scale < 1)
		image_scale = 1;

	/*
	 * If a scale was given by the user, use it
	 * unless it is too large. In any case, report
	 * the scale back to the caller.
	 */
	if (0 < *pscale	&& *pscale <= image_scale)
		image_scale = *pscale;
	else {
		/*
		 * Limit the scale to some reasonable size, to
		 * avoid silly, huge printed pixels. Do this here,
		 * to allow the user to override this by giving
		 * a scale.
		 *
		 * The limit is SCALE_LIMIT inches maximum,
		 * i.e. ``dots'' no more then SCALE_LIMIT inches wide
		 * or tall.
		 */
		int			limit	= (PS_DPI * SCALE_LIMIT);


		if (image_scale > limit)
			image_scale = limit;
		if (*pscale > image_scale)
			fprintf (
				stderr,
	"xpr: Warning: Scale %d too large; scale %d used instead.\n",
				*pscale,
				image_scale
			);
		*pscale = image_scale;
	}


	/*
	 * Initialize the output routine; this will also set the
	 * best storage format for the intermediate image, and will
	 * set the range of output colors available.
	 */
	*dst_image = *src_image; /* Original storage may be suitable */
	init_dump_ps_image (
		dst_image,
		&dst_ncolors,
		&dst_colors,
		src_image,
		src_ncolors,
		src_colors,
		color_list
	);

#if	defined(CHECK_REVERSE_VIDEO)
	if (
		!isbitmap(src_image)
	     && !isbitmap(dst_image)
	     && flags & F_INVERT
	) {
		flags &= ~F_INVERT;
		fprintf (
			stderr,
"xpr: Warning: -r option ignored for color image printed on color printer.\n"
		);
	}
#endif

	/*
	 * Convert the original image into one easily handled by
	 * the image printing routine. The image printing routine
	 * may be a better place to handle the scaling, so the
	 * destination image we get from "pix_convert()" may be
	 * narrower or shorter than the final printed image.
	 */
	pix_convert(
		src_image,
		src_ncolors,
		src_colors,
		dst_image,
		dst_ncolors,
		dst_colors,
		!*pisportrait,
		0,	/* reverse-video done in PostScript */
		1,
		1
	);


/*
 * MORE: A more general focus technique should be allowed.
 */
	/*
	 * If the image is too wide for the device, truncate it
	 * on the right. This is done after the "pix_convert()"
	 * routine has been called because the code is easier.
	 * However, investing some time to putting it before
	 * calling "pix_convert()" would probably make the program
	 * faster.
	 *
	 * Note: If the original image was too wide, then we won't
	 * be scaling it UNLESS the focus feature is active.
	 * If we were scaling, then we would have to do more work
	 * below.
	 *
	 * Note: The technique for truncating leads to an inconsistency
	 * in the image storage; the "bytes_per_line" is no longer related
	 * to the "width". No routine should be relying on the relation
	 * to hold.
	 */
	if (dst_image->width > dot_width)
		dst_image->width = dot_width;


	/*
	 * Re-calculate the printed image size in ``dots''. These
	 * dots are what the device will print, perhaps scaled up
	 * to larger dots (and spaced accordingly) by "image_scale".
	 */
	dot_width = dst_image->width;
	dot_height = dst_image->height;

	/*
	 * Calculate the left and top margins for the output
	 * image, in printed dots. Again, these dots may be scaled
	 * (by the device) by the factor "image_scale".
	 */
	if (left < 0) {
		double			d_image_width;	/* inches */


		d_image_width = (double)(dot_width * image_scale) / PS_DPI;
		left = (PS_PAGE_WIDTH - d_image_width) / 2;
	}
	if (top < 0) {
		double			d_image_height;	/* inches */


		d_image_height = (double)(dot_height * image_scale) / PS_DPI;
		top = (PS_PAGE_HEIGHT - d_image_height) / 2;
	}
	left_margin = (left * PS_DPI) / image_scale;
	top_margin = (top * PS_DPI) / image_scale;


	/*
	 * This is a little different from other printers,
	 * as we always want to back up over some of the
	 * epilogue.
	 */
	if (flags & F_APPEND)
		ps_backup_epilogue (stdout, &page, (flags & F_NOFF));
	else
		page = 0;

	/*
	 * Don't put out the prologue
	 * for an appended file, as we assume the file already
	 * has it.
	 */
	if (!(flags & F_APPEND))
		ps_prologue (stdout);


	/*
	 * Beware the difference between the "dot_width", "dot_height"
	 * pair and the "->width" and "->height" pairs. The former are
	 * the dimensions of the printed image in dots; the latter are the
	 * dimensions of the image we have in memory, in an XImage.
	 */

	if (split) {
		split_height = (dst_image->height + (split - 1)) / split;
		split_dot_height = (dot_height + (split - 1)) / split;
	} else {
		split_height = dst_image->height;
		split_dot_height = dot_height;
	}


	page++;
	begin_page (
		dst_image,
		dst_ncolors,
		dst_colors,
		left_margin,
		top_margin,
		split_dot_height,
		image_scale,
		page,
		flags & F_INVERT
	);

	/*
	 * Put out the header, if any.
	 * The origin is the bottom-left corner of where the image
	 * will be.
	 */
	if (header) {
	    fprintf (
		stdout,
		"gsave\n{ } settransfer\n/%s findfont 15 300 mul 72 div scalefont setfont\n",
		PS_TEXT_FONT
	    );
	    if (*pisportrait) {
		fprintf (
			stdout,
			"%d (%s) stringwidth pop sub 2 div %d moveto\n",
			dot_width * image_scale,
			header,
			split_dot_height * image_scale
			 + (int)(PS_DPI * TEXT_GAP)
		);
	    } else {
		fprintf (stdout, "90 rotate\n");
		fprintf (
			stdout,
			"%d (%s) stringwidth pop sub 2 div %d moveto\n",
			split_dot_height * image_scale,
			header,
			(int)(PS_DPI * TEXT_GAP)
		);
	    }
	    fprintf (stdout, "(%s) show\ngrestore\n", header);
	}

	/*
	 * Now put out the trailer, if any.
	 * The origin is (still) the bottom-left corner of the image.
	 *
	 * MORE: Do this after putting out the image, so that
	 * it appears on the last page of a split output.
	 */
	if (trailer) {
	    fprintf (
		stdout,
		"gsave\n{ } settransfer\n/%s findfont 15 300 mul 72 div scalefont setfont\n",
		PS_TEXT_FONT
	    );
	    if (*pisportrait) {
		fprintf (
			stdout,
			"%d (%s) stringwidth pop sub 2 div -%d moveto\n",
			dot_width * image_scale,
			trailer,
			(int)(PS_DPI * (TEXT_GAP + TEXT_PT / (double)72))
		);
	    } else {
		fprintf (stdout, "90 rotate\n");
		fprintf (
			stdout,
			"%d (%s) stringwidth pop sub 2 div -%d moveto\n",
			split_dot_height * image_scale,
			trailer,
			dot_width * image_scale
			 + (int)(PS_DPI * (TEXT_GAP + TEXT_PT / (double)72))
		);
	    }
	    fprintf (stdout, "(%s) show\n", trailer);
	    fprintf (stdout, "grestore\n");
	}

	for (
		row_start = 0;
		row_start < dst_image->height;
		row_start += split_height
	) {
		/*
		 * Separate the parts of a split image.
		 */
		if (row_start) {
			fprintf (stdout, PS_PAGE_END);
			page++;
			begin_page (
				dst_image,
				dst_ncolors,
				dst_colors,
				left_margin,
				top_margin,
				split_dot_height,
				image_scale,
				page,
				flags & F_INVERT
			);
		}

		fprintf (
			stdout,
			PS_PIXDUMP,
			dot_width,
			split_dot_height,
			image_scale,
			dst_image->bits_per_pixel
		);
		dump_ps_image (
			dst_image,
			0,
			row_start,
			dst_image->width,
			(
			    row_start + split_height > dst_image->height?
				  dst_image->height
				: row_start + split_height
			)
		);
	}

	/*
	 * The margins, image size, and device scale information
	 * are needed for the bounding box.
	 */
	ps_epilogue (
		stdout,
		left_margin,
		top_margin,
		dot_width,
		dot_height,
		image_scale,
		page
	);

	return;
}

/**
 ** page_limit() - LIMIT MARGIN AND IMAGE DIMENSION TO PAGE DIMENSION
 **/

static void		page_limit (image_dim_p, margin_p, page_dim)
	double *		image_dim_p;
	double *		margin_p;
	double			page_dim;
{
	/*
	 * Constrain the image dimension to the page dimension.
	 */
	if (*image_dim_p > page_dim)
		*image_dim_p = page_dim;

	/*
	 * If a margin was given, then if a image dimension was also
	 * given, constrain the margin to the space available. If a
	 * image dimension wasn't given, then compute it from the
	 * space available.
	 */
	if (*margin_p > 0)
		if (*image_dim_p > 0) {
			if (*margin_p + *image_dim_p > page_dim)
				*margin_p = page_dim - *image_dim_p;
		} else
			*image_dim_p = page_dim - *margin_p;

	/*
	 * If we still don't have the image dimension, neither it
	 * nor the margin were given. Let the image fill the page
	 * in the dimension at hand. Leave the margin alone so
	 * we know to compute it AFTER converting the dimensions
	 * to dots (from inches). This keeps the image centered.
	 */
	if (*image_dim_p <= 0)
		*image_dim_p = page_dim;
		
	return;
}

/**
 ** begin_page()
 **/

static void		begin_page (
				dst_image,
				dst_ncolors,
				dst_colors,
				left_margin,
				top_margin,
				height,
				image_scale,
				page,
				invert
			)
	XImage *		dst_image;
	int			dst_ncolors;
	XColor *		dst_colors;
	int			left_margin;
	int			top_margin;
	int			height;
	int			image_scale;
	int			page;
	int			invert;
{
	long			pixel;
	long			upper_pixel;

	fprintf (stdout, PS_PAGE_START, page);

	/*
	 * Place the origin at the top-left corner of the image,
	 * for each page that we print. The header and trailer will
	 * be placed relative to this origin. Save the previous
	 * graphics state so that we can get rid of this origin when
	 * done.
	 */
	fprintf (
		stdout,
		PS_SET_ORIGIN,
		left_margin * image_scale,
		(int)(PS_DPI * PS_PAGE_HEIGHT)
		  - (top_margin + height) * image_scale
	);

	/*
	 * Construct a transfer function that will map the pixel
	 * values into gray scales that match the intensities of
	 * the colors. This is repeated for each PostScript page
	 * to ensure that it is there even if the pages get reversed
	 * before being printed (PostScript interpreters also reset
	 * the transfer function after each page anyway).
	 */
	fprintf (stdout, PS_TRANSFER_START);
	upper_pixel = (1 << dst_image->bits_per_pixel);
	for (pixel = 0; pixel < min(dst_ncolors, upper_pixel); pixel++)
		fprintf (
			stdout,
			PS_TRANSFER_ITEM,
			(double)INTENSITY(&dst_colors[pixel]) / (double)6553600
		);
	for (; pixel < upper_pixel; pixel++)
		fprintf (stdout, PS_TRANSFER_ITEM, (double)1);
	fprintf (
		stdout,
		PS_TRANSFER_END,
		upper_pixel,
		(invert? PS_TRANSFER_INVERT : "")
	);

	return;
}
