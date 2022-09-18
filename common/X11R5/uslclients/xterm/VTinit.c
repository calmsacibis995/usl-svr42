/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xterm:VTinit.c	1.59"
#endif

/*
 VTinit.c (C source file)
	Acc: 626545815 Wed Nov  8 11:30:15 1989
	Mod: 626545757 Wed Nov  8 11:29:17 1989
	Sta: 626545793 Wed Nov  8 11:29:53 1989
	Owner: 7007
	Group: 1985
	Permissions: 644
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/

/* Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts. */

#include <X11/copyright.h>

#include <stdio.h>
#include "ptyx.h"  
#include "data.h"
#include <X11/StringDefs.h>
#include <X11/Shell.h>			/* for SetValues of WM hints resources */
#include <Xol/DynamicP.h>	/* for MAndAList */
#include <Xol/AcceleratP.h>	/* for MAndAList */
#include <Xol/VendorI.h>	/* for OlVendorPartExtension */

/* #ifdef	SYSV */
#if defined(SYSV) || defined(SVR4)
#include	"xterm_ioctl.h"
#include	"error.h"
#endif /* SYSV */

#ifdef __STDC__
#include <limits.h>
#else
#define MB_LEN_MAX 5
#endif

#include "Strings.h"
#include "messages.h"

extern void exit();
static void VTallocbuf();
static void InitMouselessMode OL_ARGS((XtermWidget w));
static void UpdateHelpKey OL_ARGS((XtPointer w));


/*
 * USL SVR4: need to define SYSV
 */
#define SYSV YES

/* SS-moved from main.h */
#define	DEFBORDER		3
#define MAX_BORDER		40
#define DEF_TEXT_BORDER 2			/* FLH dynamic */
/* SS-end */

#define	XtNboldFont		"boldFont"

#ifndef SYSV
#define	XtNc132			"c132"
#endif

				
#ifdef SVR4				/* resources for SVR4 console logging */
#define	XtNconsole		"console"
#define	XtCConsole		"Console"
#endif

#define	XtNcurses		"curses"
#define	XtNcursorColor		"cursorColor"
#define	XtNinternalBorder	"internalBorder"
#define	XtNjumpScroll		"jumpScroll"
#define	XtNlogFile		"logFile"
#define	XtNlogging		"logging"
#define XtNcursesResize	"cursesResize"		/* FLH resize */
#define	XtNlogInhibit		"logInhibit"
#define	XtNloginShell		"loginShell"
#define XtNmarginBell		"marginBell"
#define XtNmouseless		"mouseless"
#define	XtNpointerColor		"pointerColor"
#define	XtNpointerShape		"pointerShape"
#define	XtNmultiScroll		"multiScroll"
#define	XtNnMarginBell		"nMarginBell"
#define	XtNreverseWrap		"reverseWrap"
#define	XtNsaveLines		"saveLines"
#define	XtNscrollBar		"scrollBar"


#ifdef XTERM_COMPAT
#define	XtNscrollInput		"scrollInput"
#define	XtNscrollKey		"scrollKey"
#endif

#define XtNscrollPos    	"scrollPos"
#define	XtNsignalInhibit	"signalInhibit"
#define	XtNvisualBell		"visualBell"
/* RJK begin (secure keyboard) */
#define XtNallowSendEvents	"allowSendEvents"
/* RJK end */
#ifdef TEK
#define	XtNtekInhibit		"tekInhibit"
#define XtNtekGeometry		"tekGeometry"
#define	XtNtekStartup		"tekStartup"
#define XtNtekSmall             "tekSmall"
#endif /* TEK */

#ifndef SYSV
#define	XtCC132			"C132"
#endif

#define	XtCCurses		"Curses"
#define	XtCJumpScroll		"JumpScroll"
#define	XtCLogfile		"Logfile"
#define	XtCLogging		"Logging"
#define 	XtCCursesResize	"CursesResize"	/* FLH resize */
#define	XtCLogInhibit		"LogInhibit"
#define	XtCLoginShell		"LoginShell"
#define	XtCMarginBell		"MarginBell"
#define XtCMouseless		"Mouseless"
#define	XtCMultiScroll		"MultiScroll"
#define	XtCColumn		"Column"
#define	XtCReverseWrap		"ReverseWrap"
#define XtCSaveLines		"SaveLines"
#define	XtCScrollBar		"ScrollBar"
#define XtCScrollPos     	"ScrollPos"
#define	XtCScrollCond		"ScrollCond"
#define	XtCSignalInhibit	"SignalInhibit"
#define	XtCVisualBell		"VisualBell"
/* RJK begin (secure keyboard) */
#define XtCAllowSendEvents	"AllowSendEvents"
/* RJK end */
#ifdef TEK
#define XtCTekInhibit           "TekInhibit"
#define XtCTekSmall             "TekSmall"
#define XtCTekStartup           "TekStartup"
#endif /*TEK */

#ifndef lint
static char rcs_id[] = "$Header: charproc.c,v 1.23 88/02/27 15:27:38 rws Exp $";
#endif	/* lint */

/* event handlers */
#ifndef SYSV
extern void HandleEnterWindow();
extern void HandleLeaveWindow();
#endif

/* SS-ioctl */
#define XK_MISCELLANY
#include <X11/keysymdef.h>
/* SS-ioctl-end */

/*
 * NOTE: VTInitialize zeros out the entire ".screen" component of the 
 * XtermWidget, so make sure to add an assignment statement in VTInitialize() 
 * for each new ".screen" field added to this resource list.
 */

/* Defaults */
static  Boolean	defaultFALSE	   = FALSE;
static  Boolean	defaultTRUE	   = TRUE;
static  int	defaultIntBorder   = DEFBORDER;
static  int	defaultSaveLines   = SAVELINES;
static  int	defaultNMarginBell = N_MARGINBELL;
static  Dimension defaultBorder    = DEF_TEXT_BORDER; /* FLH dynamic */

/* The next two variables should really be declared in VTInitialize. */
/* But since min size hints for xterm are not being honored when     */
/* specified before the widget is realized (a bug somewhere), we must*/
/* set these hints in VTInit.  The following 2 variables will be set */
/* in VTInitialize(), and later used in VTInit().		     */

static	Dimension	scrollbarWidth = 0;
static  int     borderX2;

/* FLH dynamic */
#define BYTE_OFFSET  XtOffset(XtermWidget, misc.dyn_flags)
static _OlDynResource dyn_res[] = {
{ { XtNbackground, XtCTextBackground, XtRPixel, sizeof(Pixel), 0,
XtRString, XtDefaultBackground }, BYTE_OFFSET, OL_B_XTERM_BG, NULL },
{ { XtNfontColor, XtCTextFontColor, XtRPixel, sizeof(Pixel), 0,
XtRString, XtDefaultForeground }, BYTE_OFFSET, OL_B_XTERM_FONTCOLOR, NULL },
{ {XtNinputFocusColor, XtCForeground, XtRPixel, sizeof(Pixel), 0,
XtRString, XtDefaultForeground }, BYTE_OFFSET, OL_B_XTERM_CURSORCOLOR, NULL},
};
#undef BYTE_OFFSET
/* FLH dynamic */

static XtResource resources[] = {
    /* FLH dynamic 
     *
     *	xterm text is surrounded by a border, scrollbar is outside border
     */
{XtNborderWidth,XtCBorderWidth,XtRDimension, sizeof(Dimension),
     XtOffset(XtermWidget, core.border_width),
     XtRDimension, (XtPointer) &defaultBorder},
    /*
     *	In Motif mode, the Primitive class uses the window border
     *	to indicate input focus (it is the location cursor).
     *	We want our border to be a constant color.
     */
{XtNborderColor, XtCBorderColor, XtRPixel,sizeof(Pixel),
     XtOffset(XtermWidget,screen.bordercolor),
     XtRString, XtDefaultForeground},

{XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct *),
     XtOffset(XtermWidget, screen.fnt_norm[0]), XtRString,
     (XtPointer) NlucidaTypewriter},
{XtNboldFont, XtCFont, XtRFontStruct, sizeof(XFontStruct *),
     XtOffset(XtermWidget, screen.fnt_bold[0]), XtRString,
     (XtPointer) NlucidaTypewriter},

#ifndef SYSV
{XtNc132, XtCC132, XtRBoolean, sizeof(Boolean),
	XtOffset(XtermWidget, screen.c132),
	XtRBoolean, (XtPointer) &defaultFALSE},
{XtNcurses, XtCCurses, XtRBoolean, sizeof(Boolean),
	XtOffset(XtermWidget, screen.curses),
	XtRBoolean, (XtPointer) &defaultFALSE},
#endif

/* FLH dynamic */
{XtNbackground, XtCTextBackground, XtRPixel, sizeof(Pixel),
/* SS-color : this was changed in many files without any comments */
/* FLH dynamic -- changed back. see VTInitialize and SetValues
 * for maintenence of screen.background
 *	XtOffset(XtermWidget, screen.background), */ /* core.background_pixel), */
/* SS-color-end */

	XtOffset(XtermWidget, core.background_pixel),
	XtRString, XtDefaultBackground},
/* FLH dynamic
 * foreground was moved from screen.foreground to primitive.font_color
 *	See VTInitialize and SetValues for maintenence of screen.foreground
 *
 * foreground has been replaced by fontColor (class TextFontColor) to pick
 * up changes to font_color.
 */
{	XtNfontColor, XtCTextFontColor, XtRPixel, sizeof(Pixel),
	XtOffset(XtermWidget, primitive.font_color), XtRString, XtDefaultForeground},
{XtNinputFocusColor, XtCForeground, XtRPixel, sizeof(Pixel),
	XtOffset(XtermWidget, primitive.input_focus_color),
	XtRString, XtDefaultForeground},
#ifdef TEK
{XtNtekGeometry,XtCGeometry, XtRString, sizeof(char *),
	XtOffset(XtermWidget, misc.T_geometry),
	XtRString, (XtPointer) NULL},
#endif /* TEK */
{XtNinternalBorder,XtCBorderWidth,XtRInt, sizeof(int),
	XtOffset(XtermWidget, screen.border),
	XtRInt, (XtPointer) &defaultIntBorder},
{XtNjumpScroll, XtCJumpScroll, XtRBoolean, sizeof(Boolean),
	XtOffset(XtermWidget, screen.jumpscroll),
	XtRBoolean, (XtPointer) &defaultTRUE},
{XtNlogFile, XtCLogfile, XtRString, sizeof(char *),
	XtOffset(XtermWidget, screen.logfile),
	XtRString, (XtPointer) NULL},
{XtNlogging, XtCLogging, XtRBoolean, sizeof(Boolean),
	XtOffset(XtermWidget, misc.log_on),
	XtRBoolean, (XtPointer) &defaultFALSE},
{XtNlogInhibit, XtCLogInhibit, XtRBoolean, sizeof(Boolean),
	XtOffset(XtermWidget, misc.logInhibit),
	XtRBoolean, (XtPointer) &defaultFALSE},
{XtNloginShell, XtCLoginShell, XtRBoolean, sizeof(Boolean),
	XtOffset(XtermWidget, misc.login_shell),
	XtRBoolean, (XtPointer) &defaultFALSE},
{XtNmarginBell, XtCMarginBell, XtRBoolean, sizeof(Boolean),
	XtOffset(XtermWidget, screen.marginbell),
	XtRBoolean, (XtPointer) &defaultFALSE},
{XtNmouseless, XtCMouseless, XtRBoolean, sizeof(Boolean),
	XtOffset(XtermWidget, misc.mouseless),
	XtRImmediate, False},
{XtNpointerColor, XtCForeground, XtRPixel, sizeof(Pixel),
	XtOffset(XtermWidget, screen.mousecolor),
	XtRString, XtDefaultForeground},
{XtNpointerShape,XtCCursor, XtRString, sizeof(Cursor),
	XtOffset(XtermWidget, misc.curs_shape),
	XtRString, (XtPointer) "xterm"},
{XtNmultiScroll,XtCMultiScroll, XtRBoolean, sizeof(Boolean),
	XtOffset(XtermWidget, screen.multiscroll),
	XtRBoolean, (XtPointer) &defaultFALSE},
{XtNnMarginBell,XtCColumn, XtRInt, sizeof(int),
	XtOffset(XtermWidget, screen.nmarginbell),
	XtRInt, (XtPointer) &defaultNMarginBell},
{XtNreverseVideo,XtCReverseVideo,XtRBoolean, sizeof(Boolean),
	XtOffset(XtermWidget, misc.re_verse),
	XtRBoolean, (XtPointer) &defaultFALSE},
{XtNreverseWrap,XtCReverseWrap, XtRBoolean, sizeof(Boolean),
	XtOffset(XtermWidget, misc.reverseWrap),
	XtRBoolean, (XtPointer) &defaultFALSE},
{XtNsaveLines, XtCSaveLines, XtRInt, sizeof(int),
	XtOffset(XtermWidget, screen.savelines),
	XtRInt, (XtPointer) &defaultSaveLines},
{XtNscrollBar, XtCScrollBar, XtRBoolean, sizeof(Boolean),
	XtOffset(XtermWidget, misc.scrollbar),
	XtRBoolean, (XtPointer) &defaultTRUE},

#ifdef XTERM_COMPAT
{XtNscrollInput,XtCScrollCond, XtRBoolean, sizeof(Boolean),
	XtOffset(XtermWidget, screen.scrollinput),
	XtRBoolean, (XtPointer) &defaultTRUE},
{XtNscrollKey, XtCScrollCond, XtRBoolean, sizeof(Boolean),
	XtOffset(XtermWidget, screen.scrollkey),
	XtRBoolean, (XtPointer) &defaultFALSE},
#endif

#ifdef TEK
{XtNtekInhibit, XtCTekInhibit, XtRBoolean, sizeof(Boolean),
        XtOffset(XtermWidget, misc.tekInhibit),
        XtRBoolean, (XtPointer) &defaultFALSE},
{XtNtekSmall, XtCTekSmall, XtRBoolean, sizeof(Boolean),
        XtOffset(XtermWidget, misc.tekSmall),
        XtRBoolean, (XtPointer) &defaultFALSE},
{XtNtekStartup, XtCTekStartup, XtRBoolean, sizeof(Boolean),
        XtOffset(XtermWidget, screen.TekEmu),
        XtRBoolean, (XtPointer) &defaultFALSE},
#endif /* TEK */

{XtNsignalInhibit,XtCSignalInhibit,XtRBoolean, sizeof(Boolean),
	XtOffset(XtermWidget, misc.signalInhibit),
	XtRBoolean, (XtPointer) &defaultFALSE},
{XtNvisualBell, XtCVisualBell, XtRBoolean, sizeof(Boolean),
	XtOffset(XtermWidget, screen.visualbell),
	XtRBoolean, (XtPointer) &defaultFALSE},
/* RJK begin (secure keyboard) */
{XtNallowSendEvents, XtCAllowSendEvents, XtRBoolean, sizeof(Boolean),
	XtOffset(XtermWidget, screen.allowSendEvents),
	XtRBoolean, (XtPointer) &defaultFALSE}
/* RJK end */ 
/* FLH resize */
,{XtNcursesResize, XtCCursesResize, XtRBoolean, sizeof(Boolean),
	XtOffset(XtermWidget, misc.allow_resize), XtRBoolean,
	(XtPointer) &defaultFALSE}
/* FLH resize-end */
#ifdef SVR4			/* console logging */
,{XtNconsole, XtCConsole, XtRBoolean, sizeof(Boolean),
	XtOffset(XtermWidget, misc.console_on), XtRBoolean,
	(XtPointer) &defaultFALSE}
#endif
};

static void VTInitialize(), VTRealize(), VTExpose(), VTConfigure();
static void StatusAreaResized OL_ARGS((Widget, XtPointer, XEvent *, Boolean *));

/* FLH mouseless */
/* FLH dynamic */
static Boolean AcceptFocus();
static Boolean SetValues();
static void            FocusHandler OL_ARGS((Widget, OlDefine));
/* FLH dynamic */
static Boolean	ActivateWidget OL_ARGS((Widget, OlVirtualName, XtPointer));
extern void	HandleKeyPressed OL_ARGS((Widget, OlVirtualEvent));
extern void	VTButtonPressed OL_ARGS((Widget, OlVirtualEvent));
extern void	VTButtonReleased OL_ARGS((Widget, OlVirtualEvent));
extern void	VTMouseMoved OL_ARGS((Widget, OlVirtualEvent));
extern void 	Message OL_ARGS((Widget, OlVirtualEvent));

#ifdef I18N
static char defaultTranslations[] = "\
        <Message>:      OlAction() \n\
        <FocusIn>:      OlAction() \n\
        <FocusOut>:     OlAction() \n\
        <Key>:          OlAction() \n\
        <BtnDown>:      OlAction() \n\
        <BtnUp>:        OlAction() \n\
\
        <BtnMotion>:    OlAction()";    /* see  VTMouseMoved */
#else
static char defaultTranslations[] = "\
        <FocusIn>:      OlAction() \n\
        <FocusOut>:     OlAction() \n\
        <Key>:          OlAction() \n\
        <BtnDown>:      OlAction() \n\
        <BtnUp>:        OlAction() \n\
\
        <BtnMotion>:    OlAction()";    /* see  VTMouseMoved */
#endif

static OlEventHandlerRec
event_procs[] = {
   { KeyPress, HandleKeyPressed},
   { ButtonPress, VTButtonPressed},
   { ButtonRelease, VTButtonReleased},
   { MotionNotify, VTMouseMoved},
#ifdef I18N
   { ClientMessage, Message},
#endif

};
/* FLH mouseless */

XtermClassRec xtermClassRec = {
  {
/* core_class fields */		
    /* superclass	  */	(WidgetClass) &primitiveClassRec,
    /* class_name	  */	"VT100",
    /* widget_size	  */	sizeof(XtermWidgetRec),
    /* class_initialize   */    NULL,
    /* class_part_initialize */ NULL,
    /* class_inited       */	FALSE,
    /* initialize	  */	VTInitialize,
    /* initialize_hook    */    NULL,				
    /* realize		  */	VTRealize,
    /* actions		  */	NULL,
    /* num_actions	  */	0,
    /* resources	  */	resources,
    /* num_resources	  */	XtNumber(resources),
    /* xrm_class	  */	NULLQUARK,
    /* compress_motion	  */	TRUE,
    /* compress_exposure  */	FALSE,
    /* compress_enterleave */   TRUE,
    /* visible_interest	  */	FALSE,
    /* destroy		  */	NULL,
    /* resize		  */	VTConfigure,
    /* expose		  */	VTExpose,
    /* set_values	  */	SetValues, 		/* FLH for dynamic resources */
    /* set_values_hook    */    NULL,
    /* set_values_almost  */    NULL,
    /* get_values_hook    */    NULL,
    /* accept_focus	  */	AcceptFocus,
    /* version            */    XtVersion,
    /* callback_offsets   */    NULL,
    /* tm_table           */    defaultTranslations,
    /* query_geometry */	NULL,
    /* display_accelerator */	NULL,
    /* extension	  */	NULL
  },
  {
  	/* primitive class */	/* FLH mouseless */
    /* focus_on_select    */	True,
    /* highlight_handler  */	FocusHandler,
    /* traversal_handler  */ 	NULL,
    /* register_focus     */	NULL,
    /* activate           */	ActivateWidget,
    /* event_procs        */	event_procs, 
    /* num_event_procs    */	XtNumber(event_procs),
    /* version            */	OlVersion,
    /* extension          */	NULL,
    /* dyn_data           */	{dyn_res, XtNumber(dyn_res)}, /* FLH dynamic */	
    /* transparent_proc   */	NULL
 },
 {
	0
 }
};

WidgetClass xtermWidgetClass = (WidgetClass)&xtermClassRec;


static	Boolean	color_display = TRUE;
static	Boolean	local_display = FALSE;
static	Boolean	ega_display   = FALSE;


alldone(display_ptr)
Display	*display_ptr;
{
	register TScreen *screen = &term->screen;

	HideCursor();
	screen->cursor_set = OFF;
	VTUnselect();
	reselectwindow (screen);
	exit(0);
}


VTRun()
{
	register TScreen *screen = &term->screen;
	register int i;
	XWindowAttributes       win_attrs;
#ifdef TEK
	extern void set_vt_visibility();
#endif /* TEK */
	
	if (term->misc.log_on)
		StartLog(screen);

#ifdef TEK
	if (!screen->Vshow)
	{
/* FLH dynamic
 *
 *		term is now child of container widget, shell is two
 *		widgets above
 */
	    XtRealizeWidget (VShellWidget);
	    set_vt_visibility (TRUE);
	}
#endif /* TEK */

	if (screen->allbuf == NULL) VTallocbuf ();

	screen->cursor_state = OFF;
	screen->cursor_set = ON;

	/* if xterm was started iconofied, we don't wan't to select window */

	XGetWindowAttributes (screen->display, VWindow(screen), &win_attrs);
	if (win_attrs.map_state != IsUnviewable)
	    selectwindow(screen, FOCUS);

	if(screen->select)
		VTSelect();

	if (L_flag > 0) {
		XWarpPointer (screen->display, None, VWindow(screen),
			    0, 0, 0, 0,
			    FullWidth(screen) >> 1, FullHeight(screen) >>1);
		L_flag = -1;
	}

	bcnt = 0;
#ifdef TEK
	bptr = buffer;
	while(Tpushb > Tpushback) {
		*bptr++ = *--Tpushb;
		bcnt++;
	}

	bcnt += (i = Tbcnt);
	for( ; i > 0 ; i--)
		*bptr++ = *Tbptr++;
#endif /* TEK */
	bptr = buffer;

	XSetIOErrorHandler(alldone);

	if(!setjmp(VTend))
		VTparse();
	HideCursor();
        screen->cursor_set = OFF;
}

/*ARGSUSED*/
static void VTExpose(w, event, region)
Widget w;
XEvent *event;
Region region;
{
	register TScreen *screen = &term->screen;

#ifdef DEBUG
	if(debug)
		fputs("Expose\n", stderr);
#endif	/* DEBUG */
	if (event->type == Expose)
		HandleExposure (screen, (XExposeEvent *)event);
}

static void VTGraphicsOrNoExpose (event)
XEvent *event;
    {
	register TScreen *screen = &term->screen;
	if (screen->incopy <= 0) {
		screen->incopy = 1;
		if (screen->scrolls > 0)
			screen->scrolls--;
	}
	if (event->type == GraphicsExpose)
	  if (HandleExposure (screen, (XExposeEvent *)event))
		screen->cursor_state = OFF;
	if ((event->type == NoExpose) || ((XGraphicsExposeEvent *)event)->count == 0) {
		if (screen->incopy <= 0 && screen->scrolls > 0)
			screen->scrolls--;
		if (screen->scrolls)
			screen->incopy = -1;
		else
			screen->incopy = 0;
	}
}


/*ARGSUSED*/
static void 
VTNonMaskableEvent (w, closure, event, continue_to_dispatch)
Widget w;
XtPointer closure;
XEvent *event;
Boolean *continue_to_dispatch;
{
    XMappingEvent *map_event = (XMappingEvent *)event;
    
    switch (event->type) {
	case MappingNotify:
	    XRefreshKeyboardMapping (map_event);
	    break;
	case GraphicsExpose:
	case NoExpose:
	    VTGraphicsOrNoExpose (event);
	    break;
	}
}




static void VTConfigure(w)
Widget w;
{
/*FLH */
	TScreen *term_screen;
	TScreen *w_screen;
	XtermWidget my_term;
	ScrnBuf w_allbuf;
	ScrnBuf t_allbuf;


	my_term = (XtermWidget) w;
	term_screen = &(my_term->screen);
	w_screen = &my_term->screen;
	w_allbuf = w_screen->allbuf;
	t_allbuf = term_screen->allbuf;


       if (XtIsRealized(w))
          ScreenResize (&(term->screen), term->core.width, term->core.height, &term->flags);
}

static Boolean failed = FALSE;

int VTInit ()
{
    register TScreen *screen = &term->screen;
/* SS-color */
    register int i;
    XColor	screen_def, exact_def;
    static char color_names[8][9] = {"black", "red", "green", "yellow",
       				 "blue", "magenta", "cyan", "white"};
/* SS-color-end */
/* SS-ioctl */
    char string[3];

    static char letters[] = "PQRSTUVWXYZABCDEFGHI";
#ifndef MEMUTIL
    extern char *malloc();
#endif
/* SS-ioctl */

    if (failed) return (0);



		/*		FLH dynamic
	  	 *		Realize toplevel shell
		 *		
		 *		It is now 2 widgets above term (container, shell)
		 */
    XtRealizeWidget (VShellWidget);

	/* set up min size hints for window manager. for some reason this */
	/* does not work when done in VTInitialize			  */
	
	{
             Arg args[2]; 
             Cardinal n = 0;

	     XtSetArg(args[n],XtNminWidth, 
	      (int) (10 * FontWidth(screen) + borderX2 + scrollbarWidth)); n++;
	     XtSetArg(args[n],XtNminHeight,
		      (int) (4 * FontHeight(screen) + borderX2));n++;
	     XtSetValues(VShellWidget,args,n);
	}

/* SS-color */
#define VTColormap DefaultColormap(screen->display, \
				       DefaultScreen(screen->display))
    for (i=0; i<8; i++)
    {
         XAllocNamedColor (screen->display, VTColormap, color_names[i],
				    &screen_def, &exact_def);
         Pixels[i] = exact_def.pixel;
    }
/* SS-color-end */
	if (screen->allbuf == NULL) VTallocbuf ();

/* SS-ioctl */

    /* bind escape sequences to FK, and store these escape sequences */
    /* into FKTrans_table					     */

    string[0] = '';
    string[1] = 'O';

    for (i=0; i<20; i++)
    {
         string[2] = letters[i];
         XRebindKeysym (screen->display, XK_F1+i,
		    (KeySym *)0, 0,
		    (Char *) string, 3);
	 if ((FKTrans_table[i] = malloc((unsigned)3)) != NULL)
	      strncpy (FKTrans_table[i], string, 3);
    }
/* SS-ioctl-end */

/* SS-inter */
    if (screen->segment.str == NULL)
	if ((screen->segment.str =
		(Char *) malloc ((unsigned) MB_LEN_MAX*(screen->max_col+1))) == NULL)
	     SysError (ERROR_SMALLOC);
/* SS-inter-end */

    return (1);
}

static void VTallocbuf ()
{
    register TScreen *screen = &term->screen;
    int nrows = screen->max_row + 1;
/* SS-color : use local registers */
    int ncols = screen->max_col + 1;
    extern ScrnBuf Allocate();

    /* allocate screen buffer now, if necessary. */
    if (screen->scrollWidget)
        nrows += screen->savelines;

    screen->allbuf = (ScrnBuf) Allocate (nrows, ncols);

    if (screen->scrollWidget)
        screen->buf =    &screen->allbuf[4 * screen->savelines];
    else
        screen->buf = screen->allbuf;
    return;
}

static void 
VTInitialize (request, new)
XtermWidget request, new;
{
    static char    my_TERM[20] = "TERM=xterm";
    int count;
    unsigned int width, height;
    register TScreen *screen = &new->screen;
    Pixel cursor_fg, cursor_bg;
    int xpos, ypos;
    String geometryString = NULL;
    Arg args[11]; 
    Cardinal n = 0;
    
#ifdef I18N
    OlFontList		*font_list = new->primitive.font_list;
#endif
    
#ifdef SVR4				/* console logging */
    extern void init_console();
#endif
    
    /* Zero out the entire "screen" component of "new" widget,
       then do field-by-field assigment of "screen" fields
       that are named in the resource list. */
    bzero ((char *) &new->screen, sizeof(new->screen));
#ifdef I18N
    /* copy inputmethod parameters */
    new->screen.im = NULL;
    
    /*
     * Initialize accelerator and mnemonic
     * update counters for input method 
     */
    new->screen.im_key_index = 0;
    new->screen.m_index = 0;
    new->screen.a_index = 0;
#endif
    
#ifdef I18N
    if (font_list){
	/* copy fontstruct pointers from primitive class font list */
	for (count = 0; count < font_list->num; count++){
	    new->screen.fnt_norm[count] = font_list->fontl[count];
	    new->screen.fnt_bold[count] = font_list->fontl[count];
	}
    }
    else{
   	new->screen.fnt_norm[0] = request->screen.fnt_norm[0];
   	new->screen.fnt_bold[0] = request->screen.fnt_bold[0];
    }
#else
    new->screen.fnt_norm[0] = request->screen.fnt_norm[0];
    new->screen.fnt_bold[0] = request->screen.fnt_bold[0];
#endif
    
    /* FLH copy allowsendevents flag*/
    new->screen.allowSendEvents = request->screen.allowSendEvents;
    /* FLH allowsendevents */
    new->screen.display = new->core.screen->display;
    new->screen.c132 = request->screen.c132;
    new->screen.curses = request->screen.curses;
    /* FLH dynamic */
    /* copy text background and font color into screen.background
     * and screen.foreground
     */
    new->screen.foreground 	= request->screen.foreground 
	= request->primitive.font_color;
    /* SS-color */
    new->screen.background 	= request->screen.background 
	= request->core.background_pixel;
    /* FLH dynamic */
    /* SS-color-end */
    
    /* SS-monocrome */ 
    /*MORE: comment out the if statement to test black on black for monochrome */
    if (DefaultDepth(new->screen.display,
		     DefaultScreen(new->screen.display)) < 3)
    {
	color_display = FALSE;
	
	/* if foreground and background are the same, change then to */
	/* black on white					    */
	
	if (new->screen.foreground == new->screen.background)
	{   
	    extern Boolean Mono_change;
	    Pixel black = BlackPixel(new->screen.display,
				     DefaultScreen(new->screen.display));
	    Pixel white = WhitePixel(new->screen.display,
				     DefaultScreen(new->screen.display));
	    Arg args[2];
	    
	    XtSetArg (args[0], XtNbackground, white);
	    XtSetArg (args[1], XtNforeground, black);
	    
	    new->screen.foreground = black;
	    new->core.background_pixel = new->screen.background = white;
	    Mono_change = TRUE;
	}
	
	/* set term */
	
	/* MORE: change this to xterm when testing black on black */
	strcpy(my_TERM, "TERM=xtermm");
    }

    /* the definition of the alternate characters beging at position 11.*/
    /* anything less in unprintable characters.  so, chances are, that  */
    /* if the characters at position less then 11 are defined, alternate*/
    /* characters will also be defined.  just to be on the safe side, we*/
    /* make sure that all charaters exist.  if both conditions are true */
    /* append '-acs' to the TERM name.					*/

    if (new->screen.fnt_norm[0]->min_char_or_byte2 <= 11 &&
        new->screen.fnt_norm[0]->all_chars_exist)
        strcat(my_TERM, "-acs");

    putenv(my_TERM);
    /* SS-monocrome-end */
    
    new->screen.cursorcolor = request->screen.cursorcolor
	= request->primitive.input_focus_color;
    new->screen.jumpscroll = request->screen.jumpscroll;
    new->screen.logfile = request->screen.logfile;
    new->screen.marginbell = request->screen.marginbell;
    new->screen.mousecolor = request->screen.mousecolor;
    new->screen.multiscroll = request->screen.multiscroll;
    new->screen.nmarginbell = request->screen.nmarginbell;
    /* SS-fix */
    {
    register int savel = request->screen.savelines;
    new->screen.savelines = savel < 0 ? 0 : (savel > 2*MAX_ROWS ?
					     2*MAX_ROWS : savel);
    savel = request->screen.border;
    new->screen.border = savel < DEFBORDER ? DEFBORDER :
	(savel > MAX_BORDER ? MAX_BORDER : savel);
    }
    /* SS-fix-end */
#ifdef XTERM_COMPAT
    new->screen.scrollinput = request->screen.scrollinput;
    new->screen.scrollkey = request->screen.scrollkey;
#endif
    new->screen.visualbell = request->screen.visualbell;
#ifdef TEK
    new->screen.TekEmu = request->screen.TekEmu;
#endif
    new->misc.re_verse = request->misc.re_verse;

#ifndef LOCATION_CURSOR
    /*	Copy the borderColor resource into the Core class border_pixel
     *	resource.  We have to do this here because, in Motif mode,
     *	the Primitive class Initialize proc is forcing the Core
     *	border_pixel to match the background color.  We want the 
     *	border to stand out from the background.
     */
    new->core.border_pixel = new->screen.bordercolor;
#endif

    /*
     * set the colors if reverse video; this is somewhat tricky since
     * there are 5 colors:
     *
     *     background - paper		white
     *     foreground - text		black
     *     border - border			black (foreground)
     *     textcursor - block		black (foreground)
     *     mousecursor - mouse		black (foreground)
     *
     */
    if (new->misc.re_verse) {
	Pixel fg = new->screen.foreground;
	/* SS-color */
	Pixel bg = new->screen.background;  /* core.background_pixel; */
	/* SS-color-end */
	
	if (new->screen.mousecolor == fg) new->screen.mousecolor = bg;
	if (new->screen.cursorcolor == fg) new->screen.cursorcolor = bg;
	if (new->core.border_pixel == fg) new->core.border_pixel = bg;
	new->screen.foreground = new->core.background_pixel = bg;
	new->screen.background = new->core.background_pixel = fg;
    }	



    
    new->keyboard.flags = 0;
    /* new->screen.display = new->core.screen->display; */
    new->core.height = new->core.width = 1;
    /* dummy values so that we don't try to Realize the parent shell 
       with height or width of 0, which is illegal in X.  The real
       size is computed in the xtermWidget's Realize proc,
       but the shell's Realize proc is called first, and must see
       a valid size. */
    
#ifdef SVR4
    if (new->misc.console_on)
	init_console(&new->screen);
#endif
    
    /* look for focus related events on the shell, because we need
     * to care about the shell's border being part of our focus.
     */
#ifndef SYSV
    /* for real-estate based input focus model */
    XtAddEventHandler(XtParent(new), EnterWindowMask, FALSE,
		      HandleEnterWindow, (Opaque)NULL);
    XtAddEventHandler(XtParent(new), LeaveWindowMask, FALSE,
		      HandleLeaveWindow, (Opaque)NULL);
#endif
    XtAddEventHandler((Widget) new, 0L, TRUE,
		      VTNonMaskableEvent, (Opaque)NULL);

    /*
     * Register routines used for getting the Help Key to the
     * application.
     */
    InitMouselessMode(new);


    /* 
     * determine the size and placement of the window
     */
    /* What does this do? REMOVE IT? */
    if (screen->fnt_bold[0] == screen->fnt_norm[0])
	screen->enbolden = TRUE;  /*no bold font */
    
    
    /* find the max width and height of the font */
    
#ifdef I18N
    /* determine character cell size by using the maximum
     * height over all 4 fonts, and the width of the ascii
     * font.  Single byte codesets are assumed to have single
     * column display width, and double-byte codesets are 
     * assumed to have double-column display width.
     */
    screen->fullVwin.f_width = (screen->fnt_norm[0])->max_bounds.width;
    if (font_list){
	screen->fullVwin.f_height = font_list->max_bounds.ascent 
	    + font_list->max_bounds.descent;
    }
    else{
	screen->fullVwin.f_height = (screen->fnt_norm[0])->ascent +
	    (screen->fnt_norm[0])->descent;
    }
#else
    screen->fullVwin.f_width = (screen->fnt_norm[0])->max_bounds.width;
    screen->fullVwin.f_height = (screen->fnt_norm[0])->ascent +
	(screen->fnt_norm[0])->descent;
#endif
    
    /* making cursor */
    cursor_bg = screen->background;
    if (screen->mousecolor == screen->background){
	cursor_fg = screen->foreground;
    } 
    else{
	cursor_fg = screen->mousecolor;
    }
    if (XStrCmp(new->misc.curs_shape, "arrow") == 0) {
	screen->curs = make_arrow (new, cursor_fg, cursor_bg);
    } 
    else {
	screen->curs = make_xterm (new, cursor_fg, cursor_bg);
    }
    /*
     * Now we set the window size and location:
     *		1) get the geometry string from the shell
     *		2) convert width and height from rows and columns to pixels
     *		3) set the shell's geometry-related resources
     */
    {
	register int display_width = 
	    DisplayWidth(screen->display, DefaultScreen(screen->display));
	register int display_height = 
	    DisplayHeight(screen->display, DefaultScreen(screen->display));
	register int font_width  =  FontWidth(screen);
	register int font_height =  FontHeight(screen);
	int pr;
	Dimension scrollbarBorder;
/* see note about the next two variables in the beginning of the file */
	/* int scrollbarWidth = 0; */
	/* register int */ borderX2 = 2 * screen->border + 2 * new->core.border_width;
	
	/* set defaults */
	xpos = 1; ypos = 1; width = 80; height = 25;
	
	XtVaGetValues(VShellWidget, XtNgeometry, &geometryString, NULL);
	pr = XParseGeometry(geometryString, &xpos, &ypos, &width, &height);
	
	/* SS-max-size: 
	 *		adjust the window size so it will be no larger than the screen
	 *		The scrollbar will appear off of the window, though.
	 */
	if ((pr & WidthValue) && 
	    ((width * font_width + borderX2) > display_width))
	    width = (display_width - borderX2) / font_width;
	if ((pr & HeightValue) && 
	    ((height * font_height + borderX2) > display_height))
	    height = (display_height - borderX2) / font_height;
	/* 
	 * SS-max-size-end 
	 */
	screen->max_col = width - 1;
	screen->max_row = height - 1;
	/* 
	 * calculate width and height for xterm widget
	 * this includes the text area and the internal border,
	 *	but does not include the window border or the scrollbar 
	 */
	width = width * font_width + borderX2; 
	height = height * font_height + borderX2;
	
	if (new->misc.scrollbar){
	    screen->scrollWidget = CreateScrollBar(new);
	    XtVaGetValues(screen->scrollWidget, 
			  XtNwidth, &scrollbarWidth, 
			  XtNborderWidth, &scrollbarBorder,
			  NULL);
	    scrollbarWidth += 2*scrollbarBorder;
	}
	
	if ((pr & XValue) && (XNegative&pr)){
	    /*
	     * calculate position from right edge of screen
	     */
	    /*
	     * first get border widths of toplevel shell and rubbertile
	     */
	    Dimension container_border_width;
	    Dimension toplevel_border_width;
	    
	    XtVaGetValues((Widget) container, 
			  XtNborderWidth, &container_border_width,
			  NULL);
	    XtVaGetValues(toplevel, 
			  XtNborderWidth, &toplevel_border_width,
			  NULL);
	    xpos += display_width - width - (container_border_width * 2)
		- (toplevel_border_width * 2) - scrollbarWidth;
	}
	if ((pr & YValue) && (YNegative&pr)){
	    /*
	     * calculate position from bottom edge of screen
	     */
	    /* 
	     *    must include borders of shell and container
	     */
	    Dimension container_border_width;
	    Dimension toplevel_border_width;
	    
	    XtVaGetValues((Widget) container, 
			  XtNborderWidth, &container_border_width,
			  NULL);
	    XtVaGetValues(toplevel, 
			  XtNborderWidth, &toplevel_border_width,
			  NULL);
	    
	    ypos += 	display_height - height - (container_border_width * 2)
		- (toplevel_border_width * 2);
	}
	
	
	/* set up size hints for window manager */
	/* original shell window includes xterm widget + border + scrollbar */
	
	n = 0;

/* For some reason the next two lines are not honored here.
   We do this again in VTInit() after the widget is realized

	XtSetArg(args[n],XtNminWidth, 
		 (int) (10 * font_width + borderX2 + scrollbarWidth)); n++;
	XtSetArg(args[n],XtNminHeight, (int) (4 * font_height + borderX2));n++;
*/
	XtSetArg(args[n],XtNx, (Position) xpos);n++;
	XtSetArg(args[n],XtNy, (Position) ypos);n++;
	XtSetArg(args[n],XtNbaseWidth, scrollbarWidth + borderX2);n++;
	XtSetArg(args[n],XtNbaseHeight, borderX2);n++;
	XtSetArg(args[n],XtNwidthInc, font_width);n++;
	XtSetArg(args[n],XtNheightInc, font_height);n++;
	XtSetValues(VShellWidget,args,n);

	/*
	 *	widget width and height contain internal border but not "Core" border
	 */
	new->core.width = screen->fullVwin.fullwidth = 
	    width - 2 * new->core.border_width;
	new->core.height = screen->fullVwin.fullheight = 
	    height - 2 * new->core.border_width;
	screen->fullVwin.width = width - borderX2;
	screen->fullVwin.height = height - borderX2;
    }
}

/*ARGSUSED*/
static void 
VTRealize (w, valuemask, values)
Widget w;
XtValueMask *valuemask;
XSetWindowAttributes *values;
{
    register TScreen *screen = &term->screen;
    XPoint	*vp;
    static short failed;
    XGCValues		xgcv;
    XtGCMask			mask;
    extern int		VTgcFontMask;
    Cardinal	n = 0;
    Arg args[8];	
    
#ifdef I18N                     
    XtermWidget     	xtermw = (XtermWidget) w;
    OlFontList *		font_list = xtermw->primitive.font_list;
    
    /* input method params */
#define NUM_ICV	11										/* # of input context values */
#define NUM_WATT	9										/* # of window attributes */
    OlIm *			im = screen->im;				/* input method */
    OlIcValues	icvalues[NUM_ICV+1];			/* input context values */
    OlIcValues	p_icvalues[NUM_WATT+1]; 	/* pre-edit values */
    Widget			Toplevel;
    XPoint			spot;								/* text insertion point */
    static  OlImStyle		style = OlImPreEditPosition|OlImStatusArea;
    Window			c_win;							/* client window */
    Window			f_win;							/* focus window */
    String			input_method;					/* name of input method */
    Pixel			*preedit_fg, *preedit_bg;
    static Boolean			ic_failed = False;
    static Boolean			im_failed = False;
#endif
    
    
    if(failed)
	return;
    
    TabReset (term->tabs);
    
    
    
    if (term->misc.re_verse && (term->core.border_pixel 
				== term->screen.background))
	values->border_pixel = 
	    term->core.border_pixel =
		term->screen.foreground;
    
    values->bit_gravity = NorthWestGravity;
    term->screen.fullVwin.window = term->core.window =
	XCreateWindow(XtDisplay(term), XtWindow(term->core.parent),
		      term->core.x, term->core.y,
		      term->core.width, term->core.height,
		      term->core.border_width,
		      (int) term->core.depth,
		      InputOutput, CopyFromParent,	
		      *valuemask|CWBitGravity, values);
    
#ifdef I18N
    /*
     *	Check if current locale requires an input method
     */
    
    XtSetArg(args[0], XtNinputMethod, &input_method);
    OlGetApplicationValues(w, args, 1);
    
    if (!im_failed && im==NULL && input_method!=NULL && *input_method != NULL){
	    /*
	     *	Open a connection to the input method
	     */
	    im = screen-> im = OlOpenIm(XtDisplay(w), NULL/*rdb*/, NULL, NULL);
	    if (im == NULL){
		    OlVaDisplayWarningMsg(XtDisplay(w), OleNolopenIM,
					  OleTbadOlopenIM,
					  OleCOlClientXtermMsgs,
					  OleMolopen_badOlopenIM, input_method);
		    im_failed = True;
		}
	    else
		if (xtermw->primitive.ic == NULL && !ic_failed){
			/*
			 *	Create an input context for xterm
			 */
			Toplevel = (Widget)w;
			while (!XtIsShell(Toplevel))
			    Toplevel = XtParent(Toplevel);
			n = 0;
			icvalues[n].attr_name = OlNclientWindow;
			icvalues[n].attr_value = (void *)&c_win;
			c_win = XtWindow(toplevel);
			n++;
			icvalues[n].attr_name = OlNfocusWindow;;
			icvalues[n].attr_value = (void *)&f_win;
			f_win = XtWindow(w);
			n++;
			icvalues[n].attr_name = OlNpreeditAttributes;
			icvalues[n].attr_value = (void *)p_icvalues;
			n++;
			icvalues[n].attr_name = OlNstatusAttributes;
			icvalues[n].attr_value = (void *)p_icvalues;
			n++;
			icvalues[n].attr_name = OlNinputStyle;
			icvalues[n].attr_value = (void *)&style;
			n++;
			icvalues[n].attr_name = OlNspotLocation;
			icvalues[n].attr_value = (void *)&spot;
			spot.x = CursorX(screen, screen-> cur_col);
			spot.y = CursorY(screen, screen-> cur_row);
			n++;
			
			preedit_fg = &screen->foreground;
			preedit_bg = &screen->background;
			
			if (_OlGetStatusArea()){
				/*
				 *	Create a status widget if input method requires one
				 */
				Dimension status_width 	= xtermw->core.width + 
				    2 * xtermw->core.border_width;
				Dimension status_height =  FontHeight(screen);
				Dimension status_x		= 0;
				Dimension status_y 		= xtermw->core.height +
				    2 * xtermw->core.border_width;
				Widget status_parent = XtParent(xtermw);	/* footerpane */
				Cardinal i;
				
				i = 0;
				XtSetArg(args[i], XtNforeground, *preedit_fg);i++;
				XtSetArg(args[i], XtNbackground, *preedit_bg);i++;
				XtSetArg(args[i], XtNheight, status_height);i++;
				XtSetArg(args[i], XtNwidth, status_width);i++;
				XtSetArg(args[i], XtNx, status_x);i++;
				XtSetArg(args[i], XtNy, status_y);i++;
				screen->statuswidget = (StubWidget) XtCreateManagedWidget(
											  "statusarea",
											  stubWidgetClass,
											  status_parent,	/* footerpane */	
											  args, i);
				/*
				 *	Set the status area size and position
				 */
				screen->statusarea.x	= status_x;
				screen->statusarea.y	= status_y;
				screen->statusarea.width	= status_width;
				screen->statusarea.height	= status_height;
				icvalues[n].attr_name = OlNstatusArea;
				icvalues[n].attr_value = (void *) &screen->statusarea;
				n++;
				/*
				 * Register a handler to update status size and
				 * position in response to window resizing
				 */
				XtAddEventHandler((Widget) screen->statuswidget,
						  StructureNotifyMask,
						  FALSE,
						  StatusAreaResized,
						  (XtPointer) xtermw);
			    }
			icvalues[n].attr_name = NULL;
			icvalues[n].attr_value = NULL;
			
			n = 0;
			p_icvalues[n].attr_name = OlNbackground;
			p_icvalues[n].attr_value = (void *)preedit_bg;
			n++;
			p_icvalues[n].attr_name = OlNforeground;
			p_icvalues[n].attr_value = (void *)preedit_fg;
			n++;
			p_icvalues[n].attr_name = OlNfontSet;
			p_icvalues[n].attr_value = (void *) (xtermw->primitive.font_list);
			n++;
			p_icvalues[n].attr_name = (char *)NULL;
			p_icvalues[n].attr_value = (void *)NULL;
			
			/* 
			 * create the input context 
			 */
			xtermw-> primitive.ic = OlCreateIc(im, icvalues);
			if (xtermw-> primitive.ic == NULL){
				OlVaDisplayWarningMsg(XtDisplay(w),
						      OleNolcreateIc,
						      OleTbadOlcreateIc,
						      OleCOlClientXtermMsgs,
						      OleMolcreateIc_badOlcreateIc,
						      input_method);
				ic_failed = True;
			    }
		    }
	}
#endif
    
    /* do the GC stuff */
    
    mask = VTgcFontMask | GCForeground | GCBackground 
	| GCGraphicsExposures | GCFunction;
    
    xgcv.graphics_exposures = TRUE;	/* default */
    xgcv.function = GXcopy;
    xgcv.foreground = screen->foreground;
    xgcv.background = screen->background;
    
#ifdef I18N
    /* if a font list is being used, allocate a GC for 
     *	each font.
     */
    if (font_list){
	    /* 
	     *	Normal GCs
	     */
	    for (n=0; n < font_list->num; n++){
		    xgcv.font = (screen->fnt_norm[n])->fid;
		    screen->normalGC[n] = XtGetGC((Widget)term, mask, &xgcv);
		}
	    /*
	     * Bold GCs
	     */
	    if (screen->enbolden) { /* there is no bold font */
		    for (n = 0; n < font_list->num; n++){
			    screen->normalboldGC[n] = screen->normalGC[n];
			}
		}
	    else{
		    for (n = 0; n < font_list->num; n++){
			    xgcv.font = (screen->fnt_bold[n])->fid;
			    screen->normalboldGC[n] = XtGetGC((Widget)term,mask,&xgcv);
			}
		}
	    /* 
	     * Reverse Normal GCs 
	     */
	    xgcv.foreground = screen->background;
	    xgcv.background = screen->foreground;
	    for (n = 0; n < font_list->num; n++){
		    xgcv.font = (screen->fnt_norm[n])->fid;
		    screen->reverseGC[n] = XtGetGC((Widget)term, mask, &xgcv);
		}
	    /*
	     * Reverse Bold GCs
	     */
	    if (screen->enbolden){ 	/* there is no bold font */
		    for (n = 0; n <font_list->num; n++)
			screen->reverseboldGC[n] = screen->reverseGC[n];
		}
	    else{
		    for (n = 0; n <font_list->num; n++){
			    xgcv.font = (screen->fnt_bold[n])->fid;
			    screen->reverseboldGC[n] = XtGetGC((Widget)term, 
							       mask, &xgcv);
			}
		}
	}
    else{
	    /*
	     * Normal GC
	     */
	    xgcv.font = (screen->fnt_norm[0])->fid;
	    screen->normalGC[0] = XtGetGC((Widget)term, mask, &xgcv);
	    /* 
	     * Bold GC
	     */
	    if (screen->enbolden) { /* there is no bold font */
		    screen->normalboldGC[0] = screen->normalGC[0];
		}else{
			xgcv.font = (screen->fnt_bold[0])->fid;
			screen->normalboldGC[0] = XtGetGC((Widget)term, mask, &xgcv);
		    }
	    /*
	     * Reverse Normal GC
	     */
	    xgcv.font = (screen->fnt_norm[0])->fid;
	    xgcv.foreground = screen->background;
	    xgcv.background = screen->foreground;
	    screen->reverseGC[0] = XtGetGC((Widget)term, mask, &xgcv);
	    /*
	     * Reverse Normal Bold GC
	     */
	    if (screen->enbolden) /* there is no bold font */
		xgcv.font = (screen->fnt_norm[0])->fid;
	    else
		xgcv.font = (screen->fnt_bold[0])->fid;
	    screen->reverseboldGC[0] = XtGetGC((Widget)term, mask, &xgcv);
	}
#else
    /*
     * Normal GC
     */
    xgcv.font = (screen->fnt_norm[0])->fid;
    screen->normalGC[0] = XtGetGC((Widget)term, mask, &xgcv);
    /* 
     * Bold GC
     */
    if (screen->enbolden) { /* there is no bold font */
	    screen->normalboldGC[0] = screen->normalGC[0];
	}else{
		xgcv.font = (screen->fnt_bold[0])->fid;
		screen->normalboldGC[0] = XtGetGC((Widget)term, mask, &xgcv);
	    }
    /*
     * Reverse Normal GC
     */
    xgcv.font = (screen->fnt_norm[0])->fid;
    xgcv.foreground = screen->background;
    xgcv.background = screen->foreground;
    screen->reverseGC[0] = XtGetGC((Widget)term, mask, &xgcv);
    /*
     * Reverse Normal Bold GC
     */
    if (screen->enbolden) /* there is no bold font */
	xgcv.font = (screen->fnt_norm[0])->fid;
    else
	xgcv.font = (screen->fnt_bold[0])->fid;
    screen->reverseboldGC[0] = XtGetGC((Widget)term, mask, &xgcv);
#endif
    
    
    /* we also need a set of caret (called a cursor here) gc's */
    
    xgcv.font = (screen->fnt_norm[0])->fid;
    
    /*
     * Let's see, there are three things that have "color":
     *
     *     background
     *     text
     *     cursorblock
     *
     * And, there are four situation when drawing a cursor, if we decide
     * that we like have a solid block of cursor color with the letter
     * that it is highlighting shown in the background color to make it
     * stand out:
     *
     *    selected window, normal video - foreground on cursor (reversecursorGC)
     *    selected window, reverse video - foreground on cursor (cursorGC)
     *  NOTE: reversecursorGC used to be background on cursor, and it
     *        is still the case on monochrome monitors, but on color
     *        monitors present setup gives better effects.
     *
     *    unselected window, normal video - foreground on background
     *    unselected window, reverse video - background on foreground
     *
     * Since the last two are really just normalGC and reverseGC, we only
     * need two new GC's.  Under monochrome, we get the same effect as
     * above by setting cursor color to foreground.
     */
    
    {
	Pixel cc = screen->cursorcolor;
	Pixel fg = screen->foreground;
	Pixel bg = screen->background;
	
	if (cc != fg && cc != bg) {
		/* we have a colored cursor */
		xgcv.foreground = fg;
		xgcv.background = cc;
		screen->cursorGC = XtGetGC ((Widget) term, mask, &xgcv);
		screen->reversecursorGC = XtGetGC ((Widget) term, mask, &xgcv);
		
	    } else {
		    screen->cursorGC = (GC) 0;
		    screen->reversecursorGC = (GC) 0;
		}
    }
    
    /* Reset variables used by ANSI emulation. */
    
    screen->gsets[0] = 'B';			/* ASCII_G		*/
    screen->gsets[1] = 'B';
    screen->gsets[2] = 'B';			/* DEC supplemental.	*/
    screen->gsets[3] = 'B';
    screen->curgl = 0;			/* G0 => GL.		*/
    screen->curgr = 2;			/* G2 => GR.		*/
    screen->curss = 0;			/* No single shift.	*/
    
    XDefineCursor(screen->display, VShellWindow, screen->curs );
    
    screen->cur_col = screen->cur_row = 0;
    screen->max_col = Width(screen)  / screen->fullVwin.f_width - 1;
    screen->top_marg = 0;
    screen->bot_marg = screen->max_row = Height(screen) /
	screen->fullVwin.f_height - 1;
    
    screen->sc.row = screen->sc.col = screen->sc.flags = NULL;
    
    /* Mark screen buffer as unallocated.  We wait until the run loop so
       that the child process does not fork and exec with all the dynamic
       memory it will never use.  If we were to do it here, the
       swap space for new process would be huge for huge savelines. */
#ifdef TEK
    if (!tekWidget)
#endif /* TEK */
	screen->buf = screen->allbuf = NULL;
    
    screen->do_wrap = NULL;
    screen->scrolls = screen->incopy = 0;
    vp = &VTbox[1];
    (vp++)->x = FontWidth(screen) - 1;
    (vp++)->y = FontHeight(screen) - 1;
    (vp++)->x = -(FontWidth(screen) - 1);
    vp->y = -(FontHeight(screen) - 1);
    screen->box = VTbox;
    
    vp = &VTwbox[1];
    (vp++)->x = FontWidth(screen) * 2 - 1;
    (vp++)->y = FontHeight(screen) - 1;
    (vp++)->x = -(FontWidth(screen) * 2 - 1);
    vp->y = -(FontHeight(screen) - 1);
    screen->w_box = VTwbox;
    
    screen->savedlines = 0;
    
    if(screen->scrollbar) {
	    /* FLH mouseless */
	    XWindowAttributes sb_attrs;	
	    /* FLH mouseless-end */
	    screen->scrollbar = 0;
	    ScrollBarOn(screen);
	    
	    /* FLH mouseless */
	    /* remove Keypress and Focus masks from scrollbar event */
	    /* mask so scrollbar will not take key/focus events.	*/
	    /* Scrolling keys are sent to the scrollbar by 		*/
	    /* associating the scrollbar with the xterm widget	*/
	    /* via OlAssociateWidget() -- see scrollbar.c		*/
	    
	    /* FLH mouseless-end */
	    XGetWindowAttributes(screen->display,
				 XtWindow(screen->scrollWidget),
				 &sb_attrs);
	    sb_attrs.your_event_mask &= ~(KeyPressMask | FocusChangeMask);
	    XSelectInput(screen->display,XtWindow(screen->scrollWidget),
			 sb_attrs.your_event_mask);	
        }
    
    CursorSave (term, &screen->sc);
    
    VTUnselect();
    return;
}

VTSelect()
{
	register TScreen *screen = &term->screen;

	if (VShellWindow)
	  XSetWindowBorder (screen->display, VShellWindow,
			    term->core.border_pixel);
}

VTUnselect()
{
	register TScreen *screen = &term->screen;

	if (VShellWindow)
	  XSetWindowBorderPixmap (screen->display, VShellWindow,
				  screen->graybordertile);
}

/* FLH mouseless */
/* ActivateWidget: widget activation goes here
 *		   currently, the xterm widget ignores all events 
 */
static Boolean
ActivateWidget OLARGLIST((w, type, call_data))
OLARG( Widget,    w)
OLARG( OlVirtualName,   type)
OLGRA( XtPointer, call_data)
{
	Boolean consumed = False;

	switch (type){
		default:
			break;
	}
	return (consumed);
} /* end of ActivateWidget */
/* FLH mouseless-end */

/* FLH dynamic resources */
/*
 *	SetValues:  respond to setting of 	background 
													fontcolor 
													input_focus_color
					for dynamic resources
 */
static Boolean
SetValues(current, request, new, args, num_args)
Widget current;
Widget request;
Widget new;
ArgList args;
Cardinal *num_args;
{
	XtermWidget current_term	= (XtermWidget) current;
	XtermWidget new_term	      = (XtermWidget) new;
	Boolean realized           = XtIsRealized(current);
	Boolean redisplay          = FALSE;
	Boolean color_changed		= FALSE;
	TScreen *screen            = &(new_term->screen);
	Display *dpy					= screen->display;
	Pixel font_color           = new_term->primitive.font_color;
	Pixel background           = new_term->core.background_pixel;
	Pixel cursorcolor          = new_term->primitive.input_focus_color;
	int i;
	XColor cursor_colors[2];
	static void get_cursor_gcs();

#ifdef I18N
	OlIcValues	icvalues[3];			/* input context values */
	OlIcValues	p_icvalues[3]; 	/* pre-edit values */
	Pixel			*preedit_fg, *preedit_bg;
	Arg			s_args[2];
	Cardinal		n;
#endif

#ifdef DEBUG
	printf("xterm setvalues called\n");
#endif
		/* check if font color has been changed */
	if (current_term->primitive.font_color!= font_color){
			/* prevent user from setting foreground == background 
			 *
			 * compare *new* font_color and *new* background
			 * 
			 * If user changes both foreground and background, 
			 * either
			 * 		both will succeed 
			 *		OR foreground will fail and background will succeed.
			 */
		if (font_color != background){ 
			int numGCs = 1;

			if (current_term->primitive.font_list)
				numGCs = current_term->primitive.font_list->num;
				/* update the GCs */
			for (i=0; i<numGCs; i++){
			     if (term->flags & REVERSE_VIDEO) {
					XSetBackground (screen->display, screen->normalGC[i], 
										 font_color);
					XSetBackground (screen->display, screen->normalboldGC[i], 
										 font_color);
					XSetForeground (screen->display, screen->reverseGC[i], 
										 font_color);
					XSetForeground (screen->display, screen->reverseboldGC[i], 
										 font_color);
			     }
			     else {
					XSetForeground (screen->display, screen->normalGC[i], 
										 font_color);
					XSetForeground (screen->display, screen->normalboldGC[i], 
										 font_color);
					XSetBackground (screen->display, screen->reverseGC[i], 
										 font_color);
					XSetBackground (screen->display, screen->reverseboldGC[i], 
										 font_color);
			     }
			}
			/* update cursor GC if using color cursor (see	*/
			/* comments in VTRealize). we don't check here  */
			/* if font color become equal to background or  */
			/* cursor color.				*/	

			if (screen->cursorGC) {
				XSetForeground (screen->display, screen->cursorGC, font_color);
				XSetForeground (screen->display, screen->reversecursorGC, font_color);
			}
			else
					/* 
					 * 	do we need to create a colored reverse cursor? 
					 */
				if (cursorcolor != font_color && cursorcolor != background)
					get_cursor_gcs(screen,font_color,cursorcolor);

				/* copy font color into screen structure */
			if (term->flags & REVERSE_VIDEO) {
			    screen->background = font_color;
			    XSetWindowBackground(screen->display,
			     			 TextWindow(screen),
						 screen->background);
			    XClearWindow(screen->display, TextWindow(screen));
			    ScrnRefresh (screen, 0, 0, screen->max_row + 1,
                                   	 screen->max_col + 1, FALSE);
			}
			else
			    screen->foreground = font_color;
			color_changed = TRUE;
			redisplay = TRUE;
		}
		else		/* new font_color == new background */
			/* reset foreground to original setting */
			new_term->primitive.font_color = current_term->primitive.font_color;
#ifdef DEBUG
		printf("primitive.font_color has changed\n");
#endif
	}

	if (current_term->core.background_pixel != background){
			/* prevent user from setting foreground == background */
		if (background != screen->foreground){
			int numGCs = 1;
			if (current_term->primitive.font_list)
				numGCs = current_term->primitive.font_list->num;
				/* update the GCs */
			for (i=0; i<numGCs; i++){
			     if (term->flags & REVERSE_VIDEO) {
					XSetForeground (screen->display, screen->normalGC[i], 
										 background);
					XSetForeground (screen->display, screen->normalboldGC[i], 
										 background);
					XSetBackground (screen->display, screen->reverseGC[i], 
										 background);
					XSetBackground (screen->display, screen->reverseboldGC[i], 
										 background);
			     }
			     else {
					XSetBackground (screen->display, screen->normalGC[i], 
										 background);
					XSetBackground (screen->display, screen->normalboldGC[i], 
										 background);
					XSetForeground (screen->display, screen->reverseGC[i], 
										 background);
					XSetForeground (screen->display, screen->reverseboldGC[i], 
										 background);
			     }
			}
			/* Do we need to create a colored cursor?
			 * check updated foreground and new background 
			 */
			if ((screen->cursorGC == (GC) 0) &&
			    (screen->foreground != cursorcolor &&
			     background != cursorcolor))
			     if (term->flags & REVERSE_VIDEO)
				get_cursor_gcs(screen,screen->background,cursorcolor);
			     else
				get_cursor_gcs(screen,screen->foreground,cursorcolor);

				/* copy background color into screen structure */
			if (term->flags & REVERSE_VIDEO) {
			    screen->foreground = background;
			    XSetWindowBackground(screen->display,
			     			 TextWindow(screen),
						 screen->background);
			    XClearWindow(screen->display, TextWindow(screen));
			    ScrnRefresh (screen, 0, 0, screen->max_row + 1,
                                   	 screen->max_col + 1, FALSE);
			}
			else
			    screen->background = background;

				/* update the mouse pointer color */
       	if (screen->mousecolor == screen->background){
      		cursor_colors[0].pixel = screen->foreground;
			}
        	else {
      		cursor_colors[0].pixel = screen->mousecolor;
			}
			cursor_colors[1].pixel = screen->background;
			XQueryColors (dpy, DefaultColormap (dpy, DefaultScreen (dpy)),
					cursor_colors, 2);
			XRecolorCursor(dpy, screen->curs, cursor_colors, cursor_colors+1);
			color_changed = TRUE;
			redisplay = TRUE;
		}
		else
				/* background == screen->foreground */
				/* restore original background */
			new_term->core.background_pixel = current_term->core.background_pixel;
#ifdef DEBUG
		printf("core.background pixel has been changed\n");
#endif
	}
			
			/* check if the cursor color has changed */
	if (current_term->screen.cursorcolor != cursorcolor){
				/* only support change if color cursor was originally in use */
		if (screen->cursorGC != (GC) 0 && screen-> reversecursorGC != (GC) 0){
			XSetBackground (screen->display, screen->cursorGC, cursorcolor);
			XSetBackground (screen->display, screen->reversecursorGC, cursorcolor);
		}
		else
				/* do we need to create colored cursors?
				 * 
				 * use updated fg and bg values to check 
				 */
			if (screen->foreground != cursorcolor &&
			    screen->background != cursorcolor)
			    if (term->flags & REVERSE_VIDEO)
				get_cursor_gcs(screen,screen->background,cursorcolor);
			    else
				get_cursor_gcs(screen,screen->foreground,cursorcolor);

				/* copy cursor color into screen structure */
		screen->cursorcolor = cursorcolor;
		redisplay = TRUE;

#ifdef DEBUG
		printf("cursor color has changed\n");
#endif
	}
	if (color_changed && term->primitive.ic != NULL){
			/*
			 * update ic status and pre-edit colors if using input method
			 */
		preedit_fg = &screen->foreground;
		preedit_bg = &screen->background;
		n = 0;
		p_icvalues[n].attr_name = OlNbackground;
		p_icvalues[n].attr_value = (void *)preedit_bg;
		n++;
		p_icvalues[n].attr_name = OlNforeground;
		p_icvalues[n].attr_value = (void *)preedit_fg;
		n++;
		p_icvalues[n].attr_name = (char *)NULL;
		p_icvalues[n].attr_value = (void *)NULL;

		n = 0;
		icvalues[n].attr_name = OlNpreeditAttributes;
		icvalues[n].attr_value = (void *)p_icvalues;
		n++;
		icvalues[n].attr_name = OlNstatusAttributes;
		icvalues[n].attr_value = (void *)p_icvalues;
		n++;
		icvalues[n].attr_name = NULL;
		icvalues[n].attr_value = NULL;

		if (OlSetIcValues(term->primitive.ic, icvalues) != NULL){
			OlVaDisplayWarningMsg(screen->display,
				OleNolsetIcValues, OleTbadOlsetIcValues,
				OleCOlClientXtermMsgs,
				OleMolsetIcValues_badOlsetIcValues, NULL);
		}
			/*
			 *	Update color of status widget
			 */
		if (term->screen.statuswidget){
			n = 0;
			XtSetArg(s_args[n], XtNforeground, *preedit_fg);n++;
			XtSetArg(s_args[n], XtNbackground, *preedit_bg);n++;
			XtSetValues((Widget) term->screen.statuswidget,s_args, n);
		}
	}
	return(redisplay);
}
/* FLH dynamic resources */

/* FLH dynamic */
/*
 * FocusHandler
 */
static void
FocusHandler OLARGLIST((w, highlight_type))
   OLARG( Widget,    w)
   OLGRA( OlDefine,  highlight_type)
{
    register TScreen *screen = &term->screen;
#ifdef I18N
    XtermWidget xtermw = (XtermWidget) w;
    OlIc * ic = xtermw->primitive.ic;
    Widget	vsw;
    OlVendorPartExtension   pe;
    OlIcValues              icvalues[2];
    OlMAndAList     *      a_m_list;
    int                     im_key_index;
    int                     num_keys;
    int                     i;
    static MAndAList *		  merged_list;
    Boolean						changed = False;
#endif
    
#ifdef DEBUG
    printf("VT100 Highlight Handler\007\n");
#endif
    
    
    /*
     * Initialize the merged list
     */
    if (merged_list == NULL){
	    merged_list = (MAndAList *) XtMalloc(sizeof(MAndAList));
	    merged_list->ol_keys = merged_list-> accelerators =
		merged_list->mnemonics = NULL;
	    merged_list->num_ol_keys = merged_list->num_mnemonics =
		merged_list->num_accelerators = 0;
	}

#ifdef LOCATION_CURSOR    
    /*
     *	Envelope the Primitive class highlight handler to show/hide
     *	location cursor in Motif mode.
     */
    
    if (OlGetGui() == OL_MOTIF_GUI)
    {
#define SUPERCLASS ((XtermClassRec *)xtermClassRec.core_class.superclass)
	    
	(*SUPERCLASS->primitive_class.highlight_handler)(w, highlight_type);

#undef SUPERCLASS
    }
#endif

    
    switch((int) highlight_type){
	    
	case OL_IN:
	    /* focus in, highlight cursor */
#ifdef DEBUG
	    printf("FOCUS IN\n");
#endif
	    selectwindow(screen,FOCUS);
#ifdef I18N
            /*
             * Notify input method that xterm widget has received focus
             */
	    if (ic){
		    
		    /* get those global keys which are not stored in Vendor shell */
		    im_key_index = screen->im_key_index;
		    a_m_list = NULL;
		    num_keys = 0;
		    (void)OlGetOlKeysForIm(&a_m_list, &im_key_index, &num_keys);
		    if (a_m_list != NULL)
			{
			    /* mark the change */
			    changed = True;
			    if (merged_list->ol_keys != NULL)
				XtFree((char *)merged_list->ol_keys);
			    merged_list->ol_keys = a_m_list;
			    merged_list->num_ol_keys = num_keys;
			    /* update the count so that we know whether to get new list */
			    screen->im_key_index = im_key_index;
			}
		    /* then, get the list of mnemonics, if mnemonics are not disabled */ 
		    if (OlQueryMnemonicDisplay(w) != OL_INACTIVE)
			{			 
			    vsw = _OlFindVendorShell(w, True);
			    pe = _OlGetVendorPartExtension(vsw);
			    if (pe->a_m_index != screen->m_index)
				{
				    a_m_list = NULL;
				    num_keys = 0;
				    (void)OlGetMAndAList(pe, &a_m_list, &num_keys, True);
				    /* mark the change */
				    changed = True;
				    if (merged_list->mnemonics != NULL)
					XtFree((char *)merged_list->mnemonics);
				    merged_list->mnemonics = a_m_list;
				    merged_list->num_mnemonics = num_keys;
				    screen->m_index = pe->a_m_index;
				}
			}
		    if (OlQueryAcceleratorDisplay(w) != OL_INACTIVE)
			{
			    /* and now, for the global accelerators - make sure no mnemonics
			     *  are picked up
			     */
			    vsw = _OlFindVendorShell(w, False);
			    if (vsw)
				{
				    pe = _OlGetVendorPartExtension(vsw);
				    if (pe->a_m_index != screen->a_index)
					{
					    a_m_list = NULL;
					    (void)OlGetMAndAList(pe, &a_m_list, &num_keys, False);
					    /* mark the change */
					    changed = True;
					    if (merged_list->accelerators != NULL)
						XtFree((char *)merged_list->accelerators);
					    merged_list->accelerators = a_m_list;
					    merged_list->num_accelerators = num_keys;
					    screen->a_index = pe->a_m_index;
					}
				}
			}
		    if (changed){
			    icvalues[0].attr_name = OlNacceleratorList;
			    icvalues[0].attr_value = (void *)merged_list;
			    icvalues[1].attr_name = NULL;
			    icvalues[1].attr_value = NULL;
			    OlSetIcValues(ic, icvalues);
			}
		    OlSetIcFocus(ic);
		}
#endif
	    break;
	case OL_OUT:
	    /* focus out, make cursor hollow */
#ifdef DEBUG
	    printf("FOCUS OUT\n");
#endif
	    unselectwindow(screen,FOCUS);
#ifdef I18N
            /*
             * Notify input method that xterm widget has received focus
             */
	    if (ic)
		OlUnsetIcFocus(ic);
#endif
	    break;
	default:
#ifdef DEBUG
	    OlWarning("Xterm: Unidefined highlight_type\n");
#endif
	    break;
	}
#ifdef DEBUG
    printf("");
    printf("focus widget is %x\n",w);
    printf("term is %x\n",term);
    printf("Shell is %x\n",VShellWidget);
    printf("scrollbar is %x\n",term->screen.scrollWidget);
#endif
}
/* FLH dynamic */


/*************************************************************************
 * AcceptFocus - If this widget can accept focus then it is set here
 *		 FALSE is returned if focus could not be set
 ***************************function*header*******************************/

static Boolean
AcceptFocus(w, time)
    Widget	w;
    Time *	time;
{
	register TScreen *screen = &term->screen;

#ifdef DEBUG
	printf("VT100 AcceptFocus\007\n");
#endif

   if (OlCanAcceptFocus(w, *time)){

		OlSetInputFocus ((Widget) term, RevertToParent, *time);
		focus_switched = VT_MODE;
		return (True);
	}
		else
			return (False);
} /* AcceptFocus() */

/*
 *		create_cursor_gcs: create gcs for colored cursor or background 
 */
static void 
get_cursor_gcs(screen,fg,cc)
TScreen *screen;
Pixel fg,cc;
{
	XGCValues      xgcv;
	XtGCMask       mask;

	mask = VTgcFontMask | GCForeground | GCBackground
			| GCGraphicsExposures | GCFunction;	
	xgcv.graphics_exposures = TRUE;  /* default */
	xgcv.function = GXcopy;
	xgcv.font = (screen->fnt_norm[0])->fid;	

	/* create cursor GCs for normal and reverse video (they are the same) */
	xgcv.foreground = fg;
	xgcv.background = cc;
  	screen->cursorGC = XtGetGC ((Widget) term, mask, &xgcv);
  	screen->reversecursorGC = XtGetGC ((Widget) term, mask, &xgcv);
}

static void
StatusAreaResized OLARGLIST((w, client_data, event, continue_to_dispatch))
OLARG(Widget,		w)
OLARG(XtPointer,	client_data)
OLARG(XEvent *,	event)
OLGRA(Boolean *,	continue_to_dispatch)
{
    OlIcValues		icvalues[2];
    XtermWidget		xtermw 	= client_data;
    TScreen			*screen 	= &xtermw->screen;
    char *retval;
    
    
    if (xtermw->primitive.ic != NULL){
	/*
	 *	Get status area geometry info for input method.
	 * Note that the status height does not change with resizing
	 * because it is based on the font height.
	 */
	
	screen->statusarea.x = 0;
	screen->statusarea.y = xtermw->core.height +
	    2 * xtermw->core.border_width;
	screen->statusarea.width = xtermw->core.width +
	    2 * xtermw->core.border_width;
	/* 
	 *	 Send the geometry info to the input method
	 */
	icvalues[0].attr_name = OlNstatusArea;
	icvalues[0].attr_value = (void *)&screen->statusarea;
	icvalues[1].attr_name = NULL;
	icvalues[1].attr_value = NULL;
	
	retval = OlSetIcValues(xtermw->primitive.ic, icvalues);
	
	if (retval != NULL){
	    OlVaDisplayErrorMsg(XtDisplay(toplevel), OleNolsetIcValues,
				OleTbadOlsetIcValuesStatus,
				OleCOlClientXtermMsgs,
				OleMolsetIcValues_badOlsetIcValuesStatus, NULL);
	}
    }
} /* end of StatusAreaResized() */



/*
 *  InitMouselessMode: 	Called by Initialize
 *
 *  In mouseless mode, the user can substitute special key sequences
 *  for mouse actions. For example, the OL_MENU key sequence will bring
 *  up the xterm menu.  When mouseless mode is off, we need to send all keys
 *  to the application running in the (child) shell of xterm.  The help
 *  key is a special case because it is grabbed by olwm.  We need to be able 
 *  to (re)construct the help KeyPress event in response to a ClientMessage
 *  event sent by the window manager.  To do this, we register a raw
 *  event handler to "steal" the ClientMessage from the toolkit.  (This event
 *  handler is registered on the top level shell here.  In menu.c, it is
 *  registered on the edit menu shell and the properties window shell.  Also,
 *  note that the help key binding can be dynamically changed by the user,
 *  so we need to keep track of the changes.  We register a dynamic callback
 *  to keep track of this.  Note that we register the event handler and
 *  the dynamic callback during Initialization, but
 *  we never unregister them.  When invoked, if mouseless mode is off,
 *  the dynamic callback only updates private data, and the raw event
 *  handler knows to ignore the event.
 */

static void
InitMouselessMode OLARGLIST((w))
    OLGRA(XtermWidget, w)
{
    static int initialized = 0;

    if (!initialized){
	initialized = 1;
	/* this is the first time, initialize the HelpKey definition */
	UpdateHelpKey(w);
	OlRegisterDynamicCallback(UpdateHelpKey, (XtPointer) w);
	/* add handler on top level shell to intercept help 
	 * ClientMessages from olwm
	 */
	XtInsertRawEventHandler(VShellWidget, NoEventMask, True, HandleHelpMessage, 
				(XtPointer) w,
				XtListHead);
	
    }
}	/* end of InitMouselessMode */

/*
 * UpdateHelpKey:	update definition of the help key
 *
 *	The helpKey is a dynamic resource.  Each time it changes,
 *	we convert its new value to an OlKeyDef which can later be
 *	used to construct a fake key event by HandleHelpMessage.
 *	The helpKey may have up to 2 definitions.  We store both
 *	but will always use the information from the first to generate
 *	our fake help key events.
 */
static void
UpdateHelpKey OLARGLIST((w))
    OLGRA(XtPointer, w)
{


    static XtResource resource[] = {
	XtNhelpKey, XtNhelpKey, XtROlKeyDef, sizeof(OlKeyDef),
	XtOffset(XtermWidget, misc.help_key), XtRString, NULL
    };

    XtGetApplicationResources(w, w, resource, XtNumber(resource),
			      NULL, 0);
#ifdef DEBUG
    { 
	XtermWidget xtermw = (XtermWidget) w;
	OlKeyDef *helpKey = &(xtermw->misc.help_key);
	int i;

	for (i=0; i<helpKey->used; i++){
	    fprintf(stderr, "definition %d:\n",i);
	    fprintf(stderr, "Modifiers: %x\n", helpKey->modifier[i]);
	    fprintf(stderr, "KeySym: %x %s\n", helpKey->keysym[i], 
		    XKeysymToString(helpKey->keysym[i]));
	    fprintf(stderr, "KeyCode: %x\n\n", XKeysymToKeycode(XtDisplay(w), 
								helpKey->keysym[i]));
	}
    }
#endif
}	/* end of UpdateHelpKey */

/*
 * HandleHelpMessage:  Convert Client message to fake Key event for client
 *
 *	The OPEN LOOK window manager (olwm) grabs the help key (usually F1).
 *	When F1 is pressed, olwm sends a ClientMessage to the client's shell.
 *	Usually, the vendorshell (on behalf of the client) extracts context 
 *	information from this message and posts a context-based help message.
 *	When mouselessMode is off, xterm is required to send all keys to 
 *	the underlying shell -- including the help key.  If mouseless mode
 *	is off, this routine intercepts the ClientMessage and constructs a 
 *	fake KeyPress event to send to the client.  Otherwise, it lets
 *	the ClientMessage be handled by the VendorShell.  The KeyPress event
 *	is constructed from the Help Key definition retrieved from the 
 *	application's resource database via UpdateHelpKey.
 */
extern void
HandleHelpMessage OLARGLIST((w, client_data, event, continue_to_dispatch))
    OLARG(Widget, 	w)
    OLARG(XtPointer, 	client_data)
    OLARG(XEvent *, 	event)
    OLGRA(Boolean *, 	continue_to_dispatch)
{
    XtermWidget xtermw = (XtermWidget) client_data;
    OlKeyDef *helpKey  = &(xtermw->misc.help_key);
    Display *dpy       = XtDisplay(w);
    XKeyEvent key_event; 
    OlVirtualEventRec  virtual_event;
    Window window_return;
    int x_return;
    int y_return;
    int root_x_return;
    int root_y_return;

    /*
     * We only intercept the Help ClientMessage if mouselessMode is off
     */
    if (xtermw-> misc.mouseless == False && 
	event->xany.type == ClientMessage && 
	event->xclient.message_type == XA_OL_HELP_KEY(event->xany.display)){

	*continue_to_dispatch = False;

	/*
	 * Ignore keypress events if xterm does not have input focus
	 * This will happen if the user presses F1 while the xterm main
	 * menu is posted.
	 */
	if (!xtermw->primitive.has_focus){
	    return;
	}
	/*
	 * (Mis)Use this existing toolkit routine to retrieve the x and
	 * y position
	 */
	GetHelpKeyMessage(dpy, event, &window_return, &x_return, &y_return, 
			  &root_x_return, &root_y_return);
	/*
	 * Construct a bogus Key Event for the help key.
	 */
	key_event.type        = KeyPress;
	key_event.serial      = event-> xany.serial;
	/* this is a security hole - unlikely to be found.  Any client
	 * that can forge the Help ClientMessage sent by the window manager
	 * can get xterm to send the help key to its application.
	 */
	key_event.send_event  = False;
	key_event.display     = event-> xany.display;
	key_event.window      = XtWindow((Widget)xtermw);
	key_event.root        = RootWindowOfScreen(XtScreen(w));
	key_event.subwindow   = NULL;
	key_event.time        = CurrentTime; /* punt... */
	key_event.x           = x_return;
	key_event.y           = y_return;
	key_event.x_root      = root_x_return;
	key_event.y_root      = root_y_return;
	/*
	 * Use the saved values for the help key.
	 * The alternate key sequence will never be sent.
	 */
	key_event.state       = helpKey->modifier[0];
	key_event.keycode     = XKeysymToKeycode(dpy, helpKey->keysym[0]);
	key_event.same_screen = True;  /* punt... */
	/*
	 * Now process the key event...  But first, we must construct
	 * an OlVirtualEvent because the Input routine expects one.
	 * The following line is stolen from OlAction().
	 */
	OlLookupInputEvent(w, (XEvent *) &key_event, &virtual_event, OL_DEFAULT_IE);
	/*
	 * Now send the key along to the KeyPress Input routine.
	 */
	HandleKeyPressed(w, &virtual_event);
    }
    else{
	/* Wrong type of message OR mouseless mode is ON.
	 * Let the event pass through.
	 */
	*continue_to_dispatch = True;
    }
}	/* end of HandleHelpMessage */
