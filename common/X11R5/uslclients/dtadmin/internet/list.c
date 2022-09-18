/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:internet/list.c	1.16"
#endif

/******************************file*header********************************

    Description:
        This routine uses the flattened list widget (aka. FlatList)
        to display the contents of the /etc/hosts file.
*/

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/ScrolledWi.h>
#include <Xol/StaticText.h>
#include <Xol/Caption.h>
#include <Xol/TextEdit.h>
#include <Xol/TextField.h>
#include <Xol/PopupWindo.h>
#include <Xol/Error.h>
#include <Xol/Form.h>
#include <Xol/ControlAre.h>
#include <Xol/FList.h>
#include <Gizmos.h>
#include <STextGizmo.h>
#include <LabelGizmo.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include "inet.h"
#include "error.h"

extern char *		ApplicationName;
extern void		UnselectSelect();
extern void		GetFlatItems();
extern void		GetPermission();
extern void		ResteFields();
extern Widget		CreateCaption();

Arg arg[50];
static char **target;
static char *files;

static String itemResources[] = {
    XtNformatData,
};

static void
CleanUpCB(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	char *fname;
	char msg[BUFSIZ];
	sprintf(msg, GGT(string_installDone),
		(fname=strrchr(*target, '/'))? fname + 1: *target);
	PUTMSG(msg);
	if (*target) XtFree (*target);
} /* CleanUpCB */

static void
DropProcCB(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	HostData		*dp;
	FILE			*attrp;
	OlFlatDropCallData	*drop = (OlFlatDropCallData *)call_data;
	static char 		node_directory[PATH_MAX];
	char 			buf[PATH_MAX];
	static Boolean		first_time = True;
	struct stat		stat_buf;
	int			index, num;
	DmItemPtr		itemp;
	DmObjectPtr		op;

	dp = hf->flatItems[hf->currentItem].pField;

	/* check to see if the local machine has been selected */
	if (strcmp(dp->f_name, hf->nodeName) == 0) {
		PUTMSG(GGT(string_sameNode));
		return;
	}

	if (first_time) {
		first_time = False;
		sprintf (node_directory, "%s/.node", hf->userHome);
	}
	/* check the node directory is there or not. */
	/* if not, then create it			*/

	if (!DIRECTORY(node_directory, stat_buf) ) {
		if (mkdir(node_directory, DMODE) == -1) {
			PUTMSG(GGT(string_noAccessNodeDir));
			return;
		}
		if (chown(node_directory, getuid(), getgid()) == -1) {
			PUTMSG(GGT(string_noAccessNodeDir));
			return;
		}
	} else
		if ( stat_buf.st_mode != DMODE ) {
			PUTMSG(GGT(string_noAccessNodeDir));
			return;
		}

#ifdef debug
	fprintf(stderr,"the DMODE is: %o\n", DMODE);
	fprintf(stderr,"the mode for %s is: %o\n", node_directory,
						stat_buf.st_mode);
#endif

	itemp = (DmItemPtr ) drop->item_data.items;
	num = drop->item_data.num_items;
	if (num = 1) {
		sprintf (buf, "%s/%s", node_directory, dp->f_name);
		target = (char **)malloc(sizeof(char *) * (1 + 1));
		*(target + 1) = NULL;
		*target = strdup(buf);
		attrp = fopen( *target, "w");
		if (attrp == (FILE *) 0) {
			PUTMSG(GGT(string_noAccessNodeDir));
			return;
		}

		/* put the node's properties here */
		fprintf( attrp, "DUSER=%s\nDPATH=\n",
			hf->userName);
		(void) fflush(attrp);
		fclose( attrp );

		if (chmod( *target, MODE) == -1) {
			PUTMSG(GGT(string_noAccessNodeDir));
			return;
		}
		if (chown(*target, getuid(), getgid()) == -1) {
			PUTMSG(GGT(string_noAccessNodeDir));
			return;
		}
	} else {
		PUTMSG(GGT(string_noMultiple));
		return;
	}

	DtNewDnDTransaction(
		w,				/* owner widget */
		target,				/* file list */
		DT_B_STATIC_LIST,		/* options */
		drop->root_info->root_x,	/* root x */
		drop->root_info->root_y,	/* root y */
		drop->ve->xevent->xbutton.time,	/* timestamp */
		drop->dst_info->window,		/* dst window */
		DT_LINK_OP,			/* dnd hint */
		NULL,				/* del proc */
		CleanUpCB,			/* state proc */
		(XtPointer) *target		/* client data */
		);

} /* DropProcCB */

void
SelectCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
OlFlatCallData *call_data;
{
	int itemIndex = call_data->item_index;

	if (new) FreeNew();
	hf->currentItem = itemIndex;
	UnselectSelect ();
} /* SelectCB */

static StaticTextGizmo text = {
	NULL, "text",
	string_header,
	NorthWestGravity,
	OlDefaultFixedFont
};

static GizmoRec header[] = {
	{StaticTextGizmoClass,	&text},
};

LabelGizmo HeaderLine = {
	NULL, "header", "", header, XtNumber (header),
	OL_FIXEDROWS, 1, 0, 0, True
};


void
InitLists(form)
Widget form;
{
	Widget		sw;
	Widget		header;

	GetFlatItems (hf->filename);
	GetPermission ();

	header = CreateGizmo(form, LabelGizmoClass, &HeaderLine, arg,1);
	SetValue(header, XtNweight,		0);

	/* Create the scrolling list of indices */

	 sw = XtVaCreateManagedWidget (
		"Scrolled Window",
		scrolledWindowWidgetClass,
		form,
		XtNgranularity,         1,
		(String)0
	);
	hf->scrollingList = XtVaCreateManagedWidget (
		"Scrolling List",
		flatListWidgetClass,
		sw,
		XtNitems,		hf->flatItems,
		XtNnumItems,		hf->numFlatItems,
		XtNitemFields,		itemResources,
		XtNnumItemFields,	XtNumber(itemResources),
		XtNviewHeight,		VIEWHEIGHT,
		XtNformat,		"%14s %15s %s",
		XtNfont,		_OlGetDefaultFont(form, OlDefaultFixedFont),
		XtNselectProc,		SelectCB,
		XtNweight,              1,
		XtNdropProc,		DropProcCB,
		(String)0
	);
} /* InitLists */
