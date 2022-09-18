/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xterm:menu.c	1.2.1.74"
#endif

/*
 menu.c (C source file)
	Acc: 601052323 Tue Jan 17 09:58:43 1989
	Mod: 601054100 Tue Jan 17 10:28:20 1989
	Sta: 601054100 Tue Jan 17 10:28:20 1989
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

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xlib.h>

#include <Xol/OpenLookP.h>
#include <Xol/FButtons.h>
#include <Xol/PopupMenu.h>
#include <Xol/PopupWindo.h>

#include <signal.h>
#include <setjmp.h>
#include "ptyx.h"
#include "data.h"
#include "xterm_menu.h"

#include "Strings.h"
#include "messages.h"

/*
 * USL SVR4: need to define SYSV
 */
#define SYSV YES

void            HandleEvent();
static void	ApplyProperty(), ResetProperty(), VerifyProperty();
extern void	xmenu_redraw(), xmenu_softreset(), xmenu_fullreset();
extern void	xmenu_property(), xmenu_intr(), xmenu_hangup();
extern void	xmenu_kill(), xmenu_term();
extern void	edit_send(), edit_paste(), edit_copy();
extern void	enable_resize(), disable_resize();
#ifdef XTERM_CUT
extern void	edit_cut();
#endif /* XTERM_CUT */


#define	NAME_LEN	30

#ifdef TEK
#define TNUM_PROP	4
#define	TNAME_LEN	20

extern void	xmenu_hide_tek_window();
#endif

Boolean  prop_state[NUM_PROP] = {FALSE, FALSE, FALSE, FALSE, FALSE,
		                 FALSE, FALSE, FALSE, FALSE, FALSE,
			         FALSE 
#ifdef SECURE_KEYBOARD
				     ,FALSE
#endif /* SECURE_KEYBOARD */
/* FLH resize */	, FALSE
#ifdef XTERM_COMPAT
				 , FALSE, FALSE, FALSE
#endif
				 };

static Boolean	prop_changed = FALSE;
static Boolean	popdown;

static Widget prop_shell, prop_menu,
	      menu_pane,
	      flat_checkbox;

static char *menuFields [] = {
    XtNlabel, XtNmnemonic, XtNselectProc, XtNsensitive, XtNpopupMenu,
};

typedef struct {
    XtArgVal	label;
    XtArgVal	mnemonic;
    XtArgVal	select;
    XtArgVal	sensitive;
    XtArgVal	submenu;
} MenuItems;

static MenuItems xterm_b [] = {
    { (XtArgVal) "Edit", (XtArgVal) 'E', (XtArgVal) NULL, (XtArgVal) True, },
    { (XtArgVal) "Redraw", (XtArgVal) 'R', (XtArgVal) xmenu_redraw,
	  (XtArgVal) True, },
    { (XtArgVal) "Soft Reset", (XtArgVal) 'S', (XtArgVal) xmenu_softreset,
	  (XtArgVal) True, },
    { (XtArgVal) "Full Reset", (XtArgVal) 'F', (XtArgVal) xmenu_fullreset,
	  (XtArgVal) True, },
    { (XtArgVal) "Properties...", (XtArgVal) 'P', (XtArgVal) xmenu_property,
	  (XtArgVal) True, },
#ifdef TEK
    { (XtArgVal) "Show Tek Window", (XtArgVal) 'o',
	  (XtArgVal) xmenu_hide_tek_window, (XtArgVal) True, },
#endif /* TEK */
    { (XtArgVal) "Interrupt", (XtArgVal) 'I', (XtArgVal) xmenu_intr,
	  (XtArgVal) True, },
    { (XtArgVal) "Hangup", (XtArgVal) 'H', (XtArgVal) xmenu_hangup,
	  (XtArgVal) True, },
    { (XtArgVal) "Terminate", (XtArgVal) 'T', (XtArgVal) xmenu_term,
	  (XtArgVal) True, },
    { (XtArgVal) "Kill", (XtArgVal) 'K', (XtArgVal) xmenu_kill,
	  (XtArgVal) True, },
};

static MenuItems edit_b [] = {
    { (XtArgVal) "Send", (XtArgVal) 'S', (XtArgVal) edit_send,
	  (XtArgVal) True, },
    { (XtArgVal) "Paste", (XtArgVal) 'P', (XtArgVal) edit_paste,
	  (XtArgVal) True, },
    { (XtArgVal) "Copy", (XtArgVal) 'C', (XtArgVal) edit_copy,
	  (XtArgVal) True, },
#ifdef XTERM_CUT
    { (XtArgVal) "Cut", (XtArgVal) 'X', (XtArgVal) edit_cut,
	  (XtArgVal) True, },
#endif /* XTERM_CUT */
};

void		UpdateProperty();
static void	Prop_Changed();

typedef struct {
	XtArgVal	label;
	XtArgVal	select;
	XtArgVal	unselect;
	XtArgVal	state;
	XtArgVal	sensitive;
	XtArgVal	ljustify;
} FlatCheckbox;

String check_fields[] = { XtNlabel, XtNselectProc, XtNunselectProc, XtNset, XtNsensitive, XtNlabelJustify };

static Arg touchArg[] = {{XtNitemsTouched, (XtArgVal) True}};

static FlatCheckbox check_items[NUM_PROP];

#ifdef TEK

static Widget Tprop_shell, Tprop_menu,
              Tmenu_pane,
              flat_excl;

extern void	Tmenu_page(), Tmenu_reset(), Tmenu_copy(), Tmenu_redraw();
extern void	Tmenu_property(), Tmenu_hide_vt();

static MenuItems Txterm_b [] = {
    { (XtArgVal) "PAGE", (XtArgVal) 'A', (XtArgVal) Tmenu_page,
          (XtArgVal) True, },
    { (XtArgVal) "RESET", (XtArgVal) 'E', (XtArgVal) Tmenu_reset,
          (XtArgVal) True, },
    { (XtArgVal) "COPY", (XtArgVal) 'C', (XtArgVal) Tmenu_copy,
          (XtArgVal) True, },
    { (XtArgVal) "Redraw", (XtArgVal) 'R', (XtArgVal) Tmenu_redraw,
          (XtArgVal) True, },
    { (XtArgVal) "Properties...", (XtArgVal) 'P', (XtArgVal) Tmenu_property,
          (XtArgVal) True, },
    { (XtArgVal) "Hide VT Window", (XtArgVal) 'd', (XtArgVal) Tmenu_hide_vt,
          (XtArgVal) True, },
    { (XtArgVal) "Interrupt", (XtArgVal) 'I', (XtArgVal) xmenu_intr,
          (XtArgVal) True, },
    { (XtArgVal) "Hangup", (XtArgVal) 'H', (XtArgVal) xmenu_hangup,
          (XtArgVal) True, },
    { (XtArgVal) "Terminate", (XtArgVal) 'T', (XtArgVal) xmenu_term,
          (XtArgVal) True, },
    { (XtArgVal) "Kill", (XtArgVal) 'K', (XtArgVal) xmenu_kill,
	  (XtArgVal) True, },
};

Boolean  Tprop_state[TNUM_PROP];
static Boolean	Tprop_changed = FALSE;
static Boolean	Tpopdown;
void   	    TUpdateProperty();
static void TProp_Changed();

typedef struct {
	XtArgVal	label;
	XtArgVal	select;
	XtArgVal	unselect;
	XtArgVal	state;
	XtArgVal	ljustify;
} FlatExcl;

String excl_fields[] = { XtNlabel, XtNselectProc, XtNunselectProc, XtNset, XtNlabelJustify };

static FlatExcl excl_items[TNUM_PROP];

void   set_vt_visibility(), set_tek_visibility(), end_tek_mode(), end_vt_mode();
#endif /* TEK */

/******* xterm menu call back routines	*******/

/* ARGSUSED */
void
xmenu_redraw(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	Redraw();
}

/* ARGSUSED */
static void
xmenu_softreset (w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	VTReset(FALSE, term->screen.menuWidget);
}

/* ARGSUSED */
static void
xmenu_fullreset (w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	VTReset(TRUE, term->screen.menuWidget);
}

/* ARGSUSED */
static void
xmenu_property(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	register TScreen *screen = &term->screen;
	void     SetUpProperties();

	if (screen->property == NULL)
	    SetUpProperties();
	XtPopup(prop_shell, XtGrabNone);
	XRaiseWindow(XtDisplay(prop_shell), XtWindow(prop_shell));
}

/* the following routine is a callback for the VT menu button.  It is	*/
/* also called from the dpmodes() for the \E[?38h (enter Tek mode).     */
/* In the second case all 3 arguments are NULLs, and we always want to  */
/* give focus to the Tek window						*/
/*FLH tek-vt switch
   It is also called from Tekparse() for the \E7 (leave Tek mode)
   with all 3 arguments NULL
FLH tek-vt switch */

#ifdef TEK
void
xmenu_hide_tek_window(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	register TScreen *screen = &term->screen;

	/* show tek window	*/

	if (!screen->Tshow) {
	    XWindowAttributes       win_attrs;

	    if (screen->menuWidget) {
#if !defined(I18N)
		xterm_b[VT_HIDE_TEK_WIN].label = (XtArgVal) "Hide Tek window";
		xterm_b[VT_HIDE_TEK_WIN].mnemonic = (XtArgVal) 'd';
#else
		static char * label = NULL;
		if (label == NULL) {
		    label = OlGetMessage(
		        screen->display, NULL, 0, OleNlabel,
		        OleThideTek, OleCOlClientXtermMsgs,
		        OleMlabel_hideTek, NULL
		    );
		    xterm_b[VT_HIDE_TEK_WIN].label = (XtArgVal)label;
		}
#endif
		XtSetValues (menu_pane, touchArg, 1);
	    }
	    set_tek_visibility (TRUE);
	    end_vt_mode (w);

	    XGetWindowAttributes (screen->display, TWindow(screen),                                               &win_attrs);

	    if (win_attrs.map_state != IsUnviewable)
	    {
/* FLH mouseless XSetInputFocus --> OlSetInputFocus */
			OlSetInputFocus ((Widget) tekWidget, RevertToParent, CurrentTime);
			XRaiseWindow(XtDisplay(tekWidget), XtWindow(tekWidget));
	    }

	/* hide tek window	*/

	} else {
	    if (screen->menuWidget) {
#if !defined(I18N)
		xterm_b[VT_HIDE_TEK_WIN].label = (XtArgVal) "Show Tek window";
		xterm_b[VT_HIDE_TEK_WIN].mnemonic = (XtArgVal) 'o';
#else
		static char *label = NULL;

		if (label == NULL) {
		    label = OlGetMessage(screen->display, NULL, 0, OleNlabel,
					OleTshowTek, OleCOlClientXtermMsgs,
					OleMlabel_showTek, NULL);
		    xterm_b[VT_HIDE_TEK_WIN].label = (XtArgVal)label;
		}
#endif
		XtSetValues (menu_pane, touchArg, 1);
	    }
	    set_tek_visibility (FALSE);
	    end_tek_mode (w);
	}
	reselectwindow(screen);
}
#endif /* TEK */

/* ARGSUSED */
void
xmenu_intr (w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	register TScreen *screen = &term->screen;
	if (screen->pid > 1)
		killpg(xgetpgrp(screen->pid), SIGINT);
}

/* ARGSUSED */
void
xmenu_hangup (w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	register TScreen *screen = &term->screen;
	if (screen->pid > 1)
		killpg(xgetpgrp(screen->pid), SIGHUP);
}

/* ARGSUSED */
void
xmenu_term (w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	register TScreen *screen = &term->screen;
	if (screen->pid > 1)
		killpg(xgetpgrp(screen->pid), SIGTERM);
}

/* ARGSUSED */
void
xmenu_kill (w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	register TScreen *screen = &term->screen;
	if (screen->pid > 1)
		killpg(xgetpgrp(screen->pid), SIGKILL);
}


/***** modes menu pseudo-callback routines *****/


static void mmenu_visualbell()
{
	term->screen.visualbell = !term->screen.visualbell;
}

static void mmenu_log()
{
	register TScreen *screen = &term->screen;

	if (screen->logging)
	    CloseLog(screen);
	else
	    StartLog(screen);

	/* MORE: if login file is not open, remove the check mark */

	if (!screen->logging)
	    UpdateProperty(LOGGIN_PROP, FALSE);
}

static void mmenu_scroll()
{
	register TScreen *screen = &term->screen;

	term->flags ^= SMOOTHSCROLL;
	if (term->flags & SMOOTHSCROLL) {
	        screen->jumpscroll = FALSE;
	        if (screen->scroll_amt)
	                FlushScroll(screen);
	} else
	        screen->jumpscroll = TRUE;
}

static void mmenu_video()
{
	term->flags ^= REVERSE_VIDEO;
	ReverseVideo(term);
}

static void mmenu_wrap()
{
	term->flags ^= WRAPAROUND;
}

static void mmenu_reversewrap()
{
	term->flags ^= REVERSEWRAP;
}

static void mmenu_nlm()
{
	term->flags ^= LINEFEED;
}

static void mmenu_cursor()
{
	term->keyboard.flags ^= CURSOR_APL;
}

static void mmenu_pad()
{
	term->keyboard.flags ^= KYPD_APL;
}

static void mmenu_scrollbar()
{
	register TScreen *screen = &term->screen;

	if(screen->scrollbar)
		ScrollBarOff(screen);
	else
		ScrollBarOn(screen);
}

static void mmenu_margbell()
{
	register TScreen *screen = &term->screen;

	if(!(screen->marginbell = !screen->marginbell))
             screen->bellarmed = -1;
}

#ifdef SECURE_KEYBOARD
static void mmenu_securekeys()
{
	register TScreen *screen = &term->screen;
	char   new_title[30];
	extern Changetitle();

	/* get out of security mode */

	if (screen->grabbedKbd) {
		XUngrabKeyboard(screen->display, CurrentTime);
		screen->grabbedKbd = FALSE;
		Changetitle (title);
		if (term->flags & REVERSE_VIDEO)
		{
		    term->flags ^= REVERSE_VIDEO;
		    ReverseVideo(term);
		    UpdateProperty (REVERSE_VIDEO_PROP, FALSE);
		}
	}

	/* try to get terminal into security mode	*/

	else {
		if (XGrabKeyboard(screen->display,
/* FLH dynamic
 *
 *	term is now 2 levels below shell
 */
		    VShellWindow, True, GrabModeAsync,
		    GrabModeAsync, CurrentTime) != GrabSuccess) {
/* FLH dynamic */
			XBell(screen->display, 100);
			screen->grabbedKbd = FALSE;
		    	UpdateProperty (SECURE_KBD_PROP, FALSE);
		}
		else {
			screen->grabbedKbd = TRUE;
			if (title == NULL)
			    title = strdup(xterm_name);
/* mlp - is this the correct solution here??? */
#if !defined(I18N)
			sprintf(new_title, "%s: Secure Keyboard", title);
#else
			sprintf(new_title, 
				OlGetMessage(screen->display, NULL, 0,
					OleNtitle, OleTsecureKbd,
					OleCOlClientXtermMsgs, NULL,
					NULL), title);
#endif
			Changetitle (new_title);
			XBell(screen->display, 100);
			if (!(term->flags & REVERSE_VIDEO))
			{
		    	    term->flags ^= REVERSE_VIDEO;
		    	    ReverseVideo(term);
		    	    UpdateProperty (REVERSE_VIDEO_PROP, TRUE);
			}
/* FLH mouseless XSetInputFocus --> OlSetInputFocus */
			OlSetInputFocus ((Widget) term, RevertToParent, CurrentTime);
			XRaiseWindow(XtDisplay(term), XtWindow(term));
		}
	}
}
#endif /* SECURE_KEYBOARD */


/* FLH resize */
/* MMENU_RESIZE:	callback for applying change to "Curses resize"
 *					item in properties sheet
 */
static void mmenu_resize()
{
	/* property is set to allow resize: change it so resize is not  */
	/* allowed, and if curses application is running set the hints  */
	/* not to allow the resize					*/

	if (term->misc.allow_resize) {
	    term->misc.allow_resize = FALSE;
	    if (term->screen.in_curses) 
	        disable_resize();
	}

	/* property is set not to allow resize: change it so resize is  */
	/* allowed, and if curses application is running set the hints  */
	/* to allow the resize						*/

	else {
	    term->misc.allow_resize = TRUE;
	    if (term->screen.in_curses)
	        enable_resize();
	}
}
/* FLH resize-end */

#ifdef XTERM_COMPAT
static void mmenu_repeat()
{
	register TScreen *screen = &term->screen;

	term->flags ^= AUTOREPEAT;
	if (term->flags & AUTOREPEAT)
		XAutoRepeatOn(screen->display);
	else
		XAutoRepeatOff(screen->display);

}

static void mmenu_input_scroll()
{
	register TScreen *screen = &term->screen;

	screen->scrollinput = !screen->scrollinput;
}

static void mmenu_key_scroll()
{
	register TScreen *screen = &term->screen;

	screen->scrollkey = !screen->scrollkey;
}
#endif /* XTERM_COMPAT */


/******* edit menu callback routines	*******/

/* ARGSUSED */
void
edit_send(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	extern   int CopyText();
	extern   Boolean Have_to_paste;

	/* we cannot call CopyText() followed by PasteText(), because */
	/* in some cases ReadClipboard() could get executed before    */
	/* ReadPrimary().  Now PasteText will get called at the end   */
	/* of ReadPrimary().					      */

	Have_to_paste = TRUE;
	CopyText();
}

/* ARGSUSED */
void
edit_paste(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	extern   int PasteText();

	PasteText();
}

/* ARGSUSED */
void
edit_copy(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	extern   int CopyText();

	CopyText();
}

#ifdef XTERM_CUT
/* ARGSUSED */
void
edit_cut(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	extern   int CutText();

	CutText();
}
#endif


static void
Prop_Changed(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{

		/*
		 *	change olwm label from DISMISS to CANCEL if not already changed
		 */
	if (!prop_changed){
		XtVaSetValues(prop_shell, XtNmenuType, (XtArgVal) OL_MENU_CANCEL, (String) 0);
		prop_changed = TRUE;
	}
}

static void (*call_backs[])() = { mmenu_visualbell, mmenu_log, mmenu_scroll,
				  mmenu_video, mmenu_wrap, mmenu_reversewrap,
				  mmenu_nlm, mmenu_cursor, mmenu_pad,
				  mmenu_scrollbar, mmenu_margbell
#ifdef SECURE_KEYBOARD
				  ,mmenu_securekeys
#endif /* SECURE_KEYBOARD */

/* FLH resize */		  ,mmenu_resize
#ifdef XTERM_COMPAT
			        , mmenu_repeat, mmenu_input_scroll,
			      	  mmenu_key_scroll
#endif /* XTERM_COMPAT */
			     };


void
SetUpProperties()
{
	register TScreen *screen = &term->screen;
	register int i = 0;
	Pixel foreground, background;
	Arg args[15];
	void InitProperties();

#if !defined(I18N)
	static char names[NUM_PROP][NAME_LEN] =
	     {"Visual Bell", "Logging", "Jump Scroll", "Reverse Video",
	      "Auto Wraparound", "Reverse Wraparound", "Auto Linefeed",
	      "Application Cursor", "Application Pad", "Scroll Bar",
	      "Margin Bell",
#ifdef SECURE_KEYBOARD
		  "Secure Keyboard", 
#endif /* SECURE_KEYBOARD */		
		  "Curses Resize"
#ifdef XTERM_COMPAT
	      , "Auto Repeat", "Scroll on key", "Scroll on input"
#endif /* XTERM_COMPAT */
	     };
#else
	static char names[NUM_PROP][NAME_LEN];
	static char types[NUM_PROP][NAME_LEN] =
	      {OleTvisualBell, OleTlogging, OleTjumpScroll, OleTreverseVideo, 
	       OleTautoWrap,   OleTreverseWrap, OleTautoLf, OleTappCursor, 
	       OleTappPad,     OleTscrollbar,   OleTmarginBell,
#ifdef SECURE_KEYBOARD		   
	       OleTsecureKbd,  
#endif /* SECURE_KEYBOARD */
	       OleTcursesResize
#ifdef XTERM_COMPAT
	       , OleTautoRepeat, OleTscrollonKey, OleTscrollonInput
#endif /* XTERM_COMPAT */
	      };

	static char def_msg[NUM_PROP][NAME_LEN] =
	       {OleMnames_visualBell, OleMnames_logging, OleMnames_jumpScroll,
		OleMnames_reverseVideo, OleMnames_autoWrap,
		OleMnames_reverseWrap, OleMnames_autoLf, OleMnames_appCursor,
		OleMnames_appPad, OleMnames_scrollbar, OleMnames_marginBell,
#ifdef SECURE_KEYBOARD		    
		OleMnames_secureKbd, 
#endif /* SECURE_KEYBOARD */
		OleMnames_cursesResize
#ifdef XTERM_COMPAT
		, OleMnames_autoRepeat, OleMnames_scrollonKey,
		OleMnames_scrollonInput
#endif /* XTERM_COMPAT */
	       };
#endif /* I18N */

        static Arg query[] = {
                {XtNupperControlArea, (XtArgVal) &prop_menu}
        };

	static XtCallbackRec applycalls[] = {
		{ ApplyProperty, NULL },
		{ NULL, NULL }
	};
 
	static XtCallbackRec resetcalls[] = {
		{ ResetProperty, NULL },
		{ NULL, NULL }
	};

	static XtCallbackRec verifycalls[] = {
		{ VerifyProperty, NULL },
		{ NULL, NULL }
	};

	static Arg targs[] = {
	    {XtNtitle, NULL},
	};

	char *xterm_title;

	targs[0].value = (XtArgVal) &xterm_title;
/*	FLH dynamic
 *
 *	term is now 2 levels below shell widget
 *
 */
	XtGetValues (VShellWidget, targs, 1);
/* FLH dynamic */

	i = 0;
	XtSetArg(args[i], XtNapply, (XtArgVal) applycalls);     i++;
	XtSetArg(args[i], XtNreset, (XtArgVal) resetcalls);	i++;
	XtSetArg(args[i], XtNverify, (XtArgVal) verifycalls);	i++;
       	XtSetArg(args[i], XtNtitle, (XtArgVal) xterm_title);	i++;
       	XtSetArg(args[i], XtNgeometry, (XtArgVal) NULL);	i++;
	prop_shell = XtCreatePopupShell("prop_shell",
		     		popupWindowShellWidgetClass, (Widget) term, args, i);
	XtAddCallback (prop_shell, XtNpopdownCallback, ResetProperty, NULL);

	/* if menu has already been created, we don't have to do the */
	/* following call again					     */

        XtGetValues(prop_shell, query, XtNumber(query));

#if defined(I18N)
	/* initialize check_items */

	for (i=0; i<NUM_PROP; i++) {
	     OlGetMessage(screen->display, names[i], NAME_LEN, OleNcheckbox,
			  types[i], OleCOlClientXtermMsgs, def_msg[i], NULL);
	}
#endif /* I18N */

	for (i=0; i<NUM_PROP; i++) {
	     check_items[i].label = (XtArgVal) names[i];
	     check_items[i].select = check_items[i].unselect = (XtArgVal) Prop_Changed;
	     /* don't bother setting state: it will be set by InitProperties */
	     check_items[i].sensitive = (XtArgVal) TRUE;
	     check_items[i].ljustify  = (XtArgVal) OL_LEFT;
	}

	i = 0;
	XtSetArg(args[i], XtNitems,	(XtArgVal) check_items);	    i++;
	XtSetArg(args[i], XtNnumItems,	(XtArgVal) XtNumber(check_items));  i++;
	XtSetArg(args[i], XtNitemFields,(XtArgVal) check_fields);	    i++;
	XtSetArg(args[i], XtNnumItemFields, (XtArgVal) XtNumber(check_fields)); i++;
        XtSetArg(args[i], XtNlayoutType, (XtArgVal) OL_FIXEDCOLS);	  i++;
        XtSetArg(args[i], XtNmeasure,    (XtArgVal) 2 );	  	  i++;
	XtSetArg(args[i], XtNbuttonType, (XtArgVal) OL_CHECKBOX );	  i++;
	XtSetArg(args[i], XtNexclusives, (XtArgVal) False );	  	  i++;

	flat_checkbox = XtCreateManagedWidget("checkbox",
					      flatButtonsWidgetClass,
                           		      prop_menu, args, i);
	InitProperties();
	screen->property = prop_shell;
	
	/*
	 * Add an event handler to intercept the help ClientMessage
	 * and send a "forged" help key to the application instead
	 * of bringing up help.
	 */
	XtInsertRawEventHandler(prop_shell, NoEventMask, True, HandleHelpMessage, 
				(XtPointer) term,
				XtListHead);
}

void
InitProperties()
{
	register TScreen *screen = &term->screen;
	register int k;
	static Arg args[] = { {XtNitemsTouched, (XtArgVal) TRUE} };

	prop_state[VISUALBELL_PROP]    = (term->screen.visualbell) ? TRUE : FALSE;
	prop_state[LOGGIN_PROP]        = (screen->logging) ? TRUE : FALSE;
	prop_state[JUMPSCROLL_PROP]    = (term->flags & SMOOTHSCROLL) ? FALSE : TRUE;
	prop_state[REVERSE_VIDEO_PROP] = ((term->flags & REVERSE_VIDEO)) ? TRUE : FALSE;
	prop_state[WRAPAROUND_PROP]    = ((term->flags & WRAPAROUND)) ? TRUE : FALSE;
	prop_state[REVERSEWRAP_PROP]   = ((term->flags & REVERSEWRAP)) ? TRUE : FALSE;
	prop_state[NEWLINEMAP_PROP]    = ((term->flags & LINEFEED)) ? TRUE : FALSE;
	prop_state[APL_CURSOR_PROP]    = ((term->flags & CURSOR_APL)) ? TRUE : FALSE;
	prop_state[APL_KEYPAD_PROP]    = ((term->flags & KYPD_APL)) ? TRUE : FALSE;
	prop_state[SCROLLBAR_PROP]     = (screen->scrollbar) ? TRUE : FALSE;
	prop_state[MARGINBELL_PROP]    = (screen->marginbell) ? TRUE : FALSE;

#ifdef SECURE_KEYBOARD
	prop_state[SECURE_KBD_PROP]    = (screen->grabbedKbd) ? TRUE : FALSE;
#endif /* SECURE_KEYBOARD */

/* FLH resize */
	prop_state[RESIZE_PROP]        = (term->misc.allow_resize) ? TRUE : FALSE;
/* FLH resize-end */

#ifdef XTERM_COMPAT
	prop_state[AUTOREPEAT_PROP]    = (term->flags & AUTOREPEAT) ? TRUE : FALSE;
	prop_state[INPUT_SCROLL_PROP]  = (screen->scrollinput) ? TRUE : FALSE;
	prop_state[KEY_SCROLL_PROP]    = (screen->scrollkey) ? TRUE : FALSE;
#endif /* XTERM_COMPAT */

	for (k=0; k<NUM_PROP; k++)
	{
	    if (prop_state[k])
	        check_items[k].state = TRUE;
	    else
	        check_items[k].state = FALSE;
	}
        XtSetValues(flat_checkbox, args, 1);
}


void
UpdateProperty (prop_num, state)
int     prop_num;
Boolean state;
{
	static Arg args[] = { {XtNitemsTouched, (XtArgVal) TRUE} };

	check_items[prop_num].state = prop_state[prop_num] = state;
        XtSetValues(flat_checkbox, args, 1);
}


static void
ApplyProperty(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	if (prop_changed)
	{
	    Arg args[1];
	    register int i;

	    for (i=0; i<NUM_PROP; i++)
	    {
	         if (prop_state[i] != check_items[i].state) 
	         {
	             prop_state[i] = !prop_state[i];
		     (*call_backs[i])();
	         }
	    }

            XtSetArg(args[0], XtNpropertyChange, (XtArgVal) FALSE);
	    XtSetValues (prop_shell, args, 1);
	    prop_changed = FALSE;
	}
		/*
		 *	restore olwm menu button CANCEL --> DISMISS
		 */
	XtVaSetValues(prop_shell, XtNmenuType, (XtArgVal) OL_MENU_LIMITED, (String) 0);
	popdown = TRUE;
}


static void
ResetProperty(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	if (prop_changed)
	{
	    static Arg args[] = { {XtNitemsTouched, (XtArgVal) TRUE} };
	    register int i;

	    for (i=0; i<NUM_PROP; i++)
		 check_items[i].state = prop_state[i];
            XtSetValues(flat_checkbox, args, 1);
			/*
			 *	change olwm menu label CANCEL-->DISMISS
			 */
		XtVaSetValues (prop_shell, XtNmenuType, (XtArgVal)OL_MENU_LIMITED, (String)0);
	}
	popdown = FALSE;
}


static void
VerifyProperty(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	Boolean * ok = (Boolean *) call_data;

	if (ok)
		*ok = popdown;
}

#define GETLABEL(a,b)	OlGetMessage(screen->display, NULL, 0, OleNlabel,\
			  a, OleCOlClientXtermMsgs, b, NULL);
#define GETMNEM(a,b)	OlGetMessage(screen->display, NULL, 0, OleNmnemonic,\
			  a, OleCOlClientXtermMsgs, b, NULL)

void SetUpMenu()
{
	Arg args[10];
	register int i;
	Pixel foreground, background;
	TScreen *screen = &term->screen;

	String edit_label;
	char   *xterm_title;

	args[0].name  = XtNtitle;
	args[0].value = (XtArgVal)&xterm_title;
	

/* FLH dynamic
 *
 *	term is now 2 levels below shell widget 
 */
	XtGetValues (VShellWidget, args, 1);
/* FLH dynamic */

	i = 0;
	XtSetArg(args[i], XtNtitle, (XtArgVal) xterm_title);    i++;
	XtSetArg(args[i], XtNhasTitle, (XtArgVal) True);	i++;
	XtSetArg(args[i], XtNmenuAugment, (XtArgVal) FALSE);	i++;
	XtSetArg(args[i], XtNpushpin, (XtArgVal) OL_NONE);	i++;

	/* create menu shell and associated control area	*/

	screen->menuWidget = XtCreatePopupShell("menu_shell",
				popupMenuShellWidgetClass, toplevel, args, i);

	/* set up menu labels and mnemonics*/

	i = 0;
	edit_label = GETLABEL(OleTedit,OleMlabel_edit)
	xterm_b[i].label = (XtArgVal) edit_label;
	xterm_b[i++].mnemonic = (XtArgVal)*(GETMNEM(OleTedit,OleMmnemonic_edit));

	xterm_b[i].label = (XtArgVal)GETLABEL(OleTredraw,OleMlabel_redraw)
	xterm_b[i++].mnemonic = (XtArgVal)*(GETMNEM(OleTredraw,OleMmnemonic_redraw));

	xterm_b[i].label = (XtArgVal)GETLABEL(OleTsoftReset,OleMlabel_softReset)
	xterm_b[i++].mnemonic = (XtArgVal)*(GETMNEM(OleTsoftReset,OleMmnemonic_softReset));

	xterm_b[i].label = (XtArgVal)GETLABEL(OleTfullReset,OleMlabel_fullReset)
	xterm_b[i++].mnemonic = (XtArgVal)*(GETMNEM(OleTfullReset,OleMmnemonic_fullReset));

	xterm_b[i].label = (XtArgVal)GETLABEL(OleTproperties,OleMlabel_properties)
	xterm_b[i++].mnemonic = (XtArgVal)*(GETMNEM(OleTproperties,OleMmnemonic_properties));

#ifdef TEK
	xterm_b[i].label = (XtArgVal)GETLABEL(OleTshowTekWin,OleMlabel_showTek)
	xterm_b[i++].mnemonic = (XtArgVal)*(GETMNEM(OleTshowTekWin,OleMmnemonic_showTek));
#endif /* TEK */

	xterm_b[i].label = (XtArgVal)GETLABEL(OleTinterrupt,OleMlabel_interrupt)
	xterm_b[i++].mnemonic = (XtArgVal)*(GETMNEM(OleTinterrupt,OleMmnemonic_interrupt));

	xterm_b[i].label = (XtArgVal)GETLABEL(OleThangup,OleMlabel_hangup)
	xterm_b[i++].mnemonic = (XtArgVal)*(GETMNEM(OleThangup,OleMmnemonic_hangup));

	xterm_b[i].label = (XtArgVal)GETLABEL(OleTterminate,OleMlabel_terminate)
	xterm_b[i++].mnemonic = (XtArgVal)*(GETMNEM(OleTterminate,OleMmnemonic_terminate));

	xterm_b[i].label = (XtArgVal)GETLABEL(OleTkill,OleMlabel_kill)
	xterm_b[i++].mnemonic = (XtArgVal)*(GETMNEM(OleTkill,OleMmnemonic_kill));

	i = 0;
        XtSetArg(args[i], XtNlayoutType, (XtArgVal) OL_FIXEDCOLS);	i++;
	XtSetArg(args[i], XtNmeasure, (XtArgVal) 1);			i++;
	XtSetArg(args[i], XtNitemFields, (XtArgVal) menuFields);	i++;
	XtSetArg(args[i], XtNnumItemFields, (XtArgVal) XtNumber (menuFields));
		i++;
	XtSetArg(args[i], XtNitems, (XtArgVal) xterm_b);		i++;
	XtSetArg(args[i], XtNnumItems, (XtArgVal) XtNumber (xterm_b));	i++;
	menu_pane = XtCreateManagedWidget ("menu_pane",
                                      flatButtonsWidgetClass,
                                      screen->menuWidget,
                                      args, i);
	/* create edit menu */

        i = 0;
	XtSetArg(args[i], XtNtitle, (XtArgVal) edit_label);  i++;
	XtSetArg(args[i], XtNhasTitle, (XtArgVal) True);	i++;
	XtSetArg(args[i], XtNpushpin, (XtArgVal) OL_OUT);	i++;

        xterm_b[VT_EDIT].submenu = (XtArgVal) XtCreatePopupShell("edit_shell",
                                popupMenuShellWidgetClass, menu_pane, args, i);

	/* set up edit menu labels and mnemonics */


	edit_b[0].label = (XtArgVal)GETLABEL(OleTsend,OleMlabel_send)
	edit_b[0].mnemonic = (XtArgVal)*(GETMNEM(OleTsend,OleMmnemonic_send));

	edit_b[1].label = (XtArgVal)GETLABEL(OleTpaste,OleMlabel_paste)
	edit_b[1].mnemonic = (XtArgVal)*(GETMNEM(OleTpaste,OleMmnemonic_paste));

	edit_b[2].label = (XtArgVal)GETLABEL(OleTcopy,OleMlabel_copy)
	edit_b[2].mnemonic = (XtArgVal)*(GETMNEM(OleTcopy,OleMmnemonic_copy));

#ifdef XTERM_CUT
	edit_b[3].label = 
		(XtArgVal)GETLABEL(OleTcut,OleMlabel_cut)
	edit_b[3].mnemonic =
		(XtArgVal)*(GETMNEM(OleTcut,OleMmnemonic_cut));
#endif

        i = 0;
	XtSetArg(args[i], XtNlayoutType, (XtArgVal) OL_FIXEDCOLS);	i++;
	XtSetArg(args[i], XtNmeasure, (XtArgVal) 1);			i++;
	XtSetArg(args[i], XtNitemFields, (XtArgVal) menuFields);	i++;
	XtSetArg(args[i], XtNnumItemFields, (XtArgVal) XtNumber (menuFields));
		i++;
	XtSetArg(args[i], XtNitems, (XtArgVal) edit_b);			i++;
	XtSetArg(args[i], XtNnumItems, (XtArgVal) XtNumber (edit_b));	i++;
	(void) XtCreateManagedWidget ("edit_pane",
                                      flatButtonsWidgetClass,
                                      (Widget) xterm_b[VT_EDIT].submenu,
                                      args, i);
	/*
	 * Add an event handler to intercept the help ClientMessage
	 * and send a "forged" help key to the application instead
	 * of bringing up help.
	 */
	XtInsertRawEventHandler((Widget) xterm_b[VT_EDIT].submenu, NoEventMask, True, 
				HandleHelpMessage, (XtPointer) term, XtListHead);
}


VTReset(full, menu)
int full;
register Widget menu;
{
	register TScreen *screen = &term->screen;
	extern jmp_buf vtjmpbuf;

	/* reset scrolling region */
	screen->top_marg = 0;
	screen->bot_marg = screen->max_row;
	term->flags &= ~ORIGIN;
	if(full) {
		extern Pixel textFG, textBG;

		TabReset (term->tabs);
		term->keyboard.flags = NULL;
		screen->gsets[0] = 'B';
		screen->gsets[1] = 'B';
		screen->gsets[2] = 'B';
		screen->gsets[3] = 'B';
		screen->curgl = 0;
		screen->curgr = 2;
		screen->curss = 0;
		ClearScreen(screen);
		screen->cursor_state = OFF;
#ifdef XTERM_COMPAT
		if(!(term->flags & AUTOREPEAT))
			XAutoRepeatOn(screen->display);
#endif
		if (term->flags & REVERSE_VIDEO)
		    ReverseVideo(term);

		textFG = screen->foreground;
		textBG = screen->background;

		term->flags = term->initflags;
#ifndef SYSV
		if(screen->c132 && (term->flags & IN132COLUMNS)) {
		        Dimension junk;
			XtMakeResizeRequest(
			    (Widget) term,
			    (unsigned) 80*FontWidth(screen)
				+ 2 * screen->border,
			    (unsigned) FontHeight(screen)
			        * (screen->max_row + 1) + 2 * screen->border
					&junk, &junk);
			XSync(screen->display, FALSE);	/* synchronize */
			if(QLength(screen->display) > 0)
				xevents();
		}
#endif
		CursorSet(screen, 0, 0, term->flags);
	}
/* SS-menu */
	if (screen->property)
	    InitProperties();

        if (menu != (Widget) NULL)
            OlUnpostPopupMenu (menu);

/* SS-menu-end */
	longjmp(vtjmpbuf, 1);	/* force ground state in parser */
}



#ifdef TEK

#define	TEKHOME		((TekChar[screen->page.fontsize].nlines - 1)\
			 * TekChar[screen->page.fontsize].vsize)

static struct Tek_Char {
	int hsize;	/* in Tek units */
	int vsize;	/* in Tek units */
	int charsperline;
	int nlines;
} TekChar[TEKNUMFONTS] = {
	{56, 88, 74, 35},	/* large */
	{51, 82, 81, 38},	/* #2 */
	{34, 53, 121, 58},	/* #3 */
	{31, 48, 133, 64},	/* small */
};

static void	TApplyProperty(), TResetProperty(), TVerifyProperty();

/* static Tmodes curmodes; */

/****** Tektronix menu call back routines	*******/

/* ARGSUSED */
static void
Tmenu_reset(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	register TScreen *screen = &term->screen;

	bzero((char *)&screen->cur, sizeof(Tmodes));
	TekRefresh = (TekLink *)0;
	if (screen->Tproperty)
	    TUpdateProperty(screen->cur.fontsize, 0);
	/* screen->cur = curmodes; */
	TekPage();
	screen->cur_X = 0;
	screen->cur_Y = TEKHOME;
}


/* ARGSUSED */
static void
Tmenu_page(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	register TScreen *screen = &term->screen;

	XClearWindow(screen->display, TWindow(screen));
	Tmenu_reset(w, client_data, call_data);
}


/* ARGSUSED */
static void
Tmenu_copy(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	TekCopy();
}

/* ARGSUSED */
void
Tmenu_redraw(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	TRedraw();
}

/* ARGSUSED */
static void
Tmenu_property(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	register TScreen *screen = &term->screen;
	void     TSetUpProperties();

	if (screen->Tproperty == NULL)
	    TSetUpProperties();
	XtPopup(Tprop_shell, XtGrabNone);
	XRaiseWindow(XtDisplayOfObject(w), XtWindowOfObject(w));
}


/* ARGSUSED */
static void
Tmenu_hide_vt(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	register TScreen *screen = &term->screen;

	if (!screen->Vshow) {
	    XWindowAttributes       win_attrs;

	    if (screen->TmenuWidget) {
#if !defined(I18N)
		Txterm_b[TEK_HIDE_VT_WIN].label = (XtArgVal) "Hide VT window";
		Txterm_b[TEK_HIDE_VT_WIN].mnemonic = (XtArgVal) 'd';
#else
		static char *label = NULL;

		if (label == NULL)
		    label = OlGetMessage(screen->display, NULL, 0, OleNlabel,
					OleThideVt, OleCOlClientXtermMsgs,
					OleMlabel_hideVt, NULL);
                Txterm_b[TEK_HIDE_VT_WIN].label = (XtArgVal)label;
#endif
		XtSetValues (Tmenu_pane, touchArg, 1);
	    }
	    set_vt_visibility (TRUE);
	    end_tek_mode (w);

	    XGetWindowAttributes (screen->display, VWindow(screen),                                               &win_attrs);

/* FLH dynamic
 *
 * term is now 2 levels below the shell
 */
	    if (win_attrs.map_state != IsUnviewable)
	    {
			 OlSetInputFocus ((Widget) term, 
									RevertToParent, CurrentTime);
		    XRaiseWindow(XtDisplay(term), XtWindow(term));
	    }
/* FLH dynamic */
	} else {
	    if (screen->TmenuWidget) {
#if !defined(I18N)
		Txterm_b[TEK_HIDE_VT_WIN].label = (XtArgVal) "Show VT window";
		Txterm_b[TEK_HIDE_VT_WIN].mnemonic = (XtArgVal) 'o';
		XtSetValues (Tmenu_pane, touchArg, 1);
#else
		static char *label = NULL;

		if (label == NULL)
		    label = OlGetMessage(screen->display, NULL, 0, OleNlabel,
					OleTshowVt, OleCOlClientXtermMsgs,
					OleMlabel_showVt, NULL);
                Txterm_b[TEK_HIDE_VT_WIN].label = (XtArgVal)label;
#endif
                XtSetValues (Tmenu_pane, touchArg, 1);
	    }
	    set_vt_visibility (FALSE);
	    end_vt_mode (w);
	}
}

/**** Tektronix property callback routines ****/

static char *changesize[] = {
	"\0338",
	"\0339",
	"\033:",
	"\033;",
};


static void
Tprop_fonts(new_font)
int  new_font;
{
	register Char *tp;
	register Char *fp;

	if(!Ttoggled) {
		TCursorToggle(TOGGLE);
		Ttoggled = TRUE;
	}
	if(Tbcnt < 0)
		Tbcnt = 0;
	for(fp = (Char *) changesize[new_font], tp = &Tbptr[Tbcnt] ; *fp ;)
	{
		*tp++ = *fp++;
		Tbcnt++;
	}
}


void TSetUpMenu()
{
	Arg args[10];
	register int i;
	Pixel foreground, background;
	TScreen *screen = &term->screen;
	extern Widget toplevel;

	static Arg targs[] = {
	    {XtNtitle, NULL},
	};

	char *tek_title;

	targs[0].value = (XtArgVal)&tek_title;
	XtGetValues (tekWidget->core.parent, targs, 1);

	/* create menu shell */

      	i = 0;
      	XtSetArg(args[i], XtNlayout, (XtArgVal) OL_FIXEDCOLS);  i++;
      	XtSetArg(args[i], XtNmeasure, (XtArgVal) 1);        	i++;
      	XtSetArg(args[i], XtNmenuAugment, (XtArgVal) FALSE);    i++;
      	XtSetArg(args[i], XtNtitle, (XtArgVal) tek_title);      i++;


	screen->TmenuWidget = XtCreatePopupShell("Tmenu_shell",
				popupMenuShellWidgetClass, toplevel, args, i);
#if !defined(I18N)
	if (!screen->Vshow) {
            xterm_b[TEK_HIDE_VT_WIN].label = (XtArgVal) "Show VT window";
            xterm_b[TEK_HIDE_VT_WIN].mnemonic = (XtArgVal) 'o';
	}
#endif

	if (!(screen->inhibit & I_SIGNAL))
	{
            /* This assumes that all the signal buttons are consecutively
             * numbered with the interrupt button lowest.
             */
            for (i=TEK_INTERRUPT; i<=TEK_KILL; i++)
		Txterm_b[i].sensitive = (XtArgVal) False;
	}

	i = 0;
	XtSetArg(args[i], XtNtitle, (XtArgVal) tek_title);	i++;
	XtSetArg(args[i], XtNhasTitle, (XtArgVal) True);	i++;
	XtSetArg(args[i], XtNpushpin, (XtArgVal) OL_NONE);	i++;

        screen->TmenuWidget = XtCreatePopupShell("Tmenu_shell",
                                popupMenuShellWidgetClass, toplevel, args, i);

	/* menu labels, mnemonics */
	Txterm_b[0].label =
		(XtArgVal)GETLABEL(OleTpage,OleMlabel_page)
	Txterm_b[0].mnemonic = 
		(XtArgVal)*(GETMNEM(OleTpage,OleMmnemonic_page));

	Txterm_b[1].label =
		(XtArgVal)GETLABEL(OleTreset,OleMlabel_reset)
	Txterm_b[1].mnemonic = 
		(XtArgVal)*(GETMNEM(OleTreset,OleMmnemonic_reset));

	Txterm_b[2].label =
		(XtArgVal)GETLABEL(OleTcopy2,OleMlabel_copy2)
	Txterm_b[2].mnemonic = 
		(XtArgVal)*(GETMNEM(OleTcopy2,OleMmnemonic_copy2));

	Txterm_b[3].label =
		(XtArgVal)GETLABEL(OleTredraw,OleMlabel_redraw)
	Txterm_b[3].mnemonic = 
		(XtArgVal)*(GETMNEM(OleTredraw,OleMmnemonic_redraw));

	Txterm_b[4].label =
		(XtArgVal)GETLABEL(OleTproperties,OleMlabel_properties)
	Txterm_b[4].mnemonic = 
		(XtArgVal)*(GETMNEM(OleTproperties,OleMmnemonic_properties));

	Txterm_b[5].label =
		(XtArgVal)GETLABEL(OleThideVt,OleMlabel_hideVt)
	Txterm_b[5].mnemonic = 
		(XtArgVal)*(GETMNEM(OleThideVt,OleMmnemonic_hideVt));

	Txterm_b[6].label =
		(XtArgVal)GETLABEL(OleTinterrupt,OleMlabel_interrupt)
	Txterm_b[6].mnemonic = 
		(XtArgVal)*(GETMNEM(OleTinterrupt,OleMmnemonic_interrupt));

	Txterm_b[7].label =
		(XtArgVal)GETLABEL(OleThangup,OleMlabel_hangup)
	Txterm_b[7].mnemonic = 
		(XtArgVal)*(GETMNEM(OleThangup,OleMmnemonic_hangup));

	Txterm_b[8].label =
		(XtArgVal)GETLABEL(OleTterminate,OleMlabel_terminate)
	Txterm_b[8].mnemonic = 
		(XtArgVal)*(GETMNEM(OleTterminate,OleMmnemonic_terminate));

	Txterm_b[9].label =
		(XtArgVal)GETLABEL(OleTkill,OleMlabel_kill)
	Txterm_b[9].mnemonic = 
		(XtArgVal)*(GETMNEM(OleTkill,OleMmnemonic_kill));


        i = 0;
	XtSetArg(args[i], XtNlayoutType, (XtArgVal) OL_FIXEDCOLS);	i++;
	XtSetArg(args[i], XtNmeasure, (XtArgVal) 1);			i++;
	XtSetArg(args[i], XtNitemFields, (XtArgVal) menuFields);	i++;
	XtSetArg(args[i], XtNnumItemFields, (XtArgVal) XtNumber (menuFields));
		i++;
	XtSetArg(args[i], XtNitems, (XtArgVal) Txterm_b);		i++;
	XtSetArg(args[i], XtNnumItems, (XtArgVal) XtNumber (Txterm_b)); i++;
	Tmenu_pane = XtCreateManagedWidget ("Tmenu_pane",
                                      flatButtonsWidgetClass,
                                      screen->TmenuWidget,
                                      args, i);
}


static void
TProp_Changed(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	if (!Tprop_changed){
			/*
			 *	Change olwm menu label, DISMISS --> CANCEL
			 */
		XtVaSetValues(Tprop_shell, XtNmenuType, (XtArgVal)OL_MENU_CANCEL, 
							(String) 0);
		Tprop_changed = TRUE;
	}
}


void
TSetUpProperties()
{
	register TScreen *screen = &term->screen;
	register int i = 0;
	Pixel foreground, background;
	Arg args[10];
	void TInitProperties();

#if !defined(I18N)
	static char names[TNUM_PROP][TNAME_LEN] =
	     {"Large Characters", "Medium Characters",
	      "Small Characters", "Tiny Characters" };
#else
	static char names[TNUM_PROP][TNAME_LEN];
	static char types[TNUM_PROP][TNAME_LEN] =
				{ OleTlargeChar, OleTmediumChar,
				  OleTsmallChar, OleTtinyChar};
	static char def_msg[TNUM_PROP][TNAME_LEN] =
		{ OleMexcl_largeChar, OleMexcl_mediumChar,
		  OleMexcl_smallChar, OleMexcl_tinyChar };
#endif

        static Arg query[] = {
                {XtNupperControlArea, (XtArgVal) &Tprop_menu}
        };

	static XtCallbackRec applycalls[] = {
		{ TApplyProperty, NULL },
		{ NULL, NULL }
	};
 
	static XtCallbackRec resetcalls[] = {
		{ TResetProperty, NULL },
		{ NULL, NULL }
	};

	static XtCallbackRec verifycalls[] = {
		{ TVerifyProperty, NULL },
		{ NULL, NULL }
	};


	static Arg targs[] = {
	    {XtNtitle, NULL},
	};

	char *tek_title;

	targs[0].value = (XtArgVal)&tek_title;
	XtGetValues (tekWidget->core.parent, targs, 1);

	i = 0;
	XtSetArg(args[i], XtNapply, (XtArgVal) applycalls);	i++;
	XtSetArg(args[i], XtNreset, (XtArgVal) resetcalls);	i++;
	XtSetArg(args[i], XtNverify, (XtArgVal) verifycalls);	i++;
	XtSetArg(args[i], XtNtitle, (XtArgVal) tek_title);	i++;
       	XtSetArg(args[i], XtNgeometry, (XtArgVal) NULL);	i++;
	Tprop_shell = XtCreatePopupShell("Tprop_shell",
		     popupWindowShellWidgetClass, tekWidget, args, i);
	XtAddCallback (Tprop_shell, XtNpopdownCallback, TResetProperty, NULL);

	/* if menu has already been created, we don't have to do the */
	/* following call again					     */

        XtGetValues(Tprop_shell, query, XtNumber(query));

#if defined(I18N) /* initialize check_items */

	for (i=0; i<TNUM_PROP; i++) {
	     OlGetMessage(screen->display, names[i], TNAME_LEN, OleNexcl,
			  types[i], OleCOlClientXtermMsgs, def_msg[i], NULL);
	}
#endif /* I18N */

	for (i=0; i<TNUM_PROP; i++)
	{
	     excl_items[i].label = (XtArgVal) names[i];
	     excl_items[i].select = check_items[i].unselect = (XtArgVal) TProp_Changed;
	     /* don't bother setting state: it will be set by TInitProperties */
	     excl_items[i].ljustify  = (XtArgVal) OL_LEFT;
	}
	check_items[SECURE_KBD_PROP].sensitive = FALSE;


	i = 0;
	XtSetArg(args[i], XtNitems,    (XtArgVal) excl_items);            i++;
	XtSetArg(args[i], XtNnumItems, (XtArgVal) XtNumber(excl_items));  i++;
	XtSetArg(args[i], XtNitemFields,(XtArgVal) excl_fields);          i++;
	XtSetArg(args[i], XtNnumItemFields,
                                        (XtArgVal) XtNumber(excl_fields)); i++;

/* FLH dynamic */

	XtSetArg(args[i], XtNlayoutType, (XtArgVal) OL_FIXEDCOLS);       i++;
        XtSetArg(args[i], XtNmeasure,    (XtArgVal) 2 );                 i++;
	XtSetArg(args[i], XtNtitle, (XtArgVal) tek_title); 		 i++;
	XtSetArg(args[i], XtNbuttonType, (XtArgVal) OL_RECT_BTN);	 i++;
     	XtSetArg(args[i], XtNexclusives, (XtArgVal) True);		 i++;

	flat_excl = XtCreateManagedWidget("excl",
                                           flatButtonsWidgetClass,
                                           Tprop_menu, args, i);
	TInitProperties();
	screen->Tproperty = Tprop_shell;
}


void
TInitProperties()
{
	register int k;
	register TScreen *screen = &term->screen;
	static Arg args[] = { {XtNitemsTouched, (XtArgVal) TRUE} };

	for (k=0; k<TNUM_PROP; k++)
	{
	     if (k == screen->cur.fontsize) {
		 excl_items[k].state = Tprop_state[k] = TRUE;
	     }
	     else {
		 excl_items[k].state = Tprop_state[k] = FALSE;
	     }
	}
        XtSetValues(flat_excl, args, 1);
}

void
TUpdateProperty (old_font, new_font)
register int old_font, new_font;
{
	static Arg args[] = { {XtNitemsTouched, (XtArgVal) TRUE} };

	excl_items[old_font].state = Tprop_state[old_font] = FALSE;
	excl_items[new_font].state = Tprop_state[new_font] = TRUE;
        XtSetValues(flat_excl, args, 1);
}


static void
TApplyProperty(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	if (Tprop_changed)
	{
	    Arg args[1];
	    register int i;

	    for (i=0; i<TNUM_PROP; i++)
	    {
		 if (Tprop_state[i] != excl_items[i].state)
                 {
                     if (Tprop_state[i] = !Tprop_state[i])  /* YES, =, not == */
                         Tprop_fonts(i);
                 }
	    }

            XtSetArg(args[0], XtNpropertyChange, (XtArgVal) FALSE);
	    XtSetValues (Tprop_shell, args, 1);
	    Tprop_changed = FALSE;
		
			/*
			 *	change olwm menu label CANCEL-->DISMISS
			 */
		XtVaSetValues (Tprop_shell, XtNmenuType, (XtArgVal)OL_MENU_LIMITED, (String)0);
	}
	Tpopdown = TRUE;
}


static void
TResetProperty(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	if (Tprop_changed)
	{
	    register int i;
	    static Arg args[] = { {XtNitemsTouched, (XtArgVal) TRUE} };

	    for (i=0; i<TNUM_PROP; i++)
		 excl_items[i].state = Tprop_state[i];
            XtSetValues(flat_excl, args, 1);
			/*
			 *	change olwm menu label CANCEL-->DISMISS
			 */
		XtVaSetValues (Tprop_shell, XtNmenuType, (XtArgVal)OL_MENU_LIMITED, (String)0);
	}
	Tpopdown = FALSE;
}


static void
TVerifyProperty(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	Boolean * ok = (Boolean *) call_data;

	if (ok)
		*ok = Tpopdown;
}


void set_vt_visibility (on)
    Boolean on;
{
    register TScreen *screen = &term->screen;

    if (on) {
	if (!screen->Vshow && term) {
	    VTInit ();
/* FLH dynamic
 *
 * term is now 2 levels below the shell widget
 */
				/*
				 *	Check if shell is iconified.  We don't want to
				 * inadvertently open up an iconified xterm.
				 * This should only be a problem at startup. 
				 */
		 if (GetWMState(screen->display, VShellWindow) != IconicState)	
	    	XtMapWidget (VShellWidget);
/* FLH dynamic */
	    screen->Vshow = TRUE;
	}
    } else {
	if (screen->Vshow && term) {
/* FLH dynamic
 *
 * term is now 2 levels below the shell widget
 */
	    XtUnmapWidget (VShellWidget);
/* FLH dynamic */
	    if (screen->property)
		XtPopdown (screen->property);
	    screen->Vshow = FALSE;
	}
    }
}


void set_tek_visibility (on)
Boolean on;
{
    register TScreen *screen = &term->screen;

    if (on) {
	if (!screen->Tshow && (tekWidget || TekInit())) {
	    XtRealizeWidget (tekWidget->core.parent);
				/*
				 *	Check if shell is iconified.  We don't want to
				 * inadvertently open up an iconified xterm.
				 * This should only be a problem at startup. 
				 */
		 if (GetWMState(screen->display, XtWindowOfObject(tekWidget->core.parent))
				!= IconicState)	
	    	XtMapWidget (tekWidget->core.parent);

	    screen->Tshow = TRUE;
	}
    } else {
	if (screen->Tshow && tekWidget) {
	    XtUnmapWidget (tekWidget->core.parent);
	    if (screen->Tproperty)
		XtPopdown (screen->Tproperty);
	    screen->Tshow = FALSE;
	}
    }
}


void end_tek_mode (menu)
register Widget menu;
{
    register TScreen *screen = &term->screen;

		/* reset flag for last window given focus */
		/* focus_switched is used to determine if emulation mode should */
		/* be switched.  It is set in each widget's AcceptFocus proc */
	focus_switched = 0;
		/* clear flag that indicates that the Tek Expose routine
		 * is on the call stack before wiping it off.  This
		 * will prevent a longjmp() into oblivion in Tinput() (Tekproc.c)
		 */
	inside_TekExpose = 0;

    if (screen->TekEmu) {
	if (screen->logging) {
	    FlushLog (screen);
	    screen->logstart = buffer;
	}
        if (menu != (Widget) NULL)
            OlUnpostPopupMenu (menu);
	longjmp(Tekend, 1);
    } 
}

void end_vt_mode (menu)
register Widget menu;
{
    register TScreen *screen = &term->screen;

		/* reset flag for last window given focus */
		/* focus_switched is used to determine if emulation mode should */
		/* be switched.  It is set in each widget's AcceptFocus proc */
	focus_switched = 0;
		/* clear flag that indicates that the Tek Expose routine
		 * is on the call stack before wiping it off.  This
		 * will prevent a longjmp() into oblivion in Tinput() (Tekproc.c)
		 */
	inside_TekExpose = 0;

    if (!screen->TekEmu) {
	if(screen->logging) {
	    FlushLog(screen);
	    screen->logstart = Tbuffer;
	}
	screen->TekEmu = TRUE;
	if (menu != (Widget) NULL)
            OlUnpostPopupMenu(menu);
	longjmp(VTend, 1);
    } 
}
#endif /* TEK */

void
flatMenuPost (menu, window, event)
    Widget	menu;
    Window	window;
	 XEvent *event;
{
    int		root_x, root_y;
	 int		win_x, win_y;
	 int     x,y;
	 OlVirtualName activation_type;
	 Widget  client_shell = VShellWidget;


	 if (event-> type == ButtonPress){
			 /*
			  * mouse must be within window
			  * place menu at mouse position
			  */
			 x = event->xbutton.x_root; 
			 y = event->xbutton.y_root; 
			 activation_type = OL_MENU;
		 }
	 else{
			 /*
			  * KeyPress event
			  */
			 activation_type = OL_MENUKEY;
			 root_x = event->xbutton.x_root;
			 root_y = event->xbutton.y_root;
			 win_x = event->xbutton.x;
			 win_y = event->xbutton.y;
			 
			 /* 
			  * Is pointer inside window?
			  */
			 if ((0 < win_x && win_x < (int) client_shell->core.width) &&
				  (0 < win_y && win_y < (int) client_shell->core.height)){
					 /* 
					  * Yes, place menu at pointer
					  */
					 x = root_x;
					 y = root_y;
				 }
			 else{
					 /*
					  * No, place menu inside window at edge near pointer
					  */
					 x = (win_x < 0) ? 0 : 
						 (win_x < (int)(client_shell->core.width - menu->core.width)) ? 
							 win_x : client_shell->core.width - menu->core.width;
					 y = (win_y < 0)? 0 : 
						 (win_y < (int)(client_shell->core.height - menu->core.height))?
							 win_y : client_shell->core.height - menu->core.height;
					 x += root_x - win_x;
					 y += root_y - win_y;
				 }
		 }
    OlPostPopupMenu(
						  XtWindowToWidget(XtDisplay(menu), window), /* parent*/
						  menu,
						  activation_type,
						  (OlPopupMenuCallbackProc)NULL,
						  /* shouldn't use "2" direct...				*/
						  (Position)(x+2),	/* root_x, root_y			*/
						  (Position)(y+2),
						  (Position)(x+2),	/* init_x, init_y			*/
						  (Position)(y+2)
    );
}
