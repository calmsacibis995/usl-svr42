/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:wb_util.c	1.65.1.11"

/******************************file*header********************************

    Description:
     This file contains the source code for switching the wastebasket's
	process icon pixmap, suspending, resuming and restarting the timer,
	creating the process icon, assigning version number to a deleted file,
	and labeling icons in the wastebasket.
*/
                              /* #includes go here     */

#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/Vendor.h>

#include <Xol/OpenLook.h>
#include <Xol/FButtons.h>

#include <Dt/Desktop.h>

#include <Gizmo/Gizmos.h>
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/BaseWGizmo.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"
#include "wb.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
          1. Private Procedures
          2. Public  Procedures
*/
                         /* private procedures         */

static Boolean	DmWBTriggerNotify(Widget w, Window win, Position x, Position y,
			Atom selection, Time time_stamp, OlDnDDropSiteID
			drop_site_id, OlDnDTriggerOperation dnd_op,
			Boolean send_done, Boolean forwarded, XtPointer closure);

/***************************public*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
 * Label files in wastebasket using their basename and assigned
 * version numbers. 
 */
void
DmLabelWBFiles()
{
	struct stat wbstat;
	DmItemPtr   itp;
	DmObjectPtr op;
	int         i;
	int         ver_num;
	char 	  buf[PATH_MAX];
	char        *real_path;
	char        *bname;

	/* If Wastebasket's .dtinfo file contains entries for files which don't
	 * exist, unmanage the item and remove the object from Wastebasket.
	 */
	for (op = DESKTOP_WB_WIN(Desktop)->cp->op; op; op = op->next) {
		sprintf(buf, "%s/%s", DESKTOP_WB_WIN(Desktop)->cp->path, op->name);
	  if (stat(buf, &wbstat) != 0)
		DmDelObjectFromContainer((DmContainerPtr)DESKTOP_WB_WIN(Desktop)->cp,
			op);
	  else
		DmSetObjProperty(op, VERSION, NULL, NULL);
	}

	for (i = 0, itp = DESKTOP_WB_WIN(Desktop)->itp;
	     i < DESKTOP_WB_WIN(Desktop)->nitems; i++, itp++) {

		if (itp->managed) {
			real_path = DmGetObjProperty(ITEM_OBJ(itp), REAL_PATH, NULL);
			bname = basename(real_path);
			ver_num = DmWBGetVersion(itp, bname);
			sprintf(buf, "%d", ver_num);
			DmSetObjProperty(ITEM_OBJ(itp), VERSION, buf, NULL);
			sprintf(buf, "%s:%d", bname, ver_num);
			itp->label = (XtArgVal)strdup(buf);
			DmSizeIcon(itp, DESKTOP_FONTLIST(Desktop),
				DESKTOP_FONT(Desktop));
		}
	}
	DmTouchIconBox((DmWinPtr)DESKTOP_WB_WIN(Desktop), NULL, 0);
	DmWriteDtInfo(DESKTOP_WB_WIN(Desktop)->cp, wb_dtinfo, 0);
	DmSwitchWBIcon();

} /* end of DmLabelFiles */

/****************************procedure*header*****************************
 * Returns the next version number to be used for a deleted file.
 * Returns 1 if there's no other file of the same name; otherwise,
 * returns the next higher integer.
 */
int
DmWBGetVersion(itp, fname)
DmItemPtr itp;
char      *fname;	/* basename of file */
{
#define MAX(A,B)	(((A) > (B)) ? (A) : (B))
	DmObjectPtr op;
	char        *real_path;
	char        *bname;
	int         count;
	int         vnum = 1;

	for (op = DESKTOP_WB_WIN(Desktop)->cp->op; op; op = op->next) {
		real_path = DmGetObjProperty(op, REAL_PATH, NULL);
		bname = basename(real_path);
		if (strcmp(bname, fname) == 0 &&
		    strcmp(ITEM_OBJ(itp)->name, op->name) != 0) {
			count = atoi(DmGetObjProperty(op, VERSION, NULL)) + 1;
			vnum = MAX(vnum, count);
		}
		
	}
	return(vnum);
#undef MAX
} /* end of DmGetVersion */

/****************************procedure*header*****************************
    DmWBCleanUpMethod- Returns Wastebasket's current clean up method.

*/
/* ARGSUSED */
int
DmWBCleanUpMethod(DmDesktopPtr desktop)
{
	return(wbdp->cleanUpMethod);
}

/****************************procedure*header*****************************
 * Returns folder window ptr to wastebasket if win is window id of
 * Wastebasket window; otherwise, returns NULL. 
 */
DmFolderWinPtr
DmIsWB(win)
Window	win;
{
    if ((XtWindow(DESKTOP_WB_WIN(Desktop)->box) == win) ||
	      (win == DESKTOP_WB_ICON(Desktop)))
	return ( DESKTOP_WB_WIN(Desktop) );

    return(NULL);

} /* end of DmIsWB */

/****************************procedure*header*****************************
 * Switches icon pixmap for wastebasket process icon when the wastebasket
 * becomes empty or non-empty.
 */
void
DmSwitchWBIcon()
{
	BaseWindowGizmo *base = DESKTOP_WB_WIN(Desktop)->gizmo_shell;
	MenuGizmo       *menu = base->menu;
	MenuItems       *items = menu->items;

	items = items[0].mod.nextTier->items;
	if (WB_IS_EMPTY(Desktop))
		items[2].sensitive = False;
	else
		items[2].sensitive = True;

	DtChangeProcessIcon(wbdp->icon_shell,
		(WB_IS_EMPTY(Desktop)) ? wbdp->egp->pix : wbdp->fgp->pix,
		(WB_IS_EMPTY(Desktop)) ? wbdp->egp->mask : wbdp->fgp->mask);

} /* end of DmSwitchWBIcon */

/****************************procedure*header*****************************
 * If wastebasket is empty, don't restart timer; otherwise, restart timer
 * with an adjusted timer interval determined from the lowest time stamp of
 * files in the wastebasket.
 */
void
DmWBRestartTimer()
{
	if (WB_IS_EMPTY(Desktop)) {

		wbdp->tm_interval =(unsigned long)(wbdp->interval * wbdp->time_unit);
		wbdp->tm_remain = wbdp->tm_interval;
		wbdp->suspend = True;

		if (wbdp->timer_id) {
			XtRemoveTimeOut(wbdp->timer_id);
			wbdp->timer_id = NULL;
		}

		DmVaDisplayState((DmWinPtr)DESKTOP_WB_WIN(Desktop), TXT_WB_SUSPEND);

		/* Set button label to Resume Timer */
		DmWBToggleTimerBtn(Dm__gettxt(TXT_RESUME_LBL),
			*Dm__gettxt(TXT_M_RESUME_LBL), False);
		return;

	} else {
		int i;
		time_t ts;
		time_t lowest_ts;
		time_t current_time;
		unsigned long intv1;
		unsigned long intv2;
		DmItemPtr itp;
		Boolean reset = False;

		lowest_ts = LONG_MAX;
		time(&current_time);

		for (i = 0, itp = DESKTOP_WB_WIN(Desktop)->itp;
		     i < DESKTOP_WB_WIN(Desktop)->nitems; i++, itp++) {

			if (ITEM_MANAGED(itp)) {
				ts = atol(DmGetObjProperty(ITEM_OBJ(itp), TIME_STAMP,
						NULL));

				/* If the time stamp of file is greater than the current
				 * time (this *is* possible), then reset its time stamp
				 * to the current_time + the current interval to avoid
				 * an infinite loop.
				 */
				if (ts > current_time) {
					char buf[16];

					ts = current_time;
					sprintf(buf, "%ld", ts);
					DmSetObjProperty(ITEM_OBJ(itp), TIME_STAMP,
						buf, NULL);
					reset = True;
				}

				if (ts < lowest_ts) {
					lowest_ts = ts;
				}
			}
		}
		/* update Wastebasket's .dtinfo file if any time stamp was reset */
		if (reset)
			DmWriteDtInfo(DESKTOP_WB_WIN(Desktop)->cp, wb_dtinfo, 0);

		/* compute timer interval and time elapsed in milliseconds */
		intv1 = (unsigned long)(wbdp->interval * wbdp->time_unit);
		intv2 = (unsigned long)((current_time - lowest_ts) * 1000);

		if (intv2 > intv1)
			wbdp->tm_interval = (unsigned long)0;
		else
			wbdp->tm_interval = (unsigned long)(intv1 - intv2);

		if (wbdp->suspend == False) {
			if (wbdp->timer_id != NULL)
				XtRemoveTimeOut(wbdp->timer_id);
			 
			wbdp->timer_id = XtAddTimeOut(wbdp->tm_interval,
							(XtTimerCallbackProc)DmWBTimerProc, NULL);
			time(&(wbdp->tm_start));

			DmVaDisplayState((DmWinPtr)DESKTOP_WB_WIN(Desktop),
				TXT_WB_RESUME);

			/* Set button label to Suspend Timer */
			DmWBToggleTimerBtn(Dm__gettxt(TXT_SUSPEND_LBL),
				*Dm__gettxt(TXT_M_SUSPEND_LBL), True);

		} else {
			wbdp->timer_id = NULL;
			wbdp->tm_remain = wbdp->tm_interval;

			DmVaDisplayState((DmWinPtr)DESKTOP_WB_WIN(Desktop),
				TXT_WB_SUSPEND);

			/* Set button label to Resume Timer */
			DmWBToggleTimerBtn(Dm__gettxt(TXT_RESUME_LBL),
				*Dm__gettxt(TXT_M_RESUME_LBL), True);
		}
	}
} /* end of DmRestartTimer */

/****************************procedure*header*****************************
 * DmGetWBPixmaps
 */
void
DmGetWBPixmaps()
{
	DmGlyphPtr gp1;
	DmGlyphPtr gp2;
		
	gp1 = DmGetPixmap(DESKTOP_SCREEN(Desktop), "emptywb");

	if (!gp1) {
		gp1 = DmGetPixmap(DESKTOP_SCREEN(Desktop), NULL);
	}
	wbdp->egp = gp1;

	gp2 = DmGetPixmap(DESKTOP_SCREEN(Desktop), "filledwb");

	if (!gp2) {
		gp2 = DmGetPixmap(DESKTOP_SCREEN(Desktop), NULL);
	}
	wbdp->fgp = gp2;

} /* end of DmGetWBPixmaps */

/****************************procedure*header*****************************
 * Creates a shell for wastebasket icon to handle drops.
 */
void
DmCreateWBIconShell(Widget toplevel, Boolean iconic)
{
	Arg wb_args[1];

	/* create process icon */
	XtSetArg(wb_args[0], XtNinitialState,
			(iconic ? IconicState : NormalState));

	XtSetArg(Dm__arg[0], XtNiconPixmap, (!WB_IS_EMPTY(Desktop)) ?
		wbdp->fgp->pix : wbdp->egp->pix);
	XtSetArg(Dm__arg[1], XtNiconMask, (!WB_IS_EMPTY(Desktop)) ?
		wbdp->fgp->mask : wbdp->egp->mask);

	wbdp->icon_shell = DtCreateProcessIcon(toplevel,
					wb_args, 1,
					NULL, 0,
					Dm__arg, 2);

	DESKTOP_WB_ICON(Desktop) = XtWindow(wbdp->icon_shell);

	/* register process icon for dnd interest */
	OlDnDRegisterDDI(wbdp->icon_shell, OlDnDSitePreviewNone,
			DmWBTriggerNotify, (OlDnDPMNotifyProc)NULL, True,
			(XtPointer)NULL);
} /* end of DmCreateWBIcon */

/****************************procedure*header*****************************
 * TriggerNotify proc for wastebasket.
 */
static Boolean
DmWBTriggerNotify(Widget w,
                  Window win,
                  Position x,
                  Position y,
                  Atom selection,
                  Time time_stamp,
                  OlDnDDropSiteID drop_site_id,
                  OlDnDTriggerOperation dnd_op,
                  Boolean send_done,
                  Boolean forwarded,	/* not used */
                  XtPointer closure)
{
} /* end of DmWBTriggerNotify */

/****************************procedure*header*****************************
 * Toggles button label of first item in Actions menu between Resume Timer
 * and Suspend Timer depending on current state of timer.
 */
void
DmWBToggleTimerBtn(char *label, char mnemonic, Boolean sensitive)
{
	Widget          button;
	BaseWindowGizmo *bw_gizmo;
	MenuGizmo       *menu_gizmo;

	bw_gizmo = (BaseWindowGizmo *)(DESKTOP_WB_WIN(Desktop)->gizmo_shell);

	menu_gizmo = (MenuGizmo *)QueryGizmo(BaseWindowGizmoClass, bw_gizmo,
				GetGizmoGizmo, "wbactionmenu");
	button = GetMenu((MenuGizmo *)(menu_gizmo));

	XtSetArg(Dm__arg[0], XtNlabel,     (XtArgVal)label);
	XtSetArg(Dm__arg[1], XtNmnemonic,  (XtArgVal)mnemonic);
	XtSetArg(Dm__arg[2], XtNsensitive, (XtArgVal)sensitive);
	OlFlatSetValues(button, 0, Dm__arg, 3);

}	/* end of DmToggleTimerBtn */

/****************************procedure*header*****************************
 * Resumes the Wastebasket timer after it has been suspended.
 */
void
DmWBResumeTimer()
{
	/*
	 * wbdp->tm_remain should have been initialised either in
	 * in DmInitWasteBasket (i.e. in DmWBRestartTimer() ),
	 * when the timer was set via the Wastebasket Properties
	 * window, or when the timer was previously suspended.
	 */
	wbdp->tm_interval = wbdp->tm_remain;
	wbdp->timer_id = XtAddTimeOut(wbdp->tm_interval,
				(XtTimerCallbackProc)DmWBTimerProc, NULL);
	time(&(wbdp->tm_start));
	wbdp->suspend = False;

	/* update status area */
	DmVaDisplayState((DmWinPtr)DESKTOP_WB_WIN(Desktop),
		TXT_WB_RESUME);

	/* Set button label to Suspend Timer */
	DmWBToggleTimerBtn(Dm__gettxt(TXT_SUSPEND_LBL),
		*Dm__gettxt(TXT_M_SUSPEND_LBL), True);

} /* end of DmWBResumeTimer */

/****************************procedure*header*****************************
 * Suspend the Wastebasket timer.
 */
void
DmWBSuspendTimer()
{
	time_t current_time;
	unsigned long elapsed;

	if (wbdp->timer_id) {
		XtRemoveTimeOut(wbdp->timer_id);
		wbdp->timer_id = NULL;
	}

	time(&(current_time));
	elapsed =(unsigned long)((current_time - wbdp->tm_start) * 1000);

	if (elapsed > wbdp->tm_interval)
		wbdp->tm_remain = (unsigned long)0;
	else
		wbdp->tm_remain = (unsigned long)(wbdp->tm_interval - elapsed);

	wbdp->suspend = True;
	DmVaDisplayState((DmWinPtr)DESKTOP_WB_WIN(Desktop),
		TXT_WB_SUSPEND);

	/* Set button label to Resume Timer */
	DmWBToggleTimerBtn(Dm__gettxt(TXT_RESUME_LBL),
		*Dm__gettxt(TXT_M_RESUME_LBL), (WB_IS_EMPTY(Desktop)?False:True));

} /* end of DmWBSuspendTimer */

DmFileInfoPtr
WBGetFileInfo(path)
char *path;
{
	DmFileInfoPtr f_info = (DmFileInfoPtr)MALLOC(sizeof(DmFileInfo));
	if (f_info != NULL) {
		struct stat buf;

		stat(path, &buf);
		f_info->mode  = buf.st_mode;
		f_info->nlink = buf.st_nlink;
		f_info->uid   = buf.st_uid;
		f_info->gid   = buf.st_gid;
		f_info->size  = buf.st_size;
		f_info->mtime = buf.st_mtime;
	}
	return(f_info);
	
} /* end of WBGetFileInfo */ 
