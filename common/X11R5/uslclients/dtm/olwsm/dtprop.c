/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:olwsm/dtprop.c	1.27"

/******************************file*header********************************

    Description:
	This file contains the source code for desktop property sheet.
*/
						/* #includes go here	*/
#include <signal.h>
#include <stdlib.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/ChangeBar.h>
#include <Xol/IntegerFie.h>

#include <Gizmos.h>
#include <ChoiceGizm.h>
#include <LabelGizmo.h>
#include <NumericGiz.h>

#include <dtprop.h>
#include "error.h"
#include <exclusive.h>
#include <list.h>
#include <misc.h>
#include <node.h>
#include <property.h>
#include <resource.h>
#include <wsm.h>
#include <xtarg.h>

#include "../Dtm.h"		/* for Desktop */

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Public  Procedures 
*/
					/* private procedures		*/

static void		Import (XtPointer closure);
static void		Export (XtPointer closure);
static void		Create (Widget work, XtPointer closure);
static ApplyReturn *	ApplyCB (Widget w, XtPointer closure);
static void		ResetCB (Widget w, XtPointer closure);
static void		FactoryCB (Widget w, XtPointer closure);
static void		PingChangeBarCB (Widget, XtPointer, XtPointer);
static void		Report(Exclusive *);
static void		CreateLabelGizmo(Widget parent, LabelGizmo * gizmo,
					 ArgList args, Cardinal num_args);


/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

#define FACTORY(x)		*(ADDR *)&(_factory[x].value)
#define CURRENT(x)		*(ADDR *)&(_current[x].value)
#define GLOBAL(x) \
	resource_value(&global_resources, _factory[x].name)

#define SETVAR			GetXWINHome("adm/olsetvar")
#define DT_AT_LOGIN		"DT"
#define GUI_SWITCH		"XGUI"

static Arg desktop_args[] = {
/* Note: if you change the location of this element you must also */
/* change the index value in CreatePropertyPopup(). */
	{XtNlayoutType,		OL_FIXEDCOLS},
	{XtNalignCaptions,	True},
};

static OlDtHelpInfo DesktopHelp = {
	NULL, NULL, "DesktopMgr/dskpref.hlp", NULL, NULL
};

Property desktopProperty = {
	"Desktop",
	desktop_args,
	XtNumber (desktop_args),
	&DesktopHelp,
	'\0',
	Import,
	Export,
	Create,
	ApplyCB,
	ResetCB,
	FactoryCB,
	0,
	0,
	0,
	0,
};

/* These define the offsets for the various control on this property sheet.
   That is, the controls appear in this order.
*/
static enum { INTERFACE, SHOW_PATHS,
		  GRID_WIDTH, GRID_HEIGHT, FOLDER_ROWS, FOLDER_COLS };

/* Define strings used as resource values */
static char _openlook[]	= "OPEN_LOOK";
static char _motif[]	= "MOTIF";

static char _yes[]	= "Yes";
static char _no[]	= "No";

static char _true[]	= "True";
static char _false[]	= "False";

static char _2d[]	= "False";
static char _3d[]	= "True";

/* Define settings for each control */
static Setting settings[] = {
    { _3d,		(XtPointer)True, },
    { _false,		(XtPointer)DEFAULT_SHOW_PATHS, },
    { "86",		(XtPointer)DEFAULT_GRID_WIDTH, },
    { "64",		(XtPointer)DEFAULT_GRID_HEIGHT, },
    { "2",		(XtPointer)DEFAULT_FOLDER_ROWS, },
    { "5",		(XtPointer)DEFAULT_FOLDER_COLS, },
};

/* Define the factory and current settings for each control.

   Note that _factory entries with NULL values are those with integer
   defaults and are initialized during "Import".
*/
static Resource _factory[] = {
	{ "*threeD",		_3d },
	{ "*" XtNshowFullPaths,	_false },
	{ "*" XtNgridWidth,	NULL },
	{ "*" XtNgridHeight,	NULL },
	{ "*" XtNfolderRows,	NULL },
	{ "*" XtNfolderCols,	NULL },
};
static List factory = LIST(Resource, _factory);

static Resource _current[] = {
	{ "*threeD",		NULL },
	{ "*" XtNshowFullPaths,	NULL },
	{ "*" XtNgridWidth,	NULL },
	{ "*" XtNgridHeight,	NULL },
	{ "*" XtNfolderRows,	NULL },
	{ "*" XtNfolderCols,	NULL },
};
static List current = LIST(Resource, _current);

/* Since the _current resource values are ASCII and the numeric fields are
   integer, their converted values must be stored somewhere.  Create buffers
   for the resources which are inputted via numeric fields.
*/
static char gwidth_buf[5];
static char gheight_buf[5];
static char frow_buf[5];
static char fcol_buf[5];

/* Define the exclusive setting for the GUI Environment */
static ExclusiveItem _gui[] = {
    { (XtArgVal)"OPEN LOOK",	(XtArgVal)_openlook, False, False },
    { (XtArgVal)"Motif",	(XtArgVal)_motif, False, False },
};
static List		gui = LIST(ExclusiveItem, _gui);
static Exclusive	Gui =
  { True, "environment", "GUI Environment:",
	NULL, NULL, &gui, Report, NULL, NULL, True };

static String factory_gui = _motif;
static String current_gui;

/* Define the exclusive setting for starting desktop at login */
static ExclusiveItem	_login[] = {
	{ (XtArgVal)"Yes", (XtArgVal)_yes, False, False },
	{ (XtArgVal)"No",  (XtArgVal)_no, False, False  },
};
static List		login = LIST(ExclusiveItem, _login);
static Exclusive	Login =
    EXCLUSIVE("startAtLogin", "Start Desktop at login:", &login);

static String		factory_login = _yes;
static String		current_login;

/* Define the exclusive setting for 2D/3D */
static ExclusiveItem _interface[] = {
    { (XtArgVal)"2D", (XtArgVal)_2d, False, False },
    { (XtArgVal)"3D", (XtArgVal)_3d, False, False },
};
static List	 interface = LIST(ExclusiveItem, _interface);
static Exclusive Interface =
    EXCLUSIVE("interfaceAppearance", "Interface Appearance:", &interface);

/* Define the exclusive setting for showing full path names */
static ExclusiveItem _path[] = {
    { (XtArgVal)"Yes",	(XtArgVal)_true,	False, False },
    { (XtArgVal)"No",	(XtArgVal)_false,	False, False },
};
static List		path = LIST(ExclusiveItem, _path);
static Exclusive	Path =
    EXCLUSIVE("show_path", "Show Full Path Names:", &path);

/* Define [non]exclusive gizmos */

#define NUMERIC_GIZ(name, min, max, settings_index, label) {	\
    NULL,			/* help info */			\
    name,			/* name */			\
    NULL,			/* caption */			\
    min,			/* minimum value */		\
    max,			/* maximum value */		\
    &settings[settings_index],	/* settings */			\
    4,				/* number of digits */		\
    label,			/* "units" label */		\
    NULL, 0,			/* args and num_args */		\
}

/* Define numeric field gizmos */
static NumericGizmo
    grid_width_giz = NUMERIC_GIZ("gridWidth", 32, 499, GRID_WIDTH, TXT_GRID_WIDTH),
    grid_height_giz = NUMERIC_GIZ("gridHeight", 32, 499, GRID_HEIGHT, TXT_GRID_HEIGHT),
    folder_rows_giz = NUMERIC_GIZ("folderRows", 1, 99, FOLDER_ROWS, TXT_WINDOW_ROWS),
    folder_cols_giz = NUMERIC_GIZ("folderCols", 1, 99, FOLDER_COLS, TXT_WINDOW_COLS);

#undef NUMERIC_GIZ

/* Put gizmos together */
static GizmoRec gizmos[] = {
    { ChoiceGizmoClass,		/* &interface_giz, */ NULL },
    { ChoiceGizmoClass,		/* &show_paths_giz, */ NULL },
    { NumericGizmoClass,	&grid_width_giz, },
    { NumericGizmoClass,	&grid_height_giz, },
    { NumericGizmoClass,	&folder_rows_giz, },
    { NumericGizmoClass,	&folder_cols_giz, },
};

/* Now define "Label" gizmos which will contain control gizmos above */

#define LABEL_GIZ(name, caption, giz, num_giz) {			\
    NULL,				/* help info */			\
    name,				/* name */			\
    caption,				/* caption */			\
    giz, num_giz,			/* gizmos */			\
    OL_FIXEDCOLS, 1,			/* layout type & measure */	\
    NULL, 0,				/* args, num_args */		\
    True,				/* align captions */		\
}

static LabelGizmo
    GridSize	= LABEL_GIZ("gridSize", TXT_GRID_SIZE,
			    &gizmos[GRID_WIDTH], 2),
    FolderSize	= LABEL_GIZ("folderSize", TXT_WINDOW_SIZE,
			    &gizmos[FOLDER_ROWS], 2);
/*
    FolderSize	= LABEL_GIZ("folderSize", "Default Folder Window Size:",
			    &gizmos[FOLDER_ROWS], 2);
*/

#undef LABEL_GIZ

/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
    Import- 

	Numeric field controls pose a problem: the value for a numeric field
	(in the gizmo) is an int whereas the resource value must be ASCII.
	What's done here is the "current" buffers for the numeric field
	resources is used here temporarily to store the ASCII representation
	of the default value.  Note this is only done so that default
	resource values can be merged with the gloabl resources.

	See also Apply and Factory.  When "Apply" is called, these buffers
	are re-used to store the current values so they no longer contain
	"factory" values.  When "Factory" is called, the numeric fields are
	re-loaded with the integer values found in the DEFAULT #define's.
*/
static void
Import (XtPointer closure)
{
#define _SET_FACTORY(buf, default, index) \
    sprintf(buf, "%d", default); FACTORY(index) = buf

    _SET_FACTORY ( gwidth_buf,	DEFAULT_GRID_WIDTH,	GRID_WIDTH );
    _SET_FACTORY ( gheight_buf,	DEFAULT_GRID_HEIGHT,	GRID_HEIGHT );
    _SET_FACTORY ( frow_buf,	DEFAULT_FOLDER_ROWS,	FOLDER_ROWS );
    _SET_FACTORY ( fcol_buf,	DEFAULT_FOLDER_COLS,	FOLDER_COLS );

    merge_resources(&global_resources, &factory);
    return;

#undef	_SET_FACTORY

}				/* Import */

/****************************procedure*header*****************************
    Export-
*/
static void
Export (XtPointer closure)
{
    ExclusiveItem *	p;
    char *		value;

    p = ResourceItem(&Gui, getenv(GUI_SWITCH));
    current_gui = (p != NULL) ? (ADDR)p->addr : factory_gui;

    p = ResourceItem(&Login, getenv(DT_AT_LOGIN));
    current_login = p ? (ADDR)p->addr : factory_login;

#define _EXPORT(exclusive, index) \
    p = ResourceItem(exclusive, GLOBAL(index)); \
	CURRENT(index) = (p ? (ADDR)p->addr : FACTORY(index))

    _EXPORT ( &Interface,	INTERFACE );
    _EXPORT ( &Path,		SHOW_PATHS );

#undef	_EXPORT
#define _EXPORT(index) CURRENT(index) = \
    ((value = GLOBAL(index)) == NULL) ? FACTORY(index) : value;

    _EXPORT ( GRID_WIDTH );
    _EXPORT ( GRID_HEIGHT );
    _EXPORT ( FOLDER_ROWS );
    _EXPORT ( FOLDER_COLS );

#undef	_EXPORT

    /* Enforce our notion of valid resource values ("re-Import"). */

    merge_resources(&global_resources, &current);

}				/* Export */

/****************************procedure*header*****************************
    Create-
*/
static void
Create (Widget parent, XtPointer closure)
{
    Gui.current_item		= ResourceItem(&Gui,	current_gui);
    Login.current_item		= ResourceItem(&Login,	current_login);
    Interface.current_item	= ResourceItem(&Interface, CURRENT(INTERFACE));
    Path.current_item		= ResourceItem(&Path,	CURRENT(SHOW_PATHS));

    Gui.string		= OLG(Gui,fixedString);
    Login.string	= OLG(login,fixedString); 
    Interface.string	= OLG(interface,fixedString);
    Path.string		= OLG(Path,fixedString);

    _gui[0].name	= OLG(openlook,fixedString);
    _gui[1].name	= OLG(motif,fixedString);

    _login[0].name	= OLG(yes,fixedString);
    _login[1].name	= OLG(no,fixedString);

    _path[0].name	= OLG(yes,fixedString);
    _path[1].name	= OLG(no,fixedString);

    _interface[0].name	= OLG(2D,fixedString);
    _interface[1].name	= OLG(3D,fixedString);

    CreateExclusive(parent, &Gui, True);
    CreateExclusive(parent, &Login, True);
    CreateExclusive(parent, &Interface, True);
    CreateExclusive(parent, &Path, True);

    /* Set "previous_value" in all the settings.  This should be done from
       the resource database but for now we're making use of old code (Import
       and Export) to set up _current and _factory and then handing it off to
       new code here.
    */

#define SET_PREV(i) \
    settings[i].previous_value = (XtPointer)atoi(_current[i].value)

    SET_PREV( GRID_WIDTH );
    SET_PREV( GRID_HEIGHT );
    SET_PREV( FOLDER_ROWS );
    SET_PREV( FOLDER_COLS );

    CreateLabelGizmo(parent, &GridSize, NULL, 0);
    CreateLabelGizmo(parent, &FolderSize, NULL, 0);

#undef SET_PREV
}				/* Create */

/****************************procedure*header*****************************
    CreateLabelGizmo-
*/
#include <Xol/Category.h>

static void
CreateLabelGizmo(Widget parent, LabelGizmo * gizmo, ArgList args, Cardinal num_args)
{
    Widget	category;
    int		i;

    CreateGizmo(parent, LabelGizmoClass, gizmo, args, num_args);

    /* Set the correct (bold) font in the caption.  Get the font from the
       category widget (this is the way that it's done in CreateCaption
       (wsmproperty.c)).
    */
    for (category = parent; category != NULL; category = XtParent(category))
	if (XtIsSubclass(category, categoryWidgetClass))
	{
	    XFontStruct * bold_font;

	    XtVaGetValues(category, XtNcategoryFont, &bold_font, NULL);
	    XtVaSetValues(gizmo->captionWidget, XtNfont, bold_font, NULL);

	    break;
	}

    /* Set up callback so that ChangeBar can be set when any value changes */
    for (i = 0; i < gizmo->num_gizmos; i++)
	if (gizmo->gizmos[i].gizmo_class == NumericGizmoClass)
	{
	    NumericGizmo * numeric = (NumericGizmo *)gizmo->gizmos[i].gizmo;

	    OlAddCallback(numeric->textFieldWidget, XtNpostModifyNotification,
			  PingChangeBarCB, gizmo->captionWidget);
	}
}				/* End of CreateLabelGizmo */

/****************************procedure*header*****************************
    PingChangeBarCB-
*/
static void
PingChangeBarCB (Widget w, XtPointer client_data, XtPointer call_data)
{
    Widget	caption = (Widget)client_data;

    OlSetChangeBarState(caption, OL_NORMAL, OL_PROPAGATE);
}

/****************************procedure*header*****************************
    Report-
*/
static void
Report(Exclusive * exc)
{
    if (exc->current_item)		/* ie, SELECT */
	FooterMessage(exc->w, Dm__gettxt(TXT_footerMsg_changeGUI),
		      OL_LEFT, True);
}

/****************************procedure*header*****************************
    SetVar-
*/
static int
SetVar(String var, String value)
{
    char	buf[80];
    int		status;
    void	(*f)( int );

    sprintf(buf, "%s %s %s", SETVAR, var, value);

    /* Normally we are ignoring/catching child-process death,
       but here we want system() to catch the clean-up.
    */
    f = signal(SIGCLD, SIG_DFL);
    status = system(buf);
    signal (SIGCLD, f);
    debug((stderr, "system(%s) returned %d\n", buf, status));

    return(status);
}

/****************************procedure*header*****************************
    ApplyCB()
*/
static ApplyReturn *
ApplyCB (Widget w, XtPointer closure)
{
    static ApplyReturn	ret = { APPLY_OK };
    extern void		DmApplyShowFullPath(Boolean);

    /* Note: The "GUI Switch" and "DT at Login" capabilities require the
       presence of two external shell scripts: oladduser, to set up an
       Desktop user's environment, and olsetvar, to make the appropriate
       changes to that environment for starting the system at login.  Both of
       these scripts need to be available if this feature is to function.
    */
    if (current_gui != (ADDR)Gui.current_item->addr)
    {
	int status = SetVar(GUI_SWITCH, (String)Gui.current_item->addr);

	if (status)
	{
	    ret.bad_sheet = &desktopProperty;
	    ret.reason    = APPLY_ERROR;
	    ret.u.message = "Can't switch GUI's -- must run oladduser first";
	    return (&ret);
	}

	current_gui = (ADDR)Gui.current_item->addr;
    }

    if (current_login != (ADDR)Login.current_item->addr)
    {
	int status = SetVar(DT_AT_LOGIN, (String)Login.current_item->addr);

	if (status && (String)Login.current_item->addr == _yes)
	{
	    ret.bad_sheet = &miscProperty;
	    ret.reason    = APPLY_ERROR;
	    ret.u.message =
		"Can't start Desktop at login--must run oladduser first";
	    return (&ret);
	}

	current_login = (ADDR)Login.current_item->addr;
    }

    if (CURRENT(INTERFACE) != (ADDR)Interface.current_item->addr)
	ret.reason = APPLY_REFRESH;
    else
	ret.reason = APPLY_OK;

#define _APPLY(exclusive, index) \
	CURRENT(index) = (ADDR)(exclusive).current_item->addr; \
	OlSetChangeBarState ((exclusive).w, OL_NONE, OL_PROPAGATE)

    _APPLY (Interface,	INTERFACE );
    _APPLY ( Path,	SHOW_PATHS );

    /* Apply "Show path" to Desktop field */
    DmApplyShowFullPath(CURRENT(SHOW_PATHS) == _true);

    /* Now deal with Numeric Field Gizmos.  Normally, ManipulateGizmo with
       SetGizmoValue would be enough to save the current value in the
       resource database (using a converter).  Until that is implemented, we
       convert the current value to ASCII and store it into "current" which
       gets merged with the resource database.
    */
#undef	_APPLY
#define _APPLY(label, buf, i) \
    ManipulateGizmo(gizmos[i].gizmo_class, gizmos[i].gizmo, GetGizmoValue); \
    ManipulateGizmo(gizmos[i].gizmo_class, gizmos[i].gizmo, SetGizmoValue); \
    \
    sprintf(buf, "%d", settings[i].current_value); \
    CURRENT(i) = buf; \
    OlSetChangeBarState ((label).captionWidget, OL_NONE, OL_PROPAGATE)

    _APPLY(GridSize,	gwidth_buf, 	GRID_WIDTH);
    _APPLY(GridSize,	gheight_buf,	GRID_HEIGHT);
    _APPLY(FolderSize,	frow_buf,	FOLDER_ROWS);
    _APPLY(FolderSize,	fcol_buf,	FOLDER_COLS);

#undef	_APPLY

    merge_resources(&global_resources, &current);

    /* Now apply settings to Desktop fields */

#define CUR(i) settings[i].current_value

    GRID_WIDTH(Desktop)		= (Dimension) CUR ( GRID_WIDTH );
    GRID_HEIGHT(Desktop)	= (Dimension) CUR ( GRID_HEIGHT );
    FOLDER_ROWS(Desktop)	= (u_char) CUR ( FOLDER_ROWS );
    FOLDER_COLS(Desktop)	= (u_char) CUR ( FOLDER_COLS );

#undef CUR

    return (&ret);
}				/* ApplyCB */

/****************************procedure*header*****************************
    ResetCB()
*/
static void
ResetCB (Widget w, XtPointer closure)
{
    SetExclusive(&Gui, ResourceItem(&Gui, current_gui), OL_NONE);
    SetExclusive(&Login, ResourceItem(&Login, current_login), OL_NONE);

#define _RESET(exclusive, index) \
    SetExclusive(exclusive, ResourceItem(exclusive, CURRENT(index)), OL_NONE)

    _RESET ( &Interface,	INTERFACE );
    _RESET ( &Path,		SHOW_PATHS );

#undef	_RESET
#define _RESET(label, i) \
    ManipulateGizmo(gizmos[i].gizmo_class, gizmos[i].gizmo, ResetGizmoValue); \
    OlSetChangeBarState((label).captionWidget, OL_NONE, OL_PROPAGATE)

    _RESET ( GridSize,		GRID_WIDTH );
    _RESET ( GridSize,		GRID_HEIGHT );
    _RESET ( FolderSize,	FOLDER_ROWS );
    _RESET ( FolderSize,	FOLDER_COLS );

#undef	_RESET
}				/* ResetCB */

/****************************procedure*header*****************************
    FactoryCB-
*/
static void
FactoryCB (Widget w, XtPointer closure)
{
    SetExclusive(&Gui, ResourceItem(&Gui, factory_gui), OL_NORMAL);
    SetExclusive(&Login, ResourceItem(&Login, factory_login), OL_NORMAL);

#define _FACTORY(exclusive, index) \
	SetExclusive(exclusive, ResourceItem(exclusive, FACTORY(index)), OL_NORMAL)

    _FACTORY ( &Interface,	INTERFACE );
    _FACTORY ( &Path,		SHOW_PATHS );

#undef	_FACTORY
#define _FACTORY(label, i) \
    ManipulateGizmo(gizmos[i].gizmo_class, gizmos[i].gizmo, \
		    ReinitializeGizmoValue); \
    OlSetChangeBarState((label).captionWidget, OL_NONE, OL_PROPAGATE)

    _FACTORY ( GridSize,	GRID_WIDTH );
    _FACTORY ( GridSize,	GRID_HEIGHT );
    _FACTORY ( FolderSize,	FOLDER_ROWS );
    _FACTORY ( FolderSize,	FOLDER_COLS );

#undef	_FACTORY
}				/* FactoryCB */
