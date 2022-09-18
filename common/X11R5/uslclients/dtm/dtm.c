/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:dtm.c	1.117"

/******************************file*header********************************

    Description:
	This file contains the source code for dtm "main".
*/
						/* #includes go here	*/
#include <signal.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <Xol/OpenLookP.h>
#include <Xol/array.h>			/* for stale folders array */
#include <memutil.h>
#include <Gizmo/Gizmos.h>		/* for Exit Notice */
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/ModalGizmo.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"
#include "wb.h"
#include "olwsm/dtprop.h"		/* for DEFAULT's */
#include "dm_exit.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Public  Procedures 
*/
					/* private procedures		*/
static void	AtExit(void);
static void	CancelExitCB(Widget, XtPointer, XtPointer);
static void	DtmExit(void);
static void	Exit(int error_code, char * message);
static void	ExitCB(Widget, XtPointer, XtPointer);
static void	ExitHelpCB(Widget, XtPointer, XtPointer);
static void	SignalHandler(int signum);
static void     ShutdownHelpCB(Widget, XtPointer, XtPointer);

					/* public procedures		*/

extern void     ServiceEvent OL_ARGS((XEvent *event));
void		DmPromptExit(Boolean);
void		DmShutdownPrompt(void);

/******************************global*macros*******************************/

#define CREATE_NOTICE(gizmo) \
    if ((gizmo)->shell == NULL) \
    { \
	if (DummyExitShell == NULL) \
	    DummyExitShell = \
		XtCreateApplicationShell("", applicationShellWidgetClass, \
					 NULL, 0); \
	(void)CreateGizmo(DummyExitShell, ModalGizmoClass, gizmo, NULL, 0); \
    }

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

#ifndef NOWSM
#define RC			"/.olinitrc"
#define TIME_TO_WAIT_FOR_WM	10

extern Widget	InitShell;
extern Widget	handleRoot;

extern void	BusyPeriod(Widget w, Boolean busy);
extern Widget	CreateHandleRoot(void);
extern void	DestroyPropertyPopup(void);
extern int	DmInitWSM(int, String *);
extern void	ExecRC(char * path);
extern char *	GetPath(char * name);
extern void	SetWSMBang(Display *, Window, unsigned long);
extern void	WSMExit(void);
#endif

/* Define exit codes */
#define START_UP_ERROR		41
#define SHUTDOWN_EXIT_CODE	42	/* also in xinit.c */
#define OPEN_DT_ERROR		43
#define INIT_WB_ERROR		44
#define INIT_HM_ERROR		45
#define INIT_HD_ERROR		46
#define INIT_WSM_ERROR		47

/* DummyExitShell: a dummy application shell is created (but not realized)
   and used as the parent to the exit and shutdown notices so that they can
   be positioned in the center of the screen (the alogrithm for positioning a
   notice puts it in the center of the screen when its parent is unrealized).
   (see CREATE_NOTICE above)
*/
static Widget		DummyExitShell;

static const char * const	DmAppName  = "DesktopMgr";
static const char * const	DmAppClass = "DesktopMgr";

/* Define global desktop structure (should be 1-per display) */
DmDesktopPtr		Desktop = NULL;

/* Define variable for exit code.  Used by olwsm/wsm.c:WSMExit */
int DtmExitCode;

/****************************widget*resources*****************************
    Define resource list for dtm options
*/
/* Define resource names */
static char XtNshowHiddenFiles[]	= "showHiddenFiles";
static char XtCCols[]			= "Cols";
static char XtCRows[]			= "Rows";
static char XtCShowFullPaths[]		= "ShowFullPaths";
static char XtCShowHiddenFiles[]	= "ShowHiddenFiles";
static char XtCSyncInterval[]		= "SyncInterval";
static char XtCTreeDepth[]		= "TreeDepth";
static char XtCWbSuspend[]		= "WbSuspend";
static char XtCWbCleanUpMethod[]	= "WbCleanUpMethod";
static char XtCWbTimerInterval[]	= "WbTimerInterval";
static char XtCWbTimerUnit[]		= "WbTimerUnit";
static char XtCHelpKeyColor[]		= "HelpKeyColor";

#define OFFSET(field)	XtOffsetOf(DmOptions, field)

static XtResource resources[] = {
  { XtNfolderCols, XtCCols, XtRUnsignedChar, sizeof(u_char),
    OFFSET(folder_cols), XtRImmediate, (XtPointer)DEFAULT_FOLDER_COLS },

  { XtNfolderRows, XtCRows, XtRUnsignedChar, sizeof(u_char),
    OFFSET(folder_rows), XtRImmediate, (XtPointer)DEFAULT_FOLDER_ROWS },

  { XtNfontGroup, XtCFontGroup, XtROlFontList, sizeof(OlFontList *),
    OFFSET(font_list), XtRImmediate, NULL},

  { XtNgridWidth, XtCWidth, XtRDimension, sizeof(Dimension),
    OFFSET(grid_width), XtRImmediate, (XtPointer)DEFAULT_GRID_WIDTH },

  { XtNgridHeight, XtCHeight, XtRDimension, sizeof(Dimension),
    OFFSET(grid_height), XtRImmediate, (XtPointer)DEFAULT_GRID_HEIGHT},

  { XtNhelpKeyColor, XtCHelpKeyColor, XtRPixel, sizeof(Pixel),
    OFFSET(help_key_color), XtRString, (XtPointer)DEFAULT_HELP_KEY_COLOR },

  { XtNshowFullPaths, XtCShowFullPaths, XtRBoolean, sizeof(Boolean),
    OFFSET(show_full_path), XtRImmediate, (XtPointer)DEFAULT_SHOW_PATHS },

  { XtNshowHiddenFiles, XtCShowHiddenFiles, XtRBoolean, sizeof(Boolean),
    OFFSET(show_hidden_files), XtRImmediate, False },

  /* NOTE: should be unsigned long.  Need converter */
  { XtNsyncInterval, XtCSyncInterval, XtRInt, sizeof(int),
    OFFSET(sync_interval), XtRImmediate, (XtPointer)DEFAULT_SYNC_INTERVAL },

  { XtNtreeDepth, XtCTreeDepth, XtRUnsignedChar, sizeof(u_char),
    OFFSET(tree_depth), XtRImmediate, (XtPointer)DEFAULT_TREE_DEPTH },

  { XtNwbSuspend, XtCWbSuspend, XtRBoolean, sizeof(Boolean),
    OFFSET(wb_suspend), XtRImmediate, (XtPointer)DEFAULT_WB_SUSPEND },

  { XtNwbCleanUpMethod, XtCWbCleanUpMethod, XtRInt, sizeof(int),
    OFFSET(wb_cleanUpMethod), XtRImmediate, (XtPointer)DEFAULT_WB_CLEANUP },

  { XtNwbTimerInterval, XtCWbTimerInterval, XtRInt, sizeof(int),
    OFFSET(wb_timer_interval), XtRImmediate,
    (XtPointer)DEFAULT_WB_TIMER_INTERVAL},

  { XtNwbTimerUnit, XtCWbTimerUnit, XtRInt, sizeof(int),
    OFFSET(wb_timer_unit), XtRImmediate, (XtPointer)DEFAULT_WB_TIMER_UNIT },
};

#undef OFFSET

/****************************procedure*header*****************************
    main-
*/
void
main(int argc, char * argv[])
{
    struct utsname	unames;
    Display *		dpy;
    Window		win;
    CharArray *		dummy_array;

#ifdef MEMUTIL
    InitializeMemutil();
#endif

    /* don't buffer stdout output */
    setbuf(stdout, NULL);

    /* make sure XWINHOME is set */
    if (!getenv("XWINHOME"))
	putenv("XWINHOME=/usr/X");

#ifndef NOWSM
    setpgrp();
#endif

    if ((Desktop = (DmDesktopPtr)CALLOC(1, sizeof(DmDesktopRec))) == NULL)
    {
	/* Don't use "Exit()" since there's no InitShell, yet */
	Dm__VaPrintMsg(TXT_MEM_ERR);
	exit(START_UP_ERROR);
	/* NOTREACHED */
    }

    /* enable GUI switch */
    OlToolkitInitialize(&argc, argv, NULL);


    /* Notes from SAMC:
     *
     * You may want to use XtAppInitialize later on because of other
     * functionalities. When you do this, you also need to do changes
     * on your MainLoop, e.g., replace XtNextEvent with XtAppNextNext.
     */
    DESKTOP_SHELL(Desktop) = XtInitialize((char *)DmAppName,
					  (char *)DmAppClass, NULL, 0,
					  &argc, argv);

    XtVaSetValues(DESKTOP_SHELL(Desktop),
		  XtNmappedWhenManaged, False,
		  XtNwidth, 1,
		  XtNheight, 1,
		  NULL);

    XtRealizeWidget(DESKTOP_SHELL(Desktop));

    dpy = DESKTOP_DISPLAY(Desktop);
    win = XtWindow(DESKTOP_SHELL(Desktop));

    if (DtSetAppId(dpy, win, "_DT_QUEUE") ||
        DtSetAppId(dpy, win, "_WB_QUEUE") ||
        DtSetAppId(dpy, win, "_HELP_QUEUE"))
    {
    	Dm__VaPrintMsg(TXT_DTM_IS_RUNNING);
    	exit(START_UP_ERROR);
	/* NOTREACHED */
    }

#ifndef NOWSM
    /* Set these for olwsm */
    InitShell	= DESKTOP_SHELL(Desktop);
    handleRoot	= CreateHandleRoot();

    BusyPeriod(handleRoot, True);
    if (DmInitWSM(argc, argv))
    {
	Exit(INIT_WSM_ERROR, TXT_WSM_INIT_FAILED);
	/* NOTREACHED */
    }

    ExecRC(GetPath(RC));

#endif /* NOWSM */

    XtGetApplicationResources(DESKTOP_SHELL(Desktop),
			      (XtPointer)&DESKTOP_OPTIONS(Desktop),
			      resources, XtNumber(resources), NULL, 0);

    DtInitialize(DESKTOP_SHELL(Desktop));

    /* get node name */
    (void)uname(&unames);
    DESKTOP_NODE_NAME(Desktop) = strdup(unames.nodename);

    /* get umask */
    DESKTOP_UMASK(Desktop) = umask(0);
    umask(DESKTOP_UMASK(Desktop));
    DESKTOP_UMASK(Desktop) =
	~DESKTOP_UMASK(Desktop) & (S_IRWXU | S_IRWXG | S_IRWXO);

    /* initialize bg flag */
    Desktop->bg_not_regd	= True;
    DESKTOP_CUR_TASK(Desktop)	= NULL;

    /* Initialize/cache default fixed width font */
    DESKTOP_FIXED_FONT(Desktop) =
	_OlGetDefaultFont(DESKTOP_SHELL(Desktop), OlDefaultFixedFont);

    /* Initialize/cache default font. */
    DESKTOP_FONT(Desktop) =
	_OlGetDefaultFont(DESKTOP_SHELL(Desktop), OlDefaultFont);

    /* Initialize Sync Timer stuff:
	Alloc an array struct for stale folders.  Freed when desktop exits
	Init sync_timer.

	The alloc is a little under-handed: in order to keep
	"struct _StaleList" hidden in f_sync.c, a different type is alloced
	here and cast to "struct _StaleList *".
    */
    _OlArrayAllocate(CharArray, dummy_array,
		     _OlArrayDefaultInitial, _OlArrayDefaultStep);
    STALE_FOLDERS(Desktop) = (struct _StaleList *)dummy_array;
    SYNC_TIMER(Desktop) = NULL;

    /* Initialize Visited folder "ring buffer head" */
    VISITED_HEAD(Desktop) = 0;

    DESKTOP_CWD(Desktop) = getcwd(NULL, PATH_MAX + 1);/* remember initial CWD */
    DESKTOP_HOME(Desktop)= strdup(getenv("HOME")); /* store $HOME */

    {
	OlDefine gui_mode;
	static char str[32] = "XGUI=";

    	gui_mode = OlGetGui();
    	switch(gui_mode)
	{
    	case OL_OPENLOOK_GUI:
	    strcat(str, "OPEN_LOOK");
	    break;
    	case OL_MOTIF_GUI:
	    strcat(str, "MOTIF");
	    break;
	default:
	    str[0] = '\0';
    	}

	if (str[0] != '\0')
	    putenv(str);
    }

    DmInitDTMReqProcs(DESKTOP_SHELL(Desktop));
    DmInitWBReqProcs(DESKTOP_SHELL(Desktop));
    DmInitHMReqProcs(DESKTOP_SHELL(Desktop));

#ifndef NOWSM

    /* Need to wait for window manager to come up, at least for a while,
     * before mapping dtm windows.
     */
    {
	XWindowAttributes	win_attr;
	int			i;

	dpy = DESKTOP_DISPLAY(Desktop);
	win  = RootWindowOfScreen(DESKTOP_SCREEN(Desktop));

	for (i=TIME_TO_WAIT_FOR_WM; i; i--)
	{
	    XGetWindowAttributes(dpy, win, &win_attr);
	    if (win_attr.all_event_masks & SubstructureRedirectMask)
		break;
	    sleep(1);
	}
    }
#endif

    switch(DmOpenDesktop())
    {
    case -1:
	Exit(OPEN_DT_ERROR, TXT_OPEN_DESKTOP);
	/* NOTREACHED */

    case 0:
	/* FALLTHROUGH */
    case 1:
	/* Start Wastebasket and Help Desk here only if they weren't saved
	   in the previous session.  They should always be *initialized*
	   regardless of whether they're saved in a previous session so that
	   file deletion and adding/removing apps to/from the Help Desk will
	   work.
	*/

	if (!DESKTOP_WB_WIN(Desktop) && DmInitWasteBasket(NULL, False, False))
	{
	    Exit(INIT_WB_ERROR, TXT_WB_INIT_FAILED);
	    /* NOTREACHED */
	}

	if (DmInitHelpManager())
	{
	    Exit(INIT_HM_ERROR, TXT_HM_INIT_FAILED);
	    /* NOTREACHED */
	}

#ifdef NOT_USE
	if (!DESKTOP_HELP_DESK(Desktop) && DmInitHelpDesk(NULL, False, False))
	{
	    Exit(INIT_HD_ERROR, TXT_HD_INIT_FAILED);
	    /* NOTREACHED */
	}
#endif

	break;
    }

    /* Get Help ID for folders */
    FOLDER_HELP_ID(Desktop) = DmNewHelpAppID(DESKTOP_SCREEN(Desktop),
		XtWindow(DESKTOP_SHELL(Desktop)), Dm__gettxt(TXT_DESKTOP_MGR),
		Dm__gettxt(TXT_FOLDER_TITLE), DESKTOP_NODE_NAME(Desktop),
		NULL, "dir.icon")->app_id;

    /*catch signals */
    sigset(SIGHUP, SignalHandler);
    sigset(SIGTERM, SignalHandler);
    sigset(SIGINT, SignalHandler);

#ifndef BEEP_WHEN_OLFM_READY
    _OlBeepDisplay(InitShell, 1);
#endif
    
    BusyPeriod(handleRoot, False);

    atexit(AtExit);

    /* XtMainLoop equivalent:  This code was moved from wsm.c (olwsm) */

    for (;;) {
	XEvent event;

	XtNextEvent (&event);
	ServiceEvent(&event);
        HandleWindowDeaths(&event);
    }
    /*NOTREACHED*/
}

/***************************private*procedures****************************

    Private Procedures
*/

static void
AtExit(void)
{
    DtmExit();
    WSMExit();
}
/*
 * CancelExitCB
 *
 * This callback procedure is executed when the user SELECTs the
 * "Cancel" button in the Exit Notice.  The callback unposts the
 * notice and resets the state of the Session data structure.
 *
 */

static void
CancelExitCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
   Boolean          shutdown = (Boolean)client_data;

   BringDownPopup((Widget)_OlGetShellOfWidget(widget));

   if (shutdown)
      DmVaDisplayStatus((DmWinPtr)DESKTOP_TOP_FOLDER(Desktop),
                          True, TXT_SHUTDOWN_ABORTED);

   Session.ProcessTerminating = 0;
   Session.WindowKillCount    = 0;

   return;

} /* end of CancelExitCB */
/*
 * ExitCB
 *
 * This callback procedure is executed when the user SELECTs the 
 * "Exit and Save Session" or "Exit" buttons of the Exit Notice.
 * If the session manager was already supposed to be terminating
 * the session (i.e., Session.ProcessTerminating is set) then the
 * callback calls RealExit to force an exit.  This is done to give
 * the user control in the event that, for some reason, the normal
 * session exit is not proceeding.  If the session was not yet in
 * a termination state, the callback unposts the notice, saves the
 * Desktop properties, sets the termination flag, and attempts the
 * session termination (by calling QueryAndKillWindowTree).  If, on
 * this first attempt there are no windows left to be handled, RealExit
 * is called to terminate the desktop manager.
 * 
 * The routine also, conditionally - based on which button was SELECTed,
 * saves the current state of the desktop windows by calling DmSaveSession.
 *
 * The \fIshutdown\fP flag passed in as \fIclient_data\fP, is used to save
 * the appropriate return code in the ProcessTerminating member of the
 * Session struct.  The value of this variable is maintained to be one
 * of: 0 (indicating that the session is not in the termination state,
 * 1 (indicating that the session is in a \fInormal\fP termination state,
 * or 43 (indicating that the session is terminating and effecting a
 * system shutdown).
 *
 */

static void
ExitCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p        = (OlFlatCallData *)call_data;
   Boolean          shutdown = (Boolean)client_data;

   if (Session.ProcessTerminating) /* already exiting - force an exit */
      RealExit(Session.ProcessTerminating);

   BringDownPopup((Widget)_OlGetShellOfWidget(widget));

   DmSaveDesktopProperties(Desktop);

   if (p->item_index == 0)   /* save session before exit */
      DmSaveSession
         (DmMakePath(DmGetDTProperty(DESKTOPDIR, NULL), ".lastsession"));

   DtmExitCode = shutdown ? SHUTDOWN_EXIT_CODE : 0;

   Session.ProcessTerminating = ((shutdown) ? SHUTDOWN_EXIT_CODE : 0) + 1;

   if (QueryAndKillWindowTree(XtDisplay(widget)) == 0)
      RealExit(Session.ProcessTerminating);

   return;

} /* end of ExitCB */

/****************************procedure*header*****************************
    DtmExit- this performs final shut-down of dtm.
	This is external so that it can be called from olwsm/wsm.c.  When
	exiting normally (by the user), this is called as the result of a
	WSM_EXIT message from the olwm.
*/
static void
DtmExit()
{
    static int first = 1;

    if (!first)
	return;
    first = 0;

    DmWBExit();			/* close waste basket */
    DmHDExit();			/* close help desk */
    DmHMExit();			/* close all help windows */
    DmCloseFolderWindows();	/* close all opened folder windows */
}

/****************************procedure*header*****************************
    Exit-
*/
static void
Exit(int exit_code, char * message)
{
    BusyPeriod(handleRoot, False);
    Dm__VaPrintMsg(message);
    exit(exit_code);
}

void
DmPrepareForRestart()
{
    DmSaveSession(DmMakePath(DmGetDTProperty(DESKTOPDIR, NULL),".lastsession"));
    DtmExit();
}

/****************************procedure*header*****************************
    ExitHelpCB- 
*/
static void
ExitHelpCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
	DmHelpAppPtr help_app = DmGetHelpApp(DESKTOP_HELP_ID(Desktop));

	DmDisplayHelpSection(&(help_app->hlp_win), help_app->app_id,
		NULL, "DesktopMgr/shutdown.hlp", "60", -1000, -1000);
	XtAddGrab(help_app->hlp_win.shell, False, False);

}					/* end of ExitHelpCB */

/****************************procedure*header*****************************
    DmSaveDesktopProperties-
*/
void
DmSaveDesktopProperties(DmDesktopPtr desktop)
{
	register int i;
	register DtPropPtr pp = DESKTOP_PROPS(desktop).ptr;
	char *path;
	FILE *f;

	path = DmMakePath(DmGetDTProperty(DESKTOPDIR, NULL), ".dtprops");
	if ((f = fopen(path, "w")) == NULL) {
		Dm__VaPrintMsg(TXT_DTPROP_SAVE);
		return;
	}

	/*
	 * Save all desktop properties except DESKTOPDIR.
	 * DESKTOPDIR cannot be saved into this file, because it is used
	 * to find the fullpath of this file. A chicken & egg situation.
	 */
	for (i=DESKTOP_PROPS(desktop).count; i; i--, pp++)
		if (strcmp(pp->name, DESKTOPDIR))
			fprintf(f, "%s=%s\n", pp->name, pp->value);

	fclose(f);
}

static void
ShutdownCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    BringDownPopup((Widget)_OlGetShellOfWidget(widget));
    DmPromptExit(True);
}

/****************************procedure*header*****************************
	Displays help on Shutdown.
 */
static void
ShutdownHelpCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
     DmHelpAppPtr help_app = DmGetHelpApp(DESKTOP_HELP_ID(Desktop));

     DmDisplayHelpSection(&(help_app->hlp_win), help_app->app_id, NULL,
          "DesktopMgr/shutdown.hlp", "10", -1000, -1000);
	XtAddGrab(help_app->hlp_win.shell, False, False);

} /* end of ShutdownHelpCB */

/****************************procedure*header*****************************
    SignalHandler-
*/
static void
SignalHandler(int signum)
{
    Dm__VaPrintMsg(TXT_CAUGHT_SIGNAL, signum);
    DtmExit();
    WSMExit();
    /* NOTREACHED */
}

/***************************public*procedures*****************************

    Public Procedures
*/

/****************************procedure*header*****************************
    DmPromptExit-
*/
void
DmPromptExit(Boolean shutdown)
{
    static MenuItems	menubarItems[] = {
	MENU_ITEM( TXT_SAVE_N_EXIT, TXT_M_SAVE_N_EXIT,	ExitCB ),
	MENU_ITEM( TXT_FILE_EXIT,   TXT_M_FILE_EXIT,	ExitCB ),
	MENU_ITEM( TXT_CANCEL,      TXT_M_CANCEL,	CancelExitCB ),
	MENU_ITEM( TXT_P_HELP,      TXT_M_HELP,		ExitHelpCB ),
	{ NULL }				/* NULL terminated */
    };

    MENU_BAR("exitNoticeMenubar", menubar, NULL, 2);	/* default: Cancel */

    static HelpInfo ExitHelp = 
    { TXT_G_PRODUCT_NAME, NULL, "DesktopMgr/shutdown.hlp", "60", NULL };

    static ModalGizmo notice_gizmo = {
	&ExitHelp,			/* help info */
	"exitNotice",			/* shell name */
	TXT_G_PRODUCT_NAME,		/* title */
	&menubar,				/* menu */
	TXT_END_SESSION,		/* message */
	NULL, 0,				/* gizmos, num_gizmos */
    };

    /* Set client data each time for callbacks to work */
    menubarItems[0].client_data = menubarItems[1].client_data =
	menubarItems[2].client_data = (char *)shutdown;

    /* Create the Notice gizmo (once) and map it */
    CREATE_NOTICE(&notice_gizmo);
    MapGizmo(ModalGizmoClass, &notice_gizmo);

}					/* end of DmPromptExit */

/****************************procedure*header*****************************
    DmShutdownPrompt-
*/
void
DmShutdownPrompt(void)
{
    static	char	tst[] = "/sbin/tfadmin -t /sbin/shutdown 2>/dev/null";
    ModalGizmo *		notice;
    static XtPopdownIDRec	popdown_rec;

    static MenuItems	privItems[] = {
	MENU_ITEM ( TXT_OK, TXT_M_OK, XtCallbackPopdown ),
	{ NULL }				/* NULL terminated */
    };
    MENU_BAR("privMenubar", priv, NULL, 0);	/* default: "Ok" */

    static HelpInfo ShutdownHelp = 
    { TXT_G_PRODUCT_NAME, NULL, "DesktopMgr/shutdown.hlp", NULL, NULL };

    static ModalGizmo priv_gizmo = {
	&ShutdownHelp,				/* help info */
	"privNotice",				/* shell name */
	TXT_G_PRODUCT_NAME,			/* title */
	&priv,					/* menu */
	TXT_CANT_SHUTDOWN,			/* message */
	NULL, 0,				/* gizmos, num_gizmos */
    };

    static MenuItems	shutdownItems[] = {
	MENU_ITEM(TXT_SHUTDOWN,	TXT_M_SHUTDOWN,	ShutdownCB ),
	MENU_ITEM(TXT_CANCEL,	TXT_M_CANCEL,	XtCallbackPopdown ),
	MENU_ITEM(TXT_P_HELP,	TXT_M_HELP,	ShutdownHelpCB ),
	{ NULL }				/* NULL terminated */
    };
    MENU_BAR("shutdownMenubar", shutdown, NULL, 1);	/* default: Cancel */

    static ModalGizmo shutdown_gizmo = {
	&ShutdownHelp,				/* help info */
	"shutdownNotice",			/* shell name */
	TXT_G_PRODUCT_NAME,			/* title */
	&shutdown,				/* menu */
	TXT_CONFIRM_SHUTDOWN,			/* message */
	NULL, 0,				/* gizmos, num_gizmos */
    };

    privItems[0].client_data =
	shutdownItems[1].client_data = (char *)&popdown_rec;

    /* Create the appropriate notice gizmo (once) and map it */
    notice = (getuid()==0 || system(tst)==0) ? &shutdown_gizmo : &priv_gizmo;
    CREATE_NOTICE(notice);
    XtSetArg(Dm__arg[0], XtNgravity, WestGravity);
    XtSetArg(Dm__arg[1], XtNalignment, OL_LEFT);
    XtSetValues(notice->stext, Dm__arg, 2);
    popdown_rec.shell_widget = notice->shell;	/* for "cancel" callbacks */
    MapGizmo(ModalGizmoClass, notice);
}					/* DmShutdownPrompt */




