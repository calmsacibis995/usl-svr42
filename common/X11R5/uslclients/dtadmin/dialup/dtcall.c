/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtadmin:dialup/dtcall.c	1.11"
#endif

#include <stdio.h>
#include <wait.h>
#include <signal.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <OpenLook.h>
#include <FButtons.h>
#include <ControlAre.h>
#include <PopupWindo.h>

#include <Gizmos.h>
#include <MenuGizmo.h>
#include <PopupGizmo.h>
#include <InputGizmo.h>
#include <ChoiceGizm.h>
#include "dtcopy.h"
#include "uucp.h"
#include "error.h"


/*
 * DO NOT REMOVE.  It is used to overwrite the default help
 * file set in the error.h file.
 */
#ifdef	HELP_FILE
#undef	HELP_FILE
#endif
#define HELP_FILE	"dtadmin/qdial.hlp"

#define	XTERM	"xterm"
#define	CU	"cu"

extern void	HelpCB();
extern int	ExecProgram();
extern char *	GetAttrValue();
extern	int	AttrParse();
extern	void	NotifyUser ();
extern	void	rexit();

static	void	CreatePopup();
static	void	DialCB();
static void	DialCB();
static	void	ExitMain();
static	void	WindowManagerEventHandler();
static void	RestartQD();
static void	ReapChild();

Widget	root;
char 	*port;
char	*type;
char	ctype[16];

#define TXT_TITLE		"dtcall:1" FS "Call System: Properties"
#define FormalClientName   	TXT_TITLE

#define ClientName		"dtcall:2" FS "Call System"
#define ClientClass		"dtcall"

#define TXT_DEV_NO		"dtcall:10" FS "device not specified"
#define TXT_LOGIN_NO		"dtcall:11" FS "unable to determine user login home"
#define TXT_NODE_NO		"dtcall:12" FS "unable to load properties"

char *		ApplicationName;
char *		Program;

typedef struct _QuickDial {
	PopupGizmo *	dialPrompt;
	Widget		popup;
} QuickDial, *QuickDialPtr;

QuickDialPtr	qd;
	
ExecItem program = {
	RestartQD, NULL, NULL, 0, 0, { XTERM, "-E", CU, NULL}
};

static HelpText AppHelp = {
    title_dial, HELP_FILE, help_dtcall,
};

typedef enum {
	DialApply, DialReset, DialCancel, DialHelp
} DialMenuItemIndex;

static MenuItems  DialMenuItems[] = {
	{(XA)True, label_dial, mnemonic_dial},
	{(XA)True, label_reset, mnemonic_reset},
	{(XA)True, label_cancel, mnemonic_cancel},
	{(XA)True, label_help, mnemonic_help, NULL, HelpCB, (char*)&AppHelp },
	{ 0 }
};

static MenuGizmo DialMenu = {
	NULL, "dial", "_X_", DialMenuItems, DialCB, NULL, CMD, OL_FIXEDROWS, 1, 0
};

#define MAXPHONE 18

typedef struct _Input {
	Setting number;
} Input;

static Input Number = {""};

static InputGizmo phoneField = {
        NULL,
	"phone",
	label_phoneOrName,
	"",
	&Number.number,
	MAXPHONE,
	(void (*)())0,
};

typedef enum {
	SpeedAny, Speed38400, Speed19200, Speed9600,
	Speed4800, Speed2400, Speed1200, Speed300,
} SpeedMenuIndex;

static MenuItems  SpeedItems[] = {
	{(XA)True,	label_any,	mnemonic_any},
	{(XA)True,	label_b38400,	mnemonic_b38400},
	{(XA)True,	label_b19200,	mnemonic_b19200},
	{(XA)True,	label_b9600,	mnemonic_b9600},
	{(XA)True,	label_b4800,	mnemonic_b4800},
	{(XA)True,	label_b2400,	mnemonic_b2400},
	{(XA)True,	label_b1200,	mnemonic_b1200},
	{(XA)True,	label_b300,	mnemonic_b300},
	{ 0 }
};

static MenuGizmo SpeedMenu = {
	NULL, "speed", "_X_", SpeedItems, NULL, NULL, EXC,
        OL_FIXEDROWS,	/* Layout type	*/
        2,		/* Measure	*/
        OL_NO_ITEM	/* Default item	*/
};

typedef enum {
	ParityNone, ParityEven, ParityOdd
} ParityMenuIndex;

static MenuItems  ParityItems[] = {
	{(XA)True,	label_none,	mnemonic_none},
	{(XA)True,	label_even,	mnemonic_even},
	{(XA)True,	label_odd,	mnemonic_odd},
	{ 0 }
};
static MenuGizmo ParityMenu = {
	NULL, "parity", "_X_", ParityItems, NULL, NULL, EXC
};

typedef enum {
	CharSize7, CharSize8
} CharSizeMenuIndex;

static MenuItems  CharSizeItems[] = {
	{(XA)True,	label_seven,	mnemonic_seven},
	{(XA)True,	label_eight,	mnemonic_eight},
	{ 0 }
};

static MenuGizmo CharSizeMenu = {
	NULL, "charSize", "_X_", CharSizeItems, NULL, NULL, EXC
};

typedef enum {
	DuplexFull, DuplexHalf
} DuplexMenuIndex;

static MenuItems  DuplexItems[] = {
	{(XA)True,	label_full,	mnemonic_full},
	{(XA)True,	label_half,	mnemonic_half},
	{ 0 }
};

static MenuGizmo DuplexMenu = {
	NULL, "duplex", "_X_", DuplexItems, NULL, NULL, EXC
};

typedef struct _MySettings {
	Setting speed;
	Setting parity;
	Setting charSize;
	Setting duplex;
} MySettings;

static MySettings MySetting;

static ChoiceGizmo SpeedChoice = {
	NULL,
	"speed",
	label_speed,
	&SpeedMenu,
	&MySetting.speed,
};

static ChoiceGizmo ParityChoice = {
	NULL,
	"parity",
	label_parity,
	&ParityMenu,
	&MySetting.parity,
};

static ChoiceGizmo CharSizeChoice = {
	NULL,
	"charSize",
	label_charSize,
	&CharSizeMenu,
	&MySetting.charSize,
};

static ChoiceGizmo DuplexChoice = {
	NULL,
	"duplex",
	label_duplex,
	&DuplexMenu,
	&MySetting.duplex,
};

static GizmoRec Dials[] = {
	{ InputGizmoClass, &phoneField },
	{ ChoiceGizmoClass, &SpeedChoice },
	{ ChoiceGizmoClass, &ParityChoice },
	{ ChoiceGizmoClass, &CharSizeChoice },
	{ ChoiceGizmoClass, &DuplexChoice },
};

static	char	title[40];
static PopupGizmo QuickDialPrompt = {
	NULL,
	"dial",
	title,
	&DialMenu,
	Dials,
	XtNumber(Dials),
};

static void
ExitMain()
{
	exit(0);
}

/*
 * CreatePopup
 *
 */

static void
CreatePopup(w)
Widget    w;
{

	InputGizmo *gp;
	Widget w_acu;
	static Boolean	first_time = True;
	Window		another_window;
	if (qd->dialPrompt == NULL)
	{
		sigset(SIGCHLD, ReapChild);
		if (qd == NULL) {
			ApplicationName = GGT (ClientName);
			SET_HELP(AppHelp);
			qd = (QuickDial *)XtMalloc (sizeof(QuickDial));
		}
		qd->dialPrompt = CopyGizmo(PopupGizmoClass, &QuickDialPrompt);
		qd->popup = CreateGizmo(w, PopupGizmoClass, qd->dialPrompt, NULL, 0);
	}
	XtVaSetValues (
		qd->popup,
		XtNmappedWhenManaged,           (XtArgVal) False,
		XtNwmProtocolInterested,        (XtArgVal) OL_WM_DELETE_WINDOW,
		0
	);

	XtRealizeWidget(qd->popup);
	another_window = DtSetAppId (
				XtDisplay(qd->popup),
				XtWindow(qd->popup),
				port);
	if (another_window != None) {
		XMapWindow(XtDisplay(qd->popup), another_window);
		XRaiseWindow(XtDisplay(qd->popup), another_window);
		XFlush(XtDisplay(qd->popup));
		ExitMain();
	}

	OlAddCallback (
		qd->popup,
		XtNwmProtocol, WindowManagerEventHandler,
		(XtPointer) 0
	);

	MapGizmo(PopupGizmoClass, qd->dialPrompt);
	gp = (InputGizmo *)QueryGizmo(PopupGizmoClass,
			qd->dialPrompt,
			GetGizmoGizmo,
			"phone");
	w_acu = gp->captionWidget;
	if (strcmp(type,"datakit") == 0) {
		strcpy(ctype, "-cDK");
		if (XtIsRealized(w_acu))
			XtUnmapWidget(w_acu);
		else
			SetValue(w_acu, XtNmappedWhenManaged, False, NULL);
	} else if (strcmp(type,"direct") == 0) {
		strcpy(ctype, "-cDirect");
		if (XtIsRealized(w_acu))
			XtUnmapWidget(w_acu);
		else
			SetValue(w_acu, XtNmappedWhenManaged, False, NULL);
	} else {
		strcpy(ctype, "-cACU");
		if (XtIsRealized(w_acu))
			XtMapWidget(w_acu);
		else
			SetValue(w_acu, XtNmappedWhenManaged, TRUE, NULL);
	}
} /* CreatePopup */

static void
DialCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	register i;
	char 	line[BUFSIZ], speed[BUFSIZ];
	OlFlatCallData * p = (OlFlatCallData *)call_data;
	PopupGizmo	* popup = qd->dialPrompt;
	Setting *fp;
	Setting *sp;
	Setting *pp;
	Setting *cp;
	Setting *dp;
	Boolean acu = False;
	int	n, portNumber;

	if (!strncmp(port, "com", 3)) {
		sscanf(port, "com%d", &portNumber);
		sprintf(line, "-ltty%.2d", portNumber-1);
	} else	sprintf(line, "-l%s", port);
	switch (p->item_index) {
	case DialApply:
		SetPopupMessage(qd->dialPrompt, "");
		ManipulateGizmo(PopupGizmoClass, popup, GetGizmoValue);
		fp = (Setting *)QueryGizmo (popup->gizmos[0].gizmo_class,
				popup->gizmos[0].gizmo,
				GetGizmoSetting,
				"phone");
		sp = (Setting *)QueryGizmo (popup->gizmos[1].gizmo_class,
				popup->gizmos[1].gizmo,
				GetGizmoSetting,
				"speed");
		pp = (Setting *)QueryGizmo (popup->gizmos[2].gizmo_class,
				popup->gizmos[2].gizmo,
				GetGizmoSetting,
				"parity");
		cp = (Setting *)QueryGizmo (popup->gizmos[3].gizmo_class,
				popup->gizmos[3].gizmo,
				GetGizmoSetting,
				"charSize");
		dp = (Setting *)QueryGizmo (popup->gizmos[4].gizmo_class,
				popup->gizmos[4].gizmo,
				GetGizmoSetting,
				"duplex");
         /*
          * data check
          */

		/* if the connection is acu, check the phone number */
		if (strcmp(type, "datakit") != 0 && strcmp(type, "direct") != 0) {
			if ( strlen(fp->current_value) !=
			     strspn(fp->current_value, "0123456789,=-") ) {
			    /* it's not a legimtimate telno */
			    
			    SetPopupMessage(qd->dialPrompt, GGT(string_badPhone));
			    return;
			}
			acu = True;
		}
		/*
		* if OK, pass it to the cu command and store it for the future
		*/

		for (i = 3; i < 11; i++)
			if (program.exec_argv[i] != NULL) {
				free (program.exec_argv[i]);
				program.exec_argv[i] = NULL;
			}
		program.exec_argv[3] = strdup(line);
		i = 4;
		program.exec_argv[i++] = strdup(ctype);
		/* check if speed is "Any".  If not, get the speed string */ 
		if (n = (int) sp->current_value) {
			sprintf(speed, "-s%s", GGT(SpeedItems[n].label));
			program.exec_argv[i++] = strdup(speed);
		}
		if((int)cp->current_value)
			program.exec_argv[i++] = strdup("-b8");
		
		if((int)dp->current_value)
			program.exec_argv[i++] = strdup("-h");
		if(n = (int)pp->current_value)
			program.exec_argv[i++] = strdup((n==1)? "-e" : "-o");
		if (acu == True)
			program.exec_argv[i] = strdup(fp->current_value);
		if(ExecProgram(&program) == -1)
			SetPopupMessage(qd->dialPrompt, "Can not create a new process");
		else {
			SetPopupMessage(qd->dialPrompt, GGT(string_startCU));
			BringDownPopup(qd->popup);
		}
		ManipulateGizmo(PopupGizmoClass, popup, ApplyGizmoValue);
		break;
	case DialReset:
		ManipulateGizmo(PopupGizmoClass, popup, ResetGizmoValue);
		break;
	case DialCancel:
		BringDownPopup(qd->popup);
		ExitMain();
		break;
	default:
		(void)fprintf(stderr,"at %d in %s\n", __LINE__, __FILE__);
		break;
	}

} /* DialCB */

main( argc, argv )
int argc;
char *argv[];
{

	char	stderr_file[PATH_MAX];
	char	*home;
	char	target[UNAMESIZE];
	char	*device;
	int  	c, x, i;
	FILE	*stderr_p;
	stringll_t	*attr_list;

	Program = basename(argv[0]);
	root = InitializeGizmoClient(ClientName, ClientClass,
		Program,
		NULL, NULL,
		NULL, 0,
		&argc, argv,
		NULL,
		NULL,
		NULL,
		0,
		NULL,
		0,
		NULL,
		NULL,
		NULL);

	/* parse command line */

	port = 0;
	type = 0;
	device = 0;

	if (argc != 2)
		rexit(2, TXT_DEV_NO, "");
	else
		device = argv[1];

	sprintf(title, GGT(TXT_TITLE));

	/* get login home */
	home = getenv("HOME");
	if (!home)
		home = "";

	(void)umask(022);

	/* create path name to the property file */

	(void)sprintf(target, "%s/.port/%s", home, device);

#ifdef debug
	(void)fprintf(stderr, "reading the attribute file: \"%s\"\n", target);

#endif
	/* parse the attr.node file */

	if (AttrParse(target, &attr_list) == -1) {
		rexit(4, TXT_NODE_NO, target);
	}

	port = GetAttrValue(attr_list, "PORT");
	type = GetAttrValue(attr_list, "TYPE");

#ifdef debug
	fprintf( stderr, "PORT=%s\nTYPE=%s\n",
		port, type);
#endif
	if (port == (char *)NULL || type == (char*)NULL)
		rexit(5, TXT_NODE_NO, target);

	CreatePopup(root);

	XtMainLoop();

}/* main */

static void
RestartQD()
{
	
	/* probably need to sensitize the button */
	switch (program.exit_code) {
		case 0:
			SetPopupMessage(qd->dialPrompt, "");
			ExitMain();
		case 29:
			SetPopupMessage(qd->dialPrompt, GGT(string_killedCU));
			break;
		case 39:
			SetPopupMessage(qd->dialPrompt, GGT(string_badPhone));
			break;
		case 40:
			SetPopupMessage(qd->dialPrompt, GGT(string_usageCU));
			break;
		case 41:
			SetPopupMessage(qd->dialPrompt, GGT(string_exceed58));
			break;
		case 43:
			SetPopupMessage(qd->dialPrompt, GGT(string_connectFail));
			break;
		case 45:
			SetPopupMessage(qd->dialPrompt, GGT(string_lostConnect));
			break;
		case 47:
			SetPopupMessage(qd->dialPrompt, GGT(string_lostCarrier));
			break;
		case 49:
			SetPopupMessage(qd->dialPrompt, GGT(string_dialFail));
			break;
		case 50:
			SetPopupMessage(qd->dialPrompt, GGT(string_scriptFail));
			break;
		case 51:
			SetPopupMessage(qd->dialPrompt, GGT(string_deviceFail));
			break;
		case 52:
			SetPopupMessage(qd->dialPrompt, GGT(string_noDevice));
			break;
		case 53:
			SetPopupMessage(qd->dialPrompt, GGT(string_noSystem));
			break;
		default:
			SetPopupMessage(qd->dialPrompt, GGT(string_unknownFail));
			break;
		}
		XtPopup(qd->popup, XtGrabNone);
} /* RestartQD */

/*
 * ReapChild - the child terminated, so find out why and set the
 * timmer to call the retry procedure.
 */
static void
ReapChild()
{
	int	pid;
	int	status;

	if((pid = wait(&status)) == -1)
		return;
	if(program.pid == pid) {
		program.exit_code = WEXITSTATUS(status);
		XtAddTimeOut(0L, (XtTimerCallbackProc)RestartQD, NULL);
		return;
	}
	fprintf(stderr, "Pid #%d???\n", pid);
} /* ReapChild */

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
