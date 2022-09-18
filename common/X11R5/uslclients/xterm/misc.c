/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xterm:misc.c	1.2.1.61"
#endif

/*
 misc.c (C source file)
	Acc: 601052325 Tue Jan 17 09:58:45 1989
	Mod: 601054102 Tue Jan 17 10:28:22 1989
	Sta: 601054102 Tue Jan 17 10:28:22 1989
	Owner: 7007
	Group: 1985
	Permissions: 666
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/

/* Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts. */

#include <X11/copyright.h>

#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <ctype.h>
#include <pwd.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "ptyx.h"
#include "data.h"
#include "error.h"
#include "xterm.h"		/* for CTRL() */
#include "xterm_menu.h"
#include <X11/cursorfont.h>
#include "gray.ic"
#include <X11/Shell.h>

#ifdef SVR4
#include <unistd.h>
#endif /* SVR4 */

#include "Strings.h"
#include "messages.h"

/* SS-copy */
#include	<Xol/OpenLookP.h>
/* SS-copy-end */


/*
 * USL SVR4: need to define SYSV
 */
#define SYSV YES


#ifndef MEMUTIL
extern char *malloc();
#endif
extern char *mktemp();
extern void exit();
extern void perror();
extern void abort();

static	XSizeHints  work_SizeHints;
static	XSizeHints  orig_SizeHints;

#ifndef lint
static char rcs_id[] = "$Header: misc.c,v 1.15 88/02/27 09:38:23 rws Exp $";
#endif	/* lint */

xevents()
{
	XEvent event;
	register TScreen *screen = &term->screen;
	XtAppContext app_context =XtWidgetToApplicationContext(toplevel);
	static Boolean Scrollbar_in_use = FALSE;
	OlVirtualEventRec ve;



	if(screen->scroll_amt)
		FlushScroll(screen);
#ifdef DEBUG
	printf("\nInto xevents");
#endif /* DEBUG */
	/*if (XPending (screen->display)) */
	XtAppPending (app_context);
	{
	     do {
		if (waitingForTrackInfo)
			return;
		XtAppNextEvent(app_context, &event);
#ifdef DEBUG
		printf("\nInside xevents loop: event->type=%d",event.xany.type);
#endif /* DEBUG */

			/* only do this when there is a scrollbar	*/
			/* thus, the lookup routine should be inside of */
			/* this if block				*/
		if (screen->scrollbar && screen->scrollWidget)
		{
/* FLH mouseless */
		   OlLookupInputEvent(screen->scrollWidget, &event, &ve,
				OL_DEFAULT_IE);
		   if (event.xany.window == screen->scrollWidget->core.window &&
		        event.xany.type == ButtonPress)
			if (ve.virtual_name == OL_SELECT)
			    Scrollbar_in_use = TRUE;
		}

		if (Scrollbar_in_use)
		{
		    if (event.xany.type == ButtonRelease && ve.virtual_name == OL_SELECT)
		        Scrollbar_in_use = FALSE;
		}
/* FLH mouseless-end */
		/*
		 * RJK begin (secure keyboard)
		 *
		 * Previously, all events were dispatched ...
		 * Now, only dispatch an event if ...
		 * - it is not a send event OR
		 * - send events are allowed OR
		 * - send events AREN'T allowed but
		 *   the event is not a keyboard/mouse event
		 */
		 if (!event.xany.send_event ||
		     screen->allowSendEvents ||
		     ((event.xany.type != KeyPress) &&
		      (event.xany.type != KeyRelease) &&
		      (event.xany.type != ButtonPress) &&
		      (event.xany.type != ButtonRelease))) { 
			    XtDispatchEvent(&event);
		}
		/* RJK end */ 

#ifdef TEK
		if (focus_switched == TEK_MODE && !screen->TekEmu){
			end_vt_mode(NULL);
		}
		else
			if (focus_switched == VT_MODE && screen->TekEmu){
				end_tek_mode(NULL);
			}
#endif /* TEK */
				
	     } while (QLength(screen->display) > 0 || Scrollbar_in_use);
	}
}


Cursor make_colored_cursor (w, cursorindex, fg, bg)	
	XtermWidget w;
	int cursorindex;			/* index into font */
	unsigned long fg, bg;			/* pixel value */
{
	register TScreen *screen = &w->screen;
	Cursor c;
	register Display *dpy = screen->display;
	XColor foreback[2];
	
	c = XCreateFontCursor (dpy, cursorindex);
	if (c == (Cursor) 0) return (c);

	foreback[0].pixel = fg;
	foreback[1].pixel = bg;
	XQueryColors (dpy, DefaultColormap (dpy, DefaultScreen (dpy)),
		      foreback, 2);
	XRecolorCursor (dpy, c, foreback, foreback+1);
	return (c);
}


/* FLH mouseless */
/*
 *	HandleTekKeyPressed: pop up menu in response to menu key
 *			consume ALL other keys by sending them to the shell
 */
#ifdef TEK
void HandleTekKeyPressed(w,ve)
Widget w;
OlVirtualEvent ve;
{
	register TScreen *screen = &term->screen;
	extern TekMenu();
	XKeyEvent * key_event = &ve->xevent->xkey;

#ifdef ACTIVEWINDOWINPUTONLY
    if (w == (screen->TekEmu ? (Widget)tekWidget : (Widget)term)){
#endif
	switch(ve->virtual_name){
		case OL_MENUKEY:
			ve->consumed = True;
			TekMenu(key_event);
			break;
		default:
			ve->consumed = True;
	    		Input (&term->keyboard, screen, ve);
			break;
			break;
	}	
#ifdef ACTIVEWINDOWINPUTONLY
    }
#endif
		/* mark event as consumed or not as appropriate */
}	/* end of HandleTekKeyPressed */
#endif /* TEK */
/* FLH mouseless-end */

/* FLH mouseless */
/*
 *	HandleKeyPress: vt100 keypress handler.  
 *	If operating in mouseless mode (misc.mouseless == True),
 *	xterm interprets some mouseless keys and sends all others
 *	to the shell.
 *	If not in mouseless mode, xterm sends all input to the shell.
 *	
 *	Keys consumed in mouseless mode: menu, scrolling, help, cut, copy, paste.
 */
void HandleKeyPressed(w,ve)
Widget w;
OlVirtualEvent ve;
{
    register TScreen *screen = &term->screen;
    extern   int PasteText(), CopyText();
#ifdef XTERM_CUT
    extern	 int CutText();
#endif
    XEvent * event = ve->xevent;

    ve->consumed = False;

#ifdef ACTIVEWINDOWINPUTONLY
    if (w == (screen->TekEmu ? (Widget)tekWidget : (Widget)term)){
#endif
	    if (term->misc.mouseless == False){
		    /* mouseless mode is off, send key to shell */
		    ve->consumed = True;
		    Input (&term->keyboard, screen, ve);		    
		}
	    else{
	    switch(ve->virtual_name){
	        case OL_HELP:
		    _OlProcessHelpKey(w, ve->xevent);
		    ve->consumed = True;
		    break;
		
		case OL_PAGEUP:
		case OL_PAGEDOWN:
		case OL_SCROLLUP:
		case OL_SCROLLDOWN:
		case OL_SCROLLBOTTOM:
		case OL_SCROLLTOP: 
		    /* activate the scrollbar */
		    /* NOTE: key sequences bound to the vertical */
		    /* scrolling functions will always be intercepted */
		    /* here; they will not go to the shell.  The */
		    /* scrolling functions must be rebound to get those */
		    /* key sequences */
		    if (screen->scrollWidget)
			/* This should be done with OlActivateWidget, but that */
			/* does not work because OlActivateWidget only calls   */
			/* a widget's activate proc if the widget is managed.	 */
			/* Since the scrollbar is an unmanaged child of xterm, */
			/* we call its activate function directly here	 */
			OlActivateWidget(screen->scrollWidget,ve->virtual_name,NULL);
			ve->consumed = True;
			break;
		case OL_MENUKEY:
		    ve->consumed = True;
		    /* In VT100 mode, */
		    /* pop up sb menu if pointer is over scrollbar */
		    /* order of evaluation is important here */
#ifdef TEK
		    if ( !screen->TekEmu && screen->scrollbar 
#else
			if ( screen->scrollbar 
#endif /* TEK */
			    && (event->xkey.x > 
				(int) (((Widget)term)->core.width
				       + ((Widget)term)->core.border_width))
			    && (event->xkey.x <= (int) ((Widget)container)->core.width)
			    && (event->xkey.y >= 0)
			    && (event->xkey.y <= (int) (((Widget)term)->core.height 
					       + ((Widget)term)->core.border_width)))
			/* activate scrollbar as above */
			OlActivateWidget(screen->scrollWidget,ve->virtual_name,NULL);
			else	
			/* just bring up xterm menu */
			XtermMenu(&(event->xkey));
			break;
		    case OL_PASTE:
			ve->consumed = True;
			PasteText();
			break;
		    case OL_COPY:
			ve->consumed = True;
			CopyText();
			break;
#ifdef XTERM_CUT
		    case OL_CUT:
			ve->consumed = True;
			CutText();
			break;
#endif
		    case OL_VSBMENU:
			/* vertical scrollbar menu accelerator in VT100 mode only */
#ifdef TEK
			if (!screen->TekEmu && screen->scrollbar && screen->scrollWidget){
#else
			if (screen->scrollbar && screen->scrollWidget){
#endif /* TEK */
				ve->consumed = True;
				OlActivateWidget(screen->scrollWidget,ve->virtual_name,NULL);
				break;
			}
				/* if no scrollbar or in Tek mode, FALL THROUGH */
		default:
				/* consume all other key events */
			ve->consumed = True;
	    		Input (&term->keyboard, screen, ve);
			break;
	}  /* switch(ve->virtual_name) */
    } /* else */
#ifdef ACTIVEWINDOWINPUTONLY
    }
#endif
}	/* end of HandleKeyPressed */
/* FLH mouseless-end */


/*ARGSUSED*/
void HandleEnterWindow(w, eventdata, event)
Widget w;
register XEnterWindowEvent *event;
XtPointer eventdata;
{
#ifndef SYSV
/* ehr3 SYSV - removes this TEMPORARILY until hollow cursor problem is fixed */
    register TScreen *screen = &term->screen;

#ifdef ACTIVEWINDOWINPUTONLY
    if (w == XtParent(screen->TekEmu ? (Widget)tekWidget : (Widget)term)) 
#endif
      if (((event->detail) != NotifyInferior) &&
	  event->focus &&
	  !(screen->select & FOCUS))
	selectwindow(screen, INWINDOW);
#endif
}


/*ARGSUSED*/
void HandleLeaveWindow(w, eventdata, event)
Widget w;
register XEnterWindowEvent *event;
XtPointer eventdata;
{
#ifndef SYSV
/* ehr3 SYSV - removes this TEMPORARILY until hollow cursor problem is fixed */
    register TScreen *screen = &term->screen;

#ifdef ACTIVEWINDOWINPUTONLY
    if (w == XtParent(screen->TekEmu ? (Widget)tekWidget : (Widget)term)) 
#endif
      if (((event->detail) != NotifyInferior) &&
	  event->focus &&
	  !(screen->select & FOCUS))
	unselectwindow(screen, INWINDOW);

#endif
}


/*ARGSUSED*/
void HandleFocusChange(w, eventdata, event)
Widget w;
register XFocusChangeEvent *event;
XtPointer eventdata;
{
        register TScreen *screen = &term->screen;

	/*
	 * RJK begin (secure keyboard)
        if(event->type == FocusIn)
                selectwindow(screen, FOCUS);
        else
                unselectwindow(screen, FOCUS);
	*/
        if(event->type == FocusIn)
                selectwindow(screen, (event->detail == NotifyPointer) ?
			INWINDOW : FOCUS);
        else {
                unselectwindow(screen, (event->detail == NotifyPointer) ?
			INWINDOW : FOCUS);
		if (screen->grabbedKbd /* && (event->mode == NotifyUngrab) */)
		{
		    XWindowAttributes       win_attrs;

		    XGetWindowAttributes (screen->display, VWindow(screen),
					  &win_attrs);

		    if (win_attrs.map_state == IsUnviewable)
		    {
			extern   void	 UpdatePropery();
			extern Changetitle();

			screen->grabbedKbd = FALSE;
			Changetitle (title);
#ifdef SECURE_KEYBOARD
			UpdateProperty (SECURE_KBD_PROP, FALSE);
#endif
			if (term->flags & REVERSE_VIDEO)
			{
			    term->flags ^= REVERSE_VIDEO;
                    	    ReverseVideo(term);
                    	    UpdateProperty (REVERSE_VIDEO_PROP, FALSE);
			}
			XBell(screen->display, 100);
		    }
		}
	}
	/* RJK end */
}



selectwindow(screen, flag)
register TScreen *screen;
register int flag;
{
#ifdef TEK
	if(screen->TekEmu) {
		TekSelect();
		if(!Ttoggled)
			TCursorToggle(TOGGLE);
		screen->select |= flag;
		if(!Ttoggled)
			TCursorToggle(TOGGLE);
	} else {
#endif /* TEK */
		VTSelect();
		if(screen->cursor_state &&
	   	  (screen->cursor_col != screen->cur_col ||
	    	  screen->cursor_row != screen->cur_row))
	    		HideCursor();
		screen->select |= flag;
		if(screen->cursor_state)
			ShowCursor();
#ifdef TEK
	}
#endif /* TEK */
}

unselectwindow(screen, flag)
register TScreen *screen;
register int flag;
{
#ifdef TEK
	if(screen->TekEmu) {
		if(!Ttoggled) TCursorToggle(TOGGLE);
		screen->select &= ~flag;
		TekUnselect();
		if(!Ttoggled) TCursorToggle(TOGGLE);
	} else {
#endif /* TEK */
		screen->select &= ~flag;
		VTUnselect();
		if(screen->cursor_state &&
	   	   (screen->cursor_col != screen->cur_col ||
	    	   screen->cursor_row != screen->cur_row))
	      	   HideCursor();
		if(screen->cursor_state)
	  	   ShowCursor();
#ifdef TEK
	}
#endif /* TEK */
}

/*ARGSUSED*/
reselectwindow(screen)
register TScreen *screen;
{
#ifdef obsolete
	Window root, win;
	int rootx, rooty, x, y;
	int doselect = 0;
	unsigned int mask;

	if(XQueryPointer(
	    screen->display, 
	    DefaultRootWindow(screen->display), 
	    &root, &win,
	    &rootx, &rooty,
	    &x, &y,
	    &mask)) {
		XtTranslateCoords(term, 0, 0, &x, &y);
		if ((rootx >= x) && (rootx < x + term->core.width) &&
		    (rooty >= y) && (rooty < y + term->core.height))
		    doselect = 1;
		else if (tekWidget) {
		    XtTranslateCoords(tekWidget, 0, 0, &x, &y);
		    if ((rootx >= x) && (rootx < x + tekWidget->core.width) &&
			(rooty >= y) && (rooty < y + tekWidget->core.height))
			doselect = 1;
		}
		if (doselect)
			selectwindow(screen, INWINDOW);
		else	unselectwindow(screen, INWINDOW);
	}
#endif /* obsolete */
}

Pixmap Make_tile(w, width, height, bits, foreground, background, depth)	
	XtermWidget w;
	unsigned int width, height, depth;
	Pixel foreground, background;
	char *bits;
{
	register GC gc;
	register Pixmap pix;
	register TScreen *screen = &w->screen;
	XGCValues gcVals;
	XImage tileimage;

        pix = (Pixmap)XCreatePixmap(screen->display, 
	  DefaultRootWindow(screen->display), width, height, depth);
	gcVals.foreground = foreground;
	gcVals.background = background;
	gc = XCreateGC(screen->display, (Drawable) pix, 
	  GCForeground+GCBackground, &gcVals);
	tileimage.height = height;
	tileimage.width = width;
	tileimage.xoffset = 0;
	tileimage.format = XYBitmap;
	tileimage.data = bits;
	tileimage.byte_order = LSBFirst;
	tileimage.bitmap_unit = 8;
	tileimage.bitmap_bit_order = LSBFirst;
	tileimage.bitmap_pad = 8;
	tileimage.bytes_per_line = (width+7)>>3;
	tileimage.depth = 1;
        XPutImage(screen->display, pix, gc, &tileimage, 0, 0, 0, 0, width, height);
        XFreeGC (screen->display, gc);
	return(pix);
}


Pixmap make_gray(w, fg, bg, depth)
XtermWidget w;
Pixel fg, bg;
{
	return(Make_tile(w, gray_width, gray_height, gray_bits, fg, bg, depth));
}

/* ARGSUSED */
Cursor make_xterm(w, fg, bg)
XtermWidget w;
unsigned long fg, bg;
{
	return (make_colored_cursor (w, XC_xterm, fg, bg));
}


/* ARGSUSED */
Cursor make_arrow(w, fg, bg)
XtermWidget w;
unsigned long fg, bg;

{
	return (make_colored_cursor (w, XC_left_ptr, fg, bg));
}


Bell()
{
	extern XtermWidget term;
	register TScreen *screen = &term->screen;
	register Pixel xorPixel = screen->foreground ^ screen->background;
	XGCValues gcval;
	GC visualGC;

	if(screen->visualbell) {
		gcval.function = GXxor;
		gcval.foreground = xorPixel;
		visualGC = XtGetGC((Widget)term, GCFunction+GCForeground, &gcval);
		XFillRectangle(
		    screen->display,
		    VWindow(screen), 
		    visualGC,
		    0, 0,
		    (unsigned) FullWidth(screen),
		    (unsigned) FullHeight(screen));
		XFlush(screen->display);
		XFillRectangle(
		    screen->display,
		    VWindow(screen), 
		    visualGC,
		    0, 0,
		    (unsigned) FullWidth(screen),
		    (unsigned) FullHeight(screen));
	} else
		XBell(screen->display, 0);
}

Redraw()
{
	/* extern XtermWidget term; */
	register TScreen *screen = &term->screen;
	XExposeEvent event;

	event.type = Expose;
	event.display = screen->display;
	event.x = 0;
	event.y = 0;
	event.count = 0; 
	
	if(VWindow(screen)) {
	        event.window = VWindow(screen);
		event.width = term->core.width;
		event.height = term->core.height;
		(*term->core.widget_class->core_class.expose)((Widget)term, 
							      (XEvent *)&event, NULL);
		/* if(screen->scrollbar) 
			(*screen->scrollWidget->core.widget_class->core_class.expose)(screen->scrollWidget, &event, NULL); */
 	}
}


#ifdef TEK
TRedraw()
{
	register TScreen *screen = &term->screen;
	XExposeEvent event;

	event.type = Expose;
	event.display = screen->display;
	event.x = 0;
	event.y = 0;
	event.count = 0; 
	
	if (TWindow(screen) /*&& screen->Tshow*/) {
		event.window = TWindow(screen);
		event.width = tekWidget->core.width;
		event.height = tekWidget->core.height;
		TekExpose (tekWidget, &event, NULL);
	}
}
#endif /* TEK */

#ifdef obsolete
SyncUnmap(win, mask)
register Window win;
register long int mask;
{
	XEvent ev;
	register XEvent *rep = &ev;
	register TScreen *screen = &term->screen;

	do { /* ignore events through unmap */
		XWindowEvent(screen->display, win, mask, rep);
	} while(rep->type != UnmapNotify);
	return;
}
#endif /* obsolete */


static void logpipe(dummy)
int dummy;
{
	register TScreen *screen = &term->screen;

	if(screen->logging)
		CloseLog(screen);
}

StartLog(screen)
register TScreen *screen;
{
	register char *cp;
	register int i;
	static char *log_default;
	char
#ifndef MEMUTIL
*malloc(),
#endif
	*rindex();
	static void logpipe();

	if(screen->logging || (screen->inhibit & I_LOG))
		return;
	if(screen->logfile == NULL || *screen->logfile == 0) {
		if(screen->logfile)
			free(screen->logfile);
		if(log_default == NULL)
			mktemp(log_default = log_def_name);
		if((screen->logfile = malloc((unsigned)strlen(log_default) + 1)) == NULL)
			return;
		strcpy(screen->logfile, log_default);
	}
	if(*screen->logfile == '|') {	/* exec command */
		int p[2];
		static char *shell;

		if(pipe(p) < 0 || (i = fork()) < 0)
			return;
		if(i == 0) {	/* child */
			close(p[1]);
			dup2(p[0], 0);
			close(p[0]);
			dup2(fileno(stderr), 1);
			dup2(fileno(stderr), 2);
			close(fileno(stderr));
			fileno(stderr) = 2;
			close(screen->display->fd);
			close(screen->respond);
			if(!shell) {
				register struct passwd *pw;
				char *getenv()
#ifndef MEMUTIL
, *malloc()
#endif
					;
				struct passwd *getpwuid();

				if(((cp = getenv("SHELL")) == NULL || *cp == 0)
				 && ((pw = getpwuid(screen->uid)) == NULL ||
				 *(cp = pw->pw_shell) == 0) ||
				 (shell = malloc((unsigned) strlen(cp) + 1)) == NULL)
					shell = "/bin/sh";
				else
					strcpy(shell, cp);
			}
			signal(SIGHUP, SIG_DFL);
			signal(SIGCHLD, SIG_DFL);
			setgid(screen->gid);
			setuid(screen->uid);
			execl(shell, shell, "-c", &screen->logfile[1], (char *)0);
#if !defined(I18N)
			fprintf(stderr, "%s: Can't exec `%s'\n", xterm_name,
			 &screen->logfile[1]);
#else
			OlVaDisplayErrorMsg(screen->display, OleNexec,
                                        OleTbadExecl,
                                        OleCOlClientXtermMsgs, OleMexec_badExec1,
                                        NULL);

#endif
			exit(ERROR_LOGEXEC);
		}
		close(p[0]);
		screen->logfd = p[1];
		(void) signal(SIGPIPE, logpipe);
	} else {
		if(access(screen->logfile, F_OK) == 0) {
			errno = 0;
			if(access(screen->logfile, W_OK) < 0)
			{       
#if !defined(I18N)
			   fprintf(stderr, "\nCannot access login file '%s', ERRNO = %d\n", screen->logfile, errno);
#else
			OlVaDisplayWarningMsg(screen->display, OleNaccess,
                                        OleTloginFile,
                                        OleCOlClientXtermMsgs, OleMaccess_loginFile,
                                        NULL);
#endif
				return;
			}
		} else if(cp = rindex(screen->logfile, '/')) {
			*cp = 0;
			errno = 0;
			i = access(screen->logfile, W_OK);
			*cp = '/';
			if(i < 0)
			{       
#if !defined(I18N)
			   fprintf(stderr, "\nCannot access login file '%s', ERRNO = %d\n", screen->logfile, errno);
#else
			OlVaDisplayWarningMsg(screen->display, OleNaccess,
                                        OleTloginFile,
                                        OleCOlClientXtermMsgs, OleMaccess_loginFile,
                                        NULL);
#endif
			   return;
			}
		} else {
		           errno = 0;
			   if (access(".", W_OK) < 0) {       
#if !defined(I18N)
			       fprintf(stderr, "\nCannot access login file '%s', ERRNO = %d\n", screen->logfile, errno);
#else
			OlVaDisplayWarningMsg(screen->display, OleNaccess,
                                        OleTloginFile,
                                        OleCOlClientXtermMsgs, OleMaccess_loginFile,
                                        NULL);
#endif
			       return;
		           }
		}
		if((screen->logfd = open(screen->logfile, O_WRONLY | O_APPEND |
		 O_CREAT, 0644)) < 0)
		{       
#if !defined(I18N)
			fprintf(stderr, "\nCannot open login file '%s'\n", screen->logfile);
#else
			OlVaDisplayWarningMsg(screen->display, OleNopen,
                                        OleTloginFile,
                                        OleCOlClientXtermMsgs, OleMopen_loginFile,
                                        NULL);
#endif
			return;
		}
		chown(screen->logfile, screen->uid, screen->gid);

	}
	screen->logstart = bptr;
	screen->logging = TRUE;
}

CloseLog(screen)
register TScreen *screen;
{
	if(!screen->logging || (screen->inhibit & I_LOG))
		return;
	FlushLog(screen);
	close(screen->logfd);
	screen->logging = FALSE;
}

FlushLog(screen)
register TScreen *screen;
{
	register Char *cp;
	register int i;

	cp = bptr;
	if((i = cp - screen->logstart) > 0)
		write(screen->logfd, screen->logstart, i);
	screen->logstart = buffer;
}


do_osc(func)
int (*func)();
{
	register TScreen *screen = &term->screen;
	register int mode, c, i=0;
	register char *cp;
	char buf[512];
#ifndef MEMUTIL
	extern char *malloc();
#endif

	mode = 0;
	while(isdigit(c = (*func)()))
	{	i++;
		mode = 10 * mode + (c - '0');
	}

	if ((i == 0) || (c != ';'))
	    return;
	else
	    i = 0;

	cp = buf;
	while((++i < 512) && isprint(c = (*func)()))
		*cp++ = c;

	if (c != CTRL('G'))
	    return;

	*cp = '\0';
	switch(mode) {
	 case 0:	/* new icon name and title*/
		Changename(buf);
	 case 2:	/* that right: no break above, new title only */
    		if (title)
        	    free (title);
		if (i == 1)
#if !defined(I18N)
    		    strcpy(buf, "Untitled");
#else
    		    strcpy(buf, OlGetMessage(screen->display, NULL, 0,
						OleNtitle, OleTuntitled,
						OleCOlClientXtermMsgs,
						OleMtitle_untitled, NULL));
#endif
    		title = strdup(buf);
		if (screen->grabbedKbd)
#if !defined(I18N)
		    sprintf(buf, "%s: Secure Keyboard", title);
#else
			sprintf(buf, 
				OlGetMessage(screen->display, NULL, 0,
					OleNtitle, OleTsecureKbd,
					OleCOlClientXtermMsgs, NULL,
					NULL), title);
#endif
		Changetitle(buf);
		break;

	 case 1:	/* new icon name only */
		Changename(buf);
		break;

/* FLH resize */
	 case 3:	/* curses smcup */
		do_smcup();
		break;
	 case 4: /* curses rmcup */
		do_rmcup();
		break;
/* FLH resize-end */

	 case 46:	/* new log file */
		if((cp = malloc((unsigned)strlen(buf) + 1)) == NULL)
			break;
		strcpy(cp, buf);
		if(screen->logfile)
			free(screen->logfile);
		screen->logfile = cp;
		break;
	}
}

static ChangeGroup(attribute, value)
     String attribute;
     XtArgVal value;
{
	register TScreen *screen = &term->screen;
	Arg args[1];

	/* set title or name for VT window	*/
	XtSetArg( args[0], attribute, value );
	XtSetValues( toplevel, args, 1 );

	if (strcmp(attribute, XtNtitle) == 0)
	{
	    if (screen->menuWidget)
		XtSetValues( screen->menuWidget, args, 1 );
	    if (screen->property)
		XtSetValues( screen->property, args, 1 );
	}

	/* set title or name for Tek window	*/

#ifdef TEK
	if (tekWidget)
	{
	    char *tek_value;

	    tek_value = XtMalloc(strlen((char *)value)+7);
	    strcpy(tek_value, (char *)value);
#if !defined(I18N)
	    strcat(tek_value, "(Tek)");
#else
	    strcat(tek_value, OlGetMessage(screen->display, NULL, 0, OleNtitle,
				OleTtek, OleCOlClientXtermMsgs, OleMtitle_tek,
				NULL) );
#endif
	    XtSetArg( args[0], attribute, tek_value );
	    XtSetValues(tekWidget->core.parent, args, 1 );

	    if (strcmp(attribute, XtNtitle) == 0)
	    {
	        if (screen->TmenuWidget)
		    XtSetValues( screen->TmenuWidget, args, 1 );
	        if (screen->Tproperty)
		    XtSetValues( screen->Tproperty, args, 1 );
	    }
	    XtFree (tek_value);
	}
#endif /* TEK */
}

Changename(name)
register char *name;
{
    ChangeGroup( XtNiconName, (XtArgVal)name );
}

Changetitle(name)
register char *name;
{
    ChangeGroup( XtNtitle, (XtArgVal)name );
}

#ifndef DEBUG
/* ARGSUSED */
#endif
Panic(s, a)
char	*s;
int a;
{
#ifdef DEBUG
	if(debug) {
		fprintf(stderr, "%s: PANIC!	", xterm_name);
		fprintf(stderr, s, a);
		fputs("\r\n", stderr);
		fflush(stderr);
	}
#endif	/* DEBUG */
}

SysError (i)
int i;
{
	int oerrno;
#ifdef SYSV
	extern char *sys_errlist[];
#endif	/* SYSV */

	oerrno = errno;
#ifdef SYSV
	/* SYSV perror write(2)s to file descriptor 2 */
#if !defined(I18N)
	fprintf (stderr, "%s: Error %d, errno %d: ", xterm_name, i, oerrno);
#else
	fprintf (stderr, OlGetMessage(XtDisplay(toplevel), NULL, 0, OleNprintf,
				OleTerrmsg1, OleCOlClientXtermMsgs,
				OleMprintf_errmsg1, NULL), xterm_name, i, oerrno);
#endif /* I18N */
	fprintf (stderr, "%s\n", sys_errlist[oerrno]);

#else	/* !SYSV */
#if !defined(I18N)
	fprintf (stderr, "%s: Error %d, errno %d:\n", xterm_name, i, oerrno);
#else
	fprintf (stderr, OlGetMessage(XtDisplay(toplevel), NULL, 0, OleNprintf,
				OleTerrmsg1, OleCOlClientXtermMsgs,
				OleMprintf_errmsg1, NULL), xterm_name, i, oerrno);
#endif
	errno = oerrno;
	perror ("");
#endif	/* !SYSV */
	Exit(i);
}

Error (i)
int i;
{
#if !defined(I18N)
	fprintf (stderr, "%s: Error %d\n", xterm_name, i);
#else
	fprintf (stderr, OlGetMessage(XtDisplay(toplevel), NULL, 0, OleNprintf,
				OleTerrmsg2, OleCOlClientXtermMsgs,
				OleMprintf_errmsg2, NULL), xterm_name, i );
#endif
	Exit(i);
}


#ifndef SYSV

/*
 * sets the value of var to be arg in the Unix 4.2 BSD environment env.
 * Var should end with '=' (bindings are of the form "var=value").
 * This procedure assumes the memory for the first level of environ
 * was allocated using calloc, with enough extra room at the end so not
 * to have to do a realloc().
 */
Setenv (var, value)
register char *var, *value;
{
	extern char **environ;
	register int index = 0;
	register int len = strlen(var);

	while (environ [index] != NULL) {
	    if (strncmp (environ [index], var, len) == 0) {
		/* found it */
		environ[index] = (char *)malloc ((unsigned)len + strlen (value) + 1);
		strcpy (environ [index], var);
		strcat (environ [index], value);
		return;
	    }
	    index ++;
	}

#ifdef DEBUG
	if (debug) fputs ("expanding env\n", stderr);
#endif	/* DEBUG */

	environ [index] = (char *) malloc ((unsigned)len + strlen (value) + 1);
	(void) strcpy (environ [index], var);
	strcat (environ [index], value);
	environ [++index] = NULL;
}

/*
 * returns a pointer to the first occurrence of s2 in s1,
 * or NULL if there are none.
 */
char *strindex (s1, s2)
register char	*s1, *s2;
{
	register char	*s3;
	char		*index();

	while ((s3=index(s1, *s2)) != NULL) {
		if (strncmp(s3, s2, strlen(s2)) == 0)
			return (s3);
		s1 = ++s3;
	}
	return (NULL);
}


#endif /* SYSV */

/*ARGSUSED*/
xerror(d, ev)
Display *d;
register XErrorEvent *ev;
{
        char buffer[BUFSIZ];

/* mlp - this function, XGetErrorText(), isn't internationalized in R4... */
	XGetErrorText(d, ev->error_code, buffer, BUFSIZ);
	fprintf(stderr, "%s: %s\n", xterm_name, buffer);
#if !defined(I18N)
	fprintf(stderr, "Request code %d, minor code %d, serial #%ld, resource id %ld\n",
	 ev->request_code, ev->minor_code, ev->serial, (long)ev->resourceid);
#else
	fprintf (stderr, OlGetMessage(XtDisplay(toplevel), NULL, 0, OleNprintf,
				OleTerrmsg3, OleCOlClientXtermMsgs,
				OleMprintf_errmsg3, NULL),
	 ev->request_code, ev->minor_code, ev->serial, (long)ev->resourceid);
#endif
    	_cleanup();
    	abort();
/*	Exit(ERROR_XERROR); */
}

/*ARGSUSED*/
xioerror(d)
Display *d;
{
	perror(xterm_name);
	Exit(ERROR_XIOERROR);
}

XStrCmp(s1, s2)
char *s1, *s2;
{
  if (s1 && s2) return(strcmp(s1, s2));
  if (s1 && *s1) return(1);
  if (s2 && *s2) return(-1);
  return(0);
}


/* ICON.C: ATTACH AN ICON TO YOUR TOP WINDOW *** KAREN S. KENDLER */
/* 
   NOTE EDITING NEEDED:
	
	1.	#include <X11/Xutil.h> if you have not done so already.

	2.	edit this line to include your display name, top window
		name, and icon bitmap file name and add it to your code 
		as soon as you create your top window:

			set_icon(display,window,"filename"); 

	3.	compile this function with the rest of your code.

*/

/*
set_icon(display,window,filename)
	Display	*display;
	Window window;
	char *filename;
{
	Pixmap pixid;
	int i;
	int iconx,icony,hotx,hoty;	
	XWMHints xwmhints;


	i=XReadBitmapFile(display,window,
				filename,&iconx,&icony,&pixid,&hotx,&hoty);

	if (i == BitmapSuccess)
	{
	   xwmhints.icon_pixmap=pixid;		
	   xwmhints.flags=IconPixmapHint;	
	   XSetWMHints(display,window,&xwmhints);		
	}
}
*/


#ifdef TEK

#include "wait.ic"
#include "waitmask.ic"

/* ARGSUSED */
void HandleEightBitKeyPressed(w, event, params, nparams)
    Widget w;
    XEvent *event;
    String *params;
    Cardinal *nparams;
{
    register TScreen *screen = &term->screen;

#ifdef ACTIVEWINDOWINPUTONLY
    if (w == (screen->TekEmu ? (Widget)tekWidget : (Widget)term))
#endif
	Input (&term->keyboard, screen, event, True);
}


/* ARGSUSED */
void HandleStringEvent(w, event, params, nparams)
    Widget w;
    XEvent *event;
    String *params;
    Cardinal *nparams;
{
    register TScreen *screen = &term->screen;

#ifdef ACTIVEWINDOWINPUTONLY
    if (w != (screen->TekEmu ? (Widget)tekWidget : (Widget)term)) return;
#endif

    if (*nparams != 1) return;

    if ((*params)[0] == '0' && (*params)[1] == 'x' && (*params)[2] != '\0') {
	char c, *p, hexval[2];
	hexval[0] = hexval[1] = 0;
	for (p = *params+2; (c = *p); p++) {
	    hexval[0] *= 16;
	    if (isupper(c)) c = tolower(c);
	    if (c >= '0' && c <= '9')
		hexval[0] += c - '0';
	    else if (c >= 'a' && c <= 'f')
		hexval[0] += c - 'a' + 10;
	    else break;
	}
	if (c == '\0')
	    StringInput (screen, hexval);
    }
    else {
	StringInput (screen, *params);
    }
}

/* ARGSUSED */
Cursor make_tcross(w, fg, bg)
XtermWidget w;
Pixel fg, bg;
{
	return (make_colored_cursor (w, XC_tcross, fg, bg));
}


Cursor make_wait(w, fg, bg)
XtermWidget w;
Pixel fg, bg;
{
	register TScreen *screen = &w->screen;
	register Display *dpy = screen->display;
	Pixmap source, mask;
	XColor foreback[2];

	source = Make_tile(w, wait_width, wait_height, wait_bits, 1L, 0L, 1);
	mask = Make_tile(w, waitmask_width, waitmask_height, waitmask_bits, 
	 1L, 0L, 1);

	foreback[0].pixel = fg;
	foreback[1].pixel = bg;
	XQueryColors (dpy, DefaultColormap (dpy, DefaultScreen (dpy)),
		      foreback, 2);

	return (XCreatePixmapCursor (dpy, source, mask, foreback, foreback+1,
	 wait_x_hot, wait_y_hot));
}
#endif /* TEK */

/* FLH resize*/
/*
	*	DO_SMCUP: 	disable window resize in response to curses smcup
						if resize disable is on	
	*/
do_smcup()
{

	register TScreen *screen = &term->screen;

	screen->in_curses = TRUE;

	/* disable resize */

	if (!term->misc.allow_resize)
	    disable_resize();
}


disable_resize()
{
	register TScreen *screen = &term->screen;
   	XWindowAttributes       orig_win_attrs;

/* FLH dynamic
 *
 * term is now 2 levels below the shell
 */
	if (XGetNormalHints(XtDisplay(term), VShellWindow, &orig_SizeHints)==0)
/* FLH dynamic */
#if !defined(I18N)
	  Panic("VTparse: XGetNormalHints failed to obtain orig_SizeHints\n",0);
#else
		Panic(OlGetMessage(screen->display, NULL, 0,
				   OleNpanic, OleTpanic_msg2,
				   OleCOlClientXtermMsgs,
				   OleMpanic2, NULL), 0);
#endif
					/* SS added */
					/* get attributes of shell window */
	if (XGetWindowAttributes(screen->display,  VShellWindow,
						   &orig_win_attrs) == 0)
#if !defined(I18N)
	    Panic("VTparse: XGetWindowAttributes failed to obtain orig_win_attrs\n", 0);
#else
		Panic(OlGetMessage(screen->display, NULL, 0,
				   OleNpanic, OleTpanic_msg3,
				   OleCOlClientXtermMsgs,
				   OleMpanic3, NULL), 0);
#endif

	work_SizeHints = orig_SizeHints;
	work_SizeHints.min_width = orig_win_attrs.width;
	work_SizeHints.max_width = orig_win_attrs.width;
	work_SizeHints.min_height = orig_win_attrs.height;
	work_SizeHints.max_height = orig_win_attrs.height;
	work_SizeHints.flags |= (PMinSize | PMaxSize);

/* FLH dynamic
 *
 * term is now 2 levels below the shell
 */
	if (XSetNormalHints(XtDisplay(term), VShellWindow, &work_SizeHints)==0)
/* FLH dynamic */
#if !defined(I18N)
	    Panic("VTparse: XSetNormalHints failed to change to in-curses state\n", 0);
#else
		Panic(OlGetMessage(screen->display, NULL, 0,
				   OleNpanic, OleTpanic_msg4,
				   OleCOlClientXtermMsgs,
				   OleMpanic4, NULL), 0);
#endif
}

/* DO_RMCUP:  	re-enable screen resizing if it has been disabled
 *					by a previous smcup
 */
do_rmcup()
{
	register TScreen *screen = &term->screen;
	
	if (!screen->in_curses)
	    return;

	screen->in_curses = FALSE;
	enable_resize();
}

enable_resize()
{
	register TScreen *screen = &term->screen;

	/* enable resizing */

	work_SizeHints = orig_SizeHints;

	/* this is for the mwm sake: if max_width / height is set to  */
	/* 0, it takes it literally, rather then as 'no limit'	      */

	work_SizeHints.max_width =  50000;
	work_SizeHints.max_height = 50000;
	work_SizeHints.flags |= (PMaxSize);

/* FLH dynamic
 *
 * term is now 2 levels below shell widget 
 */
	if (XSetNormalHints(XtDisplay(term), VShellWindow, &work_SizeHints) == 0) 
/* FLH dynamic */
#if !defined(I18N)
	Panic("VTparse: XSetNormalHints failed to restore to out-of-curses state\n", 0);
#else
	Panic(OlGetMessage(screen->display, NULL, 0,
				   OleNpanic, OleTpanic_msg5,
				   OleCOlClientXtermMsgs,
				   OleMpanic5, NULL), 0);
#endif
}
/* FLH resize-end */

void
Message OLARGLIST((w, ve))
OLARG(Widget,  w)
OLGRA(OlVirtualEvent, ve)
{
XtermWidget xtermw 	=	(XtermWidget) w; 
Atom message_type 	= 	ve->xevent->xclient.message_type;
OlIc * 			ic 	=	xtermw->primitive.ic;
char * buffer;
char *ptr = buffer;
int len;
OlImStatus status;


if (message_type == _OlGetFrontEndAtom()){
			/*
			 *	Message contains converted text from a
			 * Front-end input method; call OlLookupImString 
			 * to extract the text.
			 */
   buffer = XtMalloc(BUFSIZ+1);	/* +1 for NULL terminator */
	if (ic){
		len = OlLookupImString((XKeyEvent *) ve->xevent, ic,
									  (char *) buffer, BUFSIZ, &ve->keysym, &status);
		if (status == XBufferOverflow){
				buffer = XtRealloc(buffer,len);
				len = OlLookupImString((XKeyEvent *) ve-> xevent, ic,
											  (char*)buffer, len, &ve->keysym, &status);
			}
		if (len > 0){
				/*
				 * Send returned text directly to shell after 
				 * adding null terminator
				 */		
			ptr = buffer;
			while (len--)
				unparseputc(*ptr++, xtermw->screen.respond);
			}
		}
	XtFree(buffer);
	}
} /* End of Message() */
