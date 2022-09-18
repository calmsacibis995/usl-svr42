#ident	"@(#)xpr:xpr.c	1.3"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* #include <X11/copyright.h> */
#include "copyright.h"

/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "stdio.h"

#include "Xlib.h"

#include "xpr.h"

char			*input_filename	= "(stdin)";

/**
 ** main()
 **/

int			main (argc, argv)
	int			argc;
	char			**argv;
{
	int			scale		= 0,
				flags		= 0,
				split		= 0,
				isportrait	= 0,
				ncolors;

	double			width		= 0,
				height		= 0,
				left		= 0,
				top		= 0;

	char			*header		= 0,
				*trailer	= 0,
				*color_list	= 0;

	Device			*device;

	XImage			*image;

	XColor			*colors;


	parse_args (
		argc,
		argv,
		&scale,
		&width,
		&height,
		&left,
		&top,
		&device, 
		&flags,
		&split,
		&header,
		&trailer,
		&color_list
	);

	read_image (0, &image, &ncolors, &colors);
	if (image->width <= 0 || image->height <= 0) {
		fprintf (
			stderr,
			"xpr: Degenerate image (no width or no height).\n"
		);
		exit (1);
	}

	(*(device->map)) (
		device->name,
		image,
		ncolors,
		colors,
		height,
		width,
		left,
		top,
		split,
		flags,
		header,
		trailer,
		&scale,
		&isportrait,
		color_list
	);
    
	if (flags & F_REPORT) {
		fprintf (
			stderr,
			"Width: %d, Height: %d\n",
			image->width,
			image->height
		);
		fprintf (
			stderr,
			"Orientation: %s, Scale: %d\n",
			isportrait? "Portrait" : "Landscape",
			scale
		);
	}

	exit (0);
	/*NOTREACHED*/
}
