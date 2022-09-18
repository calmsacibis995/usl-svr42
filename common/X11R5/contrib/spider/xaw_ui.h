/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4spider:xaw_ui.h	1.2"

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
 *	Spider
 *
 *	(c) Copyright 1989, Donald R. Woods and Sun Microsystems, Inc.
 *	(c) Copyright 1990, David Lemke and Network Computing Devices Inc.
 *
 *	See copyright.h for the terms of the copyright.
 *
 *	@(#)xaw_ui.h	2.1	90/04/25
 *
 */

/*
 * Athena widget interface definitions
 */

#include	<X11/Intrinsic.h>
#include	<X11/StringDefs.h>

#include	<X11/Xaw/Command.h>
#include	<X11/Xaw/Box.h>
#include	<X11/Xaw/AsciiText.h>
#include	<X11/Xaw/Paned.h>
#include	<X11/Xaw/Viewport.h>
#include	<X11/Xaw/MenuButton.h>
#include	<X11/Xaw/SimpleMenu.h>
#include	<X11/Xaw/Sme.h>
#include	<X11/Xaw/SmeBSB.h>

#include	<X11/Xaw/Cardinals.h>

/*
 *  WIPRO : Neeti
 *  CHANGE # UNKNOWN
 *  FILE # xaw_ui.h
 *  All functions used with XtAddCallback() and XtAddEventHandler() are now
 *  declared as void .
 *  ENDCHANGE # UNKNOWN
 */

extern	void	score_handler(),
			backup_handler(),
			expand_handler(),
			locate_handler(),
			file_handler(),
			help_handler(),
			change_help(),
			newgame_handler(),
			confirm_callback();

extern	void	xaw_redraw_table(),
			xaw_button_press(),
			xaw_button_release(),
			xaw_resize(),
			xaw_key_press();

extern Widget		create_help_popup();

extern	Widget	file,
		toplevel,
		confirm_box,
		confirm_label,
		helptext;
