/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:olwsm/wsm.h	1.34"

#ifndef _WSM_H
#define _WSM_H

#define WSM			"Workspace Manager"
#define DISPLAY			XtDisplay(InitShell)
#define SCREEN			XtScreen(InitShell)
#define ROOT			RootWindowOfScreen(SCREEN)
#define HELP(name)		name
#define SET_HELP(w, tag, source) \
	OlRegisterHelp(OL_WIDGET_HELP, (XtPointer) (w), (tag), \
OL_DESKTOP_SOURCE, (XtPointer)(source));
#define MOD1			"Alt"

#if	defined(FACTORY_LIST)
typedef struct ColorList {
	String			name;
	Pixel			workspace;
	Pixel			background;
	Pixel			input_focus_color;
	Pixel			input_window_header;
	Pixel			text_font_color;
	Pixel			text_background;
	Pixel			help_highlight;
}			ColorList;
typedef struct ColorLists {
	Cardinal		size;
	ColorList *		list;
}			ColorLists;
extern Boolean		StringToColorLists OL_ARGS((
	Display *		display,
	XrmValue *		args,
	Cardinal *		num_args,
	XrmValue *		from,
	XrmValue *		to,
	XtPointer *		converter_data
));
#endif

typedef struct WSMresources {
	XColor			workspace;
#ifdef DONT_USE
	Boolean			menu_pinned;
	Boolean			programs_menu_pinned;
#endif
	Boolean			warnings;
	Cardinal		depth_threshold;
#if	defined(FACTORY_LIST)
	ColorLists *		factory_color_lists;
#endif
#ifdef DONT_USE
	String			initial_sheet;
	Boolean			start_children;
#endif
}			WSMresources;

extern WSMresources	wsm;

extern Widget		InitShell;
extern Widget		handleRoot;
extern Widget		workspaceMenu;
extern Widget		programsMenu;

extern char *		GetPath OL_ARGS((
	char *			name
));
extern void		FooterMessage OL_ARGS((
	Widget			w,
	String			message,
	OlDefine		side,
	Boolean			beep
));
extern void		WSMExit OL_ARGS((
	void
));
extern int		ExecCommand OL_ARGS((
	char *			command
));
extern void		RefreshCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));
extern void		CreateWorkSpaceMenu OL_ARGS((
	Widget			parent,
	Widget *		p_workspace,
	Widget *		p_programs
));
extern void		RestartWorkspaceManager OL_ARGS((
	void
));

#endif
