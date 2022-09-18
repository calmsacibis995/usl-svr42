/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtmail:menus.c	1.27"
#endif

#define MENUS_C

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include "mail.h"
#include <Gizmo/MenuGizmo.h>

extern MailRec *	LastMailRec;
extern SendRec *	sendRec;
extern void		CancelCB();

/* Alias Manager Base Window Menubar ------------------------------------- */

static MenuItems aliasFileItems[] = {
    {True, BUT_SAVE,	MNEM_SAVE,		NULL,	AliasSaveCB},
    {True, BUT_PRINT,	MNEM_PRINT,		NULL,	AliasPrintCB},
    {True, BUT_EXIT,	MNEM_EXIT_E,		NULL,	AliasExitCB},
    {NULL}
};
MenuGizmo AliasFile =     {
	NULL, "aliasFile", NULL, aliasFileItems, 0,
	NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static MenuItems aliasEditItems[] = {
    {True, BUT_UNDO,		MNEM_UNDO,		NULL,	AliasUndoCB},
    {True, BUT_NEW,		MNEM_NEW,		NULL,	AliasNewCB},
    {True, BUT_APPLY,		MNEM_APPLY,		NULL,	AliasAddCB},
    {True, BUT_DELETE,		MNEM_DELETE,		NULL,	AliasDeleteCB},
    {True, BUT_UNDELETE_DDD,	MNEM_UNDELETE_DDD_L,	NULL,	AliasUndeleteCB},
    {NULL}
};
MenuGizmo AliasEdit =     {
	NULL, "aliasEdit", NULL, aliasEditItems, 0,
	NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static MenuItems aliasHelpItems[] = {
	{True,	BUT_ALIAS_DDD,	MNEM_ALIAS_DDD,	NULL,NULL, (XtPointer)HelpAliasManager},
	{True,	BUT_TOC_DDD,	MNEM_TOC_DDD,	NULL,NULL, (XtPointer)HelpTOC},
	{True,	BUT_HELP_DESK_DDD,MNEM_HELP_DESK_DDD,NULL,NULL, (XtPointer)HelpDesk},
   	{NULL}
};

MenuGizmo AliasHelp =     {
	NULL, "aliasHelp", NULL, aliasHelpItems, HelpCB,
	NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static MenuItems aliasMenuItems[] = {
    {True, BUT_FILE,	MNEM_FILE,		(char *) &AliasFile},
    {True, BUT_EDIT,	MNEM_EDIT,		(char *) &AliasEdit},
    {True, BUT_HELP,	MNEM_HELP,		(char *) &AliasHelp},
    {NULL}
};

MenuGizmo AliasMenu =     {
	NULL, "aliasMenu", NULL, aliasMenuItems, 0, NULL, CMD,
	OL_FIXEDROWS, 1, 0
};


/* ------------------------------------------------------- */

static MenuItems openItems[] = {
    {True, BUT_OPEN,	MNEM_OPEN,	 NULL, OpenCB},
    {True, BUT_CANCEL,	MNEM_CANCEL,	 NULL, CancelCB},
    {True, BUT_HELP,	MNEM_HELP,	 NULL, HelpCB,(XtPointer)HelpManagerOpen},
    {NULL}
};
MenuGizmo Open =     {
	NULL, "open", NULL, openItems, 0, NULL, CMD, OL_FIXEDROWS, 1, 0
};

static MenuItems readOpenItems[] = {
    {True, BUT_OPEN,	MNEM_OPEN,		NULL, ReadOpenCB},
    {True, BUT_CANCEL,	MNEM_CANCEL,		NULL, CancelCB},
    {True, BUT_HELP,	MNEM_HELP,		NULL, HelpCB,(XtPointer)HelpReaderOpen},
    {NULL}
};
MenuGizmo ReadOpen =     {
	NULL, "readopen", NULL, readOpenItems, 0, NULL, CMD, OL_FIXEDROWS, 1, 0
};

MenuItems saveReplyItems[] = {
    {True, BUT_SAVE,	MNEM_SAVE,	 NULL, SaveSaveReplyAsCB},
    {True, BUT_CANCEL,	MNEM_CANCEL,	 NULL, CancelCB},
    {True, BUT_HELP,	MNEM_HELP,	NULL,HelpCB,(XtPointer)HelpSenderSaveAs},
    {NULL}
};
MenuGizmo SaveReply =     {
	NULL, "saveReply", NULL, saveReplyItems, 0,
	NULL, CMD, OL_FIXEDROWS, 1, 0
};

static MenuItems openSendItems[] = {
    {True, BUT_OPEN,	MNEM_OPEN,	 NULL, SendOpenCB},
    {True, BUT_CANCEL,	MNEM_CANCEL,	 NULL, CancelCB},
    {True, BUT_HELP,	MNEM_HELP,	 NULL, HelpCB, (XtPointer)HelpSenderOpen},
    {NULL}
};
MenuGizmo SendOpen =     {
	NULL, "sendOpen", NULL, openSendItems, 0,
	NULL, CMD, OL_FIXEDROWS, 1, 0
};

MenuItems saveItems[] = {
    {True, BUT_SAVE,	MNEM_SAVE,	 NULL, ManageSaveSaveAsCB},
    {True, BUT_CANCEL,	MNEM_CANCEL,	 NULL, CancelCB},
    {True, BUT_HELP,	MNEM_HELP,	 NULL,HelpCB,(XtPointer)HelpManagerSaveAs},
    {NULL}
};
MenuGizmo Save =     {
	NULL, "manageSave", NULL, saveItems, 0,
	NULL, CMD, OL_FIXEDROWS, 1, 0
};

void
CancelCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	XtPopdown ((Widget) _OlGetShellOfWidget (wid));
}

static MenuItems overwriteItems[] = {
    {True, BUT_OVERWRITE,	MNEM_OVERWRITE,		NULL,	RemoveFileCB},
    {True, BUT_CANCEL,		MNEM_CANCEL,		NULL,	CancelCB},
    {NULL}
};

MenuGizmo OverWrite = {
	NULL,				/* help		*/
	"OverWrite",		/* name		*/
	NULL,				/* title	*/
	overwriteItems,		/* items	*/
	0,					/* function	*/
	NULL,				/* client_data	*/
	CMD,				/* buttonType	*/
	OL_FIXEDROWS,		/* layoutType	*/
	1,					/* measure	*/
	0					/* default Item	*/
};

extern MenuItems ErrorItems[];

static MenuGizmo Okay = {
	NULL,			/* help		*/
	"na",			/* name		*/
	NULL,			/* title	*/
	ErrorItems,		/* items	*/
	0,				/* function	*/
	NULL,			/* client_data	*/
	CMD,			/* buttonType	*/
	OL_FIXEDROWS,	/* layoutType	*/
	1,				/* measure	*/
	0				/* default Item	*/
};


void
SetSensitivity (bw, name, start, end, flag)
BaseWindowGizmo *	bw;
char *			name;
int			start;
int			end;
Boolean			flag;
{
	Widget	menuWidget;
	int	i;

	menuWidget = (Widget)QueryGizmo (
		BaseWindowGizmoClass, bw, GetGizmoWidget, name
	);
	for (i=start; i<end+1; i++) {
		OlVaFlatSetValues (
			menuWidget,
			i,
			XtNsensitive,	flag,
			(String)0
		);
	}
}

void
SetSaveFunction (func)
PFV	func;
{
	saveItems[0].function = func;
}
