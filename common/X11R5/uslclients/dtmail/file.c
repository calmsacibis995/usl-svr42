/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtmail:file.c	1.43"
#endif

#define FILE_C

#include <string.h>
#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/ScrolledWi.h>
#include <Xol/TextEdit.h>
#include <Xol/TextField.h>
#include <Xol/Error.h>
#include <Xol/Form.h>
#include <Xol/StaticText.h>
#include <Xol/OlCursors.h>	/* For OlGetBusyCursor */
#include "mail.h"
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/InputGizmo.h>
#include <Gizmo/BaseWGizmo.h>

extern BaseWindowGizmo	MainWindow;
extern int		Version;
extern char *		Mbox;
extern MailRec *	mailRec;
extern char *		GlobalBuf;
extern o_ino_t		DummyDir;

MenuItems ErrorItems[] = {
    {True,	BUT_OK,	MNEM_OK,	NUL,	CancelCB},
    {NULL}
};

static MenuGizmo errorMenu = {
	NULL,			/* help		*/
	"",			/* name		*/
	NULL,			/* title	*/
	ErrorItems,		/* items	*/
	0,			/* function	*/
	NULL,			/* client_data	*/
	CMD,			/* buttonType	*/
	OL_FIXEDROWS,		/* layoutType	*/
	1,			/* measure	*/
	0			/* default Item	*/
};

void
DisplayErrorPrompt (Widget shell, char *buf)
{
	FPRINTF ((stderr, "error message = %s\n", buf));
	(void)DefaultModalGizmo (
		shell, &errorMenu, buf
	);
}

/*
 * Add the suffix .ml to the given text string.
 * Just return the text string if the file already
 * contains .ml.
 */

char *
AddDotMl (char *text)
{
	int	i;
	char *	buf;
	char *	filename;
	char *	cp;

	/* If there is already a .ml on the end of the name
	 * then just return the name.
	 */
	if ((cp = strrchr (text, '.')) != NULL) {
		if (strcmp (cp, GetGizmoText (TXT_DOTML)) == 0) {
			return STRDUP (text);
		}
	}

	/* Separate the name into directory and basename
	 */

	if ((filename = strrchr (text, '/')) == NULL) {
		filename = text;
		text = NULL;
	}
	else {
		*filename = '\0';
		filename += 1;
	}

	/* Add .ml to the end of the filename
	 */
	i = strlen (text);
	buf = (char *)MALLOC (i + 16);
	buf[0] = '\0';
	if (i != 0) {
		strcpy (buf, text);
		strcat (buf, "/");
	}
	strncat (buf, filename, 11);
	buf[i+1+11] = '\0';
	strcat (buf, GetGizmoText (TXT_DOTML));
	if (strchr (buf, ' ') != NULL) {
		FREE (buf);
		/* Filenames can't contain blanks */
		return NULL;
	}
	return buf;
}

/*
 * Display the output from a save command in the left footer of the 
 * manage window.
 */

Boolean
DisplaySaveStatus (bw, buf)
BaseWindowGizmo *       bw;
char *			buf;
{
	char *		reason;
	static char *	regx1 = NULL;
	static char *	regx2;
	static char *	regx3;
	static char *	regx4;
	char		filename[BUF_SIZE];
	char		text[BUF_SIZE];
	char *		cp;

	/* When doing a save the message that comes back
	 * should contain either [New file] of [Appended].
	 * Otherwize, there was an error and a notice
	 * should be displayed.
	 */

	if (regx1 == NULL) {
		regx1 = (char *) regcmp (
			"\"([^ ]+)$0\" \\[Appended\\] [0-9]+/[0-9]+",
			0
		);
		regx2 = (char *) regcmp (
			"\"([^ ]+)$0\" \\[New file\\] [0-9]+/[0-9]+",
			0
		);
		regx3 = (char *) regcmp (
			"\"([^ ]+)$0\" \\[Appended\\] binary/[0-9]+",
			0
		);
		regx4 = (char *) regcmp (
			"\"([^ ]+)$0\" \\[New file\\] binary/[0-9]+",
			0
		);
	}
	if ((cp = (char *) regex (regx1, buf, filename)) != NULL ||
	    (cp = (char *) regex (regx2, buf, filename)) != NULL ||
	    (cp = (char *) regex (regx3, buf, filename)) != NULL ||
	    (cp = (char *) regex (regx4, buf, filename)) != NULL) {
		sprintf (text, GetGizmoText (TXT_MESSAGE_SAVED), filename);
		DisplayInLeftFooter (bw, text, True);
		return True;
	}
	else {
		/* Can't find either "New file" or "Appended"
		 * so this must be an error.
		 */
		if (Version == 41) {
			reason = strrchr (buf, ':')+1;
		}
		else {
			reason = strrchr (buf, '"')+1;
		}
		*reason = '\0';
		reason += 1;
		/* Pluck out the filename from the start of the
		 * error message.
		 */
		strcpy (filename, buf+1);
		cp = strchr (filename, '"');
		*cp = '\0';
		cp = GetErrorText (
			0, reason, TXT_CANT_SAVE_FILE, filename
		);
		DisplayErrorPrompt (GetBaseWindowShell (bw), cp);

		return False;
	}
}

void
ManageSaveSaveAsCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	ManageRec *	mng = FindManageRec (wid);
	MailRec *	mp = mng->mp;
	char *		filename;
	char *		ml;
	char *		buf;
	int		n;
	int		flag;

	n = ExpandFileGizmoFilename(mng->saveAsPopup, &flag);
	if ((flag != 0 && n == 1) || n == 0) {
		filename = GetFilePath (mng->saveAsPopup);
	}
	else {
		return;
	}
	ml = AddDotMl (filename);
	FREE (filename);
	if (ml == NULL) {
		SetFileGizmoMessage (mng->saveAsPopup, TXT_INVALID_FILENAME);
		return;
	}
	buf = CompactListOfSelectedItems (
		mp, mp->summary, ml, 0, mp->summary->size
	);
	if (buf[0] != '\0') {
		if (
			DisplaySaveStatus (
				mng->baseWGizmo,
				ProcessCommand (mp, SAVE_CMD, buf)
			) == True
		) {
			BringDownPopup (GetFileGizmoShell (mng->saveAsPopup));
			UpdateStatusOfMessage (mp, 0, mp->summary->size);
		}
		FREE (ml);
	}
}

void
ManageSaveAs (mng)
ManageRec *	mng;
{

	if (mng->saveAsPopup == (FileGizmo *)0) {
		CreateManageSaveAsPopup (mng);
	}
	MapGizmo (FileGizmoClass, mng->saveAsPopup);
}

void
ManageSave (mng)
ManageRec *	mng;
{
	MailRec *	mp = mng->mp;
	char *		buf;

	buf = CompactListOfSelectedItems (
		mp, mp->summary, Mbox, 0, mp->summary->size
	);

	if (buf[0] != '\0') {
		if (DisplaySaveStatus (
				mng->baseWGizmo,
				ProcessCommand (mp, SAVE_CMD, buf)
			) == True) {
			UpdateStatusOfMessage (mp, 0, mp->summary->size);
		}
	}
}

void
QuitMailx (mp)
MailRec *	mp;
{
	/* Tell mailx to quit */
	WriteToMailx (mp, QUIT_SAVE, strlen (QUIT_SAVE));
	p3close (mp->fp);
	DeleteMailRec (mp);
}

void
UnmapShell (sh)
Widget	sh;
{
	if ((GetWMState (XtDisplay(sh), XtWindow(sh))) == IconicState) {
		if (XWithdrawWindow (
			XtDisplay (sh), XtWindow (sh),
			XScreenNumberOfScreen (XtScreen(sh))
		)) {
			XSync (XtDisplay(sh), False);
		}
	}
	else {
		XtUnmapWidget (sh);
	}
}

void
ExitManager (mng)
ManageRec *	mng;
{
	/* Bring all popups down */
	if (mng == (ManageRec *)0) {
		return;
	}
	if (mng->openUserPopup != (FileGizmo *)0) {
		XtPopdown (GetFileGizmoShell (mng->openUserPopup));
	}
	if (mng->deleteListPopup != (PopupGizmo *)0) {
		XtPopdown (GetPopupGizmoShell (mng->deleteListPopup));
	}
	if (mng->saveAsPopup != (FileGizmo *)0) {
		XtPopdown (GetFileGizmoShell (mng->saveAsPopup));
	}

	UnmapShell (GetBaseWindowShell (mng->baseWGizmo));
	mng->mapped = False;

	/* Don't do anything else if there are read window up */
	if (--mng->mp->numBaseWindows != 0) {
		return;
	}

	/* If this isn't the only mailx open */
	if (mailRec->next != (MailRec *)0) {
		/* then exit it */
		QuitMailx (mng->mp);
	}
	else {
		/* else, keep it running but switch its input file to "/" */
		(void)SwitchMailx (mng->mp, DUMMY_FILE, mng->baseWGizmo);
		DeleteManageRec (mng);
	}
}

void
ManageExitCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	ManageRec *	mng = FindManageRec(wid);

	ExitManager (mng);
}

void
FreeSummaryOrDeleteList (wid, lp)
Widget		wid;
ListHead *	lp;
{
	int	i;
	int	j;
	char **	tmp;

	if (wid != (Widget)0) {
		XtVaSetValues (
			wid,
			XtNnumItems,		0,
			XtNitems,		NULL,
			(String)0
		);
	}
	if (lp->size > 0) {
		for (i=0; i<lp->size; i++) {
			tmp = (char **)lp->items[i].fields;
			/* Don't free the first item because it is a pixmap */
			for (j=1; j<lp->numFields; j++) {
				FREE (tmp[j]);
			}
			FREE ((char *)lp->items[i].fields);
		}
		FREE (lp->items);
	}
	lp->size = 0;
}

void
RaiseManageWindow (mp)
MailRec *	mp;
{
	Widget		shell;

	if (mp->mng == (ManageRec *)0) {
		CreateManageRec (mp);
		return;
	}
	shell = GetBaseWindowShell (mp->mng->baseWGizmo);
	if (mp->mng->mapped == False) {
		mp->numBaseWindows += 1;
		mp->mng->mapped = True;
		MapGizmo (BaseWindowGizmoClass, mp->mng->baseWGizmo);
	}
	else {
		DisplayInLeftFooter (
			mp->mng->baseWGizmo,
			GetGizmoText (TXT_MNG_ALREADY_ACTIVE),
			True
		);
	}
	XRaiseWindow (XtDisplay (shell), XtWindow (shell));
}

MailRec *
OpenNewMailFile (filename, bw)
char *			filename;
BaseWindowGizmo *	bw;
{
	MailRec *	mp = mailRec;
	o_ino_t		inode;
	Widget		shell;

	shell = GetBaseWindowShell (bw);
	DtLockCursor (shell, 1000L, NULL, NULL, OlGetBusyCursor(shell));

	/* First look and see if the default file is open.
	 * If so, use that mailx.
	 */
	if (mp->inode == DummyDir) {
		/* If this mailx is open to the default mailer */
		/* then use it. */
		if (SwitchMailx (mp, filename, bw) == True) {
			CreateManageRec (mp);
		}
		return mp;
	}

	/* Second, look to see if there is a mailRec with
	 * this filename already open.
	 */

	(void)StatFile(filename, &inode, (off_t *)0);
	if ((mp = GetMailx (inode)) != (MailRec *)0) {
		/* Raise the window */
		RaiseManageWindow (mp);
		return mp;
	}
	/*
	 * Otherwise, open a new mailx and create a new manage window.
	 */
	mp = OpenMailx ();
	if (SwitchMailx (mp, filename, bw) == True) {
		CreateManageRec (mp);
		return mp;
	}
	QuitMailx (mp);
	return (MailRec *)0;
}

void
DisplayAlreadyOpen (bw, filename)
BaseWindowGizmo *	bw;
char *			filename;
{
	char	buf[BUF_SIZE];

	sprintf (buf, GetGizmoText (TXT_FILE_ALREADY_OPEN), filename);
	DisplayInLeftFooter (bw, buf, True);
}

void
OpenCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	MailRec *		mp = FindMailRec (wid);
	MailRec *		tmp;
	char *			filename;
	char *			fp;
	int			n;
	int			flag;

	n = ExpandFileGizmoFilename(mp->mng->openUserPopup, &flag);
	if (n > 1) {
		return;
	}
	if (n == 0) {
		fp = GetFilePath (mp->mng->openUserPopup);
		filename = AddDotMl (fp);
		FREE (fp);
	}
	else {
		filename = GetFilePath (mp->mng->openUserPopup);
	}

	tmp = OpenNewMailFile (filename, mp->mng->baseWGizmo);
	if (tmp != (MailRec *)0) {
		BringDownPopup (GetFileGizmoShell(mp->mng->openUserPopup));
	}
	FREE (filename);
}

void
OpenUser (mng)
ManageRec *	mng;
{
	if (mng->openUserPopup == (FileGizmo *)0) {
		CreateOpenUserPopup (mng);
	}
	MapGizmo (FileGizmoClass, mng->openUserPopup);
}
