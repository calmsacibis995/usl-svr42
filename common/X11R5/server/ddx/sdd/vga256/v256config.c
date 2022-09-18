/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/v256config.c	1.4"

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

extern struct at_disp_info disp_info[];
extern int v256_num_disp;


/*
 *	v256_config(cfg, dpix, dpiy)	-- Determine the VT type in use 
 *					based on the info section of the 
 *					config structure passed in.
 *
 *	Input:
 *		SIConfigP	cfg	-- config structure
 *		int		*dpix	-- pointer to dots per inch X
 *		int		*dpiy	-- pointer to dots per inch Y
 *
 *	Returns:
 *		The index into the disp_info table or -1 if we can't 
 *		figure out the type.  dpix and dpiy are filled in.
 */
int
v256_config(cfg, dpix, dpiy)
SIConfigP cfg;
int *dpix, *dpiy;
{
	int	xpix, ypix;
	int	type, i;
	struct	at_disp_info *disp;
	float	sizex, sizey;
	char	entry[30];
	char	monitor[30];

	/*
	 * extract config info 
	 */
	sscanf(cfg->info, "%s %s %dx%d %fx%f", entry, monitor, &xpix, &ypix,
			&sizex, &sizey);
	
	type = -1;

	/*
	 * search for matching entry
	 */
	for (i = 0, disp = disp_info; i < v256_num_disp; i++, disp++) 
	{
		if ((xpix == disp->xpix) &&
		    (ypix == disp->ypix) &&
		    (strcmp(entry, disp->entry) == 0) &&
		    (strcmp(monitor, disp->monitor) == 0)) 
		{
			type = i;
			break;
		}
	}

	if (type != -1) 
	{			/* must have found something */
		*dpix = (float)xpix / sizex;
		*dpiy = (float)ypix / sizey;
	}
	else
	{
	    ErrorF("\nCannot support display entry : %s \n\
		monitor      : %s \n\
		resolution   : %dx%d and 256 colors.\n", entry, monitor, xpix, ypix);
	    ErrorF("\nValid Entries for the current init driver (/usr/X/lib/libv256i.so.1) :: \n");
	    ErrorF("\n%15s %15s  %s\n","Entry", "Monitor", "Resolution");
	    ErrorF("%15s %15s  %s\n","=====", "=======", "==========");
	    for (i = 0, disp = disp_info; i < v256_num_disp; i++, disp++) {
		ErrorF("%15s %15s  %d %d\n",disp->entry, disp->monitor, disp->xpix,disp->ypix);
	    }
	    exit ();
	}

	return(type);
}
