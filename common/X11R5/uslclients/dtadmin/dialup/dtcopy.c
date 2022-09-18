/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/dtcopy.c	1.13"
#endif
/*
	Description:
		dtcopy: transfer a directory/file to a remote system

*/

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <limits.h>
#include <wait.h>
#include "dtcopy.h"
#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <X11/Shell.h>
#include <FButtons.h>

#include <Gizmos.h>
#include <PopupGizmo.h>
#include <MenuGizmo.h>
#include <InputGizmo.h>
#include <SpaceGizmo.h>
#include "STextGizmo.h"
#include <LabelGizmo.h>
#include <ChoiceGizm.h>
#include "uucp.h"
#include "error.h"

#define GGT GetGizmoText
/*
 * DO NOT REMOVE.  It is used to overwrite the default help
 * file set in the error.h file.
 */
#ifdef	HELP_FILE
#undef	HELP_FILE
#endif
#define HELP_FILE	"dtadmin/remotsys.hlp"

char *		ApplicationName;
char *		Program;

extern void	HelpCB();
extern	int	AttrParse();
extern	char	*GetUser();
extern	char	*GetAttrValue();
extern	void	NotifyUser ();
extern	void	MailUser ();
extern	void	rexit();

extern	int	optind;
extern	char	*optarg;

static	Widget	root;
static	Boolean	changed = False;
static	char 	target[PATH_MAX];

static	void	ChangeCB();

static void CreatePopup(Widget);
static void TransferCB(Widget, XtPointer, XtPointer);
static void EnablePathCB(Widget, XtPointer, XtPointer);
static void DisablePathCB(Widget, XtPointer, XtPointer);
static void ExitMain();
static void WindowManagerEventHandler(Widget, XtPointer, XtPointer);

static void
CancelCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
        XtPopdown ((Widget)_OlGetShellOfWidget (wid));
}

typedef struct _applicationResources
   {
   Boolean      warnings;
   int          beepVolume;
   Pixel        DefaultForeground;
   Pixel        DefaultBackground;
   } ApplicationResources;

ApplicationResources ViewResources;

static XtResource resources[] =
   {
   { "warnings", "warnings", XtRBoolean, sizeof(Boolean),
     (Cardinal) &ViewResources.warnings, 
     XtRString, (XtPointer)"false" },
   { "beepVolume", "beepVolume", XtRInt, sizeof(int),
     (Cardinal) &ViewResources.beepVolume, 
     XtRString, "0" },
   { "TextFontColor", "TextFontColor", XtRPixel, sizeof(Pixel),
     (Cardinal) &ViewResources.DefaultForeground, XtRString, "black" },
   { "Background", "BackGround", XtRPixel, sizeof(Pixel),
     (Cardinal) &ViewResources.DefaultBackground, XtRString, "white" },
   };

static HelpText AppHelp = {
    title_remote, HELP_FILE, help_remote,
};

typedef enum
{ TransferApply, TransferReset, TransferCancel, TransferHelp }
TransferMenuItemIndex;

static MenuItems  TransferMenuItems[] =
   {
   {(XA)True, label_send, mnemonic_send},
   {(XA)True, label_reset, mnemonic_reset},
   {(XA)True, label_cancel, mnemonic_cancel},
   {(XA)True, label_help, mnemonic_help, NULL, HelpCB, (char *)&AppHelp },
   { 0 }
   };

static MenuGizmo TransferMenu =
   { NULL, "_X_", "_X_", TransferMenuItems, TransferCB, NULL, CMD, OL_FIXEDROWS, 1, 0 };

typedef struct _DTSetting {
	Setting *duser;
	Setting *rpath;
	Setting *dpath;
} DTSetting;
typedef struct _DtSetting {
	Setting duser;
	Setting rpath;
	Setting dpath;
} DtSetting;

DtSetting dtSettings = {
	{"Recipient",	NULL,	NULL,	(XtPointer)0	},
	{"Destiny",	NULL,	NULL,	(XtPointer)0	},
	{"Folder",	NULL,	NULL,	(XtPointer)0	},
};

/* for ``Destination System'' prompt */

static char dmach[DST_LEN];
static StaticTextGizmo System = {
	NULL,			/* HELP		*/
	"system",		/* NAME		*/
	dmach
};

static GizmoRec rhost[] = {
	{StaticTextGizmoClass,	&System},
};

static LabelGizmo Rhost = {
	NULL,			/* HELP		*/
	"rhost",		/* NAME		*/
	TXT_DESTINATION,	/* CAPTION      */
	rhost,			/* GIZMOARRAY	*/
	XtNumber(rhost),	/* 	*/
	OL_FIXEDCOLS,		/* LAYOUT	*/
	1,			/* MEASURE	*/
	0,			/* Arg array	*/
	0,			/* numArg	*/
	True			/* ALIGNCAP	*/
};

static char ruser[UNAMESIZE];
/* for ``Recipient'' prompt */
static InputGizmo Recipient = {
        NULL,
	"recipient",
	TXT_RECIPIENT,
	ruser,
	&dtSettings.duser,
	UNAMESIZE,
	ChangeCB,
};

/* for ``Delivery Method'' prompt */

static StaticTextGizmo UUCP = {
	NULL,			/* HELP		*/
	"uucp",		/* NAME		*/
	"UUCP"
};

static MenuItems rpathItems[] = {
	{True, label_uucp,	mnemonic_uucp,	"uucp", DisablePathCB},
	{True, label_inet,	mnemonic_inet,	"internet", EnablePathCB},
	{NULL, 0 }
};

static MenuGizmo rpathMenu = {
	NULL, "rpath", "", rpathItems, 0, NULL, EXC,
	OL_FIXEDROWS, 1, OL_NO_ITEM
};

ChoiceGizmo Rpath = {
	NULL,
	"rpath",
	"",
	&rpathMenu,
	&dtSettings.rpath,
	ChangeCB,
};

static char path[PATH_LEN];
static InputGizmo Dpath = {
        NULL,
	"dpath",
	"Remote Folder:",
	path,
	&dtSettings.dpath,
	UNAMESIZE,
	ChangeCB,
};

static GizmoRec folder[] = {
	{ChoiceGizmoClass,	&Rpath},
};

static LabelGizmo Folder = {
	NULL,			/* HELP		*/
	"folder",		/* NAME		*/
	"",
	folder,			/* GIZMOARRAY	*/
	XtNumber(folder),	/* 	*/
	OL_FIXEDROWS,		/* LAYOUT	*/
	1,			/* MEASURE	*/
	0,			/* Arg array	*/
	0,			/* numArg	*/
	FALSE			/* ALIGNCAP	*/
};

static GizmoRec destiny[] = {
	{StaticTextGizmoClass,	&UUCP},
	{LabelGizmoClass,	&Folder},
};

static LabelGizmo Destiny = {
	NULL,			/* HELP		*/
	"destiny",		/* NAME		*/
	TXT_DELIVERY,
	destiny,		/* GIZMOARRAY	*/
	XtNumber(destiny),	/* 	*/
	OL_FIXEDROWS,		/* LAYOUT	*/
	1,			/* MEASURE	*/
	0,			/* Arg array	*/
	0,			/* numArg	*/
	FALSE			/* ALIGNCAP	*/
};

static SpaceGizmo space = {
	(Dimension)2, (Dimension)10
};

static GizmoRec PropGiz[] = {
	{LabelGizmoClass,	&Rhost},
	{InputGizmoClass,	&Recipient},
	{LabelGizmoClass,	&Destiny},
	{InputGizmoClass,	&Dpath},
};

static char title[40];
static PopupGizmo *	transferPopup = (PopupGizmo *)0;
static PopupGizmo TransferPopup = 
   { NULL, "_X_", title, &TransferMenu, PropGiz, XtNumber(PropGiz) };

static void
ExitMain()
{
	exit(0);
}

static void
EnablePathCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	static InputGizmo *igp;
	static Boolean first_time = True;
	if (first_time) {
		first_time = False;
		igp = (InputGizmo *)QueryGizmo(PopupGizmoClass,
				&TransferPopup,
				GetGizmoGizmo,
				"dpath");
	}

	XtMapWidget( ((InputGizmo*)igp)->captionWidget);
	OlMoveFocus( ((InputGizmo*)igp)->textFieldWidget, OL_IMMEDIATE, 0);
} /* EnablePathCB */

static void
DisablePathCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	static InputGizmo *igp;
	static Boolean first_time = True;
	if (first_time) {
		first_time = False;
		igp = (InputGizmo *)QueryGizmo(PopupGizmoClass,
				&TransferPopup,
				GetGizmoGizmo,
				"dpath");
	}

	XtUnmapWidget( ((InputGizmo*)igp)->captionWidget);
} /* DisablePathCB */

static void
ShowUUCPonly()
{
	GizmoRec * gp;
	static LabelGizmo *lgp;
	static Boolean first_time = True;
	if (first_time) {
		first_time = False;
		lgp = (LabelGizmo *)QueryGizmo(PopupGizmoClass,
				&TransferPopup,
				GetGizmoGizmo,
				"destiny");
	}

	gp = lgp->gizmos;
	XtManageChild( ((StaticTextGizmo*)gp->gizmo)->widget);
	gp++;
	XtUnmanageChild( ((LabelGizmo*)gp->gizmo)->captionWidget);
} /* ShowUUCPonly */

static void
ShowMethods()
{
	GizmoRec * gp;
	static LabelGizmo *lgp;
	static Boolean first_time = True;
	if (first_time) {
		first_time = False;
		lgp = (LabelGizmo *)QueryGizmo(PopupGizmoClass,
				&TransferPopup,
				GetGizmoGizmo,
				"destiny");
	}

	gp = lgp->gizmos;
	XtUnmanageChild( ((StaticTextGizmo *)gp->gizmo)->widget);
	gp++;
	XtManageChild( ((LabelGizmo *)gp->gizmo)->captionWidget);
} /* ShowMethods */


static char	login[UNAMESIZE];
static char 	osystem[UNAMESIZE];	/* origination system */
static char	target[PATH_MAX];
char	*operand;
char 	*dsystem;		/* destination system */
char 	*duser;			/* destination user */
char	*dpath;
Boolean	dpath_yes = False;
Boolean	qflag, tflag; 
Boolean opernd;

main( argc, argv )
int argc;
char *argv[];
{

	int		c, i;
	char *		userHome;
	stringll_t	*attr_list;

	Program = basename(argv[0]);
	root = InitializeGizmoClient(ClientName, ClientClass,
		Program,
		NULL, NULL,
		NULL, 0,
		&argc, argv,
		NULL,
		NULL,
		resources,
		XtNumber(resources),
		NULL,
		0,
		NULL,
		NULL,
		NULL);

	dpath = 0;
	dsystem = 0;
	tflag = qflag = False;

	while ((c = getopt( argc, argv, "pqr:" )) != -1) {
		switch ( c ) {
		default:
			rexit(1, TXT_OPTION, &c);
			break;
		case 'p':
			tflag = True;
			break;
		case 'q':
			qflag = True;
			break;
		case 'r':
			dsystem = optarg;
			break;
		}/*endswitch*/
	}/*endwhile*/

	if (optind < argc)
		operand = argv[optind];

	if (operand == 0) {
		rexit(1, TXT_SRC_NO, "");
	}

	if (dsystem == 0) {
		rexit(2, TXT_DEST_NO, "");
	} 

	sprintf(title, GGT(TXT_TITLE));

	/* get login name */
	if (GetUser(login) == (char *)NULL) {
		rexit(3, TXT_LOGIN_NO, "");
	}
	userHome = getenv("HOME");
	if (!userHome)
		userHome = "";

	(void)umask(022);

	/* create path name to node file */

	(void)sprintf(target, "%s/.node/%s", userHome, dsystem);

#ifdef debug
	(void)fprintf(stderr, "reading the attribute file: \"%s\"\n", target);

#endif
	/* parse the node file */

	if (AttrParse(target, &attr_list) == -1) {
		rexit(4, TXT_NODE_NO, target);
	}

	duser = GetAttrValue(attr_list, "DUSER");
	dpath = GetAttrValue(attr_list, "DPATH");
	GetNodeName(osystem);

#ifdef debug
	fprintf( stderr, "these values has been retrived from \"%s\" for \"%s\":\n",
		dsystem, login);
	fprintf( stderr, "DUSER=%s\nDPATH=%s\n\n",
		duser, dpath);
#endif
#ifdef LATER
	if (duser == (char *)NULL )
		rexit(5, TXT_NODE_NO, target);
#endif
	if (dpath != 0 && !qflag) {
		/* strip off leading spaces if any */
		while (*dpath == ' ') dpath++;
		if ((*dpath)!= '/')
			rexit(6,
			TXT_FOLDER_NO, dpath);
		else
			opernd = True;
	} else
		opernd = False;

	strcpy(ruser, duser);
	strcpy(dmach, dsystem);
	strcpy(path, dpath);
	dtSettings.rpath.previous_value = (XtPointer)opernd;
	CreatePopup(root);

	if (opernd && !qflag)
		EnablePathCB(root, 0, 0);
	else
		DisablePathCB(root, 0, 0);
	if (qflag)
		ShowUUCPonly();
	else
		ShowMethods();

	XtMainLoop();
} /*main*/

static void
Transfer()
{
	char	stderr_file[PATH_MAX] = "/var/tmp/transfer";
	char	*getenv();
	char	*xtop;
	char	exitp[BUFSIZ];
	char	basedir[PATH_MAX];
	char	*mkdirscmd;
	char	*cmdp;
	char 	tcpcmdline[BUFSIZ], uucpcmdline[BUFSIZ], mdcmdline[BUFSIZ];
	char	token[25];
	FILE	*stderr_p;
	int	status;
	int	i;
	int	sfd;
	Boolean uucpflag = False;

	/* create path name to the stderr file */

#ifdef later
	stderr_p = tmpfile();
#else
	stderr_p = fopen(stderr_file, "w");
#endif

	if ( stderr_p == (FILE *)NULL ) {
		rexit(7, TXT_EFILE_NO, stderr_file);
        } else {
		/* replace stderr with stderr_file */
		sfd = dup(2);
		(void)close(2);
		(void)dup(fileno(stderr_p));
		(void)fclose(stderr_p);
	}

	(void)fprintf(stderr, "**** NETWORK SERVICE REQUESTED ****\n");

	/* form the command line the destination system */
	strcpy( target, dpath );
	if ( qflag || !dpath_yes ) {
		/* form the UUCP command line */

		uucpflag = True;
		(void)sprintf(uucpcmdline,
			  "uuto -p -m  %s %s!%s 1>&2",
			  operand, dsystem, duser);
	} else { /* rcp was specified */
		/* form the TCP command line */

		(void)sprintf(tcpcmdline,
			  "/usr/bin/rcp -r %s %s:%s 1>&2",
			  operand, dsystem, target);
	}

	if (uucpflag) {
		cmdp = uucpcmdline;
	} else {
		cmdp = tcpcmdline;
	}

	if (tflag == True) {
		(void)fprintf(stdout, "running: %s\n", cmdp);
		(void)ExitMain();
	}
	/* put out a message to the user; the command may take a long time */
	SetPopupMessage(transferPopup, GGT(string_startRFT));
	/* need to simulate XtMainloop here */
	OlUpdateDisplay(transferPopup->message);

	status = system(cmdp);

	if (status != 0) {

	    if (uucpflag) {
		switch(i= WEXITSTATUS(status)) {
		case 0:
			BringDownPopup(transferPopup);
			break;
		case 8:
			SetPopupMessage(transferPopup, GGT(string_badSystem));
			break;
		case 12:
			SetPopupMessage(transferPopup, GGT(string_badFileName));
			return;
			break;
		case 13:
			SetPopupMessage(transferPopup, GGT(string_badFile));
			break;
		case 24:
			SetPopupMessage(transferPopup, GGT(string_folderDeny));
			break;
		case 25:
			SetPopupMessage(transferPopup, GGT(string_folderEmpty));
			break;
		case 28:
			NotifyUser(root, GGT(string_partialFail));
			MailUser(login, GGT(TXT_MAIL), stderr_file);
			break;
		default:
#ifdef debug
			(void)fprintf(stderr,"at %d in %s exitcode = %d\n",
					__LINE__, __FILE__, i);
#endif
			SetPopupMessage(transferPopup, GGT(string_unknownFail));
			break;
		}
	    } else {
		/* the commmand failed on the local system */
		/* notify user and don't exit */
		NotifyUser(root, GGT(TXT_FAIL));
		MailUser(login, GGT(TXT_MAIL), stderr_file);
	    }
	    /* put back the stderr as before */
	    (void)close(2);
	    (void)dup(sfd);
	    (void)close(sfd);
	    (void)unlink(stderr_file);
#ifdef debug
	    (void)sprintf(exitp,
		    "\ncommand (%s) failed (%s): exit code (%d)",
		    cmdp, "network service failure",
		    ((status>>8) && 0xff));
	    (void)fprintf(stderr, "%s\n",exitp);
#endif
	    return;
	} else {
		/* put back the stderr as before */
		(void)close(2);
		(void)dup(sfd);
		(void)close(sfd);
		(void)unlink(stderr_file);
#ifdef debug
		(void)sprintf(exitp,
			"File transfer completed (%s)",
				uucpflag ?"uucp":"tcp");
		(void)fprintf(stderr, "%s\n",exitp);
#endif
		/* notify user and exit */
		if (uucpflag)
			rexit(0, GGT(TXT_QUEUED), "");
		else
			rexit(0, GGT(TXT_SUCCESS), "");
	}
} /* Transfer */

static int
GetExitValue(fin,token)
FILE	*fin;
char	*token;
{
	char	buf[BUFSIZ];
	char	*namep;
	int	exit_val;
	int	not_found;
	int	save_pos;
	int	len;

	exit_val  = 0;
	not_found = 0;
	len       = strlen(token);

	save_pos = fseek(fin, 0L, SEEK_CUR);
	rewind(fin);
	while(fgets(buf, sizeof buf, fin) != NULL) {

		if ( (namep = strstr(buf, token))
				!= (char *)NULL &&
				isdigit((int)(*(namep + len))) ) {
			exit_val = atoi(namep + len);
			not_found = 1;
		}

		if (not_found == 0)
			continue;
		else
			break;
	}
	fseek(fin, save_pos, SEEK_SET);
	return(exit_val);
} /* GetExitValue */

/*
 * CreatePopup
 *
 * This procedure creates the TransferPopup dialog window and maps it.
 *
 */

static void 
CreatePopup(Widget Shell)
{
	Window		another_window;
	Widget popup;
	ApplicationName = GGT (ClientName);
	SET_HELP(AppHelp);
	transferPopup = &TransferPopup;
	popup = CreateGizmo(Shell, PopupGizmoClass, &TransferPopup, NULL, 0);
	XtAddCallback (
		popup,
		XtNpopdownCallback,
		ExitMain,
		(XtPointer)0
		);
	XtVaSetValues (
		popup,
		XtNmappedWhenManaged,           (XtArgVal) False,
		XtNwmProtocolInterested,        (XtArgVal) OL_WM_DELETE_WINDOW,
		0
	);

	XtRealizeWidget(popup);
	another_window = DtSetAppId (
				XtDisplay(popup),
				XtWindow(popup),
				dsystem);
	if (another_window != None) {
		XMapWindow(XtDisplay(popup), another_window);
		XRaiseWindow(XtDisplay(popup), another_window);
		XFlush(XtDisplay(popup));
		ExitMain();
	}

	OlAddCallback (
		popup,
		XtNwmProtocol, WindowManagerEventHandler,
		(XtPointer) 0
	);


	MapGizmo(PopupGizmoClass, &TransferPopup);

} /* end of CreatePopup */
/*
 * TransferCB
 *
 *
 * The callback procedure is called when any of the buttons in the menu bar
 * of the TransferPopup dialog are selected.  The callback switched on the
 * index of the flat button in the menu bar and either:
 * Retrieves help for the user.
 * 
 */

static void
TransferCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	OlFlatCallData *	p = (OlFlatCallData *)call_data;
	Widget			shell;
	static char		buf[BUF_SIZE];
	DTSetting 		s;

	shell = GetPopupGizmoShell(transferPopup);

	switch (p->item_index) {
	    case TransferApply:
		ManipulateGizmo (
			(GizmoClass)&PopupGizmoClass,
			transferPopup,
			GetGizmoValue
		);
		s.duser = (Setting *)QueryGizmo (
			PopupGizmoClass, transferPopup,
			GetGizmoSetting, "recipient"
		);
		s.rpath = (Setting *)QueryGizmo (
			PopupGizmoClass, transferPopup,
			GetGizmoSetting, "rpath"
		);
		s.dpath = (Setting *)QueryGizmo (
			PopupGizmoClass, transferPopup,
			GetGizmoSetting, "dpath"
		);
		  
		duser = (char *) s.duser->current_value;
		dpath_yes = (Boolean)s.rpath->current_value;
		dpath = (char *) s.dpath->current_value;

		/* strip off leading spaces if any */
		while (*duser == ' ') duser++;

		if (strcmp(duser, "") == 0) {
			SetPopupMessage(transferPopup, GGT(TXT_LOGIN_NO));
			return;
		}
		/* strip off leading spaces if any */
		while (*dpath == ' ') dpath++;

		if ((strcmp(dpath, "") == 0 || (*dpath) != '/') && dpath_yes) {
			SetPopupMessage(transferPopup, GGT(TXT_FOLDER_NO));
			return;
		}
#ifdef debug
		fprintf(stderr, "duser=%s\nrapth=%d\ndpath=%s\n",
			duser,
			dpath_yes,
			dpath);
#endif
		/* call the transfer routine */
		Transfer();
		break;
	case TransferReset:
		ManipulateGizmo (
			(GizmoClass)&PopupGizmoClass,
			transferPopup,
			ResetGizmoValue
		);
		SetPopupMessage(transferPopup, "");
		break;
	case TransferCancel:
		XtPopdown(shell);
		break;
	default:
		(void)fprintf(stderr,"at %d in %s\n", __LINE__, __FILE__);
		break;
    }
} /* TransferCB */

static void
ChangeCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
#ifdef debug
	fprintf(stderr, "Something was touched\n");
#endif
	changed = True;
}

static void
WindowManagerEventHandler(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	OlWMProtocolVerify *	p = (OlWMProtocolVerify *)call_data;

	switch (p->msgtype) {
	case OL_WM_DELETE_WINDOW:
#ifdef debug
		fprintf (stdout, "Delete yourself\n");
#endif
		ExitMain ();
		break;

	case OL_WM_SAVE_YOURSELF:
		/*
		 *	Do nothing for now; just respond.
		 */
#ifdef debug
		fprintf (stdout, "Save yourself\n");
#endif
		ExitMain ();
		break;

	default:
#ifdef debug
		fprintf (stdout, "Default action\n");
#endif
		OlWMProtocolAction(wid, p, OL_DEFAULTACTION);
		break;
	}
} /* WindowManagerEventHandler */
