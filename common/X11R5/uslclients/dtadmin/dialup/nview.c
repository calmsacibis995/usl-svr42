/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/nview.c	1.12"
#endif
/*
	Description:
		nview: view/update a remote systems attribute file

*/

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <limits.h>
#include <dtcopy.h>
#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <X11/Shell.h>
#include <FButtons.h>

#include <Gizmos.h>
#include <PopupGizmo.h>
#include <MenuGizmo.h>
#include <ModalGizmo.h>
#include <InputGizmo.h>
#include <SpaceGizmo.h>
#include "STextGizmo.h"
#include <LabelGizmo.h>
#include <ChoiceGizm.h>

#include "uucp.h"
#include "error.h"

/*
 * DO NOT REMOVE.  It is used to overwrite the default help
 * file set in the error.h file.
 */
#ifdef	HELP_FILE
#undef	HELP_FILE
#endif
#define HELP_FILE	"dtadmin/remotsys.hlp"

char *		Program;
char *		ApplicationName;

extern void	HelpCB();

extern	int	optind;
extern	char	*optarg;

extern	int	AttrParse();
extern	char	*GetUser();
extern	char	*GetAttrValue();
extern	void	rexit();

static	Widget	root;
static	Boolean	changed = False;
static	char 	target[PATH_MAX];

static	void	ChangeCB();

static void CreatePopup(Widget);
static void ViewCB(Widget, XtPointer, XtPointer);
static void EnablePathCB(Widget, XtPointer, XtPointer);
static void DisablePathCB(Widget, XtPointer, XtPointer);
static void ExitMainCB(Widget, XtPointer, XtPointer);

static void
CancelCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
        XtPopdown ((Widget)_OlGetShellOfWidget (wid));
	exit(0);
}

typedef struct _applicationResources
   {
   Boolean      warnings;
   int          beepVolume;
   Pixel        DefaultForeground;
   Pixel        DefaultBackground;
   } ApplicationResources;

ApplicationResources ViewResources;

static XtResource resources[] =
   {
   { "warnings", "warnings", XtRBoolean, sizeof(Boolean),
     (Cardinal) &ViewResources.warnings, 
     XtRString, (XtPointer)"false" },
   { "beepVolume", "beepVolume", XtRInt, sizeof(int),
     (Cardinal) &ViewResources.beepVolume, 
     XtRString, "0" },
   { "TextFontColor", "TextFontColor", XtRPixel, sizeof(Pixel),
     (Cardinal) &ViewResources.DefaultForeground, XtRString, "black" },
   { "Background", "BackGround", XtRPixel, sizeof(Pixel),
     (Cardinal) &ViewResources.DefaultBackground, XtRString, "white" },
   };

static HelpText AppHelp = {
    title_remote, HELP_FILE, help_remote,
};

typedef enum { ViewApply, ViewReset, ViewCancel, ViewHelp } ViewMenuItemIndex;

static MenuItems  ViewMenuItems[] =
   {
   {(XA)True, label_apply, mnemonic_apply},
   {(XA)True, label_reset, mnemonic_reset},
   {(XA)True, label_cancel, mnemonic_cancel},
   {(XA)True, label_help, mnemonic_help, NULL, HelpCB, (char *)&AppHelp },
   { 0 }
   };

static MenuGizmo ViewMenu =
   { NULL, "_X_", "_X_", ViewMenuItems, ViewCB, NULL, CMD, OL_FIXEDROWS, 1, 0 };

typedef struct _DTSetting {
	Setting *duser;
	Setting *rpath;
	Setting *dpath;
} DTSetting;
typedef struct _DtSetting {
	Setting duser;
	Setting rpath;
	Setting dpath;
} DtSetting;

DtSetting dtSettings = {
	{"Recipient",	NULL,	NULL,	(XtPointer)0	},
	{"Destiny",	NULL,	NULL,	(XtPointer)0	},
	{"Folder",	NULL,	NULL,	(XtPointer)0	},
};

/* for ``Destination System'' prompt */

static char dmach[DST_LEN];
static StaticTextGizmo System = {
	NULL,			/* HELP		*/
	"system",		/* NAME		*/
	dmach
};

static GizmoRec rhost[] = {
	{StaticTextGizmoClass,	&System},
};

static LabelGizmo Rhost = {
	NULL,			/* HELP		*/
	"rhost",		/* NAME		*/
	TXT_DESTINATION,	/* CAPTION      */
	rhost,			/* GIZMOARRAY	*/
	XtNumber(rhost),	/* 	*/
	OL_FIXEDCOLS,		/* LAYOUT	*/
	1,			/* MEASURE	*/
	0,			/* Arg array	*/
	0,			/* numArg	*/
	True			/* ALIGNCAP	*/
};

static char ruser[UNAMESIZE];
/* for ``Recipient'' prompt */
static InputGizmo Recipient = {
        NULL,
	"recipient",
	TXT_RECIPIENT,
	ruser,
	&dtSettings.duser,
	UNAMESIZE,
	ChangeCB,
};

/* for ``Delivery Method'' prompt */

static StaticTextGizmo UUCP = {
	NULL,			/* HELP		*/
	"uucp",		/* NAME		*/
	"UUCP"
};

static MenuItems rpathItems[] = {
	{True, label_uucp,	mnemonic_uucp,	"uucp", DisablePathCB},
	{True, label_inet,	mnemonic_inet,	"internet", EnablePathCB},
	{NULL, 0 }
};

static MenuGizmo rpathMenu = {
	NULL, "rpath", "", rpathItems, 0, NULL, EXC,
	OL_FIXEDROWS, 1, OL_NO_ITEM
};

ChoiceGizmo Rpath = {
	NULL,
	"rpath",
	"",
	&rpathMenu,
	&dtSettings.rpath,
	ChangeCB,
};

static char path[PATH_LEN];
static InputGizmo Dpath = {
        NULL,
	"dpath",
	"Remote Folder:",
	path,
	&dtSettings.dpath,
	UNAMESIZE,
	ChangeCB,
};

static GizmoRec folder[] = {
	{ChoiceGizmoClass,	&Rpath},
};

static LabelGizmo Folder = {
	NULL,			/* HELP		*/
	"folder",		/* NAME		*/
	"",
	folder,			/* GIZMOARRAY	*/
	XtNumber(folder),	/* 	*/
	OL_FIXEDROWS,		/* LAYOUT	*/
	1,			/* MEASURE	*/
	0,			/* Arg array	*/
	0,			/* numArg	*/
	FALSE			/* ALIGNCAP	*/
};

static GizmoRec destiny[] = {
	{StaticTextGizmoClass,	&UUCP},
	{LabelGizmoClass,	&Folder},
};

static LabelGizmo Destiny = {
	NULL,			/* HELP		*/
	"destiny",		/* NAME		*/
	TXT_DELIVERY,
	destiny,		/* GIZMOARRAY	*/
	XtNumber(destiny),	/* 	*/
	OL_FIXEDROWS,		/* LAYOUT	*/
	1,			/* MEASURE	*/
	0,			/* Arg array	*/
	0,			/* numArg	*/
	FALSE			/* ALIGNCAP	*/
};

static SpaceGizmo space = {
	(Dimension)2, (Dimension)10
};

static GizmoRec PropGiz[] = {
	{LabelGizmoClass,	&Rhost},
	{InputGizmoClass,	&Recipient},
	{LabelGizmoClass,	&Destiny},
	{InputGizmoClass,	&Dpath},
};

static char title[40];
static PopupGizmo *	viewPopup = (PopupGizmo *)0;
static PopupGizmo ViewPopup = 
   { NULL, "_X_", title, &ViewMenu, PropGiz, XtNumber(PropGiz) };

static void
EnablePathCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	static InputGizmo *igp;
	static Boolean first_time = True;
	if (first_time) {
		first_time = False;
		igp = (InputGizmo *)QueryGizmo(PopupGizmoClass,
				&ViewPopup,
				GetGizmoGizmo,
				"dpath");
	}

	XtMapWidget( ((InputGizmo*)igp)->captionWidget);
	OlMoveFocus( ((InputGizmo*)igp)->textFieldWidget, OL_IMMEDIATE, 0);
} /* EnablePathCB */

static void
DisablePathCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	static InputGizmo *igp;
	static Boolean first_time = True;
	if (first_time) {
		first_time = False;
		igp = (InputGizmo *)QueryGizmo(PopupGizmoClass,
				&ViewPopup,
				GetGizmoGizmo,
				"dpath");
	}

	XtUnmapWidget( ((InputGizmo*)igp)->captionWidget);
} /* DisablePathCB */

static void
ShowUUCPonly()
{
	GizmoRec * gp;
	static LabelGizmo *lgp;
	static Boolean first_time = True;
	if (first_time) {
		first_time = False;
		lgp = (LabelGizmo *)QueryGizmo(PopupGizmoClass,
				&ViewPopup,
				GetGizmoGizmo,
				"destiny");
	}

	gp = lgp->gizmos;
	XtManageChild( ((StaticTextGizmo*)gp->gizmo)->widget);
	gp++;
	XtUnmanageChild( ((LabelGizmo*)gp->gizmo)->captionWidget);
} /* ShowUUCPonly */

static void
ShowMethods()
{
	GizmoRec * gp;
	static LabelGizmo *lgp;
	static Boolean first_time = True;
	if (first_time) {
		first_time = False;
		lgp = (LabelGizmo *)QueryGizmo(PopupGizmoClass,
				&ViewPopup,
				GetGizmoGizmo,
				"destiny");
	}

	gp = lgp->gizmos;
	XtUnmanageChild( ((StaticTextGizmo *)gp->gizmo)->widget);
	gp++;
	XtManageChild( ((LabelGizmo *)gp->gizmo)->captionWidget);
} /* ShowMethods */

Boolean	qflag; 
Boolean opernd;

main( argc, argv )
int argc;
char *argv[];
{

	char	stderr_file[PATH_MAX];
	char	login[UNAMESIZE];
	char 	osystem[DST_LEN];	/* origination system */
	char 	*dsystem;		/* destination system */
	char 	*duser;			/* destination user */
	char	*dpath;
	char	*userHome;
	int  	c, x, i;
	FILE	*stderr_p;
	stringll_t	*attr_list;

	Program = basename(argv[0]);
	root = InitializeGizmoClient(ClientName, ClientClass,
		Program,
		NULL, NULL,
		NULL, 0,
		&argc, argv,
		NULL,
		NULL,
		resources,
		XtNumber(resources),
		NULL,
		0,
		NULL,
		NULL,
		NULL);

	/* parse command line */

	dpath = 0;
	dsystem = 0;
	qflag = False;

	while ((c = getopt( argc, argv, "qr:" )) != -1) {
		switch ( c ) {
		default:
			rexit(1, TXT_OPTION, &c);
			break;
		case 'q':
			qflag = True;
			break;
		case 'r':
			dsystem = optarg;
			break;
		}/*endswitch*/
	}/*endwhile*/

	if (dsystem == 0) {
		rexit(2, TXT_DEST_NO, "");
	} 

	sprintf(title, GGT(TXT_TITLE));

	/* get login name */
	if (GetUser(login) == (char *)NULL) {
		rexit(3, TXT_LOGIN_NO, "");
	}

	userHome = getenv("HOME");
	if (!userHome)
		userHome = "";

	(void)umask(022);

	/* create path name to node file */

	(void)sprintf(target, "%s/.node/%s", userHome, dsystem);

#ifdef debug
	(void)fprintf(stderr, "reading the attribute file: \"%s\"\n", target);

#endif
	/* parse the attr.node file */

	if (AttrParse(target, &attr_list) == -1) {
		rexit(4, TXT_NODE_NO, target);
	}

	duser = GetAttrValue(attr_list, "DUSER");
	dpath = GetAttrValue(attr_list, "DPATH");
	GetNodeName(osystem);

#ifdef debug
	fprintf( stderr, "these values has been retrived from \"%s\" for \"%s\":\n",
		dsystem, login);
	fprintf( stderr, "DUSER=%s\nDPATH=%s\n\n",
		duser, dpath);
#endif
#ifdef LATER
	if (duser == (char *)NULL )
		rexit(5, TXT_NODE_NO, target);
#endif
	if (dpath != 0 && !qflag) {
		/* strip off leading spaces if any */
		while (*dpath == ' ') dpath++;
		if ((*dpath)!= '/')
			rexit(6,
			TXT_FOLDER_NO, dpath);
		else
			opernd = True;
	} else
		opernd = False;

	strcpy(ruser, duser);
	strcpy(dmach, dsystem);
	strcpy(path, dpath);
	dtSettings.rpath.previous_value = (XtPointer) opernd;
	CreatePopup(root);
	if (opernd && !qflag)
		EnablePathCB(root, 0, 0);
	else
		DisablePathCB(root, 0, 0);
	if (qflag)
		ShowUUCPonly();
	else
		ShowMethods();

	XtMainLoop();

}/* main */

static void
ExitMainCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	exit(2);
}

/*
 * CreatePopup
 *
 * This procedure creates the ViewPopup dialog window and maps it.
 *
 */

static void 
CreatePopup(Widget Shell)
{
	Widget popup;
	viewPopup = &ViewPopup;
	ApplicationName = GGT (ClientName);
	SET_HELP(AppHelp);
	popup = CreateGizmo(Shell, PopupGizmoClass, &ViewPopup, NULL, 0);
	OlAddCallback (
		popup,
		XtNpopdownCallback, ExitMainCB,
		(XtPointer) 0
	);
	MapGizmo(PopupGizmoClass, &ViewPopup);

} /* end of CreatePopup */

/*
 * ViewCB
 *
 *
 * The callback procedure is called when any of the buttons in the menu bar
 * of the ViewPopup dialog are selected.  The callback switched on the
 * index of the flat button in the menu bar and either:
 * Retrieves help for the user.
 * 
 */

static void
ViewCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	OlFlatCallData *	p = (OlFlatCallData *)call_data;
	char			*dpath;
	Widget			shell;
	static char		buf[BUF_SIZE];
	DTSetting 		s;
	FILE			*attrp;

	shell = GetPopupGizmoShell(viewPopup);

	switch (p->item_index) {
	    case ViewApply:
		ManipulateGizmo (
			(GizmoClass)&PopupGizmoClass,
			viewPopup,
			GetGizmoValue
		);
		  
		s.duser = (Setting *)QueryGizmo (
			PopupGizmoClass, viewPopup,
			GetGizmoSetting, "recipient"
		);
		s.rpath = (Setting *)QueryGizmo (
			PopupGizmoClass, viewPopup,
			GetGizmoSetting, "rpath"
		);
		s.dpath = (Setting *)QueryGizmo (
			PopupGizmoClass, viewPopup,
			GetGizmoSetting, "dpath"
		);
		dpath = s.dpath->current_value;
		
		if (strcmp(dpath, "") && !qflag) {
			/* strip off leading spaces if any */
			while (*dpath == ' ') dpath++;
			if ((*dpath)!= '/') {
				SetPopupMessage(viewPopup, GGT(TXT_FOLDER_NO));
				return;
			}
		}
		attrp = fopen( target, "w");
		if (attrp == (FILE *) 0) {
			rexit(7, TXT_SAVE_NO, target);
		}

		/* put the node's properties here */
		fprintf( attrp, "DUSER=%s\nDPATH=%s\n",
			s.duser->current_value,
			s.rpath->current_value? s.dpath->current_value: NULL
			);
		(void) fflush(attrp);
		fclose( attrp );
		BringDownPopup(shell);
		break;
	case ViewReset:
		ManipulateGizmo (
			(GizmoClass)&PopupGizmoClass,
			viewPopup,
			ResetGizmoValue
		);
		if (opernd && !qflag)
			EnablePathCB(wid, 0, 0);
		else
			DisablePathCB(wid, 0, 0);
		break;
	case ViewCancel:
		XtPopdown(shell);
		break;
	default:
		(void)fprintf(stderr,"at %d in %s\n", __LINE__, __FILE__);
		break;
    }
} /* ViewCB */

static void
ChangeCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	changed = True;
}
