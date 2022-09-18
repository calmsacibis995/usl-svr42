/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:binder.c	1.42"

#include <ctype.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/PopupWindo.h>
#include <Xol/FButtons.h>
#include <Xol/StaticText.h>
#include <Xol/Caption.h>
#include <Gizmo/Gizmos.h>
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/PopupGizmo.h>
#include <Gizmo/InputGizmo.h>
#include <Gizmo/STextGizmo.h>
#include <Gizmo/ChoiceGizm.h>
#include <Gizmo/LabelGizmo.h>
#include <Gizmo/SpaceGizmo.h>
#include "Dtm.h"
#include "CListGizmo.h"
#include "dm_strings.h"
#include "extern.h"

typedef enum {
	PropApply, PropCancel, PropHelp
} PropMenuItemIndex;

typedef enum {
	Delete, InsertEnd, InsertBegin, VisualChange, TypingChange
} ClassAction;

typedef struct {
	Setting	class;
	Setting	pattern;
	Setting	path;
	Setting	type;
	Setting iconfile;
	Setting dflticonfile;
	Setting open;
	Setting drop;
	Setting print;
	Setting template;
} BinderSettings;

static BinderSettings inputSettings;

static void DmBinderSelect(Widget, XtPointer, XtPointer);
static void DmInsertClass(Widget, XtPointer, XtPointer);
static void DmDeleteClass(Widget, XtPointer, XtPointer);
static void DmChangeClass(Widget, XtPointer, XtPointer);
static void DmResetClass(Widget, XtPointer, XtPointer);
static void DmFindClass(DmFclassPtr, ClassAction);
static void ApplyCB(Widget, XtPointer, XtPointer);
static void SyncClassList(void);
static void UnsyncClassList();

#define XA	XtArgVal
#define ITEM_CLASS(IP) ((DmFnameKeyPtr)(ITEM_OBJ(IP)->fcp->key))

static StaticTextGizmo class_list_label = {
	NULL,			/* help */
	"class_list",		/* name */
	TXT_IB_CLASS,		/* string */
	WestGravity		/* gravity */
};

static MenuItems InsertItems[] = {
  {True, TXT_IB_ADD_DATA,     TXT_M_IB_ADD_DATA,     NULL, DmInsertClass},
  {True, TXT_IB_ADD_GRAPHICS, TXT_M_IB_ADD_GRAPHICS, NULL, DmInsertClass},
  {True, TXT_IB_ADD_CHAR,     TXT_M_IB_ADD_CHAR,     NULL, DmInsertClass},
	{NULL}
};

static MenuGizmo insertmenu = {
	NULL,
	"insert",
	NULL,
	InsertItems,
	0,
	NULL,
	CMD,
	OL_FIXEDCOLS,
	1,
	(int)0
};

static MenuItems EditItems[] = {
 {True, TXT_IB_EDIT_INSERT, TXT_M_IB_EDIT_INSERT, (void *)&insertmenu, NULL },
 {True, TXT_IB_EDIT_MODIFY, TXT_M_IB_EDIT_MODIFY, NULL,	  DmChangeClass },
 {True, TXT_IB_EDIT_RESET,  TXT_M_IB_EDIT_RESET,  NULL,	  DmResetClass  },
 {True, TXT_IB_EDIT_DELETE, TXT_M_IB_EDIT_DELETE, NULL,	  DmDeleteClass },
 {False,TXT_IB_EDIT_UNDEL,  TXT_M_IB_EDIT_UNDEL,  NULL,	  DmDeleteClass },
 {NULL}
};

static MenuGizmo editmenu = {
	NULL,
	"edit",
	NULL,
	EditItems,
	0,
	NULL,
	CMD,
	OL_FIXEDROWS,
	1,
	(Cardinal)OL_NO_ITEM
};

static CListGizmo clist = {
	"binder_clist",		/* name */
	4,			/* view width */
	NULL,			/* required property */
	True,			/* file */
	False,			/* sys class */
	False,                  /* xenix class */
	True,			/* usr class */
	True,			/* overridden class */
	True,			/* exclusives behavior */
	False,			/* noneset behavior */
	DmBinderSelect,		/* select proc */
};

static SpaceGizmo space3mm = {
	3, 2
};

static SpaceGizmo space1mm = {
	1, 2
};

static InputGizmo class = {
	NULL, "class", TXT_IB_CLASS_NAME, NULL, &inputSettings.class, 20
};

static InputGizmo pattern = {
	NULL, "pattern", "", NULL, &inputSettings.pattern, 19
};

static InputGizmo path = {
	NULL, "path", TXT_IB_PATH, NULL, &inputSettings.path, 19
};

static GizmoRec label2_gizmos[] = {
	{InputGizmoClass,	&pattern},
	{InputGizmoClass,	&path},
};

static LabelGizmo label2 = {
	NULL,			/* help */
	"label2",		/* widget name */
	TXT_IB_PATTERN,		/* caption label */
	label2_gizmos,		/* gizmo array */
	XtNumber(label2_gizmos),/* # of gizmos */
	OL_FIXEDROWS,		/* layout type */
	1,			/* measure */
	NULL,			/* arglist */
	0,			/* num_args */
	True,			/* align caption */
};

static MenuItems typeItems[] = {
	{True, TXT_TYPE_FOLDER },
	{True, TXT_TYPE_EXEC   },
	{True, TXT_TYPE_DATA   },
	{True, TXT_TYPE_PIPE   },
	{True, TXT_TYPE_CHRDEV },
	{True, TXT_TYPE_BLKDEV },
	{True, TXT_TYPE_SEM    },
	{True, TXT_TYPE_SHMEM  },
	{NULL}
};

static DmFileType typemap[] = {
	DM_FTYPE_DIR,
	DM_FTYPE_EXEC,
	DM_FTYPE_DATA,
	DM_FTYPE_FIFO,
	DM_FTYPE_CHR,
	DM_FTYPE_BLK,
	DM_FTYPE_SEM,
	DM_FTYPE_SHD
};

static MenuGizmo typemenu = {
	NULL,
	"type",
	NULL,
	typeItems,
	0,
	NULL,
	ENS,
	OL_FIXEDROWS,
	2,
	(Cardinal)OL_NO_ITEM
};

static ChoiceGizmo typeG = {
	NULL,			/* help */
	"file type",		/* widget name */
	TXT_IB_FILE_TYPE,	/* caption label */
	&typemenu,		/* menu gizmo */
	&inputSettings.type,	/* settings */
	NULL,			/* verify callback */
	NULL,			/* arglist */
	0			/* number of args */
};

static InputGizmo iconfile = {
	NULL, "iconfile", "", NULL,
	&inputSettings.iconfile, 19
};

static InputGizmo dflticonfile = {
	NULL, "dflticonfile", TXT_IB_DFLT_ICON_FILE, NULL,
	&inputSettings.dflticonfile, 16
};

static GizmoRec label3_gizmos[] = {
	{InputGizmoClass,	&iconfile},
	{InputGizmoClass,	&dflticonfile},
};

static LabelGizmo label3 = {
	NULL,			/* help */
	"label3",		/* widget name */
	TXT_IB_ICON_FILE,	/* caption label */
	label3_gizmos,		/* gizmo array */
	XtNumber(label3_gizmos),/* # of gizmos */
	OL_FIXEDROWS,		/* layout type */
	1,			/* measure */
	NULL,			/* arglist */
	0,			/* num_args */
	True,			/* align caption */
};

static InputGizmo opencmd = {
	NULL, "open", TXT_IB_OPEN, NULL, &inputSettings.open, 36
};

static InputGizmo dropcmd = {
	NULL, "drop", TXT_IB_DROP, NULL, &inputSettings.drop, 36
};

static InputGizmo printcmd = {
	NULL, "print", TXT_IB_PRINT, NULL, &inputSettings.print, 36
};

static InputGizmo template = {
	NULL, "template", TXT_IB_TEMPLATE, NULL, &inputSettings.template, 36
};

static GizmoRec label11_gizmos[] = {
	{SpaceGizmoClass,	&space3mm},
	{InputGizmoClass,	&class},

	{SpaceGizmoClass,	&space3mm},
	{LabelGizmoClass,	&label3},

	{SpaceGizmoClass,	&space3mm},
	{LabelGizmoClass,	&label2},
	{SpaceGizmoClass,	&space1mm},
	{ChoiceGizmoClass,	&typeG},

	{SpaceGizmoClass,	&space3mm},
	{InputGizmoClass,	&opencmd},
	{InputGizmoClass,	&dropcmd},
	{InputGizmoClass,	&printcmd},

	{SpaceGizmoClass,	&space3mm},
	{InputGizmoClass,	&template},
};

Arg label11_args[] = {
	XtNcenter,	True,
	XtNsameSize,	(OlDefine)OL_NONE,
};

static LabelGizmo label11 = {
	NULL,			/* help */
	"label11",		/* widget name */
	"",			/* caption label */
	label11_gizmos,		/* gizmo array */
	XtNumber(label11_gizmos),/* # of gizmos */
	OL_FIXEDCOLS,		/* layout type */
	1,			/* measure */
	label11_args,		/* arglist */
	XtNumber(label11_args),	/* num_args */
	True,			/* align caption */
};

static GizmoRec BinderGiz[] = {
	{CListGizmoClass,	&clist},
	{MenuBarGizmoClass,	&editmenu},
	{LabelGizmoClass,	&label11},
};

/* Define the Apply menu */
static MenuItems ApplyItems[] = {
	{True, TXT_IB_SAVE,	TXT_M_IB_SAVE},
	{True, TXT_FILE_EXIT,	TXT_M_FILE_EXIT},
	{True, TXT_P_HELP,	TXT_M_HELP},
	{NULL}
};

static MenuGizmo menubar = {
	NULL,			/* help */
	"menubar",		/* menu title */
	NULL,			/* widget name */
	ApplyItems,		/* menu items */
	ApplyCB,		/* default selectProc */
	NULL,			/* client data */
	CMD,			/* button type */
	OL_FIXEDROWS,		/* layout type */
	1,			/* measure */
	0			/* default item */
};

static HelpInfo BinderWinHelp = 
{ TXT_IB_TITLE, NULL, "DesktopMgr/iconset.hlp", NULL, NULL };

static PopupGizmo BinderWindow = {
	&BinderWinHelp,	/* help */
	"binder",		/* widget name */
	TXT_IB_TITLE,		/* title */
	&menubar,		/* menu */
	BinderGiz,		/* gizmo array */
	XtNumber(BinderGiz),	/* number of gizmos */
};

/* useful #defines */
#define NEW_FNKP	(DmFnameKeyPtr)((clist.cp->op->attrs & DM_B_HIDDEN) ? \
					clist.cp->op->fcp : \
					clist.cp->op->fcp->key)

static int
isemptystr(str)
register char *str;
{
	while (*str && ((*str == ' ') || (*str == '\t'))) str++;
	return(!((int)*str));
}

static char *
real_string(str)
char *str;
{
	if (!str || (*str == '\0') || isemptystr(str))
		return(NULL);
	else
		return(str);
}

static int
SetItem(idx)
Cardinal idx;
{
	Cardinal old_idx;


	/*
	 * Make sure the selected item is managed. Otherwise, find the next
	 * managed item
	 */
	for (;((clist.itp + idx)->managed == False) &&
	      (idx < clist.cp->num_objs); idx++) ;

	if (idx < clist.cp->num_objs) {
		XtSetArg(Dm__arg[0], XtNset, True);
		OlFlatSetValues(clist.boxWidget, idx, Dm__arg, 1);
	}
	else
		idx = OL_NO_ITEM;

	return(idx);
}

static void
DmShowClassStatus(fnkp)
DmFnameKeyPtr fnkp;
{
	Boolean delete;
	char *msg;

	if (fnkp->attrs & DM_B_READONLY) {
		/* read only */
		XtSetArg(Dm__arg[0], XtNsensitive, False);
		XtSetValues(GetMenu(&editmenu), Dm__arg, 1);
		msg = TXT_RO_ENTRY;
	}
	else {
		/* reset sensitivity to True first */
		XtSetArg(Dm__arg[0], XtNsensitive, True);
		XtSetValues(GetMenu(&editmenu), Dm__arg, 1);

		if (fnkp->attrs & DM_B_DELETED) {
			delete = False;
			msg = TXT_DELETED_ENTRY;
		}
		else {
			delete = True;
			msg = (fnkp->attrs & DM_B_REPLACED) ?
			    TXT_CHANGED_ENTRY : NULL;
		}

		if (EditItems[3].sensitive != delete ) {
			EditItems[1].sensitive = (XA)delete;
			EditItems[2].sensitive = (XA)delete;
			EditItems[3].sensitive = (XA)delete;
			EditItems[4].sensitive = (XA)((delete != False) ?
							 False : True);
			OlFlatRefreshItem(GetMenu(&editmenu), 1, True);
			OlFlatRefreshItem(GetMenu(&editmenu), 2, True);
			OlFlatRefreshItem(GetMenu(&editmenu), 3, True);
			OlFlatRefreshItem(GetMenu(&editmenu), 4, True);
		}
	}

	SetPopupMessage(&BinderWindow, msg ? Dm__gettxt(msg) : " ");
}

/*
 * This routine is called whenever an icon in the class list is selected.
 */
static void
DmSelectClass(ip)
DmItemPtr ip;
{
#define SETTING(FIELD)		inputSettings.FIELD.previous_value
#define _STRDUP(S)		(XtPointer)((p = (S)) ? strdup(p) : strdup(""))
	DmFclassPtr fcp = FCLASS_PTR(ip);
	DtPropListPtr plist = &(fcp->plist);
	char *p;

	/* free old strings */
	XtFree(SETTING(class));
	XtFree(SETTING(pattern));
	XtFree(SETTING(path));
	SETTING(type) = (XtPointer)(Cardinal)OL_NO_ITEM;
	XtFree(SETTING(iconfile));
	XtFree(SETTING(dflticonfile));
	XtFree(SETTING(open));
	XtFree(SETTING(drop));
	XtFree(SETTING(print));
	XtFree(SETTING(template));

	SETTING(class)=(XtPointer)strdup(((DmFnameKeyPtr)(fcp->key))->name);
	SETTING(pattern) = _STRDUP(DtGetProperty(plist, PATTERN, NULL));
	SETTING(path)    = _STRDUP(DtGetProperty(plist, FILEPATH, NULL));
	SETTING(type)    = (XtPointer)DmTypeToIdx(DtGetProperty(plist,
						 FILETYPE, NULL));
	SETTING(dflticonfile) = _STRDUP(DtGetProperty(plist,DFLTICONFILE,NULL));
	SETTING(iconfile)= _STRDUP(DtGetProperty(plist, ICONFILE, NULL));
	SETTING(open)    = _STRDUP(DtGetProperty(plist, OPENCMD, NULL));
	SETTING(drop)    = _STRDUP(DtGetProperty(plist, DROPCMD, NULL));
	SETTING(print)   = _STRDUP(DtGetProperty(plist, PRINTCMD, NULL));
	SETTING(template)= _STRDUP(DtGetProperty(plist, TEMPLATE, NULL));
	
	ManipulateGizmo(PopupGizmoClass, &BinderWindow,
			ResetGizmoValue);

	DmShowClassStatus((DmFnameKeyPtr)(fcp->key));
#undef SETTING
#undef _STRDUP
}

void
DmDisplayBinder()
{
	Widget shell = GetPopupGizmoShell(&BinderWindow);
	DmItemPtr ip;
	int i;
	Arg arg[1];

	if (shell == NULL) {
		Widget uca;

		CreateGizmo(DESKTOP_SHELL(Desktop), PopupGizmoClass,
			    &BinderWindow, NULL, 0);

		/* Change XtNcenter to True for upper control area */
		XtSetArg(arg[0], XtNupperControlArea, &uca);
		XtGetValues(BinderWindow.shell, arg, 1);
		XtSetArg(arg[0], XtNcenter, True);
		XtSetValues(uca, arg, 1);

		/* turn off focus on select */
		XtSetArg(Dm__arg[0], XtNfocusOnSelect, False);
		XtSetValues(GetMenu(&editmenu), Dm__arg, 1);

		/* register help for binder */ 
		shell = GetPopupGizmoShell(&BinderWindow);
		BINDER_HELP_ID(Desktop) = DmNewHelpAppID(XtScreen(shell),
			XtWindow(shell), Dm__gettxt(TXT_DESKTOP_MGR),
				Dm__gettxt(TXT_ICON_SETUP),
				DESKTOP_NODE_NAME(Desktop), NULL,
				"binder.icon")->app_id;
	}
	else {
		/*
		 * Need to call this here in case the last popdown was by
		 * pulling the pushpin out. In such case, ApplyCB was not
		 * called.
		 */
		UnsyncClassList();
	}

	/* select the first writable class */
	for (ip=clist.itp, i=0; i < clist.cp->num_objs; i++, ip++) {
		if (ITEM_MANAGED(ip) == False)
			continue;

		if (((DmFnameKeyPtr)(FCLASS_PTR(ip)->key))->attrs &
			DM_B_READONLY)
			continue;
		else {
			Widget hsb;

			if (ip != clist.itp) {
				Position x = ITEM_X(ip);

				/* try to center the selected icon */
				if ((int)(x-GRID_WIDTH(Desktop)*2) > 0)
					x -= GRID_WIDTH(Desktop) * 2;

				XtSetArg(arg[0], XtNset, True);
				OlFlatSetValues(clist.boxWidget,
					  ip - clist.itp, arg, 1);

				/* move icon box manually */
				XtSetArg(arg[0], XtNhScrollbar, &hsb);
				XtGetValues(clist.swinWidget, arg, 1);
				XtSetArg(arg[0], XtNsliderValue, x);
				XtSetValues(hsb, arg, 1);
				XtMoveWidget(clist.boxWidget, -x, 0);
			}

			break;
		}
	}

	/* No "writable" class, just select the first one */
	if (i == clist.cp->num_objs)
		ip = clist.itp;

	DmSelectClass(ip);
	DmShowClassStatus(ITEM_CLASS(ip));

	MapGizmo(PopupGizmoClass, &BinderWindow);

}

static void
ApplyCB(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	OlFlatCallData		*cd = (OlFlatCallData *)call_data;
	Widget			shell = GetPopupGizmoShell(&BinderWindow);

	switch (cd->item_index) {
		case PropApply:
			SyncClassList();
			BringDownPopup(shell);
			break;
		case PropCancel:
			UnsyncClassList();
			XtPopdown(shell);
			break;
		case PropHelp:
			{
    				DmHelpAppPtr help_app;

				help_app=DmGetHelpApp(BINDER_HELP_ID(Desktop));
				DmDisplayHelpSection(&(help_app->hlp_win),
						help_app->app_id, NULL, "DesktopMgr/iconset.hlp",
						"10", -1000, -1000);
			}
			break;
		default:
			break;
	}
} /* end of ApplyCB */

static char *
DmIdxToType(idx)
Cardinal idx;
{
	if (idx == OL_NO_ITEM)
		return(NULL);
	else
		return((char *)((DESKTOP_FMKP(Desktop) + idx)->name));
}

static int
DmTypeToIdx(str)
char *str;
{
	DmFmodeKeyPtr fmkp = DmStrToFmodeKey(str);

	if (fmkp)
		return(fmkp - DESKTOP_FMKP(Desktop));
	else
		return(OL_NO_ITEM);
}

static int
CheckClassName(name)
char *name;
{
	char *p;

	/* check for valid names */
	if (real_string(name) == NULL) {
		SetPopupMessage(&BinderWindow,
			 Dm__gettxt(TXT_INVALID_CLASSNAME));
		return(1);
	}

	/* check for white spaces */
	for (p=name; *p; p++)
		if (isspace(*p)) {
			_OlBeepDisplay(BinderWindow.shell, 1);
			SetPopupMessage(&BinderWindow,
				 Dm__gettxt(TXT_CLASSNAME_W_SPACE));
			return(1);
		}
	
#ifdef NOT_USE
	/* check for new name colliding with existing one */
	if (DmGetClassInfo(NEW_FNKP, name)) {
		SetPopupMessage(&BinderWindow, Dm__gettxt(TXT_DUP_CLASSNAME));
		return(1);
	}
#endif

	return(0);
}

static void
DmBinderSelect(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	OlFIconBoxButtonCD *data = (OlFIconBoxButtonCD *)call_data;
	DmItemPtr ip = ITEM_CD(data->item_data);

	DmSelectClass(ip);
}

static void
UpdateProperties(plistp, change)
DtPropListPtr plistp;
DtAttrs change;
{
#define NEW_VALUE(FIELD)	inputSettings.FIELD.current_value
#define NEW_STRING(FIELD)	real_string(NEW_VALUE(FIELD))

	if (change & DM_B_VISUAL) {
		DtSetProperty(plistp,ICONFILE, NEW_STRING(iconfile), 0);
		DtSetProperty(plistp,DFLTICONFILE,NEW_STRING(dflticonfile),0);
	}

	if (change & DM_B_TYPING) {
		DtSetProperty(plistp, PATTERN,  NEW_STRING(pattern), 0);
		DtSetProperty(plistp, FILEPATH, NEW_STRING(path), 0);
		DtSetProperty(plistp, FILETYPE, DmIdxToType(NEW_VALUE(type)),0);
	}

	/* update action properties */
	DtSetProperty(plistp, OPENCMD,  NEW_STRING(open), DT_PROP_ATTR_MENU);
	DtSetProperty(plistp, DROPCMD,  NEW_STRING(drop), DT_PROP_ATTR_MENU);
	DtSetProperty(plistp, PRINTCMD, NEW_STRING(print),DT_PROP_ATTR_MENU);
	DtSetProperty(plistp, TEMPLATE, NEW_STRING(template),0);
#undef NEW_STRING
#undef NEW_VALUE
}

/*
 * This routine is called to change an existing class.
 * The change is actually implemented as a deletion of the current class
 * and an insertion of a new class with the changes. Only the first change
 * to an existing class will cause a new structure to be created. All
 * subsequent changes will only change the new structure.
 */
static void
DmChangeClass(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
#define OLD_VALUE(FIELD)	inputSettings.FIELD.previous_value
#define NEW_VALUE(FIELD)	inputSettings.FIELD.current_value
#define NEW_PLIST		&(new_fcp->plist)
	int i;
	DtAttrs change = DM_B_NEW;
	DmFclassPtr new_fcp, fcp;
	DmFnameKeyPtr new_fnkp, fnkp;
	DmItemPtr ip;

	XtSetArg(Dm__arg[0], XtNlastSelectItem, &i);
	XtGetValues(clist.boxWidget, Dm__arg, 1);
	ip = clist.itp + i;
	fcp = ITEM_OBJ(ip)->fcp;
	fnkp = (DmFnameKeyPtr)(fcp->key);

	ManipulateGizmo(PopupGizmoClass, &BinderWindow, GetGizmoValue);

	/* check for class name change */
	if (strcmp(OLD_VALUE(class), NEW_VALUE(class))) {
		if (CheckClassName(NEW_VALUE(class)))
			return;
		change |= DM_B_NEW_NAME;
	}

	/* check for changes that affect icon visuals */
	if (strcmp(OLD_VALUE(iconfile), NEW_VALUE(iconfile)) ||
	    strcmp(OLD_VALUE(dflticonfile), NEW_VALUE(dflticonfile)))
		change |= DM_B_VISUAL;

	/* check for changes that affect file typing */
	if (strcmp(OLD_VALUE(pattern), NEW_VALUE(pattern)) ||
	    strcmp(OLD_VALUE(path), NEW_VALUE(path)) ||
	    (OLD_VALUE(type) != NEW_VALUE(type)))
		change |= DM_B_TYPING;
	
	if (fnkp->attrs & DM_B_NEW) {
		/* the entry has already been modified */
		new_fnkp = fnkp;
		new_fcp  = fnkp->fcp;
	}
	else {
		/* Create a copy of the original entry */
		change |= DM_B_REPLACED;

		/* mark old entry for deletion */
		fnkp->attrs |= DM_B_DELETED;

		/* create new entry */
		if (((new_fnkp = (DmFnameKeyPtr)calloc(1,
			 sizeof(DmFnameKeyRec))) == NULL) ||
		    ((new_fcp = DmNewFileClass(new_fnkp)) == NULL)) {
			SetPopupMessage(&BinderWindow,
				 Dm__gettxt(TXT_NO_MEMORY));
			return;
		}
	}

	if (!(new_fnkp->attrs & DM_B_NEW)) {
		/*
		 * Copy info from original structure and insert the new entry in
		 * front of the old one.
		 */
		*new_fnkp = *fnkp;
		new_fnkp->name = strdup(fnkp->name);
		new_fnkp->re  = NULL;
		new_fnkp->lre  = NULL;
		new_fnkp->next = fnkp;
		if (fnkp->prev)
			fnkp->prev->next = new_fnkp;
		fnkp->prev = new_fnkp;

		/* set up the new structure */
		new_fnkp->fcp = new_fcp;
		new_fcp->key  = (void *)new_fnkp;
		new_fcp->glyph   = fcp->glyph;
		new_fcp->glyph->count++; /* bump usage count */
		if (new_fcp->cursor) {
			new_fcp->cursor = fcp->cursor;
			new_fcp->cursor->count++; /* bump usage count */
		}
		(void)DtCopyPropertyList(&(new_fcp->plist), &(fcp->plist));

		/* update op to point to the new fcp */
		ITEM_OBJ(ip)->fcp = new_fcp;
	}
	else {
		if (new_fnkp->re) {
			free(new_fnkp->re);
			new_fnkp->re = NULL;
		}

		if (new_fnkp->lre) {
			free(new_fnkp->lre);
			new_fnkp->lre = NULL;
		}
	}

	UpdateProperties(&(new_fcp->plist), change);
	new_fnkp->attrs &= ~(DM_B_REGEXP | DM_B_FILETYPE |
			     DM_B_FILEPATH | DM_B_DELETED);
	DmInitFileClass(new_fnkp);

	/* check for name change */
	if (change & DM_B_NEW_NAME) {
		if (new_fnkp->attrs & DM_B_NEW_NAME)
			free(new_fnkp->name);
		new_fnkp->name = strdup(NEW_VALUE(class));
		ChangeCListItemLabel(&clist, i, NEW_VALUE(class));
	}

	if (change & DM_B_VISUAL) {
		if (new_fnkp->attrs & DM_B_VISUAL) {
			DmReleasePixmap(XtScreen(clist.boxWidget),
					new_fnkp->fcp->glyph);
			DmReleasePixmap(XtScreen(clist.boxWidget),
					new_fnkp->fcp->cursor);
		}
		new_fnkp->fcp->glyph = NULL;
		ChangeCListItemGlyph(&clist, i);
	}

	new_fnkp->attrs |= change; /* remember changes for "apply" processing */

	ManipulateGizmo((GizmoClass)PopupGizmoClass, &BinderWindow,
			ApplyGizmoValue);

	SetPopupMessage(&BinderWindow, Dm__gettxt(TXT_IB_APPLY_MSG));
#undef NEW_PLIST
#undef NEW_VALUE
#undef OLD_VALUE
}

static char *
DmNewClassName()
{
#define DFLT_LEN	19
	static char buffer[32];
	int	i = 1;

	do {
		sprintf(buffer, "user_class_%d", i++);
	} while (DmGetClassInfo(DESKTOP_FNKP(Desktop), buffer));

	return(buffer);
#undef DFLT_LEN
}

/*
 * This routine populates the form with default values for the specific
 * "type" of class. A new and unique class name is also generated.
 */
static void
DmNewClass(type)
int type;
{
			/*      DATA		X		NON-X */
static char *dflt_pattern[] = { "*.txt",	"*",		"*"};
static char *dflt_path[]    = { "",		"",		""};
static char *dflt_type[]    = { "DATA",		"EXEC",		"EXEC"};
static char *dflt_iconfile[]= { "datafile.icon","%f.icon",	"%f.icon"};
static char *dflt_alt_file[]= { "",		"exec.icon",	"exec.icon"};
static char *dflt_drop[]    = { "",		"",		""};
static char *dflt_open[]    = { "exec dtedit %F &",
						"exec %F &",	"exec xterm -E %F &"};
static char *dflt_print[]   = { "cat \"%F\" | $XWINHOME/bin/wrap | $XWINHOME/bin/PrtMgr -p %_DEFAULT_PRINTER &", "", ""};

#define SETTING(FIELD)		inputSettings.FIELD.previous_value

	/* free old strings */
	XtFree(SETTING(class));
	XtFree(SETTING(pattern));
	XtFree(SETTING(path));
	SETTING(type) = (XtPointer)(Cardinal)OL_NO_ITEM;
	XtFree(SETTING(iconfile));
	XtFree(SETTING(dflticonfile));
	XtFree(SETTING(open));
	XtFree(SETTING(drop));
	XtFree(SETTING(print));
	XtFree(SETTING(template));

	/* Generate a new and unique class name */
	SETTING(class) = (XtPointer)strdup(DmNewClassName());

	SETTING(pattern) = strdup(dflt_pattern[type]);
	SETTING(path)    = strdup(dflt_path[type]);
	SETTING(type)    = (XtPointer)DmTypeToIdx(dflt_type[type]);
	SETTING(dflticonfile) = strdup(dflt_alt_file[type]);
	SETTING(iconfile)= strdup(dflt_iconfile[type]);
	SETTING(open)    = strdup(dflt_open[type]);
	SETTING(drop)    = strdup(dflt_drop[type]);
	SETTING(print)   = strdup(dflt_print[type]);
	SETTING(template)= strdup("");
	
	ManipulateGizmo(PopupGizmoClass, &BinderWindow,
			ResetGizmoValue);

#undef SETTING
#undef _STRDUP
}

static void
DmInsertClass(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
#define NEW_VALUE(FIELD)	inputSettings.FIELD.current_value
#define NEW_PLIST		&(new_fcp->plist)
	OlFlatCallData		*cd = (OlFlatCallData *)call_data;
	int idx;
	DmItemPtr ip, itp;
	DmObjectPtr op;
	DmFclassPtr new_fcp;
	DmFnameKeyPtr new_fnkp, fnkp;
	char *p;

	/* get new values */
	DmNewClass(cd->item_index);

	/* allocate new structures */
	if (((new_fnkp = (DmFnameKeyPtr)calloc(1, sizeof(DmFnameKeyRec)))
              == NULL) ||
	    ((new_fcp = DmNewFileClass(new_fnkp)) == NULL)) {
		SetPopupMessage(&BinderWindow, Dm__gettxt(TXT_NO_MEMORY));
		return;
	}

	/* check class name */
	if (CheckClassName(NEW_VALUE(class)))
		return;

	/* set up the new structure */
	new_fnkp->name = strdup(NEW_VALUE(class));
	new_fnkp->attrs = DM_B_NEW | DM_B_NEW_NAME | DM_B_VISUAL | DM_B_TYPING;
	new_fnkp->re  = NULL;
	new_fnkp->lre  = NULL;
	new_fnkp->fcp = new_fcp;
	new_fcp->key  = (void *)new_fnkp;

	UpdateProperties(&(new_fcp->plist), DM_B_VISUAL | DM_B_TYPING);

	DmInitFileClass(new_fnkp);

	/* find position to be inserted */
	XtSetArg(Dm__arg[0], XtNlastSelectItem, &idx);
	XtGetValues(clist.boxWidget, Dm__arg, 1);
	ip = clist.itp + idx;
	fnkp = (DmFnameKeyPtr)(ITEM_OBJ(ip)->fcp->key);

	/* copy info from the original entry */
	new_fnkp->level = fnkp->level;

#ifdef NOT_USE
	/* insert into the list */
	if (cd->item_index == 0) {
		/* after */

		/*
		 * If the current entry is a replacement, skip over the next
		 * deleted entry and insert the new entry after the deleted
		 * entry. A replaced entry will always be followed by a
		 * deleted entry!
		 */
		if (fnkp->attrs & DM_B_REPLACED)
			fnkp = fnkp->next;

		if (fnkp->next)
			fnkp->next->prev = new_fnkp;
		new_fnkp->prev = fnkp;
		new_fnkp->next = fnkp->next;
		fnkp->next = new_fnkp;
	}
	else {
#endif
		/* before */
		if (fnkp->prev)
			fnkp->prev->next = new_fnkp;
		new_fnkp->next = fnkp;
		new_fnkp->prev = fnkp->prev;
		fnkp->prev = new_fnkp;
#ifdef NOT_USE
	}
#endif

	/** create a new item **/
	op = Dm__NewObject(clist.cp, new_fnkp->name);
	op->fcp = new_fcp;

	/* unselect the old entry BEFORE the list gets realloc'ed */
	ip->select = (XtArgVal)False;

	/* make a hole in the array */
	itp = (DmItemPtr)realloc(clist.itp, sizeof(DmItemRec) *
				 (clist.cp->num_objs));
	if (itp == NULL) {
		/* attempt to recover from error */
		clist.cp->num_objs--;
		ip->select = (XtArgVal)True;
		new_fnkp->attrs = DM_B_DELETED;
		SetPopupMessage(&BinderWindow, Dm__gettxt(TXT_NO_MEMORY));
		return;
	}

	/* ip now points to the new entry */
#ifdef NOT_USE
	if (cd->item_index == 0)
		idx++; /* AFTER */
#endif
	ip = itp + idx;
	memmove((void *)(itp + idx + 1), (void *)(itp + idx),
		sizeof(DmItemRec) * (clist.cp->num_objs - idx - 1));

	ip->label       = (XtArgVal)strdup(new_fnkp->name);
	ip->managed     = (XtArgVal)True;
	ip->select      = (XtArgVal)True; /* select the new entry */
	ip->busy        = (XtArgVal)False;
	ip->client_data = (XtArgVal)NULL;
	ip->object_ptr  = (XtArgVal)op;

	DmInitObjType(clist.boxWidget, op);
	DmSizeIcon(ip, DESKTOP_FONTLIST(Desktop), DESKTOP_FONT(Desktop));

	/* set the last selected item to the new entry */
	XtSetArg(Dm__arg[0], XtNlastSelectItem, idx);
	XtSetValues(clist.boxWidget, Dm__arg, 1);

	clist.itp = itp;
	LayoutCListGizmo(&clist, True);

	ManipulateGizmo(PopupGizmoClass, &BinderWindow, ApplyGizmoValue);

	DmShowClassStatus(ITEM_CLASS(ip));

	/* update footer message */
	SetPopupMessage(&BinderWindow, Dm__gettxt(
			(cd->item_index == 0) ? TXT_IB_ADD_DATA_MSG :
			(cd->item_index == 1) ? TXT_IB_ADD_X_MSG :
						TXT_IB_ADD_NON_X_MSG));
#undef NEW_PLIST
#undef NEW_VALUE
}

static void
DmResetClass(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	ManipulateGizmo(PopupGizmoClass, &BinderWindow, ResetGizmoValue);
}

static void
DmDeleteClass(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	int idx;
	DmItemPtr ip;
	DmFclassPtr fcp;
	DmFnameKeyPtr fnkp;
	Boolean state;

	XtSetArg(Dm__arg[0], XtNlastSelectItem, &idx);
	XtGetValues(clist.boxWidget, Dm__arg, 1);
	ip = clist.itp + idx;
	fcp = ITEM_OBJ(ip)->fcp;
	fnkp = (DmFnameKeyPtr)(fcp->key);

	if (fnkp->attrs & DM_B_DELETED) {
		/* undelete an entry */
		fnkp->attrs &= ~DM_B_DELETED;
		state = False;
	}
	else {
		/* delete an entry */
		fnkp->attrs |= DM_B_DELETED;
		state = True;
	}

#ifdef NOT_USE
	XtSetArg(Dm__arg[0], XtNbusy, state);
	OlFlatSetValues(clist.boxWidget, idx, Dm__arg, 1);
#endif

	/* update footer message */
	DmShowClassStatus(fnkp);
}

static void
UnsyncClassList()
{
	DmFnameKeyPtr fnkp;
	DmFnameKeyPtr save_fnkp;
	DmItemPtr ip;
	int i;
	int delete;
	int changed = 0;

	for (ip=clist.itp, i=0; i < clist.cp->num_objs; i++, ip++) {
		if (ITEM_MANAGED(ip) == False)
			continue;

		delete = 0;
		fnkp = ITEM_CLASS(ip);
		if (fnkp->attrs & DM_B_REPLACED) {
/*
 * Can't change the original list, another window (like "New" or "Find") may
 * be using it.
 */
#ifdef NOT_USE
			if (!(fnkp->attrs & DM_B_NEW_NAME))
				fnkp->name = NULL;

			if (!(fnkp->attrs & DM_B_VISUAL))
				fnkp->fcp->glyph = NULL;
#endif

			/* points item to the original entry */
			save_fnkp = fnkp->next;
			ITEM_OBJ(ip)->fcp = save_fnkp->fcp;

			/* turn off delete bit in the original entry */
			save_fnkp->attrs &= ~DM_B_DELETED;

			/* adjust label */
			free(ITEM_LABEL(ip));
			ip->label = (XtArgVal)strdup(save_fnkp->name);
			DmSizeIcon(ip, DESKTOP_FONTLIST(Desktop),
		       		DESKTOP_FONT(Desktop));

			delete++;
		}
		else if (fnkp->attrs & DM_B_NEW) {
			ip->managed = (XtArgVal)False;
			delete++;
		}
		else if (fnkp->attrs & DM_B_DELETED) {
			/* unmark */
			fnkp->attrs &= ~DM_B_DELETED;
			ip->busy = False;
			changed++;
		}

		if (delete) {
			/* remove from the linked list and then free it */
			if (fnkp->prev)
				fnkp->prev->next = fnkp->next;
			if (fnkp->next)
				fnkp->next->prev = fnkp->prev;
			DmFreeFileClass(fnkp);
			changed++;
		}
	}

	if (changed) {
		/* set selected item to be the first one */
		i = SetItem(0);
		DmSelectClass(clist.itp + i);
		DmShowClassStatus(ITEM_CLASS(clist.itp + i));

		LayoutCListGizmo(&clist, False);
	}
} /* UnsyncClassList() */

static void
SyncClassList()
{
#define CHANGED_CLASS	(DM_B_NEW | DM_B_REPLACED | DM_B_DELETED)
	DmFnameKeyPtr fnkp = NEW_FNKP;
	DmFnameKeyPtr fnkp_list = fnkp;
	DmFnameKeyPtr save_fnkp;
	DmFnameKeyPtr del_fnkp = NULL;

	DmItemPtr ip;
	int i;
	int idx;
	char *p;

	/*
	 * Before processing changes in the class list, make sure the
	 * affected class file header entries are flagged with DM_B_WRITE_FILE.
	 */
	for (;fnkp; fnkp=fnkp->next)
		if ((fnkp->attrs & DM_B_CLASSFILE) && fnkp->next) {
			unsigned short ilevel; /* initial file level */
			Boolean changed = False;

			/* check if any entries in this file have changed */
			for (save_fnkp=fnkp->next, ilevel=save_fnkp->level;
			     save_fnkp; save_fnkp=save_fnkp->next) {
				if (save_fnkp->level > ilevel)
					continue;
				if (save_fnkp->level < ilevel)
					break;
				if (save_fnkp->attrs & CHANGED_CLASS) {
					changed = True;
					break;
				}
			}

			if (changed == True)
				fnkp->attrs |= DM_B_WRITE_FILE;
		}

	/*
	 * Process the new list:
	 * NEW entries - turn off attributes.
	 * REPLACED entries - turn off attributes, set all the borrowed
	 *		      resources from the next deleted entries to NULL.
	 * DELETED entries - free resources and remove it from the list.
	 */
	fnkp = fnkp_list;
	while (fnkp) {
		if (fnkp->attrs & DM_B_REPLACED) {
			/* The next fnkp MUST be the one being replaced. */
			save_fnkp = fnkp->next;
/*
 * Can't change the original list, another window (like "New" or "Find") may
 * be using it.
 */
#ifdef NOT_USE
			if (!(fnkp->attrs & DM_B_NEW_NAME))
				save_fnkp->name = NULL;

			if (!(fnkp->attrs & DM_B_VISUAL))
				save_fnkp->fcp->glyph = NULL;
#endif

			/* turn off bits so that it can used */
			fnkp->attrs &= ~(DM_B_NEW | DM_B_NEW_NAME |
					 DM_B_VISUAL | DM_B_TYPING);

			fnkp = fnkp->next;
		}
		else if (fnkp->attrs & DM_B_NEW) {
			/* turn off bits so that it can used */
			fnkp->attrs &= ~DM_B_NEW;

			/* turn on replaced bit, will be used later */
			fnkp->attrs |= DM_B_REPLACED;

			fnkp = fnkp->next;
		}
		else if (fnkp->attrs & DM_B_DELETED) {
			save_fnkp = fnkp->next;

			/* remove from the linked list and then free it */
			if (fnkp->prev)
				fnkp->prev->next = fnkp->next;
			if (fnkp->next)
				fnkp->next->prev = fnkp->prev;

			/* if it is the head of the list, update the head */
			if (fnkp == fnkp_list)
				fnkp_list = fnkp->next;

			/* put it in the deleted list */
			fnkp->next = del_fnkp;
			del_fnkp = fnkp;

			fnkp = save_fnkp;
		}
		else {
			fnkp = fnkp->next;
		}
	}

	/* install the new class list */
	DESKTOP_FNKP(Desktop) = fnkp_list;

	/* now do the dynamic update */
	DmSyncWindows(fnkp_list, del_fnkp);

	/* turn off the replaced bit */
	for (fnkp=fnkp_list; fnkp; fnkp=fnkp->next)
		fnkp->attrs &= ~DM_B_REPLACED;

	/* remove the deleted entries from the class list and display it */
	for (ip=clist.itp, i=0; i < clist.cp->num_objs; i++, ip++) {
		if (ITEM_MANAGED(ip) == False)
			continue;

		if (ITEM_CLASS(ip)->attrs & DM_B_DELETED)
			ip->managed = (XtArgVal)False;
	}

	/* free the deleted list */
	for (fnkp=del_fnkp; fnkp;) {
		save_fnkp = fnkp->next;
		DmFreeFileClass(fnkp);
		fnkp = save_fnkp;
	}

	/* set selected item to be the first one */
	i = SetItem(0);
	DmSelectClass(clist.itp + i);

	LayoutCListGizmo(&clist, False);

	DmWriteFileClassDBList(fnkp_list);
}

