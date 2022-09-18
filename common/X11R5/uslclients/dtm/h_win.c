/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:h_win.c	1.97"

/******************************file*header********************************

    Description:
     This file contains the source code for creating a help window
	and displaying a section or a string in a help file, popping and
	pushing a "visited" section from/onto a stack, popping up the help
	window, and closing a help window.
*/
                              /* #includes go here     */

#include <stdio.h>
#include <sys/stat.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLook.h>
#include <Xol/Form.h>
#include <Xol/PopupWindo.h>
#include <Xol/ScrolledWi.h>
#include <Xol/Mag.h>
#include <Xol/FButtons.h>

#include <Gizmo/Gizmos.h>
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/BaseWGizmo.h>
#include "FormGizmo.h"

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
          1. Private Procedures
          2. Public  Procedures
*/
                         /* private procedures         */

static void HelpWinWMCB(Widget w, XtPointer client_data, XtPointer call_data);
static void PopupHelpWindow(DmHelpWinPtr hwp);
static void DisplayHelpString(DmHelpWinPtr hwp, char *title, char *string,
			Boolean realized);
static void EventHandler(Widget w, XtPointer client_data, XEvent *xevent,
			Boolean *continue_to_use);
static void DeactivateMenuBar(Widget menubar);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

#define STACK_STEP	16

#define XA	XtArgVal
static MenuItems GoToMenuItems[] = {
 { True, TXT_HW_TOC,        TXT_M_HW_TOC,        NULL, DmTOCCB },
 { True, TXT_HW_GLOSSARY,   TXT_M_HW_GLOSSARY,   NULL, DmGlossaryCB },
 { True, TXT_HELP_HELPDESK, TXT_M_HELP_HELPDESK, NULL, DmOpenHelpDeskCB },
 { True, TXT_HW_USING_HELP, TXT_M_HW_USING_HELP, NULL, DmUsingHelpCB },
 { NULL }
};

MENU("gotomenu", GoToMenu);

static MenuItems HelpWinMenuBarItems[] = {
 { True, TXT_HW_PREV,      TXT_M_HW_PREV,      NULL, DmPrevSectionCB },
 { True, TXT_HW_NEXT,      TXT_M_HW_NEXT,      NULL, DmNextSectionCB },
 { True, TXT_HW_BACKTRACK, TXT_M_HW_BACKTRACK, NULL, DmBackTrackCB },
 { True, TXT_HW_GOTO,      TXT_M_HW_GOTO,      (void *)&GoToMenu, NULL },
 { True, TXT_HW_BOOKMARK,  TXT_M_HW_BOOKMARK,  NULL, DmBookmarkCB },
 { True, TXT_HW_SEARCH,    TXT_M_HW_SEARCH,    NULL, DmSearchCB },
 { True, TXT_HW_NOTES,     TXT_M_HW_NOTES,     NULL, DmNotesCB },
 { NULL }
};

MENUBAR("helpmenubar", HelpWinMenuBar);

static FormGizmo helpform = {
     "helpform",    /* name */
     OL_HORIZONTAL, /* orientation */
     NULL,          /* formWidget */
};

static GizmoRec hwin_gizmos[] = {
     {FormGizmoClass, &helpform},
};

static HelpInfo HelpWinHelpInfo = {
	TXT_G_PRODUCT_NAME, NULL, "DesktopMgr/help.hlp", "10", NULL
};

static BaseWindowGizmo HelpWindow = {
	&HelpWinHelpInfo,/* help */
	"helpwindow",    /* shell widget name */
	NULL,            /* title */
	&HelpWinMenuBar, /* menu bar */
	NULL,            /* gizmo array */
	0,               /* # of gizmos in array */
	"",              /* icon_name */
	"",              /* name of pixmap file */
	" ",             /* error message */
	" ",             /* status message */
	75               /* size of error message in percent of footer */
};

#undef XA

static Boolean	first = True;
static Pixmap	dflt_pixmap = NULL;

/***************************public*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
 * This function creates a help window, open the help file, and display
 * the specified section in the help window. If file_name or sect_name
 * are NULL, it means the current file_name or sect_name.
 */
int
DmCreateHelpWindow(
	DmHelpWinPtr   hwp,
	int            x,
	int            y,
	char           *geom_str,
	Boolean        iconic)
{
	int             cnt = 0;
	char            buf[256];
	DmHelpAppPtr    hap;
	DmMenuItemsPtr  menubar_items;
	BaseWindowGizmo *base;

	if (first) {
		DmGlyphPtr gp = NULL;

		if (OlGetGui() == OL_OPENLOOK_GUI) {
			gp = DmGetPixmap(DESKTOP_SCREEN(Desktop), "exec.icon");
			if (gp != NULL)
				dflt_pixmap = gp->pix;
		}
		first = False;
	}
	
	hap = DmGetHelpApp(hwp->app_id);

	if (OlGetGui() == OL_OPENLOOK_GUI) {
		HelpWindow.gizmos = hwin_gizmos;
		HelpWindow.num_gizmos = 1;
	}
	else {
		HelpWindow.gizmos = NULL;
		HelpWindow.num_gizmos = 0;
	}

	/* make a copy of the basewindow gizmo structure */
	base = CopyGizmo(BaseWindowGizmoClass, &HelpWindow);
	hwp->gizmo_shell = base;
	hwp->attrs       = DM_B_HELP_WIN;
	base->title      = NULL;
	base->icon_name  = NULL;

	((MenuGizmo *)(base->menu))->client_data = (XtPointer)hwp;
	((MenuGizmo *)QueryGizmo(MenuGizmoClass, base->menu, GetGizmoGizmo,
		"gotomenu"))->client_data = (XtPointer)hwp;

	cnt = 0;
	XtSetArg(Dm__arg[cnt], XtNwinType,           OL_WT_HELP); cnt++;
	XtSetArg(Dm__arg[cnt], XtNpushpin,           OL_IN); cnt++;
	XtSetArg(Dm__arg[cnt], XtNwindowHeader,      True); cnt++;
	XtSetArg(Dm__arg[cnt], XtNmenuButton,        False); cnt++;
	XtSetArg(Dm__arg[cnt], XtNmenuType,          OL_MENU_LIMITED); cnt++;
	XtSetArg(Dm__arg[cnt], XtNmappedWhenManaged, False); cnt++;
	XtSetArg(Dm__arg[cnt], XtNresizeCorners,     True); cnt++;

	hwp->shell = CreateGizmo(NULL, BaseWindowGizmoClass, base, Dm__arg, cnt);
	hwp->menubar = ((MenuGizmo *)(base->menu))->child;

	if (OlGetGui() == OL_OPENLOOK_GUI) {
		Widget form;
		int    cnt = 0;

		form = (Widget)QueryGizmo(BaseWindowGizmoClass, base,
				GetGizmoWidget, "helpform");

		cnt = 0;
		XtSetArg(Dm__arg[cnt], XtNweight,      0); ++cnt;
		XtSetArg(Dm__arg[cnt], XtNborderWidth, 0); ++cnt;

		if (hap->icon_pixmap) {
			XtSetArg(Dm__arg[cnt], XtNpixmap, hap->icon_pixmap); ++cnt;
		} else if (x != UNSPECIFIED_POS && y != UNSPECIFIED_POS) {
			XtSetArg(Dm__arg[cnt], XtNmouseX, x); ++cnt;
			XtSetArg(Dm__arg[cnt], XtNmouseY, y); ++cnt;
			XtSetArg(Dm__arg[cnt], XtNpixmap, NULL); ++cnt;
		} else {
			XtSetArg(Dm__arg[cnt], XtNpixmap,
				(hap->icon_pixmap ? hap->icon_pixmap : dflt_pixmap));
			++cnt;
		}
		hwp->magnifier = XtCreateManagedWidget("magnifier",
						magWidgetClass, form, Dm__arg, cnt);

		XtSetArg(Dm__arg[0], XtNxRefWidget, hwp->magnifier);
		XtSetArg(Dm__arg[1], XtNxAddWidth,  True);
		XtSetArg(Dm__arg[2], XtNxOffset,    10);

		hwp->swin = XtCreateManagedWidget("swin",
					scrolledWindowWidgetClass, form, Dm__arg, 3);
	} else {
		hwp->swin = GetBaseWindowScroller(base);
		hwp->magnifier = NULL;
	}

	/* hddp->key_color must be already initialized in DmOpenDesktop()
	 * or DmInitHelpDesk() and updated whenever the Help Highlighting
	 * color is changed in the Color property sheet.
	 */
	cnt = 0;
	XtSetArg(Dm__arg[cnt], XtNkeyColor, hddp->key_color); cnt++;
	hwp->htext = XtCreateManagedWidget("hypertext", hyperTextWidgetClass,
					   hwp->swin, Dm__arg, cnt);

	XtAddCallback(hwp->htext, XtNselect, DmHtextSelectCB, (XtPointer)hwp);

	hwp->def_shell    = NULL;
	hwp->gloss_shell  = NULL;
	hwp->notes_shell  = NULL;
	hwp->search_shell = NULL;
	hwp->bmark_shell  = NULL;
	/* initialize stack for Backtrack to NULL */
	hwp->stack        = NULL;
	/* initialize ptr to current section to NULL */
	hwp->hsp          = NULL;
	hwp->hfp          = NULL;

	return(0);

} /* end of DmCreateHelpWindow */

/****************************procedure*header*****************************
 * This function displays the specified section in the help window.
 * A history stack of previous displayed locations are kept in the
 * window structure, so that users can backtrack.
 */

int
DmDisplayHelpSection(
	DmHelpWinPtr   hwp,
	int            app_id,
	char           *title,
	char           *file_name,
	char           *sect_name,
	int            x,
	int            y)
{
	Boolean sensitive;
	char buf[256];
	char *file_sv = NULL;
	char *sectname_sv = NULL;
	char *secttag_sv = NULL;
	Boolean realized = True;
	DmHelpSectPtr hsp;
	DmHelpAppPtr hap = DmGetHelpApp(app_id);

	if (file_name == NULL && sect_name == NULL)
		return;

	if (hwp->shell == NULL) {
		realized = False;
		hwp->app_id = app_id;
		if (DmCreateHelpWindow(hwp, x, y, NULL, False))
			return(-1);
	} else {
		/* if requested section is being displayed, simply return */
		if (hwp->hfp && strcmp(hwp->hfp->name, file_name) == 0) {
			if ((sect_name && strcmp(hwp->hsp->name, sect_name) == 0) ||
		    	    (sect_name && strcmp(hwp->hsp->tag, sect_name) == 0) ||
		         (sect_name == NULL &&
					strcmp(hwp->hsp->name, DEFAULT_SECTION_NAME) == 0)) {
				/* Just raise the help window and return */
				XRaiseWindow(XtDisplay(hwp->shell), XtWindow(hwp->shell));
				return;
			}
		}

		if (hwp->hsp && hwp->hfp) {
			/* save these for DmPushHelpSection() */
			if (hwp->hfp->name)
				file_sv = strdup(hwp->hfp->name);
			if (hwp->hsp->name)
				sectname_sv = strdup(hwp->hsp->name);
			if (hwp->hsp->tag)
				secttag_sv  = strdup(hwp->hsp->tag);
		}

		if (hwp->magnifier) {
			int cnt = 0;

			if (hap->icon_pixmap) {
				XtSetArg(Dm__arg[cnt], XtNpixmap, hap->icon_pixmap); ++cnt;
			} else if (x != UNSPECIFIED_POS && y != UNSPECIFIED_POS) {
				XtSetArg(Dm__arg[cnt], XtNmouseX, x); ++cnt;
				XtSetArg(Dm__arg[cnt], XtNmouseY, y); ++cnt;
				XtSetArg(Dm__arg[cnt], XtNpixmap, NULL); ++cnt;
			} else {
				XtSetArg(Dm__arg[cnt], XtNpixmap,
					(hap->icon_pixmap ? hap->icon_pixmap : dflt_pixmap));
				++cnt;
			}
			XtSetValues(hwp->magnifier, Dm__arg, cnt);
		}
		DmVaDisplayStatus((DmWinPtr)hwp, 0, NULL);
	}

	if (file_name) {
		if (hwp->hfp == NULL ||
		   (hwp->hfp && strcmp(hwp->hfp->name, file_name))) {

			struct stat   hstat;
			DmHelpFilePtr hfp;

			if ((hfp = (DmHelpFilePtr)DmOpenHelpFile(app_id, file_name))
			           == NULL) {

				if (hwp->hfp && hwp->hsp) {
					DmPushHelpStack(hwp,file_sv, sectname_sv, secttag_sv);
					DmCloseHelpFile(hwp->hfp);
					hwp->hfp = NULL;
					hwp->hsp = NULL;
				}

				sprintf(buf, "%s %s", hap->title,Dm__gettxt(TXT_HELP_STR));
				DisplayHelpString(hwp, buf, Dm__gettxt(TXT_NO_HELP_AVAIL),
					realized);

				if (file_sv)
					FREE(file_sv);
				if (sectname_sv)
					FREE(sectname_sv);
				if (secttag_sv)
					FREE(secttag_sv);
				return(-1);
			}

			/* release all previous resources */
			if (hwp->hfp)
				DmCloseHelpFile(hwp->hfp);

			hwp->hfp = hfp;
		}
	}

	if (sect_name) {
		if((hsp = DmGetSection(hwp->hfp, sect_name)) == NULL) {

			if (hwp->hfp && hwp->hsp) {
				DmPushHelpStack(hwp,file_sv, sectname_sv, secttag_sv);
				DmCloseHelpFile(hwp->hfp);
				hwp->hfp = NULL;
				hwp->hsp = NULL;
			}

			sprintf(buf, "%s %s", hap->title,Dm__gettxt(TXT_HELP_STR));
			DisplayHelpString(hwp, buf, Dm__gettxt(TXT_NO_HELP_AVAIL),
				realized);
			DmVaDisplayStatus((DmWinPtr)hwp, 1, TXT_SECT_NOT_FOUND,
				sect_name);

			if (file_sv)
				FREE(file_sv);
			if (sectname_sv)
				FREE(sectname_sv);
			if (secttag_sv)
				FREE(secttag_sv);
		    return(-1);
		}
			
	} else {
		/*
		 * Defaults to the first section only if a new file
		 * is specified.
		 */
		if (file_name)
			hsp = hwp->hfp->sections;
	}

	hap = DmGetHelpApp(app_id);

	if (hsp != hwp->hsp) {

		/* set up hypertext widget */
		char   buf[256];
		Widget vsb;

		/* cook it first */
		DmProcessHelpSection(hsp);

		/*
		 * Help window title should be "<app_title> Help: <sect_name>".
		 * If application is a module of the desktop manager, use the
		 * name of the module as app_title.
		 */
		if (strcmp(hsp->name, DEFAULT_SECTION_NAME) != 0)
			sprintf(buf, "%s %s: %s", hap->title, Dm__gettxt(TXT_HELP_STR),
				hsp->name);
		else if (title)
			sprintf(buf, "%s %s: %s", hap->title,
					Dm__gettxt(TXT_HELP_STR), title);
		else if (hwp->hfp->title)
			sprintf(buf, "%s %s: %s", hap->title,
					Dm__gettxt(TXT_HELP_STR), hwp->hfp->title);
		else
			sprintf(buf, "%s %s", hap->title, Dm__gettxt(TXT_HELP_STR));

		/* reposition view to beginning of section */
		XtMoveWidget(hwp->htext, 0, 0);

		XtSetArg(Dm__arg[0], XtNvScrollbar, &vsb);
		XtGetValues(hwp->swin, Dm__arg, 1);

		XtSetArg(Dm__arg[0], XtNsliderValue, 0);
		XtSetValues(vsb, Dm__arg, 1);

		XtSetArg(Dm__arg[0], XtNtitle, buf);
		XtSetValues(hwp->shell, Dm__arg, 1);

		XtSetArg(Dm__arg[0], XtNstring, hsp->cooked_data);
		XtSetValues(hwp->htext, Dm__arg, 1);

		if (strcmp(hsp->name, DEFAULT_SECTION_NAME) == 0) {
			DeactivateMenuBar(hwp->menubar);
		} else {
			/* a flag to indicate whether any button is turned off */
			Boolean set = False;

			/* Activate Go To, Bookmark, Search and Notes buttons */
			XtSetArg(Dm__arg[0], XtNsensitive, True);
			OlFlatSetValues(hwp->menubar, 3, Dm__arg, 1);
			OlFlatSetValues(hwp->menubar, 4, Dm__arg, 1);
			OlFlatSetValues(hwp->menubar, 5, Dm__arg, 1);
			OlFlatSetValues(hwp->menubar, 6, Dm__arg, 1);

			/*
			 * The following checks are not mutually exclusive because
			 * a help file can contain only one section and there may
			 * or may not be a section to backtrack to regardless of
			 * whether this new section is the first or last section
			 * in the file.
			 */
			if (hsp == hwp->hfp->sections) {
				/* is first section - deactivate Previous Topic */
				XtSetArg(Dm__arg[0], XtNsensitive, False);
				OlFlatSetValues(hwp->menubar, 0, Dm__arg, 1);
				set = True;
			} else {
				/*
				 * need to do this in case DmChgHelpWinBtnState is
				 * not called.
				 */
				XtSetArg(Dm__arg[0], XtNsensitive, True);
				OlFlatSetValues(hwp->menubar, 0, Dm__arg, 1);
			}

			if ((int)(hsp - hwp->hfp->sections) ==
				(int)(hwp->hfp->num_sections - 1)) {

				/* is last section - deactivate Next Topic */
				XtSetArg(Dm__arg[0], XtNsensitive, False);
				OlFlatSetValues(hwp->menubar, 1, Dm__arg, 1);
				set = True;
			} else {
				/*
				 * need to do this in case DmChgHelpWinBtnState is
				 * not called.
				 */
				XtSetArg(Dm__arg[0], XtNsensitive, True);
				OlFlatSetValues(hwp->menubar, 1, Dm__arg, 1);
			}

			if (hwp->stack == NULL || hwp->sp == -1) {
				/* nothing to backtrack to */
				XtSetArg(Dm__arg[0], XtNsensitive, False);
				OlFlatSetValues(hwp->menubar, 2, Dm__arg, 1);
				set = True;
			} else {
				/*
				 * need to do this in case DmChgHelpWinBtnState is
				 * not called.
				 */
				XtSetArg(Dm__arg[0], XtNsensitive, True);
				OlFlatSetValues(hwp->menubar, 2, Dm__arg, 1);
			}
			DmChgHelpWinBtnState(hwp, set, True);
		}

		/* update notes window if it's up */
		if (hwp->notes_shell != NULL) {
			if ((GetWMState(XtDisplay(hwp->notes_shell),
				XtWindow(hwp->notes_shell))) == NormalState)

				DmReadNotes(hwp, hsp->tag ? hsp->tag : hsp->name);
		}

		/*
		 * It is save to push section onto stack at this point but
		 * only do so if not displaying 1st section.
		 */
		if (hwp->hsp != NULL) {
			DmPushHelpStack(hwp, file_sv, sectname_sv, secttag_sv);
			if (file_sv)
				FREE(file_sv);
			if (sectname_sv)
				FREE(sectname_sv);
			if (secttag_sv)
				FREE(secttag_sv);
		}
		hwp->hsp = hsp; /* set current segment */
	}
	if (realized)
		XRaiseWindow(XtDisplay(hwp->shell), XtWindow(hwp->shell));
	else
		PopupHelpWindow(hwp);

	/* If help is on a modal shell, add help window to the modal cascade */
	if (XtModalCascadeActive(hwp->shell))
		XtAddGrab(hwp->shell, False, False);

	return(0);

} /* end of DmDisplayHelpSection */

/****************************procedure*header*****************************
 * This function displays the specified string in the help window.
 */

int
DmDisplayHelpString(
	DmHelpWinPtr hwp,
	int          app_id,
	char         *title,
	char         *string,
	int          x,
	int          y)
{
	Widget       vsb;
	char         buf[256];
	Boolean      realized = True;
	DmHelpAppPtr hap = DmGetHelpApp(app_id);

	if (string == NULL)
		return;

	if (hwp->shell == NULL) {
		realized = False;
		hwp->app_id = app_id;
		if (DmCreateHelpWindow(hwp, x, y, NULL, False))
			return(-1);

	} else {

		if (hwp->magnifier) {
			int cnt = 0;

			if (hap->icon_pixmap) {
				XtSetArg(Dm__arg[cnt], XtNpixmap, hap->icon_pixmap); ++cnt;
			} else if (x != UNSPECIFIED_POS && y != UNSPECIFIED_POS) {
				XtSetArg(Dm__arg[cnt], XtNmouseX, x); ++cnt;
				XtSetArg(Dm__arg[cnt], XtNmouseY, y); ++cnt;
				XtSetArg(Dm__arg[cnt], XtNpixmap, NULL); ++cnt;
			} else {
				XtSetArg(Dm__arg[cnt], XtNpixmap,
					(hap->icon_pixmap ? hap->icon_pixmap : dflt_pixmap));
				++cnt;
			}
			XtSetValues(hwp->magnifier, Dm__arg, cnt);
		}
	}

	/* update help window title */
	if (!title)
		sprintf(buf, "%s %s", hap->title, Dm__gettxt(TXT_HELP_STR));
	else
		sprintf(buf, "%s %s: %s", hap->title, Dm__gettxt(TXT_HELP_STR),
			title);

	/* reposition view to beginning of section */
	XtMoveWidget(hwp->htext, 0, 0);

	XtSetArg(Dm__arg[0], XtNvScrollbar, &vsb);
	XtGetValues(hwp->swin, Dm__arg, 1);

	XtSetArg(Dm__arg[0], XtNsliderValue, 0);
	XtSetValues(vsb, Dm__arg, 1);

	/* save current section, if any, on visited section stack */
	if (hwp->hsp && hwp->hfp) {
		DmPushHelpStack(hwp, hwp->hfp->name, hwp->hsp->name, hwp->hsp->tag);
		DmCloseHelpFile(hwp->hfp);
		hwp->hfp = NULL;
		hwp->hsp = NULL;
	}

	DisplayHelpString(hwp, buf, string, realized);

	/* If help is on a modal shell, add help window to the modal cascade */
	if (XtModalCascadeActive(hwp->shell))
		XtAddGrab(hwp->shell, False, False);

} /* end of DmDisplayHelpString */

/*************************************************************************
 * This variable is only used by DmPushHelpStack() and DmPopHelpStack() to
 * prevent a push operation when a pop operation is in progress. When a
 * location is being pop off of the stack, the view needs to be updated by
 * calling DmDisplayHelpSection(). The problem is that this function will
 * push the current location onto the stack.
 */
static int pop_in_progress = 0;

/****************************procedure*header*****************************
 * This routine pushes the current view location onto a stack.
 */
int
DmPushHelpStack(hwp, file, sect_name, sect_tag)
DmHelpWinPtr hwp;
char *file;
char *sect_name;
char *sect_tag;
{
	DmHelpLocPtr hlp;

	DmVaDisplayStatus((DmWinPtr)hwp, 0, NULL);

	if (pop_in_progress)
		return(1);

	if ((hwp->stack == NULL) || (hwp->sp == (hwp->stack_size - 1))) {

		/* expand stack allocation */
		hwp->stack_size += STACK_STEP;
		hlp = (DmHelpLocPtr)realloc(hwp->stack,
				 hwp->stack_size * sizeof(DmHelpLocRec));

		if (hlp == NULL) {
			hwp->stack_size -= STACK_STEP;
			return(1);
		}
		hwp->stack = hlp;
	}

	hlp = hwp->stack + ++(hwp->sp); /* bump stack ptr */
	hlp->file = strdup(file);

	if (sect_tag)
		hlp->sect_tag  = strdup(sect_tag);
	else
		hlp->sect_tag  = NULL;

	if (sect_name)
		hlp->sect_name = strdup(sect_name);
	else
		hlp->sect_name = NULL;

	/*
	 * Activate Backtrack button which may be already activated
	 * but it's cheaper to just do one XtSetValues instead of a
	 * XtGetValues followed by a XtSetValues.
	 */
	OlVaFlatSetValues(hwp->menubar, 2, XtNsensitive, True, NULL);

	return(0);

} /* end of DmPushHelpStack */

/****************************procedure*header*****************************
 * Pops a section off the stack of "visited" sections.
 */
int
DmPopHelpStack(w, hwp)
Widget       w;
DmHelpWinPtr hwp;
{
	DmHelpLocPtr hlp;
	char *file = NULL;
	char *sect_name = NULL;
	char *sect_tag = NULL;

	if (hwp->hfp->name)
		file = strdup(hwp->hfp->name);

	if (hwp->hsp->name)
		sect_name = strdup(hwp->hsp->name);

	if (hwp->hsp->tag)
		sect_tag = strdup(hwp->hsp->tag);

	DmVaDisplayStatus((DmWinPtr)hwp, 0, NULL);
	hlp = hwp->stack + (hwp->sp)--;

	/*
	 * To prevent DmDisplayHelpSection from pushing the same thing
	 * that was just poppped here, set the pop_in_progress flag.
	 */
	pop_in_progress = 1;

	/* check if the section is "TOC" */
	if (!strcmp(hlp->sect_name, TABLE_OF_CONTENTS)) {

		if (strcmp(hlp->file, hwp->hfp->name) == 0 &&
		    hwp->hfp->toc != NULL) {

			Widget	vsb;

			DmProcessHelpSection(hwp->hfp->toc);

			/* reposition htext to (0,0) */
			XtMoveWidget(hwp->htext, 0, 0);

			XtSetArg(Dm__arg[0], XtNvScrollbar, &vsb);
			XtGetValues(hwp->swin, Dm__arg, 1);

			XtSetArg(Dm__arg[0], XtNsliderValue, 0);
			XtSetValues(vsb, Dm__arg, 1);

			XtSetArg(Dm__arg[0], XtNstring,
				hwp->hfp->toc->cooked_data);
			XtSetValues(hwp->htext, Dm__arg, 1);

			DmPushHelpStack(hwp, file, sect_name, sect_tag);
			if (file)
				FREE(file);
			if (sect_name)
				FREE(sect_name);
			if (sect_tag)
				FREE(sect_tag);
			hwp->hsp = hwp->hfp->toc;

			/* deactivate the "irrelevant" buttons */
			DmChgHelpWinBtnState(hwp, False, False);

		} else {
			DmDisplayHelpTOC(w, hwp, NULL, hlp->file, hwp->app_id);
		}
		/* Activate Backtrack if section stack is not empty */
		if (hwp->stack != NULL && hwp->sp != -1)
			OlVaFlatSetValues(hwp->menubar, 2, XtNsensitive, True, NULL);

	} else {
		DmHelpAppPtr	hap;

		hap = DmGetHelpApp(hwp->app_id);
		/*
		 * Use hlp->sect_name if set; otherwise, use hlp->sect_tag
		 * since the former is required and the latter isn't.
		 */ 
		DmDisplayHelpSection(hwp, hwp->app_id, NULL, hlp->file,
			hlp->sect_tag ? hlp->sect_tag : hlp->sect_name,
			UNSPECIFIED_POS, UNSPECIFIED_POS);
	}
	pop_in_progress = 0;

	if (hlp->file)
		free(hlp->file);

	if (hlp->sect_tag)
		free(hlp->sect_tag);

	if (hlp->sect_name)
		free(hlp->sect_name);

	/* if stack is now empty, deactivate Backtrack button */
	if (hwp->sp == -1) {
		OlVaFlatSetValues(hwp->menubar, 2, XtNsensitive, False, NULL);
	} else {
		OlVaFlatSetValues(hwp->menubar, 2, XtNsensitive, True, NULL);
	}

} /* end of DmPopHelpStack */

/****************************procedure*header*****************************
 * Close all application's help windows.
 */
void
DmCloseAllHelpWindows()
{
	DmHelpAppPtr hap;

	for (hap = DESKTOP_HELP_INFO(Desktop); hap; hap = hap->next) {
		if (hap->hlp_win.shell != NULL)
			DmCloseHelpWindow(&(hap->hlp_win));
	}
} /* end of DmCloseAllHelpWindows */

/****************************procedure*header*****************************
 * DmCloseHelpWindow
 */
void
DmCloseHelpWindow(hwp)
DmHelpWinPtr hwp;
{
	if (hwp == NULL || hwp->shell == NULL)
		return;

	if (hwp->hfp)
		DmCloseHelpFile(hwp->hfp);

	if (hwp->def_shell) {
		XtDestroyWidget(hwp->def_shell);
          hwp->def_shell = NULL;
	}

	if (hwp->gloss_shell) {
		XtDestroyWidget(hwp->gloss_shell);
          hwp->gloss_shell = NULL;
	}

	if (hwp->notes_shell) {
		XtDestroyWidget(hwp->notes_shell);
          hwp->notes_shell = NULL;
	}

	if (hwp->search_shell) {
		XtDestroyWidget(hwp->search_shell);
          hwp->search_shell = NULL;
	}

	if (hwp->bmark_shell) {
		XtDestroyWidget(hwp->bmark_shell);
          hwp->bmark_shell = NULL;
	}

	/* free section stack */
	if (hwp->stack) {
		DmHelpLocPtr hlp;

		for (hlp = hwp->stack; hwp->sp != -1;
			hlp = hwp->stack + (hwp->sp)--) {

			if (hlp->file)
				free(hlp->file);

			if (hlp->sect_tag)
				free(hlp->sect_tag);

			if (hlp->sect_name)
				free(hlp->sect_name);
		}
	}
	if (hwp->stack) {
		free(hwp->stack);
		hwp->stack = NULL;
		hwp->sp = -1;
	}

	/* set ptr to current section to NULL */
	if (hwp->hsp)
		hwp->hsp = NULL;

	/* set ptr to current help file to NULL */
	if (hwp->hfp)
		hwp->hfp = NULL;

	FreeGizmo(BaseWindowGizmoClass, hwp->gizmo_shell);
	XtDestroyWidget(hwp->shell);
	hwp->shell = NULL;

} /* end of DmCloseHelpWindow */

/****************************procedure*header*****************************
 * Called when Dismiss/Close is selected from window menu.  Calls
 * DmCloseWHelpWindow() to destroy help window.
 */
static void
HelpWinWMCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
     OlWMProtocolVerify *wm_data = (OlWMProtocolVerify *)call_data;
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;

     if (wm_data->msgtype == OL_WM_DELETE_WINDOW)
          DmCloseHelpWindow(hwp);

} /* end of HelpWinWMCB */


/****************************procedure*header*****************************
	Realizes the help window and sets its maximum and minimum width
	if the window isn't already mapped; otherwise, maps the help window
	shell.  Also sets viewHeight, viewWidth, etc. for hwp->swin.
 */
static void
PopupHelpWindow(DmHelpWinPtr hwp)
{
	int height;
	int width;
	int max_width;
	Dimension lm, rm;
	OlSWGeometries geom;
	XFontStruct *font;
	DmHelpAppPtr hap = DmGetHelpApp(hwp->app_id);

	XtSetArg(Dm__arg[0], XtNfont, &font);
	XtGetValues(hwp->htext, Dm__arg, 1);

	/* height of each line; includes space above and below it */
	height = font->max_bounds.ascent + font->max_bounds.descent + 2;

	if (hwp->hfp)
		width = hwp->hfp->width;
	else
		width = DEFAULT_HELPFILE_WIDTH;

	XtSetArg(Dm__arg[0], XtNrightMargin, &rm);
	XtSetArg(Dm__arg[1], XtNleftMargin, &lm);
	XtGetValues(hwp->htext, Dm__arg, 2);

	geom = GetOlSWGeometries((ScrolledWindowWidget)hwp->swin);

	/* Set granularity and stopPosition of vertical and horizontal
	 * scrollbars. Account for left and right margins and the width
	 * of the vertical scrollbar in viewHeight. 
	 */
	XtSetArg(Dm__arg[0], XtNvStepSize, height);
	XtSetArg(Dm__arg[1], XtNhStepSize, font->max_bounds.width);
	XtSetArg(Dm__arg[2], XtNstopPosition, OL_GRANULARITY);
	XtSetArg(Dm__arg[3], XtNviewHeight, DEFAULT_HWIN_HEIGHT * height);
	XtSetArg(Dm__arg[4], XtNviewWidth,
		lm + rm + geom.vsb_width + (width * font->max_bounds.width));
	XtSetValues(hwp->swin, Dm__arg, 5);

	XtRealizeWidget(hwp->shell);
	OlAddCallback(hwp->shell, XtNwmProtocol, HelpWinWMCB, hwp);
	XtAddEventHandler(hwp->shell, StructureNotifyMask, False, EventHandler,
		hwp);

	XtSetArg(Dm__arg[0], XtNwidth, &width);
	XtGetValues(hwp->shell, Dm__arg, 1);

	/* Set maxWidth and minWidth of shell so that the help window
	 * cannot be resized horizontally.
	 */
	if (hwp->magnifier) /* OL mode */
		max_width = width + (geom.vsb_width * 3);
	else /* Motif mode */
		max_width = width;

	/* disable resizing the help window horizontally. */
	XtSetArg(Dm__arg[0], XtNmaxWidth, max_width);
	XtSetArg(Dm__arg[1], XtNminWidth, max_width);
	XtSetArg(Dm__arg[2], XtNwidth,    max_width);
	XtSetValues(hwp->shell, Dm__arg, 3);

	/* finally, map the shell */
	XtMapWidget(hwp->shell);
	XRaiseWindow(XtDisplay(hwp->shell), XtWindow(hwp->shell));

} /* end of PopupHelpWindow */

/****************************procedure*header*****************************
 * Displays the table of contents of the current help file.
 */
int
DmDisplayHelpTOC(w, hwp, title, file_name, app_id)
Widget w;
DmHelpWinPtr hwp;
char *title;
char *file_name;
int  app_id;
{
	char buf[256];
	char *sect_name = NULL;
	char *sect_tag = NULL;
	char *file = NULL;

	DmHelpSectPtr hsp;
	DmHelpAppPtr  hap = DmGetHelpApp(hwp->app_id);
	Boolean       realized = True;

	if (hwp->shell == NULL) {
		realized = False;
		hwp->app_id = app_id;
		if (DmCreateHelpWindow(hwp, UNSPECIFIED_POS, UNSPECIFIED_POS,
			NULL, False))
			return(1);
	}

	/* Simply raise help window and return if TOC for file_name is
	 * being displayed.
	 */
	if (hwp->hfp && strcmp(hwp->hfp->name, file_name) == 0 &&
	    strcmp(hwp->hsp->name, TABLE_OF_CONTENTS) == 0) {
		XRaiseWindow(XtDisplay(hwp->shell), XtWindow(hwp->shell));
		return;
	}

	if (hwp->hsp && hwp->hfp) {
		if (hwp->hfp->name)
			file = strdup(hwp->hfp->name);

		if (hwp->hsp->name)
			sect_name = strdup(hwp->hsp->name);

		if (hwp->hsp->tag)
			sect_tag  = strdup(hwp->hsp->tag);
	}
	DmVaDisplayStatus((DmWinPtr)hwp, False, NULL);

	/* check whether file exists and is valid */
	if (file_name) {
		if (hwp->hfp == NULL ||
		   (hwp->hfp && strcmp(hwp->hfp->name, file_name))) {

			struct stat	hstat;
			DmHelpFilePtr	hfp;

			if ((hfp = (DmHelpFilePtr)
			     DmOpenHelpFile(hwp->app_id, file_name)) == NULL) {

				if (hwp->hfp && hwp->hsp) {
					DmPushHelpStack(hwp, file, sect_name, sect_tag);
					DmCloseHelpFile(hwp->hfp);
					hwp->hfp = NULL;
					hwp->hsp = NULL;
				}
				sprintf(buf, "%s %s", hap->title,Dm__gettxt(TXT_HELP_STR));
				DisplayHelpString(hwp, buf, Dm__gettxt(TXT_NO_HELP_AVAIL),
					realized);
				if (file)
					FREE(file);
				if (sect_name)
					FREE(sect_name);
				if (sect_tag)
					FREE(sect_tag);
				return(-1);
			}

			/* release all previous resources if new file */
			if (hwp->hfp)
				DmCloseHelpFile(hwp->hfp);
			hwp->hfp = hfp;
		}
	}

	if (file) {
		DmPushHelpStack(hwp, file, sect_name, sect_tag);
		if (file)
			FREE(file);
		if (sect_name)
			FREE(sect_name);
		if (sect_tag)
			FREE(sect_tag);
	}
	DmTOCCB(w, (XtPointer)hwp, NULL);
	if (realized)
		XRaiseWindow(XtDisplay(hwp->shell), XtWindow(hwp->shell));
	else
		PopupHelpWindow(hwp);

	/* If help is on a modal shell, add help window to the modal cascade */
	if (XtModalCascadeActive(hwp->shell))
		XtAddGrab(hwp->shell, False, False);

	return(0);

} /* end of DmDisplayHelpTOC */

/****************************procedure*header*****************************
 * Displays a string in a help window.
 */
static void
DisplayHelpString(DmHelpWinPtr hwp, char *title, char *string,
	Boolean realized)
{
	XtSetArg(Dm__arg[0], XtNstring, string);
	XtSetValues(hwp->htext, Dm__arg, 1);

	XtSetArg(Dm__arg[0], XtNtitle, title);
	XtSetValues(hwp->shell, Dm__arg, 1);

	DeactivateMenuBar(hwp->menubar);

	if (hwp->stack != NULL && hwp->sp != -1)
		/* Make Backtrack sensitive  */
		OlVaFlatSetValues(hwp->menubar, 2, XtNsensitive, True, NULL);

	if (realized) {
		/* deactivate buttons in Bookmark and Notes windows */
		DmChgHelpWinBtnState(hwp, True, False);
		XRaiseWindow(XtDisplay(hwp->shell), XtWindow(hwp->shell));
	} else
		PopupHelpWindow(hwp);

} /* end of DisplayHelpString */

/****************************procedure*header*****************************
 * Event handler to track UnmapNotify events.  This is needed to destroy
 * a help window when a help window is closed using the keyboard.
 */
static void
EventHandler(Widget w, XtPointer client_data, XEvent *xevent,
	Boolean *continue_to_use)
{
	if (xevent->type == UnmapNotify) {
		DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;

		if (hwp->shell)
          	DmCloseHelpWindow(hwp);
	}
} /* end of EventHandler */

/****************************procedure*header*****************************
 * Insensitizes every item in help window's menubar.
 */
static void
DeactivateMenuBar(menubar)
Widget menubar;
{
	XtSetArg(Dm__arg[0], XtNsensitive, False);
	OlFlatSetValues(menubar, 0, Dm__arg, 1);
	OlFlatSetValues(menubar, 1, Dm__arg, 1);
	OlFlatSetValues(menubar, 2, Dm__arg, 1);
	OlFlatSetValues(menubar, 3, Dm__arg, 1);
	OlFlatSetValues(menubar, 4, Dm__arg, 1);
	OlFlatSetValues(menubar, 5, Dm__arg, 1);
	OlFlatSetValues(menubar, 6, Dm__arg, 1);

} /* end of DeactivateMenuBar */
