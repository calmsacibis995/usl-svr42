#ident	"@(#)xpr:invert.c	1.2"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "Xlib.h"

/**
 ** bit_invert() - PERFORM ``REVERSE VIDEO'' ON IMAGE
 **/

void			bit_invert (image)
	XImage			*image;
{
	register int		y,
				nbytes;

	register char		*pd	= image->data;


	for (y = 0; y < image->height; y++)
		for (nbytes = image->bytes_per_line; nbytes--; pd++)
			*pd = ~*pd;
	return;
}
