/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* ident	"@(#)dtm:tb_cbs.c	1.20" */

#include <stdio.h>
#include <libgen.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Error.h>
#include <Xol/TextField.h>
#include <Xol/ScrolledWi.h>
#include <Xol/PopupWindo.h>
#include <FIconBox.h>

#include "Dtm.h"
#include "strings.h"
#include "extern.h"

#define ICON_SPACING	3
#define INITIAL_X	4
#define INITIAL_Y	4

static void DmTBCopy(Widget, XtPointer, XtPointer);
static void DmTBMove(Widget, XtPointer, XtPointer);
static void DmTBCreate(Widget, XtPointer, XtPointer);

static DmPromptInfoRec copy_info = {
    NULL, NULL, NULL, NULL, NULL, DmTBCopy, DmCancelPrompt, NULL
};

static DmPromptInfoRec move_info = {
    NULL, NULL, NULL, NULL, NULL, DmTBMove, DmCancelPrompt, NULL
};

static DmPromptInfoRec create_info = {
    NULL, NULL, NULL, NULL, NULL, DmTBCreate, DmCancelPrompt, NULL
};

static int firsttime = 1;

static void
Dm__InitTBPromptStrings()
{

        int i;
	char buf[4];

        char  **pcaptionPointers[] = {
		&copy_info.caption_label,	/* cap */
		&move_info.caption_label,	/* cap */
		&create_info.caption_label,	/* cap */
		&copy_info.title,		/* copy title */
		&move_info.title,		/* move title */
		&create_info.title,		/* create title */
		};

char  **plabelPointers[] = {
		&copy_info.action_label,	/* copy */
		&copy_info.cancel_label,	/* cancel */
		&move_info.action_label,	/* move */
		&move_info.cancel_label,	/* cancel */
		&create_info.action_label,	/* create */
		&create_info.cancel_label,	/* cancel */
		};

char  *pmnePointers[] = {
		&copy_info.action_mnemonic,	/* copy */
		&copy_info.cancel_mnemonic,	/* cancel */
		&move_info.action_mnemonic,	/* move */
		&move_info.cancel_mnemonic,	/* cancel */
		&create_info.action_mnemonic,	/* create */
		&create_info.cancel_mnemonic,	/* cancel */
		};

char *labandmne[] = {
		"tbp_copy",
		"tbp_cancel",
		"tbp_move",
		"tbp_cancel",
		"tbp_create",
		"tbp_cancel",
		};

char *caps[] = {
		"tbp_cp_cap",
		"tbp_mv_cap",
		"tbp_cr_cap",
		"tbp_cp_title",
		"tbp_mv_title",
		"tbp_cr_title",
		};

	firsttime = 0;
	for (i=0; i < XtNumber(caps); i++)
		{
		*pcaptionPointers[i] =	OlGetMessage(
                                        XtDisplay(Desktop->init_shell), NULL,
                                        0, OleNlabel, caps[i],
                                        OleCOlClientDtmMsgs,
                                        toolbox_cap_labels[i],
                                        (XrmDatabase)NULL);
		}

	for (i=0; i < XtNumber(labandmne); i ++)
		{
		*plabelPointers[i] =	OlGetMessage(
                                        XtDisplay(Desktop->init_shell), NULL,
                                        0, OleNlabel, labandmne[i],
                                        OleCOlClientDtmMsgs,
                                        toolbox_pbox_labels[i],
                                        (XrmDatabase)NULL);

		*pmnePointers[i] =	*OlGetMessage(
                                        XtDisplay(Desktop->init_shell), NULL,
                                        0, OleNlabel, labandmne[i],
                                        OleCOlClientDtmMsgs,
                                        toolbox_pbox_mnemonics[i],
                                        (XrmDatabase)NULL);
		}
		
}

static void
Dm__TBNoticePopdownCB(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	DmNoticePtr notice = (DmNoticePtr)client_data;

	XtDestroyWidget(notice->shell);
	free((void *)notice);
}

static char *
Dm__TBValidateInput(twp, prompt, options)
DmToolboxWinPtr twp;
DmPromptPtr prompt;
DtAttrs options;
{
	char *p;
	DmContainerPtr dst_cp;

	/* clear footer first */
	DmVaDisplayMsg((DmWinPtr)twp, Dm__TBErrorMessages, TB_ERR_CLEAR);

	XtSetArg(Dm__arg[0], XtNstring, &p);
	XtGetValues(prompt->input, Dm__arg, 1);

	if (prompt->current)
		free(prompt->current);

	if (p) {
		/* empty string */
		if (*p == '\0') {
			free(p);
			p = NULL;
		}
	}
	prompt->current = p;

	if (p) {
		if (options & DM_B_EXCL) {
			if (DmGetObjectInContainer(twp->cp, p)) {
				DmVaDisplayMsg((DmWinPtr)twp,
						Dm__TBErrorMessages,
						TB_ERR_EXIST, p);
				return(NULL);
			}
		}

		/*
		 * Get this now. It'll be used in several places later.
		 */
		dst_cp = DmGetToolboxInfo(p);

		if (options & DM_B_TOOLBOX) {
			if (dst_cp) {
				DmVaDisplayMsg((DmWinPtr)twp,
						Dm__TBErrorMessages,
						TB_ERR_TBEXIST, p);
				return(NULL);
			}
		}

		if (options & DM_B_TOOLBOX_DST) {
			if (!dst_cp) {
				DmVaDisplayMsg((DmWinPtr)twp,
						Dm__TBErrorMessages,
						TB_ERR_TBMISSING, p);
				return(NULL);
			}
		}
	}

	return(p);
}

static void
DmTBCopy(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	DmToolboxWinPtr twp = (DmToolboxWinPtr)client_data;
	char *p;

	/* assume error */
	twp->copy_prompt.flag = True;

	if (p = Dm__TBValidateInput(twp, &(twp->copy_prompt), 0)) {
		int i;
		DmItemPtr ip;
		DmContainerPtr dst_cp;

		if ((dst_cp = DmGetToolboxInfo(p)) == NULL) {
			DmVaDisplayMsg((DmWinPtr)twp, Dm__TBErrorMessages,
					TB_ERR_TBMISSING, p);
			return;
		}

		for (i=twp->nitems, ip=twp->itp; i; i--, ip++)
			if ((ITEM_MANAGED(ip) != False) &&
			    (ITEM_SELECT(ip) != False)) {
				if (!DmTBCopyShortcut(dst_cp,
						     twp->cp, ITEM_OP(ip),
						     0, 0)) {
					DmVaDisplayMsg((DmWinPtr)twp,
							Dm__TBErrorMessages,
							TB_ERR_TBCOPY,
							ITEM_LABEL(ip));
					return;
				}
			}

		twp->copy_prompt.flag = False;
	}

}

static void
DmTBMove(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	DmToolboxWinPtr twp = (DmToolboxWinPtr)client_data;
	char *p;

	/* assume error */
	twp->move_prompt.flag = True;

	if (p = Dm__TBValidateInput(twp, &(twp->move_prompt), 0)) {
		int i;
		DmItemPtr ip;
		DmContainerPtr dst_cp;

		if ((dst_cp = DmGetToolboxInfo(p)) == NULL) {
			DmVaDisplayMsg((DmWinPtr)twp, Dm__TBErrorMessages,
					TB_ERR_TBMISSING, p);
			return;
		}

		for (i=0, ip=twp->itp; i < twp->nitems; i++, ip++)
			if ((ITEM_MANAGED(ip) != False) &&
			    (ITEM_SELECT(ip) != False)) {
				if (DmTBMoveShortcut(dst_cp,
						     twp->cp, ITEM_OP(ip)))
					DmUnmanageIcon(twp, i);
				else {
					/* failed */
					DmVaDisplayMsg((DmWinPtr)twp,
							Dm__TBErrorMessages,
							TB_ERR_TBMOVE,
							ITEM_LABEL(ip));
					return;
				}
			}

		twp->move_prompt.flag = False;
	}
}

static void
DmTBCreate(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	DmToolboxWinPtr twp = (DmToolboxWinPtr)client_data;
	char *p;

	/* assume error */
	twp->create_prompt.flag = True;

	/* clear footer first */
	DmVaDisplayMsg((DmWinPtr)twp, Dm__TBErrorMessages, TB_ERR_CLEAR);

	if (p = Dm__TBValidateInput(twp, &(twp->create_prompt),
				 DM_B_EXCL | DM_B_TOOLBOX)) {
		if (DmCreateToolbox(twp->cp, p, 0, 0))
			twp->create_prompt.flag = False;
		else {
			DmVaDisplayMsg((DmWinPtr)twp, Dm__TBErrorMessages,
					TB_ERR_TBCREATE, p);
		}
	}
}

DmBusySelectedItems(dwp, state)
DmWinPtr dwp;
Boolean state;
{
	int i;
	DmItemPtr ip;

	XtSetArg(Dm__arg[0], XtNbusy, state);
	for (i=0, ip = dwp->itp; i < dwp->nitems; ip++, i++)
		if ((ITEM_MANAGED(ip) != False) && (ITEM_SELECT(ip) != False)) {
			OlFlatSetValues(dwp->box, i, Dm__arg, 1);
			OlFlatRefreshItem(dwp->box, i, True);
		}
}

void
DmTBOpenCB(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	DmToolboxWinPtr twp = (DmToolboxWinPtr)DmGetWinPtr(w);
	register DmItemPtr ip;
	DmObjectPtr op;
	int i;

	/* clear footer first */
	DmVaDisplayMsg((DmWinPtr)twp, Dm__TBErrorMessages, TB_ERR_CLEAR);

	/* busy all the selected icons */
	DmBusySelectedItems((DmWinPtr)twp, True);

	for (i=twp->nitems,ip = twp->itp; i; ip++, i--)
		if ((ITEM_MANAGED(ip) != False) && (ITEM_SELECT(ip) != False)) {
	        	op = ITEM_OP(ip);
			if (op->objectdata)
	        		DmOpenObject((DmWinPtr)twp, TB_REALOP(op));
			else
	        		DmOpenObject((DmWinPtr)twp, op);
		}

	/* unbusy all the selected icons */
	DmBusySelectedItems((DmWinPtr)twp, False);

}

void 
DmTBCopyCB(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	DmToolboxWinPtr twp = (DmToolboxWinPtr)DmGetWinPtr(w);

	if (firsttime)
		Dm__InitTBPromptStrings();

	DmCreatePromptBox(twp->shell, &twp->copy_prompt, &copy_info,
			  (XtPointer)twp, (XtPointer)&twp->copy_prompt, 0);
}

void 
DmTBMoveCB(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	DmToolboxWinPtr twp = (DmToolboxWinPtr)DmGetWinPtr(w);

	if (firsttime)
		Dm__InitTBPromptStrings();

	DmCreatePromptBox(twp->shell, &twp->move_prompt, &move_info,
			  (XtPointer)twp, (XtPointer)&twp->move_prompt, 0);
}

static void 
DmTBPropApplyCB(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	DmToolboxWinPtr twp = (DmToolboxWinPtr)client_data;
	DmObjectPtr op;
	char *new_label;
	int idx;
	int err = -1;

	/* get selected item */
	XtSetArg(Dm__arg[0], XtNlastSelectItem, &idx);
	XtGetValues(twp->box, Dm__arg, 1);
	if (idx == OL_NO_ITEM)
		return; /* do nothing */
	op = ITEM_OP(twp->itp + idx);

	/* get new label */
	XtSetArg(Dm__arg[0], XtNstring, &new_label);
	XtGetValues(twp->prop_prompt.input, Dm__arg, 1);

	if (!new_label || (*new_label == '\0'))
		err = TB_ERR_BADLABEL;
	else if (!strcmp(op->name, new_label)) {
		/* same label, nothing to do */
	}
	else if (DmGetObjectInContainer(twp->cp, new_label))
		 err = TB_ERR_EXIST;
	else {
		DmItemPtr ip = twp->itp + idx;
		Dimension save_w, save_h, tmp;
		Position x;

		if (op->ftype == DM_FTYPE_TOOLBOX) {
			if (DmChangeToolboxName(op->name, new_label)) {
				err = TB_ERR_TBEXIST;
				goto bye;
			}
		}
		free(op->name);
		op->name = new_label;

		free(ITEM_LABEL(ip));
		ip->label = (XtArgVal)strdup(new_label);
		save_w = ITEM_WIDTH(ip);
		save_h = ITEM_HEIGHT(ip);
		DmSizeIcon(ip,  DESKTOP_FONTLIST(Desktop),
				DESKTOP_FONT(Desktop),
				DESKTOP_LABELHT(Desktop));
		/* calculate x */
		x = ITEM_X(ip) + ((int)save_w - (int)ITEM_WIDTH(ip)) / 2;

		tmp = ITEM_WIDTH(ip);
		ip->icon_width = (XtArgVal)save_w;
		save_w = tmp;
		tmp = ITEM_HEIGHT(ip);
		ip->icon_height = (XtArgVal)save_h;
		save_h = tmp;

		XtSetArg(Dm__arg[0], XtNx, x);
		XtSetArg(Dm__arg[1], XtNwidth, save_w);
		XtSetArg(Dm__arg[2], XtNheight, save_h);
		OlFlatSetValues(twp->box, idx, Dm__arg, 3);
	}

bye:
	if (err != -1) {
		twp->prop_prompt.flag = True;
		DmVaDisplayMsg((DmWinPtr)twp, Dm__TBErrorMessages,
				err, new_label);
		free(new_label);
	}
}

static void 
DmTBPropResetCB(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	DmToolboxWinPtr twp = (DmToolboxWinPtr)client_data;
	DmObjectPtr op;
	int idx;

	/* get selected item */
	XtSetArg(Dm__arg[0], XtNlastSelectItem, &idx);
	XtGetValues(twp->box, Dm__arg, 1);
	if (idx != OL_NO_ITEM) {
		op = ITEM_OP(twp->itp + idx);
		XtSetArg(Dm__arg[0], XtNstring, op->name);
		XtSetValues(twp->prop_prompt.input, Dm__arg, 1);
	}

	/* don't pop down */
	twp->prop_prompt.flag = True;
}

void
DmCancelBusyPrompt(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	DmToolboxWinPtr twp = (DmToolboxWinPtr)client_data;

	if (twp->prop_prompt.shell) {
		DmBusyWindow(twp->shell, False, NULL, NULL);
		XtDestroyWidget(twp->prop_prompt.shell);
		twp->prop_prompt.shell = NULL;
	}
}

void 
DmTBPropCB(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	DmToolboxWinPtr twp = (DmToolboxWinPtr)DmGetWinPtr(w);
	DmObjectPtr op;
	int idx;

	/* get selected item */
	XtSetArg(Dm__arg[0], XtNlastSelectItem, &idx);
	XtGetValues(twp->box, Dm__arg, 1);
	if (idx == OL_NO_ITEM)
		return; /* do nothing */
	op = ITEM_OP(twp->itp + idx);

	if (twp->prop_prompt.shell == NULL) {
		Widget upper_ca;
		Widget lower_ca;
		char *objtype;
		char *objname;
		char *sys;
		static char *fields[] = { XtNlabel, XtNmnemonic,
				  	  XtNselectProc, XtNdefault};
		static XtArgVal button_items[] = {
			(XtArgVal)"Apply",
			(XtArgVal)'A',
			(XtArgVal)DmTBPropApplyCB,
			(XtArgVal)True,
			(XtArgVal)"Reset",
			(XtArgVal)'R',
			(XtArgVal)DmTBPropResetCB,
			(XtArgVal)False,
			(XtArgVal)"Cancel",
			(XtArgVal)'C',
			(XtArgVal)DmCancelBusyPrompt,
			(XtArgVal)False,
		};
		
		DmBusyWindow(twp->shell, True, NULL, NULL);

		twp->prop_prompt.shell = XtCreatePopupShell("Toolbox Property",
						popupWindowShellWidgetClass,
						twp->shell, NULL, 0);
		XtSetArg(Dm__arg[0], XtNupperControlArea, &upper_ca);
		XtSetArg(Dm__arg[1], XtNlowerControlArea, &lower_ca);
		XtGetValues(twp->prop_prompt.shell, Dm__arg, 2);

		if (!(objtype = DtGetProperty(&(op->plist), OBJECT_TYPE, NULL)))
			objtype = "FILE";

		if (!strcmp(objtype, "FILE")) {
			objtype = "File Name:";
			objname = DtGetProperty(&(op->plist), REAL_PATH, NULL);
		}
		else if (!strcmp(objtype, "DEVICE")) {
			objtype = "Device Name:";
			objname = DtGetProperty(&(op->plist), DEVICE_NAME,NULL);
			if (objname == NULL) {
				objname = DtGetProperty(&(op->plist),
							REAL_PATH, NULL);
				objname = basename(objname);
			}
		}
		else if (!strcmp(objtype, "TOOLBOX")) {
			objtype = "Toolbox File:";
			objname = basename(DtGetProperty(&(op->plist),
						TOOLBOX_FILE, NULL));
		}
		else {
			/* default */
			objname = DtGetProperty(&(op->plist), REAL_PATH, NULL);
		}

		twp->prop_prompt.input = DmCreateInputPrompt(upper_ca, 
					"Shortcut Name:", op->name);

		if (DtGetProperty(&(op->plist), SYSTEM_PROP, NULL))
			sys = "System";
		else
			sys = "User";
		(void)DmCreateStaticText(upper_ca, "Created By:", sys);
		(void)DmCreateStaticText(upper_ca, objtype, objname);
		(void)Dm__CreateButtons(lower_ca, button_items, 3,
					fields, XtNumber(fields),
					(XtPointer)twp, False, False);
		XtAddCallback(twp->prop_prompt.shell, XtNverify,
				Dm__VerifyPromptCB,
				(XtPointer)&(twp->prop_prompt));
		XtAddCallback(twp->prop_prompt.shell, XtNpopdownCallback,
				DmCancelBusyPrompt,
				(XtPointer)twp);
		XtPopup(twp->prop_prompt.shell, XtGrabNone);
	}	
	else
		XRaiseWindow(XtDisplay(twp->prop_prompt.shell),
			     XtWindow(twp->prop_prompt.shell));
}

void 
DmTBDeleteCB(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	DmToolboxWinPtr twp = (DmToolboxWinPtr)DmGetWinPtr(w);
	register DmItemPtr ip;
	DmObjectPtr op;
	int i;
	int err = -1;

	/* clear footer first */
	DmVaDisplayMsg((DmWinPtr)twp, Dm__TBErrorMessages, TB_ERR_CLEAR);

	for (i=0,ip = twp->itp; i < twp->nitems; ip++, i++)
		if ((ITEM_MANAGED(ip) != False) && (ITEM_SELECT(ip) != False)) {
			op = ITEM_OP(ip);
			if (DtGetProperty(&(op->plist), SYSTEM_PROP, NULL)) {
				err = TB_ERR_DELSYS;
			}
			else if (DmDelShortcut(twp->cp, op)) {
				err = TB_ERR_TBDEL;
			}

			if (err != -1) {
				DmVaDisplayMsg((DmWinPtr)twp,
						Dm__TBErrorMessages,
						err, op->name);
				return;
			}
		}
}

void 
DmTBCreateCB(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	DmToolboxWinPtr twp = (DmToolboxWinPtr)DmGetWinPtr(w);

	if (firsttime)
		Dm__InitTBPromptStrings();

	DmCreatePromptBox(twp->shell, &twp->create_prompt, &create_info,
			  (XtPointer)twp, (XtPointer)&twp->create_prompt, 0);
}

void 
DmTBSortItemsCB(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	DmToolboxWinPtr twp = (DmToolboxWinPtr)DmGetWinPtr(w);
	OlFlatCallData *d = (OlFlatCallData *)call_data;

	if (d->item_index)
		DmTBSortItems(w, twp, ByType);
	else
		DmTBSortItems(w, twp, ByName);
}

static int
TBByPos(n1, n2)
DmItemPtr  n1;
DmItemPtr  n2;
{
	int i, abs_i;
 
#define ABS(X)	((X < 0) ? -(X) : (X))

	if (ITEM_MANAGED(n2) == False)
		return(-1);
	else if (ITEM_MANAGED(n1) == False)
		return (1);

	/* this does not work correctly */
	i = (int)ITEM_Y(n1) - (int)ITEM_Y(n2);
	abs_i = ABS(i);
	if (abs_i < (int)ITEM_HEIGHT(n1)) {
		/* y is considered equal */
		return((int)ITEM_X(n1) - (int)ITEM_X(n2));
	}
	else
		return(i);
#undef ABS
}

void 
DmTBAlign(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	DmToolboxWinPtr twp = (DmToolboxWinPtr)DmGetWinPtr(w);

	DmTBSortItems(w, twp, TBByPos);
}

void 
DmTBSortItems(w, twp, cmp_func)
Widget w;
DmToolboxWinPtr twp;
int (*cmp_func)();
{
	Dimension width;
	OlSWGeometries geom;
	DmItemPtr ip;
	int i;
	Position x = INITIAL_X;
	Position y = INITIAL_Y;

	geom = GetOlSWGeometries((ScrolledWindowWidget)(twp->swin));
	width = geom.sw_view_width;

	qsort(twp->itp, twp->nitems, sizeof(DmItemRec), cmp_func);

	(void)DmComputeLayout(w, twp->itp, twp->nitems, DM_ICONIC, width, 0, 0);

	XtSetArg(Dm__arg[0], XtNitemsTouched, True);
	XtSetValues(twp->box, Dm__arg, 1);
}


