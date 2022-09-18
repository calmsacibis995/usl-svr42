/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/container.c	1.20"
#endif

/******************************file*header********************************

	Description:
	    This routine uses the flattened icon container widget
	(aka. FlatList)
	    to display the contents of the /etc/uucp/Devices file.
*/

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/ScrolledWi.h>
#include <Xt/Shell.h>		/* for XtNminWidth and XtNminHeight */

#include <Dt/Desktop.h>
#include <libDtI/DtI.h>
#include <libDtI/FIconBox.h>
#include <sys/stat.h>
#include <unistd.h>
#include "uucp.h"
#include "error.h"

/* 40755 */
extern void	GetContainerItems();
extern void	AlignIcons();
extern XtArgVal	GetValue();
extern caddr_t	PortNumber();
extern caddr_t	ModemFamily();
extern char *	device_path;

void		SelectProc(Widget, XtPointer, XtPointer);

XFontStruct	*font;

Arg arg[50];
static char **target;

DeviceFile *df;

static void
CleanUpCB(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	char msg[BUFSIZ];
	sprintf(msg, GGT(string_installDone),
		*target);
	FooterMsg(df->footer, "%s", msg);
	if (*target) XtFree (*target);
} /* CleanUpCB */

static void
DropProcCB(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	FILE			*attrp;
	OlFlatDropCallData	*drop = (OlFlatDropCallData *)call_data;
	static char 		port_directory[PATH_MAX];
	char 			buf[PATH_MAX];
	static Boolean		first_time = True;
	struct stat		stat_buf;
	int			index, num;
	DmItemPtr		itemp;
	DmObjectPtr		op;
	char			*port, *type;

	if (first_time) {
		first_time = False;
		sprintf (port_directory, "%s/.port", sf->userHome);
	}
	ClearFooter(df->footer);
	/* check the port directory is there or not. */
	/* if not, then create it			*/

	if (!DIRECTORY(port_directory, stat_buf) )
		mkdir(port_directory, DMODE);
	else
		if ( stat_buf.st_mode != DMODE ) {
			FooterMsg(df->footer, "%s", GGT(string_noAccessPortDir));
			return;
		}

#ifdef debug
	fprintf(stderr,"the DMODE is: %o\n", DMODE);
	fprintf(stderr,"the mode for %s is: %o\n", port_directory,
						stat_buf.st_mode);
#endif

	itemp = (DmItemPtr ) drop->item_data.items;
	index = drop->item_data.item_index;
	op = (DmObjectPtr)OBJECT_PTR(itemp + index);
	num = drop->item_data.num_items;
	if (num = 1) {
		sprintf (buf, "%s/%s", port_directory, (char *)itemp[index].label);
		port = strdup(((DeviceData *)op->objectdata)->portNumber);
		type = strdup(((DeviceData *)op->objectdata)->modemFamily);
		target = (char **)malloc(sizeof(char *) * (1 + 1));
		*(target + 1) = NULL;
		*target = strdup(buf);
		attrp = fopen( *target, "w");
		if (attrp == (FILE *) 0) {
			FooterMsg(df->footer, "%s", GGT(string_noAccessPortDir));
			return;
		}

		if (chmod( *target, MODE) == -1) {
			FooterMsg(df->footer, "%s", GGT(string_noAccessPortDir));
			return;
		}

		/* put the node's properties here */
		fprintf( attrp, "PORT=%s\nTYPE=%s\n",
			port, type);
		(void) fflush(attrp);
		fclose( attrp );
		XtFree(port);
		XtFree(type);
	} else {
		fprintf (stderr, "Multiple installation not supported");
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
InitContainer(form, reference)
Widget form;
Widget reference;
{
	extern XFontStruct	*font;
	extern XFontStruct	* _OlGetDefaultFont();
	Widget			swin;

	font = _OlGetDefaultFont(form, OlDefaultFont);
	GetContainerItems(device_path);

	    /* Create icon container        */

	XtSetArg(arg[0], XtNselectProc, (XtArgVal)SelectProc);
	XtSetArg(arg[1], XtNadjustProc, (XtArgVal)SelectProc);
	XtSetArg(arg[2], XtNmovableIcons, (XtArgVal)False);
	XtSetArg(arg[3], XtNexclusives, True);
	XtSetArg(arg[4], XtNdropProc, DropProcCB);
	XtSetArg(arg[5], XtNdrawProc, (XtArgVal)DmDrawIcon);
	XtSetArg(arg[6], XtNminWidth, (XtArgVal)1);
	XtSetArg(arg[7], XtNminHeight, (XtArgVal)1);
	df->iconbox = DmCreateIconContainer(form,
		DM_B_NO_INIT | DM_B_CALC_SIZE,
		arg,
		8,
		df->cp->op,
		df->cp->num_objs,
		&df->itp,
		df->cp->num_objs,
		&swin,
		NULL,
		font,
		1);

	AlignIcons();

	XtVaSetValues(swin,
		XtNyResizable,	True,
   		XtNxResizable,	True,
		XtNxAttachRight,  True,
		XtNyAttachBottom, True,
		XtNyAddHeight,	  True,
		XtNyAttachOffset, OFFSET,
		XtNyRefWidget,	  reference,
		(String)0
	);

} /* InitContainer */

void
AlignIcons()
{
	Cardinal	i;
	DmItemPtr	item;
	Dimension	width;
	Cardinal	nitems;


	nitems = GetValue(df->iconbox, XtNnumItems);
	width = GetValue(df->iconbox, XtNwidth);
#ifdef NITEMS
		fprintf(stderr, "In AlignIcons, nitems = %d\n", nitems);
		fprintf(stderr, "In AlignIcons, width = %u\n", width);
#endif
	if ( width < 30 || width > 800)
		width = (Dimension) BNU_WIDTH - 30;
	else
		width -= 30; 
#ifdef NITEMS
		fprintf(stderr, "In AlignIcons, nitems = %d\n", nitems);
		fprintf(stderr, "In AlignIcons, width = %u\n", width);
#endif
	for (i = 0, item = df-> itp; i < nitems;i++, item++)
		item-> x = item-> y = UNSPECIFIED_POS;

	for (i = 0, item = df-> itp; i < nitems;i++, item++)
		if (ITEM_MANAGED(item)) {
			DmGetAvailIconPos(df->itp, nitems,
				  ITEM_WIDTH(item), ITEM_HEIGHT(item), width,
				  INC_X, INC_Y,
				  (Position *)&item->x, (Position *)&item->y);
#ifdef debug
			fprintf(stderr, "x = %u, y = %u\n",item->x, item->y);
#endif
		}

	SetValue(df-> iconbox, XtNitemsTouched, True);
} /* AlignIcons */

void
SelectProc(w, client_data, call_data)
Widget	      w;
XtPointer	   client_data;
XtPointer	   call_data;
{
	String type;
	OlFIconBoxButtonCD *d = (OlFIconBoxButtonCD *)call_data;

	ClearFooter(df->footer);
#ifdef debug
	printf("SelectProc(idx=%d, cnt=%d)\n",d->item_data.item_index,d->count);
#endif
	df->select_op = OBJECT_CD(d->item_data);
	PortNumber(SET,
		((DeviceData*)(df->select_op->objectdata))->portNumber);
	ModemFamily(SET,
		((DeviceData*)(df->select_op->objectdata))->modemFamily);
	if (df->w_acu == NULL) return;
	type = ((DeviceData*)(df->select_op->objectdata))->modemFamily;
	if (strcmp(type, "datakit") != 0 && strcmp(type, "direct") != 0)
		if (XtIsRealized(df->w_acu))
			XtMapWidget(df->w_acu);
		else
			SetValue(df->w_acu, XtNmappedWhenManaged, TRUE, NULL);
	else
		if (XtIsRealized(df->w_acu))
			XtUnmapWidget(df->w_acu);
		else
			SetValue(df->w_acu, XtNmappedWhenManaged, False, NULL);
} /* SelectProc */
