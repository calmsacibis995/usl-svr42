/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtmail:manage.c	1.76"
#endif

#define MANAGE_C

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/ScrolledWi.h>
#include <Xol/TextEdit.h>
#include <Xol/TextField.h>
#include <Xol/Error.h>
#include <Xol/Form.h>
#include <Xol/Flat.h>
#include <Xol/FList.h>
#include <Xol/ControlAre.h>
#include <Xol/StaticText.h>
#include <Xol/RubberTile.h>
#include <X11/Shell.h>			/* need this for XtNtitle */
#include "mail.h"
#include "RMailGizmo.h"
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/STextGizmo.h>
#include <Gizmo/LabelGizmo.h>

extern HelpInfo		ManagerHelp;
extern ReadRec *	readRec;
extern int		LastSelectedMessage;
extern MailRec *	mailRec;
extern BaseWindowGizmo	MainWindow;
extern int		Version;
extern Widget		Root;
extern Boolean		SendOnly;
extern char *		UserName;

typedef enum {
	MenuOpenUser, MenuSave, MenuSaveAs, MenuPrint, MenuExit
} MenuFileItemIndex;

typedef enum {
	MenuUndo, MenuDelete, MenuUndelete, MenuSelect, MenuUnselect
} MenuEditItemIndex;

typedef enum {
	MenuRead, MenuMailer, MenuReplySender, MenuReplySenderAtt,
	MenuReplyAll, MenuReplyAllAtt, MenuForward
} MenuMailItemIndex;

typedef enum {
	MenuFile, MenuEdit, MenuMail, MenuHelp
} MenuMenuItemIndex;

ManageRec *	manageRec = (ManageRec *)0;

#define DELETE_BUTTON	0
#define UNDELETE_BUTTON	1
#define FILE_BUTTON	0
#define EDIT_BUTTON	1
#define UPDATE_BUTTON	2

#define PRINT_FAILED	"Pipe to: \"PrtMgr\"\nUX:mailx: ERROR: Pipe to \"PrtMgr\" failed\n"

ManageRec * 
FindManageRec(wid)
Widget wid;
{
	ManageRec *	mng;
	Widget		shell;

	shell = GetToplevelShell (wid);
	for (mng=manageRec; mng!=(ManageRec *)0; mng=mng->next) {
		if (shell == GetBaseWindowShell (mng->baseWGizmo)) {
			return mng;
		}
	}
	return (ManageRec *)0;
}

ListGizmo *
GetSummaryListGizmo (mng)
ManageRec *	mng;
{
	return (ListGizmo *) QueryGizmo (
		BaseWindowGizmoClass, mng->baseWGizmo,
		GetGizmoGizmo, SUMMARY_LIST
	);
}

Widget
GetSummaryListWidget (mng)
ManageRec *	mng;
{
	if (mng == (ManageRec *)0) {
		return (Widget)0;
	}
	return (Widget) QueryGizmo (
		BaseWindowGizmoClass, mng->baseWGizmo,
		GetGizmoWidget, SUMMARY_LIST
	);
}

/* Update the status field of the current mail message.
 * This is accomplished by rereading the header for the message
 * and extracting the status field.
 */

void
DisplayStatus (mp, buf)
MailRec *	mp;
char *		buf;
{
	int		item;
	int		i;
	char *		string;
	char *		tmp;
	char *		savept = NULL;
	ListGizmo *	gizmo;

	tmp = buf;
	while ((string = MyStrtok (tmp, "\n", &savept)) != NULL) {
		sscanf (string+2, "%d", &item);
		/* Look backwards because the index will always be
		 * less than the message number (item).
		 */
		i = (item>mp->summary->size)
			? mp->summary->size-1
			: item-1;
		for (; i>=0; i--) {
			if (item == mp->summary->items[i].clientData) {
				break;
			}
		}
		tmp = NULL;
		SetStatusField (
			string+1,
			(char **)mp->summary->items[i].fields
		);
		if (mp->mng != (ManageRec *)0) {
			OlVaFlatSetValues (
				GetSummaryListWidget (mp->mng),
				i,
				XtNformatData, mp->summary->items[i].fields,
				(String)0
			);
		}
	}
}

static Boolean
DisplayInRightFooter (bw, buf)
BaseWindowGizmo *       bw;
char *			buf;
{
	int i = strlen (buf)-1;
	if (buf [i] == '\n') {
		buf[i] = '\0';
	}
	SetBaseWindowStatus (bw, buf);
	return True;
}

void
UpdateFooter (mp)
MailRec *mp;
{
	static Pixmap	rdmail = 0;
	static Pixmap	nrdmail;
	static Pixmap	manmail;
	static Pixmap	nmanmail;
	ReadRec *	rp;
	char **		tmp;
	static int	height;
	static int	width;
	int		i;
	int		new = 0;
	int		unread = 0;
	char		buf[256];
	Widget		shell;

	for (i=0; i<mp->summary->size; i++) {
		tmp = (char **)mp->summary->items[i].fields;
		switch (tmp[F_TEXT][F_STATUS]) {
			case 'N': 
				new += 1;
			case 'U': 
				unread += 1;
				break;
			case 'D': 
			case 'O': 
			case 'H': 
			case 'R': 
			case 'S': 
			case 'M': 
				break;
		}
	}
	sprintf (
		buf, GetGizmoText (TXT_MANAGE_RIGHT_FOOTER),
		mp->summary->size, new, unread
	);
	if (mp->mng != (ManageRec *)0) {
		DisplayInRightFooter (mp->mng->baseWGizmo, buf);
	}
	if (rdmail == (Pixmap)0) {
		rdmail = PixmapOfFile (
			Root,
			"rdmail.icon",
			GetGizmoText (TXT_READER_ICON_NAME),
			&width, &height
		);
		nrdmail = PixmapOfFile (
			Root,
			"nrdmail.icon",
			GetGizmoText (TXT_READER_ICON_NAME),
			&width, &height
		);
		manmail = PixmapOfFile (
			Root,
			"manmail.icon",
			GetGizmoText (TXT_MANAGER_ICON_NAME),
			&width, &height
		);
		nmanmail = PixmapOfFile (
			Root,
			"nmanmail.icon",
			GetGizmoText (TXT_MANAGER_ICON_NAME),
			&width, &height
		);
	}
	/*
	 * Set the icon pixmaps values depending on whether there
	 * is new mail or not.
	 */
	for (rp=readRec; rp!=(ReadRec *)0; rp=rp->next) {
		if (rp->mp == mp) {
			DisplayInRightFooter (rp->baseWGizmo, buf);
			shell = rp->baseWGizmo->icon_shell;
			XtVaSetValues (
				shell,
				XtNbackgroundPixmap,
				(new == 0) ? rdmail : nrdmail,
				XtNwidth,		width,
				XtNheight,		height,
				(String)0
			);
			XClearWindow (XtDisplay (shell), XtWindow (shell));
		}
	}
	if (mp->mng != (ManageRec *)0) {
		shell = mp->mng->baseWGizmo->icon_shell;
		XtVaSetValues (
			shell,
			XtNbackgroundPixmap,	(new==0) ? manmail : nmanmail,
			XtNwidth,		width,
			XtNheight,		height,
			(String)0
		);
		XClearWindow (XtDisplay (shell), XtWindow (shell));
	}
}

/* Update the status of messages between start and end that
 * are selected.
 */
void
UpdateStatusOfMessage (mp, start, end)
MailRec *	mp;
int		start;
int		end;
{
	int i;
	char *buf;

	buf = CompactListOfSelectedItems (mp, mp->summary, NULL, start, end);
	if (buf[0] != '\0') {
		DisplayStatus (mp, ProcessCommand (mp, FROM_CMD, buf));
	}
	/* Update the footer of the main window */
	UpdateFooter (mp);
}

static void
ToggleItem (mp, list, lp, item)
MailRec *	mp;
Widget		list;
ListHead *	lp;
int		item;
{
	if (lp->items[item].set == False) {
		SelectItem (mp, lp, item);
	}
	else {
		UnselectItem (mp, lp, item);
	}
}

static void
SelectAll (mng)
ManageRec *	mng;
{
	MailRec *mp = mng->mp;
	Widget	list;
	int	i;

	/* Set the number of selected items = size */
	mp->summary->clientData = (XtArgVal)mp->summary->size;
	list = GetSummaryListWidget (mng);
	for (i=0; i<mp->summary->size; i++) {
		if (mp->summary->items[i].set == False) {
			OlVaFlatSetValues (
				list, i, XtNset, True, (String)0
			);
		}
	}
}

void
UnselectAll (mng, lp, except)
ManageRec *	mng;
ListHead *	lp;
int		except;	/* Unselect all items except this one */
{
	MailRec *mp = mng->mp;
	Widget	list = (Widget)0;
	int	i;

	FPRINTF ((stderr, "UnselectAll\n"));
	/* Set the number of selected items = 0 */
	lp->clientData = (XtArgVal)0;
	if (mng != (ManageRec *)0) {
		list = GetSummaryListWidget (mng);
		if (lp == mng->mp->deleted) {
			list = (Widget)0;
			if (mng->deleteListPopup != (PopupGizmo *)0) {
				list = GetDeletedListWidget (mng);
			}
		}
	}
	for (i=0; i<lp->size; i++) {
		if (lp->items[i].set == True) {
			if (i == except) {
				lp->clientData = (XtArgVal)1;
				LastSelectedMessage =
					(int)lp->items[i].clientData;
			}
			else {
				if (list != (Widget)0) {
					OlVaFlatSetValues (
						list, i, XtNset,
						False, (String)0
					);
				}
				else {
					lp->items[i].set = False;
				}
			}
		}
	}
}

void
DisplayInLeftFooter (bw, buf, raise)
BaseWindowGizmo *	bw;
char *			buf;
{
	int i = strlen (buf)-1;
	if (buf [i] == '\n') {
		buf[i] = '\0';
	}
	SetBaseWindowMessage (bw, buf);
	if (raise == True) {
		MapGizmo (BaseWindowGizmoClass, bw);
		XRaiseWindow (XtDisplay (bw->shell), XtWindow (bw->shell));
	}
}

void
LookForAdjustCB (wid, lp, call_data)
Widget		wid;
ListHead *	lp;
XtPointer	call_data;
{
	OlVirtualEvent	ve = (OlVirtualEvent)call_data;
	MailRec		*mp = FindMailRec (wid);

	if (mp == (MailRec *)0 ||
	    mp->mng == (ManageRec *)0 ||
	    mp->mng->mapped == False) {
		FPRINTF ((stderr, "Record no longer there\n"));
		return;
	}
	switch (ve->virtual_name) {
		case OL_ADJUST:
		case OL_ADJUSTKEY: {
			if (
				ve->xevent->type == ButtonPress ||
				ve->xevent->type == KeyPress
			) {
				/* Toggle the current item */
				FPRINTF ((stderr, "Got Adjust\n"));
				ResetUndo (mp);
				ve->consumed = True;
				ToggleItem (mp, wid, lp, ve->item_index);
			}
			break;
		}
		case OL_SELECTKEY:
		case OL_SELECT: {
			ResetUndo (mp);
			if (ve->xevent->type == ButtonPress ||
			    ve->xevent->type == KeyPress
			) {
				FPRINTF ((stderr, "Got Select\n"));
				UnselectAll (mp->mng, lp, ve->item_index);
			}
			break;
		}
	}
}

void
UnselectCB (list, client_data, call_data)
Widget			list;
XtArgVal		client_data;
OlFlatCallData *	call_data;
{
	MailRec *	mp = FindMailRec (list);
	int		item = call_data->item_index;
	ListHead *	lp = (ListHead *)call_data->user_data;

	UnselectItem (mp, lp, item);
	FPRINTF ((stderr, "UnselectCB=%d\n", lp->clientData));
}

void
SelectCB (list, client_data, call_data)
Widget			list;
XtArgVal		client_data;
OlFlatCallData *	call_data;
{
	MailRec *	mp = FindMailRec (list);
	int		item = call_data->item_index;
	ListHead *	lp = (ListHead *)call_data->user_data;
	int i;

	SelectItem (mp, lp, item);
	FPRINTF ((stderr, "SelectCB=%d\n", lp->clientData));
}

/* Double clicking on an item means read the item */

void
ExecuteCB (wid, client_data, call_data)
Widget			wid;
XtPointer		client_data;
OlFlatCallData *	call_data;
{
	MailRec *	mp = FindMailRec (wid);
	ListHead *	lp = (ListHead *)call_data->user_data;
	int		i = call_data->item_index;

	/* First, unselect all other items */

	ResetUndo (mp);
	SelectItem (mp, lp, i);

	if (lp == mp->summary) {
		/* Then, go read the currently selected item */

		ReadProc (mp);
	}
	else {
		/* Then go and undelete the message */
		ManageUndelete (mp->mng);

		BringDownPopup (_OlGetShellOfWidget (wid));
	}
	FPRINTF ((stderr, "ExecuteCB=%d\n", lp->clientData));
}

void
PrintMessageList (mp, bw, messages, printed)
MailRec *		mp;
BaseWindowGizmo *	bw;
char *			messages;	/* List of messages to be printed */
char *			printed;	/* Message put in footer of bw */
{
	XEvent	event;
	char *	buf;

	while (XtPending ()) {
		XtNextEvent (&event);
		XtDispatchEvent (&event);
	}
	buf = ProcessCommand (mp, PIPE_CMD, messages);
	if (strcmp (buf, PRINT_FAILED) == 0) {
		DisplayInLeftFooter (
			bw, GetGizmoText (TXT_PRINT_FAILED), True
		);
	}
	else {
		DisplayInLeftFooter (bw, GetGizmoText (printed), True);
	}
	UpdateFooter (mp);
	UpdateStatusOfMessage (mp, 0, mp->summary->size);
}

static void
PrintMessages (mng)
ManageRec *	mng;
{
	char *		messages;
	MailRec *	mp = mng->mp;

	messages = CompactListOfSelectedItems (
		mp, mp->summary, NULL, 0, mp->summary->size
	);
	PrintMessageList (mp, mng->baseWGizmo, messages, TXT_PRINTED_MSGS);
}

static void
FileMenuCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	OlFlatCallData *	p = (OlFlatCallData *)call_data;
	ManageRec *		mng = FindManageRec (wid);

	DisplayInLeftFooter (mng->baseWGizmo, "", True);
	DisplayInLeftFooter (&MainWindow, "", False);

	switch (p->item_index) {
		case MenuOpenUser: {
			OpenUser (mng);
			ResetUndo (mng->mp);
			break;
		}
		case MenuSave: {
			ManageSave(mng);
			ResetUndo (mng->mp);
			break;
		}
		case MenuSaveAs: {
			ManageSaveAs(mng);
			ResetUndo (mng->mp);
			break;
		}
		case MenuPrint: {
			PrintMessages (mng);
			ResetUndo (mng->mp);
			break;
		}
		case MenuExit: {
			ManageExitCB (wid, client_data, call_data);
			break;
		}
	}
}

static void
Undo (mng)
ManageRec *	mng;
{
	if (mng->lastOp == DeleteOp) {
		ManageUndo (mng);
	}
	ResetUndo (mng->mp);
}

static void
EditMenuCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	OlFlatCallData *	p = (OlFlatCallData *)call_data;
	ManageRec *		mng = FindManageRec (wid);

	DisplayInLeftFooter (mng->baseWGizmo, "", True);
	DisplayInLeftFooter (&MainWindow, "", False);
	switch (p->item_index) {
		case MenuUndo: {
			Undo (mng);
			break;
		}
		case MenuDelete: {
			ManageDelete (mng->mp);
			break;
		}
		case MenuUndelete: {
			UndeleteProc (mng);
			break;
		}
		case MenuSelect: {
			SelectAll (mng);
			ResetUndo (mng->mp);
			break;
		}
		case MenuUnselect: {
			UnselectAll (mng, mng->mp->summary, -1);
			ResetUndo (mng->mp);
			break;
		}
	}
}

static void
MailMenuCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	OlFlatCallData *	p = (OlFlatCallData *)call_data;
	ManageRec *		mng = FindManageRec (wid);

	DisplayInLeftFooter (mng->baseWGizmo, "", True);
	DisplayInLeftFooter (&MainWindow, "", False);
	switch (p->item_index) {
		case MenuRead: {
			ReadProc (mng->mp);
			break;
		}
		case MenuMailer: {
			ManageSend ();
			break;
		}
		case MenuReplySender: {
			ReplySenderProc (wid);
			break;
		}
		case MenuReplySenderAtt: {
			ReplySenderAttProc (wid);
			break;
		}
		case MenuReplyAll: {
			ReplyAll (wid);
			break;
		}
		case MenuReplyAllAtt: {
			ReplyAllAtt (wid);
			break;
		}
		case MenuForward: {
			Forward (mng->mp);
			break;
		}
	}
	ResetUndo (mng->mp);
}

static ManageRec *
FindScrollManageRec (wid)
Widget	wid;
{
	MailRec *	mp;
	Widget		w;

	for (mp=mailRec; mp!=(MailRec *)0; mp=mp->next) {
		if (mp->mng != (ManageRec *)0) {
			w = GetSummaryListWidget (mp->mng);
			if (w == wid) {
				return mp->mng;
			}
		}
	}
	return (ManageRec *)0;
}

typedef enum {PipeOpen, PipeClose} PipeType;

static XtCallbackProc
FinishDropCB (wid, tmpfile, call_data)
Widget		wid;
char *		tmpfile;
XtPointer	call_data;
{
	FILE *		fp;
	char		buf[BUF_SIZE];
	char		cmd[BUF_SIZE];
	ManageRec *	mng;

	mng = FindScrollManageRec (wid);
	if (mng == (ManageRec *)0) {
		return;
	}

	/*
	 * Open the temporary file created by the drop transaction
	 * and read in the name of the file dropped on.
	 */
	if ((fp = fopen (tmpfile, "r")) == NULL) {
		FPRINTF ((stderr, "Can't open temporary file %s\n", tmpfile));
		CreateInvalidTargetModal (wid);
		return;
	}
	fgets (buf, BUF_SIZE, fp);
	*(strchr (buf, '\n')) = '\0';
	sprintf (cmd, "%s %d", SAVE_CMD, LastSelectedMessage);
	/*
	 * Save to this drop target.
	 */
	(void)DisplaySaveStatus (
		mng->baseWGizmo, ProcessCommand (mng->mp, cmd, buf)
	);

	fclose (fp);
	unlink (tmpfile);
}

static void
DroppedOutsideDtmail (wid, p)
Widget		wid;
OlFlatDropCallData *    p;
{
	ManageRec *	mng;
	int		x, y;
	DtDnDSendPtr	ret;
	char **		files;
	Window		child;
	static char *	tmpfile;

	mng = FindScrollManageRec (wid);
	if (mng == (ManageRec *)0) {
		return;
	}
	if (p->drop_status != OlDnDDropSucceeded) {
		FPRINTF ((stderr, "Drop cancelled or unregistered\n"));
		return;
	}

	XTranslateCoordinates (
		XtDisplay (Root),
		/* p->drop_data.window, */
		p->dst_info->window,
		RootWindowOfScreen(XtScreen(Root)),
		(Position)(p->dst_info)->x,
		(Position)(p->dst_info)->y,
		&x, &y,
		&child
	);

	tmpfile = tmpnam(0);

	files = (char **)MALLOC (sizeof(char *)*2);
	files[0] = STRDUP (tmpfile);
	files[1] = NULL;

	/*
	 * When a drop occurs on a .ml file the DtNewDnDTransaction
	 * routine will execute the appropriate line in the classdb
	 * file.  This command will echo the name of the file dropped
	 * on and put it into the file specified by tmpfile.
	 * Later, in the callback FinishDropCB(), this file is read
	 * to obtain the file name that was dropped on and this file
	 * is used for the save.
	 */
	ret = DtNewDnDTransaction (
		wid,
		files,
		0,
		x, y,
		(p->root_info)->drop_timestamp,
		(p->dst_info)->window,
		0,
		0,
		(XtCallbackProc)FinishDropCB,
		(XtPointer)tmpfile
	);
	if (ret == (DtDnDSendPtr)0) {
		FPRINTF ((stderr, "DtNewDnDTransaction failed\n"));
	}
}

static void
AppProc(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	DtDnDInfoPtr	dip = (DtDnDInfoPtr)call_data;
	char **		p;
	ManageRec *	mng = FindManageRec (w);
	Widget		shell;

	if (dip->error != 0) {
		return;
	}
	if (dip->nitems != 1) {
		if (mng == (ManageRec *)0) {
			shell = GetBaseWindowShell (&MainWindow);
		}
		else {
			shell = GetBaseWindowShell (mng->baseWGizmo);
		}
		DisplayErrorPrompt (shell, GetGizmoText (TXT_ONLY_ONE_FILE));
		return;
	}
	if (dip->files != (char **)0) {
		if (dip->files[0][0] == '\0') {
			/* Just popup the main window */
			MapGizmo (BaseWindowGizmoClass, &MainWindow);
			XRaiseWindow (
				XtDisplay (MainWindow.shell),
				XtWindow (MainWindow.shell)
			);
		}
		else {
			(void)OpenNewMailFile (*dip->files, &MainWindow);
		}
	}
}

Boolean
ManageDropNotify (
	Widget w, Window win, Position x, Position y, Atom selection,
	Time timestamp, OlDnDDropSiteID drop_site_id,
	OlDnDTriggerOperation op, Boolean send_done, Boolean forwarded,
	XtPointer closure)
{
	FPRINTF ((stderr, "Got a drop on manage\n"));
	DtGetFileNames (w, selection, timestamp, send_done, AppProc, closure);
}

static void
DropProcCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
OlFlatDropCallData *call_data;
{
	int		i = call_data->item_data.item_index;
	OlFlatDropCallData *	p = call_data;
	MailRec *	mp = FindMailRec (wid);
	ManageRec *	mng;
	SendRec *	sp;
	ReadRec *	rp;
	ReadRec *	trp;
	Widget		widget;

	/*
	 * The button press that initiated this drop would have
	 * unselected all of the items.  We need to indicate that
	 * one item is selected here.  LastSelectedItem will be set
	 * by the previous select of the dragged item.
	 */
	mp->summary->clientData = 1;
	FPRINTF ((stderr, "DropCB=%d\n", 1));

	widget = XtWindowToWidget (XtDisplay (Root), (p->dst_info)->window);
	if (p->drop_status != OlDnDDropSucceeded) {
		FPRINTF ((stderr, "Drop cancelled or unregistered\n"));
		return;
	}

	if (widget == (Widget)0) {
		DroppedOutsideDtmail (wid, p);
		return;
	}
	if ((sp = FindSendRec (widget)) != (SendRec *)0) {
		FPRINTF ((stderr, "Dropped on send window\n"));
		SendDropReply (sp, wid);
		return;
	}

	if ((rp = FindReadRec (widget)) != (ReadRec *)0 && rp->mp == mp) {
		FPRINTF ((stderr, "Dropped on read window\n"));
		/*
		 * First see if the message is already being displayed
		 */
		trp = FindMessageInList (mp, MessageNumber (mp, i));
		if (trp != (ReadRec *)0) {
			/*
			 * If so, then just raise this window
			 */
			widget = GetBaseWindowShell (trp->baseWGizmo);
		}
		/*
		 * Otherwise, read the message into the window that
		 * received the drop.
		 */
		else {
			ReadItem (rp->mp, rp, i);
			widget = GetBaseWindowShell (rp->baseWGizmo);
		}
		XRaiseWindow (XtDisplay (widget), XtWindow (widget));
		return;
	}

	if ((mng = FindManageRec (widget)) != (ManageRec *)0) {
		FPRINTF ((stderr, "Dropped on manage window\n"));
		return;
	}

}


static void
FileSelectCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	ManageRec *	mng = FindManageRec (wid);
	MailRec *	mp = mng->mp;
	Boolean		flag = True;

	if (mp->summary->size == 0 || mp->summary->clientData == 0) {
		flag = False;
	}
	SetSensitivity (mng->baseWGizmo, "manageFile", 1, 3, flag);
}

static void
EditSelectCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	ManageRec *	mng = FindManageRec (wid);
	MailRec *	mp = mng->mp;
	Boolean		flag = True;

	/*
	 * Set the sensitivity of the edit buttons
	 * SelectAll and UnselectAll.
	 */
	if (mp->summary->size == 0) {
		flag = False;
	}
	SetSensitivity (mng->baseWGizmo, "manageEdit", 3, 4, flag);
	/*
	 * Set the sensitivity of the edit button Delete,
	 */
	flag = True;
	if (mp->summary->size == 0 || mp->summary->clientData == 0) {
		flag = False;
	}
	SetSensitivity (mng->baseWGizmo, "manageEdit", 1, 1, flag);
	/*
	 * Set the sensitivity of the edit Undo button.
	 */
	flag = True;
	if (mng->lastOp == UndoOp) {
		flag = False;
	}
	SetSensitivity (mng->baseWGizmo, "manageEdit", 0, 0, flag);
	/*
	 * Set the sensitivity of the edit Undelete button.
	 */
	flag = True;
	if (mp->deleted->size == 0) {
		flag = False;
	}
	SetSensitivity (mng->baseWGizmo, "manageEdit", 2, 2, flag);
}

static void
MailSelectCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	ManageRec *	mng = FindManageRec (wid);
	MailRec *	mp = mng->mp;
	Boolean		flag = True;

	/*
	 * Set the sensitivity of the Reader... button.
	 */
	if (mp->summary->clientData == 0) {
		flag = False;
	}
	SetSensitivity (mp->mng->baseWGizmo, "manageMail", 0, 0, flag);
	/*
	 * Set the sensitivity of the reply buttons.  This can only
	 * be sensitive if only one item is selected.
	 */
	flag = False;
	if (mp->summary->clientData == 1) {
		flag = True;
	}
	SetSensitivity (mp->mng->baseWGizmo, "manageMail", 2, 6, flag);
}

static void
ShowLimitsCB (wid, client_data, ep)
Widget				wid;
XtArgVal			client_data;
OlFListItemsLimitExceededCD *	ep;
{
	MailRec *	mp = FindMailRec (wid);
	char		buf[BUF_SIZE];

	sprintf (buf, GetGizmoText (TXT_LIMITS_EXCEEDED), ep->preferred);
	DisplayInLeftFooter (mp->mng->baseWGizmo, buf, False);
	ep->ok = True;
}

static XtCallbackRec	LimitsCB[] = {
	{ShowLimitsCB,	(XtPointer)NULL},
	{NULL}
};

static StaticTextGizmo firstHalf = {
	NULL, "firstHalf",
	TXT_FIRST_HALF_HEADER,
	NorthWestGravity,
	NULL
};

static StaticTextGizmo secondHalf = {
	NULL, "secondHalf",
	TXT_SECOND_HALF_HEADER,
	NorthWestGravity,
	NULL
};

static GizmoRec headerHalfs[] = {
	{StaticTextGizmoClass,	&firstHalf},
	{StaticTextGizmoClass,	&secondHalf}
};

LabelGizmo HeaderLine = {
	NULL, "header", "", headerHalfs, XtNumber (headerHalfs),
	OL_FIXEDROWS, 1, 0, 0, True
};


static Setting summaryListSetting = {
	"None",
	(XtPointer)NULL,
	(XtPointer)NULL,
	(XtPointer)"_"
};

static ListGizmo summaryList = {
	NULL, SUMMARY_LIST, "", &summaryListSetting,
	FORMAT, False, 5,
	NULL, NULL, ExecuteCB, SelectCB, UnselectCB, LimitsCB
};

static GizmoRec summaryListArray[] = {
	{LabelGizmoClass,	&HeaderLine},
	{ListGizmoClass,	&summaryList},
};

/* Manage Base Window Menubar ---------------------------------------- */

static MenuItems fileItems[] = {
    {True, BUT_OPEN_DDD,		MNEM_OPEN_DDD},
    {True, BUT_SAVE_MESSAGES,		MNEM_SAVE_MESSAGES},
    {True, BUT_SAVE_MESSAGES_AS_DDD,	MNEM_SAVE_MESSAGES_AS_DDD},
    {True, BUT_PRINT,			MNEM_PRINT},
    {True, BUT_EXIT,			MNEM_EXIT_E},
    {NULL}
};

static MenuGizmo file =     {
	NULL, "manageFile", NULL, fileItems, FileMenuCB,
	NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static MenuItems editItems[] = {
    {True, BUT_UNDO,		MNEM_UNDO},
    {True, BUT_DELETE,		MNEM_DELETE},
    {True, BUT_UNDELETE_DDD,	MNEM_UNDELETE_DDD_N},
    {True, BUT_SELECT_ALL,	MNEM_SELECT_ALL},
    {True, BUT_UNSELECT_ALL,	MNEM_UNSELECT_ALL},
    {NULL}
};

static MenuGizmo edit =     {
	NULL, "manageEdit", NULL, editItems, EditMenuCB,
	NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static MenuItems mailItems[] = {
    {True, BUT_READ_DDD,		MNEM_READ_DDD},
    {True, BUT_MAILER_DDD,		MNEM_MAILER_DDD},
    {True, BUT_REPLY_SENDER_DDD,	MNEM_REPLY_SENDER_DDD},
    {True, BUT_REPLY_SENDER_ATT_DDD,	MNEM_REPLY_SENDER_ATT_DDD},
    {True, BUT_REPLY_ALL_DDD,		MNEM_REPLY_ALL_DDD},
    {True, BUT_REPLY_ALL_ATT_DDD,	MNEM_REPLY_ALL_ATT_DDD},
    {True, BUT_FORWARD_DDD,		MNEM_FORWARD_DDD},
    {NULL}
};
static MenuGizmo mail = {
	NULL, "manageMail", NULL, mailItems, MailMenuCB,
	NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static MenuItems manageHelpItems[] = {
    {True, BUT_MANAGER_DDD,	MNEM_MANAGER_DDD,NULL,NULL,(XtPointer)HelpManager},
    {True, BUT_TOC_DDD,		MNEM_TOC_DDD,	NULL,NULL,(XtPointer)HelpTOC},
    {True, BUT_HELP_DESK_DDD,	MNEM_HELP_DESK_DDD,NULL,NULL,(XtPointer)HelpDesk},
    {NULL}
};
static MenuGizmo manageHelp = {
	NULL, "sendHelp", NULL, manageHelpItems, HelpCB,
	NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static MenuItems menuItems[] = {
    {True, BUT_FILE,	MNEM_FILE,	(char *) &file,		FileSelectCB},
    {True, BUT_EDIT,	MNEM_EDIT,	(char *) &edit,		EditSelectCB},
    {True, BUT_MESS,	MNEM_MESS,	(char *) &mail,		MailSelectCB},
    {True, BUT_HELP,	MNEM_HELP,	(char *) &manageHelp},
    {NULL}
};

static MenuGizmo manage =     {
	NULL, "manageMenu", NULL, menuItems, 0,
	NULL, CMD, OL_FIXEDROWS, 1, 0
};


static BaseWindowGizmo ManageWindow = {
	&ManagerHelp,
	"ManageWindow",
	" ",
	&manage,
	summaryListArray,
	XtNumber (summaryListArray),
	TXT_MANAGER_ICON_NAME,
	"manmail.icon",
	" ",
	" ",
	50,
};

void
WindowManagerEventHandler(wid, client_data, call_data)
Widget		wid;
XtPointer	client_data;
XtPointer	call_data;
{
	PFV			func = (PFV)client_data;
	OlWMProtocolVerify *	p = (OlWMProtocolVerify *)call_data;

	switch (p->msgtype) {
		case OL_WM_DELETE_WINDOW:
			FPRINTF ((stdout, "Delete yourself\n"));

		case OL_WM_SAVE_YOURSELF:
			/*
			 *	Do nothing for now; just respond.
			 */
			FPRINTF ((stdout, "Save yourself\n"));
			(func) (wid, 0, call_data);
			break;

		default:
			FPRINTF ((stdout, "Default action\n"));
			OlWMProtocolAction(wid, p, OL_DEFAULTACTION);
			break;
	}
}

static void
InitScrollingList (mng)
ManageRec *	mng;
{
	MailRec *		mp = mng->mp;
	char *			cp;
	char			title[BUF_SIZE];
	Widget			list;
	Widget			menuwidget;
	Arg			arg[10];
	static XtCallbackRec	protocolCB[] = {
		{WindowManagerEventHandler, (XtPointer)ManageExitCB},
		{NULL}
	};

	summaryList.list = mp->summary;
	if (firstHalf.font == NULL) {
		firstHalf.font =
		secondHalf.font = GetGizmoText (TXT_LUCY_BOLD);
		summaryList.font = GetGizmoText (TXT_LUCY_MEDIUM);
	}

	mng->baseWGizmo = CopyGizmo (
		BaseWindowGizmoClass, &ManageWindow
	);
	XtSetArg(arg[0], XtNwmProtocol, protocolCB);
	(void)CreateGizmo (
		Root,
		BaseWindowGizmoClass,
		mng->baseWGizmo,
		arg,
		1
	);
	/* Set up callback for drops of the manage window */
	OlDnDRegisterDDI (
		mng->baseWGizmo->icon_shell, OlDnDSitePreviewNone,
		ManageDropNotify, (OlDnDPMNotifyProc)NULL, True,
		(XtPointer)NULL
	);
	OlDnDRegisterDDI (
		mng->baseWGizmo->form, OlDnDSitePreviewNone, ManageDropNotify,
		(OlDnDPMNotifyProc)NULL, True, (XtPointer)NULL
	);
	/* Set the weights of the gizmos in the base window */
	XtVaSetValues (
		QueryGizmo (
			BaseWindowGizmoClass, mng->baseWGizmo,
			GetGizmoWidget, "manageMenu"
		),
		XtNweight,	0,
		(String)0
	);
	XtVaSetValues (
		GetSummaryListWidget (mng),
		XtNweight,		1,
		XtNdropProc,		DropProcCB,
		(String)0
	);
	XtVaSetValues (
		QueryGizmo (
			BaseWindowGizmoClass, mng->baseWGizmo,
			GetGizmoWidget, "header"
		),
		XtNweight,	0,
		(String)0
	);

	if (mp->numBaseWindows == 1) {
		SelectItem (mp, mp->summary, mp->defaultItem);
		FPRINTF ((stderr, "Default selected item = %d\n", mp->defaultItem));
	}

	list = GetSummaryListWidget (mng);

	XtVaSetValues (
		list,
		XtNnumItems,		mp->summary->size,
		XtNitems,		mp->summary->items,
		XtNviewItemIndex,	GetLastSelectedItem (mp),
		XtNuserData,		mp->summary,
		(String)0
	);

	/* Display the number of new messages */

	UpdateFooter (mp);

	/* Set the title of the manage base window */

	cp = GetGizmoText (TXT_ELECTRONIC_MAIL);
	sprintf (title, cp, "");
	if (strcmp (GetUserMailFile (), mp->filename) != 0 &&
	    strcmp (mp->filename, DUMMY_FILE) != 0) {
		sprintf (title, cp, mp->filename);
	}
	XtVaSetValues (
		GetBaseWindowShell (mng->baseWGizmo),
		XtNtitle,			title,
		XtNwmProtocolInterested,	OL_WM_DELETE_WINDOW,
		(String)0
	);

	/* Suck up the adjust event */
	OlAddCallback (
		list,
		XtNconsumeEvent,
		(XtCallbackProc)LookForAdjustCB,
		(XtPointer)mp->summary
	);
}

void
CreateManageRec (mp)
MailRec *	mp;
{
	ManageRec *		mng;
	char *			cp;

	mp->numBaseWindows += 1;
	mng = (ManageRec *)MALLOC (sizeof(ManageRec));
	mng->next = manageRec;
	manageRec = mng;

	mng->openUserPopup = (FileGizmo *)0;
	mng->saveAsPopup = (FileGizmo *)0;
	mng->deleteListPopup = (PopupGizmo *)0;
	mng->lastOp = UndoOp;
	mng->previousDeletes = NULL;
	mng->mp = mp;		/* Point to the appropriate mailx */
	mp->mng = mng;
	InitScrollingList (mng);

	XtAddCallback (
		GetBaseWindowShell (mng->baseWGizmo),
		XtNdestroyCallback, ClientDestroyCB, mng->baseWGizmo
	);

	MapGizmo (BaseWindowGizmoClass, mng->baseWGizmo);
	mp->mng->mapped = True;

	if (mp->noMail == True) {
		cp = GetGizmoText (TXT_NO_MAIL);
		DisplayInLeftFooter (mng->baseWGizmo, cp, True);
		DisplayInLeftFooter (&MainWindow, cp, False);
	}
}

void
DeleteManageRec (mng)
ManageRec *	mng;
{
	ManageRec *	tp;
	ManageRec *	last = (ManageRec *)0;

	FPRINTF ((stderr, "In DeleteManageRec\n"));
	if (mng == (ManageRec *)0) {
		FPRINTF ((stderr, "Returning because mng == 0\n"));
		return;
	}
	for (tp=manageRec; tp!=(ManageRec *)0; tp=tp->next) {
		if (mng == tp) {
			if (last != (ManageRec *)0) {
				last->next = mng->next;
			}
			else {
				manageRec = mng->next;
			}
			break;
		}
		last = tp;
	}
	if (mng->mp != (MailRec *)0) {
		mng->mp->mng = (ManageRec *)0;
	}
	if (mng->mp->summary->size != 0) {
		XtDestroyWidget (GetBaseWindowShell (mng->baseWGizmo));
	}
	FREEGIZMO (FileGizmoClass, mng->openUserPopup);
	FREEGIZMO (PopupGizmoClass, mng->deleteListPopup);
	FREEGIZMO (FileGizmoClass, mng->saveAsPopup);
	FREE (mng);
	mng = NULL;
}
