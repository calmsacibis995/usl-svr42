/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtmail:popups.c	1.40"
#endif

#define POPUPS_C

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>			/* need this for XtNtitle */
#include <Xol/OpenLook.h>
#include "mail.h"
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/InputGizmo.h>

extern int		Version;
extern HelpInfo		ManagerOpenHelp;
extern HelpInfo		ManagerSaveAsHelp;
extern HelpInfo		ReaderOpenHelp;
extern HelpInfo		ReaderSaveAsHelp;
extern HelpInfo		SenderOpenHelp;
extern HelpInfo		SenderSaveAsHelp;
extern char *		Mbox;
extern char *		ApplicationName;
extern String *		ItemResources;

static char *	ErrorText;	/* Used to pass transmission errors */
static ModalGizmo defaultModal = {
    NULL,
    "no name",
    TXT_JUST_MAIL,
    NULL,
    NULL,
};

static void
PopdownCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	Widget	shell = (Widget)client_data;

	XtDestroyWidget (shell);
}

ModalGizmo *
DefaultModalGizmo (parent, menu, message)
Widget		parent;
MenuGizmo *	menu;
char *		message;
{
	static ModalGizmo *	gp = (ModalGizmo *)0;
	Widget			shell;

	defaultModal.menu = menu;
	defaultModal.menu->parent = (Widget)0;
	gp = &defaultModal;
	shell = CreateGizmo (parent, ModalGizmoClass, gp, 0, 0);
	XtAddCallback (
		GetModalGizmoShell(gp), XtNpopdownCallback, PopdownCB, shell
	);
	SetModalGizmoMessage (gp, message);
	XtMapWidget (parent);
	MapGizmo (ModalGizmoClass, gp);
	return gp;
}

char *
GetCurrentDirectory ()
{
	char		buf[BUF_SIZE];
	char *		cp;
	static char *	dir = NULL;

	if (dir == NULL) {
		strcpy (buf, Mbox);
		cp = strrchr (buf, '/');
		/* Check to make sure that Mbox had an embedded '/'.
		 * Otherwise, just use Mbox as is.
		 */
		dir = NULL;
		if (cp != NULL) {
			*cp = '\0';
			if (buf[0] != '\0') {
				dir = STRDUP (buf);
			}
		}
		if (dir == NULL) {
			dir = STRDUP (Mbox);
		}
	}
	return dir;
}

extern MenuGizmo SendOpen;

static FileGizmo SendOpenPopup = {
	&SenderOpenHelp,
	"openSend",
	TXT_OPEN_OUTGOING,
	&SendOpen,
	"", "", "", FOLDERS_AND_FILES
};

void
CreateSendOpenPopup (sp)
SendRec *	sp;
{
	SendOpenPopup.directory = GetCurrentDirectory ();
	sp->openPopup = CopyGizmo (
		FileGizmoClass,
		&SendOpenPopup
	);
	CreateGizmo (
		GetBaseWindowShell (sp->baseWGizmo),
		FileGizmoClass,
		sp->openPopup,
		0, 0
	);
}

extern MenuGizmo SaveReply;

static FileGizmo SaveReplyAsPopup = {
	&SenderSaveAsHelp,
	"saveReplyAs",
	TXT_SAVE_OUTGOING,
	&SaveReply,
	"", "", "", FOLDERS_ONLY
};

void
CreateSaveReplyAsPopup (sp)
SendRec *	sp;
{
	SaveReplyAsPopup.directory = GetCurrentDirectory ();
	sp->saveAsPopup = CopyGizmo (
		FileGizmoClass,
		&SaveReplyAsPopup
	);
	CreateGizmo (
		GetBaseWindowShell (sp->baseWGizmo),
		FileGizmoClass,
		sp->saveAsPopup,
		0, 0
	);
}

/* SaveAs MenuGizmo info */

extern MenuGizmo Save;

static FileGizmo ManageSaveAsPopup = {
	&ManagerSaveAsHelp,
	"manageSaveAs",
	TXT_SAVE_MANAGE_MAIL,
	&Save,
	"", "", "", FOLDERS_ONLY
};

void
CreateManageSaveAsPopup (mng)
ManageRec *	mng;
{
	extern MenuItems *	saveItems;

	ManageSaveAsPopup.directory = GetCurrentDirectory ();
	SetSaveFunction (ManageSaveSaveAsCB);
	mng->saveAsPopup = CopyGizmo (
		FileGizmoClass,
		&ManageSaveAsPopup
	);
	CreateGizmo (
		GetBaseWindowShell (mng->baseWGizmo),
		FileGizmoClass,
		mng->saveAsPopup,
		0, 0
	);
}

static FileGizmo ReadSaveAsPopup = {
	&ReaderSaveAsHelp,
	"readSaveAs",
	TXT_SAVE_READ_MAIL,
	&Save,
	"", "", "", FOLDERS_ONLY
};

void
CreateReadSaveAsPopup (rp)
ReadRec *	rp;
{

	ReadSaveAsPopup.directory = GetCurrentDirectory ();
	SetSaveFunction (ReadSaveSaveAsCB);
	rp->readSaveAsPopup = CopyGizmo (
		FileGizmoClass,
		&ReadSaveAsPopup
	);
	CreateGizmo (
		GetBaseWindowShell (rp->baseWGizmo),
		FileGizmoClass,
		rp->readSaveAsPopup,
		0, 0
	);
}

extern MenuGizmo OverWrite;

void
CreateFileExistsModal (parent)
Widget		parent;
{
	(void)DefaultModalGizmo (
		parent, &OverWrite, GetGizmoText (TXT_FILE_OVERWRITE)
	);
}

extern MenuGizmo Open;

static FileGizmo OpenPopup = {
	&ManagerOpenHelp,
	"openUser",
	TXT_OPEN_MANAGER_FILE,
	&Open,
	"",
	"",	/* "*.ml" */
	"",
	FOLDERS_AND_FILES
};

void
CreateOpenUserPopup (mng)
ManageRec *	mng;
{
	OpenPopup.path = GetGizmoText (TXT_STAR_DOT_ML);
	OpenPopup.directory = GetCurrentDirectory ();
	mng->openUserPopup = CopyGizmo (
		FileGizmoClass,
		&OpenPopup
	);
	CreateGizmo (
		GetBaseWindowShell (mng->baseWGizmo),
		FileGizmoClass,
		mng->openUserPopup,
		0, 0
	);
}

extern MenuGizmo ReadOpen;

static Setting dummySetting;

static FileGizmo ReadOpenPopup = {
	&ReaderOpenHelp,
	"readOpenUser",
	TXT_OPEN_READ_FILE,
	&ReadOpen,
	"",			/* message */
	"",			/* path */
	"",			/* directory */
	FOLDERS_AND_FILES
};

void
CreateReadOpenUserPopup (rp)
ReadRec *	rp;
{
	ReadOpenPopup.path = GetGizmoText (TXT_STAR_DOT_ML);
	ReadOpenPopup.directory = GetCurrentDirectory ();
	rp->readOpenUserPopup = CopyGizmo (
		FileGizmoClass,
		&ReadOpenPopup
	);
	CreateGizmo (
		GetBaseWindowShell (rp->baseWGizmo),
		FileGizmoClass,
		rp->readOpenUserPopup,
		0, 0
	);
}

extern PopupGizmo DeleteListPopup;

void
CreateDeleteListPopup (mng)
ManageRec *	mng;
{
	mng->deleteListPopup = CopyGizmo (
		PopupGizmoClass,
		&DeleteListPopup
	);
	CreateGizmo (
		GetBaseWindowShell (mng->baseWGizmo),
		PopupGizmoClass,
		mng->deleteListPopup,
		0, 0
	);
}

static MenuItems noChangeItems[] = {
    {True, BUT_SEND,	MNEM_SEND,	NULL,	SendAgainCB},
    {True, BUT_CANCEL,	MNEM_CANCEL,	NULL,	CancelCB},
    {True, BUT_HELP,	MNEM_HELP,	NULL,	HelpCB, (XtPointer)HelpManagerSaveAs},
    {NULL}
};

static MenuGizmo noChange = {
	NULL,			/* help		*/
	"na",			/* name		*/
	TXT_NO_CHANGE,		/* title	*/
	noChangeItems,		/* items	*/
	0,			/* function	*/
	NULL,			/* client_data	*/
	CMD,			/* buttonType	*/
	OL_FIXEDROWS,		/* layoutType	*/
	1,			/* measure	*/
	1			/* default Item	*/
};

void
CreateNoChangeModal (parent)
Widget		parent;
{
	(void)DefaultModalGizmo (
		parent, &noChange, GetGizmoText (TXT_NO_CHANGE)
	);
}

static MenuItems textChangedItems[] = {
    {True, BUT_EXIT,	MNEM_EXIT_E,	NULL,	NULL},
    {True, BUT_CANCEL,	MNEM_CANCEL,	NULL,	CancelCB},
    {True, BUT_HELP,	MNEM_HELP,	NULL,	HelpCB, (XtPointer)HelpTOC},
    {NULL}
};

static MenuGizmo textChanged = {
	NULL,			/* help		*/
	"na",			/* name		*/
	"na",			/* title	*/
	textChangedItems,	/* items	*/
	0,			/* function	*/
	NULL,			/* client_data	*/
	CMD,			/* buttonType	*/
	OL_FIXEDROWS,		/* layoutType	*/
	1,			/* measure	*/
	1			/* default Item	*/
};

void
CreateTextChangedModal (parent)
Widget		parent;
{
	textChangedItems[0].label = BUT_EXIT;
	textChangedItems[0].mnemonic = MNEM_EXIT_E;
	textChangedItems[0].function = ReallyExitCB;
	(void)DefaultModalGizmo (
		parent, &textChanged, GetGizmoText (TXT_NOT_SENT)
	);
}

void
CreateExitModal (parent)
Widget		parent;
{
	textChangedItems[0].label = BUT_EXIT;
	textChangedItems[0].mnemonic = MNEM_EXIT_E;
	textChangedItems[0].function = ExitMainCB;
	(void)DefaultModalGizmo (
		parent, &textChanged, GetGizmoText (TXT_RU_SURE)
	);
}

void
CreateNewSendModal (parent)
Widget		parent;
{
	textChangedItems[0].label = BUT_NEW;
	textChangedItems[0].mnemonic = MNEM_NEW;
	textChangedItems[0].function = NewCB;
	(void)DefaultModalGizmo (
		parent, &textChanged, GetGizmoText (TXT_NEW_TEXT)
	);
}
static void
FreeShowCB (wid, client_data, call_data)
Widget		wid;
XtPointer	client_data;
XtPointer	call_data;
{
	CancelCB (wid, client_data, call_data);
	FREE (ErrorText);
}

static MenuItems showErrorItems[] = {
    {True, BUT_OK,	MNEM_OK,	NULL,	FreeShowCB},
    {True, BUT_HELP,	MNEM_HELP,	NULL,	HelpCB, (XtPointer)HelpSender},
    {NULL}
};

static MenuGizmo showError = {
	NULL,			/* help		*/
	"na",			/* name		*/
	TXT_TRANS_ERROR,	/* title	*/
	showErrorItems,		/* items	*/
	0,			/* function	*/
	NULL,			/* client_data	*/
	CMD,			/* buttonType	*/
	OL_FIXEDROWS,		/* layoutType	*/
	1,			/* measure	*/
	1			/* default Item	*/
};

void
ShowErrorCB (wid, client_data, call_data)
Widget		wid;
char *		client_data;
XtPointer	call_data;
{
	ModalGizmo *	gp;
	int		len;
	char *		buf;
	char *		savept = NULL;
	char *		string;
	char *		cp;

	/*
	 * Bring down the error notice popup.
	 */
	CancelCB (wid, client_data, call_data);

	/*
	 * Copy the errors without UX:mail: ERROR.
	 */
	if (Version == 41) {
		buf = (char *)ErrorText;
		cp = buf;
		while ((string = MyStrtok (buf, "\n", &savept)) != NULL) {
			buf = NULL;
			if (strncmp (
				string, NTS_UX_MAILX, sizeof(NTS_UX_MAILX)-1
			) == 0) {
				string += sizeof(NTS_UX_MAILX)-1;
			}
			if (strncmp (
				string, NTS_UX_MAIL, sizeof(NTS_UX_MAIL)-1
			) == 0) {
				string += sizeof(NTS_UX_MAIL)-1;
				string = strchr (string, ':')+2;
			}
			len = strlen (string)+1;
			strcpy (cp, string);
			strcat (cp, "\n");
			cp += len;
		}
	}
	gp = DefaultModalGizmo (
		XtParent ((Widget)_OlGetShellOfWidget (wid)),
		&showError, (char *)ErrorText
	);
	XtVaSetValues (
		gp->stext, XtNalignment, OL_LEFT,
		(String)0
	);
}

static MenuItems transmitErrorItems[] = {
    {True, BUT_OK,	MNEM_OK,	NULL,	FreeShowCB},
    {True, BUT_SHOW,	MNEM_SHOW,	NULL,	ShowErrorCB},
    {True, BUT_HELP,	MNEM_HELP,	NULL,	HelpCB, (XtPointer)HelpSender},
    {NULL}
};

static MenuGizmo transmitError = {
	NULL,			/* help		*/
	"na",			/* name		*/
	TXT_TRANS_ERROR,	/* title	*/
	transmitErrorItems,	/* items	*/
	0,			/* function	*/
	NULL,			/* client_data	*/
	CMD,			/* buttonType	*/
	OL_FIXEDROWS,		/* layoutType	*/
	1,			/* measure	*/
	1			/* default Item	*/
};

void
CreateTransErrorModal (parent, text)
Widget	parent;
char *	text;
{
	ErrorText = STRDUP (text);
	(void)DefaultModalGizmo (
		parent, &transmitError, GetGizmoText (TXT_TRANS_ERROR)
	);
}

static MenuItems invalidAddressItems[] = {
    {True, BUT_OK,	MNEM_OK,	NULL,	CancelCB},
    {True, BUT_HELP,	MNEM_HELP,	NULL,	HelpCB, (XtPointer)HelpSender},
    {NULL}
};

static MenuGizmo InvalidAddress = {
	NULL,			/* help		*/
	"na",			/* name		*/
	TXT_TRANS_ERROR,	/* title	*/
	invalidAddressItems,	/* items	*/
	0,			/* function	*/
	NULL,			/* client_data	*/
	CMD,			/* buttonType	*/
	OL_FIXEDROWS,		/* layoutType	*/
	1,			/* measure	*/
	1			/* default Item	*/
};

void
CreateInvalidAddressModal (parent, buf)
Widget	parent;
char *	buf;
{
	ModalGizmo *	gp;

	gp = DefaultModalGizmo (parent, &InvalidAddress, buf);

	XtVaSetValues (
		gp->stext, XtNalignment, OL_LEFT,
		(String)0
	);
}

static MenuItems invalidTargetItems[] = {
    {True, BUT_OK,	MNEM_OK,	NULL,	CancelCB},
    {True, BUT_HELP,	MNEM_HELP,	NULL,	HelpCB, (XtPointer)HelpManager},
    {NULL}
};

static MenuGizmo InvalidTarget = {
	NULL,			/* help		*/
	"na",			/* name		*/
	TXT_INVALID_TARGET,	/* title	*/
	invalidTargetItems,	/* items	*/
	0,			/* function	*/
	NULL,			/* client_data	*/
	CMD,			/* buttonType	*/
	OL_FIXEDROWS,		/* layoutType	*/
	1,			/* measure	*/
	1			/* default Item	*/
};

void
CreateInvalidTargetModal (parent)
Widget	parent;
{
	(void)DefaultModalGizmo (
		parent, &InvalidTarget, GetGizmoText (TXT_INVALID_TARGET)
	);
}
