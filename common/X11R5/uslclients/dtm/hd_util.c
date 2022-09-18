/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:hd_util.c	1.30"

/******************************file*header********************************

    Description:
     This file contains the source code to add and remove an application
	to and from the Help Desk.
*/
                              /* #includes go here     */


#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLook.h>
#include <Xol/ScrolledWi.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
          1. Private  Procedures
          2. Public Procedures
*/

/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
 * Reads a one-line description from a help file to be displayed when
 * an icon in the Help Desk is SELECTed on.
 */
void
DmGetHDAppInfo(help_file, label, descrp)
char	*help_file;
char	**label;
char	**descrp;
{
	DmMapfilePtr mp;
	char *save_curptr;
	char *bp;
	char *ep;
	char *val;

	/* map file for reading */
	if (!(mp = Dm__mapfile(help_file, PROT_READ, MAP_SHARED)))
		return;

	save_curptr = MF_CURPTR(mp);

	if (*descrp) {
		/* scan the file for description */
		val = NULL;
		if ((bp = Dm__strstr(mp, "\n^?")) != NULL) {
			MF_NEXTC(mp); MF_NEXTC(mp); MF_NEXTC(mp);
			bp = MF_GETPTR(mp);
			if ((ep = Dm__findchar(mp, '\n')) != NULL) {
				val = (char *)strndup(bp, ep - bp);
			}
		}
		*descrp = val;
	}

	if (*label) {
		val = NULL;
		MF_CURPTR(mp) = save_curptr;
		/* scan the file for icon label */
		if ((bp = Dm__strstr(mp, "\n^:")) != NULL) {
			MF_NEXTC(mp); MF_NEXTC(mp); MF_NEXTC(mp);
			bp = MF_GETPTR(mp);
			if ((ep = Dm__findchar(mp, '\n')) != NULL) {
				val = (char *)strndup(bp, ep - bp);
			}
		}
		*label = val;
	}
	Dm__unmapfile(mp);

} /* end of DmGetHDAppInfo */

/***************************public*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
 * Adds an application (icon) to the Help Desk when a DT_ADD_TO_HELPDESK
 * client request is received.
 */
int
DmAddAppToHD(app_name, icon_label, icon_file, help_file, help_dir, full_path)
char *app_name;
char *icon_label; /* icon label from DT_ADD_TO_HELPDESK request */
char *icon_file;
char *help_file;
char *help_dir;
char *full_path;
{
	int i;
	int n;
	char buf[32];
	char *prop;
	char *descrp;
	char *icon_label_hf; /* icon label from help file */
	DmItemPtr	  itp;
	DmObjectPtr op;

	if (DESKTOP_HELP_DESK(Desktop) == NULL)
		DmInitHelpDesk(NULL, False, False);

	/* check if app already exists in Help Desk */
	for (i = 0, itp = DESKTOP_HELP_DESK(Desktop)->itp;
	     i < DESKTOP_HELP_DESK(Desktop)->nitems; i++, itp++) {

		if (ITEM_MANAGED(itp)) {
			prop = DmGetObjProperty(ITEM_OBJ(itp), HELP_FILE, NULL);
			if (strcmp(ITEM_OBJ(itp)->name, app_name) == 0 &&
				strcmp(prop, help_file) == 0)
			{
				/* application already exists */
				DmVaDisplayStatus((DmWinPtr)DESKTOP_HELP_DESK(Desktop),
					True, TXT_HDESK_APP_EXISTS, icon_label ? icon_label
					: app_name);
				return(-1);	
			}
		}
	}

	if (!(op = (DmObjectPtr)CALLOC(1, sizeof(DmObjectRec)))) {
		Dm__VaPrintMsg(TXT_MEM_ERR);
		return(-1);
	}

	op->name      = strdup(app_name);
	op->container = DESKTOP_HELP_DESK(Desktop)->cp;
	op->ftype     = DM_FTYPE_DATA;
	op->fcp       = hddp->fcp;

	DmSetObjProperty(op, HELP_FILE, help_file, NULL);

	if (help_dir)
		DmSetObjProperty(op, HELP_DIR, help_dir, NULL);
	 
	if (icon_label) {
		DmSetObjProperty(op, DFLT_ICONLABEL, icon_label, NULL);
		DmSetObjProperty(op, ICON_LABEL, icon_label, NULL);
		DmGetHDAppInfo(full_path, NULL, &descrp);
	} else {
		DmGetHDAppInfo(full_path, &icon_label_hf, &descrp);
		if (icon_label_hf)
			DmSetObjProperty(op, ICON_LABEL, icon_label_hf, NULL);
	}

	if (descrp)
		DmSetObjProperty(op, DESCRP, descrp, NULL);

	if (icon_file)
		DmSetObjProperty(op, ICONFILE, icon_file, NULL);
	 
	if ((n = DmAddObjectToIconContainer(DESKTOP_HELP_DESK(Desktop)->box,
	     &(DESKTOP_HELP_DESK(Desktop)->itp),
		&(DESKTOP_HELP_DESK(Desktop)->nitems),
	     DESKTOP_HELP_DESK(Desktop)->cp, op, 1, 1, 
	     DM_B_CALC_SIZE | DM_B_SPECIAL_NAME,
	     DESKTOP_FONTLIST(Desktop),
	     DESKTOP_FONT(Desktop),
	     DM_FontHeight(Desktop))) != OL_NO_ITEM) {

		OlSWGeometries swin_geom;
		Dimension wrap_width;

		/* set item label */
		if (icon_label)
			(DESKTOP_HELP_DESK(Desktop)->itp + n)->label =
				(XtArgVal)(strdup(icon_label));
		else if (icon_label_hf)
			(DESKTOP_HELP_DESK(Desktop)->itp + n)->label =
				(XtArgVal)(icon_label);
		else
			(DESKTOP_HELP_DESK(Desktop)->itp + n)->label =
				(XtArgVal)(strdup(app_name));
		 
		DmSizeIcon(DESKTOP_HELP_DESK(Desktop)->itp + n,
			   DESKTOP_FONTLIST(Desktop),
			   DESKTOP_FONT(Desktop));

		/* sort items by name */
		swin_geom = GetOlSWGeometries((ScrolledWindowWidget)
					(DESKTOP_HELP_DESK(Desktop)->swin));
		wrap_width = swin_geom.sw_view_width - swin_geom.vsb_width;
		DmSortItems(DESKTOP_HELP_DESK(Desktop), 0, 0, 0, wrap_width);

		DmWriteDtInfo(DESKTOP_HELP_DESK(Desktop)->cp,
			DESKTOP_HELP_DESK(Desktop)->cp->path, 0);
		return(0);

	} else {
		DmVaDisplayStatus((DmWinPtr)DESKTOP_HELP_DESK(Desktop), True,
			TXT_ADD_TO_HDESK_FAILED, icon_label ? icon_label : app_name);
		return(-1);
	}

} /* end of DmAddAppToHD */

/****************************procedure*header*****************************
 * Removes an application (icon) from the Help Desk when a DT_DEL_FROM_HELPDESK
 * request.
 */
int
DmDelAppFromHD(app_name, help_file)
char	*app_name;
char	*help_file;
{
	int i;
	char *prop;
	DmItemPtr	itp;

	if (DESKTOP_HELP_DESK(Desktop) == NULL)
		DmInitHelpDesk(NULL, False, False);

	for (i = 0, itp = DESKTOP_HELP_DESK(Desktop)->itp;
	     i < DESKTOP_HELP_DESK(Desktop)->nitems; i++, itp++) {

		if (ITEM_MANAGED(itp)) {
			prop = DmGetObjProperty(ITEM_OBJ(itp), HELP_FILE, NULL);
			if (strcmp(ITEM_OBJ(itp)->name, app_name) == 0 &&
			     strcmp(prop, help_file) == 0)
			{
				OlSWGeometries swin_geom;
				Dimension wrap_width;

				itp->managed = (XtArgVal)False;
				DmDelObjectFromContainer(DESKTOP_HELP_DESK(Desktop)->cp,
					ITEM_OBJ(itp));

				/* sort items by name */
				swin_geom = GetOlSWGeometries((ScrolledWindowWidget)
							(DESKTOP_HELP_DESK(Desktop)->swin));
				wrap_width = swin_geom.sw_view_width - swin_geom.vsb_width;
				DmSortItems(DESKTOP_HELP_DESK(Desktop), 0, 0, 0,
					wrap_width);

				DmWriteDtInfo(DESKTOP_HELP_DESK(Desktop)->cp,
					DESKTOP_HELP_DESK(Desktop)->cp->path, 0);
				return(0);
			}
		}
	}
	DmVaDisplayStatus((DmWinPtr)DESKTOP_HELP_DESK(Desktop), True,
		TXT_HDESK_APP_NOT_EXIST, app_name);
	return(-1);

} /* end of DmDelAppFromHD */

/****************************procedure*header*****************************
 * TriggerNotify proc for Help Desk.
 */
Boolean
DmHDTriggerNotify(Widget		w,
                  Window		win,
                  Position		x,
                  Position		y,
                  Atom			selection,
                  Time			time_stamp,
                  OlDnDDropSiteID	drop_site_id,
                  OlDnDTriggerOperation	dnd_op,
                  Boolean		send_done,
                  XtPointer		closure)
{
} /* end of DmHDTriggerNotify */
