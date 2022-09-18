/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtmail:delete.c	1.31"
#endif

#define DELETE_C

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/FList.h>
#include "mail.h"
#include "RMailGizmo.h"
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/LabelGizmo.h>
#include <string.h>
#include <stddef.h>

extern HelpInfo		ManagerUndeleteHelp;
extern ReadRec *	readRec;
extern Widget		Root;
extern ManageRec *	manageRec;
extern void		ExecuteCB();
extern void		SelectCB();
extern void		UnselectCB();

static void		SecondUndeleteCB();

void
ResetUndo (mp)
MailRec *mp;
{
	ReadRec *	rp;

	if (mp->mng != (ManageRec *)0) {
		mp->mng->lastOp = UndoOp;
	}
	for (rp=readRec; rp!=(ReadRec *)0; rp=rp->next) {
		if (rp->mp == mp) {
			rp->lastOp = UndoIt;
		}
	}
}

/*
 * uses strpbrk and strspn to break string into tokens on
 * sequentially subsequent calls.  returns NULL when no
 * non-separator characters remain.
 * `subsequent' calls are calls with first argument NULL.
 */

char *
MyStrtok(string, sepset, savept)
register char *	string;
const char *	sepset;
char **		savept;
{
	register char	*q, *r;

	/*first or subsequent call*/
	if (string == NULL)
		string = *savept;

	if(string == 0)		/* return if no tokens remaining */
		return(NULL);

	q = string + strspn(string, sepset);	/* skip leading separators */

	if(*q == '\0')		/* return if no tokens remaining */
		return(NULL);

	if((r = strpbrk(q, sepset)) == NULL)	/* move past token */
		*savept = 0;	/* indicate this is last token */
	else {
		*r = '\0';
		*savept = r+1;
	}
	return(q);
}

Widget
GetDeletedListWidget (mng)
ManageRec *	mng;
{
	if (mng != (ManageRec *)0 && mng->deleteListPopup != (PopupGizmo *)0) {
		return (Widget) QueryGizmo (
			PopupGizmoClass, mng->deleteListPopup,
			GetGizmoWidget, DELETE_LIST
		);
	}
	return (Widget)0;
}

static ListGizmo *
GetDeletedListGizmo (mng)
ManageRec *	mng;
{
	return (ListGizmo *) QueryGizmo (
		PopupGizmoClass, mng->deleteListPopup,
		GetGizmoGizmo, DELETE_LIST
	);
}

void
UpdateLists(mng)
ManageRec *	mng;
{
	if (mng != (ManageRec *)0) {
		if (mng != (ManageRec *)0) {
			UpdateList (GetSummaryListGizmo (mng));
		}
		if (mng->deleteListPopup != (PopupGizmo *)0) {
			UpdateList (GetDeletedListGizmo (mng));
		}
	}
}

/*
 * MoveListItem
 *
 * Move a given item in a list from one list to another.
 * This isn't a generic routine and should only be used with
 * summary and delete lists.  This is due to the fact that the
 * routine has exact knowledge of the contents of the fields.
 */

static void
MoveListItem (src, dst, sitem, ditem)
ListHead *	src;
ListHead *	dst;
int		sitem;		/* Item to be moved out of the source list */
int		ditem;		/* Destination item number */
{
	int	i;

	/* Make room in the destination for the new item */

	if ((i = dst->size++) == 0) {
		dst->items = (ListItem *)MALLOC(sizeof (ListItem) * 1);
	}
	else {
		dst->items = (ListItem *)REALLOC(
			dst->items, sizeof (ListItem) * dst->size
		);
		for (; i>ditem; i--) {
			dst->items[i].set = dst->items[i-1].set;
			dst->items[i].fields = dst->items[i-1].fields;
			dst->items[i].clientData = dst->items[i-1].clientData;
		}
	}

	/* Move the item from source to destination */

	dst->items[ditem].set = src->items[sitem].set;
	dst->items[ditem].fields = src->items[sitem].fields;
	dst->items[ditem].clientData = src->items[sitem].clientData;

	/* Close up the space made by the move */

	src->size -= 1;
	if (src->size == 0) {
		FREE (src->items);
		src->items = (ListItem *)0;
	}
	else {
		for (i=sitem; i<src->size; i++) {
			src->items[i].set = src->items[i+1].set;
			src->items[i].fields = src->items[i+1].fields;
			src->items[i].clientData = src->items[i+1].clientData;
		}
		src->items = (ListItem *)REALLOC (
			src->items, sizeof (ListItem) * src->size
		);
	}
}

static void
MoveDeletedItem (mp, item)
MailRec *mp;
int item;
{
	int		i;
	int		ditem;
	int		sitem;
	char **		tmp;
	ReadRec *	rp;

	/* Move the item from the summary list to the deleted list */

	/* Need to compare message numbers, not list indexes */
	sitem = (int)mp->summary->items[item].clientData;
	tmp = (char **)mp->summary->items[item].fields;
	SetStatusField ("D", tmp);

	for (i=0; i<mp->deleted->size; i++) {
		ditem = (int)mp->deleted->items[i].clientData;
		if (sitem < ditem) {
			break;
		}
	}
	MoveListItem (
		mp->summary, mp->deleted, item, i
	);
}

static void
ShowDeletedLimitsCB (wid, client_data, ep)
Widget				wid;
XtArgVal			client_data;
OlFListItemsLimitExceededCD *	ep;
{
	MailRec *	mp = FindMailRec (wid);
	char		buf[BUF_SIZE];

	ep->ok = True;
	sprintf (buf, GetGizmoText (TXT_DELETE_LM_EXCEEDED), ep->preferred);
	if (mp->mng->deleteListPopup != (PopupGizmo *)0) {
		SetPopupMessage (mp->mng->deleteListPopup, buf);
	}
}

static XtCallbackRec	DeletedLimitsCB[] = {
	{(XtCallbackProc)ShowDeletedLimitsCB,	(XtPointer)NULL},
	{NULL}
};

static Setting deleteListSetting = {
	"None",
	(XtPointer)NULL,
	(XtPointer)NULL,
	(XtPointer)"x"
};

static ListGizmo deleteList = {
	NULL, DELETE_LIST, "", &deleteListSetting,
	FORMAT, False, 5, 
	NULL, NULL, ExecuteCB, SelectCB, UnselectCB, DeletedLimitsCB
};

extern LabelGizmo HeaderLine;

static GizmoRec deleteListArray[] = {
	{LabelGizmoClass,	&HeaderLine},
	{ListGizmoClass,	&deleteList},
};

static MenuItems undeleteItems[] = {
    {True,BUT_UNDELETE,	MNEM_UNDELETE,	NULL,	SecondUndeleteCB},
    {True,BUT_CANCEL,	MNEM_CANCEL,	NULL,	CancelCB},
    {True,BUT_HELP,	MNEM_HELP,	NULL,	HelpCB,(XtPointer)HelpManagerUndelete},
    {NULL}
};

static MenuGizmo undelete =     {
	NULL, "undelete", NULL, undeleteItems, 0,
	NULL, CMD, OL_FIXEDROWS, 1, 0
};

PopupGizmo DeleteListPopup = {
	&ManagerUndeleteHelp,
	"delete list",
	TXT_MAIL_MNG_DELETE,
	&undelete,
	deleteListArray,
	XtNumber (deleteListArray),
};

void
UndeleteProc (mng)
ManageRec *	mng;
{
	MailRec *	mp = mng->mp;
	int		i;
	Widget		list;

	if (mng->deleteListPopup == (PopupGizmo *)0) {
		deleteList.list = mp->deleted;
		deleteList.font = GetGizmoText (TXT_LUCY_MEDIUM);
		CreateDeleteListPopup (mng);

		/* Setup for the adjust button such that adjust after
		 * select toggles additional items
		 */
		list = GetDeletedListWidget (mng);
		OlAddCallback (
			list, XtNconsumeEvent,
			(XtCallbackProc)LookForAdjustCB, (XtPointer)mp->deleted
		);
		XtVaSetValues (
			list,
			XtNuserData,	mp->deleted,
			(String)0
		);
	}

	MapGizmo (PopupGizmoClass, mng->deleteListPopup);
}

static void
UndeleteMessages (mp, items)
MailRec *	mp;
char *		items;
{
	ReadRec *	rp;
	char		buf[265];
	int		i;
	int		j;
	int		ditem;
	int		sitem;

	if (items[0] != '\0') {
		if (mp->mng->deleteListPopup != (PopupGizmo *)0) {
			SetPopupMessage (mp->mng->deleteListPopup, " ");
		}
		(void)ProcessCommand (mp, UNDELETE_CMD, items);

		for (j=mp->deleted->size-1; j>=0; j--) {
			if (mp->deleted->items[j].set == True) {
				/* Move the item from the deleted
				 * list to the summary list.
				 */
				/* Need to compare message numbers,
				 * not list indexes
				 */
				sitem = (int)mp->deleted->items[j].clientData;

				for (i=0; i<mp->summary->size; i++) {
					ditem =
					(int)mp->summary->items[i].clientData;
					if (sitem < ditem) {
						break;
					}
				}
				MoveListItem (mp->deleted, mp->summary, j, i);
				mp->summary->clientData += (XtArgVal)1;
				mp->deleted->clientData -= (XtArgVal)1;
				UpdateLists (mp->mng);
				/*
				 * If there is an empty reader up then 
				 * display this message in that reader
				 */
				for (rp=readRec;rp!=(ReadRec *)0;rp=rp->next) {
					if (rp->mp == mp) {
						if (rp->message == -1) {
							ReadItem (mp, rp, i);
							break;
						}
					}
				}
			}
		}
		DisplayStatus (mp, ProcessCommand (mp, FROM_CMD, items));
		UpdateFooter (mp);
	}
}

void
ManageUndelete (mng)
ManageRec *	mng;
{
	MailRec *	mp = mng->mp;
	int		i;
	int		j;
	char *		items;
	ReadRec *	rp;
	char *		cp;
	char *		msp;

	/*
	 * Remember what items need to be updated and do the update
	 * after all items have been undeleted and moved.
	 */
	/* We don't use a compact list here because we may
	 * have a situation were the delete list contains on 
	 * items 1 and 5000.  A compact list would be "1-5000".
	 * This would update evenything between 1 and 5000 in
	 * DisplayStatus() below.
	 */
	items = ListOfSelectedItems (
		mp, mp->deleted, NULL, 0, mp->deleted->size
	);

	UndeleteMessages (mp, items);
	if (mng->deleteListPopup != (PopupGizmo *)0) {
		BringDownPopup (GetPopupGizmoShell (mng->deleteListPopup));
	}
}

static void
SecondUndeleteCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	MailRec *	mp = FindMailRec (wid);

	ManageUndelete (mp->mng);
}

/* Handle the deletion of the read message.
 * In this case we don't really delete the message, but
 * update it to the next readable message.
 */
static int
HandleReadDelete (mp, rp)
MailRec *	mp;
ReadRec *	rp;
{
	ReadRec *	nrp;
	char		buf[BUF_SIZE];
	ReadMailGizmo *	gizmo;

	rp->lastMessage = rp->message;
	/* Switch last read window to next message or blank it */
	/*
	 * If there are any other read windows up for this
	 * mailer then delete this window
	 */
	for (nrp=readRec; nrp!=(ReadRec *)0; nrp=nrp->next) {
		if (nrp->mp == mp && nrp != rp) {
			DeleteReadRec (rp);
			return -1;
		}
	}
	if (Next (rp) == False && Prev (rp) == False) {
		/* Clear this read window */
		gizmo = (ReadMailGizmo *)QueryGizmo (
			BaseWindowGizmoClass, rp->baseWGizmo,
			GetGizmoGizmo, READ_MAIL
		);
		DisplayMailText (gizmo, "", "", OL_STRING_SOURCE);
		sprintf (
			buf, GetGizmoText (TXT_NO_MORE_MESSAGES),
			rp->mp->filename
		);
		DisplayInLeftFooter (rp->baseWGizmo, buf, True);
		if (strcmp (GetUserMailFile (), mp->filename) == 0) {
			sprintf (buf, "%s", GetGizmoText (TXT_EMAIL_READER));
		}
		else {
			sprintf (
				buf,
				"%s %s",
				GetGizmoText (TXT_EMAIL_READER),
				mp->filename
			);
		}
		SetBaseWindowTitle (rp->baseWGizmo, buf);
		rp->message = -1;
	}
	return rp->message;
}

void
ReadDelete (rp)
ReadRec *	rp;
{
	int		next;
	int		this;
	int		i;
	char *		cp;
	char *		msp;
	char *		savept;
	MailRec *	mp = rp->mp;
	char		message[BUF_SIZE];

	/* Remember what message was last deleted */
	this = rp->message;

	/* Tell mailx that the selected item has been deleted */
	sprintf (message, "%d ", this);

	/* Deal with deleting the read window(s). */
	next = HandleReadDelete (mp, rp);
	/*
	 * Tell mailx that the selected message have been deleted
	 */
	(void)ProcessCommand (mp, DELETE_CMD, message);
      	/*
      	 * Unselect all the items so only the new item moved
      	 * into the delete list is selected.
      	 */
      	UnselectAll (mp->mng, mp->deleted, -1);
	/*
	 * Move this item to the delete list
	 */
	i = ItemNumber (mp, this);
	MoveDeletedItem (mp, i);
	mp->deleted->clientData += (XtArgVal)1;
	mp->summary->clientData -= (XtArgVal)1;
	/*
	 * LastSelectedMessage has to be reset because the above select
	 * changed its value
	 */
	if (next != -1) {
		SelectItem (mp, mp->summary, ItemNumber (mp, next));
	}
	UpdateLists (mp->mng);
	/*
	 * Select this item only in the delete list
	 */
	SelectItem (mp, mp->deleted, ItemNumber (mp, this));
	UpdateFooter (mp);
}

void
ManageDelete (mp)
MailRec *	mp;
{
	char *		items;
	int		i;
	ReadRec *	rp;
	ManageRec *	mng = mp->mng;

	/*
	 * Remember what messages were deleted
	 */
	items = ListOfSelectedItems (
		mp, mp->summary, NULL, 0, mp->summary->size
	);
	mng->lastOp = DeleteOp;

	/*
	 * For Undo, remember what read windows were deleted
	 */
	FREENULL (mng->previousDeletes);
	mng->previousDeletes = STRDUP (ListOfSelectedReadMessages (mp));

	/* Tell mailx that the selected messages have been deleted */
	(void)ProcessCommand (mp, DELETE_CMD, items);
	UnselectAll (mng, mp->deleted, -1);

	for (i=mp->summary->size-1; i>-1; i--) {
		if (mp->summary->items[i].set == True) {
			rp = FindMessageInList (mp, MessageNumber (mp, i));
			DeleteReadRec (rp);
			MoveDeletedItem (mp, i);
			mp->deleted->clientData += (XtArgVal)1;
			mp->summary->clientData -= (XtArgVal)1;
		}
	}
	UpdateLists (mng);
	UpdateFooter (mp);
}

void
ReadUndo (rp)
ReadRec *	rp;
{
	MailRec *	mp = rp->mp;
	char		item[BUF_SIZE];

	ResetUndo (mp);
	UnselectAll (mp->mng, mp->summary, -1);
	/*
	 * Go move the messages from the delete list to the summary list
	 */
	sprintf (item, "%d ", rp->lastMessage);
	UndeleteMessages (mp, item);

	ReadItem (mp, rp, ItemNumber (mp, rp->lastMessage));
	/*
	 * Remove any possible "No more messages" message
	 */
	DisplayInLeftFooter (rp->baseWGizmo, " ", True);
}

void
ManageUndo (mng)
ManageRec *	mng;
{
	MailRec *	mp = mng->mp;
	int		i;
	char *		items;
	ReadRec *	rp;
	char *		cp;
	char *		msp;
	char *		savept;

	/*
	 * Make sure all read windows can't do another undo
	 */
	ResetUndo (mp);
	/*
	 * Go move the messages from the delete list to the summary list
	 */
	items = ListOfSelectedItems (
		mp, mp->deleted, NULL, 0, mp->deleted->size
	);
	UndeleteMessages (mp, items);
	/*
	 * Now redisplay the correct read windows
	 */
	msp = mng->previousDeletes;

	*(msp + strlen (msp) - 1) = '\0';
	while ((cp = MyStrtok (msp, " ", &savept)) != NULL) {
		msp = NULL;
		i = atoi (cp);
		rp = CreateReadRec (mp);
		ReadItem (mp, rp, ItemNumber (mp, i));
	}
}
