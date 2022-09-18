/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtadmin:dialup/dial.c	1.22"
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
#include "uucp.h"
#include "error.h"

#define	XTERM	"xterm"
#define	CU	"cu"
#define	TXT_TITLE	"DialupMgr:000" FS "Dialup Setup: Quick Dial"

static char	ctype[16];
static void	DialCB();
static void	RestartQD();
static void	ReapChild();

extern void	HelpCB();
extern int	ExecProgram();

typedef struct _QuickDial {
	PopupGizmo *	dialPrompt;
	Widget		popup;
} QuickDial, *QuickDialPtr;

QuickDialPtr	qd;
	
ExecItem program = {
	RestartQD, NULL, NULL, 0, 0, { XTERM, "-E", CU, NULL}
};
 
static HelpText AppHelp = {
    title_dial, HELP_FILE, help_dial,
};

typedef enum {
	DialApply, DialReset, DialCancel, DialHelp
} DialMenuItemIndex;

static MenuItems  DialMenuItems[] = {
	{(XA)True, label_dial, mnemonic_dial},
	{(XA)True, label_reset, mnemonic_reset},
	{(XA)True, label_cancel, mnemonic_cancel},
	{(XA)True, label_help, mnemonic_help, NULL, HelpCB, (char *)&AppHelp },
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

static PopupGizmo QuickDialPrompt = {
	NULL,
	"dial",
	TXT_TITLE,
	&DialMenu,
	Dials,
	XtNumber(Dials),
};

/*
 * QuickDialCB
 *
 */

extern void
QuickDialCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	String type;
	InputGizmo *gp;
	static Boolean	first_time = True;

	ClearFooter(df->footer);
	/* nothing selected, just return */
	if (df->select_op == (DmObjectPtr) NULL) {
		FooterMsg(df->footer, "%s", GGT(string_noSelect));
		return;
	} else
		type = ((DeviceData*)(df->select_op->objectdata))->modemFamily;

	if (qd->dialPrompt == NULL)
	{
		sigset(SIGCHLD, ReapChild);
		if (qd == NULL) {
			SET_HELP(AppHelp);
			qd = (QuickDial *)XtMalloc (sizeof(QuickDial));
		}
		qd->dialPrompt = CopyGizmo(PopupGizmoClass, &QuickDialPrompt);
		qd->popup = CreateGizmo(w, PopupGizmoClass, qd->dialPrompt, NULL, 0);
		df->QDPopup = qd->popup;
	}
	SetPopupMessage(qd->dialPrompt, "");
	MapGizmo(PopupGizmoClass, qd->dialPrompt);
	if (first_time) {
		first_time = False;
		gp = (InputGizmo *)QueryGizmo(PopupGizmoClass,
				qd->dialPrompt,
				GetGizmoGizmo,
				"phone");
		df->w_acu = gp->captionWidget;
	}

	if (strcmp(type,"datakit") == 0) {
		strcpy(ctype, "-cDK");
		if (XtIsRealized(df->w_acu))
			XtUnmapWidget(df->w_acu);
		else
			SetValue(df->w_acu, XtNmappedWhenManaged, False, NULL);
	} else if (strcmp(type,"direct") == 0) {
		strcpy(ctype, "-cDirect");
		if (XtIsRealized(df->w_acu))
			XtUnmapWidget(df->w_acu);
		else
			SetValue(df->w_acu, XtNmappedWhenManaged, False, NULL);
	} else {
		strcpy(ctype, "-cACU");
		if (XtIsRealized(df->w_acu))
			XtMapWidget(df->w_acu);
		else
			SetValue(df->w_acu, XtNmappedWhenManaged, TRUE, NULL);
	}
} /* QuickDialCB */

static
void DialCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	register i;
	char	speed[BUFSIZ];
	char 	line[BUFSIZ];
	OlFlatCallData * p = (OlFlatCallData *)call_data;
	PopupGizmo	* popup = qd->dialPrompt;
	Setting *fp;
	Setting *sp;
	Setting *pp;
	Setting *cp;
	Setting *dp;
	String  port = ((DeviceData*)(df->select_op->objectdata))->portNumber;
	String  type = ((DeviceData*)(df->select_op->objectdata))->modemFamily;
	Boolean acu = False;
	int	n, portNumber;

	if (!strncmp(port, "com", 3)) {
		sscanf(port, "com%d", &portNumber);
		sprintf(line, "-ltty%.2d", portNumber-1);
	} else	sprintf(line, "-l%s", port);
	switch (p->item_index) {
	case DialApply:
		ClearFooter(df->footer); /* clear mesaage area */
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
			SetPopupMessage(qd->dialPrompt, GGT(string_forkFail));
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
		break;
	default:
		(void)fprintf(stderr,"at %d in %s\n", __LINE__, __FILE__);
		break;
	}

} /* DialCB */

static void
RestartQD()
{
	
	ClearFooter(df->footer); /* clear mesaage area */
	/* probably need to sensitize the button */
	switch (program.exit_code) {
		case 0:
			SetPopupMessage(qd->dialPrompt, "");
			return;
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
