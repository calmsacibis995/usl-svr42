/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga16:vga16/vgacolor.c	1.4"

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
#include "vga.h"



/*
 *	vga_set_cmap(visual, cmap, colors, count)	-- fill in a colormap.
 *
 *	Input:
 *		int	visual		-- index of the visual being updated
 *		int	cmap		-- index of the colormap being updated
 *		SIcolor	*colors		-- colors to be updated
 *		int	count		-- number of colors being updated
 */
SIBool
vga_set_cmap(visual, cmap, colors, count)
int visual;
int cmap;
register SIColor *colors;
register int count;
{
	while (count--)
		vga_set_color(*colors++);
	return(SI_SUCCEED);
}



/*
 *	vga_set_color(color)	-- determine the EGA/VGA pallette value
 *				for an attribute register based on the 
 *				color structure passed in.
 *
 *	Input:
 *		SIColor	color	-- the index and RGB values for the color
 */
vga_set_color(color)
SIColor color;
{
	register int	i;

	i = vga_attr_map[color.SCpindex];

	if (vt_info.is_vga) {
		vga_pallette[i].red   = (color.SCred   >> 10) & 0x3f;
		vga_pallette[i].green = (color.SCgreen >> 10) & 0x3f;
		vga_pallette[i].blue  = (color.SCblue  >> 10) & 0x3f;

		vga_color(i, vga_pallette[i].red,
			     vga_pallette[i].green,
			     vga_pallette[i].blue);
	}
	else {
		ega_cmap[i] = 0;

		if (color.SCred   & 0x4000) ega_cmap[i] |= 0x20;
		if (color.SCred   & 0x8000) ega_cmap[i] |= 0x04;
		if (color.SCgreen & 0x4000) ega_cmap[i] |= 0x10;
		if (color.SCgreen & 0x8000) ega_cmap[i] |= 0x02;
		if (color.SCblue  & 0x4000) ega_cmap[i] |= 0x08;
		if (color.SCblue  & 0x8000) ega_cmap[i] |= 0x01;

		ega_color(i, ega_cmap[i]);
	}
}



/*
 *	vga_set_attrs()	-- set up all the attribute registers based on 
 *			the current color map.
 */
vga_set_attrs()
{
	int i, j;

	if (vt_info.is_vga) {
		for (i = 0; i < vt_info.colors; i++)  {
			j = vga_attr_map[i];
			vga_color(j, vga_pallette[j].red,
				     vga_pallette[j].green,
				     vga_pallette[j].blue);
		}
	}
	else {
		for (i = 0; i < vt_info.colors; i++)  {
			j = vga_attr_map[i];
			ega_color(j, ega_cmap[j]);
		}
	}
}
