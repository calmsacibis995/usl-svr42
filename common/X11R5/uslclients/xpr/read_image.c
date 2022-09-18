#ident	"@(#)xpr:read_image.c	1.2"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "stdio.h"
#include "errno.h"

#include "X.h"
#include "Xlib.h"
#include "XWDFile.h"

#include "xpr.h"

extern void		convert_shorts(),
			convert_longs();

/**
 ** read_image() - READ XWDFILE AND CONVERT TO IMAGE
 **/

void			read_image (fd, pimage, pncolors, pcolors)
	int			fd;
	XImage			**pimage;
	int			*pncolors;
	XColor			**pcolors;
{
	int			n,
				len;

	XWDFileHeader		fh;

	XImage			*pi;

	XColor			*pc;


	pi = (XImage *)Malloc(sizeof(XImage));

	Read (fd, (char *)&fh, sizeof(XWDFileHeader));
	if (endian() != MSBFirst)
		convert_longs (&fh, sizeof(XWDFileHeader) / 4);

	if (fh.file_version != XWD_FILE_VERSION) {
		fprintf (stderr, "xpr: File format version mismatch.\n");
		exit (1);
	}
	if (fh.header_size < sizeof(fh)) {
		fprintf (stderr, "xpr: Header size is too small.\n");
		exit (1);
	}

	pi->width = fh.pixmap_width;
	pi->height = fh.pixmap_height;
	pi->xoffset = fh.xoffset;
	pi->format = fh.pixmap_format;
	pi->byte_order = fh.byte_order;
	pi->bitmap_unit = fh.bitmap_unit;
	pi->bitmap_bit_order = fh.bitmap_bit_order;
	pi->bitmap_pad = fh.bitmap_pad;
	pi->depth = fh.pixmap_depth;
	pi->bytes_per_line = fh.bytes_per_line;
	pi->bits_per_pixel = fh.bits_per_pixel;

	_XInitImageFuncPtrs (pi);

	*pimage = pi;

	/*
	 * The ``window name''; we don't use it.
	 */
	len = fh.header_size - sizeof(XWDFileHeader);
	Read (0, Malloc(len), len);

	if ((*pncolors = fh.ncolors)) {
		len = fh.ncolors * sizeof(XColor);
		pc = (XColor *)Malloc(len);
		Read (0, (char *)pc, len);
		if (endian() != MSBFirst)
			for (n = 0; n < fh.ncolors; n++) {
				convert_longs (&(pc[n].pixel), 1);
				convert_shorts (&(pc[n].red), 3);
			}
		*pcolors = pc;
	} else
		*pcolors = 0;

	len = pi->bytes_per_line * pi->height;
	if (pi->format != ZPixmap)
		len *= pi->depth;
	pi->data = Malloc(len);
	Read (0, pi->data, len);
	/*
	 * Instead of converting the image data here, we'll do it
	 * as part of the more comprehensive conversion later.
	 * (Note that it is possible the image data is already in
	 * ``native'' form, even if the header data was not.)
	 */

	return;
}
