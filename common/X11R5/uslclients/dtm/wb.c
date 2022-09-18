/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:wb.c	1.129"

/******************************file*header********************************

    Description:
     This file contains the source code for initializing the wastebasket,
	and functions to relabel the icons in it by version and setting up
	a wastebasket file class.
*/
                              /* #includes go here     */

#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include <X11/Shell.h>

#include <Xol/OpenLook.h>
#include <Xol/Stub.h>

#include <Dt/Desktop.h>

#include <Gizmo/Gizmos.h>
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/BaseWGizmo.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"
#include "wb.h"
#include "SWinGizmo.h"
#include "StatGizmo.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
          1. Private Procedures
          2. Public  Procedures
*/
                         /* private procedures         */

static void	InitWBData(Boolean iconic);
static void	WBWMCB(Widget, XtPointer, XtPointer);
static void	ExitWBCB(Widget, XtPointer, XtPointer);
static void	WBEventHandler(Widget, XtPointer, XEvent *, Boolean *);
static void	CreateWBFileClass();
static void	UpdateWBObjs();

/* Define the menus and submenus for the wastebasket menu bar.  */

#define XA XtArgVal
#define B_A	(XtPointer)DM_B_ANY
#define B_O	(XtPointer)DM_B_ONE
#define B_M	(XtPointer)DM_B_ONE_OR_MORE

static MenuItems WbActionMenuItems[] = {
 { True, TXT_WB_TIMER_ON,  TXT_M_WB_TIMER_ON,  NULL, DmWBTimerCB,      B_A },
 { True, TXT_WB_SET_PROP,  TXT_M_WB_SET_PROP,  NULL, DmWBPropCB,       B_A },
 { True, TXT_WB_EMPTY,     TXT_M_WB_EMPTY,     NULL, DmConfirmEmptyCB, B_A },
 { True, TXT_FILE_EXIT,    TXT_M_FILE_EXIT,    NULL, ExitWBCB,         B_A },
 { NULL }
};

static MenuItems ViewSortMenuItems[] = {
 { True, TXT_SORT_TYPE, TXT_M_SORT_TYPE, NULL, DmViewSortCB },
 { True, TXT_SORT_NAME, TXT_M_SORT_NAME, NULL, DmViewSortCB },
 { True, TXT_SORT_SIZE, TXT_M_SORT_SIZE, NULL, DmViewSortCB },
 { True, TXT_SORT_TIME, TXT_M_SORT_TIME, NULL, DmViewSortCB },
 { NULL }
};

MENU("viewsortmenu", ViewSortMenu);

static MenuItems WbViewMenuItems[] = {
 { True, TXT_VIEW_ALIGN, TXT_M_VIEW_ALIGN, NULL, DmViewAlignCB },
 { True, TXT_VIEW_SORT,  TXT_M_VIEW_SORT,  (void *)&ViewSortMenu, NULL },
 { NULL }
};
 
static MenuItems WbEditMenuItems[] = {
 { True, TXT_EDIT_SELECT,   TXT_M_EDIT_SELECT,   NULL, DmEditSelectAllCB,  B_A},
 { True, TXT_EDIT_UNSELECT, TXT_M_EDIT_UNSELECT, NULL, DmEditUnselectAllCB,B_M},
 { True, TXT_WB_FILEPROP,   TXT_M_WB_FILEPROP,   NULL, DmWBEMFilePropCB,   B_M},
 { True, TXT_WB_PUTBACK,    TXT_M_WB_PUTBACK,    NULL, DmWBEMPutBackCB,    B_M},
 { True, TXT_WB_DELETE,     TXT_M_WB_DELETE,     NULL, DmWBEMDeleteCB,     B_M},
 { NULL }
};

static MenuItems WbHelpMenuItems[] = {
 { True,TXT_HELP_WB,       TXT_M_HELP_WB,       NULL,DmHelpSpecificCB, B_A },
 { True,TXT_HELP_TOC,      TXT_M_HELP_TOC,      NULL,DmHelpTOCCB,      B_A },
 { True,TXT_HELP_HELPDESK, TXT_M_HELP_HELPDESK, NULL,DmHelpDeskCB,     B_A },
 { NULL }
};

MENU("wbactionmenu", WbActionMenu);
MENU("wbviewmenu", WbViewMenu);
MENU("wbeditmenu", WbEditMenu);
MENU("wbhelpmenu", WbHelpMenu);

static MenuItems WbMenuBarItems[] = {
 { True, TXT_ACTION, TXT_M_ACTION,       (void *)&WbActionMenu, DmMenuSelectCB},
 { True, TXT_EDIT,      TXT_M_EDIT,      (void *)&WbEditMenu,  DmMenuSelectCB},
 { True, TXT_VIEW,      TXT_M_VIEW,      (void *)&WbViewMenu,  DmMenuSelectCB},
 { True, TXT_HELP,      TXT_M_HELP,      (void *)&WbHelpMenu,  DmMenuSelectCB},
 { NULL }
};

MENUBAR("wbmenubar", WbMenuBar);

static SWinGizmo swin_gizmo = {
	"swin",			/* name */
};

static StatusGizmo status_gizmo = {
	"status",			/* name */
	50				/* left percent */
};

static GizmoRec wb_gizmos[] = {
	{SWinGizmoClass,	&swin_gizmo},
	{StatusGizmoClass,	&status_gizmo},
};

static HelpInfo WBWinHelp =
{ TXT_WB_TITLE, NULL, "DesktopMgr/wb.hlp", NULL, NULL };

static BaseWindowGizmo WbWindow = {
	&WBWinHelp,           /* help */
	"wastebasket",        /* shell widget name */
	TXT_WB_TITLE,         /* title */
	&WbMenuBar,           /* menu bar */
	wb_gizmos,            /* gizmo array */
	XtNumber(wb_gizmos),  /* # of gizmos in array */
	"",                   /* icon_name */
	"",                   /* name of pixmap file */
	" ",                  /* error message */
	" ",                  /* status message */
	75                    /* size of error message in percent of footer */
};

#undef XA

DmWbDataPtr	wbdp;
char           *wb_dtinfo;
static char	*wbdir;

static unsigned long timer_unit[]  = { MINUNIT, HOURUNIT, DAYUNIT };

/***************************public*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
 * Initializes the wastebasket.
 */
unsigned int
DmInitWasteBasket(char *geom_str, Boolean iconic, Boolean map_window)
{
	DmFolderWindow	wb_win;
	struct stat	wbstat;
	char buf[PATH_MAX];

	if ((wbdp = (DmWbDataPtr)CALLOC(1, sizeof(DmWbDataRec))) == NULL) {
		Dm__VaPrintMsg(TXT_MEM_ERR);
		return(1);
	}
	InitWBData(iconic);

	/* Alloc a wastebasket window and store it in the Desktop struct */
	wb_win = (DmFolderWinPtr)CALLOC(1, sizeof(DmFolderWinRec));
	DESKTOP_WB_WIN(Desktop) = wb_win;

	wb_win->attrs        = DM_B_WASTEBASKET_WIN;
	wb_win->title        = strdup(GetGizmoText(TXT_WB_TITLE));
	wb_win->view_type    = DM_ICONIC;
	wb_win->sort_type    = DM_BY_TYPE;
	wb_win->next         = NULL;
	wb_win->finderWindow = NULL;

	DmGetWBPixmaps();

	/* Create application shell */
	XtSetArg(Dm__arg[0], XtNiconic, iconic);
        XtSetArg(Dm__arg[1], XtNwidth,
		 FOLDER_WINDOW_WIDTH(DESKTOP_SHELL(Desktop)));
        XtSetArg(Dm__arg[2], XtNheight,
		 FOLDER_WINDOW_HEIGHT(DESKTOP_SHELL(Desktop)));
	XtSetArg(Dm__arg[3], XtNgeometry, geom_str);
	XtSetArg(Dm__arg[4], XtNmappedWhenManaged, map_window);

	((MenuGizmo *)(WbWindow.menu))->client_data = (XtPointer)wb_win;
	WbViewMenu.default_item = 1;
	wb_win->shell = CreateGizmo(NULL, BaseWindowGizmoClass, &WbWindow,
				    Dm__arg, 5);
	wb_win->swin  = (Widget)QueryGizmo(BaseWindowGizmoClass, &WbWindow,
				 	GetGizmoWidget, "swin");
	wb_win->gizmo_shell = &WbWindow;

	DmCreateWBIconShell(wb_win->shell, iconic);

	wbdir = (char *)DmGetDTProperty(WBDIR, NULL);
	/* check if wbdir exists */
	if ((stat(wbdir, &wbstat)) != 0) {
		if (errno == ENOENT) {
			Dm__VaPrintMsg(TXT_WB_NOT_EXIST, wbdir);
			/* create wbdir */
			if (mkdir(wbdir, 0755) != 0) {
				Dm__VaPrintMsg(TXT_MKDIR, wbdir);
				return(1);
			}
		}
	}

	CreateWBFileClass();
	sprintf(buf, "%s/.Wastebasket", wbdir);
	wb_dtinfo = strdup(buf);
	wb_win->cp = Dm__NewContainer(wbdir);
	DmReadDtInfo(wb_win->cp, wb_dtinfo, 0);

	if (!WB_IS_EMPTY(Desktop))
		UpdateWBObjs();
	else /* make Empty button insensitive */
		WbActionMenuItems[2].sensitive = False;

	wb_win->nitems = wb_win->cp->num_objs + 5;

	/* Create icon container	*/
	XtSetArg(Dm__arg[0], XtNdropProc,       DmWBDropProc);
	XtSetArg(Dm__arg[1], XtNmovableIcons,   True);
	XtSetArg(Dm__arg[2], XtNdrawProc,       DmDrawLinkIcon);
	XtSetArg(Dm__arg[3], XtNmenuProc,       DmIconMenuProc);
	XtSetArg(Dm__arg[4], XtNclientData,     wb_win);  /* for MenuProc */
	XtSetArg(Dm__arg[5], XtNpostSelectProc, DmButtonSelectProc);
	XtSetArg(Dm__arg[6], XtNpostAdjustProc, DmButtonSelectProc);
	XtSetArg(Dm__arg[7], XtNfont,           DESKTOP_FONT(Desktop));

	wb_win->box = DmCreateIconContainer(wb_win->swin,
				DM_B_SPECIAL_NAME | DM_B_FLUSH_DATA,
				Dm__arg, 8, wb_win->cp->op,
				wb_win->cp->num_objs,
				&(wb_win->itp), wb_win->nitems,
				NULL, DESKTOP_FONTLIST(Desktop),
				DESKTOP_FONT(Desktop),
				DM_FontHeight(Desktop));

	DmLabelWBFiles();
	/* put something in the status area */
	DmDisplayStatus((DmWinPtr)wb_win);

	/* If wbdp->suspend is True, reset it to False if map_window is
	 * False (wastebasket window is withdrawn) or iconic is True, and
	 * always set wbdp->restart to False.
	 * If wbdp->suspend is False, set wbdp->restart to True if wastebasket
	 * window is in normal state.
	 */

	if (WB_BY_TIMER(wbdp)) {
		if (wbdp->suspend == True) {
			if (map_window == False || iconic == True)
				wbdp->suspend = False;
			wbdp->restart = False;
		} else {
			if (iconic == False && map_window == True)
				wbdp->restart = True;
			else
				wbdp->restart = False;
		}

		if (wbdp->suspend)
			DmWBRestartTimer();
		else
			DmWBTimerProc(NULL, NULL);

	} else {
		DmWBToggleTimerBtn(Dm__gettxt(TXT_SUSPEND_LBL),
			*Dm__gettxt(TXT_M_SUSPEND_LBL), False);
		DmVaDisplayState((DmWinPtr)DESKTOP_WB_WIN(Desktop), NULL);
	}

	XtRealizeWidget(wb_win->shell);
	OlAddCallback(wb_win->shell, XtNwmProtocol, WBWMCB, NULL);
	OlAddCallback(wbdp->icon_shell, XtNwmProtocol, WBWMCB, NULL);

	XtAddEventHandler(wb_win->shell, PropertyChangeMask, False,
			  WBEventHandler, NULL);

	/* register for help */
	WB_HELP_ID(Desktop) = DmNewHelpAppID(XtScreen(wb_win->shell),
               XtWindow(wb_win->shell), Dm__gettxt(TXT_DESKTOP_MGR),
			GetGizmoText(TXT_WB_TITLE), DESKTOP_NODE_NAME(Desktop),
			NULL, "wb32.icon")->app_id;

	XtSetArg(Dm__arg[0], XtNvStepSize, GRID_HEIGHT(Desktop));
	XtSetArg(Dm__arg[1], XtNhStepSize, GRID_WIDTH(Desktop));
	XtSetValues(wb_win->swin, Dm__arg, 1);

	return(0);

} /* end of DmInitWasteBasket */

/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
 * Called when the Exit is selected from the Wastebasket's File menu.
 */
static void
ExitWBCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	DmUnmapWindow((DmWinPtr)DESKTOP_WB_WIN(Desktop));
} /* end if ExitWBCB */

/****************************procedure*header*****************************
 * Called when the Quit/Close is selected from the Wastebasket window menu.
 */
static void
WBWMCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	OlWMProtocolVerify *wm_data = (OlWMProtocolVerify *)call_data;

	if (wm_data->msgtype == OL_WM_DELETE_WINDOW) {
		DmBringDownIconMenu((DmWinPtr)DESKTOP_WB_WIN(Desktop));
		DmUnmapWindow((DmWinPtr)DESKTOP_WB_WIN(Desktop));
	}
} /* end of WBWMCB */

/****************************procedure*header*****************************
 * Called from DtmExit when the session terminates.
 */
void
DmWBExit()
{
	char buf[1024];

	if (DESKTOP_WB_WIN(Desktop) == NULL)
		return;

	DmCloseWindow((DmWinPtr)DESKTOP_WB_WIN(Desktop));
	if (WB_ON_EXIT(wbdp) && wbdir) {
		sprintf(buf, "rm -rf %s/* %s", wbdir, wb_dtinfo);
		system(buf);
	}
	DESKTOP_WB_WIN(Desktop) = NULL;

} /* end of DmWBExit */

/****************************procedure*header*****************************
 * Event handler to track opening and closing the wastebasket window.
 * The state of timer is updated appropriately if wbdp->cleanUpMethod
 * is WBByTimer. 
 */
static void
WBEventHandler(Widget w, XtPointer client_data, XEvent *xevent,
Boolean *cont_to_dispatch)
{
	extern void MergeResource();

	if (!WB_BY_TIMER(wbdp))
		return;

	if ((xevent->type != PropertyNotify) ||
	    (xevent->xproperty.state != PropertyNewValue))
		return;

	if (xevent->xproperty.atom == XA_WM_STATE(xevent->xany.display)) {
		char buf[32];

		if ((GetWMState(XtDisplay(DESKTOP_WB_WIN(Desktop)->shell),
			XtWindow(DESKTOP_WB_WIN(Desktop)->shell)))
			== IconicState && wbdp->suspend) {

			DmWBResumeTimer();

		} else if ((GetWMState(XtDisplay(DESKTOP_WB_WIN(Desktop)->shell),
				XtWindow(DESKTOP_WB_WIN(Desktop)->shell)))
				== NormalState && !wbdp->suspend) {

			/* if Wastebasket is not iconized at the start of a session
			 * and the timer was not suspended at the end of the previous
			 * session, don't suspend the timer - but only if wastebasket
			 * is not empty.
			 */
			if (wbdp->restart && !WB_IS_EMPTY(Desktop)) {
				wbdp->restart = False;
				return;
			}

			DmWBSuspendTimer();
		}
		/* update wbSuspend resource in .Xdefaults file */
		sprintf(buf, "dtm.wbSuspend:%d\n", wbdp->suspend);
		MergeResources(buf);
	}

} /* end of WBEventHandler */

/****************************procedure*header*****************************
 * Create a file class to be used for files in the Wastebasket.
 */
static void
CreateWBFileClass()
{
	char	*dflt_icon = "datafile.icon";

	if ((wbdp->fcp = (DmFclassPtr)CALLOC(1, sizeof(DmFclassRec)))
	     == NULL) {
		Dm__VaPrintMsg(TXT_MEM_ERR);
		return;
	}

	wbdp->fcp->glyph = DmGetPixmap(DESKTOP_SCREEN(Desktop), dflt_icon);
	DtSetProperty(&(wbdp->fcp->plist), ICONFILE, dflt_icon, NULL);
	DtSetProperty(&(wbdp->fcp->plist), DFLTICONFILE, dflt_icon, NULL);

} /* end of CreateWBFileClass */

/****************************procedure*header*****************************
 * Update file class of files in Wastebasket.
 */
static void
UpdateWBObjs()
{
	DmObjectPtr op;
	struct stat buf;
	struct stat lbuf;
	char        *p;

	for (op = DESKTOP_WB_WIN(Desktop)->cp->op; op; op = op->next) {
		/* This is just to set ftype. */
		p = DmObjPath(op);
		(void)lstat(p, &lbuf);
		if (stat(p, &buf) == -1)
			Dm__SetDfltFileClass(op, NULL, &lbuf);
		else
			Dm__SetDfltFileClass(op, &buf, &lbuf);

		/* override it with WB's special fcp */
		op->fcp = wbdp->fcp;
	}
} /* end of UpdateWBObjs */

/****************************procedure*header*****************************
 * Initializes wbdp to resource values saved in .Xdefaults.
 */
static void
InitWBData(Boolean iconic)
{
	wbdp->cleanUpMethod = DESKTOP_OPTIONS(Desktop).wb_cleanUpMethod;
	wbdp->interval      = DESKTOP_OPTIONS(Desktop).wb_timer_interval;
	wbdp->unit_idx      = DESKTOP_OPTIONS(Desktop).wb_timer_unit;
	wbdp->suspend       = DESKTOP_OPTIONS(Desktop).wb_suspend;
	wbdp->time_unit     = timer_unit[wbdp->unit_idx];
	wbdp->timer_id      = NULL;
	wbdp->tm_start      = (time_t)0;
	wbdp->tm_remain     = (unsigned long)0;
	wbdp->tm_interval   = (unsigned long)0;

} /* end of InitWBData */
