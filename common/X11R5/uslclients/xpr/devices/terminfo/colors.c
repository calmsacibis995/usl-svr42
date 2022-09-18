#ident	"@(#)xpr:devices/terminfo/colors.c	1.2"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "stdio.h"

#include "Xlib.h"

#include "xpr_term.h"

extern char		*tparm();

/**
 ** map_colors() - CONSTRUCT NEW COLOR MAP FOR OUTPUT DEVICE
 **/

/*
 * This routine determines the color map based on either the
 * Terminfo capability "colornm" or a supplied list. If the
 * printer doesn't have a color capability, or the color list
 * only gives the colors black and white (at most), then the
 * returned color map will be empty.
 */

void			map_colors (pncolors, pcolors, color_list)
	int			*pncolors;
	XColor			**pcolors;
	char			*color_list;
{
	if (colors <= 0 || bitype != 1)
		goto NoColors;

	if (color_list)
		parse_color (color_list, pncolors, pcolors);
	else {
		int			band;

		char			*color;


		for (band = 1; band <= colors; band++) {
			color = tparm(colornm, band);
			if (OKAY(color))
				parse_color (color, pncolors, pcolors);
		}
		do {
			color = tparm(colornm, band++);
			if (OKAY(color))
				parse_color (color, pncolors, pcolors);
		} while (OKAY(color));
	}

	/*
	 * We rely on "parse_color()" automatically adding black and
	 * white to the color map. Thus if only two colors are in the
	 * map, they must be black and white, and this printer (or the
	 * ribbon at hand) doesn't really support color.
	 */
	if (*pncolors <= 2) {
NoColors:	*pncolors = 0;
		*pcolors = 0;
	}

	return;
}

/**
 ** colornames[]
 **/

struct colorname	colornames[] = {
	112, 219, 147,	"aquamarine",		"Aquamarine",
	 50, 204, 153,	"medium aquamarine",	"MediumAquamarine",
	  0,   0,   0,	"black",		"Black",
	  0,   0, 255,	"blue",			"Blue",
	 95, 159, 159,	"cadet blue",		"CadetBlue",
	 66,  66, 111,	"cornflower blue",	"CornflowerBlue",
	107,  35, 142,	"dark slate blue",	"DarkSlateBlue",
	191, 216, 216,	"light blue",		"LightBlue",
	143, 143, 188,	"light steel blue",	"LightSteelBlue",
	 50,  50, 204,	"medium blue",		"MediumBlue",
	127,   0, 255,	"medium slate blue",	"MediumSlateBlue",
	 47,  47,  79,	"midnight blue",	"MidnightBlue",
	 35,  35, 142,	"navy blue",		"NavyBlue",
	 35,  35, 142,	"navy",			"Navy",
	 50, 153, 204,	"sky blue",		"SkyBlue",
	  0, 127, 255,	"slate blue",		"SlateBlue",
	 35, 107, 142,	"steel blue",		"SteelBlue",
	255, 127,   0,	"coral",		"Coral",
	  0, 255, 255,	"cyan",			"Cyan",
	142,  35,  35,	"firebrick",		"Firebrick",
	165,  42,  42,	"brown",		"Brown",
	204, 127,  50,	"gold",			"Gold",
	219, 219, 112,	"goldenrod",		"Goldenrod",
	234, 234, 173,	"medium goldenrod",	"MediumGoldenrod",
	  0, 255,   0,	"green",		"Green",
	 47,  79,  47,	"dark green",		"DarkGreen",
	 79,  79,  47,	"dark olive green",	"DarkOliveGreen",
	 35, 142,  35,	"forest green",		"ForestGreen",
	 50, 204,  50,	"lime green",		"LimeGreen",
	107, 142,  35,	"medium forest green",	"MediumForestGreen",
	 66, 111,  66,	"medium sea green",	"MediumSeaGreen",
	127, 255,   0,	"medium spring green",	"MediumSpringGreen",
	143, 188, 143,	"pale green",		"PaleGreen",
	 35, 142, 107,	"sea green",		"SeaGreen",
	  0, 255, 127,	"spring green",		"SpringGreen",
	153, 204,  50,	"yellow green",		"YellowGreen",
	 47,  79,  79,	"dark slate grey",	"DarkSlateGrey",
	 47,  79,  79,	"dark slate gray",	"DarkSlateGray",
	 84,  84,  84,	"dim grey",		"DimGrey",
	 84,  84,  84,	"dim gray",		"DimGray",
	168, 168, 168,	"light grey",		"LightGrey",
	168, 168, 168,	"light gray",		"LightGray",
	192, 192, 192,	"gray",			"grey",
	192, 192, 192,	"Gray",			"Grey",
	159, 159,  95,	"khaki",		"Khaki",
	255,   0, 255,	"magenta",		"Magenta",
	142,  35, 107,	"maroon",		"Maroon",
	204,  50,  50,	"orange",		"Orange",
	219, 112, 219,	"orchid",		"Orchid",
	153,  50, 204,	"dark orchid",		"DarkOrchid",
	147, 112, 219,	"medium orchid",	"MediumOrchid",
	188, 143, 143,	"pink",			"Pink",
	234, 173, 234,	"plum",			"Plum",
	255,   0,   0,	"red",			"Red",
	 79,  47,  47,	"indian red",		"IndianRed",
	219, 112, 147,	"medium violet red",	"MediumVioletRed",
	255,   0, 127,	"orange red",		"OrangeRed",
	204,  50, 153,	"violet red",		"VioletRed",
	111,  66,  66,	"salmon",		"Salmon",
	142, 107,  35,	"sienna",		"Sienna",
	219, 147, 112,	"tan",			"Tan",
	216, 191, 216,	"thistle",		"Thistle",
	173, 234, 234,	"turquoise",		"Turquoise",
	112, 147, 219,	"dark turquoise",	"DarkTurquoise",
	112, 219, 219,	"medium turquoise",	"MediumTurquoise",
	 79,  47,  79,	"violet",		"Violet",
	159,  95, 159,	"blue violet",		"BlueViolet",
	216, 216, 191,	"wheat",		"Wheat",
	252, 252, 252,	"white",		"White",
	255, 255,   0,	"yellow",		"Yellow",
	147, 219, 112,	"green yellow",		"GreenYellow",

	  0,   0,    0,	0,			0
};
