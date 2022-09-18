#ident	"@(#)xpr:devices/terminfo/ti_map.c	1.3"
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

/*
 * A macro that finds the next integral factor of two values.
 * It assumes the current value of the factor should be skipped.
 * The third argument must be an lvalue.
 */
#define next_factor(A,B,F) \
	if (--F > 0) \
		while ((A) % F || (B) % F) \
			F--; \
	else \
		F = 1

extern char		*tparm();

static void		page_limit();

static int		set_device_scale();

/**
 ** ti_map()
 **/

void			ti_map (
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
	char			*TERM;
	XImage			*src_image;
	int			src_ncolors;
	XColor			*src_colors;
	double			height,
				width,
				left,
				top;
	int			split,
				flags;
	char			*header,
				*trailer;
	int			*pscale,
				*pisportrait;
	char			*color_list;
{
	double			d_page_width,
				d_page_height,
				text_adjust;

	int			dot_width,
				dot_height,
				pix_width,
				pix_height,
				w_comp,
				h_comp,
				w_scale,
				h_scale,
				image_scale,
				text_scale,
				device_scale,
				print_scale, print_hscale, print_wscale,
				our_image_scale,
				our_text_scale, 
				pix_image_hscale, pix_image_wscale,
				pix_text_hscale, pix_text_wscale,
				left_margin,
				top_margin,
				row_start,
				split_height,
				split_dot_height,
				dst_ncolors	= 0,
				invert_text	= 0;

	struct model		*pmodel;

	XImage			image1,
				*dst_image	= &image1,
				*header_image,
				*trailer_image;

	XColor			*dst_colors	= 0;


	read_terminfo_database (TERM);

	if (bitype < 1 || nmodels < bitype) {
		fprintf (
			stderr,
	"xpr: Error: The device \"%s\" has a bad value for bitype.\n",
			TERM
		);
		exit (1);
	}
	pmodel = &models[bitype - 1];


	/*
	 * If the aspect ratio of the device is way out,
	 * we'll compensate by printing a single ``dot''
	 * as a contiguous row or column of device dots.
	 */
	if (spinh < SPINV) {
		h_comp = (int)(((double)SPINV / spinh) + .5);
		w_comp = 1;
	} else if (SPINV < spinh) {
		h_comp = 1;
		w_comp = (int)(((double)spinh / SPINV) + .5);
	} else
		h_comp = w_comp = 1;

	/*
	 * Calculate the size of the page, in inches.
	 * This becomes the default size of the printed image window.
	 */
	d_page_width = (double)(cols * orc) / orhi;
	d_page_height = (double)(lines * orl) / orvi;

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
	 * Note: Although it is conceivable that printers definable
	 * in Terminfo could have the ability to rotate the output,
	 * we don't try to make use of that fact. Perhaps later,
	 * when these issues have been resolved: (1) which rotates
	 * faster, host or device; (2) what happens to various Terminfo
	 * capabilities and parameters when the device rotates.
	 */
	if (flags & F_PORTRAIT)
		*pisportrait = 1;
	else if (flags & F_LANDSCAPE)
		*pisportrait = 0;
	else if (src_image->width == src_image->height)
		*pisportrait = 1;
	else if (src_image->width < src_image->height)
		if (d_page_width <= d_page_height)
			*pisportrait = 1;
		else
			*pisportrait = 0;
	else
		if (d_page_width < d_page_height)
			*pisportrait = 0;
		else
			*pisportrait = 1;


	/*
	 * The header and trailer are printed using a font
	 * we have with us. This allows a (reasonably) consistent
	 * image regardless of the device. We'd like to print
	 * the text in as close to TEXT_PT-point type as possible.
	 * (There are about 72 points to the inch.)
	 *
	 * Note: We need to do this calculation before computing
	 */
	if (header || trailer) {
		double			text_height;


		if (*pisportrait)
			pick_best_font (TEXT_PT, (double)h_comp / SPINV);
		else
			pick_best_font (TEXT_PT, (double)w_comp / spinh);

		pix_height = 0;
		if (
			header
		     && (header_image = text_to_image(header))
		     && pix_height < header_image->height
		)
			pix_height = header_image->height;
		if (
			trailer
		     && (trailer_image = text_to_image(trailer))
		     && pix_height < trailer_image->height
		)
			pix_height = trailer_image->height;

		/*
		 * Calculate what the actual height is, and add room
		 * for a gap to separate the text from the image.
		 * We may have to scale the text images to make them
		 * big enough.
		 */
		if (*pisportrait)
			text_height = ((double)pix_height / SPINV) * h_comp;
		else
			text_height = ((double)pix_height / spinh) * w_comp;
		text_scale = (((double)TEXT_PT / 72) / text_height + .5);
		if (text_scale <= 0)
			text_scale = 1;
		text_adjust = text_height * text_scale + TEXT_GAP;
		if (header && trailer)
			text_adjust *= 2;
	} else {
		text_scale = 0;
		text_adjust = 0;
	}


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
		page_limit (&width, &left, d_page_width);
		page_limit (&height, &top, d_page_height - text_adjust);
	} else {
		page_limit (&width, &left, d_page_height);
		page_limit (&height, &top, d_page_width - text_adjust);
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
		dot_width = spinh * width;
		dot_height = SPINV * height;
		pix_width = src_image->width;
		pix_height = src_image->height;
	} else {
		dot_width = spinh * height;
		dot_height = SPINV * width;
		pix_width = src_image->height;
		pix_height = src_image->width;
	}

	/*
	 * Reduce the APPARENT page size (in dots) by the aspect ratio
	 * compensation, since the effect of the compensation is to change
	 * the bit-map spacing.
	 */
	dot_width /= w_comp;
	dot_height /= h_comp;

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
		int			w_limit	= (spinh * SCALE_LIMIT),
					h_limit	= (SPINV * SCALE_LIMIT);


		if (image_scale > min(w_limit, h_limit))
			image_scale = min(w_limit, h_limit);
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
	 * If the device can scale the image, try to get it to do
	 * the scaling. If the device can scale, but not do the scale
	 * we need, split the scaling between the device and us.
	 * Since both the device and we are only capable of integral
	 * scaling, the split must be along two factors of the desired
	 * scale.
	 */
	device_scale = set_device_scale(
		image_scale,
		text_scale,
		dot_width,
		dot_height
	);
	our_image_scale = image_scale / device_scale;
	our_text_scale = text_scale / device_scale;

	/*
	 * The scaling may be further partitioned between "pix_convert()"
	 * and the printing routines. "pix_convert()" will be called
	 * separately for each of the header, trailer, and image, while
	 * the printing routine will be called for a composite image
	 * containing all three. Thus, any scaling done by the printing
	 * routine has to be an integral factor of the scaling needed
	 * by the text and the image. The aspect ratio compensation
	 * isn't considered here because that is done uniformly.
	 */

	if (header || trailer) {
		print_scale = min(our_image_scale, our_text_scale);
		next_factor (our_image_scale, our_text_scale, print_scale);
	} else
		print_scale = our_image_scale;

	if (pmodel->does_wscale) {
		pix_text_wscale = our_text_scale / print_scale;
		pix_image_wscale = our_image_scale / print_scale;
		print_wscale = print_scale;
	}
	else {
		pix_text_wscale = our_text_scale * w_comp;
		pix_image_wscale = our_image_scale * w_comp;
		print_wscale = 1 * w_comp;
	}

	if (pmodel->does_hscale) {
		pix_text_hscale = our_text_scale / print_scale;
		pix_image_hscale = our_image_scale / print_scale;
		print_hscale = print_scale;
	}
	else {
		pix_text_hscale = our_text_scale * h_comp;
		pix_image_hscale = our_image_scale * h_comp;
		print_hscale = 1 * h_comp;
	}

	/*
	 * Initialize the output routine; this will also set the
	 * best storage format for the intermediate image, and will
	 * set the range of output colors available.
	 */
	*dst_image = *src_image; /* Original storage may be suitable */
	(*pmodel->set_data_storage) (
		dst_image,
		&dst_ncolors,
		&dst_colors,
		src_image,
		src_ncolors,
		src_colors,
		color_list,
		&invert_text
	);
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
		flags & F_INVERT,
		pix_image_wscale,
		pix_image_hscale
	);

	/*
	 * Rotate and scale the header and trailer.
	 */
	if (header) {
		static XImage		image2;

		XImage			*pi	= &image2;


		/*
		 * Keep the same image storage. It should be a bitmap,
		 * which is easily ``converted'' to a pixmap (if needed)
		 * using the bitblt operator later.
		 */
		*pi = *header_image;

		pix_convert(
			header_image,
			0,
			(XColor *)0,
			pi,
			0,
			(XColor *)0,
			!*pisportrait,
			invert_text,
			pix_text_wscale,
			pix_text_hscale
		);
		header_image = pi;
	}
	if (trailer) {
		static XImage		image3;

		XImage			*pi	= &image3;


		/*
		 * Keep the same image storage. It should be a bitmap,
		 * which is easily ``converted'' to a pixmap (if needed)
		 * using the bitblt operator later.
		 */
		*pi = *trailer_image;

		pix_convert(
			trailer_image,
			0,
			(XColor *)0,
			pi,
			0,
			(XColor *)0,
			!*pisportrait,
			invert_text,
			pix_text_wscale,
			pix_text_hscale
		);
		trailer_image = pi;
	}

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
	if (header && header_image->width > dot_width)
		header_image->width = dot_width;
	if (trailer && trailer_image->width > dot_width)
		trailer_image->width = dot_width;

	/*
	 * Concatenate the header, image, and trailer to make a
	 * single image. Leave a 1/4 inch gap between each.
	 */
	if (header || trailer) {
		static XImage		image4;

		XImage			*pi	= &image4;

		int			x,
					y,
					gap;


#define	ROUND2(N,D)	((double)(N) / (double)(D) + .5)

		/*
		 * Use a device compatible storage technique.
		 */
		*pi = *dst_image;

		if (*pisportrait) {
			gap = ROUND2(
				TEXT_GAP * SPINV,
				h_comp * print_scale * device_scale
			);

			pi->height = dst_image->height;
			if (header)
				pi->height += header_image->height + gap;
			if (trailer)
				pi->height += trailer_image->height + gap;

			pi->width = dst_image->width;
			if (header && pi->width < header_image->width)
				pi->width = header_image->width;
			if (trailer && pi->width < trailer_image->width)
				pi->width = trailer_image->width;

			pi->bytes_per_line = BYTES_PER_LINE(pi);
			pi->data = Calloc(IMAGE_SIZE(pi), 1);

#define CENTER_X(I1,I2,Y) \
	image_bitblt( \
		(I1), 0, 0, (I1)->width, (I1)->height, \
		(I2), ((I2)->width - (I1)->width) / 2, (Y) \
	)

			y = 0;
			if (header) {
				CENTER_X (header_image, pi, y);
				y += header_image->height + gap;
			}
			CENTER_X (dst_image, pi, y);
			y += dst_image->height + gap;
			if (trailer)
				CENTER_X (trailer_image, pi, y);
		} else {
			gap = ROUND2(
				TEXT_GAP * spinh,
				w_comp * print_scale * device_scale
			);

			pi->width = dst_image->width;
			if (header)
				pi->width += header_image->width + gap;
			if (trailer)
				pi->width += trailer_image->width + gap;

			pi->height = dst_image->height;
			if (header && pi->height < header_image->height)
				pi->height = header_image->height;
			if (trailer && pi->height < trailer_image->height)
				pi->height = trailer_image->height;

			pi->bytes_per_line = BYTES_PER_LINE(pi);
			pi->data = Calloc(IMAGE_SIZE(pi), 1);

#define CENTER_Y(I1,I2,X) \
	image_bitblt( \
		(I1), 0, 0, (I1)->width, (I1)->height, \
		(I2), (X), ((I2)->height - (I1)->height) / 2 \
	)

			x = 0;
			if (header) {
				CENTER_Y (header_image, pi, x);
				x += header_image->width + gap;
			}
			CENTER_Y (dst_image, pi, x);
			x += dst_image->width + gap;
			if (trailer)
				CENTER_Y (trailer_image, pi, x);
		}

		dst_image = pi;
	}


	/*
	 * Re-calculate the printed image size in ``dots''. These
	 * dots are what the device will print, perhaps scaled up
	 * to larger dots (and spaced accordingly) by "device_scale".
	 */
	dot_width = dst_image->width *
		(pmodel->does_wscale? w_comp * print_scale : 1);
	dot_height = dst_image->height *
		(pmodel->does_hscale? h_comp * print_scale : 1);

	/*
	 * Calculate the left and top margins for the output
	 * image, in printed dots. Again, these dots may be scaled
	 * (by the device) by the factor "device_scale".
	 *
	 * Note: The Terminfo capabilities assume numbering starts
	 * with zero.
	 */
	if (left < 0) {
		double			d_image_width;	/* inches */


		d_image_width = (double)(dot_width * device_scale) / spinh;
		left = (d_page_width - d_image_width) / 2;
	}
	if (top < 0) {
		double			d_image_height;	/* inches */


		d_image_height = (double)(dot_height * device_scale) / SPINV;
		top = (d_page_height - d_image_height) / 2;
	}
	left_margin = (left * spinh) / device_scale;
	top_margin = (top * SPINV) / device_scale;


	if (flags & F_NOFF)
		ti_backup_epilogue (TERM, stdout);

	/*
	 * Don't put out the initialization control sequences
	 * for an appended file, as we assume the file already
	 * has them.
	 */
	if (!(flags & F_APPEND))
		ti_prologue (TERM, stdout);


	/*
	 * Beware the difference between the "dot_width", "dot_height"
	 * pair and the "->width" and "->height" pairs. The former are
	 * the dimensions of the printed image in dots; the latter are the
	 * dimensions of the image we have in memory, in a bitmap.
	 */

	if (split) {
		split_height = (dst_image->height + (split - 1)) / split;
		split_dot_height = (dot_height + (split - 1)) / split;
	} else {
		split_height = dst_image->height;
		split_dot_height = dot_height;
	}

	for (
		row_start = 0;
		row_start < dst_image->height;
		row_start += split_height
	) {
		/*
		 * Separate the parts of a split image with formfeeds.
		 */
		if (row_start)
			if (OKAY(ff))
				putp (ff);
/* MORE */		/* else
				simulate formfeed */

		start_bit_image_graphics (
			left_margin,
			top_margin,
			dot_width,
			split_dot_height,
			device_scale,
			dst_image->depth
		);
		(*pmodel->dump_image) (
			dst_image,
			0,
			row_start,
			dst_image->width,
			(
			    row_start + split_height > dst_image->height?
				  dst_image->height
				: row_start + split_height
			),
			print_wscale,
			print_hscale
		);
		stop_bit_image_graphics ();

	}

	/*
	 * The margin, image size, and device scale information
	 * are needed for PostScript printers.
	 */
	ti_epilogue (
		TERM,
		stdout,
		left_margin,
		top_margin,
		dot_width,
		dot_height,
		device_scale
	);

	return;
}

/**
 ** page_limit() - LIMIT MARGIN AND IMAGE DIMENSION TO PAGE DIMENSION
 **/

static void		page_limit (image_dim_p, margin_p, page_dim)
	double			*image_dim_p,
				*margin_p,
				page_dim;
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
 ** set_device_scale()
 **/

static int		set_device_scale (image_scale, text_scale, dot_width, dot_height)
	int			image_scale,
				text_scale,
				dot_width,
				dot_height;
{
	int			device_scale;

	/*
	 * Note: The scale for the image and the scale for the header
	 * and trailer may differ. However, the image sent to the device
	 * will be a composite that includes the header and trailer.
	 * Thus, the scaling done by the device must be an integral factor
	 * of the scaling needed by BOTH the header/trailer and the image.
	 *
	 * Note: Don't include the aspect ratio compensation here.
	 * We assume that if the device can scale it is probably a
	 * clever device (i.e. a laser printer) that has a 1:1 aspect
	 * ratio (and thus no compensation). In other words, we'll do
	 * the compensation internally, since it isn't symmetric.
	 */
	if (OKAY(defbi)) {
		int			_text_scale	=
				(text_scale? text_scale : image_scale);
					/*
					 * If no header/trailer, then we
					 * can ``ignore'' the text scaling
					 * by using "image_scale" in its
					 * place.
					 */


		/*
		 * The first guess for the device scale is the smaller
		 * of the two scales needed. Add 1 because "next_factor()"
		 * skips the initial value.
		 */
		device_scale = min(image_scale, _text_scale) + 1;
		next_factor (image_scale, _text_scale, device_scale);

		while (
			device_scale > 1
		     && !*tparm(defbi, 0, 0, dot_width, dot_height, device_scale)
		)
			next_factor (image_scale, _text_scale, device_scale);
	} else
		device_scale = 1;

	return (device_scale);
}
