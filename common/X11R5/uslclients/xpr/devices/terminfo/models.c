#ident	"@(#)xpr:devices/terminfo/models.c	1.3"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "xpr_term.h"

/*
 * Most models can't do better than "enlarge()" in scaling
 * vertically.
 */
struct model		models[] = {
	{ init_image_to_cells, image_to_cells, 1, 0 },
	{ init_image_to_bits,  image_to_bits,  0, 0 },
	{ init_image_to_bits,  image_to_bits,  0, 0 },
};

int			nmodels	 = 3;
