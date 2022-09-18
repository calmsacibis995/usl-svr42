/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga16:vga16/vgaconfig.c	1.10"

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

extern struct at_disp_info disp_info[];
extern int vga_num_disp;


/*
 *	vga_config(cfg, dpix, dpiy, colors) -- Determine the VT type in use 
 *					based on the info section of the 
 *					config structure passed in.
 *
 *	Input:
 *		SIConfigP	cfg	-- config structure
 *		int		*dpix	-- pointer to dots per inch X
 *		int		*dpiy	-- pointer to dots per inch Y
 *		int		*colors	-- pointer to number of colors
 *
 *	Returns:
 *		The index into the disp_info table or -1 if we can't 
 *		figure out the type.  dpix and dpiy are filled in.
 */
vga_config(cfg, dpix, dpiy, colors)
SIConfigP cfg;
int *dpix, *dpiy;
int *colors;
{
	int	xpix, ypix, tmp_xpix, tmp_ypix;
	int	type, i;
	struct	at_disp_info *disp;
	float	sizex, sizey;
	char	entry[30];
	char	monitor[30];

	sscanf(cfg->info, "%s %s %dx%d %d %fx%f", entry, monitor,
			&xpix, &ypix, colors, &sizex, &sizey);

	type = -1;
	switch (*colors) {
		case 2:
		case 4:
		case 16:
			break;
		default:
			ErrorF("Number of colors must be 2, 4, or 16.\n");
			return(-1);
	}
		
	for (i = 0, disp = disp_info; i < vga_num_disp; i++, disp++) {
		if ((xpix == disp->xpix) &&
		    (ypix == disp->ypix) &&
		    (strcmp(entry, disp->entry) == 0) &&
		    (strcmp(monitor, disp->monitor) == 0) &&
		    (*colors <= disp->colors)) {
			type = i;
			break;
		}
	}

	if (type == -1) {			
	    ErrorF("\nCannot support display entry : %s \n\
		monitor      : %s \n\
		resolution   : %dx%d and %d colors.\n", entry, monitor, xpix, ypix, colors);
	    ErrorF("\nValid Entries for the current init driver (/usr/X/lib/libv16i.so.1) :: \n");
	    ErrorF("\n%15s %15s  %s %s\n","Entry", "Monitor", "Resolution", "# Colors");
	    ErrorF("%15s %15s  %s %s\n","=====", "=======", "==========", "========");
	    for (i = 0, disp = disp_info; i < vga_num_disp; i++, disp++) {
		ErrorF("%15s %15s  %5d %4d %6d\n",disp->entry, disp->monitor, disp->xpix,disp->ypix, disp->colors);
	    }
	    exit ();
	}
	else { /* found something */
		/*
		 * Figure out the dots per inch in the X and Y dimemsions.  This
		 * is complicated by the fact that on a Panning mode, the number
		 * of pixels described in the disp_info structure is more than
		 * what's visible on the screen.  
		 */
		tmp_xpix = xpix;
		tmp_ypix = ypix;
		switch(disp->vt_type) {
		case VT_EGAPAN_6:
		case VT_EGAPAN_8:
		case VT_EGAPAN_1:
			tmp_xpix = 640;
			tmp_ypix = 350;
			break;

		default:
		   if ( (xpix>0) && (ypix>0) ) {
			tmp_xpix = xpix;
			tmp_ypix = ypix;
		   }
		   else {
			tmp_xpix = 640;
			tmp_ypix = 480;
		   }
		   break;
		}

		*dpix = (float)tmp_xpix / sizex;
		*dpiy = (float)tmp_ypix / sizey;
	}
	return(type);
}
