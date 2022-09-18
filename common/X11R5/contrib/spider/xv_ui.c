/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4spider:xv_ui.c	1.1"
/*
 *      Spider
 *
 *      (c) Copyright 1989, Donald R. Woods and Sun Microsystems, Inc.
 *      (c) Copyright 1990, David Lemke and Network Computing Devices Inc.
 *      (c) Copyright 1990, Heather Rose and Sun Microsystems, Inc.
 *
 *      See copyright.h for the terms of the copyright.
 *
 *      @(#)xv_ui.c	2.2	90/04/27
 *
 */

/*
 * XView Toolkit interface to Spider
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/icon_load.h>
#include <xview/panel.h>
#include <xview/scrollbar.h>
#include <xview/svrimage.h>
#include <xview/text.h>
#include <xview/defaults.h>
#include <xview/notice.h>
#include "xv_ui.h"
#include "spider.bm"

int defaultHelpDir = FALSE;

/*
 * Create object `BackUpMenu' in the specified instance.
 *
 * TODO:  Change from menu gen proc style to menu notify style with pins.
 */
Xv_opaque
spider_BackUpMenu_create(ip, owner)
	caddr_t		*ip;
	Xv_opaque	owner;
{
	extern Menu_item	undo_onemove_handler();
	extern Menu_item	undo_startover_handler();
	extern Menu_item	undo_replay_handler();
	Xv_opaque	obj;
	
	obj = xv_create(owner, MENU_COMMAND_MENU,
		XV_KEY_DATA, INSTANCE, ip,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, "One Move",
			MENU_GEN_PROC, undo_onemove_handler,
			0,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, "Start Over",
			MENU_GEN_PROC, undo_startover_handler,
			0,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, "Replay",
			MENU_GEN_PROC, undo_replay_handler,
			0,
		0);
	return obj;
}

/*
 * Create object `HelpMenu' in the specified instance.
 *
 * TODO:  Change from menu gen proc style to menu notify style with pins.
 */
Xv_opaque
spider_HelpMenu_create(ip, owner)
	caddr_t		*ip;
	Xv_opaque	owner;
{
	extern Menu_item	help_handler();
	Xv_opaque	obj;
	
	obj = xv_create(owner, MENU_COMMAND_MENU,
		XV_KEY_DATA, INSTANCE, ip,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			XV_KEY_DATA, HELPKEY, HELP_INTRO,
			MENU_STRING, "Intro...",
			MENU_GEN_PROC, help_handler,
			0,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			XV_KEY_DATA, HELPKEY, HELP_RULES,
			MENU_STRING, "Rules...",
			MENU_GEN_PROC, help_handler,
			0,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			XV_KEY_DATA, HELPKEY, HELP_CNTRLS,
			MENU_STRING, "Controls...",
			MENU_GEN_PROC, help_handler,
			0,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			XV_KEY_DATA, HELPKEY, HELP_EXS,
			MENU_STRING, "Examples...",
			MENU_GEN_PROC, help_handler,
			0,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			XV_KEY_DATA, HELPKEY, HELP_MISC,
			MENU_STRING, "Extras...",
			MENU_GEN_PROC, help_handler,
			0,
		MENU_ITEM,
                        XV_KEY_DATA, INSTANCE, ip,
                        XV_KEY_DATA, HELPKEY, HELP_SUM,
                        MENU_STRING, "Summary...",
                        MENU_GEN_PROC, help_handler,
                        0,
		0);
	return obj;
}

/*
 * Create object `FileMenu' in the specified instance.
 *
 * TODO:  Change from menu gen proc style to menu notify style with pins.
 */
Xv_opaque
spider_FileMenu_create(ip, owner)
	caddr_t		*ip;
	Xv_opaque	owner;
{
	extern Menu_item	file_save_handler();
	extern Menu_item	file_resume_handler();
	extern Menu_item	file_resumefromselection_handler();
	extern Menu_item	file_properties_handler();
	Xv_opaque	obj;
	
	obj = xv_create(owner, MENU_COMMAND_MENU,
		XV_KEY_DATA, INSTANCE, ip,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, "Save in File",
			MENU_GEN_PROC, file_save_handler,
			0,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, "Resume from File",
			MENU_GEN_PROC, file_resume_handler,
			0,
		MENU_ITEM,
			XV_KEY_DATA, INSTANCE, ip,
			MENU_STRING, "Resume from Selection",
			MENU_GEN_PROC, file_resumefromselection_handler,
			0,
		MENU_ITEM,
                        XV_KEY_DATA, INSTANCE, ip,
                        MENU_STRING, "Properties...",
                        MENU_GEN_PROC, file_properties_handler,
			0,
		0);
	return obj;
}

/*
 * Initialize an instance of object `window1'.
 */
spider_window1_objects *
spider_window1_objects_initialize(ip, owner)
	spider_window1_objects	*ip;
	Xv_opaque	owner;
{
	extern void newgame_handler(), expand_handler(), locate_handler();
	extern void score_handler();
	extern int helpfiles_exist();

	if (!ip &&
	    !(ip = (spider_window1_objects *)
	      calloc(1, sizeof (spider_window1_objects))))
		return (spider_window1_objects *) NULL;
	ip->defaults = spider_defaults_objects_initialize(ip, ip->window1);
	ip->window1 = spider_window1_window1_create(ip, owner);
	ip->controls1 = spider_window1_controls1_create(ip, ip->window1);
	ip->button1 = spider_button_create(ip, ip->controls1, 
		"New Game", newgame_handler);
	xv_set(ip->button1, XV_HELP_DATA, "spider:NewGame", 0);
	ip->button2 = spider_button_menu_create(ip, ip->controls1,
		"Back Up", spider_BackUpMenu_create(ip, NULL));
	xv_set(ip->button2, XV_HELP_DATA, "spider:BackUp", 0);
	ip->button3 = spider_button_create(ip, ip->controls1,
		"Expand", expand_handler);
	xv_set(ip->button3, XV_HELP_DATA, "spider:Expand", 0);
	ip->button4 = spider_button_create(ip, ip->controls1,
		"Locate", locate_handler);
	xv_set(ip->button4, XV_HELP_DATA, "spider:Locate", 0);
	ip->button5 = spider_button_create(ip, ip->controls1,
		"Score", score_handler);
	xv_set(ip->button5, XV_HELP_DATA, "spider:Score", 0);
	ip->button6 = spider_button_menu_create(ip, ip->controls1,
		"Help", spider_HelpMenu_create(ip, NULL));
	if (!helpfiles_exist(ip->defaults->helpDir)) {
		xv_set(ip->button6, PANEL_INACTIVE, TRUE, 0);
	}
	xv_set(ip->button6, XV_HELP_DATA, "spider:Help", 0);
	ip->button7 = spider_button_menu_create(ip, ip->controls1,
		"File", spider_FileMenu_create(ip, NULL));
	xv_set(ip->button7, XV_HELP_DATA, "spider:File", 0);
	ip->textfield1 = spider_textfield_create(ip, ip->controls1, "Name:");
	xv_set(ip->textfield1, XV_HELP_DATA, "spider:Name.textfield", 0);
	window_fit_height(ip->controls1);
	ip->canvas1 = spider_window1_canvas1_create(ip, ip->window1);
	ip->subwindow1 = (spider_subwindow1_objects *)NULL;
	ip->subwindow2 = (spider_subwindow2_objects *)NULL;
	return ip;
}

/*
 * Create object `window1' in the specified instance.
 */
Xv_opaque
spider_window1_window1_create(ip, owner)
	spider_window1_objects *ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	Icon		icon;
	Server_image	icon_image;
	
	obj = xv_create(owner, FRAME,
		XV_KEY_DATA, INSTANCE, ip,
		XV_WIDTH, TABLE_WIDTH + 20,
		XV_HEIGHT, TABLE_HEIGHT + 20,
		XV_LABEL, ip->defaults->instanceName,
		FRAME_SHOW_FOOTER, TRUE,
		0);

	icon_image = (Server_image)xv_create(XV_NULL, SERVER_IMAGE,
		XV_WIDTH,		spider_width,
		XV_HEIGHT,		spider_height,
		SERVER_IMAGE_X_BITS,	spider_bits,
		NULL);

	icon = (Icon)xv_create(obj, ICON,
		ICON_IMAGE,	icon_image,
		NULL);

	xv_set(obj, FRAME_ICON, icon, NULL);

	return obj;
}

/*
 * Create object `controls1' in the specified instance.
 */
Xv_opaque
spider_window1_controls1_create(ip, owner)
	caddr_t		*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 0,
		XV_Y, 0,
		XV_WIDTH, WIN_EXTEND_TO_EDGE,
		OPENWIN_SHOW_BORDERS, FALSE,
		0);
	return obj;
}

/*
 * Create a button.
 */
Xv_opaque
spider_button_create(ip, owner, label, proc)
	caddr_t		*ip;
	Xv_opaque       owner;
	char		*label;
	void		*proc;
{
	Xv_opaque       obj;
	
	obj = xv_create(owner, PANEL_BUTTON,
                XV_KEY_DATA, INSTANCE, ip,
                PANEL_LABEL_STRING, label,
		PANEL_NOTIFY_PROC, proc,
                0);
        return obj;
}

/*
 * Create a button with a menu.
 */
Xv_opaque
spider_button_menu_create(ip, owner, label, menu)
	caddr_t         *ip;
        Xv_opaque       owner;
        char            *label;
	Xv_opaque	menu;
{
	Xv_opaque       obj;
        
        obj = xv_create(owner, PANEL_BUTTON,
                XV_KEY_DATA, INSTANCE, ip,
                PANEL_LABEL_STRING, label,
                PANEL_ITEM_MENU, menu,
                0);
        return obj;
}

/*
 * Create a textfield.
 */
Xv_opaque
spider_textfield_create(ip, owner, label)
	spider_window1_objects	*ip;
	Xv_opaque	owner;
	char		*label;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_TEXT,
		XV_KEY_DATA, INSTANCE, ip,
		PANEL_LABEL_STRING, label,
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_VALUE_DISPLAY_LENGTH, ip->defaults->textField,
		PANEL_VALUE_STORED_LENGTH, 256,
		PANEL_READ_ONLY, FALSE,
		0);
	return obj;
}

/*
 * Create object `canvas1' in the specified instance.
 */
Xv_opaque
spider_window1_canvas1_create(ip, owner)
	caddr_t		*ip;
	Xv_opaque	owner;
{
	extern Notify_value	handle_card_event();
	extern void		spider_resize_proc();
	Xv_opaque	obj;
	
	obj = xv_create(owner, CANVAS,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 0,
		XV_WIDTH, WIN_EXTEND_TO_EDGE,
		XV_HEIGHT, WIN_EXTEND_TO_EDGE,
		CANVAS_X_PAINT_WINDOW, TRUE,
		CANVAS_RETAINED, TRUE,
		CANVAS_RESIZE_PROC, spider_resize_proc,
		0);
	xv_set(xv_get(obj, CANVAS_PIXWIN), WIN_CONSUME_EVENTS,
		WIN_ASCII_EVENTS,
		WIN_MOUSE_BUTTONS,
		LOC_DRAG,
		0, 0);
	notify_interpose_event_func(xv_get(obj, CANVAS_PIXWIN),
		handle_card_event, NOTIFY_SAFE);
	return obj;
}

void
set_help_label(win, name)
	Xv_opaque win;
	char	*name;
{
	char	*buf;

	buf = malloc(strlen(name) + 8);
	sprintf(buf, "%s Help", name);
	xv_set(win, XV_LABEL, buf, 0);
}

/*
 * Initialize an instance of object `subwindow1'.
 */
spider_subwindow1_objects *
spider_subwindow1_objects_initialize(ip, owner)
	spider_window1_objects	*ip;
	Xv_opaque	owner;
{
	spider_subwindow1_objects *obj = ip->subwindow1;
	extern void subhelp_handler(), help_done_handler();

	if (obj != (spider_subwindow1_objects *)NULL ){
		return obj;
	} else if ( !(obj = (spider_subwindow1_objects *)
	    calloc(1, sizeof (spider_subwindow1_objects)))) {
		return (spider_subwindow1_objects *) NULL;
	}
	obj->window2 = spider_popup_create(ip, owner);
	set_help_label(obj->window2, ip->defaults->instanceName);
	obj->controls2 = xv_get(obj->window2, FRAME_CMD_PANEL);
	obj->button8 = spider_button_create(ip, obj->controls2,
		"Intro", subhelp_handler);
	xv_set(obj->button8, XV_KEY_DATA, HELPKEY, HELP_INTRO,
		0);
	obj->button9 = spider_button_create(ip, obj->controls2,
		"Rules", subhelp_handler);
	xv_set(obj->button9, XV_KEY_DATA, HELPKEY, HELP_RULES,
		0);
	obj->button10 = spider_button_create(ip, obj->controls2,
		"Controls", subhelp_handler);
	xv_set(obj->button10, XV_KEY_DATA, HELPKEY, HELP_CNTRLS,
		0);
	obj->button11 = spider_button_create(ip, obj->controls2,
		"Examples", subhelp_handler);
	xv_set(obj->button11, XV_KEY_DATA, HELPKEY, HELP_EXS,
		0);
	obj->button12 = spider_button_create(ip, obj->controls2,
		"Extras", subhelp_handler);
	xv_set(obj->button12, XV_KEY_DATA, HELPKEY, HELP_MISC,
		0);
	obj->button13 = spider_button_create(ip, obj->controls2,
                "Summary", subhelp_handler);
        xv_set(obj->button13, XV_KEY_DATA, HELPKEY, HELP_SUM,
                0);
	obj->button14 = spider_button_create(ip, obj->controls2,
		"Dismiss", help_done_handler);
	xv_set(obj->button14, 
		XV_X, (int) xv_get(obj->button14, XV_X) + 
		    (int) xv_get(obj->button14, XV_WIDTH),
		0);
	window_fit_height(obj->controls2);
	obj->textsw1 = spider_subwindow1_textsw1_create(ip, obj->window2);

	return obj;
}

/*
 * Create a popup window.
 */
Xv_opaque
spider_popup_create(ip, owner)
	caddr_t		*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, FRAME_CMD,
		XV_KEY_DATA, INSTANCE, ip,
		FRAME_SHOW_LABEL, TRUE,
		FRAME_CMD_PUSHPIN_IN, TRUE,
		0);
	return obj;
}

/*
 * Create object `textsw1' in the specified instance.
 */
Xv_opaque
spider_subwindow1_textsw1_create(ip, owner)
	caddr_t		*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, TEXTSW,
		XV_KEY_DATA, INSTANCE, ip,
		0);

	return obj;
}

/*
 * Initialize an instance of object `defaults'.
 */
spider_defaults_objects *
spider_defaults_objects_initialize(ip, owner)
	spider_window1_objects  *ip;
        Xv_opaque       owner;
{
	spider_defaults_objects *obj=ip->defaults;
	char buf1[256];
	char *s;
	extern int usebell;
#ifdef ROUND_CARDS
	extern int round_cards;
#endif
	extern int deltamod;
	extern char *helpDir;
	extern char *instanceName;
	extern char *resourceFile;

	if (obj != (spider_defaults_objects *)NULL ){
                return obj;
        } else if ( !(obj = (spider_defaults_objects *)
	    calloc(1, sizeof (spider_defaults_objects)))) {
		return (spider_defaults_objects *) NULL;
	}
	defaults_load_db(resourceFile);
		
	obj->instanceName = instanceName;
	sprintf(buf1,"%s.%s", obj->instanceName, "bell");
	obj->bell = usebell =
		(int) defaults_get_boolean(buf1, "Bell", TRUE);
	sprintf(buf1,"%s.%s", obj->instanceName, "replayTime");
	obj->replayTime = 
		defaults_get_integer(buf1, "ReplayTime", 200);
	sprintf(buf1,"%s.%s", obj->instanceName, "confirm");
	obj->confirm =
		(int) defaults_get_boolean(buf1, "Confirm", TRUE);
#ifdef  ROUND_CARDS
	sprintf(buf1,"%s.%s", obj->instanceName, "roundCards");
	obj->roundCards = round_cards = 
		defaults_get_boolean(buf1, "RoundCards", TRUE);
#endif
	sprintf(buf1,"%s.%s", obj->instanceName, "deltaMod");
	obj->deltaMod = deltamod = 
		defaults_get_integer(buf1, "DeltaMod", 1);

	sprintf(buf1,"%s.%s", obj->instanceName, "textField");
        obj->textField = defaults_get_integer_check(buf1, "TextField",32,8,45);

	sprintf(buf1,"%s.%s", obj->instanceName, "helpDir");
	s = (char *)defaults_get_string(buf1, "HelpDir", (char *)NULL);
	if (s == (char *)NULL) {
		obj->helpDir = helpDir;
	} else {
		obj->helpDir = malloc(strlen(s) + 1);
		sprintf(obj->helpDir, "%s", s);
		helpDir = obj->helpDir;
	}
	return(obj);
}

void
set_props_label(win, name)
	Xv_opaque win;
	char	*name;
{
	char	*buf;

	buf = malloc(strlen(name) + 16);
	sprintf(buf, "%s Properties", name);
	xv_set(win, XV_LABEL, buf, 0);
}

/*
 * Initialize an instance of object `subwindow2'.
 */
spider_subwindow2_objects *
spider_subwindow2_objects_initialize(ip, owner)
	spider_window1_objects	*ip;
	Xv_opaque	owner;
{
	char *buf;

	spider_subwindow2_objects *obj = ip->subwindow2;
	int row=1, width, bwidth;
	extern void     props_done_handler(), props_apply_handler(), 
		props_reset_handler();

	if (obj != (spider_subwindow2_objects *)NULL ){
		return obj;
	} else if ( !(obj = (spider_subwindow2_objects *)
	    calloc(1, sizeof (spider_subwindow2_objects)))) {
		return (spider_subwindow2_objects *) NULL;
	}
	obj->window3 = spider_popup_create(ip, owner);
	set_props_label(obj->window3, ip->defaults->instanceName);
	obj->controls3 = xv_get(obj->window3, FRAME_CMD_PANEL);
	xv_set(obj->controls3, XV_HELP_DATA, "spider:props.panel", 0);
	obj->choice1 = spider_boolean_choice_create(ip, obj->controls3, 
		"Use audible bell to notify:");
	xv_set(obj->choice1,
		XV_X, xv_cols(obj->controls3, 1),
		XV_Y, xv_rows(obj->controls3, row++),
		PANEL_VALUE, ip->defaults->bell,
		XV_HELP_DATA, "spider:bell",
		0);
	obj->numtext1 = spider_subwindow2_numtext1_create(ip, obj->controls3, 
		row++);
	xv_set(obj->numtext1, XV_HELP_DATA, "spider:replayTime", 0);
#ifdef ROUND_CARDS
	obj->choice2 = spider_boolean_choice_create(ip, obj->controls3,
		"Use round cards:");
	xv_set(obj->choice2, 
                XV_X, xv_cols(obj->controls3, 1), 
                XV_Y, xv_rows(obj->controls3, row++), 
                PANEL_VALUE, ip->defaults->roundCards,
		XV_HELP_DATA, "spider:roundCards",
                0);
#endif
	obj->choice3 = spider_boolean_choice_create(ip, obj->controls3,
                "Use notice to verify deletion of data:");
        xv_set(obj->choice3,
                XV_X, xv_cols(obj->controls3, 1),
                XV_Y, xv_rows(obj->controls3, row++),
                PANEL_VALUE, ip->defaults->confirm,
		XV_HELP_DATA, "spider:confirm",
                0);
	obj->slider1 = spider_slider_create(ip, obj->controls3, 
		"Delta Modification:", 0, 30, 15);
	xv_set(obj->slider1, 
                XV_X, xv_cols(obj->controls3, 1), 
                XV_Y, xv_rows(obj->controls3, row++), 
                PANEL_VALUE, ip->defaults->deltaMod, 
		XV_HELP_DATA, "spider:deltaMod",
                0);
	obj->slider2 = spider_slider_create(ip, obj->controls3, 
		"Characters visible in text fields:", 8, 45, 20);
	xv_set(obj->slider2,
		XV_X, xv_cols(obj->controls3, 1),
                XV_Y, xv_rows(obj->controls3, row++),
                PANEL_VALUE, ip->defaults->textField,
		XV_HELP_DATA, "spider:textField",
                0);
	obj->textfield2 = spider_textfield_create(ip, obj->controls3, 
		"Help Directory:");
	xv_set(obj->textfield2,
		XV_X, xv_cols(obj->controls3, 1),
		XV_Y, xv_rows(obj->controls3, row++),
		PANEL_VALUE, ip->defaults->helpDir,
		XV_HELP_DATA, "spider:helpDir",
		0);
	window_fit_width(obj->controls3);
	width = xv_get(obj->controls3, XV_WIDTH);
	obj->button15 = spider_button_create(ip, obj->controls3,
		"Apply", props_apply_handler);
	obj->button16 = spider_button_create(ip, obj->controls3,
		"Reset", props_reset_handler);
	obj->button17 = spider_button_create(ip, obj->controls3,
		"Dismiss", props_done_handler);
	bwidth = ((int)xv_get(obj->button15, XV_WIDTH) + 
		(int)xv_get(obj->button16, XV_WIDTH) + 
		(int)xv_get(obj->button17, XV_WIDTH))/3;
	xv_set(obj->button15,
		XV_Y, xv_rows(obj->controls3, ++row),
		XV_X, (width - 5*bwidth) / 2,
		XV_HELP_DATA, "spider:props.apply",
		0);
	xv_set(obj->button16,
		XV_Y, xv_rows(obj->controls3, row),
		XV_X, (width - 5*bwidth) / 2 + 2*bwidth,
                XV_HELP_DATA, "spider:props.reset", 
		0);
	xv_set(obj->button17,
		XV_Y, xv_rows(obj->controls3, row),
		XV_X, (width - 5*bwidth) / 2 + 4*bwidth,
                XV_HELP_DATA, "spider:props.dismiss", 
		0);
	window_fit_height(obj->controls3);
	window_fit(obj->window3);

	return obj;
}

/*
 * Create boolean choice.
 */
Xv_opaque
spider_boolean_choice_create(ip, owner, label)
        spider_window1_objects	*ip;
        Xv_opaque       	owner;
	char			*label;
{
        Xv_opaque       obj;
	
	obj = xv_create(owner, PANEL_CHOICE,
		XV_KEY_DATA, INSTANCE, ip,
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_LABEL_STRING, label,
		PANEL_CHOICE_STRINGS,	"False", "True", 0,
		0);
	return(obj);
}

/*
 * Create object `numtext1' in the specified instance. 
 */ 
Xv_opaque 
spider_subwindow2_numtext1_create(ip, owner, row) 
        spider_window1_objects	*ip;
        Xv_opaque       	owner;
	int			row;
{ 
        Xv_opaque       obj;
	
	obj = xv_create(owner, PANEL_NUMERIC_TEXT, 
                XV_KEY_DATA, INSTANCE, ip, 
		XV_X, xv_cols(owner, 1),
		XV_Y, xv_rows(owner, row),	
                PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_LABEL_STRING, "Replay Time (in microseconds):",
		PANEL_MIN_VALUE, 20,
		PANEL_MAX_VALUE, 20000,
		PANEL_VALUE_DISPLAY_LENGTH, 6,
		PANEL_VALUE_STORED_LENGTH, 10,
		PANEL_VALUE, ip->defaults->replayTime,
		0);

	return(obj);
}	

/*
 * Create a slider object in the specified instance. 
 */ 
Xv_opaque 
spider_slider_create(ip, owner, label, min, max, cols) 
        spider_window1_objects	*ip;
        Xv_opaque       	owner;
	char			*label;
	int			min, max, cols;
{ 
        Xv_opaque       obj;
	
	obj = xv_create(owner, PANEL_SLIDER, 
                XV_KEY_DATA, INSTANCE, ip, 
                PANEL_LAYOUT, PANEL_HORIZONTAL,
		PANEL_LABEL_STRING, label,
		PANEL_MIN_VALUE, min,
		PANEL_MAX_VALUE, max,
		PANEL_SLIDER_WIDTH, xv_col(owner, cols),
		PANEL_SHOW_RANGE, TRUE,
		0);

	return(obj);
}	

int
do_notice(message)
        char *message;
{
        int result;
	extern spider_window1_objects *spider_window1;
 
        result = notice_prompt(spider_window1->controls1, NULL,
                NOTICE_MESSAGE_STRINGS, message, 0,
                NOTICE_BUTTON_YES, "Yes",
                NOTICE_BUTTON_NO, "No",
                0);
        if (result == NOTICE_YES) {
                return(TRUE);
        } else {
                return(FALSE);
        }
}

