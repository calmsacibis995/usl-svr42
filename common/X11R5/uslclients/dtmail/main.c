/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtmail:main.c	1.74"
#endif

#define MAIN_C

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Dynamic.h>	/* For ButtonAction */
#include <Xol/OlCursors.h>	/* For OlGetBusyCursor */
#include <DtI.h>
#include <Shell.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include "mail.h"
#include <Gizmo/LabelGizmo.h>
#include <Gizmo/Gizmos.h>

#define ROOT		RootWindowOfScreen(XtScreen(Root))

#define HELP_PATH	"dtmail" "/" "mail.hlp"

#define TIME_OUT	15000	/* 15 seconds */

#define DROP_RESOURCE	"dtmail"
#define DEFAULT_PRINT_COMMAND	     "/usr/X/bin/wrap | /usr/X/bin/PrtMgr"


extern MailRec *	mailRec;
extern ReadRec *	readRec;
extern SendRec *	sendRec;
extern ManageRec *	manageRec;
extern o_ino_t		DummyDir;
extern BaseWindowGizmo	AliasWindow;
extern BaseWindowGizmo	MainWindow;
extern AliasRec *       aliasRec;

extern HelpInfo		MailHelp = {
	TXT_JUST_MAIL, HELP_MAIL_TITLE, HELP_PATH, HELP_MAIL_SECT
};

extern HelpInfo		ManagerHelp = {
	TXT_JUST_MAIL, HELP_MANAGER_TITLE, HELP_PATH, HELP_MANAGER_SECT
};
extern HelpInfo		ReaderHelp = {
	TXT_JUST_MAIL, HELP_READER_TITLE, HELP_PATH, HELP_READER_SECT
};
extern HelpInfo		SenderHelp = {
	TXT_JUST_MAIL, HELP_SENDER_TITLE, HELP_PATH, HELP_SENDER_SECT
};
extern HelpInfo		AliasManagerHelp = {
	TXT_JUST_MAIL, HELP_ALIAS_MANAGER_TITLE,
	HELP_PATH, HELP_ALIAS_MANAGER_SECT
};

extern HelpInfo		TOCHelp = {
	TXT_JUST_MAIL, HELP_TOC_TITLE, HELP_PATH, HELP_TOC_SECT
};
extern HelpInfo		HelpDeskHelp = {
	TXT_JUST_MAIL, HELP_DESK_TITLE, HELP_PATH, HELP_DESK_SECT
};

extern HelpInfo		ManagerOpenHelp = {
	TXT_JUST_MAIL, HELP_MANAGER_OPEN_TITLE, HELP_PATH, HELP_MANAGER_OPEN_SECT
};
extern HelpInfo		ManagerSaveAsHelp = {
	TXT_JUST_MAIL, HELP_MANAGER_SAVEAS_TITLE,
	HELP_PATH, HELP_MANAGER_SAVEAS_SECT
};
extern HelpInfo		ManagerUndeleteHelp = {
	TXT_JUST_MAIL, HELP_MANAGER_UNDELETE_TITLE,
	HELP_PATH, HELP_MANAGER_UNDELETE_SECT
};

extern HelpInfo		ReaderOpenHelp = {
	TXT_JUST_MAIL, HELP_READER_OPEN_TITLE, HELP_PATH, HELP_READER_OPEN_SECT
};
extern HelpInfo		ReaderSaveAsHelp = {
	TXT_JUST_MAIL, HELP_READER_SAVE_TITLE, HELP_PATH, HELP_READER_SAVE_SECT
};

extern HelpInfo		SenderOpenHelp = {
	TXT_JUST_MAIL, HELP_SENDER_OPEN_TITLE, HELP_PATH, HELP_SENDER_OPEN_SECT
};
extern HelpInfo		SenderSaveAsHelp = {
	TXT_JUST_MAIL, HELP_SENDER_SAVE_TITLE, HELP_PATH, HELP_SENDER_SAVE_SECT
};

extern HelpInfo		ReaderPropertiesHelp = {
	TXT_JUST_MAIL, HELP_READPROP_TITLE, HELP_PATH, HELP_READPROP_SECT
};
extern HelpInfo		SenderPropertiesHelp = {
	TXT_JUST_MAIL, HELP_SENDPROP_TITLE, HELP_PATH, HELP_SENDPROP_SECT
};

extern HelpInfo		AliasesHelp = {
	TXT_JUST_MAIL, HELP_ALIASES_TITLE, HELP_PATH, HELP_ALIASES_SECT
};
extern HelpInfo		AliasManagerUndeleteHelp = {
	TXT_JUST_MAIL, HELP_ALIAS_MANAGER_UNDELETE_TITLE,
	HELP_PATH, HELP_ALIAS_MANAGER_UNDELETE_SECT
};
extern HelpInfo		AliasManagerOverwriteHelp = {
	TXT_JUST_MAIL, HELP_ALIAS_MANAGER_OVERWRITE_TITLE,
	HELP_PATH, HELP_ALIAS_MANAGER_OVERWRITE_SECT
};
extern HelpInfo		AliasManagerSureHelp = {
	TXT_JUST_MAIL, HELP_ALIAS_MANAGER_SURE_TITLE,
	HELP_PATH, HELP_ALIAS_MANAGER_SURE_SECT
};


/*
 * The Send only variable, when True, means that only the send
 * window should be displayed.  This means that the mailer will
 * be open to "/" and will have nothing to display.
 */
Widget		Root;
uid_t		UserId;
char *		UserName;
char *		Home;
o_ino_t		DummyDir;
char		MailDirectory[] = NTS_VAR_MAIL;
char *		FormalApplicationName;
char *		ApplicationName;
char *		ApplicationClass;
char *		Argv_0;
int		LastSelectedMessage;	/* For include in the mailer */
MailRec *	LastMailRec;		/* MailRec containing */
					/* LastSelectedMessage. */

char *			PrintCommand = DEFAULT_PRINT_COMMAND;
static Boolean		Warnings = False;

static XtResource	Resources[] = {
	{
		"warnings", "Warnings", XtRBoolean, sizeof(Boolean),
		(Cardinal) &Warnings, XtRBoolean, (XtPointer) &Warnings
	},
	{
		"printCommand", "printCommand", XtRString, sizeof(char *),
		(Cardinal) &PrintCommand, XtRString, DEFAULT_PRINT_COMMAND
	}
};

Arg arg[50];

static char *
GetUserName (id)
uid_t id;
{
	struct passwd *p;

	p = getpwuid (id);
	return STRDUP (p->pw_name);
}

char *
GetUserMailFile ()
{
	static char	filename[BUF_SIZE];
	static Boolean	first = True;
	char *		tmp;

	if (first == True) {
		/*
		 * Check MAIL environment variable, if set return this as the
		 * system mailbox name else use the default.
		 */
		tmp = (char *)getenv ("MAIL");
		strcpy (filename, tmp);
		if (tmp == NULL || tmp[0] == '\0') {
			strcpy (filename, MailDirectory);
			strcat (filename, "/");
			strcat (filename, UserName);
		}
		first = False;
	}
	return filename;
}

static void
ExitAllReaders ()
{
      	ReadRec *       rp;
      	ReadRec *       next;

      	for (rp=readRec; rp!= (ReadRec *)0; ) {
      		next = rp->next;
      		ExitReader (rp);
      		rp = next;
      	}
}

void
HelpCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	Widget			shell;
	shell = GetBaseWindowShell (&MainWindow);

 	switch ((int)client_data) {
		case HelpMail: {
			PostGizmoHelp (shell, &MailHelp);
			break;
		}
		case HelpManager: {
			PostGizmoHelp (shell, &ManagerHelp);
			break;
		}
		case HelpReader: {
			PostGizmoHelp (shell, &ReaderHelp);
			break;
		}
		case HelpSender: {
			PostGizmoHelp (shell, &SenderHelp);
			break;
		}
		case HelpAliasManager: {
			PostGizmoHelp (shell, &AliasManagerHelp);
			break;
		}
		case HelpTOC: {
			PostGizmoHelp (shell, &TOCHelp);
			break;
		}
		case HelpDesk: {
			PostGizmoHelp (shell, &HelpDeskHelp);
			break;
		}
		case HelpManagerOpen: {
			PostGizmoHelp (shell, &ManagerOpenHelp);
			break;
		}
		case HelpManagerSaveAs: {
			PostGizmoHelp (shell, &ManagerSaveAsHelp);
			break;
		}
		case HelpManagerUndelete: {
			PostGizmoHelp (shell, &ManagerUndeleteHelp);
			break;
		}
		case HelpReaderOpen: {
			PostGizmoHelp (shell, &ReaderOpenHelp);
			break;
		}
		case HelpReaderSaveAs: {
			PostGizmoHelp (shell, &ReaderSaveAsHelp);
			break;
		}
		case HelpSenderOpen: {
			PostGizmoHelp (shell, &SenderOpenHelp);
			break;
		}
		case HelpSenderSaveAs: {
			PostGizmoHelp (shell, &SenderSaveAsHelp);
			break;
		}
		case HelpReaderProperties: {
			PostGizmoHelp (shell, &ReaderPropertiesHelp);
			break;
		}
		case HelpSenderProperties: {
			PostGizmoHelp (shell, &SenderPropertiesHelp);
			break;
		}
		case HelpAliases: {
			PostGizmoHelp (shell, &AliasesHelp);
			break;
		}
		case HelpAliasManagerUndelete: {
			PostGizmoHelp (shell, &AliasManagerUndeleteHelp);
			break;
		}
		case HelpAliasManagerOverwrite: {
			PostGizmoHelp (shell, &AliasManagerOverwriteHelp);
			break;
		}
		case HelpAliasManagerSure: {
			PostGizmoHelp (shell, &AliasManagerSureHelp);
			break;
		}
		default: {
			fprintf (
			    stderr,
			    "Impossible help entry requested, AliasHelpCB\n"
			);
		}
	}
}


void
ExitMainCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	MailRec *	mp = mailRec;
	SendRec *	sp;

	for (sp=sendRec; sp!=(SendRec *)0; sp=sp->next) {
		DeleteSendRec (sp);
	}
	UpdateMailrc (mp);
	QuitMailx (mp);
	exit (0);
}

static void
ExitCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	MailRec *	mp;
        SendRec *	sp;
	Boolean		changed = False;

        /*
         * Exit all of the manager windows.
	 */
	for (mp=mailRec; mp!=(MailRec *)0; mp=mp->next) {
		ExitManager (mp->mng);
	}
        /*
         * Exit all of the reader windows.
	 */
	ExitAllReaders ();
	/*
         * Put up a warning if text has changed in the alias window
	 */
	if (aliasRec->unsaved) {
		changed = True;
	}
        else {
              	AliasExit();
	}
	/*
	 * Put up a warning if text has changed in any of the sender windows
	 */
	for (sp=sendRec; sp!=(SendRec *)0; sp=sp->next) {
		if (sp->used == True) {
			if (TextChanged (sp) == True) {
				changed = True;
			}
			else {
				LeaveSend (sp);
			}
		}
	}
	if (changed == True) {
		CreateExitModal (wid);
	}
	else {
		ExitMainCB (wid, client_data, call_data);
	}
}

static void
OpenDefaultManageCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	(void)OpenNewMailFile (GetUserMailFile(), &MainWindow);
}

static void
OpenDefaultReadCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	DtLockCursor (
		GetBaseWindowShell (&MainWindow),
		1000L, NULL, NULL, OlGetBusyCursor(wid)
	);
	(void)OpenReadWindowOnly (GetUserMailFile(), &MainWindow);
}

static void
OpenDefaultSendCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	DtLockCursor (
		GetBaseWindowShell (&MainWindow),
		1000L, NULL, NULL, OlGetBusyCursor(wid)
	);
	ManageSend ();
}

static void
OpenDefaultAliasCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	BaseWindowGizmo * AliasWindowP = &AliasWindow;
	Widget		shell;

	DtLockCursor (
		GetBaseWindowShell (&MainWindow),
		1000L, NULL, NULL, OlGetBusyCursor(wid)
	);

	if (aliasRec->baseWGizmo != (BaseWindowGizmo *) 0 ) {
		MapGizmo (BaseWindowGizmoClass, (Gizmo) AliasWindowP);
		shell = GetBaseWindowShell (AliasWindowP);
		XRaiseWindow (XtDisplay (shell), XtWindow (shell));
		if (aliasRec->mapped) {
			/*
			 * If it already exists and is mapped,
			 * pop it to the front
			 */
			return;
		}
		else {
			/*
			 * If it already exists and is not mapped,
			 * bring it back
			 */
			aliasRec->mapped = True;
			/* initialize menu sensitivies */
			SetSensitivity (AliasWindowP,"aliasEdit", 0, 0, False);
			SetSensitivity (AliasWindowP,"aliasEdit", 1, 1, True);
			SetSensitivity (AliasWindowP,"aliasEdit", 2, 2, True);
			/* SetSensitivity (AliasWindowP,"aliasEdit", 3, 3, True);
			/***  fprintf (stderr, "Remap set sens true\n"); /***/
			SetSensitivity (AliasWindowP,"aliasEdit", 4, 4, False);
		}
	}
	else {
		/* It doesn't yet exist.  Create it. */
		UserId = getuid ();
		UserName = GetUserName (UserId);
		AliasWinInit (UserName);
		MapGizmo (BaseWindowGizmoClass, (Gizmo) AliasWindowP);
		aliasRec->mapped = True;
	}
}

void
DisplayNewMailMessage (mp, filename)
MailRec *	mp;
char *		filename;
{
	char	buf[BUF_SIZE];

	sprintf (buf, GetGizmoText (TXT_NEW_MAIL), filename);
	SetBaseWindowMessage (&MainWindow, buf);
	if (mp != (MailRec *)0) {
		UpdateFooter (mp);
		if (mp->mng != (ManageRec *)0) {
			SetBaseWindowMessage (mp->mng->baseWGizmo, buf);
		}
	}
	_OlBeepDisplay(GetBaseWindowShell (&MainWindow), 1);
}

static void
ShowNewMail (filename, inode)
char *		filename;
o_ino_t		inode;
{
	MailRec *	mp;
	char *		cp;

	/* is this mail file already open?
	 */
	if (
		(mp = GetMailx (inode))         != 0 ||
		(mp = GetMailx (0))             != 0 ||
		(mp = GetMailx ((o_ino_t)-1))   != 0 ||
		(mp = GetMailx (DummyDir))      != 0
	) {
		if (mp->noMail == True) {
			if (SwitchMailx(mailRec,filename,&MainWindow)==True) {
				XtVaSetValues (
					GetSummaryListWidget (mp->mng),
					XtNnumItems,	mp->summary->size,
					XtNitems,	mp->summary->items,
					(String)0
				);
			}
		}
		else {
			/* This will check for new mail */
			(void)ProcessCommand (mp, EQUALS, NULL);
			/* Display new mail message in
			 * main window */
		}
	}
	DisplayNewMailMessage (mp, filename);
}

/*
 * IsThereNewMail? - periodically check the user's mail file to see
 * if new mail has arrived.
 */
void
IsThereNewMail (filename, id)
char *		filename;
XtIntervalId *	id;
{
	o_ino_t		inode;
	static time_t	mdate = (time_t)-1;
	time_t		t;
	static off_t	lastsize = 0;
	off_t		size;

	/* See if times on the file have changed
	 */
	if ((t = StatFile (filename, &inode, &size)) != mdate) {
		/* Don't do this the first time here */
		if (mdate != -1 && size > lastsize) {
			/* Times have changed */
			ShowNewMail (filename, inode);
		}
	}
	lastsize = size;
	mdate = t;
	XtAddTimeOut (TIME_OUT, (XtTimerCallbackProc)IsThereNewMail, filename);
}

static void
LookForDblClickCB (w, func, call_data)
Widget		w;
PFV		func;
XtPointer	call_data;
{
	OlVirtualEvent	ve = (OlVirtualEvent)call_data;
	Arg		arg[10];
	ButtonAction	ba;

	if (ve->virtual_name ==  OL_SELECT) {
		switch (ve->xevent->type) {
			case ButtonPress: {
				ba = OlDetermineMouseAction (w, ve->xevent);
				if (ba == MOUSE_MULTI_CLICK) {
					OlVaFlatSetValues (
						w, 0, XtNbusy, True, (String)0
					);
					(*func)(w, NULL, NULL);
					OlVaFlatSetValues (
						w, 0, XtNbusy, False, (String)0
					);
				}
				else if (ba == MOUSE_MOVE) {
					XUngrabPointer (
						XtDisplay(w), CurrentTime
					);
				}
				ve->consumed = True;
				break;
			}
		}
	}
}

static void
SetMenuPixmaps (menu, caption, filename, func)
Widget	menu;
Widget	caption;
char *	filename;
PFV	func;
{
	Screen *	screen = XtScreen(menu);
	DmGlyphRec *	gp;
	static char *	flatMenuFields[] = {
		XtNsensitive,  /* sensitive                      */
		XtNlabel,      /* label                          */ 
		XtNuserData,   /* mnemonic string		 */
		XtNuserData,   /* nextTier | resource_value      */
		XtNselectProc, /* function                       */
		XtNclientData, /* client_data                    */
		XtNset,        /* set                            */
		XtNpopupMenu,  /* button                         */
		XtNmnemonic,   /* mnemonic                       */ 
		XtNbackgroundPixmap,
		XtNbusy
	};

	/* Change the item fields to add in a background pixmap */
	XtVaSetValues (
		menu,
		XtNitemFields,		flatMenuFields,
		XtNnumItemFields,	XtNumber(flatMenuFields),
		XtNhSpace,		0,
		(String)0
	);

	gp = DmGetPixmap (screen, filename);

	OlVaFlatSetValues (
		menu, 0,
		XtNlabel, NULL,
		XtNbackgroundPixmap, gp->pix,
		(String)0
	);

	/*
	 * Add callback that will make sure a double click occurred
	 * before calling the appropriate routine.
	 */
	OlAddCallback (
		menu,
		XtNconsumeEvent,
		(XtCallbackProc)LookForDblClickCB,
		(XtPointer)func
	);

	XtVaSetValues (
		caption,
		XtNposition,	OL_BOTTOM,
		XtNalignment,	OL_CENTER,
		(String)0
	);
}

static MenuItems mainExitItems[] = {
	{True, BUT_READ_PROP,	MNEM_READ_PROP,	 NULL, ReadPropPopupCB},
	{True, BUT_SEND_PROP,	MNEM_SEND_PROP_S,NULL, SendPropPopupCB},
	{True, BUT_EXIT,	MNEM_EXIT_E,	 NULL, ExitCB},
	{NULL}
};

static MenuGizmo mainExitMenu = {
	NULL, "mainExit", NULL, mainExitItems,
	0, NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static MenuItems mainMailItems[] = {
	{True, BUT_MANAGE_MAIL_DDD,	MNEM_MANAGE_MAIL_DDD,  NULL, OpenDefaultManageCB},
	{True, BUT_READ_MAIL_DDD,	MNEM_READ_MAIL_DDD,  NULL, OpenDefaultReadCB},
	{True, BUT_SEND_MAIL_DDD,	MNEM_SEND_MAIL_DDD,  NULL, OpenDefaultSendCB},
	{True, BUT_ALIAS_MNG_DDD,	MNEM_ALIAS_MNG_DDD,  NULL, OpenDefaultAliasCB},
	{NULL}
};

static MenuGizmo mainMailMenu = {
	NULL, "mainMail", NULL, mainMailItems,
	0, NULL, CMD, OL_FIXEDCOLS, 1, 0
};

/* Define the three picture buttons */

static MenuItems button0Items[] = {
	{True, "",		(XtArgVal)0,	NULL,	OpenDefaultManageCB},
	{NULL}
};

static MenuItems button1Items[] = {
	{True, "",		(XtArgVal)0,	NULL,	OpenDefaultReadCB},
	{NULL}
};

static MenuItems button2Items[] = {
	{True, "",		(XtArgVal)0,	NULL,	OpenDefaultSendCB},
	{NULL}
};

static MenuItems button3Items[] = {
	{True, "",		(XtArgVal)0,	NULL,	OpenDefaultAliasCB},
	{NULL}
};

static MenuGizmo menu0 = {
	NULL, "pixmapMenu0", NULL, button0Items,
	0, NULL, CMD, OL_FIXEDROWS, 1, OL_NO_ITEM
};

static MenuGizmo menu1 = {
	NULL, "pixmapMenu1", NULL, button1Items,
	0, NULL, CMD, OL_FIXEDROWS, 1, OL_NO_ITEM
};

static MenuGizmo menu2 = {
	NULL, "pixmapMenu2", NULL, button2Items,
	0, NULL, CMD, OL_FIXEDROWS, 1, OL_NO_ITEM
};

static MenuGizmo menu3 = {
	NULL, "pixmapMenu3", NULL, button3Items,
	0, NULL, CMD, OL_FIXEDROWS, 1, OL_NO_ITEM
};

static GizmoRec buttonArray0[] = {
	{MenuBarGizmoClass,	&menu0}
};

static GizmoRec buttonArray1[] = {
	{MenuBarGizmoClass,	&menu1}
};

static GizmoRec buttonArray2[] = {
	{MenuBarGizmoClass,	&menu2}
};

static GizmoRec buttonArray3[] = {
	{MenuBarGizmoClass,	&menu3}
};

static Arg buttonArgs[] = {
	XtNhPad,	0
};

static LabelGizmo label0 = {
	NULL, "label0", TXT_MANAGER_ICON_NAME,
	buttonArray0, XtNumber (buttonArray0),
	OL_FIXEDROWS, 1, buttonArgs, 1, False
};

static LabelGizmo label1 = {
	NULL, "label1", TXT_READER_ICON_NAME,
	buttonArray1, XtNumber (buttonArray1),
	OL_FIXEDROWS, 1, buttonArgs, 1, False
};

static LabelGizmo label2 = {
	NULL, "label2", TXT_SENDER_ICON_NAME,
	buttonArray2, XtNumber (buttonArray2),
	OL_FIXEDROWS, 1, buttonArgs, 1, False
};

static LabelGizmo label3 = {
	NULL, "label3", TXT_ALIAS_MANAGER_ICON,
	buttonArray3, XtNumber (buttonArray3),
	OL_FIXEDROWS, 1, buttonArgs, 1, False
};

static GizmoRec menuGiz[] = {
	{LabelGizmoClass,	&label0},
	{LabelGizmoClass,	&label1},
	{LabelGizmoClass,	&label2},
	{LabelGizmoClass,	&label3}
};

static Arg labelArgs[] = {
	XtNhSpace,	0,
	XtNhPad,	0
};

static LabelGizmo pixmapLabel = {
	NULL, "pixmaps", "", menuGiz, XtNumber (menuGiz),
	OL_FIXEDROWS, 1, labelArgs, 2, False
};

static GizmoRec pixmapArea[] = {
	{LabelGizmoClass,	&pixmapLabel},
};

static MenuItems mainHelpItems[] = {
	{True, BUT_MAIL_DDD,	MNEM_MAIL_DDD,	 NULL, NULL, (XtPointer)HelpMail},
	{True, BUT_TOC_DDD,	MNEM_TOC_DDD,	 NULL, NULL, (XtPointer)HelpTOC},
	{True, BUT_HELP_DESK_DDD,	MNEM_HELP_DESK_DDD,NULL, NULL, (XtPointer)HelpDesk},
	{NULL}
};

static MenuGizmo mainHelpMenu = {
	NULL, "mainHelp", NULL, mainHelpItems, HelpCB,
	NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static MenuItems mainMenuItems[] = {
	{True, BUT_FILE,	MNEM_FILE,		(char *)&mainExitMenu},
	{True, BUT_MAIL,	MNEM_MAIL,		(char *)&mainMailMenu},
	{True, BUT_HELP,	MNEM_HELP,		(char *)&mainHelpMenu},
	{NULL}
};

static MenuGizmo Main = {
	NULL, "mainWindow", NULL, mainMenuItems,
	0, NULL, CMD, OL_FIXEDROWS, 1, 0
};

BaseWindowGizmo MainWindow = {
	&MailHelp,
	"MainWindow",
	"",
	&Main,
	pixmapArea,
	XtNumber (pixmapArea),
	TXT_E_MAIL_ICON_NAME,
	"mail.icon",
	" ",
	" ",
	100,
};

void
InitDtmail (argc, argv)
int	argc;
char *	argv[];
{
	char			buf[BUF_SIZE];
	Widget			shell;
	MailRec *		mp;
	Arg			arg[10];
	static XtCallbackRec	protocolCB[] = {
		{WindowManagerEventHandler, (XtPointer)ExitCB},
		{NULL}
	};

	UserId = getuid ();
	UserName = GetUserName (UserId);
	Home = (char *)getenv ("HOME");
	if (Home == NULL || Home[0] == '\0') {
		Home = "./";
	}

	/* Indicated there was no previously selected message */
	LastSelectedMessage = -1;
	LastMailRec = (MailRec *)0;
	mp = OpenMailx ();
	/* Look for mailx variables: "record", "folder", "outfolder" */
	GetSettings (mp, ProcessCommand (mp, SET_CMD, NULL));
	DummyDir = mp->inode;
	sprintf (buf, GetGizmoText (TXT_E_MAIL_COLON));
	MainWindow.title = buf;
	XtSetArg(arg[0], XtNwmProtocol, protocolCB);
	shell = CreateGizmo (
		Root, BaseWindowGizmoClass, &MainWindow, arg, 1
	);
	OlDnDRegisterDDI (
		MainWindow.icon_shell, OlDnDSitePreviewNone, ManageDropNotify,
		(OlDnDPMNotifyProc)NULL, True, (XtPointer)NULL
	);
	OlDnDRegisterDDI (
		MainWindow.form, OlDnDSitePreviewNone, ManageDropNotify,
		(OlDnDPMNotifyProc)NULL, True, (XtPointer)NULL
	);
	SetMenuPixmaps (
		(Widget)QueryGizmo (
			BaseWindowGizmoClass, &MainWindow,
			GetGizmoWidget, "pixmapMenu0"
		),
		(Widget)QueryGizmo (
			BaseWindowGizmoClass, &MainWindow,
			GetGizmoWidget, "label0"
		),
		"manmail.icon", OpenDefaultManageCB
	);
	SetMenuPixmaps (
		(Widget)QueryGizmo (
			BaseWindowGizmoClass, &MainWindow,
			GetGizmoWidget, "pixmapMenu1"
		),
		(Widget)QueryGizmo (
			BaseWindowGizmoClass, &MainWindow,
			GetGizmoWidget, "label1"
		),
		"rdmail.icon", OpenDefaultReadCB
	);
	SetMenuPixmaps (
		(Widget)QueryGizmo (
			BaseWindowGizmoClass, &MainWindow,
			GetGizmoWidget, "pixmapMenu2"
		),
		(Widget)QueryGizmo (
			BaseWindowGizmoClass, &MainWindow,
			GetGizmoWidget, "label2"
		),
		"sndmail.icon", OpenDefaultSendCB
	);
	SetMenuPixmaps (
		(Widget)QueryGizmo (
			BaseWindowGizmoClass, &MainWindow,
			GetGizmoWidget, "pixmapMenu3"
		),
		(Widget)QueryGizmo (
			BaseWindowGizmoClass, &MainWindow,
			GetGizmoWidget, "label3"
		),
		"alias.icon", OpenDefaultAliasCB
	);
	XtAddTimeOut (
		TIME_OUT, (XtTimerCallbackProc)IsThereNewMail,
		GetUserMailFile()
	);

	MapGizmo (BaseWindowGizmoClass, &MainWindow);
	/* Construct the brief list for the read property */
	InitBriefList (mp);

	if (argc == 2) {
		(void)OpenNewMailFile (argv[1], &MainWindow);
	}

	/* Initialize Alias Management */
	aliasRec->baseWGizmo = (BaseWindowGizmo *) 0;
	AliasWinInit (UserName);
}

void
main(argc, argv)
int argc;
char *argv[];
{
#ifdef MEMUTIL
	_SetMemutilDebug(2);
#endif

	FormalApplicationName = GetGizmoText (TXT_JUST_MAIL);
	ApplicationName = "dtmail";
	ApplicationClass = "dtmail";
	Root = InitializeGizmoClient (
		ApplicationName,
		ApplicationClass,
		FormalApplicationName,
		NULL,
		NULL,
		NULL,
		0,
		&argc, argv,
		NULL,
		NULL,
		Resources, XtNumber(Resources),
		NULL,
		0,
		DROP_RESOURCE,
		(Boolean) ManageDropNotify,
		NULL
	);

	PrintCommand = STRDUP (PrintCommand);

	if (Root) {
		InitDtmail (argc, argv);
	}
	XtMainLoop ();
}
