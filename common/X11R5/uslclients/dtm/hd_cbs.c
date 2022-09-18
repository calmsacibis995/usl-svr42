/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:hd_cbs.c	1.42"

/******************************file*header********************************

    Description:
     This file contains the source code to display a description of
	and help on an application in the Help Desk.
*/
                              /* #includes go here     */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLook.h>
#include <Xol/ScrolledWi.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"
#include "HyperText.h"

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

static char *dflt_help_icon = "exec.icon";

/***************************private*procedures****************************

    Private Procedures
*/
static void GetAppInfo(DmObjectPtr op, char **help_file, char **help_dir,
char **icon_file, char **icon_label, Pixmap *icon_pixmap);

/***************************public*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
 * Displays a one-line description of an application in the footer when
 * an icon is SELECTed.
 */
void
DmHDSelectProc(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	OlFIconBoxButtonCD *d = (OlFIconBoxButtonCD *)call_data;
	char               *descrp;
	Boolean            set;

	DmVaDisplayStatus((DmWinPtr)DESKTOP_HELP_DESK(Desktop), False, NULL);

	XtSetArg(Dm__arg[0], XtNset, &set);
	OlFlatGetValues(w, d->item_data.item_index, Dm__arg, 1);
 
	if (set)
		return;

	descrp = DmGetObjProperty(OBJECT_CD(d->item_data), DESCRP, NULL);

	if (descrp)
		/* can't use DmVaDisplayStatus with actual string */
		SetBaseWindowMessage(
			(DmWinPtr)DESKTOP_HELP_DESK(Desktop)->gizmo_shell, descrp);
	else
		DmVaDisplayStatus((DmWinPtr)DESKTOP_HELP_DESK(Desktop), True,
			 TXT_NO_DESCRIP);

}				/* end of DmHDSelectProc */

/****************************procedure*header*****************************
 * Displays help on an application in a help window when its icon is
 * double-clicked on.
 */
void
DmHDDblSelectProc(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	DmHelpAppPtr hap;
	Pixmap icon_pixmap;
	char *help_file  = NULL;
	char *help_dir   = NULL;
	char *icon_file  = NULL;
	char *icon_label = NULL;

	OlFIconBoxButtonCD *d = (OlFIconBoxButtonCD *)call_data;

	DmVaDisplayStatus((DmWinPtr)DESKTOP_HELP_DESK(Desktop), False, NULL);

	GetAppInfo(OBJECT_CD(d->item_data), &help_file, &help_dir, &icon_file,
		&icon_label, &icon_pixmap);

	/* Get hap of selected application and use it to get a help window */
	if (hap = DmNewHelpAppID(DESKTOP_SCREEN(Desktop), NULL,
			OBJECT_CD(d->item_data)->name, icon_label, NULL, help_dir,
			icon_file)) {

		DmDisplayHelpSection(&(hap->hlp_win), hap->app_id,
			NULL, help_file, NULL, UNSPECIFIED_POS, UNSPECIFIED_POS);
	}

} /* end of DmHDDblSelectProc */

/****************************procedure*header*****************************
 * Callback for Open button in File menu to display help for selected
 * applications.
 */
void
DmHDOpenCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	int  i;
	char *help_file;
	char *help_dir;
	char *icon_file;
	char *icon_label;
	DmHelpAppPtr hap;
	DmItemPtr itp;
	Pixmap icon_pixmap;
	Boolean found = False;

	DmClearStatus((DmWinPtr)DESKTOP_HELP_DESK(Desktop));

	for (i = 0, itp = DESKTOP_HELP_DESK(Desktop)->itp;
		i < DESKTOP_HELP_DESK(Desktop)->nitems; i++, itp++) {

		if (ITEM_MANAGED(itp) && ITEM_SELECT(itp)) {
			found = True;
			help_file = help_dir = icon_file = icon_label = NULL;
			GetAppInfo(ITEM_OBJ(itp), &help_file, &help_dir,
				&icon_file, &icon_label, &icon_pixmap);

			/* Get hap of selected application and use it to get a
			 * help window.
			 */
			if (hap = DmNewHelpAppID(DESKTOP_SCREEN(Desktop), NULL,
				ITEM_OBJ(itp)->name, icon_label, NULL, help_dir,
				icon_file)) {

			DmDisplayHelpSection(&(hap->hlp_win), hap->app_id,
				NULL, help_file, NULL, UNSPECIFIED_POS, UNSPECIFIED_POS);
			}
		}
	}

	if (!found)
		DmVaDisplayStatus((DmWinPtr)DESKTOP_HELP_DESK(Desktop), True,
			TXT_HD_NONE_SELECTED);

} /* end of DmHDOpenCB */

/****************************procedure*header*****************************
 * Callback for Open in icon menu to display help for selected application.
 */
void
DmHDIMOpenCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	DmHelpAppPtr hap;
	Pixmap       icon_pixmap;
	DmItemPtr    itp = (DmItemPtr)client_data;
	char *help_file  = NULL;
	char *help_dir   = NULL;
	char *icon_file  = NULL;
	char *icon_label = NULL;

	DmVaDisplayStatus((DmWinPtr)DESKTOP_HELP_DESK(Desktop), False, NULL);

	/* display application's help */
	GetAppInfo(ITEM_OBJ(itp), &help_file, &help_dir, &icon_file,
		&icon_label, &icon_pixmap);

	/* Get hap of selected application and use it to get a help window */
	if (hap = DmNewHelpAppID(DESKTOP_SCREEN(Desktop), NULL,
			ITEM_OBJ(itp)->name, icon_label, NULL, help_dir, icon_file)) {

		DmDisplayHelpSection(&(hap->hlp_win), hap->app_id,
			icon_label ? icon_label : ITEM_OBJ(itp)->name,
			help_file, NULL, UNSPECIFIED_POS, UNSPECIFIED_POS);
	}

} /* end of DmHDIMOpenCB */

/****************************procedure*header*****************************
 * Callback for Align button in View menu to sort icons by name.
 */
void
DmHDAlignCB(w, client_data, call_data)
Widget	w;
XtPointer	client_data;
XtPointer	call_data;
{
	OlSWGeometries swin_geom;
	Dimension wrap_width;

	/* sort items by name */
	swin_geom = GetOlSWGeometries((ScrolledWindowWidget)
				(DESKTOP_HELP_DESK(Desktop)->swin));
	wrap_width = swin_geom.sw_view_width - swin_geom.vsb_width;
	DmSortItems(DESKTOP_HELP_DESK(Desktop), 0, 0, 0, wrap_width);

} /* end of DmHDAlignCB */

/****************************procedure*header*****************************
 * Callback for Help Desk button in Help menu.
 */
void
DmHDHelpCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{

	DmVaDisplayStatus((DmWinPtr)DESKTOP_HELP_DESK(Desktop), False, NULL);

	if (hddp->hap->help_dir)
		free(hddp->hap->help_dir);

	hddp->hap->help_dir = NULL;
	DmDisplayHelpSection(&(hddp->hap->hlp_win), hddp->hap->app_id,
		NULL, "DesktopMgr/helpdesk.hlp", NULL, UNSPECIFIED_POS,
		UNSPECIFIED_POS);

} /* end of DmHDHelpCB */

/****************************procedure*header*****************************
 * Callback for Table of Contents button in Help menu.
 */
void
DmHDHelpTOCCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	DmVaDisplayStatus((DmWinPtr)DESKTOP_HELP_DESK(Desktop), False, NULL);

	if (hddp->hap->help_dir)
		free(hddp->hap->help_dir);

	hddp->hap->help_dir = NULL;
	DmDisplayHelpTOC(w, &(hddp->hap->hlp_win), NULL,
		"DesktopMgr/helpdesk.hlp", hddp->hap->app_id);
		
} /* end of DmHDHelpTOCCB */

static void
GetAppInfo(op, help_file, help_dir, icon_file, icon_label, icon_pixmap)
DmObjectPtr op;
char **help_file;
char **help_dir;
char **icon_file;
char **icon_label;
Pixmap *icon_pixmap;
{
	DmGlyphPtr   gp = NULL;

	*help_file  = DmGetObjProperty(op, HELP_FILE, NULL);
	*help_dir   = DmGetObjProperty(op, HELP_DIR, NULL);
	*icon_file  = DmGetObjProperty(op, ICONFILE, NULL);
	*icon_label = DmGetObjProperty(op, ICON_LABEL, NULL);

	if (hddp->hap->help_dir) {
		free(hddp->hap->help_dir);
		hddp->hap->help_dir = NULL;
	}

	if (*help_dir)
		hddp->hap->help_dir = strdup(*help_dir);

	if (*icon_file == NULL)
		*icon_file = dflt_help_icon;

	gp = DmGetPixmap(DESKTOP_SCREEN(Desktop), *icon_file);
	if (gp != NULL)
		*icon_pixmap = gp->pix;
	else
		*icon_pixmap = hddp->fcp->glyph->pix;
} /* end of GetAppInfo */
