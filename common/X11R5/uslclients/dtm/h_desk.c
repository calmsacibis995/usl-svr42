/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:h_desk.c	1.73"

/******************************file*header********************************

    Description:
     This file contains the source code to initialize the Help Desk.
*/
                              /* #includes go here     */

#include <stdio.h>
#include <errno.h>
#include <locale.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include <X11/Shell.h>

#include <Xol/OpenLook.h>
#include <Xol/ScrolledWi.h>

#include <Dt/Desktop.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

#include <Gizmo/Gizmos.h>
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/BaseWGizmo.h>

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
          1. Public  Procedures
          2. Private Procedures
*/
                         /* private procedures         */

static void	UpdateHDObjs();
static void	SetUpHDIcons(Boolean new_locale);
static void	CreateHDFileClass();
static void	HDWMCB(Widget, XtPointer, XtPointer);
static void	HDExitCB(Widget, XtPointer, XtPointer);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

/* to be used to get value of _locale line in Help Desk's .dtinfo file. */
#define LOCALE         "locale"

#define XA	XtArgVal
#define B_A	(XtPointer)DM_B_ANY
#define B_O	(XtPointer)DM_B_ONE
#define B_M	(XtPointer)DM_B_ONE_OR_MORE

static MenuItems HDFileMenuItems[] = {
 { True, TXT_FILE_OPEN, TXT_M_FILE_OPEN,    NULL, DmHDOpenCB, B_M },
 { True, TXT_FILE_EXIT, TXT_M_FILE_EXIT,    NULL, HDExitCB, B_A },
 { NULL }
};

MENU("hdfilemenu", HDFileMenu);

static MenuItems HDViewMenuItems[] = {
 { True, TXT_VIEW_ALIGN, TXT_M_VIEW_ALIGN, NULL, DmHDAlignCB, B_A },
 { NULL }
};

MENU("hdviewmenu", HDViewMenu);

static MenuItems HDHelpMenuItems[] = {
 { True, TXT_HELP_HELPDESK,  TXT_M_HELP_HELPDESK,    NULL, DmHDHelpCB,    B_A },
 { True, TXT_HELP_TOC,       TXT_M_HELP_TOC,         NULL, DmHDHelpTOCCB, B_A },
 { NULL }
};

MENU("hdhelpmenu", HDHelpMenu);

MenuItems HDMenuBarItems[] = {
 { True, TXT_FILE,      TXT_M_FILE,      (void *)&HDFileMenu, DmMenuSelectCB },
 { True, TXT_VIEW,      TXT_M_VIEW,      (void *)&HDViewMenu, DmMenuSelectCB },
 { True, TXT_HELP,      TXT_M_HELP,      (void *)&HDHelpMenu, DmMenuSelectCB },
 { NULL }
};

MENUBAR("hdmenubar", HDMenuBar);

static HelpInfo HDWinHelp =
{ TXT_HELPDESK_TITLE, NULL, "DesktopMgr/helpdesk.hlp", NULL, NULL };

static BaseWindowGizmo HelpWindow = {
	&HDWinHelp,		/* help */
	"helpdesk",		/* shell widget name */
	TXT_HELPDESK_TITLE,	/* title */
	&HDMenuBar,		/* menu bar */
	NULL,			/* gizmo array */
	0,				/* # of gizmos in array */
	TXT_HELPDESK_TITLE, /* icon_name */
	"hdesk48.icon",     /* name of pixmap file */
	" ",				/* error message */
	" ",				/* status message */
	75				/* size of error message in percent of footer */
};
#undef XA

/***************************public*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
   DmInitHelpDesk - Initializes the Help Desk. Creates the Help Desk window,
	and reads in the list of applications/objects to be installed in it
	from $XWINHOME/desktop/Help_Desk which is the Help Desk's .dtinfo file.
 */

unsigned int
DmInitHelpDesk(char *geom_str, Boolean iconic, Boolean map_window)
{
	int cnt = 0;
	char *hdpath;
	char *p;
	char *q;
	Boolean new_locale = False;
	struct stat hstat;
	DmFolderWindow hd_win;

	hd_win = (DmFolderWinPtr)CALLOC(1, sizeof(DmFolderWinRec));

	hd_win->title        = strdup(GetGizmoText(TXT_HELPDESK_TITLE));
	hd_win->view_type    = DM_ICONIC;
	hd_win->attrs        = DM_B_HELPDESK_WIN;
	hd_win->finderWindow = NULL;

	/* allocate structure to store help desk info if this hasn't
	 * already been done in DmInitDesktop().
	 */
	
	if (hddp == NULL) {
		if ((hddp = (DmHDDataPtr)CALLOC(1, sizeof(DmHDDataRec))) == NULL) {
			Dm__VaPrintMsg(TXT_MEM_ERR);
			return(1);
		}
	}

	/* Create application shell	*/
	XtSetArg(Dm__arg[0], XtNiconic, iconic);
	XtSetArg(Dm__arg[1], XtNgeometry, geom_str);
	XtSetArg(Dm__arg[2], XtNmappedWhenManaged, map_window);

	hd_win->gizmo_shell = &HelpWindow;
	HDMenuBar.client_data = (XtPointer)hd_win;
	hd_win->shell = CreateGizmo(NULL, BaseWindowGizmoClass, &HelpWindow,
				    Dm__arg, 3);
	hd_win->swin = GetBaseWindowScroller(&HelpWindow);

	if (geom_str == NULL) {
    		Dimension view_width;
		Dimension margin = Ol_PointToPixel(OL_HORIZONTAL,ICON_MARGIN)*2;
		OlSWGeometries swin_geom =
	    		GetOlSWGeometries((ScrolledWindowWidget)(hd_win->swin));

    		/* Compute the view width and set the scrolled window to it */
    		view_width = GRID_WIDTH(Desktop) * FOLDER_COLS(Desktop) +
	    		     swin_geom.vsb_width + margin;

		XtSetArg(Dm__arg[0], XtNviewWidth, view_width);
		XtSetArg(Dm__arg[1], XtNviewHeight,
		 	GRID_HEIGHT(Desktop)*FOLDER_ROWS(Desktop) + margin);
    		XtSetValues(hd_win->swin, Dm__arg, 2);
    	}

	hdpath = (char *)(DmGetDTProperty(HDPATH, NULL));

	/* check if hdpath exists. */
	if ((stat(hdpath, &hstat)) != 0) {
		if (errno == ENOENT) {
			Dm__VaPrintMsg(TXT_NO_HELPDESK_FILE);
			return(1);
		}
	}
	
	CreateHDFileClass();

	/* allocate a container struct */
	if ((hd_win->cp = (DmContainerPtr)CALLOC(1, sizeof(DmContainerRec)))
		== NULL) {

		Dm__VaPrintMsg(TXT_MEM_ERR);
		return(1);
	}
	hd_win->cp->path = strdup(hdpath);
	hd_win->cp->count = 1;

	DmReadDtInfo(hd_win->cp, hdpath, 0);
	DESKTOP_HELP_DESK(Desktop) = hd_win;

	if (hd_win->cp->num_objs > 0)
		UpdateHDObjs();

	hd_win->nitems = hd_win->cp->num_objs + 5;

	/* Create icon container	*/
	cnt = 0;
	XtSetArg(Dm__arg[cnt], XtNmovableIcons,  False); cnt++;
	XtSetArg(Dm__arg[cnt], XtNdrawProc,      DmDrawLinkIcon); cnt++;
	XtSetArg(Dm__arg[cnt], XtNmenuProc,      DmIconMenuProc); cnt++;
	XtSetArg(Dm__arg[cnt], XtNselectProc,    DmHDSelectProc); cnt++;
	XtSetArg(Dm__arg[cnt], XtNdblSelectProc, DmHDDblSelectProc); cnt++;
	XtSetArg(Dm__arg[cnt], XtNadjustProc,    DmHDSelectProc); cnt++;
	XtSetArg(Dm__arg[cnt], XtNclientData,    hd_win); cnt++;
	XtSetArg(Dm__arg[cnt], XtNfont,          DESKTOP_FONT(Desktop)); cnt++;

	hd_win->box = DmCreateIconContainer(hd_win->swin,
				DM_B_CALC_SIZE | DM_B_SPECIAL_NAME |
				DM_B_FLUSH_DATA,
				Dm__arg, cnt, hd_win->cp->op,
				hd_win->cp->num_objs,
				&(hd_win->itp), hd_win->nitems,
				NULL, DESKTOP_FONTLIST(Desktop),
				DESKTOP_FONT(Desktop),
				DM_FontHeight(Desktop));

	/* Determine if locale is changed since the last time the
	 * Help Desk database was created.
	 */
	p = DtGetProperty(&(hd_win->cp->plist), LOCALE, NULL);
	q = setlocale(LC_MESSAGES, NULL);

	if (p && q)
		if (strcmp(p, q))
			new_locale = True;
	else
		new_locale = True;

	if (new_locale)
		DtSetProperty(&(hd_win->cp->plist), LOCALE, q, NULL);

	SetUpHDIcons(new_locale);


	XtRealizeWidget(hd_win->shell);

	DmHDAlignCB(NULL, NULL, NULL);

	OlAddCallback(hd_win->shell, XtNwmProtocol, HDWMCB, NULL);

	/* register for help */
	hddp->hap = (DmHelpAppPtr)DmNewHelpAppID(XtScreen(hd_win->shell),
				XtWindow(hd_win->shell), Dm__gettxt(TXT_DESKTOP_MGR),
				Dm__gettxt(TXT_HELP_DESK),
				DESKTOP_NODE_NAME(Desktop), NULL, "hdesk32.icon"); 

	/* get key color from XtNhelpKeyColor resource */
	hddp->key_color = DESKTOP_OPTIONS(Desktop).help_key_color;

	XtSetArg(Dm__arg[0], XtNvStepSize, GRID_HEIGHT(Desktop));
	XtSetArg(Dm__arg[1], XtNhStepSize, GRID_WIDTH(Desktop));
	XtSetValues(hd_win->swin, Dm__arg, 2);

	return(0);
} /* end of DmInitHelpDesk */

/****************************procedure*header*****************************
    HDExit - Called when a session terminates.
*/
void
DmHDExit()
{
	if (DESKTOP_HELP_DESK(Desktop) == NULL)
		return;

	DESKTOP_HELP_DESK(Desktop)->cp->count = 1;
	DmCloseWindow((DmWinPtr)DESKTOP_HELP_DESK(Desktop));
	DESKTOP_HELP_DESK(Desktop) = NULL;
} /* end of DmHDExit */

/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
   CreateHDFileClass - This is only called once when Help Desk is initialized
   to create a new file class for icons in the Help Desk.
 */   
static void
CreateHDFileClass()
{
	char	*dflt_icon = "exec.icon";

	if ((hddp->fcp = (DmFclassPtr)CALLOC(1, sizeof(DmFclassRec)))
	     == NULL) {
		Dm__VaPrintMsg(TXT_MEM_ERR);
		return;
	}

	hddp->fcp->glyph = DmGetPixmap(DESKTOP_SCREEN(Desktop), dflt_icon);
	DtSetProperty(&(hddp->fcp->plist), ICONFILE, dflt_icon, NULL);
	DtSetProperty(&(hddp->fcp->plist), DFLTICONFILE, dflt_icon, NULL);

} /* end of CreateHDFileClass */

/****************************procedure*header*****************************
    HDExitCB - Callback for Exit button in File menu.
*/

static void
HDExitCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	DmUnmapWindow((DmWinPtr)DESKTOP_HELP_DESK(Desktop));
} /* end of HDExitCB */

/****************************procedure*header*****************************
    HDExitCB - Called when Quit is selected from Help Desk's window menu.
*/
static void
HDWMCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	OlWMProtocolVerify	*wm_data = (OlWMProtocolVerify *)call_data;

	if (wm_data->msgtype == OL_WM_DELETE_WINDOW) {
		DmBringDownIconMenu((DmWinPtr)DESKTOP_HELP_DESK(Desktop));
		DmUnmapWindow((DmWinPtr)DESKTOP_HELP_DESK(Desktop));
	}
} /* end of HDWMCB */


/****************************procedure*header*****************************
    UpdateHDObjs - Sets fcp and ftype of Help Desk objects.
*/
static void
UpdateHDObjs()
{
	DmObjectPtr op;
	
	for (op = DESKTOP_HELP_DESK(Desktop)->cp->op; op; op = op->next) {
		op->fcp   = hddp->fcp;
		op->ftype = DM_FTYPE_DATA;

	}
} /* end of UpdateHDObjs */

/****************************procedure*header*****************************
    SetUpHDIcons - If switching locale, retrieve icon label and one-line
    description from help file; otherwise, use existing information in
    /usr/X/desktop/Help_Desk.  If the ICON_LABEL property is not set,
    use the value of DFLT_ICONLABEL.  If DFLT_ICONLABEL is not set,
    use the application name as icon label.
*/
static void
SetUpHDIcons(Boolean new_locale)
{
	int i;
	char *prop;
	char *descrp;
	char *label;
	char *dflt_label;
	char *fullpath;
	DmItemPtr itp;
	Boolean touched = False;
	
	for (i = 0, itp = DESKTOP_HELP_DESK(Desktop)->itp;
	     i < DESKTOP_HELP_DESK(Desktop)->nitems; i++, itp++) {

		if (ITEM_MANAGED(itp)) {
			/* Get default icon label, if set, to be used if no icon
			 * label is found in help file.
			 */
			dflt_label = DmGetObjProperty(ITEM_OBJ(itp), DFLT_ICONLABEL,
						NULL);
			/* Read icon label and one-line description from its
			 * help file.
			 */
			if (new_locale ||
			    DmGetObjProperty(ITEM_OBJ(itp), DESCRP, NULL) == NULL ||
			    DmGetObjProperty(ITEM_OBJ(itp), ICON_LABEL, NULL) == NULL)
			{
				prop = DmGetObjProperty(ITEM_OBJ(itp), HELP_FILE, NULL);

				if (prop) {
					fullpath = XtResolvePathname(DESKTOP_DISPLAY(Desktop),
							"help", prop, NULL, NULL, NULL,
							0, NULL);
					if (fullpath)
						DmGetHDAppInfo(fullpath, &label, &descrp);
					else
						descrp = label = NULL;
				}
				if (descrp)
					DmSetObjProperty(ITEM_OBJ(itp), DESCRP, descrp, NULL);

				if (label) {
					DmSetObjProperty(ITEM_OBJ(itp), ICON_LABEL, label,
						NULL);
					itp->label = (XtArgVal)label;
				} else if (dflt_label)
					itp->label = (XtArgVal)strdup(dflt_label);
				else
					itp->label = (XtArgVal)strdup(ITEM_OBJ(itp)->name);
				touched = True;
			} else {
				label = DmGetObjProperty(ITEM_OBJ(itp), ICON_LABEL, NULL);
				if (label)
					itp->label = (XtArgVal)strdup(label);
				else if (dflt_label)
					itp->label = (XtArgVal)strdup(dflt_label);
				else
					itp->label = (XtArgVal)strdup(ITEM_OBJ(itp)->name);
			}
			prop = DmGetObjProperty(ITEM_OBJ(itp), "_LINK", NULL);
			if (prop)
				ITEM_OBJ(itp)->attrs = DM_B_SYMLINK;

			DmSizeIcon(itp, DESKTOP_FONTLIST(Desktop),
				   DESKTOP_FONT(Desktop));
		}
	}
	XtSetArg(Dm__arg[0], XtNitemsTouched, True);
	XtSetValues(DESKTOP_HELP_DESK(Desktop)->box, Dm__arg, 1);

	/* Flush Help Desk database to disk, if any changes were made,
	 * to be reused if locale is not changed in the next session.
	 * If locale is changed in the same session or in the next session,
	 * this database will have to be recreated.
	 */ 
	if (touched)
		DmWriteDtInfo(DESKTOP_HELP_DESK(Desktop)->cp,
			DESKTOP_HELP_DESK(Desktop)->cp->path, 0);

} /* end of SetUpHDIcons */
