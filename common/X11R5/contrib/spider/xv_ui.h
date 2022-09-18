/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4spider:xv_ui.h	1.1"
/*
 *      Spider
 *
 *      (c) Copyright 1989, Donald R. Woods and Sun Microsystems, Inc.
 *      (c) Copyright 1990, David Lemke and Network Computing Devices Inc.
 *      (c) Copyright 1990, Heather Rose and Sun Microsystems, Inc.
 *
 *      See copyright.h for the terms of the copyright.
 *
 *      @(#)xv_ui.h	2.2	90/04/27
 */
#ifndef	spider_HEADER
#define	spider_HEADER

#include "defs.h"
#include "globals.h"

#define HELP_MIN	0
#define HELP_INTRO	0
#define HELP_RULES	1
#define HELP_CNTRLS	2
#define HELP_EXS	3
#define HELP_MISC	4
#define HELP_SUM	5
#define HELP_MAX	6

#ifndef SPIDER_DEFAULTS
#define SPIDER_DEFAULTS	".spiderrc"
#endif

#ifndef HELPDIR
#define HELPDIR		"/usr/lib/help"
#endif

#ifndef SPIDER_NAME
#define SPIDER_NAME "Spider"
#endif

typedef struct {
        Xv_opaque       window2;
        Xv_opaque       controls2;
        Xv_opaque       button8;
        Xv_opaque       button9;
        Xv_opaque       button10;
        Xv_opaque       button11;
        Xv_opaque       button12;
        Xv_opaque       button13;
        Xv_opaque       button14;
        Xv_opaque       textsw1;
} spider_subwindow1_objects;

typedef struct {
        Xv_opaque       window3;
        Xv_opaque       controls3;
	Xv_opaque	choice1;
	Xv_opaque	numtext1;
#ifdef ROUND_CARDS
	Xv_opaque	choice2;
#endif
	Xv_opaque	choice3;
	Xv_opaque	slider1;
	Xv_opaque	slider2;
	Xv_opaque	textfield2;
	Xv_opaque	textfield3;
        Xv_opaque       button15;
        Xv_opaque       button16;
        Xv_opaque       button17;
} spider_subwindow2_objects;

typedef struct {
	int	bell;
	int	replayTime;
#ifdef ROUND_CARDS
	int	roundCards;
#endif
	int	deltaMod;
	int	confirm;
	int	textField;
	char	*helpDir;
	char	*instanceName;
} spider_defaults_objects;

typedef struct {
	Xv_opaque	window1;
	Xv_opaque	controls1;
	Xv_opaque	button1;
	Xv_opaque	button2;
	Xv_opaque	button3;
	Xv_opaque	button4;
	Xv_opaque	button5;
	Xv_opaque	button6;
	Xv_opaque	button7;
	Xv_opaque	textfield1;
	Xv_opaque	canvas1;
	spider_subwindow1_objects	*subwindow1;
	spider_subwindow2_objects	*subwindow2;
	spider_defaults_objects		*defaults;
} spider_window1_objects;

extern int	INSTANCE, HELPKEY;

extern Xv_opaque	spider_BackUpMenu_create();
extern Xv_opaque	spider_HelpMenu_create();
extern Xv_opaque	spider_FileMenu_create();

extern spider_defaults_objects	*spider_defaults_objects_initialize();

extern spider_window1_objects	*spider_window1_objects_initialize();
extern Xv_opaque	spider_window1_window1_create();
extern Xv_opaque	spider_window1_controls1_create();
extern Xv_opaque	spider_button_create();
extern Xv_opaque	spider_button_menu_create();
extern Xv_opaque	spider_textfield_create();
extern Xv_opaque	spider_window1_canvas1_create();

extern spider_subwindow1_objects   *spider_subwindow1_objects_initialize();
extern Xv_opaque	spider_popup_create();
extern Xv_opaque	spider_subwindow1_textsw1_create();

extern spider_subwindow2_objects   *spider_subwindow2_objects_initialize();
extern Xv_opaque	spider_boolean_choice_create();
extern Xv_opaque	spider_subwindow2_numtext1_create();
extern Xv_opaque	spider_slider_create();

#endif
