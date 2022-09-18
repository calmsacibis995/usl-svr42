/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olwm:wm.c	1.134"
#endif

/*
 * To do list:
 *
 * _x_ Handle client substructure requests
 *
 * _x_ Handle title when too wide
 *
 * _x_ Constrain move of groups
 *
 * _i_ Constrain resize to hints -> not aspect resize
 *
 * _i_ Scale stuff        -> no data created
 *
 * _i_ Handle Property Changes
 *     _x_ XA_WM_NAME
 *     _x_ XA_WM_ICON_NAME
 *     _i_ WM_PROTOCOLS   -> need to clarify save/delete stuff
 *     _i_ _OL_WIN_BUSY   -> [un]grab button/key events & prevent wm ops?
 *     _x_ _OL_WIN_ATTR
 *     _x_ _OL_DECOR_ADD
 *     _x_ _OL_DECOR_DEL
 *     _x_ _OL_WIN_COLORS -> done
 *     _x_ _OL_PIN_STATE
 *
 * _i_ Handle BANG        -> what should happen? (olpixmap hangs the system)
 *
 * _x_ Handle Quit/Dismiss
 *
 * _x_ Handle Colormaps   -> Includes a strong attempt at WM_COLORMAP_WINDOWS
 *
 * _x_ Handle Focus Management
 * _x_ Handle Keys When No Focus
 *
 * _x_ Use hints 
 *
 * _x_ Window Placement
 *
 * _x_ Icon Parking
 *
 */

/*
 * wm.c
 *
 * Flattened Window Manager Widget
 *
 */

#include <stdio.h>

#include <X11/keysym.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/ShellP.h>
#include <X11/Shell.h>
#include <X11/Xatom.h>

#include <Xol/OpenLookP.h>
#include <Xol/DynamicP.h>
#include <Xol/Stub.h>
#include <Xol/Olg.h>
#include <Xol/WSMcomm.h>
#include <Xol/Flat.h>

#include <wm.h>
#include <Xol/OlCursors.h>
#include <Xol/VendorI.h>
#include <WMStepP.h>

#include <Extern.h>
#include <X11/cursorfont.h>
#include <limits.h>

/*
 *************************************************************************
 *
 * Forward Procedure declarations
 *
 **************************forward*declarations***************************
 */

extern Window		find_highest_group OL_ARGS((WMStepWidget,Window));
extern Cardinal		find_leader OL_ARGS((Window));
extern int              FindCurrentWindow OL_ARGS((Boolean));
static void		ReparentWindows OL_ARGS((Display *, Window));

static void		WMIconGravityConverter OL_ARGS((XrmValue *, Cardinal,
					XrmValue *, XrmValue *));

static void		WMDynamic OL_ARGS((XtPointer));
static void		AddKeyGrabs OL_ARGS((Widget));
extern void		AddButtonGrabs OL_ARGS((Widget));
static void		CheckColorSanity OL_ARGS((Screen *, Boolean));
extern Boolean		CheckMotifFocusStops OL_ARGS((WMStepWidget,
					OlDefine));
extern Boolean		FocusSwitchOK OL_ARGS((WMStepWidget));

extern void   ClientEnterLeave OL_ARGS((Widget, XtPointer, XEvent *,
                                                 Boolean *));
extern void   IconEnterLeave OL_ARGS((Widget, XtPointer, XEvent *,
                                                  Boolean *));

static void	RaiseSetFocus OL_ARGS((WMStepWidget));

extern Boolean	ReadyToDie OL_NO_ARGS();

static void	SelectPushpin OL_ARGS((WMStepWidget, XEvent *, WMPiece));

static void	SelectMenuMark OL_ARGS((WMStepWidget, XEvent *, WMPiece));

extern void	Select OL_ARGS((WMStepWidget, XEvent *, WMPiece));

static void	HandlePushpin OL_ARGS((Widget, XEvent *));

static void	PollHandlePushpin OL_ARGS((XtPointer, XtIntervalId *));

static void	HandleMenuMark OL_ARGS((Widget, XEvent *));
static void	PollHandleMenuMark OL_ARGS((XtPointer, XtIntervalId *));

extern void	ReadProtocols OL_ARGS((WMStepWidget, Widget, Window));

static Boolean	KeyPressEvent OL_ARGS((Widget, XtPointer, XEvent *));
static void	NonMaskable OL_ARGS((Widget, XtPointer, XEvent *, Boolean *));
static void	ButtonPressGrab OL_ARGS((Widget, XtPointer, XEvent *, Boolean *));

extern void	StartWindowMappingProcess OL_ARGS((WMStepWidget));
static void	SubstructureRedirect OL_ARGS((Widget, XtPointer, XEvent *,
							Boolean *));

extern void	DestroyParent	OL_ARGS((Widget, XtPointer, XtPointer));
static Widget	Reparent	OL_ARGS((Window, XWindowAttributes *));
static void	WMError		OL_ARGS((int, XtPointer));
static void	WMMainLoop	OL_ARGS((Display *));

static void	GetShadowColors OL_ARGS((WMStepWidget, Pixel *, Pixel *));
extern void	SendLongMessage OL_ARGS((Display *, Window, Atom, unsigned long,
					unsigned long, unsigned long,
					unsigned long, unsigned long));
extern void	SendConfigureNotify OL_ARGS((Display *, Window, int, int,
					int, int, int));

extern void	AddWidgetToWidgetBuffer OL_ARGS((WMStepWidget, WidgetBuffer *));
extern void	RemoveWidgetFromWidgetBuffer OL_ARGS((WMStepWidget,
							WidgetBuffer *));
extern void	ResolveColors OL_ARGS((Pixel *, Pixel *,Pixel *));

extern Boolean CheckTransientModality OL_ARGS((WMStepWidget));
extern int Next_Prev_Application OL_ARGS((WMStepWidget, OlDefine, int, int));
extern int Next_Prev_Window OL_ARGS((WMStepWidget, OlDefine, int, int));
extern void OlwmPostMenu OL_ARGS((WMStepWidget, XEvent *));
extern void SendKeyEvent OL_ARGS((WMStepWidget, XEvent *));
extern void SetShadowGCS OL_ARGS((Widget, int, int));
extern Pixel GetLighterColor OL_ARGS((Screen *, Pixel));
extern Pixel GetDarkerColor OL_ARGS((Screen *, Pixel));

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */
#define DEFAULT_ICON_WIDTH	65
#define DEFAULT_ICON_HEIGHT	65

#if !defined(POSTMARK)
#define POSTMARK
#endif

#if !defined(memmove)
#define memmove(dest, src, n)  bcopy((char*)src, (char*)dest, (int)n)
#endif

/*
static OlKeyOrBtnRec OlWmKeyBindings[] = {
   { XtNwmOpenCloseKey,  "a<F5>",             OL_WM_OPENCLOSE   },
   { XtNwmSizeKey,       "a<F10>",            OL_WM_SIZE        },
   { XtNwmPropertiesKey, "s a<F7>",           OL_WM_PROPERTIES  },
   { XtNwmRefreshKey,    "a c<l>",            OL_WM_REFRESH     },
   { XtNwmBackKey,       "a<F3>",             OL_WM_BACK        },
   { XtNwmQuitKey,       "a<F4>",             OL_WM_QUIT        },
   { XtNwmDismissThisKey,"a<F9>",             OL_WM_DISMISSTHIS },
   { XtNwmDismissAllKey, "s a<F9>",           OL_WM_DISMISSALL  },
   { XtNwmMoveKey,       "a<F7>",             OL_WM_MOVE        },
   { XtNwmResizeKey,     "a<F8>",             OL_WM_RESIZE      },
   { XtNwmOwnerKey,      "s a<F8>",           OL_WM_OWNER       },
};
*/

/* Keys private to Open Look - the openclose key is common to
 * both OL and Motif, but they have different default accelerators.
 * In Motif, the Restore key is Alt+F5.
 */
static OlKeyOrBtnRec OlWmKeyBindings[] = {
   { XtNwmOpenCloseKey,  "a<F5>",             OL_WM_OPENCLOSE   },
   { XtNwmPropertiesKey, "s a<F7>",           OL_WM_PROPERTIES  },
   { XtNwmRefreshKey,    "a c<l>",            OL_WM_REFRESH     },
   { XtNwmDismissThisKey,"a<F9>",             OL_WM_DISMISSTHIS },
   { XtNwmDismissAllKey, "s a<F9>",           OL_WM_DISMISSALL  },
   { XtNwmOwnerKey,      "s a<F8>",           OL_WM_OWNER       },
/* Move this one here from OlCommon */
   { XtNwmSizeKey,       "a<F10>",		OL_WM_SIZE },
};

static OlKeyOrBtnRec OlCommonKeyBindings[] = {
	{ XtNwmBackKey,       "a<F3>",	OL_WM_BACK },
	{ XtNwmQuitKey,       "a<F4>",	OL_WM_QUIT },
	{ XtNwmMoveKey,       "a<F7>",	OL_WM_MOVE },
	{ XtNwmResizeKey,     "a<F8>",	OL_WM_RESIZE },
};

static OlKeyOrBtnRec OlMotifKeyBindings[] = {
   { XtNwmRestoreKey,	"a<F5>",		OL_WM_RESTORE },
   { XtNwmMinimizeKey,	"a<F9>",		OL_WM_MINIMIZE },
   { XtNwmMaximizeKey,	"a<F10>",		OL_WM_MAXIMIZE },
};



/*
 *************************************************************************
 *
 * Procedures
 *
 ***************************private*procedures****************************
 */

/*
 * SetupWindowManager
 *
 */


extern void
SetupWindowManager OLARGLIST((shell))
OLGRA(Widget, shell)
{
Screen *  WMScreen  = XtScreen(shell);
Window    root_window = RootWindowOfScreen(WMScreen);
OlVirtualEventTable	OlCommonDB,
			GUISpecificDB;

Shell = shell;
XSetErrorHandler(IgnoreClientErrors);

/*
 * need to calculate resolution
 * don't use the garbage in OlGetRes.c
 */

ScreenResolution = 75;
currentGUI = OlGetGui();

/* Add the converter WMIconGravityConverter()- convert from a string to
 * "WMIconGravity".  No arguments (NULL, 0).
 */
XtAddConverter(XtRString, XtRWMIconGravity,
			(XtConverter)WMIconGravityConverter, NULL, 0);

/* Convert a string, such as "50x50", to an IconWinDim struct */
XtAddConverter(XtRString, XtRIconWinDim,
			(XtConverter)WMIconWinDimConverter, NULL, 0);

/* Convert a string to a long with the defines for icon decorations */
XtAddConverter(XtRString, XtRIDecor,
			(XtConverter)WMIconDecorConverter, NULL, 0);

/* Convert a string to a long with the defines for motif feedback
 * options.
 */
XtAddConverter(XtRString, XtRShowFeedback,(XtConverter)WMShowFeedbackConverter,
							NULL, 0);

XtAddConverter(XtRString, XtRTransientDecoration,
		(XtConverter)WMTransientDecorConverter, NULL, 0);

XtAddConverter(XtRString, XtRTransientFunctions,
		(XtConverter)WMTransientFunctionsConverter, NULL, 0);

XtAddConverter(XtRString, XtRClientDecoration,
		(XtConverter)WMClientDecorConverter, NULL, 0);

XtAddConverter(XtRString, XtRClientFunctions,
		(XtConverter)WMClientFunctionsConverter, NULL, 0);
			
/* Turn the root window into a widget.  Parent widget
 * is the Shell - possibly returned from Xt[App]Initialize(); this widget class
 * is meant only for the root window, so no reparenting is done.
 */
 if ((Frame = XtWindowToWidget(XtDisplay(shell), root_window)) == (Widget)NULL)
 {
		/* The code below won't be exercised because	*/
		/* the O/L toolkit (troll) already takes care of it. 	*/
		/* If for any reason, it gets here then we want to 	*/
		/* remove the translation table from this "stub" 	*/
		/* otherwise it may cause a X protocol error (BadAccess)*/
	Arg		args[2];
	XtTranslations	empty_translations;

	empty_translations = XtParseTranslationTable("");
	XtSetArg(args[0], XtNwindow, root_window);
	XtSetArg(args[1], XtNtranslations, empty_translations);
	Frame = XtCreateWidget("Root", stubWidgetClass, shell, args, 2);
		/* remove self from the traversal list */
	_OlDeleteDescendant(Frame);
 }

if (currentGUI == OL_MOTIF_GUI) {
	/* Timer while we set up */
	MotifModeStartup(True);
	/* Create motif default window cursor (when cursor isn't on
	 * the root window.
	 */
	motifWindowCursor = XCreateFontCursor(XtDisplay(shell), XC_left_ptr);
}

/* Register function for dynamic resources, and make an initial
 * call to get the "first call" out of the way
 */

OlRegisterDynamicCallback(WMDynamic, (XtPointer)Frame);
WMDynamic((XtPointer)Frame);

/* Get other resources used by motif mode.
 *  motif globals, motif components
 */
if (currentGUI == OL_MOTIF_GUI) {
	Display *display = XtDisplay(shell);
	Widget mwmshell = XtAppCreateShell("Mwm","Mwm",
		applicationShellWidgetClass, display, NULL, 0);
	if (mwmshell) {
		ResolveMotifGlobalResources(mwmshell);
		ResolveMotifComponentResources(mwmshell);
		ResolveMotifCSResources(mwmshell);
		XtDestroyWidget(mwmshell);
	}
	ParseResourceFile();
}


PermScale = wmrcs->scale;

/*  mlp - I used to initialize the motif color GCs here - I now do them
 * in CreateGCs for each client, but it may be necessary to do them
 * here, because if initial placement/sizing is on, then I need the
 * top and bottom shadow colors for the little feedback window.
 */

if (currentGUI == OL_OPENLOOK_GUI) {
	int	def_icon_width,
		def_icon_height;
	int	vertical_res;
	int	diff;
	int	ipbpp; /* image plus border plus pad */
	int	fh;	/* icon font height */

/*
  - One size fits all... All icons in open look mode are approximately 65 points
 * square.
 */
#define ICON_BORDER_WIDTH	3
		/* Everything else is considered medium to high resolution */

		icon_width = OlScreenPointToPixel(OL_HORIZONTAL,
					65, WMScreen);
		if ( (ol_icon_image_width =  OlScreenPointToPixel(OL_HORIZONTAL,
					57, WMScreen) ) < 48) 
			ol_icon_image_width = 48;
		ipbpp = 2 * ICON_BORDER_WIDTH + ol_icon_image_width +
							2*ICON_IMAGE_PAD;
		if (ipbpp <= icon_width)
			icon_width = ipbpp;
		else {
			/* Make even boundaries on both sides of image */
			diff = icon_width - ol_icon_image_width;
			if (diff % 2)
				icon_width++;
		}
			
		
		icon_height = OlScreenPointToPixel(OL_VERTICAL, 65,
							WMScreen);
		if ( (ol_icon_image_height = OlScreenPointToPixel(
					OL_VERTICAL, 39, WMScreen)) < 48)
			ol_icon_image_height = 48;

		/* Leave enough room for the title - I have no choice with
		 * these 48 pixel icons
		 */
		fh = OlFontHeight(wmrcs->iconFont, ol_font_list);
		if (fh == 0)
			fh = 10; /* why not */
		ipbpp = ol_icon_image_height + 2 * ICON_BORDER_WIDTH +
			2 * ICON_IMAGE_PAD + fh;
		if (ipbpp > icon_height)
			icon_height = ipbpp;
		if (icon_height % 2)
			icon_height++;

}
else {
	InitIconDims((Widget)Frame);
	icon_width = mot_icon_iwidth;
	icon_height = mot_icon_iheight + mot_icon_label_height;

	/* Create Motif Feedback window for move/resize */
	CreateMotifFeedbackWindow(XtDisplay(shell),WMScreen);
}
/* Now that we have the wm resources, including the grid size, we
 * may want to adjust the width and height of icons to be a multiple
 * the grid size.  
 *
 */

/* for now, just use openlook.  Motif has a strict icon layout procedure */

if (currentGUI == OL_OPENLOOK_GUI && wmrcs->iconGrid && wmrcs->iconGridSize > 0) {
	int remainder;


	IconGridX = OlScreenPointToPixel(OL_HORIZONTAL,
		wmrcs->iconGridSize, WMScreen);
	IconGridY = OlScreenPointToPixel(OL_VERTICAL,
		wmrcs->iconGridSize, WMScreen);
	remainder = icon_width % IconGridX;
	while (remainder && IconGridX > 0) {
			IconGridX--;
			remainder = icon_width % IconGridX;
	}
	remainder = icon_height % IconGridY;
	while (remainder && IconGridY > 0) {
			IconGridY--;
			remainder = icon_height % IconGridY;
	}
	if (IconGridX == 0 && IconGridY == 0)
		wmrcs->iconGrid = False;
} /* iconGrid */
else {
	IconGridX = IconGridY = 0;
	wmrcs->iconGrid = False;
}


/* Initialize coordinates where the next icon is to be placed on the screen;
 * uses variables CurrentIconX, CurrentIconY.
 */
if (currentGUI == OL_OPENLOOK_GUI)
	ResetIconPosition(WMScreen);
else
	InitIconPlacementStrategy(WMScreen);

 /* Add Event Handler for tracking focus in and out on root window -
  * this is necessary because sometimes focus can 'disappear' -
  * policy is RevertToNone; if the root window receives a FocusChange
  * event with a detail of NotifyDetailNone, then we must delegate input
  * focus somewhere.
  */
  XtAddEventHandler(Frame, FocusChangeMask, False,
				ClientFocusChange, (XtPointer)NULL);

/* Get / compile window manager database to recognize accelerators */
OlCommonDB = OlCreateInputEventDB(Shell, OlCommonKeyBindings,
		XtNumber(OlCommonKeyBindings), NULL,0);

/* Register window manager database with stub widget class - when
 * calling OlLookupInputEvent() with OL_DEFAULT_IE or OlCommonDB -
 * search window mgr. database first
 */
OlClassSearchIEDB(stubWidgetClass,OlCommonDB);

if (currentGUI == OL_OPENLOOK_GUI)
	GUISpecificDB = OlCreateInputEventDB(Shell, OlWmKeyBindings,
		XtNumber(OlWmKeyBindings), NULL,0);
else /* Motif */
	GUISpecificDB = OlCreateInputEventDB(Shell, OlMotifKeyBindings,
		XtNumber(OlMotifKeyBindings), NULL,0);

OlClassSearchIEDB(stubWidgetClass,GUISpecificDB);

XtRealizeWidget(Frame);

} /* SetupWindowManager */

/*
 * StartWindowManager
 *
 * Called with shell returned by Xt[App]Initialize(); call this after
 * calling SetupWindowManager().
 * Frame =  the root window's widget, initialized
 * in SetupWindowManager().
 */

extern void StartWindowManager(Shell)
Widget Shell;
{
Display * WMDisplay = XtDisplay(Frame);
Screen *  WMScreen  = XtScreen(Frame);
Window    WMRoot    = RootWindowOfScreen(WMScreen);
XIconSize xiconsize;
Colormap Default_Colormap = DefaultColormapOfScreen(WMScreen);

/******************************************
 *
 * mlp -we are going to change all this so that the icon size set on
 * the root window is actually the icon IMAGE size - that we will
 * calculate as we do in motif mode.
 */
/* Set up the icon size property on the root window */
/*
xiconsize.max_width = xiconsize.min_width = icon_width;
xiconsize.max_height = xiconsize.min_height = icon_height;
xiconsize.width_inc = xiconsize.height_inc = 0;
 */
	if (currentGUI == OL_OPENLOOK_GUI) {
		xiconsize.max_width = xiconsize.min_width = ol_icon_image_width;
		xiconsize.max_height = xiconsize.min_height =
					ol_icon_image_height;
		xiconsize.width_inc = xiconsize.height_inc = 0;

		XSetIconSizes(WMDisplay, WMRoot, &xiconsize, 1);
	}
	else
		SetMotifIconProperty(WMDisplay, WMRoot);

Default_Colormap = DefaultColormapOfScreen(XtScreen(Frame));

/* - Call function (event handler) NonMaskable() for Non-Maskable events
 * sent to the root window (e.g., ClientMessage events);
 * - Call SubstructureRedirect() on client redirected events (mapping of
 * client windows).  First check to see if another window manager is
 * running - must set the error handler first.
 */
XSync(WMDisplay,False);
XSetErrorHandler(CatchRedirectErrors);
XtAddEventHandler(Frame, NoEventMask, True, NonMaskable, (caddr_t)NULL);
XtAddEventHandler(Frame, SubstructureRedirectMask, False, 
                         SubstructureRedirect, (XtPointer)NULL);
XSync(WMDisplay,False);
XSetErrorHandler(IgnoreClientErrors);

/* Add passive grabs on the root window.
 */
 AddKeyGrabs(Frame);

/* Create static window menus */
MenuInitialize(Shell);
/* set up additional atom */
if ( (WM_COLORMAP_WINDOWS = XInternAtom (WMDisplay,
			 		"WM_COLORMAP_WINDOWS", False))
				== None)
#if !defined(I18N)
	fprintf(stderr,"Window Manager Warning; Can't XInternAtom WM_COLORMAP_WINDOWS\n");
#else
	OlVaDisplayWarningMsg(WMDisplay, OleNbadAtom, OleTinternAtom,
OleCOlClientOlwmMsgs, OleMbadAtom_internAtom, NULL);
#endif

wm_unique_context = XUniqueContext();
MWM_HINTS = XInternAtom(WMDisplay, _XA_MWM_HINTS, False);

/* The following code sets the _MOTIF_WM_INFO property so that if a client
 * cares to check which window manager is running, using XmIsMotifRunning
 * will get an positive answer.  Be carefully, the values we put in are
 * not necessary valid!
 */

if (currentGUI == OL_MOTIF_GUI) {
	Atom    motif_info;
   	struct {
		long    f;
		Window  w;
	}       prop;

        prop.w = 0;
	prop.f = 1;
	motif_info = XInternAtom(WMDisplay, "_MOTIF_WM_INFO", False);
   	XChangeProperty(WMDisplay, WMRoot, motif_info, motif_info, 32,
   			PropModeReplace, (unsigned char *) &prop, 2);
}

CurrentColormap = Default_Colormap;
XInstallColormap(WMDisplay, CurrentColormap);
CurrentColormapWindow = XtWindow(Frame);
/*CurrentColormapCount = 1;*/

/* Initially, set focus - either Real-estate based or Click-To-Type
 */
SetFocus(Frame, GainFocus, 0);

/* Set up minimum width/ht. for motif mode */
if (currentGUI == OL_MOTIF_GUI)
	SetMotifMins();

start_or_terminate = START_FLAG;

ReparentWindows(WMDisplay, WMRoot);

start_or_terminate = 0;


#if defined(IS_OTHER)
if (OlIsWMRunning(WMDisplay, XtScreen(Shell)) == 0)
	fprintf(stderr,"We're O.K. - olwm isn't running yet\n");
else
	fprintf(stderr,"Olwm is running - we got a problem\n");
#endif

(void)SetOlwmRunning(WMDisplay,XtScreen(Shell));

	SetSignals(WMDisplay, WMRoot, WMScreen);

#if defined(IS_OTHER)
/******************/
if (OlIsWMRunning(WMDisplay, XtScreen(Shell)) == 0)
	fprintf(stderr,"Bad News. - olwm isn't running yet\n");
else
	fprintf(stderr,"Good news: Olwm is running \n");
SetOlwmNotRunning(WMDisplay,XtScreen(Shell));
if (OlIsWMRunning(WMDisplay, XtScreen(Shell)) == 0)
	fprintf(stderr,"Good News. - Turning it off works \n");
else
	fprintf(stderr,"Bad news: Turning it off didn't work\n");
/******************/
#endif

/* An attempt to minimize contention at startup:  in the olinitrc file, DO
 * NOT start the window manager or the file manager in the background.  This
 * way, the initialization code for the two applications is run sequentially.
 */

	if (wmrcs->do_fork)
	{
			/* on parent success or failure */
		if (fork() != 0) exit(0);
	}

if (currentGUI == OL_OPENLOOK_GUI)
	XDefineCursor(WMDisplay, WMRoot, OlGetStandardCursor(Frame));
else {
	MotifModeStartup(False);
}

WMMainLoop(WMDisplay);

} /* end of StartWindowManager */

/*
 * WMMainLoop
 *
 */

static void
WMMainLoop OLARGLIST((dsp))
OLGRA(Display *, dsp)
{
XEvent event;
Display *display = XtDisplay(Frame);
Widget wid;
Window rootwin = RootWindowOfScreen(XtScreen(Frame));

for (;;) {
	int sendit = 1;
	XtNextEvent(&event);

  if (event.type == MapRequest) {
	if (XtWindowToWidget(display, event.xmaprequest.window) == NULL) {
/*
		if (event.xmaprequest.parent != rootwin) {
*/
			Boolean bval = False;
			sendit = 0;

			XGrabServer(display);
			SubstructureRedirect(Frame, (XtPointer)NULL,
							&event, &bval);
			XUngrabServer(display);
			XFlush(display);
			continue;
/*
		}
*/
	}
  } /* MapRequest */

  if (event.type == ConfigureRequest &&
	   (wid = XtWindowToWidget(display, event.xany.window)) == NULL) {
	Boolean bval = False;

	sendit = 0;
	/* Reuse the code in SubstructureRedirect() */
	SubstructureRedirect(Frame,(XtPointer)NULL, &event, &bval);
	continue;
  } /* if ConfigureRequest && WindowToWidget fails */

   	FPRINTF((stderr,"%d event on window %x\n", event.type,
					 event.xany.window));

#define WIN	event.xany.window
#define TIME	event.xbutton.time
#define XX	event.xbutton.x
#define YY	event.xbutton.y

	if ( event.type == ButtonPress &&
	     (wid = XtWindowToWidget(display, WIN)) != NULL &&
	     XtIsSubclass(wid, wmstepWidgetClass) )
	{
		WMStepWidget	wm	= (WMStepWidget)wid;
		WMStepPart *	wmstep	= (WMStepPart *)&(wm->wmstep);

		if ( EventIn((WMStepWidget)wid, XX, YY) != WM_NULL )
		{
			ServiceEvent(&event);
		}
		else
		{
			RaiseLowerGroup(wm, WMRaise);

			if (!(wmstep->is_current)) {
				SetCurrent(wm);
			}

			if (!(wmstep->protocols & HasFocus)) {
				if (wmrcs->pointerFocus)
					XSetInputFocus(
						display,
						PointerRoot,
						RevertToPointerRoot,
						TIME);
				else
					XSetInputFocus(
						display,
						RootWindowOfScreen(
							XtScreen(wid)),
						RevertToNone,
						TIME);
			}
			/* Finally, install colormaps */
			CurrentColormapWindow = XtWindow(wm);
			WMInstallColormaps(wm);
		}

		XAllowEvents(display, ReplayPointer, event.xbutton.time);
		continue;
	} /* ButtonPress */

#undef WIN
#undef TIME
#undef XX
#undef YY


	if (event.type == KeyPress) {
		if (KeyPressEvent(Frame, (XtPointer)NULL, &event) == True) {
			sendit = 0;
			continue;
		}
	}
	if (event.type == ColormapNotify) {
		int index;
		if ( (index = IsWMStepChild(event.xcolormap.window)) < 0) {
			/* This is a window we may have selected for
			 * ColormapNotify or StructureNotify on, but it's not
			 * a toplevel window.
			 */
			sendit = False;
			if (event.xcolormap.new == True) {
				/* Newly installed colormap */
				ReworkColormapInfo(event.xcolormap.display,
					event.xcolormap.window,
					NEWCMAP);
			}
			continue;
		}		
	}

	if (event.type == FocusIn || event.type == FocusOut) {
		/* We asked for FocusChange events on the root window; if we
	 	 * get a detail of NotifyDetailNone, then dispatch the event
	 	 * to our event handler to deal with it; we have a policy in
	 	 * the Open Look toolkit that input focus reverts to NONE;
	 	 * the same is true for the window manager when the root
	 	 * window is given input focus (see SetFocus()).  Therefore,
	 	 * when this specific event and detail mode occur, we must
	 	 * delegate the input focus somewhere - to the client that
	 	 * previously had it, or another (if the previous one isn't
	 	 * around anymore).
	 	 */
		if (event.xfocus.window == XtWindow(Frame)) {
#ifdef PFOCUS
		fprintf(stderr,"Main Loop: got focus event for root...\n");
if (event.xfocus.detail == NotifyDetailNone)
fprintf(stderr,"It's a NotifyDetailNone\n");
if (event.xfocus.detail == NotifyInferior)
fprintf(stderr,"It's a NotifyInferior\n");
if (event.xfocus.detail == NotifyPointerRoot)
fprintf(stderr,"It's a NotifyPointerRoot\n");
if (event.xfocus.detail == NotifyPointer)
fprintf(stderr,"It's a NotifyPointer\n");
if (event.xfocus.detail == NotifyNonlinear)
fprintf(stderr,"It's a NotifyNonlinear\n");
if (event.xfocus.detail == NotifyNonlinearVirtual)
fprintf(stderr,"It's a NotifyNonlinearVirtual\n");
if (event.xfocus.detail == NotifyVirtual)
fprintf(stderr,"It's a NotifyVirtual\n");
if (event.xfocus.detail == NotifyAncestor)
fprintf(stderr,"It's a NotifyAncestor\n");

if (event.xfocus.mode == NotifyNormal)
	fprintf(stderr," *** And mode is NotifyNormal\n");
    else
	if (event.xfocus.mode == NotifyGrab)
		fprintf(stderr," *** And mode is NotifyGrab\n");
	    else if (event.xfocus.mode == NotifyUngrab)
		fprintf(stderr," *** And mode is NotifyUngrab\n");
if (event.type == FocusIn)
	fprintf(stderr,"And it's a FocusIn\n");
else
	fprintf(stderr,"And it's a FocusOut\n");
#endif

			if ( (event.type == FocusOut) ||
				 event.xfocus.detail != NotifyDetailNone ||
			/* Inserted - 6/23/92 */
				event.xfocus.mode == NotifyGrab ||
				event.xfocus.mode == NotifyUngrab )
				/* Following line removed - we will
				 * now send NotifyNormal or NotifyWhileGrabbed.
				 */
				/*|| event.xfocus.mode != NotifyNormal)*/
							{
#ifdef PFOCUS
fprintf(stderr,"Main loop - root window focus event is NOT normal or none (or FocusOut), don't send it\n");
#endif
				sendit = False;
			}
		}
	}
	if (event.type == UnmapNotify && event.xunmap.send_event) {
		/* This is a synthetic UnmapNotify, possibly from
		 * an XWithdrawWindow().  Well, don't just stand
		 * there, do something!!
		 */
		int	k;
		Widget	w;

		sendit = 0;
		if ( (k = IsWMStepChild(event.xunmap.window)) != -1) {
			Boolean temp_bool = True;;
			w = wmstep_kids[k];
			ClientStructureNotify(w, (XtPointer)w, &event,
						&temp_bool);
		} /* end if (!= -1) */
	} /* end if(type == UnmapNotify && send_event) */
		if (sendit)
			ServiceEvent(&event);
} /* end for() */

} /* end of WMMainLoop */

/*
 * find_leader
 *
 */

extern Cardinal
find_leader(leader)
Window		leader;
{
	Cardinal	n;
	Cardinal	nchildren = num_wmstep_kids;
	WidgetList	children  = wmstep_kids;
	WMStepWidget	wm;

	for (n = 0; n < nchildren; n++) {
		wm = (WMStepWidget)children[n];
		if (
			XtClass(wm) == wmstepWidgetClass
		     && leader == wm-> wmstep.window
		)
			break;
	}
	return (n);
}

#define LEADER(wm) (wm)->wmstep.xwmhints->window_group

extern Window
find_highest_group OLARGLIST((wm, target_window_group))
OLARG(WMStepWidget, wm)
OLGRA(Window, target_window_group)
{
Window window_group = LEADER(wm);
int k;
	/* Return the window group if it's THIS window, or
	 * it's a window that isn't known to olwm (e.g.,
	 * isn't decorated or not under our control.
	 * However, STOP at window if you have found a window with
	 * a window group that matches - useful if you don't want to
	 * go all the way to the top window group for some reason.
	 */
	if ( (target_window_group != (Window) 0) &&
				(window_group == target_window_group))
		return(window_group);
	if (window_group == wm->wmstep.window ||
			((k = IsWMStepChild(window_group)) == -1) )
		return(window_group);
	else
		return(find_highest_group((WMStepWidget)wmstep_kids[k],
					target_window_group));
} /* end find_highest_group() */

#undef LEADER

/*
 * KeyPressEvent
 *
 */

static Boolean
KeyPressEvent OLARGLIST((w, client_data, event))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XEvent *, event)
{
Display *		display = XtDisplay(w);
OlVirtualEventRec	ve;
WMStepWidget		wm;
WMStepPart		*wmstep;
int			i;
Widget			use_widget;
Boolean			passkeys = False;
WMStepWidget		would_be_current_wm = (WMStepWidget)NULL;
Window			focus_win = (Window)0;
int			revert_to = 0;


	/* who is the current window??
	 * Remember, even if the window is iconic, it can
	 * still be current.  The grabbed keys still count.
	 */

	for (i = 0; i < num_wmstep_kids; i++) {
		wm = (WMStepWidget)wmstep_kids[i];
		if ( XtClass(wm) == wmstepWidgetClass &&
#ifdef WITHDRAWNSTATE
			wm->wmstep.size != WMWITHDRAWN &&
#endif
			wm->wmstep.is_current) {
			would_be_current_wm = wm;
			break;
		}
	}
	if (i >= num_wmstep_kids || wm->wmstep.private_db == NULL) {
		/* nobody current, or no private database */
		use_widget = w; /* probably Frame */
		wm = (WMStepWidget) NULL;
	}
	else {
		use_widget = (Widget)wm->wmstep.child;
	}

		/* Send keypress event to the current window.
		 * propagate flag == False, event mask == KeyPressMask
		 * There is also a way to send this message to the window
		 * that is under the pointer - see the manual.
		 * 
		 */
/*
		event->xkey.send_event = True;
		XSendEvent(display, wm->wmstep.window, False, KeyPressMask,
								event);
		return(True);
	}
 */

OlLookupInputEvent(use_widget, event, &ve, OL_DEFAULT_IE);

/* Ungrab keyboard for the "grabbed" */
switch(ve.virtual_name) {
	case OL_NEXTAPP:
	case OL_PREVAPP:
	case OL_NEXTWINDOW:
	case OL_PREVWINDOW:
	case OL_WINDOWMENU:
			XAllowEvents(display, AsyncKeyboard, event->xkey.time);
			XtUngrabKeyboard(Frame,event->xkey.time);
			break;
	case OL_WM_OPENCLOSE:
	case OL_WM_SIZE:
	case OL_WM_PROPERTIES:
	case OL_WM_REFRESH:
	case OL_WM_BACK:
	case OL_WM_QUIT:
	case OL_WM_DISMISSTHIS:
	case OL_WM_DISMISSALL:
	case OL_WM_MOVE:
	case OL_WM_RESIZE:
	case OL_WM_OWNER:
	case OL_HELP:
	case OL_WM_RESTORE:
	case OL_WM_MINIMIZE:
	case OL_WM_MAXIMIZE:
			/* If menu is posted, then pay attention to
			 * accelerators.
			 */
			if ( (!NumMenushellsPosted) && wmrcs->pass_keys
				&& ( would_be_current_wm &&
				(!(IsIconic(would_be_current_wm)))) ) {
					XAllowEvents(display,
						ReplayKeyboard,
						event->xkey.time);
				passkeys = True;
			}
			else
				XAllowEvents(display, AsyncKeyboard,
						event->xkey.time);

			XtUngrabKeyboard(Frame,event->xkey.time);
/*
 * Change this to use the  event timestamp; timing problem is
 * causing me to lose focus...
			XAllowEvents(display, AsyncKeyboard, CurrentTime);
		XtUngrabKeyboard(Frame,CurrentTime);
 */
		break;
	default:
		/* Is it a private database key from the MWM_MENU cmd? */
		if (wm)
		{
			OlKeyOrBtnRec *keys = wm->wmstep.private_keys;
			if (wm->wmstep.private_db) {
			  for (i=0; i < wm->wmstep.private_keys_used; i++)
				if (keys[i].virtual_name == ve.virtual_name)
					break;
			  if (i < wm->wmstep.private_keys_used) {
				int k;

				if ( (!NumMenushellsPosted) &&
						wmrcs->pass_keys) {
					/* Icons - process; otherwise,
					 * dont process if passKeys is
					 * on.
					 */
					if (IsIconic(wm))
						XAllowEvents(display,
					 	  AsyncKeyboard,
						  event->xkey.time);
					else {
					XAllowEvents(display,
					 ReplayKeyboard, event->xkey.time);
					passkeys = True;
					}
				}
				else
					XAllowEvents(display,
					 AsyncKeyboard, event->xkey.time);

				XtUngrabKeyboard(Frame,event->xkey.time);
				/* we have the step widget, and we have the
				 * callback, so just do it.  But remember,
				 * we must find the right callback - it can
				 * only be for a button that has an
				 * accelerator, or we wouldn't be here.
				 * I want you to watch this closely, because
				 * I'm not sure how I actually came up with
				 * this, but it works.
				 */
				wmstep = (WMStepPart *)&(wm->wmstep);
				for (k=0; k < wmstep->private_buttons_used;
								k++)
				   if (wmstep->private_keys[i].default_value==
				     wmstep->private_buttons[k].accelerator){
					/* Without checking the actual
					 * function, set up the "args"
					 * for it.  Only a handful actually
					 * use an argument, but it's faster
					 * to just put this in.  When you
					 * set up this flat_call_data, the
					 * only functions that use it are those
	* that accept arguments; well, the callback will need a way to
	 * distinguish whether the call was from a menu callback or from here,
	 * because we otherwise don't know what the index in the combined_menu
	 * array will be (because the combined_menu array may not even be
	 * set up!!  Probably, we can use the index in the private_buttons
	 * array, and have a way for the menu to know where it came from.
	 * Set the MwmMenuArgsFlag to 1, telling the callback that the
	 * call came from a keypress here.  The callback should set it
	 * back to 0.
	 */
				OlFlatCallData	flat_call_data;

				/* Check "pass thru" flag */
				if ( !passkeys && (NumMenushellsPosted ||
					wmstep->private_buttons[k].selectProc!=
				 	  OlwmPass_Keys && !PassKeysThrough)||
				   (wmstep->private_buttons[k].selectProc ==
							OlwmPass_Keys) )
				   {
				   flat_call_data.item_index = k;
				   MwmMenuArgsFlag++;
				   (*(wmstep->private_buttons[k].selectProc))(
					(Widget)wm, (XtPointer)(&wm),
						 (XtPointer)&flat_call_data);
				   } /* if */
				   else { /* PassKeysThrough */
					/* Check passkeys again - if it is
					 * true, then we don't have to call
					 * SendKeyEvent.
					 */
					if (!passkeys)
						SendKeyEvent(wm, event);
				   } /* else */
				} /* found */
				/* Unpost menu if it is posted */
				if (NumMenushellsPosted)
					_OlPopdownCascade(
						_OlRootOfMenuStack(w),
						False);
				return(True);
			  } /* if (i < wm->private_keys_used) */
			} /* wm->wmstep.private_db */
		} /* End if (wm) block to look for "private" grabbed keys */
		break;
} /* switch */

/* if passkeys (set above) then return true because we consumed the event
 * by doing XAllowEvents with ReplayKeyboard.
 */
if (passkeys)
	return(True);

if (wm && (!NumMenushellsPosted) && PassKeysThrough) {
	SendKeyEvent(wm, event);
	return(True);
}

/* The following block of code searches for the current (active) winodow;
 * it it doesn't find one, and the keyboard input event being asked for
 * is a window menu operation, then DON'T arbitrarily choose a window
 * to send it to; instead, consume the event and wait for a NEXTAPP,
 * PREVAPP, NEXTWIN, or PREVWIN.
 */
{ /* begin block */
int n = 0;
switch(ve.virtual_name) {
        default:
                break;
/* Important - use one of these if nobody is current - but if nobody is
 * current and someone has focus, forget it.
 */
case OL_NEXTAPP:
case OL_PREVAPP:
case OL_NEXTWINDOW:
case OL_PREVWINDOW:
	if ( (n = FindCurrentWindow(False) ) == -1) {
		/* No current window - does anyone have focus? */
		XGetInputFocus(display, &focus_win, &revert_to);
		if (focus_win != None && focus_win !=
			  PointerRoot &&
			  XtWindowToWidget(display, focus_win) == NULL) {
			/* Could be an oerride_redirect - or a menu.
			 */
			XBell(XtDisplay(w), 0);
			break;
		}
		else
			/* Reset to 0 so we stay in the function */
			n = 0;
	}
	break;
case OL_WM_OPENCLOSE:
case OL_WM_SIZE:
case OL_WM_PROPERTIES:
case OL_WM_REFRESH:
case OL_WM_BACK:
case OL_WM_QUIT:
case OL_WM_DISMISSTHIS:
case OL_WM_DISMISSALL:
case OL_WM_MOVE:
case OL_WM_RESIZE:
case OL_WM_OWNER:
case OL_WM_RESTORE:
case OL_WM_MINIMIZE:
case OL_WINDOWMENU:
case OL_WM_MAXIMIZE:
        if ( (n = FindCurrentWindow(False) ) == -1) {
                XBell(XtDisplay(w), 0);
                break; /* consume */
        }
        else
                break;
} /* switch */
if (n == -1)
        return(True);
} /* end block */

switch(ve.virtual_name) {

default:
   if (!IsModifierKey(ve.keysym)) {
	if (NumMenushellsPosted || wm_help_win_mapped == True) {
		/* dispatch the event, don't consume it here */
		return(False);
	}
	else
		XBell(XtDisplay(w), 0);
   }
   break;
case OL_NEXTAPP:
case OL_PREVAPP:
case OL_NEXTWINDOW:
case OL_PREVWINDOW:
	if (wmrcs->pointerFocus)
		return(True);
case OL_WINDOWMENU:
	/* If the window menu is posted when we get one of these above,
	 * just ignore the event and "consume" it (return True);
	 * else fall through.
	 */
	if (NumMenushellsPosted)
		return(True);
case OL_WM_OPENCLOSE:
case OL_WM_SIZE:
case OL_WM_PROPERTIES:
case OL_WM_REFRESH:
case OL_WM_BACK:
case OL_WM_QUIT:
case OL_WM_DISMISSTHIS:
case OL_WM_DISMISSALL:
case OL_WM_MOVE:
case OL_WM_RESIZE:
case OL_WM_OWNER:
case OL_WM_RESTORE:
case OL_WM_MINIMIZE:
case OL_WM_MAXIMIZE:
   {
/* BEGIN FIRST CHUNK - function FindCurrentWindow */
	Cardinal	n;
	int		k;	/* must be signed! */
	Cardinal	nchildren = num_wmstep_kids;
	WidgetList	children  = wmstep_kids;
	WMStepWidget	wm;
	WMStepPart *	wmstep;
	Window		leader;
	WMMenuInfo *	WMMenu = WMCombinedMenu;

#define LEADER(wm) (wm)->wmstep.xwmhints->window_group

	if ( (n = FindCurrentWindow(True) ) == -1) {
		XBell(XtDisplay(w), 0);
		break; /* consume */
	}
	wm = (WMStepWidget)children[n];


	/* We have now selected a current window, n;
	 * wm = the current window step widget.
	 * Is this a motif client, and are there restrictions on
	 * the input focus?  If there are, set retval, return.
	 */
	if (CheckMotifFocusStops(wm, ve.virtual_name)) {
		XBell(XtDisplay(w), 0);
		return(True);
	}

	/* Find the group leader of the current window */
	leader = find_highest_group(wm, (Window) 0);

	/*
	 * Bring the window menu down in case it is up.
	 *
	 * MORE: Should this be done on ANY keystroke that gets to us?
	 */
	switch(ve.virtual_name) {
	case OL_WM_OPENCLOSE:
	case OL_WM_SIZE:
	case OL_WM_PROPERTIES:
	case OL_WM_REFRESH:
	case OL_WM_BACK:
	case OL_WM_QUIT:
	case OL_WM_DISMISSTHIS:
	case OL_WM_DISMISSALL:
	case OL_WM_MOVE:
	case OL_WM_RESIZE:
	case OL_WM_OWNER:
	case OL_WM_RESTORE:
	case OL_WM_MINIMIZE:
	case OL_WM_MAXIMIZE:
		if (NumMenushellsPosted != 0) {
			WMStepWidget	wm;

			wm = (WMStepWidget)(WMMenu->w);
		

			if (currentGUI == OL_OPENLOOK_GUI &&
					!HasPushpin(wm) && 
					wm->wmstep.decorations & WMHasFullMenu)
				FlipMenuMark(wm);
			/* Bring down the menu */
			_OlPopdownCascade(_OlRootOfMenuStack(w), False);
		}
		break;
	} /* switch - menu operations */

	switch(ve.virtual_name) {

	case OL_NEXTAPP:
	case OL_PREVAPP:
		/*
		 * Find next/previous group-leader window outside current
		 * window group (if any):
		 * The 4th argument tells the function no to selectively
		 * pick out iconic state stepchildren for the next or
		 * previous application.
		 */
		k = Next_Prev_Application(wm, ve.virtual_name, n, 0);
		goto SetIt;

	case OL_NEXTWINDOW:
	case OL_PREVWINDOW:
		/*
		 * Find next or previous window in current window group:
		 * Simply gather all windows in the "group",
		 * where group may include a higher, nested group.
		 * Ignore the focus count of each step widget;
		 *
		 * wm = the current window,
		 * n = the index of wm in wmstep_kids.
		 */

		k = Next_Prev_Window(wm, ve.virtual_name, n, 0);
		/*
		 * Having found next or previous window--group or
		 * follower--make it current and move the focus.
		 */
SetIt:		wm = (WMStepWidget)children[k];

		/*
		 * MORE: The following is essentially equal to Select(),
		 * so perhaps it should be replaced with that? Problem
		 * is, Select() checks mouse button events, and we don't
		 * have any.
		 */
		/* mlp - Debatable - If pointerFocus, this shouldn't be
		 * done at all - should only be allowed if click-to-type.
		 */
		RaiseSetFocus(wm);
		break;

	case OL_WINDOWMENU:
		/*
		 * Pop up window menu for current window.
		 *
		 * MORE: Hitting the OL_WINDOWMENU key while the
		 * menu is still posted causes another menu to pop
		 * up!
		 *
		 * MORE: Warp pointer to Menu Mark?
		 * If you do this, you must check for
		 * cases where the window header is off
		 * the screen. In this case, warp pointer
		 * to some edge that is visible.
		 * You must also check for case where
		 * is no menu mark (pushpin instead,
		 * or no header whatsoever).
		 *
		 * With no warping, menu will post in
		 * funnny places.
		 *
		 * MORE: Are there some windows that
		 * shouldn't get menus but can have focus?
		 * If so, those should be screened out
		 * here.
		 */
		if (!NumMenushellsPosted) {
			OlwmPostMenu(wm, event);
		}
		break;

	case OL_WM_RESTORE:
		switch(wm->wmstep.size) {
			case WMICONICNORMAL:
			case WMICONICFULL:
			case WMFULLSIZE:
				/* O.K. */
				MenuMotifRestore((Widget)wm, (XtPointer)wm,
							(XtPointer)NULL);
				break;
			default:
				XBell(XtDisplay(w), 0);
				break;
		} /* switch (wm->wmstep.size) */
		break;

	case OL_WM_MINIMIZE:
		if ( (wm->wmstep.menu_functions & WMFuncMinimize) &&
						(!(IsIconic(wm))) ) {
			MenuOpenClose((Widget)wm, (XtPointer)&wm,
							(XtPointer)NULL);
		}
		break;

	case OL_WM_OPENCLOSE:
		if (HasFullMenu(wm))
			MenuOpenClose ((Widget)0, (XtPointer)&wm, (XtPointer)0);
		break;

	case OL_WM_SIZE:
	case OL_WM_MAXIMIZE:
		if (HasFullMenu(wm))
			MenuFullRestore ((Widget)0, (XtPointer)&wm, (XtPointer)0);
		break;

	case OL_WM_PROPERTIES:
		XBell(XtDisplay(w), 0);
		break;

	case OL_WM_REFRESH:
		MenuRefresh ((Widget)0, (XtPointer)&wm, (XtPointer)0);
		break;

	case OL_WM_BACK:
		MenuBack ((Widget)0, (XtPointer)&wm, (XtPointer)0);
		break;

	case OL_WM_QUIT:
                if (currentGUI == OL_OPENLOOK_GUI && HasFullMenu(wm) ||
				wm->wmstep.menu_functions & WMFuncClose)
			MenuQuit ((Widget)0, (XtPointer)&wm, (XtPointer)0);
		break;

	case OL_WM_DISMISSTHIS:
		if (HasLimitedMenu(wm))
			MenuDismiss ((Widget)0, (XtPointer)&wm, (XtPointer)0);
		break;

	case OL_WM_DISMISSALL:
		if (HasLimitedMenu(wm))
			MenuDismissPopups ((Widget)0, (XtPointer)&wm, (XtPointer)0);
		break;

	case OL_WM_MOVE:
		Menu_Move ((Widget)0, (XtPointer)&wm, (XtPointer)0);
		break;

	case OL_WM_RESIZE:
		if ((!(IsIconic(wm))) && Resizable(wm))
			Menu_Resize ((Widget)0, (XtPointer)&wm, (XtPointer)0);
		break;

	case OL_WM_OWNER:
		if (HasLimitedMenu(wm))
			MenuOwner ((Widget)0, (XtPointer)&wm, (XtPointer)0);
		break;

	}
   }
   break;

case OL_HELP:
   HandleHelpKeyPress(w, event);
   break;
}   /* end switch (the big one) */
FPRINTF((stderr, "got a key press\n"));
return(True);

} /* end of KeyPressEvent */

/*
 *  CatchRedirectErrors
 */
extern int
CatchRedirectErrors OLARGLIST((display, event))
OLARG(Display *, display)
OLGRA(XErrorEvent *, event)
{
#if !defined(I18N)
	fprintf(stderr,"Another window manager is already running!!\n");
	exit(-1);
#else
	OlVaDisplayErrorMsg(display, OleNolwm, OleTrunning,
			OleCOlClientOlwmMsgs, OleMolwm_running, NULL);
#endif
} /* CatchRedirectErrors */

/*
 * IgnoreClientErrors
 *
 */

extern int
IgnoreClientErrors OLARGLIST((display, event))
OLARG(Display *, display)
OLGRA(XErrorEvent *, event)
{
#undef DBG_ICE

#ifdef DBG_ICE
	char buff[2048];

	XGetErrorText(display, event-> error_code, buff, 2047);
	printf("Client Error: %s, major_code: %d\n", buff, event->request_code);
fprintf(stderr,"\tResource ID=%x\n", event->resourceid);
#endif
FPRINTF((stderr,"Error: error code = %d, major_code: %d\n",event->error_code, event->request_code));
switch (event-> error_code)
   {
   case BadDrawable:
   case BadMatch:
   case BadPixmap:
   case BadWindow:
      return (0);
      break;
   default:
#ifdef COMPLETELY_BUG_FREE
      return (_XDefaultError(display, event));
#else
      FPRINTF((stderr," ignoring error  %d %x\n", event-> error_code));
      return (0);
#endif
   }

} /* end of IgnoreClientErrors */

/*
 * WMReparent
 * Callback, called by CreateHelpTree() (possibly others())
 * For CreateHelpTree(), it adds a popCallback (for this) to the
 * help shell
 * The function (when applicable) calls Reparent() to create a wmstep widget
 * for the widgets window.
 */

extern void
WMReparent OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
ShellWidget  shell  = (ShellWidget)w;
WMStepWidget wm     = (WMStepWidget)XtParent(w);
/* XEvent *     event  = (XEvent *)call_data; */
Window       window;
int          i;

/* Remove anything related to the event stuff - this was originally
 * there for use with a possibly combined olwm/olwsm - now (R5),
 * event is not NULL under any circumstances - it's equal to "".
if (event == NULL)
   {
 */
   /* First, if NULL event... do nothing if override_redirect*/
   if (shell-> shell.override_redirect == True)
      ;
   else
      {
      XtRealizeWidget(w);
      window = XtWindow(w);
      /* if already a subclass of a step widget, then no need to do work-
       * it's already decorated as much as it's going to be.
       */
      if (!(wm && XtIsSubclass((Widget)wm, wmstepWidgetClass)))
         {
	 /* This is NOT a subclass of wmstep widget - make it one! */
#if 0
         wm = (WMStepWidget)Reparent(window, NULL);
#endif
	 /* If the shell this was called with is the help window, then set
	  * help_parent to wm (the wmstep widget); else add a destroy
	  * callback.
	  */
         if ((Widget)shell == help_shell)
	    {
	    if (help_parent == NULL)
		    help_parent = (WMStepWidget)Reparent(window, NULL);
	    wm = help_parent;
	    }
         else
	    {
            wm = (WMStepWidget)Reparent(window, NULL);
            XtAddCallback((Widget)shell, XtNdestroyCallback,
			(XtCallbackProc)DestroyParent, (XtPointer)wm);
	    } 
         }
      RaiseLowerGroup(wm, WMRaise);
      XtMapWidget((Widget)wm);
      }
/*
 * event related stuff - being removed (see comment above).
   }
else
   {
   switch (event-> type)
      {
      case MapNotify:
         if (!event-> xmap.override_redirect)
            {
            for (i = 0; i < window_list->used; i++)
               {
               wm = (WMStepWidget)window_list->p[i];
               if (wm-> wmstep.window == event-> xmap.window)
                  break;
               }
            if (i == window_list->used)
               { 
               FPRINTF((stderr, "reparenting my shell\n"));
               wm = (WMStepWidget)Reparent(event-> xmap.window, NULL);
               if (event-> xmap.window == XtWindow(help_shell))
                  help_parent = wm;
               }
            else
               {
               FPRINTF((stderr, "my shell already reparented\n"));
               RaiseLowerGroup(wm, WMRaise);
               }
            }
         else
            FPRINTF((stderr, "my override_redirect menu???\n"));
         break;
      default:
         break;
      }
   }
 *
 */

} /* end of WMReparent */

/*
 * ReparentWindows
 * Called from StartWindowManager().  Get list of windows now mapped.
 *
 */

static void
ReparentWindows OLARGLIST((WMDisplay, WMRoot))
OLARG(Display *, WMDisplay)
OLGRA(Window,    WMRoot)
{
Window            root;
Window            parent;
Window *          children;
unsigned int      num_children;
Window *          w;
XWindowAttributes xwa;

Cardinal	n;
int		k;	/* must be signed! */
WidgetList	wchildren;
WMStepWidget	wm;
WMStepPart *	wmstep;
Window		leader;
int		nchildren;
unsigned long	window_state;

/* Get list of windows now on the screen (e.g., children of root */
if (XQueryTree(WMDisplay, WMRoot, &root, &parent, &children, &num_children))
   {
   if (num_children != 0)
      {
      for (w = children; w < &children[num_children]; w++)
         {
         if (XGetWindowAttributes(WMDisplay, *w, &xwa) == 0)
            WMError(WM_CANT_GET_WA, (XtPointer)*w);
         else
            {
            if (xwa.map_state == IsViewable && xwa.override_redirect == False)
               {
	       /* GetWindowState() - calls GetWMState().
		* Call Reparent() for those windows not in WithdrawnState.
		*/
/*** - what about IsUnmapped ??? */
		/*
		 *   -Think of this case:  bring up olwsm
		 * menu, pin it, and olwm will decorate it and set
		 * state = Normal; next unpin it => olwm sets state =
		 * Withdrawn.  Kill olwm, and bring up olwsm menu and pin
		 * it => you get an undecorated pinned menu.
		 * Finally, bring up olwm; it will see a state == Withdrawn
		 * and unmap it.  You can't get this menu back again.
		 *...
 */
               if ((window_state = GetWindowState(WMDisplay, *w)) == WithdrawnState) {
			/* Probably, this window was withdrawn by another
			 * window manager, or this window manager, and
			 * that window manager died an unstoppable
			 * death - for example, kill -9.  Think of  the case
			 * above:  the menu code thinks that it is
			 * alive an well, if the menu was brought up w/o
			 * the help of a window manager; yet the
			 * stupid state of the item is Withdrawn;  best bet:
			 * decorate it as a normal window.  Here's why:
			 * the window is certainly viewable because the
			 * if above said so;  if it was unmapped, we wouldn't
			 * know about it in this if.  Therefore, this
			 * Withdrawn state must be wrong, right??
			 * Don't take any changes, just reparent the window
			 * and put it in normal state.  Worst that can
			 * happen is it appears on the screen when it shouldn't.
			 */
			WMState wms;
			wms.state = NormalState;
			wms.icon = (Window)NULL;
			SetWMState(WMDisplay, *w, &wms);

		}
/*	
                  XUnmapWindow(WMDisplay, *w);
               else
 */
                  (void)Reparent(*w, &xwa);
		  /* Reparent() uses w and xwa as resources, creates a
		   *  wmstep widget
		   */
               }
            }
         }
   /* Set Focus to someone (make current) if any windows out there */
	nchildren = num_wmstep_kids;
	wchildren  = wmstep_kids;
	for (n = 0; n < nchildren; n++) {
		wm = (WMStepWidget)wchildren[n];
			if (
				(XtClass(wm) == wmstepWidgetClass)
			     	&& LEADER(wm) == wm-> wmstep.window
			)
				break;
	} /* end for loop */
	if ( n < nchildren) {
		/* can n be >= nchildren? */
		/* If pointerFocus, you don't know which window to raise,
		 * because you have not queried the pointer - so don't
		 * raise anything and don't set focus yet.
		 */
		if (!(wmrcs->pointerFocus))
			RaiseSetFocus(wm);
	}
      } /* num_children != 0 */
   XFree((char *) children);
   children=NULL;
   } /* XQueryTree() != 0 */

} /* end of ReparentWindows */


/*
 * WMIconGravityConverter
 * This is an Xt OLD format resource "Converter" function, called from
 * SetupWindowManager();
 *
 *  An XrmValue is a typedef for a struct {
					    unsigned int size;
					    caddr_t addr;
					  }
 */

static void
WMIconGravityConverter OLARGLIST((args, num_args, fromVal, toVal))
OLARG(XrmValue *, args)
OLARG(Cardinal, num_args)
OLARG(XrmValue *, fromVal)
OLGRA(XrmValue *, toVal)
{
static WMIconGravity gravity;
char * p = fromVal-> addr;

if (0 == strcmp(p, "north"))
   gravity = WMNorthGravity;
else 
   if (0 == strcmp(p, "south"))
      gravity = WMSouthGravity;
   else 
      if (0 == strcmp(p, "east"))
         gravity = WMEastGravity;
      else 
         if (0 == strcmp(p, "west"))
            gravity = WMWestGravity;
         else
            gravity = WMSouthGravity;

toVal-> addr = (caddr_t)&gravity;
toVal-> size = sizeof(WMIconGravity);

} /* end of WMIconGravityConverter */

/*
 * Legal combination of colors  (in order of priority):
 *	1) Background	2) Foreground	3) InputFocus
 *	-------------	------------	-------------
 *	   X		  Y (!= X)	  Z (!= Y) && !=X
 *
 *  - Background is the window frame background.
 *  - Foreground is most noticeable with the window /icon title.
 *  - inputFocusColor only takes affect when the window gains focus.
 * For the most part, the three must be mutually exclusive; however,
 * on a black and white system, the input focus color is achieved by
 * reversing the text foreground and the background.  Thus, we
 * introduce a new variable, headcase.  If the headcase global variable
 * is non-zero, it tells the re-display function that when a client
 * has input focus and it has a header, then the foreground and background
 * colors should be reversed.
 *
 * - if there are duplicate colors for anything, they will be resolved
 * with black and white for substitutes, regardless of monochrome or color
 * monitors.  InputFocusColor is resolved last.  If using a monochrome
 * monitor or the only colors mentioned in the foreground, background,
 * and inputfocus color resources are white and black, then resolve
 * inputfocuscolor later when re-displaying the header;
 * otherwise, resolve it now with makeshift black or white colors.
 * takes input focus.
 */
extern void
ResolveColors OLARGLIST((foreground, background, inputfocus))
OLARG(Pixel *, foreground)
OLARG(Pixel *, background)
OLGRA(Pixel *, inputfocus)
{
Pixel	back = *background,
	fore = *foreground,
	input = *inputfocus;
int	change = 0;
Pixel	white = WhitePixelOfScreen(XtScreen(Shell));
Pixel	black = BlackPixelOfScreen(XtScreen(Shell));

	headcase = 0;
	if (back == fore) {
		/* Case 1: foreground and background colors are equal.
		 * Change to foreground only - to black if background is
		 * white, white otherwise.  The white foreground will not
		 * look good on very light-colored backgrounds in the
		 * window header, but what's to do?	
		 */
		if (DefaultDepthOfScreen(XtScreen(Shell)) != 1)
#if !defined(I18N)
		fprintf(stderr,"Window Manager Warning: Same foreground and background colors given, now resolving...\n");
#else
	OlVaDisplayWarningMsg(XtDisplay(Shell), OleNdupColor,
		OleTforegroundBackground,
		OleCOlClientOlwmMsgs, OleMdupColor_foregroundBackground,
		NULL);
#endif
		if (back == white)
			fore = black; /* Make it black on white */
		else
			fore = white; /* Make it white on whatever */
	}
	/* Now, the foreground and background colors are not the same.
	 * The next thing to worry about (don't worry be happy) is
	 * the input focus color can't be the same as the foreground color.
	 * otherwise the text will not be visible.
	 * It should also be different from the background color, so
	 * the difference is visible between a window with input focus
	 * and one without it!
	 * On exit from this function, the only things that may still be
	 * equal are the input focus color and the background.
	 */
	if (input == fore)
		change = 1;
	if (input == back)
		change = 2;
	if (input == *foreground)  {
		if (DefaultDepthOfScreen(XtScreen(Shell)) != 1)
#if !defined(I18N)
			fprintf(stderr,"Window Manager Warning: Same foreground and inputWindowHeader colors given, now resolving...\n");
#else
	OlVaDisplayWarningMsg(XtDisplay(Shell), OleNdupColor,
		OleTforegroundInputWindowHeader,
		OleCOlClientOlwmMsgs, OleMdupColor_foregroundInputWindowHeader,
		NULL);
#endif
		}
	if (input == *background)  {
		if (DefaultDepthOfScreen(XtScreen(Shell)) != 1)
#if !defined(I18N)
			fprintf(stderr,"Window Manager Warning: Same background and inputWindowHeader colors given, now resolving...\n");
#else
	OlVaDisplayWarningMsg(XtDisplay(Shell), OleNdupColor,
		OleTbackgroundInputWindowHeader,
		OleCOlClientOlwmMsgs, OleMdupColor_backgroundInputWindowHeader,
		NULL);
#endif
	}
	if (change == 1) {
		/* input focus color and foreground are the same; back must be
		 * diff.  What is it??
		 */
		if (back == black) {
			if (fore != white)
				input = white;
			else
				headcase = 1; /* reverse something */
				/* headcase 1:  back = black,
						fore = white.
				  On focus, make back= white, fore = black.
				 */
		}
		else
			if (back == white) {
				if (fore != black)
					input = black;
				else
					headcase = 2; /* another reverse */
				/* headcase 2: back = white,
						fore = black;
				  On focus, make back = black,
						fore = white.
				 */
			}
			else { /* back is neither black or white */
				if (fore == black)
					input = white;
				else 
					input = black;
			}
	} /* end if(change == 1) */
	if (change == 2) { /* input == back - these must be diff. */
		if (back == black) {
			if (fore == white)
				headcase = 1; /* must reverse something */
				/* Headcase 1: back = black,
						fore = white;
				 On focus, make back = white,
						fore = black.
				 */
			else
				input = white;
		}
		else
			if (back == white) {
				if (fore == black)
					/* headcase 2: back = white,
							fore = black;
					 On focus, make back = black,
							fore = white.
					 */
					headcase = 2;
				else
					input = black;
			}
			else {
				/* back and input are some neat color */
				if (fore == white)
					input = black;
				else
					input = white;
			}
	} /* end if (change == 2) */

	*foreground = fore;
	*background = back;
	*inputfocus = input;
} /* end ResolveColors() */
					

/*
 * WMDynamic
 */
static void
WMDynamic OLARGLIST((data))
	OLGRA( XtPointer, data)
{
    Widget		frame = (Widget)data;
    WMStepWidget	temp;
    Boolean           redisplay, fontflag = False;
    int                       i, result=False;

    static Boolean	first_call = TRUE;
    Arg			args[1];
    static OlDefine	PreviousHelpModel;
    OlDefine		help_model = (OlDefine)OL_POINTER;
    Screen		*screen = XtScreen(frame);
	MotifCompResourceInfo	*mcri;
	MotCompAppearanceInfo	*mcai;
	int		shadow_flag, ashadow_flag, f_flag;
    Display *    display = XtDisplay(frame);
    Window       window  = XtWindow(frame);


#define ASSIGN_PREVIOUS(ptr)					\
	PreviousIconBorder	= ptr->iconBorder;		\
	PreviousIconGravity	= ptr->iconGravity;		\
	PreviousInputFocusColor	= ptr->inputFocusColor;		\
	PreviousWindowFrameColor= ptr->windowFrameColor;	\
	PreviousForegroundColor	= ptr->foregroundColor;		\
	PreviousBackgroundColor	= ptr->backgroundColor;		\
	PreviousFocusPolicy	= ptr->pointerFocus;		\
	PreviousFont		= ptr->font;			\
	PreviousIconFont	= ptr->iconFont;		\
	PreviousFontList	= ptr->font_list;		\
	PreviousUsingThreeD	= OlgIs3d();			\
	PreviousIconForeground	= ptr->iconForeground;		\
	PreviousIconBackground	= ptr->iconBackground;		\
	PreviousIconParentRelative = ptr->iconParentRelative;	\
	PreviousHelpModel	= help_model

    shadow_flag = 0;
    ashadow_flag = 0;
    f_flag = 0;
    XtGetApplicationResources(frame, (XtPointer)wmrcs,
			global_resources, num_global_resources, NULL, 0);

    XtSetArg(args[0], XtNhelpModel, &help_model);
    OlGetApplicationValues(frame, args, 1);

    /* This is mandatory */
    if (currentGUI == OL_MOTIF_GUI) {
	wmrcs->moveOpaque = False;
	wmrcs->iconGrid = False;
	wmrcs->selectDoesPreview = False;
	wmrcs->iconParentRelative = False;
    }
    else
	/* Shut iconGrid off at all times in this release, until we can fix up
	 * the grid placement for all gravity's - now it will not even work for
	 * south gravity - just try moving it from it's original position,
	 * and you will not be able to move it back. I suspect it will work from
	 * North, that's it - fix would be in PollDragWindowAround, where 
	 * iconGrid is mentioned.
	 */
	wmrcs->iconGrid = False;

	/* Monochrome - icons must have borders */
	if (DefaultDepthOfScreen(screen) == 1)
		wmrcs->iconBorder = True;

    if (first_call == TRUE)
    {
	first_call = FALSE;

	if (help_model == (OlDefine)OL_POINTER)
	{
		OlGrabVirtualKey(frame, OL_HELP, False,
					GrabModeAsync, GrabModeSync);
	}

	PreviousBasicLocale = XtNewString(wmrcs->xnlLanguage);

	/* Colors */
	CheckColorSanity(screen, (Boolean)True);

	/* if iconParentRelative then iconBorder MUST be false.
	 * On a different note, if iconParentRelative, then the
	 * background for the screen can't be the normal X background
	 * or you will not be able to read the label on the icon -
	 * it must be some color.
	 *
	 * This can override the iconBorder when it comes to monochrome
	 * displays - but should we always turn this off for monochrome
	 * displays??
	 */
	if (wmrcs->iconParentRelative)
		wmrcs->iconBorder = False;

	/* Save wmrcs->font_list - this variable is not dynamic.  Always
	 * reference ol_font_list
	 */
	if (wmrcs->font_list)
		ol_font_list = wmrcs->font_list;
	else
		ol_font_list = NULL;

	/* Fix up fonts */
	if (wmrcs->font == NULL) {
		/* Always have some font set up */
		if (currentGUI == OL_MOTIF_GUI) {
		   wmrcs->font = _OlGetDefaultFont(Frame,
"-misc-fixed-medium-r-semicondensed--13-120-75-75-c-60-iso8859-1");
		}
		else
			wmrcs->font = _OlGetDefaultFont(Frame, NULL);
	} /* font */
	if  (currentGUI == OL_OPENLOOK_GUI && !(wmrcs->iconFont))
		   wmrcs->iconFont = _OlGetDefaultFont(Frame,
"-misc-fixed-medium-r-semicondensed--13-120-75-75-c-60-iso8859-1");

	ASSIGN_PREVIOUS(wmrcs);
    }
    else
    {
	UpdateMenuItems(frame);

	if (help_model != PreviousHelpModel)
	{
		if (help_model == (OlDefine)OL_POINTER)
			OlGrabVirtualKey(frame, OL_HELP, False,
					GrabModeAsync, GrabModeSync);
		else
			OlUngrabVirtualKey(frame, OL_HELP);
	}

	redisplay =
		(PreviousWindowFrameColor != wmrcs->windowFrameColor) ||
		(PreviousInputFocusColor  != wmrcs->inputFocusColor) ||
		(PreviousForegroundColor  != wmrcs->foregroundColor) ||
		(PreviousBackgroundColor  != wmrcs->backgroundColor) ||
		(PreviousUsingThreeD	!= OlgIs3d()) ||
		(PreviousFocusPolicy	!= wmrcs->pointerFocus) ||
		(PreviousFont		!= wmrcs->font) ||
		(PreviousFontList	!= wmrcs->font_list) ||
		(PreviousIconBackground !=  wmrcs->iconBackground) ||
		(PreviousIconForeground !=  wmrcs->iconForeground) ||
		(PreviousIconBorder	!= wmrcs->iconBorder);

        if(PreviousFont           != wmrcs->font)
                fontflag = True;

	if( PreviousFocusPolicy != wmrcs->pointerFocus)	{
	    void    (*proc)();
            proc = (void (*)()) (wmrcs->pointerFocus ?
                        XtAddEventHandler : XtRemoveEventHandler);
	    for (i = 0; i < window_list->used; i++)
            {
                WMStepPart *    wmstep ;
                temp = (WMStepWidget)window_list->p[i];
                wmstep  = (WMStepPart *)&(temp-> wmstep);

		if (currentGUI == OL_MOTIF_GUI &&
                                        motwmrcs->resizeCursors) 
		;
		else
                	(*proc)(temp, EnterWindowMask | LeaveWindowMask, False,
                                        ClientEnterLeave, NULL);

                if(IsIconic(temp) && (wmstep->icon_widget))      
        	   (*proc)(wmstep->icon_widget, EnterWindowMask|LeaveWindowMask,
                 		False, IconEnterLeave, (XtPointer) temp);
	    }
	}

	if (PreviousIconGravity != wmrcs->iconGravity)
		ResetIconPosition(XtScreen(frame));
	if (strcmp(PreviousBasicLocale, wmrcs->xnlLanguage) != 0) {
		XtFree(PreviousBasicLocale);
		PreviousBasicLocale = NULL;
		PreviousBasicLocale = XtNewString(wmrcs->xnlLanguage);
		/* Now update the labels where necessary, such as menus,
		 * titles, etc.  To do this, we need to rebuild the
		 * RESOURCE_MANAGER database.  That means re-reading
		 * the app-defaults file, etc.  There's a good way to
		 * do this: exec a whole new window manager!!  Go for it.
		 */
		RestartWindowMgr(XtDisplay(frame));
		/* You should be gone by this point */
	}

        if (currentGUI == OL_MOTIF_GUI) {
                int i;

                if (PreviousInputFocusColor  != wmrcs->inputFocusColor) {
			ashadow_flag = 1;
                        for (i=0; i < NUM_MOTIF_COMPONENTS; i++) {
                                mcri = (MotifCompResourceInfo *) &(motCompRes[i]);
                                mcai = (MotCompAppearanceInfo *) &(mcri->compai);
                                mcai->activeBackground = wmrcs->inputFocusColor;                	}
		}
                if (PreviousBackgroundColor  != wmrcs->backgroundColor) {
			shadow_flag = 1;
                        for (i=0; i < NUM_MOTIF_COMPONENTS; i++) {
                                mcri = (MotifCompResourceInfo *) &(motCompRes[i]);
                                mcai = (MotCompAppearanceInfo *) &(mcri->compai);
                                mcai->background = wmrcs->backgroundColor;
			}
                }
        }

	if (currentGUI == OL_MOTIF_GUI && mcai->fontList) {
		if (wmrcs->font != PreviousFont && wmrcs->font){
			/* Grounds for changing the fonts */
	int i;
	for (i=0; i < NUM_MOTIF_COMPONENTS; i++) {
	   mcri = (MotifCompResourceInfo *) &(motCompRes[i]);
	   mcai = (MotCompAppearanceInfo *) &(mcri->compai);
	   mcai->fontList = NULL;
	} /* for */
		} /* if */
	} /* if MOTIF */
	/* Fix up fonts */
	if (wmrcs->font == NULL) {
		/* Always  set up wmrcs->font */
		if (currentGUI == OL_MOTIF_GUI) {
			wmrcs->font = _OlGetDefaultFont(Frame,
"-misc-fixed-medium-r-semicondensed--13-120-75-75-c-60-iso8859-1");
		}
		else
			wmrcs->font = _OlGetDefaultFont(Frame, NULL);
	} /* font */

	if  (currentGUI == OL_OPENLOOK_GUI && !(wmrcs->iconFont))
		   wmrcs->iconFont = _OlGetDefaultFont(Frame,
"-misc-fixed-medium-r-semicondensed--13-120-75-75-c-60-iso8859-1");

	if (redisplay)
	{
		/* Make sure you save these values BEFORE ResolveColors(),
		 * because ResolveColors() may change the values within wmrcs.
		 * In that case, you need the previous values in
		 * Previous *** so you know what the TRUE previous values are.
		 */
		Boolean re_create = (PreviousUsingThreeD != OlgIs3d());

							 /* for windows that
							  * use OL_WIN_COLORS
							  */
		Boolean re_resolve = 1;	/* to force resolve color conflict */

		Boolean icon_resolve =
			(PreviousIconForeground != wmrcs->iconForeground) ||
			(PreviousIconBackground != wmrcs->iconBackground);
		

		ASSIGN_PREVIOUS(wmrcs);
		CheckColorSanity(screen, re_resolve);

	/* Fix up fonts */
	if (wmrcs->font == NULL) {
		/* Skip font list, set up font */
		if (currentGUI == OL_MOTIF_GUI) {
			wmrcs->font = _OlGetDefaultFont(Frame,
"-misc-fixed-medium-r-semicondensed--13-120-75-75-c-60-iso8859-1");
		}
		else
			wmrcs->font = _OlGetDefaultFont(Frame, NULL);
	} /* font */

		if (PreviousFont != wmrcs->font && currentGUI == OL_MOTIF_GUI) {
		  DestroyMotifFeedbackWindow(XtDisplay(Frame),
						XtScreen(Frame));
		  CreateMotifFeedbackWindow(XtDisplay(Frame),
						XtScreen(Frame));
		}

		for (i = 0; i < window_list->used; i++)
		{
                        WMStepPart *    wmstep ;
			temp = (WMStepWidget)window_list->p[i];
                        wmstep  = &temp-> wmstep;

                        if ((fontflag==True) && wmrcs->font){

                        int childwidth;
                        int childheight;
                        int newparentwidth;
                        int newparentheight;
                        int oldsize;
                        int fh;


                           fh =  OlFontHeight(wmrcs->font, ol_font_list)+3;
                           wmstep->metrics->motifButtonHeight = fh;
                           wmstep->metrics->motifButtonWidth = fh;

                           if(IsIconic(temp))   {
                                oldsize = wmstep->size;
                                wmstep-> size    = WMNORMALSIZE;
			   }

                           wmstep-> metrics = GetMetrics(temp);
                           childwidth = temp->wmstep.child->core.width;
                           childheight = temp->wmstep.child->core.height;
                        if (currentGUI == OL_OPENLOOK_GUI)      {
                           newparentwidth = ParentWidth(temp, childwidth);
                           newparentheight = ParentHeight(temp, childheight);
		
                           XtResizeWidget((Widget)temp, newparentwidth ,
                                         newparentheight , (Dimension)0);
                           XtMoveWidget(wmstep->child, OriginX(temp),
                                                         OriginY(temp));
                        }
                        else    {       /* motif */
                           newparentwidth = MParentWidth(temp, childwidth);
                           newparentheight = MParentHeight(temp, childheight);
                          XtResizeWidget((Widget)temp, newparentwidth ,
                                         newparentheight , (Dimension)0);
                           XtMoveWidget(wmstep->child, MOriginX(temp),
                                                         MOriginY(temp));
                        }
                           if(wmstep->icon_widget)      {
                                wmstep-> size    = oldsize;
                                wmstep-> metrics = GetMetrics(temp);
                           }
                        }       /* if fontflag */

			if (re_resolve &&
			    !temp->wmstep.olwincolors || icon_resolve)
			{
				/* Need to do this code if changing one
				 * the big 3 colors.
				 */
					/* Update component colors
					 * for inputFocusColor,
					 * and potentially fg and bg
					 */
				if (wmrcs->foregroundColor !=
						wmrcs->backgroundColor)
				{
					temp->wmstep.foreground_pixel =
						wmrcs->foregroundColor;
				}

				if (!wmrcs->parentRelative)
				{
					temp->core.background_pixel =
						wmrcs->backgroundColor;
					XSetWindowBackground(XtDisplay(temp),
						XtWindow(temp),
						wmrcs->backgroundColor);
				}
				if (icon_resolve) {
					WMStepWidget wm = (WMStepWidget)temp;
					if (IsIconic(wm) &&
							wm->wmstep.icon_widget)
						XtVaSetValues(wm->
						   wmstep.icon_widget,
						   XtNbackground,
							wmrcs->iconBackground,
						   (char *)NULL);
				}
						
			}
			if (currentGUI == OL_MOTIF_GUI && (shadow_flag || ashadow_flag) && f_flag == 0) {
				int j;

                        	for (j=0; j < NUM_MOTIF_COMPONENTS; j++) {
                                	mcri = (MotifCompResourceInfo *) &(motCompRes[j]);
                                	mcai = (MotCompAppearanceInfo *) &(mcri->compai);
                			if (shadow_flag) {
						GetShadowColors(temp,
							&(mcai->topShadowColor),
                                			&(mcai->bottomShadowColor));
						SetShadowGCS((Widget)temp, j, 0);
					if (mcai->topShadowColor == mcai->background) {
						mcai->topShadowColor = 
							GetLighterColor(XtScreen((Widget) temp), 
								mcai->topShadowColor);
}
							
					if (mcai->bottomShadowColor == mcai->background) {
						mcai->bottomShadowColor = 
							GetDarkerColor(XtScreen((Widget) temp), 
								mcai->bottomShadowColor);
}
					}
                			if (ashadow_flag && j < 3) {
						GetShadowColors(temp,
							&(mcai->activeTopShadowColor),
                                			&(mcai->activeBottomShadowColor));
						SetShadowGCS((Widget)temp, j, 1);
						if (mcai->activeTopShadowColor == mcai->activeBackground) {
							mcai->activeTopShadowColor = 
								GetLighterColor(XtScreen((Widget) temp), 
									mcai->activeTopShadowColor);
}
							
						if (mcai->activeBottomShadowColor == mcai->activeBackground) {
							mcai->activeBottomShadowColor = 
								GetDarkerColor(XtScreen((Widget) temp), 
									mcai->activeBottomShadowColor);
}
					}
				}
				f_flag = 1;
			}

	/* Call CreateGC() if the window does not use _OL_WIN_COLORS
	 * or it does and re_create is true (meaning we changed from
	 * 2D to 3D or vice-versa.
	 */
			if (!(temp->wmstep.olwincolors) || re_create )
			{
				CreateGC(temp, &temp-> wmstep, TRUE);
				XClearArea(XtDisplay(temp), XtWindow(temp),
						0, 0, 0, 0, False);
			}

				/* Redisplay all decorated windows */
			DisplayWM((WMStepWidget)window_list->p[i], NULL);
		} /* end for (all decorated windows) */
	} /* end if (redisplay)  */
    } /* end else (first_call) */
#undef ASSIGN_PREVIOUS
} /* end of WMDynamic */

static void
GetShadowColors OLARGLIST((wm, bright, dark))
OLARG(WMStepWidget, wm)
OLARG(Pixel	*, bright)
OLGRA(Pixel	*, dark)
{
	GC brightGC, darkGC;
	unsigned long mask;
	Pixel brightcolor, darkcolor;
	XGCValues gcvals;
	MotifCompResourceInfo	*mcri;
	MotCompAppearanceInfo	*mcai;
	OlgAttrs *mdm;

	mdm = OlgCreateAttrs(XtScreen((Widget)wm), wm->wmstep.foreground_pixel,
       		(OlgBG *)&(wm->core.background_pixel),
        	(Boolean)FALSE, (Dimension)12);

	mask = GCForeground;
	brightGC = OlgGetBrightGC(mdm);
	XGetGCValues(XtDisplay((Widget) wm), brightGC, mask, &gcvals);
	*bright = gcvals.foreground;
	darkGC = OlgGetBg3GC(mdm);
	XGetGCValues(XtDisplay((Widget) wm), darkGC, mask, &gcvals);
	*dark = gcvals.foreground;
	if (*dark == *bright) {
		*bright = WhitePixelOfScreen(XtScreen(wm));
		*dark = BlackPixelOfScreen(XtScreen(wm));
	}
	OlgDestroyAttrs(mdm);
}

/* 
 * CheckColorSanity.
 * If Boolean checkprev is false, go through the whole function; 
 * if Boolean checkprev is true, don't call ResolveColors()
 */
static void
CheckColorSanity OLARGLIST((scr, do_resolve))
OLARG(Screen *, screen)
OLGRA(Boolean, do_resolve)
{
Pixel white = WhitePixelOfScreen(screen);
Pixel black = BlackPixelOfScreen(screen);

	if (wmrcs->foregroundColor == ULONG_MAX ||
					wmrcs->backgroundColor == ULONG_MAX) {

		if ( wmrcs->foregroundColor == ULONG_MAX) {
			if (wmrcs->backgroundColor != black)
				wmrcs->foregroundColor = black;
			else
				wmrcs->foregroundColor = white;
		}
		if (wmrcs->backgroundColor == ULONG_MAX) {
			if (wmrcs->foregroundColor != white)
				wmrcs->backgroundColor = white;
			else
				wmrcs->backgroundColor = white;
		}
	} /* foregroundColor == ULONG_MAX || backgroundColor == ULONG_MAX */

	if (do_resolve)
		ResolveColors(&wmrcs->foregroundColor, &wmrcs->backgroundColor,
						&wmrcs->inputFocusColor);

	if (wmrcs->iconBackground == ULONG_MAX)
		wmrcs->iconBackground = wmrcs->backgroundColor;
	if (wmrcs->iconForeground == ULONG_MAX)
		wmrcs->iconForeground = wmrcs->foregroundColor;
	if (wmrcs->iconBackground == wmrcs->iconForeground) {
		wmrcs->iconBackground = white;
		wmrcs->iconForeground = black;
	}
} /* CheckColorSanity */





/*
 * AddKeyGrabs
 * mlp - called from StartWindowManager()
 * Passive Grab on all virtual keys, except for the HelpKey. (The help key
 * is set in WMDynamic().
 */

static void
AddKeyGrabs OLARGLIST((frame))
	OLGRA( Widget,	frame)
{
int i;
#define GRAB(v)	\
	OlGrabVirtualKey(frame, (v), False, GrabModeAsync, GrabModeSync)
#define COMMON_GRABS	9
#define OLWM_GRABS	7
#define MOTIF_GRABS	3
OlVirtualName vcommonkeys[COMMON_GRABS] = {
	OL_NEXTAPP,
	OL_PREVAPP,
	OL_NEXTWINDOW,
	OL_PREVWINDOW,
	OL_WINDOWMENU,
	/*OL_WM_SIZE,*/
	OL_WM_BACK,
	OL_WM_QUIT,
	OL_WM_MOVE,
	OL_WM_RESIZE,
};
OlVirtualName volwmkeys[OLWM_GRABS] = {
	OL_WM_OPENCLOSE,
	OL_WM_PROPERTIES,
	OL_WM_REFRESH,
	OL_WM_DISMISSTHIS,
	OL_WM_DISMISSALL,
	OL_WM_OWNER,
	OL_WM_SIZE,
};
OlVirtualName vmotifkeys[MOTIF_GRABS] = {
	OL_WM_RESTORE ,
	OL_WM_MINIMIZE ,
	OL_WM_MAXIMIZE ,
};
for (i=0; i < COMMON_GRABS; i++)
	GRAB(vcommonkeys[i]);
if (currentGUI == OL_MOTIF_GUI)
	for (i=0; i < MOTIF_GRABS; i++)
		GRAB(vmotifkeys[i]);
else
	for (i=0; i < OLWM_GRABS; i++)
		GRAB(volwmkeys[i]);
/*
 * In case you're interested in what the defaults are...

   { XtNnextAppKey,      "a<Escape>",         OL_NEXTAPP,       COREKEY },
   { XtNnextWinKey,      "a<F6>",             OL_NEXTWINDOW,    COREKEY },
   { XtNprevAppKey,      "a s<Escape>",       OL_PREVAPP,       COREKEY },
   { XtNprevWinKey,      "a s<F6>",           OL_PREVWINDOW,    COREKEY },
   { XtNwindowMenuKey,   "s<Escape>",         OL_WINDOWMENU,    COREKEY },
   { XtNwmOpenCloseKey,  "a<F5>",             OL_WM_OPENCLOSE,  COREKEY },
   { XtNwmSizeKey,       "a<F10>",            OL_WM_SIZE,       COREKEY },
   { XtNwmPropertiesKey, "s a<F7>",           OL_WM_PROPERTIES, COREKEY },
   { XtNwmRefreshKey,    "a c<l>",            OL_WM_REFRESH,    COREKEY },
   { XtNwmBackKey,       "a<F3>",             OL_WM_BACK,       COREKEY },
   { XtNwmQuitKey,       "a<F4>",             OL_WM_QUIT,       COREKEY },
   { XtNwmDismissThisKey,"a<F9>",             OL_WM_DISMISSTHIS,COREKEY },
   { XtNwmDismissAllKey, "s a<F9>",           OL_WM_DISMISSALL, COREKEY },
   { XtNwmMoveKey,       "a<F7>",             OL_WM_MOVE,       COREKEY },
   { XtNwmResizeKey,     "a<F8>",             OL_WM_RESIZE,     COREKEY },
   { XtNwmOwnerKey,      "s a<F8>",           OL_WM_OWNER,      COREKEY },
*/
#undef GRAB
} /* end of AddKeyGrabs */

/*
 * Button
 * The function is on the actions list, actionsList[].
 * The event is a buttonpress of some kind.
 */

extern void
Button OLARGLIST((w, event, params, num_params))
OLARG(Widget,     w)
OLARG(XEvent *,   event)
OLARG(String *,   params)      /* unused */
OLGRA(Cardinal *, num_params)  /* unused */
{
WMStepWidget		wm    = (WMStepWidget)w;
WMPiece			piece =
			 EventIn(wm, event-> xbutton.x, event-> xbutton.y);
OlVirtualEventRec	ve;
Display			*display = XtDisplay(wm);
WMStepPart		*wmstep = (WMStepPart *)&(wm->wmstep);
Time first_time, second_time;
Arg     args[2];
XEvent  newevent;
WMPiece p1;

DragMask = event-> xbutton.state | 1<<(event-> xbutton.button+7);

/* 
 * OlLookupInputEvent. Return value is one of
 * OL_[SELECT|ADJUST|MENU|CONSTRAIN|DUPLICATE|PAN] from array of
 * button bindings.
 */
OlLookupInputEvent(w, event, &ve, OL_DEFAULT_IE);
switch(ve.virtual_name)
   {
    case OL_SELECT:
    case OL_CONSTRAIN:
       if (piece == WM_PP)
         SelectPushpin(wm, event, piece);
      else {
	 if (currentGUI == OL_MOTIF_GUI) {
		if (!(FocusSwitchOK(wm))) {
			XBell(XtDisplay((Widget)wm), 0);
			break;
		}
		if (!(wm->wmstep.is_current)) {
		  if (IsIconic(wm)) {
			/* THis code shouldn't get executed because
			 * the icon is separate from the decoration
			 * set now.
			 */
			if (wmrcs->pointerFocus)
			  XSetInputFocus(display, PointerRoot, 
				RevertToPointerRoot, LastEventTime);
			else
			  XSetInputFocus(display,
				RootWindowOfScreen(XtScreen(wm)),
				RevertToNone, LastEventTime);
			SetCurrent(wm);
		  }
		  else /* !iconic */
			if (piece == WM_MINB) {
				/* Don't set focus to window yet - but raise
				 * it...
				 */
				RaiseLowerGroup(wm, WMRaise);
				if (wmrcs->pointerFocus)
			  		XSetInputFocus(display, PointerRoot, 
					  RevertToPointerRoot, LastEventTime);
				else
			  		XSetInputFocus(display,
					  RootWindowOfScreen(XtScreen(wm)),
					  RevertToNone, LastEventTime);
				SetCurrent(wm);
			}
			else
				RaiseSetFocus(wm);
		} /* ! current */
	 } /* currentGUI == OL_MOTIF_GUI */

         if (piece == WM_MM) {
	    /* if power user feature, SelectMenuMark() triggers default menu
	     * item, else map the menu.
	     */
            if (wmrcs->selectDoesPreview) {
		/* This must be Open Look mode because has no
		 * selectDoesPreview; 
		 * Raise window and make current, but don't set focus yet.
		 * If selectDoesPreview, then we'll set focus to the window
		 * on the ButtonUp() - if it's not iconic, that is.
		 * If !selectDoesPreview, then the menu will be posted.
		 * On the way up (in ButtonUp - the only case
		 * where Open Look mode hits ButtonUp) it the window is
		 * no longer iconic, set focus to the window.
		 */
		RaiseLowerGroup(wm, WMRaise);
		if (wmrcs->pointerFocus)
			XSetInputFocus(display, PointerRoot, 
				RevertToPointerRoot, LastEventTime);
		else
			XSetInputFocus(display,
				RootWindowOfScreen(XtScreen(wm)),
				RevertToNone, LastEventTime);
		SetCurrent(wm);
		SelectMenuMark(wm, event, piece);
		break;
	    }
	    else {
		if (currentGUI == OL_OPENLOOK_GUI)
			mmstate = FALSE;
		else
			wm->wmstep.menu_functions &= ~WMMenuButtonState;
		/* inmm tells me that the pointer is inside the menu mark */
		inmm = True;
		FlipMenuMark(wm);
		Menu(wm, event, piece);
		}
	} /* WM_MM */
	else if (piece == WM_MINB) {
		DrawMotifMinimizeButton((Widget)wm, AM_SELECTED);
		wmstep->menu_functions |= WMMinButtonState;
	}
        else if (piece == WM_MAXB) {
		DrawMotifMaximizeButton((Widget)wm, AM_SELECTED);
		wmstep->menu_functions |= WMMaxButtonState;
	}
	else if(!IsIconic(wm)) 
        	Select(wm, event, piece);
	} /* else piece != WM_PP */
      break;
   case OL_ADJUST:
      if (!IsIconic(wm))
         SetFocus((Widget)wm, wm-> wmstep.protocols & HasFocus ?
			LoseFocus : GainFocus,
			wm->wmstep.protocols & HasFocus? 0: 1);
      break;
   case OL_MENU:
      Menu(wm, event, piece);
      break;
   default:
      OlReplayBtnEvent(w, NULL, event);
      break;
   }

} /* end of Button */

/*
 * ButtonUp
 * The function is on the actions list, actionsList[].
 * The event is a Button Release.
 *
 * *******    Motif mode use only   *******************
 * We're looking for a Button Release on the menu, minimize, or
 * maximize buttons (if they exist).  The function redraws the
 * header decoration if it is inverted (selected), redrawing
 * it to normal mode.
 * The window can't be iconic - should we check for this??
 *
 */

extern void
ButtonUp OLARGLIST((w, event, params, num_params))
OLARG(Widget,     w)
OLARG(XEvent *,   event)
OLARG(String *,   params)      /* unused */
OLGRA(Cardinal *, num_params)  /* unused */
{
WMStepWidget		wm    = (WMStepWidget)w;
WMPiece			piece =
			 EventIn(wm, event-> xbutton.x, event-> xbutton.y);
OlVirtualEventRec	ve;
WMStepPart *wmstep = (WMStepPart *)&wm->wmstep;

   if (currentGUI != OL_MOTIF_GUI)
		return;

   DragMask = event-> xbutton.state | 1<<(event-> xbutton.button+7);

   OlLookupInputEvent(w, event, &ve, OL_DEFAULT_IE);
   switch(ve.virtual_name)
   {
    case OL_SELECT:
	if (currentGUI == OL_OPENLOOK_GUI) {
		if (IsIconic(wm))
			break;
		/* This window should be current already - it may not have
		 * focus yet.  The only case where this is a factor is
		 * when the menubutton (WM_MM) is pressed. This call may
		 * be redundant.
		 */
		if (!(wmstep->protocols & HasFocus))
				SetFocus((Widget)wm, GainFocus, 1);
	}
	if (piece == WM_MM && wmstep->menu_functions & WMMenuButtonState) {
		/* Flip will turn off mmstate */

		FlipMenuMark(wm);
		if (wmrcs->selectDoesPreview)
			(*wm-> wmstep.default_cb)(wm, &wm, NULL);
		break;
	}
	else if (piece == WM_MINB && wmstep->menu_functions&WMMinButtonState) {
		/* iconify */
			DrawMotifMinimizeButton(w, AM_NORMAL);
			wmstep->menu_functions &= ~WMMinButtonState;
			/* Iconify.  We just gave focus to the root window;
			 * at the same time we highlighted this window;
			 * when we call MenuOpenClose(), we will be
			 * calling SetFocus() with LoseFocus  to give focus
			 * to another step.
			 */
			MenuOpenClose(w,(XtPointer)&wm,NULL);
		break;
	}
	else if (piece == WM_MAXB && wmstep->menu_functions&WMMaxButtonState) {
		/* Full/Restore */
			DrawMotifMaximizeButton(w, AM_NORMAL);
			wmstep->menu_functions &= ~WMMaxButtonState;
         		MenuFullRestore(w, (XtPointer)&wm, NULL);
		break;
	}
	break;
    default:
	break;
    } /* switch */
	/* turn off all selected states */
	if (wmstep->menu_functions & WMMenuButtonState)
		FlipMenuMark(wm);
	else
		if (wmstep->menu_functions & WMMinButtonState) {
			wmstep->menu_functions &= ~ WMMinButtonState;
			if (!(IsIconic(wm)))
				DrawMotifMinimizeButton(w, AM_NORMAL);
			/* Does this window have focus?? In Button(), we
			 * didn't give the window focus because it could
			 * become an icon - and then we would heve to
			 * take focus away from it right away.  How do
			 * you say, "Protocol Error for client XXX"
			 */
			if (!(wmstep->protocols & HasFocus))
				SetFocus((Widget)wm, GainFocus, 1);
		}
		  else
		if (wmstep->menu_functions & WMMaxButtonState) {
				wmstep->menu_functions &= ~ WMMaxButtonState;
				DrawMotifMaximizeButton(w, AM_NORMAL);
			}
		else
		if (wmstep->menu_functions & WMTitleButtonState) {
				wmstep->menu_functions &= ~ WMTitleButtonState;
				if (!(IsIconic(wm)))
					DrawMotifTitleShadow(w, AM_NORMAL);
			}
} /* ButtonUp */

/*
 * SelectPushpin
 * - Called from Button() - SELECT pressed over the pin
 */

static void
SelectPushpin OLARGLIST((wm, event, piece))
OLARG(WMStepWidget, wm)
OLARG(XEvent *,     event)
OLGRA(WMPiece,      piece)
{
/* ppstate - is pin in or out */
ppstate = wm-> wmstep.decorations & (WMPushpin | WMPinIn);

/* inpp is a static Boolean - tells us the initial state of the
 * pin when entering this function
 */
inpp = wm->wmstep.decorations & (WMPinIn);

/* NOte that FlipPushpin() will reset the global ppstate to the new value */
FlipPushpin(wm);

switch(OlDetermineMouseAction((Widget)wm, event))
   {
   case MOUSE_MOVE:
      HandlePushpin((Widget)wm, event);
      break;
   case MOUSE_MULTI_CLICK:
	if (!inpp) /* pushpin started OUT. We already flipped it once, so filp it
		    * again and call SetPushpinsState with new state - let it
		    * dismiss it!  For now, we'll forget about setting the
		    * intermediate state to IN.
		    */
		FlipPushpin(wm);
	/* otherwise, pushpin started in and we flipped it above.
	 * Set the state to and dismiss.
	 */
      SetPushpinState(wm, ppstate, TRUE);
	break;
   case MOUSE_CLICK:
	/* SetPushpinState() will set the new state of the pushpin in the
	 * decorations to ppstate.
	 */
      SetPushpinState(wm, ppstate, TRUE);
   default:
     break;
   }

} /* end of SelectPushpin */

/*
 * SelectMenuMark
 * Called from Button() when wmrcs->selectDoesPreview
 * (power user) is turned on.  Trigger the menu default.
 */

static void
SelectMenuMark OLARGLIST((wm, event, piece))
OLARG(WMStepWidget, wm)
OLARG(XEvent *,     event)
OLGRA(WMPiece,     piece)
{

if (currentGUI == OL_OPENLOOK_GUI) {
	mmstate = FALSE;
	inmm = True;
}
else
	wm->wmstep.menu_functions &= ~WMMenuButtonState;

/* Go from menu mark "unset" state to "set", or vice-versa */
FlipMenuMark(wm);

if (currentGUI == OL_MOTIF_GUI)
	/* this case will be handled in ButtonUp() - will it be handled
	 * even if the button up event isn't in the window?
	 */
	return;

/* "Dynamic function to determine the type of mouse action.
 * The possible return values are checked; uses mouseDamping and
 * mouseClickTimeout; performs active pointer grab - released if click,
 * but NOT for MOUSE_MOVE; must call ungrab here later if get that one.
 */
switch(OlDetermineMouseAction((Widget)wm, event))
   {
   case MOUSE_MOVE:
	HandleMenuMark((Widget)wm, event);
      break;
   case MOUSE_CLICK:
#ifdef OLDWM
      /* Choose menu callback from combined_menu[] array defined at top of file.
       * wmstep.menu_default is initialized to 0 (first one) in Initialize.
       */
      (*combined_menu[wm-> wmstep.menu_default].cb)(WMCombinedMenu->MenuShell,
							wm, NULL);
#else
      (*wm-> wmstep.default_cb)(wm, &wm, NULL);
#endif
		FlipMenuMark(wm);
      break;
   case MOUSE_MULTI_CLICK:
		FlipMenuMark(wm);
   default:
     break;
   }

} /* end of SelectMenuMark */

/*
 * Select
 * Called from Menu(), Button().
 */

extern void
Select OLARGLIST((wm, event, piece))
OLARG(WMStepWidget, wm)
OLARG(XEvent *, event)
OLGRA(WMPiece, piece)
{
WMAction action = WM_NOP;
Display *display = XtDisplay(wm);
WMStepPart *wmstep = (WMStepPart *) &(wm->wmstep);
Widget w;
WMStepWidget tempwm, owm;
Cardinal n;
int go = 0;
PropMwmHints *mwmp;
        Time first_time, second_time;
        Arg     args[2];
        XEvent newevent;

WMAction act;

if (!(IsIconic(wm))) {
	NewX = wm-> core.x;
	NewY = wm-> core.y;
	w = (Widget)wm;
}
else {
	NewX = wmstep->icon_widget->core.x;
	NewY= wmstep->icon_widget->core.y;
	w = wmstep->icon_widget;
}

switch (piece)
   {
   case WM_SE:
   case WM_SW:
   case WM_NW:
   case WM_NE:
      action = WM_RESIZE;
		/* Raise for icons only - Button() takes care of windows */
/*
	if (currentGUI == OL_MOTIF_GUI && !(IsIconic(wm)) &&FocusSwitchOK(wm))
		RaiseSetFocus(wm);
 */
      ShowResizeFeedback(wm, piece, 1);
      break;
   case WM_T:
   case WM_B:
   case WM_L:
   case WM_R:
	if (currentGUI == OL_MOTIF_GUI) {
		action = WM_RESIZE;
	/* RaiseSetFocus() done in Button() */
/*
		if (FocusSwitchOK(wm))
			RaiseSetFocus(wm);
 */
	}
	else
		action = WM_MOVE;
	break;
   case WM_BANNER:
	if (currentGUI == OL_MOTIF_GUI && FocusSwitchOK(wm)) {
 		if (!(IsIconic(wm)) && wmstep->decorations & WMHeader) {
			DrawMotifTitleShadow((Widget)wm, AM_SELECTED);
			wmstep->menu_functions |= WMTitleButtonState;
		}
	if (IsIconic(wm)) {
#ifdef RAW
		dispatch_expose = False;
		dispatch_widget = w;
#endif
		/* Assume that for motif mode especially, the window has
		 * already been raised in the passive button handler;
		 * on the buttonpress, the activelabel is placed
		 * above the icon; therefore the button release will
		 * be above the active icon label - motifAIW.
		 * Watch below the argument to OlDetermineMouseAction.
		 * (if the button event was really in the image part
		 * as opposed to the label part then you have to pass
		 * the real icon widget, else the motifAIW)
		 */

		if (event->xany.window == XtWindow(motifAIW)) {
			w = motifAIW;
		}
		if (wmrcs->pointerFocus) {
			  XSetInputFocus(display, PointerRoot,
				   RevertToPointerRoot, 
				XtLastTimestampProcessed(display));
		}
		else /* Click-To-Type */ {
		 XSetInputFocus(display, RootWindowOfScreen(
				XtScreen((Widget)wm)), 
				RevertToNone,
				XtLastTimestampProcessed(display));
		}
		RaiseLowerGroup(wm, WMRaise);
		if (!(wm->wmstep.is_current)) {
			SetCurrent(wm);
		}
		CurrentColormapWindow = XtWindow(Frame);
		if (CurrentColormap !=
				DefaultColormapOfScreen(XtScreen(Frame))) {
			CurrentColormap =
				DefaultColormapOfScreen(XtScreen(Frame));
			XInstallColormap(display, CurrentColormap);
		}
	} /* iconic */
	}
	/* Fall through */
   default:
      action = WM_MOVE;
      break;
   }

switch(OlDetermineMouseAction(w, event))
   {
   case MOUSE_MOVE:
      if (action == WM_MOVE) {
		/* If currentGUI == OL_MOTIF_GUI, then you can't just
		 * set focus to the window any time you want - you
		 * have to go through the same rules with MWM_HINTS
		 * (input_mode) before setting focus 
		 */
		/*
		if (currentGUI == OL_MOTIF_GUI && FocusSwitchOK(wm)) {
			RaiseSetFocus(wm);
		}  */ /* currentGUI == OL_MOTIF_GUI && OK */
		DragWindowAround(wm, event, 1);
		if ( (!(IsIconic(wm))) &&
				wmstep->menu_functions&WMTitleButtonState) {
			/* Presumably, WMTitleButtonState will only be set
			 * in MOtif mode.
			 */
			wmstep->menu_functions &= ~WMTitleButtonState;
			DrawMotifTitleShadow((Widget)wm, AM_NORMAL);
		} /* !iconic && WMTitleButtonState */
	} /* WM_MOVE */
      else
         if (action == WM_RESIZE) {
		/*
		if (currentGUI == OL_MOTIF_GUI)
			SetFocus((Widget)wm, GainFocus, 1);
		 */
		ResizeWindow(wm, event, piece);
	  } /* WM_RESIZE */
         else
            FPRINTF((stderr,"nop window\n"));
      break;
   case MOUSE_CLICK:
	first_time = event->xbutton.time;
	/* Before doing any work, check to see if any input mode hints
	 * are set.
	 *
	 * Get current window:
	 */
    if (!wm->wmstep.is_current) {
	for (n = 0; n < num_wmstep_kids; n++) {
		owm = (WMStepWidget)wmstep_kids[n];
		if ( XtClass(owm) == wmstepWidgetClass
		     && (owm-> wmstep.is_current == True) )
			break;
	} /* for */
	if (owm->wmstep.decorations & WMUsesMwmh &&
			   owm->wmstep.mwmh.flags & MWM_HINTS_INPUT_MODE) {
			switch(owm->wmstep.mwmh.inputMode){
				default:
					/* check PRIMARY APP MODAL */
					go = 2; break;
				case MWM_INPUT_SYSTEM_MODAL:
					/* Focus can't go anywhere */
					break;
				case MWM_INPUT_FULL_APPLICATION_MODAL:
					/* Focus can't go to any other wins
					 * in the app that is FULL APP MODAL
					 */
					go = 3;
					break;
				/*
					ConstructGroupList(owm,
						NestedNoAppendAny, True);
				
					for (n=0; n < group_list->used; n++)
					{
					tempwm = (WMStepWidget)group_list->p[n];
					if (tempwm == wm)
						break;
					}
					if (n >= group_list->used)
						go++;
					break;
				 */
			} /* switch */
	} /* if */
	else
		go++;
	if (go && !(IsIconic(wm))) {
		/* Now we have to check the application that is getting focus.
	 	 * Is the window that wants focus in some sort of modality
	 	 * binge by the application?
	 	 */
		/* FIrst: is any window in this application FULL MODAL?
		 * If so, then only that window can get focus, and unless
		 * it is this one, then we must beep and set go to 0.
		 */
		ConstructGroupList(wm, NestedNoAppendAny, True);
		for (n=0; n < group_list->used; n++) {
			tempwm = (WMStepWidget)group_list->p[n];
			if (!(tempwm->wmstep.decorations & WMUsesMwmh))
				continue;
			mwmp = (PropMwmHints *)&(tempwm->wmstep.mwmh);
			if (mwmp->flags & MWM_HINTS_INPUT_MODE &&
			   mwmp->inputMode == MWM_INPUT_FULL_APPLICATION_MODAL)
				break;
		}
		if (n >= group_list->used) {
			/* nothing bad above; how about PRIMARY modal? */
		/*
		 * Second, is any window PRIMARY application modal that
		 * is a child of this window, or for that matter, this
		 * window too?  If so, then this window can't get focus.
		 */
			ConstructGroupList(wm, NoAppend, True);
			/* Start with n=1, because this window isn't
			 * an ancestor of itself for this purpose,
			 * so it's O.K. for it to get focus.
			 */
			for (n=1; n < group_list->used; n++) {
				tempwm = (WMStepWidget)group_list->p[n];
				if (!(tempwm->wmstep.decorations & WMUsesMwmh))
					continue;
				mwmp = (PropMwmHints *)&(tempwm->wmstep.mwmh);
				if (mwmp->flags & MWM_HINTS_INPUT_MODE &&
				 mwmp->inputMode == 
					  MWM_INPUT_PRIMARY_APPLICATION_MODAL)
					break;
			}
			if (n < group_list->used)
				go = 0;
		}
		else
			go = 0;
	} /* if go */
	/* If go hasn't been set, then don't reset focus or raise the
	 * window/group.
	 */
	if (!go) {
		XBell(XtDisplay((Widget)wm), 0);
		break;
	}
    } /* if not selecting the window already current */
	if (currentGUI == OL_MOTIF_GUI && !(IsIconic(wm)) &&
			wmstep->decorations & WMHeader) {
		DrawMotifTitleShadow((Widget)wm, AM_SELECTED);
		wmstep->menu_functions |= WMTitleButtonState;
	}
	if (IsIconic(wm)) {
	    if (currentGUI == OL_OPENLOOK_GUI) {
			RaiseSetFocus(wm);
			CurrentColormapWindow = XtWindow(Frame);
			if (CurrentColormap !=
				   DefaultColormapOfScreen(XtScreen(Frame))){
				CurrentColormap =
				   DefaultColormapOfScreen(XtScreen(Frame));
				XInstallColormap(display, CurrentColormap);
			}
		}
		/* Mouse click in motif mode: pop up the window menu */
	    if (currentGUI == OL_MOTIF_GUI) {
		if (motwmrcs->iconClick) {
			/* This was a button CLICK, not a press.
			 * We must use the OL_MENUKEY event for
			 * this, rather than a button press, or
			 * the menu will not get focus.
			 */

			event->type = ButtonRelease;
			Menu(wm, event, WM_BANNER);
		} /* end of if (motwmrcs->iconClick)  */
	    }
	}
	else {
		RaiseLowerGroup(wm, WMRaise);
		if (currentGUI == OL_MOTIF_GUI) {
		  if (wmstep->menu_functions & WMTitleButtonState) {
			wmstep->menu_functions &= ~WMTitleButtonState;
			DrawMotifTitleShadow((Widget)wm, AM_NORMAL);
		  } /* WMTitleButtonState */
		} /* MOTIF */
		else {
		  switch(piece) {
			default:
				break;
			case WM_NW:
			case WM_SE:
			case WM_SW:
			case WM_NE:
				ShowResizeFeedback(wm, piece, 0);
				break;
		  } /* switch */
		} /* else Open Look */

	/* I'm not event sure youi want to do this in Open Look, but leave it
	 * at that for now.
	 */
	if (currentGUI == OL_OPENLOOK_GUI)
		SetFocus((Widget)wm, GainFocus, 1);
	} /* else */
      break;
   case MOUSE_MULTI_CLICK:
      if (currentGUI == OL_OPENLOOK_GUI && HasFullMenu(wm) ||
	  (currentGUI == OL_MOTIF_GUI && IsIconic(wm) ) ) {
	if (IsIconic(wm) && !motwmrcs->iconClick)
		MenuOpenClose((Widget)wm,(XtPointer)&wm,NULL);
	else
	   if (currentGUI == OL_OPENLOOK_GUI)
         MenuFullRestore((Widget)wm, (XtPointer)&wm, NULL);
	} /* HasFullMenu */
	if ((!IsIconic(wm))) {
		if (wmstep->menu_functions & WMTitleButtonState) {
			wmstep->menu_functions &= ~WMTitleButtonState;
			DrawMotifTitleShadow((Widget)wm, AM_NORMAL);
		}
	} /* !iconic */
      break;
   } /* switch */

} /* end of Select */

/*
 * HandlePushpin
 *
 */

static void
HandlePushpin OLARGLIST((w, event))
OLARG(Widget, w)
OLGRA(XEvent *, event)
{
WMStepWidget wm = (WMStepWidget)w;

OlGrabDragPointer(w, OlGetStandardCursor(w), XtWindow(XtParent(w)));
OlAddTimeOut(w, (unsigned long)0, PollHandlePushpin, (XtPointer)w);

} /* end of HandlePushpin */
/*
 * PollHandlePushpin
 *
 */

static void
PollHandlePushpin OLARGLIST((client_data, id))
OLARG(XtPointer, client_data)
OLGRA(XtIntervalId *, id)
{
Widget w  = (Widget)client_data;
WMStepWidget wm = (WMStepWidget)client_data;

Window            root;
Window            child;
int               rootx, rooty, winx, winy;
unsigned int      mask;

XQueryPointer(XtDisplay(w), XtWindow(w),
              &root, &child, &rootx, &rooty, &winx, &winy, &mask);

if (mask != DragMask)
   {
   OlUngrabDragPointer(w);
   SetPushpinState(wm, ppstate, TRUE);
   }
else
   {
   if (EventIn(wm, winx, winy) == WM_PP)
      {
      if (!inpp)
         FlipPushpin((WMStepWidget)w);
      inpp = True;
      }
   else
      {
      if (inpp)
         FlipPushpin((WMStepWidget)w);
      inpp = False;
      }

	OlAddTimeOut(w, (unsigned long)0, PollHandlePushpin, (XtPointer)w);
   }

} /* end of PollHandlePushpin */

/*
 * HandleMenuMark
 * Called from SelectMenuMark() when SELECT is pressed over the menu mark, AND
 * the mouse is moved before releasing.  Grabs the mouse pointer and calls
 * PollHandleMenuMark().
 * - NOTE - XtAddTimeOut() has been replaced in the Intrinsics by
 *  XtAppAddTimeOut().
 */

static void
HandleMenuMark OLARGLIST((w, event))
OLARG(Widget, w)
OLGRA(XEvent *, event)
{
WMStepWidget wm = (WMStepWidget)w;

/* Actively grab the mouse pointer */
OlGrabDragPointer(w, OlGetStandardCursor(w), XtWindow(XtParent(w)));

/* Has the effect of calling procedure PollHandleMenuMark() after 0 seconds */
OlAddTimeOut(w, (unsigned long)0, PollHandleMenuMark, (XtPointer)w);

} /* end of HandleMenuMark */

/*
 * PollHandleMenuMark
 * Called from HandleMenuMark(); enter with the following info:
 *   mouse button was pressed over menu button, and the pointer is
 *   now actively grabbed.
 * Being that you got here by pressing SELECT over the window menu button,
 * it continuously checks the state of
 * the mouse (and keybd.) for a change (ButtonRelease).
 * If it gets the ButtonRelease over the menu button, then call the menu
 * default callback - note that this may be WRONG - possibly, it should be
 * checking the wmrcs->selectDoesPreview to see if you should post the menu!!
 * Until it gets the release or some change in state, then it assumes that
 * the mouse may move in and out of the menu button, so it toggle the 
 * button's drawing  with FlipMenuMark().
 */

static void
PollHandleMenuMark OLARGLIST((client_data, id))
OLARG(XtPointer, client_data)
OLGRA(XtIntervalId *, id)
{
Widget       w  = (Widget)client_data;
WMStepWidget wm = (WMStepWidget)client_data;

Window       root;
Window       child;
int          rootx, rooty, winx, winy;
unsigned int mask;

/* return: child = window that the ptr. is in;
 *	   winx, winy: position relative to child;
 *	   rootx, rooty: position relative to root.
 *	   mask: CURRENT STATE of modifier keys and pointer buttons.
 */
XQueryPointer(XtDisplay(w), XtWindow(w),
              &root, &child, &rootx, &rooty, &winx, &winy, &mask);

if (mask != DragMask)
   {
   /* Some change in state of buttons and modifiers occurred - ungrab the
    * pointer.
    */
   OlUngrabDragPointer(w);

   /* mmstate - tells us if menu button was selected.  It
    * is toggled in FlipMenuMark.  If non-zero -> menu button was just
    * selected.
    */
   if (mmstate)
      {
#ifdef OLDWM
      (*combined_menu[wm-> wmstep.menu_default].cb)(w, wm, NULL);
#else
          (*wm-> wmstep.default_cb)(w, &wm, NULL);
#endif
      FlipMenuMark(wm);
      }
   }
else
   {
   /* mask == DragMask, no change in state of mouse or keybd. buttons */
   if (EventIn(wm, winx, winy) == WM_MM)
      {
      if (!inmm)
         FlipMenuMark(wm);
      inmm = True;
      }
   else
      {
      if (inmm)
         FlipMenuMark(wm);
      inmm = False;
      }

	OlAddTimeOut(w, (unsigned long)0, PollHandleMenuMark, (XtPointer)w);
   }

} /* end of PollHandleMenuMark */

/*
 * ReadProtocols
 * Read WM_PROTOCOLS property from window w; called from Initialize()
 * and ClientPropertyChange().  Currently there are 3 atoms in this property:
 *  - WM_TAKE_FOCUS
 *  - WM_SAVE_YOURSELF
 *  - WM_DELETE_WINDOW
 * Pass the wmstep widget( parent)  and the child widget (handle widget) whose
 * window  must be read for this property.  Adjust the parent->wmstep.protocols
 * field with the flags of the atoms that are present, set the input focus
 * model for this client that owns the window.
 */

extern void
ReadProtocols OLARGLIST((parent, w, window))
OLARG(WMStepWidget, parent)
OLARG(Widget, w)
OLGRA(Window, window)
{
int n;
Atom * atoms = (Atom *) 0;
Display *dpy = XtDisplay(w);
Atom a1, actual_type;
int actual_format;
unsigned long bytes_after;

parent-> wmstep.protocols = 0L;

a1 = XInternAtom(dpy, "WM_PROTOCOLS", False);
        /* XGetWindowProperty() is used instead of XGetWMProtocols(), in                   order to achieve some backward compatibility.  Old versions
           of Motif toolkit apparently were using "WM_PROTOCOLS" to be 
           the type of window manager protocol hints, which is not in
           compliance with ICCCM */
if (XGetWindowProperty(dpy, window, a1, 0, 2500,
                        False, AnyPropertyType, &actual_type,
                        &actual_format, (unsigned long *) &n, &bytes_after,                             (unsigned char **) &atoms) == 0) {
        if (actual_type != a1 && actual_type != XA_ATOM && atoms != (Atom *) 0) {
                        XFree((char *)atoms);
                        atoms = (Atom *) 0;
        }
}
else if (atoms != (Atom *) 0) {
                XFree((char *)atoms);
                atoms = (Atom *) 0;
        }

if (atoms != NULL)
   {
   while (n--)
      {
      if (atoms[n] == XA_WM_TAKE_FOCUS(dpy))
         parent-> wmstep.protocols |= TakeFocus;
      else
         if (atoms[n] == XA_WM_SAVE_YOURSELF(dpy))
            parent-> wmstep.protocols |= SaveYourself;
         else
            if (atoms[n] == XA_WM_DELETE_WINDOW(dpy))
               parent-> wmstep.protocols |= DeleteWindow;
      }
   XFree((char *)atoms);
   atoms = (Atom *) 0;
   }

if (parent-> wmstep.protocols & TakeFocus)
   {
   if ((parent-> wmstep.xwmhints-> flags & InputHint) &&
        parent-> wmstep.xwmhints-> input)
      parent-> wmstep.protocols |= LocallyActive;
   else
      parent-> wmstep.protocols |= GloballyActive;
   }
else
   {
   if ((parent-> wmstep.xwmhints-> flags & InputHint) &&
        parent-> wmstep.xwmhints-> input)
      parent-> wmstep.protocols |= Passive;
   else
      parent-> wmstep.protocols |= NoInput;
   }

} /* end of ReadProtocols */

/*
 * NonMaskable
 * Handle non-maskable events on root window. (e.g. ClientMessage)
 *
 */

static void
NonMaskable OLARGLIST((w, client_data, event, bool))
OLARG(Widget,	w)
OLARG(XtPointer, client_data)
OLARG(XEvent *,	event)
OLGRA(Boolean *, bool)
{
int    i;
Display *dpy = event->xany.display;

switch (event-> type)
   {
   case ClientMessage:
      if (event-> xclient.message_type == XA_BANG(dpy))
	 /* request to die from olwsm */
         {
         if (help_parent)
            XtUnmapWidget(help_parent);
         for (i = 0; i < window_list->used; i++)
            {
            WMStepWidget wm = (WMStepWidget)window_list->p[i];
            FPRINTF((stderr, "sending quit for %s\n", wm-> wmstep.window_name));
#ifdef WITHDRAWNSTATE
	    if (wm->wmstep.size == WMWITHDRAWN)
		continue;
#endif
            if (wm-> wmstep.xwmhints-> window_group == wm-> wmstep.window)
               MenuQuit((Widget)wm, (XtPointer)&wm, NULL);
	       /* MenuQuit() handles the ICCCM protocols (sends WM_DELETE_WINDOW
		* message, SAVE_YOURSELF, etc.  */
/*
 * wait for detroy of widget???
 */
            }
	 /* I'd like to wait until all StepChildren are destroyed, but
	  * what if someone doesn't want to exit??  It could be a long
	  * wait, in other words, until num_wmstep_kids == 0.
	  *  Furthermore, in order to do this, we'd have to dispatch all
	  * the events that come in that come under the category of
	  * DestroyNotify or UnmapNotify;  Basically, the system could
	  * freeze up here; maybe, although it could get sloppy, we
	  * should do a check in the clientStructureNotify event
	  * handler, or possibly in Destroy() where it decrements
	  * the num_wmstep_children variable, as to whether a global
	  * variable is set to indicate that we got the BANG message
	  * from olwsm; if it is, and after calling AddDeleteWMStep(),
	  * we get down to num_wmstep_kids = 0, then we should
	  * enqueue WSM_EXIT.
	  */
	 /* Send final message to olwsm - we're done */
	if (ReadyToDie())
         EnqueueWSMRequest(event-> xclient.display, event-> xclient.window, 
            WSM_EXIT, &wsmr);
	else
		start_or_terminate = TERMINATE_FLAG;
		/*we_are_being_destroyed++ ;*/
         }
      else
         if (event-> xclient.message_type == XA_WM_CHANGE_STATE(dpy))
            {
            if (event-> xclient.data.l[0] == IconicState)
               {
               WMStepWidget wm;
               for (i = 0; i < window_list->used; i++)
                  {
                  wm = (WMStepWidget)window_list->p[i];
                  if (wm-> wmstep.window == event-> xclient.window)
                     break;
                  }
               if (i == window_list->used)
                  FPRINTF((stderr, "unknown window for IconicState\n"));
               else
                  if (!IsIconic(wm))
                     MenuOpenClose((Widget)wm, (XtPointer)wm, NULL);
               }
            }
         else
            FPRINTF((stderr, "got an unknown client message\n"));
      break;
   default:
      FPRINTF((stderr, "got a non-maskable event\n"));
      break;
   }

} /* end of NonMaskable */

extern Boolean
ReadyToDie()
{
int i;
WMStepWidget wm;

	if ( (num_wmstep_kids == 0) || ((num_wmstep_kids == 1) &&
		((WMStepWidget)wmstep_kids[0] == help_parent)) )
	return(True);
	for (i=0; i < num_wmstep_kids; i++) {
		wm = (WMStepWidget)wmstep_kids[i];
		if (wm == help_parent)
			continue;
#ifdef WITHDRAWNSTATE
		if (wm->wmstep.size != WMWITHDRAWN)
#endif
			return(False);
	}
	return(True);
} /* ReadyToDie */		 

/*
 * SubstructureRedirect
 * Basic event handler on the root window to catch redirected events, such as
 * Map requests.  It all starts here!  Handler added by StartWindowManager().
 * Does not handle CirculateRequest events and ResizeRequest events.
 */

static void
SubstructureRedirect OLARGLIST((w, client_data, event, bool_ret))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLARG(XEvent *, event)
OLGRA(Boolean *, bool_ret)
{
XWindowChanges wc;
switch (event-> type)
   {
   case CirculateRequest:
      FPRINTF((stderr, "CirculateRequest: event\n"));     break;
   case ConfigureRequest:
   if (XtWindowToWidget(XtDisplay(w), event->xconfigure.window) ==
                                                        (Widget)NULL) {
      wc.x            =   event-> xconfigurerequest.x;
      wc.y            =   event-> xconfigurerequest.y;
      wc.width        =   event-> xconfigurerequest.width;
      wc.height       =   event-> xconfigurerequest.height;
      wc.border_width =   event-> xconfigurerequest.border_width;
      wc.sibling      =   event-> xconfigurerequest.above;
      wc.stack_mode   =   event-> xconfigurerequest.detail;
      XConfigureWindow(event-> xconfigurerequest.display,
         event-> xconfigurerequest.window,
         event-> xconfigurerequest.value_mask, &wc);
    }
   else {
        int k;
        Boolean cont_to_disp = False;
        /* Could be the timing:  we may have received a configure request from
         * a client that was in the process of being reparented by us, but
         * hadn't been given the proper event handler yet
         * (SubstructureRedirect).  If this else fails , then this event goes
         * nowhere.
         */
        if ( (k = IsWMStepChild(event->xconfigure.window)) != -1) {
                WMStepWidget wm = (WMStepWidget)(wmstep_kids[k]);
                ClientSubstructureRedirect((Widget)wm, (XtPointer)wm, event,
                                                &cont_to_disp);
        }
   }
      FPRINTF((stderr, "ConfigureRequest: event\n"));     break;
   case MapRequest:
      /* If window not already widgetized, Reparent() gives it a WMStepWidget */
      if (XtWindowToWidget(XtDisplay(w), event-> xmaprequest.window) == NULL)
         (void) Reparent(event-> xmaprequest.window, NULL);
      else
         FPRINTF((stderr, "ignore request for %d\n", event-> xmaprequest.window));
      FPRINTF((stderr, "MapRequest: event\n"));           break;
   case ResizeRequest:
      FPRINTF((stderr, "ResizeRequest: event\n"));        break;
   default:
      FPRINTF((stderr, "unknown type of structure request event\n"));
   }

} /* end of SubstructureRedirect */

/*
 * DestroyParent() Callback; also called by ClientStructureNotify on a client's
 * XUnmapWindow() request, with args w = client_data == WMStepWidget
 * (parent of client widget).
 * Remove focus, reparent window and icon window (if any) to root,
 * feed WMStepWidget to  XtDestroyWidget().
 */

extern void
DestroyParent OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm      = (WMStepWidget)client_data;
Display *    display = XtDisplay(wm);
Window       window  = XtWindow(wm);
WMStepPart * wmstep  = &wm-> wmstep;
Screen		*screen = XtScreen(wm);
Window	root_win = RootWindowOfScreen(screen);
XEvent ev;
Boolean cont_disp;

/* Give up focus if Click-to-type model; SetFocus() will try and give it to
 * someone else.  If it does give it someone else, make that someone else
 * the "current" window.
 */
/*
if (!wmrcs->pointerFocus)
   SetFocus((Widget)wm, LoseFocus, 1);
 */

/* Unmap the WMStepWidget window */
/*
XUnmapWindow(display, window);
 */

/* Set state on the client window (wm->wmstep.window) */
/*SetWindowState(wm, WithdrawnState);*/

/* What does the WMIconWindowReparented flag mean?  Here they are asking if the
 * flag is NOT turned on, and if NOT, unmap the icon window provided in the
 * window mgr. hints, and reparent it to root.
 */
/*
if (!(wmstep-> decorations & WMIconWindowReparented))
*/
if (wmstep-> decorations & WMIconWindowReparented && 
			wmstep->xwmhints->icon_window)
   {
   Window window = wmstep-> xwmhints-> icon_window;

   XUnmapWindow(display, window);
   XReparentWindow(display, window, 
      root_win, wm-> core.x, wm-> core.y);
   XChangeSaveSet(display, window, SetModeDelete);
   }

/* Finally, reparent the client window to root, destroy WMStepWidget, etc.
 * Before reparenting to the root window: check if there is a pending
 * ReparentNotify event on the queue FOR THIS CLIENT WINDOW - if there
 * is, then it means that the client (or someone) reparented it to another
 * window, and we shouldn't reparent it to root.
 * One more - ConfigureRequest events.
 */
	/* Remove this event handler */
/*
	XtRemoveEventHandler((Widget)wm, SubstructureRedirectMask, False,
				ClientSubstructureRedirect, NULL);
 */
	XSync(display,False);
	while (XCheckTypedWindowEvent(display, wmstep->window,
			ConfigureRequest, &ev) != 0)
		ClientSubstructureRedirect((Widget)wm, (XtPointer)NULL,
				&ev,&cont_disp);
	if (XCheckTypedWindowEvent(display, wmstep->window, ReparentNotify,
								&ev) == 0)
		XReparentWindow(display, wmstep-> window, 
   			root_win, wm-> core.x, wm-> core.y);

XChangeSaveSet(display, wmstep->window, SetModeDelete);
XSetWindowBorderWidth(display, wmstep->window, wmstep-> prev.border_width);
#if defined(PRINT_KIDS)
fprintf(stderr,"number of kids left BEFORE destroy= %d\n",num_wmstep_kids);
#endif
XtDestroyWidget((Widget)wm);
/* We can send the  WSM_EXIT here assuming that Destroy() gets called and
 * num_wmstep_kids is decremented properly.
 */
#if defined(PRINT_KIDS)
fprintf(stderr,"number of kids left after destroy= %d\n",num_wmstep_kids);
#endif
} /* end of DestroyParent */

/*
 * Reparent
 * Called from ReparentWindows() from StartWindowManager() - each
 * window already mapped is sent here along with its XWindowAttributes.
 * Use the window and it's attributes (and f.g. and b.g. color) to
 * create a wmstep widget instance.
 * Also called from WMReparent callback with NULL wattr arg.
 *
 * The wmstepClassRec tells us that there is an Initialize() procedure
 * (there is a class_initialize proc, ClassInitialize(),
 * that does nothing, and a NULL class_part_initialize().  The Initialize()
 * function does most of the initializing of fields and reading of essential
 * properties.
 */

static Widget
Reparent OLARGLIST((w, wattr))
OLARG(Window, w)
OLGRA(XWindowAttributes *, wattr)
{
Arg arg[9];
Widget frame;
WMStepWidget wm, owm;
WMStepPart *wmstep;
int x, y;
unsigned int width, height, bw, depth;
Window root;
int go = 0, n;
unsigned long window_state;

	XtSetArg(arg[0], XtNwindow, w);
	XtSetArg(arg[1], XtNforeground, wmrcs->foregroundColor);
	XtSetArg(arg[2], XtNbackground, wmrcs->backgroundColor);
	XtSetArg(arg[3], XtNwindowAttributes, wattr);
	frame = XtCreateWidget("step", wmstepWidgetClass, Frame, arg, 4);
	if (XtIsRealized(frame) == FALSE) {
		XtRealizeWidget(frame);
	}
	wm = (WMStepWidget)frame;

/* Check to see if the window is still around - if it isn't, I don't
 * want to leave an untitled decoration set around for good.
 */
if (XGetGeometry(XtDisplay(wm), w, &root,
            &x, &y, &width, &height, &bw, &depth) == 0) {
	/* problems, abort */
	DestroyParent((Widget)wm, (XtPointer)wm, NULL);
	return((Widget)NULL);
}


StartWindowMappingProcess(wm);
return(frame);

} /* end of Reparent */

extern void
StartWindowMappingProcess OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
WMStepPart	*wmstep = (WMStepPart *)&(wm->wmstep);
int		go, n;
WMStepWidget	owm,
		tempwm;
unsigned long	window_state;
int		CurrentFocusWidget = -1;

	SendWindowMovedEvent(wm);

	window_state = GetWindowState(XtDisplay(wm), wm->wmstep.window);

	/* Notices always map */
	if (wm->wmstep.decorations & WMUsesOlwa &&
			(wmstep->olwa.win_type == XA_OL_WT_NOTICE(XtDisplay(wm))) ) {
		XtMapWidget((Widget)wm);
		/* Redundant if window is first brought up after creation,
		 * but necessary if window is brought up from Withdrawn state
		 */
		XtMapWidget(wm->wmstep.child);
	}
	else
	  if  (window_state == NormalState) {
		Cardinal k;
		int mapme = 1;
		Window leader;

		if (wm->wmstep.window != wm->wmstep.xwmhints->window_group) {
			if ( (leader = find_highest_group(wm, (Window)NULL)) !=
								NULL) {
				k = find_leader(leader);
				if (k < num_wmstep_kids) {
					WMStepWidget tempwm =
						(WMStepWidget) (wmstep_kids[k]);
					if (IsIconic(tempwm) &&
						   tempwm->wmstep.icon_widget) {
 						XtUnmapWidget(wm-> wmstep.child);
						mapme = 0;
						SetWindowState(wm, IconicState,
							tempwm->wmstep.window);
						wm->wmstep.protocols |= IgnoreUnmap;
						wm->wmstep.size    = WMICONICNORMAL;
					}
				} /* k >= 0 */
			}
		}
		if (mapme) {
			XtMapWidget((Widget)wm);
			XtMapWidget(wm->wmstep.child);
		}
	}
	else
	  if (window_state == IconicState) {
 		XtUnmapWidget(wm->wmstep.child);
		/* Only map the icon widget if it exists - true if
	 	 * it's the group leader (head transient parent)
	 	 * Otherwise, don't turn on WMIconWindowMapped 
	 	 * because it doesn't have an icon - but it is in
	 	 * the group.
	 	 */
		if (wm->wmstep.icon_widget) {
			Cardinal k;
			Cardinal n;
			int unmapme = 0;
			WMStepWidget tempwm;
			WMStepPart *twmstep;
			Window leader;

			XtMapWidget(wm->wmstep.icon_widget);
			wm->wmstep.decorations |= WMIconWindowMapped;

			/* This is a Merry-Go-Round check in a reverse order
			 * of what we did above (no relation to the specialty
			 * retailer).  Look at all mapped step widgets in
			 * Normal state, and unmap and set the state to
			 * iconic the ones in the same window group as this
			 * window.
			 */
			for (k=0; k < num_wmstep_kids; k++) {
				tempwm = (WMStepWidget)(wmstep_kids[k]);
				/* Continue if this index is for this window
				 * or if the index has a step widget already
				 * iconic - would be redundant to check.	
				 */
				if (tempwm == wm || IsIconic(tempwm))
					continue;
				twmstep  = (WMStepPart *)&(tempwm->wmstep);

				if(twmstep->transient_parent ==
						wm->wmstep.window ||
					twmstep->xwmhints->window_group ==
						wm->wmstep.window ||
					find_highest_group(tempwm, (Window)NULL) ==
							wm->wmstep.window) {
					if (!IsIconic(tempwm)) {
 					  XtUnmapWidget(twmstep->child);
						SetWindowState(tempwm, IconicState,
							wm->wmstep.window);
					  twmstep->protocols |= IgnoreUnmap;
					  twmstep->size    = WMICONICNORMAL;
					}
				}
			}
		} /* if wm->wmstep.icon_widget */
	} /* iconic state */

	if (start_or_terminate != START_FLAG && !NumMenushellsPosted) {

		/* Find out who has focus now;
		 *  - If the window that has focus is modal, then this window
		 * may get focus if it's NOT part of the application that has
		 * focus;
		 *
		 * - If the application that has focus is application modal,
		 * then this window may get focus if it's in the application;
		 * else it doesn't get focus.  If an application modal window
		 * gets focus, grab pointer and restrict focus to this
		 * application. (We have to worry about keyboard ops later).
		 * When ptr is grabbed, what should be done with button
		 * presses outside an application?  Send them to the app.
		 * with XSendEvent()??
		 *
		 * -If the window that has focus is system modal, then
		 * this window doesn't get focus regardless of anything.
		 * If a system modal window gets focus, grab pointer,
		 * and restrict focus to this window only.
		 * then grab pointer, and restrict focus to
		 *
		 * Regardless of anything, if a window menu is posted, don't
		 * set focus to anything else
		 *
		 */
	go = 0;
	for (n = 0; n < num_wmstep_kids; n++) {
		owm = (WMStepWidget)wmstep_kids[n];
		if ( XtClass(owm) == wmstepWidgetClass
		     && (owm-> wmstep.is_current == True) )
			break;
	} /* for */

	if (n >= num_wmstep_kids)
		go++; /* set focus to wm  */
	else
		CurrentFocusWidget = n;
	if (!go) { /* keep working - someone has focus */
		if (owm->wmstep.decorations & WMUsesMwmh &&
			   owm->wmstep.mwmh.flags & MWM_HINTS_INPUT_MODE) {
			WMStepWidget tempwm;
			switch(owm->wmstep.mwmh.inputMode) {
				case MWM_INPUT_MODELESS:
					/* Allow focus to be set */
					go++; break;
				case MWM_INPUT_SYSTEM_MODAL:
					/* do nothing, let it ride */
					break;
				case MWM_INPUT_FULL_APPLICATION_MODAL:
					/*
					 * "Input does not go to other
					 * windows in this application".
					 * If this window isn't in the
					 * current focus app, set focus
					 * to it.  We have the window that
					 * has focus.  Get all windows
					 * in the application.  Is this
					 * one of them?
					 * Call ConstructGroupList with
					 * NestedNoAppendAny, because we
					 * want the window group at the
					 * very top of the hierarchy.
					 */
					ConstructGroupList(owm,
						NestedNoAppendAny, True);
					/* Is the window in question, wm,
					 * in the list?
					 */
				
					for (n=0; n < group_list->used; n++)
					{
					tempwm = (WMStepWidget)group_list->p[n];
					if (tempwm == wm)
						break;
					}
					if (n >= group_list->used)
						go++;
					break;
				case MWM_INPUT_PRIMARY_APPLICATION_MODAL:
					/* focus can't go to any of the
					 * windows ancestors.  That means
					 * children (windows that have this
					 * the current focus window as
					 * transient)are O.K. to get focus.
					 * Windows not in the application can
					 * get focus.
					 * If wm is not an ancestor, or if
					 * it is a child (transient) window,
					 * or a window not in the application,
					 * then O.K.  Construct a group list
					 * with append == NoAppend.  This uses
					 * the owm window ID as the window
					 * group.
					 */
					ConstructGroupList(owm,
						NoAppend, True);
					/* We just found all windows in the
					 * group that owm is the leader of.
					 * Any of these can get focus,
					 * including wm, if it wants it.
					 * But we still need another
					 * exhaustive list of windows in the
					 * supergroup - if the window is
					 * in the supergroup - the same
					 * application, then it can't get focus.
					 */
					for (n=0; n < group_list->used; n++)
					{
					tempwm = (WMStepWidget)group_list->p[n];
					if (tempwm == wm)
					 {	go++;
						break;
					 }
					}
					if (n >= group_list->used)
						break;

					/* Now the exhaustive list */
					for (n=0; n < group_list->used; n++)
					{
					tempwm = (WMStepWidget)group_list->p[n];
					if (tempwm == wm)
						break;
					}
					if (n >= group_list->used)
						/* Not in supergroup list,
						 * give it focus.
						 */
						go++;
					break;
					/* This doesn't take care of some cases-
					 * what if focus leaves a group, then
					 * comes back?  Or for this above case,
					 * if the focus does go to a transient,
					 * then the transient may not have
					 * a modality property - we may have to
					 * go back to the transient parent just
					 * to see where the NEXT focus item may
					 * be allowed.
					 */
				} /* switch */
			} /* INPUT_MODE */
			else	/* No specification => no restrictions on
				 * input mode
				 */
				go++;
		} /* !go */

		if (go && !(wmrcs->pointerFocus)) {
			if (!(IsIconic(wm)))
				SetFocus((Widget)wm, GainFocus, 1);
			else /* iconic */
			if (wm->wmstep.decorations & WMIconWindowMapped &&
						CurrentFocusWidget == -1) {
				SetFocus(Frame, GainFocus, 0);
				SetCurrent(wm);
			}
		} /* go */
	} /* start_or_terminate != START_FLAG */
} /* StartWindowMappingProcess */


/*
 * WMError
 *
 */

static void
WMError OLARGLIST((error_number, p))
OLARG(int, error_number)
OLGRA(XtPointer,  p)
{

(void) fprintf(stderr, "WMError: %d '%x'\n", error_number, p);

} /* end of WMError */

/*
 * SendLongMessage
 *
 */

extern void
SendLongMessage OLARGLIST((display, window, type, l0, l1, l2, l3, l4))
OLARG(Display *,display)
OLARG(Window, window)
OLARG(Atom,type)
OLARG(unsigned long, l0)
OLARG(unsigned long, l1)
OLARG(unsigned long, l2)
OLARG(unsigned long, l3)
OLGRA(unsigned long, l4)
{
XEvent  sev;

sev.xclient.type         = ClientMessage;
sev.xclient.display      = display;
sev.xclient.window       = window;
sev.xclient.message_type = type;
sev.xclient.format       = 32;
sev.xclient.data.l[0]    = l0;
sev.xclient.data.l[1]    = l1;
sev.xclient.data.l[2]    = l2;
sev.xclient.data.l[3]    = l3;
sev.xclient.data.l[4]    = l4;

XSendEvent(display, window, False, NoEventMask, &sev);

} /* end of SendLongMessage */

/*
 * SendConfigureNotify
 * mlp - called from ClientSubstructureRedirect() event handler whenever
 * a configure request asks for a new position only (e.g., not change in
 * width or height).  Appears to call it with NEW x,y position of overall
 * frame, as opposed to client window.
 *   Send a synthetic event to the client.
 */

extern void
SendConfigureNotify OLARGLIST((display, window, x, y, width, height,
							border_width))
OLARG(Display *, display)
OLARG(Window,    window)
OLARG(int,       x)
OLARG(int,       y)
OLARG(int,       width)
OLARG(int,       height)
OLGRA(int,       border_width)
{
XConfigureEvent event;

event.type = ConfigureNotify;
event.display           = display;
event.event             =
event.window            = window;
event.x                 = x;
event.y                 = y;

event.width             = width;
event.height            = height;
event.send_event        = True;
event.border_width      = 0;

event.border_width      = border_width; 

event.override_redirect = False;
event.above             = None;

FPRINTF((stderr, "sending configure notify %d,%d %dx%d (%d)\n",
   event.x, event.y, event.width, event.height, event.border_width));

XSendEvent(display, window, False, StructureNotifyMask, (XEvent *)&event);

} /* end of SendConfigureNotify */

/*
 * AddWidgetToWidgetBuffer
 * - Called from Initialize(), and ConstructGroupList().
 * From Initialize(), a wm widget has just been created, add it to the
 * window_list->] buffer.
 */

extern void
AddWidgetToWidgetBuffer OLARGLIST((wm, buffer))
OLARG(WMStepWidget,   wm)
OLGRA(WidgetBuffer *, buffer)
{

/* BufferFilled() is a macro - checks if buf->size==buf->used; initially, this
 * is true (buffer list is NULL).  GrowBuffer() <Xol/buffutil.c> realloc's list
 * by LIST_INCREMENT.
 */
if (BufferFilled(buffer))
   GrowBuffer((Buffer *)buffer, LIST_INCREMENT);

/* Add wm widget to list - access it as an array, update used field in this
 * buffer element to tell use that it is in use
 */
buffer-> p[buffer-> used++] = (Widget)wm;

} /* end of AddWidgetToWidgetBuffer */

/*
 * RemoveWidgetFromWidgetBuffer
 *
 */

extern void
RemoveWidgetFromWidgetBuffer OLARGLIST((wm, buffer))
OLARG(WMStepWidget,   wm)
OLGRA(WidgetBuffer *, buffer)
{
int i;

for (i = 0; i < buffer-> used; i++)
   if (buffer-> p[i] == (Widget)wm)
      break;

if (i < buffer-> used)
   {
   if (i != --buffer-> used)
      memmove(&buffer-> p[i], &buffer-> p[i + 1], 
         (buffer-> used - i) * buffer-> esize);
   }

} /* end of RemoveWidgetFromWidgetBuffer */


/* FindCurrentWindow.
 *	- Find the current window in the list of windows,
 * wmstep_kids.  If there is none, then make one up through a
 * scientific method.
 * - Return the index into the wmstep_kids array, or -1 if not found.
 ***************
 * ADDENDUM: if this code is going to select a window to be current,
 * then it must be modified for MWM_HINTS.input_mode restrictions.
 **************
 */
extern int
FindCurrentWindow OLARGLIST((any_window))
OLGRA(Boolean, any_window)
{
	Cardinal	n;
	Cardinal	nchildren = num_wmstep_kids;
	WidgetList	children  = wmstep_kids;
	WMStepWidget	wm;
	WMStepPart *	wmstep;
	Window		leader;


	/*
	 * Find ``current'' window.
	 * This code insures that there is always a current
	 * window (so long as there is at least one step widget).
	 */
	for (n = 0; n < nchildren; n++) {
		wm = (WMStepWidget)children[n];
		if (
			XtClass(wm) == wmstepWidgetClass
#ifdef WITHDRAWNSTATE
		     && wm->wmstep.size != WMWITHDRAWN
#endif
			&& wm->wmstep.is_current
		)
			break;
	}
        if (n >= nchildren && !any_window)
                return(-1);
	/* Generally, by this time, we will have a current window; if not
	 * search on.
	 */
	if (n >= nchildren) {
		/*
		 * No window is marked ``current'', so pick one.
		 * First choice = the one that has input focus;
		 * Second choice = the one that most recently had it;
		 * Third choice = a group leader;
		 * Fourth choice = the first on the list;
		 * Fifth choice (nobody on list) = XBell().
		 */
		int	maxindex = 0,
			maxfocus = 0;
		for (n = 0; n < nchildren; n++) {
			wm = (WMStepWidget)children[n];
			/* This case - if it has focus, then it
			 * should already be marked current, but just
			 * in case, we check.
			 */
			if ( (XtClass(wm) == wmstepWidgetClass) &&
				     (wm->wmstep.protocols & HasFocus))
				break;
			if (wm->wmstep.focus > maxfocus) {
				maxfocus = wm->wmstep.focus;
				maxindex = n;
			}
		}
		/* Now, if n < nchildren, then it is current;
		 * Else, if maxfocus == 0, then nobody has had focus yet
		 * or can take focus, for that matter;
		 * in that case, the current will be considered maxindex.
		 * But first, try and find a group leader...
		 */

		if (n >= nchildren && maxfocus == 0) {
			/* current not found */
			for (n = 0; n < nchildren; n++) {
				wm = (WMStepWidget)children[n];
				/* This is actually looking for what we
				 * call a "base" window - will not
				 * be a transient window, for example.
				 */
				if (
					(XtClass(wm) == wmstepWidgetClass)
			     	&& LEADER(wm) == wm-> wmstep.window 
#ifdef WITHDRAWNSTATE
				   && wm->wmstep.size != WMWITHDRAWN
#endif
				)
					break;
			}
			/* Give up, just use maxindex */
			if (n >= nchildren && nchildren > 0) {
				n = maxindex;
			}
			else {
				/* There are no children! */
				return(-1);
			}
		} /* end if (n >= nchildren && !maxfocus) */
		else
			if (n >= nchildren)
				/* nobody is current, just pick one */
				n = maxindex;
	} /* end if (n >= nchildren) */
	return(n);
} /* FindCurrentWindow */

/* CheckMotifFocusStops().
 * Return True if you can't move focus away from this window.
 */
extern Boolean
CheckMotifFocusStops OLARGLIST((wm,virtual_name))
OLARG(WMStepWidget, wm)
OLGRA(OlDefine, virtual_name)
{
	
	if (wm->wmstep.decorations & WMUsesMwmh &&
			   wm->wmstep.mwmh.flags & MWM_HINTS_INPUT_MODE) {
		int retval = 0;
	 switch(virtual_name) {
		default:
			break;
		case OL_NEXTWINDOW:
		case OL_PREVWINDOW:
			if(wm->wmstep.mwmh.inputMode == 
				MWM_INPUT_FULL_APPLICATION_MODAL) {
					/* Can't move focus to another
					 * window in this app.
					 */
					retval++;
			}
			/* fall through because if trying to move
			 * away from this window, you can't move
			 * if full app modal OR system modal.
			 */
		case OL_NEXTAPP:
		case OL_PREVAPP:
			if(wm->wmstep.mwmh.inputMode ==
					 MWM_INPUT_SYSTEM_MODAL) {
				/* Can't move focus anywhere! */
				retval++;
			}
			break; /* don't change focus windows */
		} /* switch */
		if (retval)
			return(True);
		else
			return(False);
	} /* if */
	else
		/* Property, or field in property, not present - all OK */
		return(False);
}


/* Next_Prev_Application.
 * - Find the next application in the list of windows.
 * Get next application for focus.  Possibly modified by "modifier" arg.
 * If modifier == ICON, then search for the next iconic application.
 * Otherwise, which == 0, just search for the next application in any state.
 *
 * w = the current step widget;
 * virtual_name = OL_NEXTAPP or OL_PREVAPP.
 * step_index = the index of w in the wmstep_kids array.
 * modifier = ICON, if the next/prev search is for an iconic application,
 * or 0.
 *
 * -  Return the index into the wmstep_kids array where to set the focus
 * next.
 */
extern int
Next_Prev_Application OLARGLIST((w, virtual_name, step_index, modifier))
OLARG(WMStepWidget, w)
OLARG(OlDefine, virtual_name)
OLARG(int, step_index)
OLGRA(int, modifier)
{
	int	n = step_index;
	int	k; 	/* must be signed */
	int	nchildren = num_wmstep_kids;
	WMStepWidget	wm;
	Window		leader = find_highest_group(w, (Window) 0);

		/*
		 * Find next/previous group-leader window outside current
		 * window group (if any):
		 *
		 * (1) Find group leader for current window,
		 *
		 * (2) search from there to find the next/previous group
		 * leader.
		 *
		 * If (2) fails to find a different group, "k" will
		 * be end up referencing the group leader of the current
		 * window; this will become next current window. Thus
		 * when only one application is running, OL_NEXTAPP or
		 * OL_PREVAPP moves to group leader (base window).
		 *
		 * Before doing anything, check to see if the window has
		 * motif hints, and is system modal.
		 */
		k = n;	/* k = current window index */
		n = find_leader(leader);
		if (n >= nchildren) {
			/*
			 * If the leader can't be found, something
			 * is screwy--let's assume it's the client
			 * who has screwed up, and make believe the
			 * current window is a base window.
			 *
			 * More (mlp) -
			 * maybe the leader isn't a mapped window,
			 * such as the workspace manager's popups;
			 * these all have the same leader, an
			 * unmapped window.  When looking for the
			 * next application, keep this in mind.
			 * Remember that, entering this code,
			 * leader == the group leader of THIS
			 * application (the window_group field in this
			 * window's WM_HINTS).  Be careful: this window
			 * may in turn have child popup windows that have
			 * IT as a group leader.
			 */
			n = k;
		}

		/* Note for nextapp and prevapp: we don't check the
		 * motif MODAL values if they exist, but we do for
		 * nextwin and prevwin.  For nextapp and prevapp,
		 * we assume that the last window that had focus
		 * will be assigned the focus, and this window has
		 * any modality constraints related to focus already
		 * set.  THIS WILL FAIL if a bunch of windows are on the
		 * screen when olwm is brought up, and the nextapp key is
		 * hit to go to an app with some unusual modal constraints,
		 * such as a window in the middle of a "tier" being
		 * primary modal.
		 */
	
		switch(virtual_name) {

		case OL_NEXTAPP:
			/* find an application whose "highest" leader is one
			 * other than the current leader just chosen.
			 * That will be the "next" application should be about.
			 */
			for (k = (int)((n == (nchildren - 1)) ? 0 : n + 1);
			     k != (int)n;
			     k = (int)((k == (nchildren - 1)) ? 0 : k + 1)) {
				wm = (WMStepWidget)wmstep_kids[k];

				/* Skip olwm help if not mapped */
				if ((wm_help_win_mapped == FALSE) && help_shell &&
				    (wm->wmstep.window == XtWindow(help_shell))
#ifdef WITHDRAWNSTATE
					|| wm->wmstep.size == WMWITHDRAWN
#endif
				)
					continue;
				if (
					XtClass(wm) == wmstepWidgetClass
				&& (find_highest_group(wm,(Window)0) != leader)
				) /* found someone in another group, not
				   * necessarily a group leader, though.
				   * Arg. "modifier" is non-zero iff == ICON.
				   * If which == 0 then take anything.
				   */
					if ( (modifier && IsIconic(wm) ) ||
								(!modifier))
						break;
			}
			if (k != n) {
				/* We found someone in another group for
				 * the next application; try and find the
				 * last window in that application that
				 * had input focus, set it; else,
				 * -If in iconic state, set icon to current.
				 * - else, try and use the group leader
				 * if it's mapped.
				 * Finally, there's another problem that
				 * is foreseeable: take olwsm - it has
				 * the main pinned menu and some popups
				 * beneath it (property sheets); the main
				 * pinned menu will have the same invisible
				 * group leader that the popup beneath it
				 * has (e.g., the color property sheet popup
				 * window).  Suppose these are at positions
				 * i and k in the wmstep_kids array; and a
				 * window in a diff. group, is at pos. j.
				 * If I go to the next application from j,
				 * it would take me to k; but SUPPOSE that
				 * I wanted to go to the window in the app.
				 * that had focus MOST recently - it may take
				 * me back around the array to window i, if
				 * i had focus more recently than j; and
				 * from here, typing NEXTAPP would take me
				 * to j, then to i, and so on - an infinite
				 * loop without getting to any other apps!
				 * 
				 * The problem is, we're trying to juggle 2
				 * things at once - a current marker in
				 * the wmstep_kids array, and a current
				 * marker with respect to the last window
				 * in a group that had focus!  As a result,
				 * you could wind up jumping around the
				 * array in such a way that you'll wind
				 * up toggleing between two application.
				 * The idea is you can't jump past any
				 * APPLICATION in the wmstep_kids array.
				 * One possibile solution: have two separate
				 * lists, one for ALL windows, and one
				 * for applications.
				 * The better and BEST solution - compress
				 * the array when an application (or
				 * window) is added or deleted to/from it.
				 * This was what I did in AddDeleteWMStep().
				 */
				k = TargetGroupLeader(k);
			} /* end if (k != n) */
			break;

		case OL_PREVAPP:
			/*
			 * "k" must be signed so that it can become
			 * negative.
			 */
			for (k = (int)((n == 0) ? (nchildren - 1) : n - 1);
			     k != (int)n;
			     k = (int)((k == 0) ? (nchildren - 1) : k - 1)) {
				wm = (WMStepWidget)wmstep_kids[k];
				if ((wm_help_win_mapped == FALSE) && help_shell &&
				    (wm->wmstep.window == XtWindow(help_shell))
#ifdef WITHDRAWNSTATE
					|| wm->wmstep.size == WMWITHDRAWN
#endif
				)
					continue;
				if (
					XtClass(wm) == wmstepWidgetClass
				&& (find_highest_group(wm,(Window)0) != leader)
				)
					if (modifier && (!(IsIconic(wm))) ||
								!modifier)
						break;
			} /* for */
			if (k != n) {
				/* we found someone in another group for
				 * the previous application; try and find the
				 * last window in this application that
				 * had input focus; if none did, then
				 * big deal - try and use the group leader					 * if it's mapped, else use this one!
				 */
				k = TargetGroupLeader(k);
			} /* end if k != n */
			break;
		default:
			break;
		} /* switch */
	return(k);
} /* Next_Prev_Application */

/* 
 * TargetGroupLeader.
 * Utility called by Next_Prev_Application.
 * Return the index into the wmstep_kids array of the group leader
 * being targeted for the step_index passed in.
 */
extern int
TargetGroupLeader OLARGLIST((step_index))
OLGRA(int, step_index)
{
	int	i,
		maxfocus,
		use_index = -1,
		k = step_index;
	WMStepWidget temp;

	temp = (WMStepWidget)wmstep_kids[k];
	if (IsIconic(temp)) {
		Window actual_group_leader;
		int newstep;
				
		actual_group_leader = find_highest_group(temp,(Window)0);
		if ( (newstep = IsWMStepChild( actual_group_leader)) >= 0) {
			temp = (WMStepWidget) wmstep_kids[newstep];
			if (IsIconic(temp) && temp->wmstep.icon_widget)
				 k = newstep;
		} /* if */
	} /* if IsIconic */
	else {
		ConstructGroupList((WMStepWidget)wmstep_kids[k],
						NoAppendAny, True);
		maxfocus = ( (WMStepWidget) (wmstep_kids[k]))->wmstep.focus;
		for (i = 1; i < group_list->used; i++) {
   			temp = (WMStepWidget)group_list->p[i];
			if (temp->wmstep.focus > maxfocus) {
				use_index = i;
				maxfocus = temp->wmstep.focus;
			} /* if */
		} /* end little for */
		if (use_index >=0 && (use_index = IsWMStepChild(
					temp->wmstep.window)) != -1) {
			k = use_index;
		} /* if */
	} /* end else */
	return(k);
} /* TargetGroupLeader */

/* Next_Prev_Window.
 *	- Within an application, find the next or previous
 * window. Modifier may be 0 (no restriction) or equal to TRANSIENT,
 * meaning the next window to traverse to must be a transient window.
 * (For motif rhetoric).
 */
extern int
Next_Prev_Window OLARGLIST((w, virtual_name, step_index, modifier))
OLARG(WMStepWidget, w)
OLARG(OlDefine, virtual_name)
OLARG(int, step_index)
OLGRA(int, modifier)
{
	int n = step_index;
	int k = step_index;
	int nchildren = num_wmstep_kids;
	WMStepWidget wm;
	Window leader;

	if (IsIconic(w)) {
		/* We consider all icons are in the same group */
		return(n);
	}
			
	/* Find the group leader of the current window */
	leader = find_highest_group(w, (Window) 0);

	switch(virtual_name) {
		default:
			break;
		case OL_NEXTWINDOW:

		 for (k = (int)((n == (nchildren - 1)) ? 0 : n + 1);
		     k != (int)n;
		     k = (int)((k == (nchildren - 1)) ? 0 : k + 1)) {
			wm = (WMStepWidget)wmstep_kids[k];
			if ( XtClass(wm) == wmstepWidgetClass
#ifdef WITHDRAWNSTATE
			   && wm->wmstep.size != WMWITHDRAWN
#endif
				&& (find_highest_group(wm,(Window)0) == leader) &&
				( (!modifier) || 
				  (modifier && wm->wmstep.transient_parent) )){
					/* Window found in the same application
					 * group; is there a motif restriction
					 * on traversing to it? 
					 */
					if (!(CheckTransientModality(wm)))
						break; /* for loop - all O.K. */
			} /* if stepwidget class and in highest group */
		} /* for loop */
		break;
		case OL_PREVWINDOW:
		  for (k = (int)((n == 0) ? (nchildren - 1) : n - 1);
		     k != (int)n;
		     k = (int)((k == 0) ? (nchildren - 1) : k - 1)) {
			wm = (WMStepWidget)wmstep_kids[k];
			if (XtClass(wm) == wmstepWidgetClass
#ifdef WITHDRAWNSTATE
			    && wm->wmstep.size != WMWITHDRAWN
#endif
				&& (find_highest_group(wm,(Window)0) == leader) &&
				( (!modifier) || 
				  (modifier && wm->wmstep.transient_parent) )){
				if (!(CheckTransientModality(wm)))
					break; /* for loop- all O.K. */
			} /* if class = step widget and in highest group */
		} /* for */
		break;
	} /* switch */
	return(k);
} /* Next_Prev_Window */

/*
 * CheckTransientModality.
 * - Given a step widget, check the modality of it's "children"
 * (windows that have it as a transient parent).  If such a window is
 * found that is (Motif) primary application modal, then this window
 * (wm) is not allowed to take focus - the rule says that if a window is
 * primary application modal, then only transient children of it may take
 * focus.
 *
 * Return False if no such window exists, otherwise return True.
 */
extern Boolean
CheckTransientModality OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
	WMStepWidget tempwm;
	int i;
	PropMwmHints *mwmp;

	ConstructGroupList(wm, NoAppend, True);

	/* What we want to know is if there are
	 * any primary app modal windows beneath
	 * this window (beneath => transient.
	 * If the answer is yes, then we can't
	 * use this window and we have to continue.
	 * The rule says that if a window is primary
	 * app modal, then only it's transient children
	 * may get focus.  So if a child of this
	 * window is prim. app. modal, then this is					 * a parent, and it isn't allowed to get focus.
	 */

	for (i=1; i < group_list->used; i++) {
		tempwm = (WMStepWidget)group_list->p[i];
		if (
#ifdef WITHDRAWNSTATE
			tempwm->wmstep.size == WMWITHDRAWN ||
#endif
			!(tempwm->wmstep.decorations & WMUsesMwmh))
				continue;
		mwmp = (PropMwmHints *)&(tempwm->wmstep.mwmh);
		if ((mwmp->flags & MWM_HINTS_INPUT_MODE) && (mwmp->inputMode
				 == MWM_INPUT_PRIMARY_APPLICATION_MODAL))
			break;
	} /* for */
	if (i >= group_list->used)
		return(False);
	return(True);
}

/*
 * OlwmWarpPointer.
 * - Given a step widget wm, warp the pointer to the widget, post menu,
 * then warp the pointer back to it's originial position.
 */
extern void
OlwmPostMenu OLARGLIST((wm, event))
OLARG(WMStepWidget, wm)
OLGRA(XEvent *, event)
{
	WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
	int x,y;
	int screenwid, screenht;
	int rootx, rooty, winx, winy; 
	Window root_win, child_win;
	unsigned int mask_ret;
	Display *display = XtDisplay((Widget)wm);
	Widget w = (IsIconic(wm) ? wm->wmstep.icon_widget:
				(Widget)wm);

	screenwid = WidthOfScreen(XtScreen(wm));
	screenht = HeightOfScreen(XtScreen(wm));
	x = w->core.x + Offset(wm);
	y = w->core.y + BorderY(wm) + (BannerHt(wm) - (int) mmHeight) / 2;

	if (x < 0)
		x = 0;
	else
	   	if (x >= screenwid)
			x = screenwid -1;

	if (y < 0) 
		y=0;
	else
	   if (y >= screenht)
		y = screenht -1;

	XQueryPointer(display,XtWindow(wm), &root_win,&child_win,
			&rootx,&rooty,&winx, &winy, &mask_ret);
	XWarpPointer(display,(Window)0, root_win,
		0, 0, (unsigned int)0,
		(unsigned int)0, x,y);

	Menu(wm, event, ((currentGUI == OL_OPENLOOK_GUI && !HasPushpin(wm) &&
		wmstep->decorations & WMHasFullMenu) ||
		currentGUI == OL_MOTIF_GUI &&
			wmstep->decorations & WMMenuButton) ? WM_MM :
							 WM_BANNER);

	/* Return pointer to previous location, from XQueryPointer */
	XWarpPointer(display, (Window)0, root_win,
			0, 0, (unsigned int)0,
			(unsigned int)0, rootx, rooty);
} /* OlwmPostMenu */

static void
SendKeyEvent OLARGLIST((wm, event))
OLARG(WMStepWidget, wm)
OLGRA(XEvent *, event)
{

	if (wm && (!NumMenushellsPosted) && PassKeysThrough) {
		/* Send keypress event to the current window.
		 * propagate flag == False, event mask == KeyPressMask
		 * There is also a way to send this message to the window
		 * that is under the pointer - see the manual.
		 * 
		 */
		event->xkey.send_event = True;
		XSendEvent(XtDisplay(wm), wm->wmstep.window, False, KeyPressMask,
								event);
	}
} /* SendKeyEvent */

extern Boolean
FocusSwitchOK OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
int ok = 0;
Window highest_window, w1;
WMStepWidget tempwm;
WMStepPart *wmstep;
int n;
	if (currentGUI != OL_MOTIF_GUI || wm->wmstep.is_current)
		return(True);
	n = FindCurrentWindow(True);
	highest_window = find_highest_group(wm, NULL);
			
	if (n == -1) /* nobody current */
		ok++;
	else {
		tempwm = (WMStepWidget)wmstep_kids[n];

		 /* wm = the current window step widget.
	 	  * Is this a motif client, and are there
		  * restrictions on the input focus?  
		  */ 
		/* Is the current window in the same app? */
		w1 = find_highest_group(tempwm, NULL);
		if (w1 == highest_window) {
			/* Same app */
			if(!CheckMotifFocusStops(tempwm, OL_NEXTWINDOW))
				ok++;
		}
		else /* diff apps */
		  if(!CheckMotifFocusStops(tempwm, OL_NEXTAPP))
			ok++;
		} /* else */
		if (ok && !(IsIconic(wm))) {
		/* Are there any transient children with
		 * input focus restrictions??
		 */
			if ((CheckTransientModality(wm)))
				ok = 0;
		} /* if ok */
	return(ok == 0 ? False : True);
} /* FocusSwitchOK */

static void
RaiseSetFocus OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
Display *display = XtDisplay(wm);

	/*RaiseLowerGroup(wm, WMRaise);*/
	/* Try this: set current for icons first */
	if (IsIconic(wm) && wm->wmstep.icon_widget && !(wm->wmstep.is_current))
		SetCurrent(wm);

	/* RaiseLowerGroup for all */
	RaiseLowerGroup(wm, WMRaise);

	if (IsIconic(wm)) {
		if (wmrcs->pointerFocus) {
			  XSetInputFocus(display, PointerRoot,
				   RevertToPointerRoot, LastEventTime);
		}
		else /* Click-To-Type */ {
		 XSetInputFocus(display, RootWindowOfScreen(
				XtScreen((Widget)wm)), 
				RevertToNone, LastEventTime);
		}
		/* SetCurrent(wm); */
	}
	else
		SetFocus((Widget)wm, GainFocus, 1);
} /* RaiseSetFocus */
