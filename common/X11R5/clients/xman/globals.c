/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5xman:globals.c	1.2"

/*
 *	Copyright (c) 1991, 1992 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyright (c) 1988, 1989, 1990 AT&T
 *	All Rights Reserved 
 */

/*
 * xman - X window system manual page display program.
 *
 * $XConsortium: globals.c,v 1.8 91/06/08 18:15:23 rws Exp $
 *
 * Copyright 1987, 1988 Massachusetts Institute of Technology
 *
 *
 * Author:    Chris D. Peterson, MIT Project Athena
 * Created:   October 22, 1987
 */

#include "man.h"

Xman_Resources resources;	/* Resource manager sets these. */

/* bookkeeping global variables. */

Widget help_widget;		/* The help widget. */

int default_height,default_width; /* Approximately the default with and
					    height, of the manpage when shown,
					    the the top level manual page 
					    window */

Manual * manual;		/* The manual structure. */
int sections;			/* The number of manual sections. */

int man_pages_shown;		/* The current number of manual
				   pages being shown, if 0 we exit. */

Widget initial_widget;		/* The initial widget, never realized. */

XContext manglobals_context;	/* The context for man_globals. */
Boolean man_file;
char * man_file_name;
