/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:olwsm/wsmproperty.c	1.67"

#include <stdio.h>
#include <string.h>

#include <X11/IntrinsicP.h>
#include <X11/CoreP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <Xol/OpenLookP.h>
#include <Xol/Caption.h>
#include <Xol/Category.h>
#include <Xol/MenuShell.h>
#include <Xol/ControlAre.h>     /* For extern controlAreaWidgetClass */
#include <Xol/ChangeBar.h>      /* For XtNallowChangeBars */
#include <Xol/FButtons.h>

#include "error.h"
#include <misc.h>
#include <node.h>
#include <list.h>
#include <exclusive.h>
#include <nonexclu.h>
#include <notice.h>
#include <property.h>
#include <resource.h>
#include <xtarg.h>
#include <wsm.h>

#include "../Dtm.h"
#include "../dm_strings.h"

/*
 * Convenient macros:
 */

#define MAX_BUF		256
#define DEFS		"/.Xdefaults"

#define PropertyOf(X) ((Property *)DNODE_data(X))

/*
 * Global data:
 */
extern char * NoticeMenuFields[];

List			global_resources = LISTOF(Resource);

/*
 * Local functions:
 */

static void		DestroyPropertyPopup_Phase2 OL_ARGS((
	void
));
static Widget		CreatePropertyPopup OL_ARGS((
	void
));
static void		CheckApplyAll OL_ARGS((
	Boolean			apply_all
));
static void		PopdownCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));
static void		PropertyResetCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));
static void		PropertyFactoryCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));
static void		NewPageCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));
static void		ChangeSheets OL_ARGS((
	Property *		old,
	Property *		new,
	Boolean			tell_category,
	Boolean			apply_all
));
static DNODE *		GetSheet OL_ARGS((
	Property *		sheet
));
static void		BringDownPopup OL_ARGS((
	Widget			popup
));
static Boolean		IsPinned OL_ARGS((
	Widget			popup
));
static DNODE *		CreateSheet OL_ARGS((
	Property *		p
));
static void		DeleteWindowCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));
static void		SetMinimumSize OL_ARGS((
	void
));

/*
 * Local data:
 */

static Property *	*_property;

static List		property;
static List		buffer		= LISTOF(char);
static List		resources	= LISTOF(Resource);

static DNODE *		sheets		= NULL;
static DNODE *		current_sheet;

static Widget		properties	= 0;
static Widget		category	= 0;
static Widget		popupmenu	= 0;

static String		resource_filename;

typedef struct _button {
	XtArgVal			label;
	XtArgVal			deefault;
	XtArgVal			cb;
	XtArgVal			mnemonic;
} _button;

static char * flatMenuFields[] = {
	XtNlabel,	/* label */
	XtNdefault,	/* deefault */
	XtNselectProc,	/* cb */
	XtNmnemonic,	/* mnemonic */
};

static Widget			LcaApplyMenu;
static Widget			MenupaneApplyMenu;
static _button buttons[] = {
  { (XtArgVal)"Apply", True, (XtArgVal)PropertyApplyCB, (XtArgVal)'A'},
  { (XtArgVal)"Reset", False, (XtArgVal)PropertyResetCB, (XtArgVal)'R'},
  { (XtArgVal)"Reset to Factory", False, (XtArgVal)PropertyFactoryCB, (XtArgVal)'F'},
  { (XtArgVal)"Cancel", False, (XtArgVal)DestroyPropertyPopup, (XtArgVal)'C'},
  { (XtArgVal)"Help...", False, (XtArgVal)HelpCB, (XtArgVal)'H'},
};

#define APPLY_BUTTON		0

static String		ApplyLabel =	"Apply";
static String		ApplyAllLabel = "Apply All";
static String		CurrentApplyLabel;

/* name of property sheets and corresponding help files */

static char *helpFileInfo[][2] = {
	NULL, "DesktopMgr/clrpref.hlp",
	NULL, "DesktopMgr/dskpref.hlp",
	NULL, "DesktopMgr/icnpref.hlp",
	NULL, "DesktopMgr/keypref.hlp",
	NULL, "DesktopMgr/keypref.hlp",
	NULL, "DesktopMgr/keypref.hlp",
	NULL, "DesktopMgr/keypref.hlp",
	NULL, "DesktopMgr/keypref.hlp",
	NULL, "DesktopMgr/keypref.hlp",
	NULL, "DesktopMgr/keypref.hlp",
	NULL, "DesktopMgr/keypref.hlp",
	NULL, "DesktopMgr/keypref.hlp",
	NULL, "DesktopMgr/mispref.hlp",
	NULL, "DesktopMgr/moupref.hlp",
	NULL, "DesktopMgr/moupref.hlp",
	NULL, "DesktopMgr/locpref.hlp",
};

static char *(*HelpFileInfo)[2];

static OlDtHelpInfo PrefHelp = {
	NULL, NULL, "DesktopMgr/pref.hlp", NULL, NULL
};

/**
 ** InitProperty()
 **/

void
#if	OlNeedFunctionPrototypes
InitProperty (
	Display* dpy
)
#else
InitProperty (dpy)
  Display *dpy;
#endif
{
	list_ITERATOR		I;

	Property **		p;
	Property *		q;
	int i = 0, j;


	delete_RESOURCE_MANAGER();
	resource_filename = GetPath(DEFS);

	colorProperty.pLabel = OLG(color,pageLabel);
	desktopProperty.pLabel = OLG(desktop,pageLabel);
	iconProperty.pLabel = OLG(icons,pageLabel);
	localeProperty.pLabel = OLG(setLocale,pageLabel);
	miscProperty.pLabel = OLG(misc,pageLabel);
	mouseProperty.pLabel = OLG(mouseM,pageLabel);
	settingsProperty.pLabel = OLG(settings,pageLabel);

	setUpKbdCaptions(dpy);

	_property = (Property **)malloc(sizeof(Property *)
					* (8 + numKbdSheets));
	_property[i++] = &colorProperty;
	_property[i++] = &desktopProperty;
	_property[i++] = &iconProperty;
	for (j = 0; j < numKbdSheets; j++)
	  _property[i++] = PropertyList[j];
	_property[i++] = &miscProperty;
	_property[i++] = &mouseProperty;
	_property[i++] = &settingsProperty;
	_property[i++] = &localeProperty;

	property.entry = (ADDR)_property;
	property.size = sizeof(Property *);
	property.count = i;
	property.max = 0;

	I = list_iterator(&property);
	while (p = (Property **)list_next(&I)) {
		q = *p;
		if (q->import)
			(*q->import) (q->closure);
	}
	list_clear (&buffer);

	if (read_buffer(resource_filename, &buffer)) {
		list_clear (&resources);
		buffer_to_resources (&buffer, &resources);
		merge_resources (&global_resources, &resources);
		free_resources (&resources);
	} else {
		Dm__VaPrintMsg(TXT_warningMsg_noDefaults,
				resource_filename ? resource_filename : ""); 
	}

	I = list_iterator(&property);
	while (p = (Property **)list_next(&I)) {
		q = *p;
		if (q->export)
			(*q->export) (q->closure);
	}
	UpdateResources ();

	return;
} /* InitProperty */

/**
 ** UpdateResources()
 **/

void
#if	OlNeedFunctionPrototypes
UpdateResources (
	void
)
#else
UpdateResources ()
#endif
{
	list_clear (&buffer);
	resources_to_buffer (&global_resources, &buffer);
	change_RESOURCE_MANAGER (&buffer);
	if (!write_buffer(resource_filename, &buffer)) {
		Dm__VaPrintMsg(TXT_warningMsg_cannotWrite,
			      resource_filename ? resource_filename : "");
	}

	return;
} /* UpdateResources */

/**
 ** MergeResources()
 **/

void
#if	OlNeedFunctionPrototypes
MergeResources (
	String			str
)
#else
MergeResources (str)
	String			str;
#endif
{
	list_clear (&buffer);
	list_clear (&resources);
	list_append (&buffer, str, strlen(str));
	buffer_to_resources (&buffer, &resources);
	merge_resources (&global_resources, &resources);
	free_resources (&resources);
	UpdateResources ();

	return;
} /* MergeResources */

/**
 ** DeleteResources()
 **/

void
#if	OlNeedFunctionPrototypes
DeleteResources (
	String			str
)
#else
DeleteResources (str)
	String			str;
#endif
{
	list_clear (&buffer);
	list_clear (&resources);
	list_append (&buffer, str, strlen(str));
	buffer_to_resources (&buffer, &resources);
	delete_resources (&global_resources, &resources);
	free_resources (&resources);
	UpdateResources ();

	return;
} /* DeleteResources */

/**
 ** PropertySheetByName()
 **/

void
#if	OlNeedFunctionPrototypes
PropertySheetByName (
	String			name
)
#else
PropertySheetByName (name)
	String			name;
#endif
{
	Cardinal		n;


	for (n = 0; n < property.count; n++)
		if (MATCH(_property[n]->name, name)) {
			PropertyCB (
			  (Widget)0, (XtPointer)_property[n], (XtPointer)0
			);
			break;
		}
	return;
} /* PropertySheetByName */

/**
 ** PropertyCB()
 **/

void
#if	OlNeedFunctionPrototypes
PropertyCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
#else
PropertyCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	Property *		p	= (Property *)client_data;

	DNODE *			sheet	= GetSheet(p);

	Display *		display	= XtDisplayOfObject(handleRoot);


#if	defined(MEMDEBUG)
	_SetMemutilDebug(True);
#endif

	/*
	 * Make sure the user can see the property window, if it already
	 * exists. We do this only here (not, e.g., in ChangeSheets()",
	 * to give the user the chance to NOT bring the window to the
	 * front yet still switch pages.
	 */
	if (properties)
		XRaiseWindow (display, XtWindow(properties));
	ChangeSheets ((Property *)0, p, True, True);

	return;
} /* PropertyCB */

/**
 ** DestroyPropertyPopup()
 **/

void
#if	OlNeedFunctionPrototypes
DestroyPropertyPopup (
	void
)
#else
DestroyPropertyPopup ()
#endif
{
	/*
	 * Phase 1: Just pop the window down--this will cause
	 * the XtNpopdownCallback to be issued.
	 */
	if (properties)
		XtPopdown (properties);
	return;
} /* DestroyPropertyPopup */

/**
 ** DestroyPropertyPopup_Phase2()
 **/

static void
#if	OlNeedFunctionPrototypes
DestroyPropertyPopup_Phase2 (
	void
)
#else
DestroyPropertyPopup_Phase2 ()
#endif
{
	if (properties) {
		OlUpdateDisplay (properties);
		XtDestroyWidget (properties);
		properties    = 0;
		current_sheet = 0;
	}
	return;
} /* DestroyPropertyPopup_Phase2 */

/**
 ** ResourceItem()
 **/

ExclusiveItem *
#if	OlNeedFunctionPrototypes
ResourceItem (
	Exclusive *		exclusive,
	String			name
)
#else
ResourceItem (exclusive, name)
	Exclusive *		exclusive;
	String			name;
#endif
{
	list_ITERATOR		I;

	ExclusiveItem *		p;


	if (name) {
		I = list_iterator(exclusive->items);
		while ((p = (ExclusiveItem *)list_next(&I)))
			if (MATCH(name, (String)p->addr))
				return (p);
	}
	return (0);
} /* ResourceItem */

/**
 ** NonexclusiveResourceItem()
 **/

NonexclusiveItem *
#if	OlNeedFunctionPrototypes
NonexclusiveResourceItem (
	Nonexclusive *		nonexclusive,
	String			name
)
#else
NonexclusiveResourceItem (nonexclusive, name)
	Nonexclusive *		nonexclusive;
	String			name;
#endif
{
	list_ITERATOR		I;

	NonexclusiveItem *	p;


	if (name) {
		I = list_iterator(nonexclusive->items);
		while ((p = (NonexclusiveItem *)list_next(&I)))
			if (MATCH(name, (String)p->addr))
				return (p);
	}
	return (0);
} /* NonexclusiveResourceItem */

/**
 ** CreateCaption()
 **/

Widget
#if	OlNeedFunctionPrototypes
CreateCaption (
	String			name,
	String			label,
	Widget			parent
)
#else
CreateCaption (name, label, parent)
	String			name;
	String			label;
	Widget			parent;
#endif
{
 	Widget			w;
	Widget			p	= parent;
	Screen *		screen	= XtScreenOfObject(parent);
	XFontStruct *		fs;

#define _1horzinch OlScreenMMToPixel(OL_HORIZONTAL,2.54,screen)

	w = XtVaCreateManagedWidget(
		name,
		captionWidgetClass,
		parent,
		XtNlabel, (XtArgVal) label,
		(String)0
	);
	while (p = XtParent(p)) {
		if (p == category) {
			/* Set the following resources if this	*/
			/* is a decendent of the category	*/
			/* widget. Satisfies the resource	*/
			/* specification:			*/
			/* *category*Caption			*/
		  XtVaGetValues (
				 p,
				 XtNcategoryFont,	&fs,
				 (String)0
				 );
			XtVaSetValues (
                                w,
                                XtNspace,	(XtArgVal)_1horzinch,
                                XtNalignment,	(XtArgVal)OL_TOP,
				XtNfont, 	(XtArgVal)fs,
				(String)0
			);
			break;
		}
	}
	OlAddCallback (w, XtNconsumeEvent, PopupMenuCB, (XtPointer)0);
	return (w);
} /* CreateCaption */

/**
 ** CreatePropertyPopup()
 **/

static Widget
#if	OlNeedFunctionPrototypes
CreatePropertyPopup (
	void
)
#else
CreatePropertyPopup ()
#endif
{
	Widget			pup;
	Widget			w;
	Widget			lca;
	Widget			menupane;

	list_ITERATOR		I;

	Property **		pp;
	Property *		q;

	Cardinal		i;
	Screen *		screen	= XtScreenOfObject(handleRoot);

	Display *		dpy;
	char *			temp;

	/*
	 * Create Property Window with Category manager:
	 */

        /* Fill in resources in color property sheet.  This can't */
	/* be done at compile time because we need screen. */
	colorProperty.args[0].value = OlScreenMMToPixel (
		OL_VERTICAL, 5.08, screen	/* .2 inches */
	);
        colorProperty.args[1].value = OlScreenMMToPixel (
		OL_VERTICAL, 6.35, screen	/* .25 inches */
	);
	/* Fill in resources in property sheet.	 This can't */
	/* be done at compile time because we need screen. */
#ifdef davef
	colorProperty.args[0].value = OlScreenPointToPixel (
		OL_VERTICAL, 12, screen /* 12 vertical points */
	);
#endif

	pup = XtVaCreatePopupShell(
		"properties",
		transientShellWidgetClass,
		handleRoot,
		XtNmenuButton,	(XtArgVal)False,
		XtNpushpin,	(XtArgVal)OL_OUT,
		XtNwinType,	(XtArgVal)OL_WT_CMD,
		(String)0
	);
	dpy = XtDisplay(pup);
	XtVaSetValues(pup,
		XtNtitle,	(XtArgVal)OLG(propsTitle,fixedString),
		(String)0);
		
	XtAddCallback (pup, XtNpopdownCallback, PopdownCB, (XtPointer)0);
	OlAddCallback (pup, XtNwmProtocol, DeleteWindowCB, (XtPointer)0);

	category = XtVaCreateManagedWidget(
		"category",
		categoryWidgetClass,
		pup,
		XtNcategoryLabel, (XtArgVal)OLG(Category,fixedString),
		XtNshowFooter,		(XtArgVal)True,
		XtNtraversalOn,		(XtArgVal)False,
		(String)0
	);
	XtAddCallback (category, XtNnewPage, NewPageCB, (XtPointer)0);
	
	XtVaGetValues (
		category,
		XtNlowerControlArea, (XtArgVal)&lca,
		(String)0
	);
        XtVaSetValues (
		lca,
		XtNlayoutType,	(XtArgVal)OL_FIXEDROWS,
		XtNvPad,	(XtArgVal)4,
		XtNhPad,	(XtArgVal)4,
		XtNhSpace,	(XtArgVal)0,
		XtNallowChangeBars,	(XtArgVal)True,
		(String)0
	);

	/*
	 * Create Property Window Popup Menu:
	 */

	dpy = XtDisplayOfObject(category);
	menupane = popupmenu = XtVaCreatePopupShell(
		"menu",
		popupMenuShellWidgetClass,
		category,
		XtNpushpin,	(XtArgVal)OL_NONE,
                XtNtitle,	(XtArgVal)OLG(settings,fixedString),
		(String)0
	);
	OlAddCallback (category, XtNconsumeEvent, PopupMenuCB, (XtPointer)0);

	buttons[0].label = (XtArgVal) OLG(apply,fixedString);
	temp = OLG(apply,mnemonic);
	buttons[0].mnemonic = temp[0];
	buttons[1].label = (XtArgVal) OLG(reset,fixedString);
	temp = OLG(reset,mnemonic);
	buttons[1].mnemonic = temp[0];
	buttons[2].label = (XtArgVal) OLG(factory,fixedString);
	temp = OLG(factory,mnemonic);
	buttons[2].mnemonic = temp[0];

	buttons[3].label = (XtArgVal) OLG(cancel,fixedString);
	temp = OLG(cancel,mnemonic);
	buttons[3].mnemonic = temp[0];
	buttons[4].label = (XtArgVal) OLG(helpdot,fixedString);
	temp = OLG(helpdot,mnemonic);
	buttons[4].mnemonic = temp[0];

	LcaApplyMenu = XtVaCreateManagedWidget(
		"lcaApply",
		flatButtonsWidgetClass,
		lca,
		XtNitemFields,	  (XtArgVal)flatMenuFields,
		XtNnumItemFields, (XtArgVal)XtNumber(flatMenuFields),
		XtNitems,	  (XtArgVal)buttons,
		XtNnumItems,	  (XtArgVal)XtNumber(buttons),
		(String)0
	);
        MenupaneApplyMenu = XtVaCreateManagedWidget(
		"menupaneApply",
		flatButtonsWidgetClass,
		menupane,
		XtNitemFields,	  (XtArgVal)flatMenuFields,
		XtNnumItemFields, (XtArgVal)XtNumber(flatMenuFields),
		XtNitems,	  (XtArgVal)buttons,
		XtNnumItems,	  (XtArgVal)XtNumber(buttons),
		(String)0
	);

	/* Set value of apply button "Apply/Apply All" */

	CurrentApplyLabel = ApplyLabel;

	/*
	 * Create each Property Sheet composite (with no contents):
	 */
	I = list_iterator(&property);
	while (pp = (Property **)list_next(&I)) {
		q = *pp;

#define _8horz OlScreenPointToPixel(OL_HORIZONTAL,8,screen)
#define _12vert OlScreenPointToPixel(OL_VERTICAL,12,screen)
#define _pt2vinch OlScreenMMToPixel(OL_VERTICAL,5.08,screen)
#define _pt25vinch OlScreenMMToPixel(OL_VERTICAL,6.35,screen)

		q->w = XtVaCreateManagedWidget(
			q->name,
                        controlAreaWidgetClass,
			category,
			XtNuserData,               (XtArgVal)q,
                        XtNvPad,		   (XtArgVal)_12vert,
			XtNhPad,		   (XtArgVal)_8horz,
			XtNallowChangeBars,	   (XtArgVal)True,
			XtNpageLabel,		   (XtArgVal)q->pLabel,
			(String)0
		);
		OlAddCallback (q->w, XtNconsumeEvent, PopupMenuCB, (XtPointer)0);
		XtSetValues (q->w, q->args, q->num_args);
	}

	return (pup);
} /* CreatePropertyPopup */

/**
 ** PropertySheetStatus()
 **/

void
#if	OlNeedFunctionPrototypes
PropertySheetStatus (
	Widget *		window,
	String *		sheet
)
#else
PropertySheetStatus (window, sheet)
	Widget *		window;
	String *		sheet;
#endif
{
 	String			ret	= 0;


	if (IsPinned(properties)) {
		*window = properties;
		*sheet  = PropertyOf(current_sheet)->name;
	} else {
		*window = 0;
		*sheet  = 0;
	}
	return;
} /* PropertySheetStatus */

/**
 ** CheckApplyAll()
 **/

static void
#if	OlNeedFunctionPrototypes
CheckApplyAll (
	Boolean			apply_all
)
#else
CheckApplyAll (apply_all)
	Boolean			apply_all;
#endif
{
  static Bool firsttime = TRUE;
  Display *dpy = XtDisplay(LcaApplyMenu);

  if (firsttime) {
    firsttime = FALSE;
    ApplyLabel = OLG(apply,fixedString);
    ApplyAllLabel = OLG(applyall,fixedString);
  }
	if (apply_all && CurrentApplyLabel == ApplyLabel) {
		buttons->label = (XtArgVal) ApplyAllLabel;
		CurrentApplyLabel = ApplyAllLabel;
	} else if (!apply_all && CurrentApplyLabel == ApplyAllLabel) {
		buttons->label = (XtArgVal) ApplyLabel;
		CurrentApplyLabel = ApplyLabel;
	}
	XtVaSetValues (
		LcaApplyMenu,
		XtNitemsTouched, (XtArgVal)True,
		(String)0
	);
        XtVaSetValues (
		MenupaneApplyMenu,
		XtNitemsTouched, (XtArgVal)True,
		(String)0
	);

	return;
}

/**
 ** PopdownCB()
 **/

static void
#if	OlNeedFunctionPrototypes
PopdownCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
#else
PopdownCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	dring_ITERATOR		I;

	DNODE *			q;


	I = dring_iterator(&sheets);
	while (q = dring_next(&I)) {
		Property *		p = PropertyOf(q);

		if (p->popdown)
			(*p->popdown) (p->w, p->closure);
		free_DNODE (dring_delete(&sheets, q));
	}
	DestroyPropertyPopup_Phase2 ();

	return;
} /* PopdownCB */

/**
 ** PropertyApplyCB()
 **/

void
#if	OlNeedFunctionPrototypes
PropertyApplyCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
#else
PropertyApplyCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	dring_ITERATOR		I;

	DNODE *			q;

	Boolean			ok	= True;
	Boolean			refresh = False;
	Boolean			restart = False;

	Property *		p	= 0;
	Property *		bad_p;


	BusyPeriod (handleRoot, True);

	I = dring_iterator(&sheets);
	while (ok && (q = dring_next(&I))) {
		ApplyReturn *		r;
		Boolean			changed;

		p = PropertyOf(q);

		XtVaGetValues (p->w, XtNchanged, (XtArgVal)&changed, (String)0);
		if (!p->apply || !changed)
			continue;

		r = (*p->apply)(p->w, p->closure);
		switch (r->reason) {

		case APPLY_RESTART:
			restart = True;
			goto Ok;

		case APPLY_REFRESH:
			refresh = True;
			/*FALLTHROUGH*/

		case APPLY_OK:
Ok:			XtVaSetValues (p->w, XtNchanged, (XtArgVal)False, (String)0);
			FooterMessage (category, (p->footer = 0), OL_LEFT, False);
			break;

		case APPLY_NOTICE:
			/*
			 * Don't use "w" here, because one of the sheets
			 * may have called us directly, using some other
			 * widget for "w". We want the Apply/ApplyAll
			 * button as the emanate widget.
			 */

			XtPopup (r->u.notice->w, XtGrabExclusive);
			bad_p = (r->bad_sheet? r->bad_sheet : p);
			ok = False;
			break;

		case APPLY_ERROR:
			bad_p = (r->bad_sheet? r->bad_sheet : p);
			FooterMessage (
				category,
				(bad_p->footer = r->u.message),
				OL_LEFT,
				True
			);
			ok = False;
			break;
		}
	}

	/*
	 * If we went through the loop at least once ("p" != 0),
	 * and if we emerged without an error ("ok" is True), then
	 * it is time to tell the world about the changes.
	 */
	if (p && ok) {
		UpdateResources ();
		if (restart) {
			RestartWorkspaceManager ();
			/*NOTREACHED*/
		}
		if (refresh)
			RefreshCB (w, (XtPointer)0, (XtPointer)0);
		CheckApplyAll (False);
		BringDownPopup (properties);

	/*
	 * If we had an error, then switch to that sheet so that
	 * the user can see what needs to be done.
	 */
	} else if (!ok)
		ChangeSheets ((Property *)0, bad_p, True, True);

	BusyPeriod (handleRoot, False);
	return;
} /* PropertyApplyCB */

/**
 ** PopupMenuCB()
 **/

void
#if	OlNeedFunctionPrototypes
PopupMenuCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
#else
PopupMenuCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	OlVirtualEvent		ve	= (OlVirtualEvent)call_data;


	switch (ve->virtual_name) {
	case OL_MENU:
	case OL_MENUKEY:
		if (ve->xevent->type == ButtonPress) {
			ve->consumed = True;
		}
		break;
	}

	return;
} /* PopupMenuCB */

/**
 ** PropertyResetCB()
 **/

static void
#if	OlNeedFunctionPrototypes
PropertyResetCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
#else
PropertyResetCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	Property *		p = PropertyOf(current_sheet);


	if (p->reset)
		(*p->reset) (p->w, p->closure);
	XtVaSetValues (p->w, XtNchanged, (XtArgVal)False, (String)0);
	FooterMessage (category, (p->footer = 0), OL_LEFT, False);

	return;
} /* PropertyResetCB */

/**
 ** PropertyFactoryCB()
 **/

static void
#if	OlNeedFunctionPrototypes
PropertyFactoryCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
#else
PropertyFactoryCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	Property *		p = PropertyOf(current_sheet);


	if (p->factory)
		(*p->factory) (p->w, p->closure);
	XtVaSetValues (p->w, XtNchanged, (XtArgVal)True, (String)0);
	FooterMessage (category, (p->footer = 0), OL_LEFT, False);

	return;
} /* PropertyFactoryCB */

/**
 ** NewPageCB()
 **/

static void
#if	OlNeedFunctionPrototypes
NewPageCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
#else
NewPageCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	OlCategoryNewPage *	np	= (OlCategoryNewPage *)call_data;

	Property *		old;
	Property *		new;


	XtVaGetValues (np->old_page, XtNuserData, (XtArgVal)&old, (String)0);
	XtVaGetValues (np->new_page, XtNuserData, (XtArgVal)&new, (String)0);
	ChangeSheets (old, new, False, np->apply_all);

	return;
} /* NewPageCB */

/**
 ** ChangeSheets()
 **/

static void
#if	OlNeedFunctionPrototypes
ChangeSheets (
	Property *		old,
	Property *		new,
	Boolean			tell_category,
	Boolean			apply_all
)
#else
ChangeSheets (old, new, tell_category, apply_all)
	Property *		old;
	Property *		new;
	Boolean			tell_category;
	Boolean			apply_all;
#endif
{
	Boolean			do_popup = False;


	BusyPeriod (handleRoot, True);

	/*
	 * If the property window doesn't exist yet, create it.
	 */
	if (!properties) {
		properties = CreatePropertyPopup();
		PrefHelp.app_title = OLG(propsTitle,fixedString),
		SET_HELP(properties, NULL, &PrefHelp);
		do_popup = True;
	}

	/*
	 * Specifying NULL for the "old" sheet is a convenient way
	 * of saying it's the current sheet.
	 */
	if (!old) {
		if (current_sheet)
			old = PropertyOf(current_sheet);
	}

	/*
	 * If we're already on the new sheet, we're done.
	 */
	if (old && new && old == new)
		goto Return;

	/*
	 * Tell the window manager to (temporarily) allow an arbitrarily
	 * small window size--this keeps the window manager from refusing
	 * a change to a smaller size than the current limit. Since we
	 * don't yet know the limit (not until we switch to the new sheet)
	 * we can't set a correct limit yet.
	 */
	OlClearMinHints (category);

	/*
	 * Create the new sheet if it doesn't exist yet, tell the category
	 * widget to manage and display it (unless it was the one who told
	 * US about the new sheet), then restore its last footer.
	 *
	 * (The creation routine will have provided defaults if this
	 * is a new sheet).
	 */
	if (!(current_sheet = GetSheet(new)))
		current_sheet = CreateSheet(new);
	if (tell_category)
		apply_all = OlCategorySetPage(category, new->w);
	FooterMessage (category, new->footer, OL_LEFT, False);

	/*
	 * Now pop up the property window if it isn't up yet.
	 * Only if it is already displayed is it possible to have
	 * need to show the Apply All button (think about it).
	 */
	if (do_popup)
		XtPopup (properties, XtGrabNone);
	else
		CheckApplyAll (apply_all);

	/*
	 * Tell the window manager our new resize limit.
	 */
	OlSetMinHints (category);

	/*
	 * MORE: It might be nice to keep track of where the focus
	 * was last on each sheet, and restore the focus when moving
	 * to a new sheet. Two problems make this difficult at the
	 * present:
	 *
	 *	(1) We can't move focus to a widget inside the new
	 *	sheet until it is mapped, but that doesn't happen
	 *	until after we return from this callback.
	 *
	 *	(2) The user will often choose the new sheet from
	 *	the CATEGORY menu, so that is where the focus is;
	 *	asking for the current focus on the old sheet here,
	 *	for the purpose of saving it for later, will return
	 *	nothing.
	 */

Return:
	BusyPeriod (handleRoot, False);
	return;
} /* ChangeSheets */

/**
 ** GetSheet()
 **/

static DNODE *
#if	OlNeedFunctionPrototypes
GetSheet (
	Property *		sheet
)
#else
GetSheet (sheet)
	Property *		sheet;
#endif
{
	dring_ITERATOR		I;

	DNODE *			p;


	I = dring_iterator(&sheets);
	while ((p = dring_next(&I)))
		if (sheet == PropertyOf(p))
			return (p);
	return (0);
} /* GetSheet */

/**
 ** BringDownPopup()
 **/

static void
#if	OlNeedFunctionPrototypes
BringDownPopup (
	Widget			popup
)
#else
BringDownPopup (popup)
	Widget			popup;
#endif
{
	if (!IsPinned(popup))
		XtPopdown (popup);
	return;
} /* BringDownPopup */

/**
 ** IsPinned()
 **/

static Boolean
#if	OlNeedFunctionPrototypes
IsPinned (
	Widget			popup
)
#else
IsPinned (popup)
	Widget			popup;
#endif
{
	OlDefine		pushpin;


	XtVaGetValues (popup, XtNpushpin, (XtArgVal)&pushpin, (String)0);
	switch (pushpin) {
	case OL_IN:
		return (True);
	case OL_OUT:
	default:
		return (False);
	}
} /* IsPinned */

/**
 ** CreateSheet()
 **/

static DNODE *
#if	OlNeedFunctionPrototypes
CreateSheet (
	Property *		p
)
#else
CreateSheet (p)
	Property *		p;
#endif
{
	DNODE *			sheet;
	char *			lab;


	sheet = alloc_DNODE((ADDR)p);
	dring_push (&sheets, sheet);
	(*p->create) (p->w, p->closure);
	XtVaGetValues(p->w, XtNpageLabel, (XtArgVal)&lab, (String)0);
	p->help->app_title = OLG(propsTitle,fixedString);
	SET_HELP (p->w, lab, p->help);

	return (sheet);
} /* CreateSheet */

/**
 ** DeleteWindowCB()
 **/

static void
#if	OlNeedFunctionPrototypes
DeleteWindowCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
#else
DeleteWindowCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	OlWMProtocolVerify *	p	= (OlWMProtocolVerify *)call_data;


	switch (p->msgtype) {
	case OL_WM_DELETE_WINDOW:
		XtPopdown (w);
		break;

	case OL_WM_SAVE_YOURSELF:
	case OL_WM_TAKE_FOCUS:
		OlWMProtocolAction (w, p, OL_DEFAULTACTION);
		break;
	}

	return;
} /* DeleteWindowCB */

/**
 ** BusyPeriod()
 **/

void
#if	OlNeedFunctionPrototypes
BusyPeriod (
	Widget			w,
	Boolean			busy
)
#else
BusyPeriod (w, busy)
	Widget			w;
	Boolean			busy;
#endif
{
	XDefineCursor (
		XtDisplayOfObject(w),
		XtWindowOfObject(w),
		(busy?
			  GetOlBusyCursor(XtScreenOfObject(w))
			: GetOlStandardCursor(XtScreenOfObject(w))
		)
	);
	return;
} /* BusyPeriod */

static void
#if	OlNeedFunctionPrototypes
HelpCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
#else
HelpCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	static DmHelpAppPtr hap = NULL;
	Property *p = PropertyOf(current_sheet);

	if (hap == NULL) {
		hap = (DmHelpAppPtr)DmNewHelpAppID(XtScreen(properties),
				XtWindow(properties), Dm__gettxt(TXT_DESKTOP_MGR),
				OLG(propsTitle,fixedString), DESKTOP_NODE_NAME(Desktop),
				NULL, "prefldr.icon");

		HelpFileInfo = helpFileInfo;

		(*HelpFileInfo++)[0] = OLG(color, pageLabel);
		(*HelpFileInfo++)[0] = OLG(desktop, pageLabel);
		(*HelpFileInfo++)[0] = OLG(icons, pageLabel);
		(*HelpFileInfo++)[0] = OLG(core, pageLabel);
		(*HelpFileInfo++)[0] = OLG(cutcopypaste, pageLabel);
		(*HelpFileInfo++)[0] = OLG(interwindow, pageLabel);
		(*HelpFileInfo++)[0] = OLG(intrawindow, pageLabel);
		(*HelpFileInfo++)[0] = OLG(mouse, pageLabel);
		(*HelpFileInfo++)[0] = OLG(scrolling, pageLabel);
		(*HelpFileInfo++)[0] = OLG(textselection, pageLabel);
		(*HelpFileInfo++)[0] = OLG(textedit, pageLabel);
		(*HelpFileInfo++)[0] = OLG(system, pageLabel);
		(*HelpFileInfo++)[0] = OLG(misc, pageLabel);
		(*HelpFileInfo++)[0] = OLG(mouseM, pageLabel);
		(*HelpFileInfo++)[0] = OLG(settings, pageLabel);
		(*HelpFileInfo++)[0] = OLG(setLocale, pageLabel);
	}

	for (HelpFileInfo = helpFileInfo;
		HelpFileInfo < (helpFileInfo + XtNumber(helpFileInfo));
		HelpFileInfo++) {

		if (strcmp((*HelpFileInfo)[0], p->name) == 0) {
			DmDisplayHelpSection(&(hap->hlp_win), hap->app_id,
				NULL, (*HelpFileInfo)[1], NULL, UNSPECIFIED_POS,
				UNSPECIFIED_POS);
			return;
		}
	}
	/* should never get here */
} /* end of HelpCB */
