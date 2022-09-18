/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/v256color.c	1.1"

/*
 *	Copyright (c) 1991 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 *	All rights reserved.
 */

#include "Xmd.h"
#include "sidep.h"
#include "miscstruct.h"
#include "sys/types.h"
#include "sys/at_ansi.h"
#include "sys/kd.h"
#include "vtio.h"
#include "v256.h"



/*
 *	v256_set_cmap(visual, cmap, colors, count)	-- fill in a colormap.
 *
 *	Input:
 *		int	visual		-- index of the visual being updated
 *		int	cmap		-- index of the colormap being updated
 *		SIcolor	*colors		-- colors to be updated
 *		int	count		-- number of colors being updated
 */
v256_set_cmap(visual, cmap, colors, count)
int visual;
int cmap;
register SIColor *colors;
register int count;
{
	while (count--)
		v256_set_color(*colors++);
}


/*
 *	v256_set_color(color)	-- determine the V256 pallette value
 *				for an attribute register based on the 
 *				color structure passed in.
 *
 *	Input:
 *		SIColor	color	-- the index and RGB values for the color
 */
v256_set_color(color)
SIColor color;
{
	register int	i;

	i = color.SCpindex;

	v256_pallette[i].red   = (color.SCred   >> 10) & 0x3f;
	v256_pallette[i].green = (color.SCgreen >> 10) & 0x3f;
	v256_pallette[i].blue  = (color.SCblue  >> 10) & 0x3f;

	v256_color(i, v256_pallette[i].red,
		     v256_pallette[i].green,
		     v256_pallette[i].blue);
}

v256_get_color(color)
SIColor *color;
{
	register i;

	i = color->SCpindex;
	color->SCred   = v256_pallette[i].red << 10;
	color->SCgreen = v256_pallette[i].green << 10;
	color->SCblue  = v256_pallette[i].blue << 10;
}



/*
 *	v256_set_attrs()	-- set up all the attribute registers based on 
 *			the current color map.
 */
v256_set_attrs()
{
	int i;

	for (i = 0; i < V256_MAXCOLOR; i++)  {
		v256_color(i, v256_pallette[i].red,
			     v256_pallette[i].green,
			     v256_pallette[i].blue);
	}
}
