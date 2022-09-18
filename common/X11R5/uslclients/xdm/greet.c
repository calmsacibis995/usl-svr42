/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)xdm:greet.c	1.13"
/*
 * xdm - display manager daemon
 *
 * $XConsortium: greet.c,v 1.29 91/04/02 11:58:51 rws Exp $
 *
 * Copyright 1988 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * Author:  Keith Packard, MIT X Consortium
 */

/*
 * Get username/password
 *
 */

# include <X11/Intrinsic.h>
# include <X11/StringDefs.h>
# include <X11/Shell.h>
# include <X11/keysym.h>

#include <Xol/OpenLookP.h>
#include <Xol/buffutil.h>
#include <Xol/textbuff.h>
#include <Xol/TextField.h>

#include "dm.h"
#include "dtlogin.h"

extern Display	*dpy;
extern void	SensitiseLogin ();
extern void	PopupHelp ();

static int		done, code;
static char		name[128], password[128];
static Widget		login;
static XtIntervalId	pingTimeout;

Widget			toplevel;
XtAppContext		context;
Login_info		*linfo;
void			FailedLogin ();

/*ARGSUSED*/
static void
GreetPingServer (closure, intervalId)
    XtPointer	    closure;
    XtIntervalId    *intervalId;
{
    struct display *d;

    d = (struct display *) closure;
    if (!PingServer (d, XtDisplay (toplevel)))
	SessionPingFailed (d);
    pingTimeout = XtAppAddTimeOut (context, d->pingInterval * 60 * 1000,
				   GreetPingServer, (closure));
}

/*ARGSUSED*/
GreetDone (w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    Login_info			*info = (Login_info *)client_data;
    OlTextFieldVerify		*verify = (OlTextFieldVerify *)call_data;

    Debug ("GreetDone: password is %d long\n",
	    strlen (verify->string));
    switch (verify->reason) {
    case OlTextFieldReturn:
	OlTextFieldCopyString (info->login_text, name);
	OlTextFieldCopyString (info->password_text, password);
      Debug ("Before SetSensitive\n");
	XtSetSensitive (linfo->password_text, FALSE);
      Debug ("After SetSensitive\n");
	code = 0;
	done = 1;
	break;
    }
}

/*ARGSUSED*/
logCB (w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	struct display	*d = (struct display *)client_data;
	int		str_len;
	char		*tmp;
	Time		t = CurrentTime;

	tmp = OlTextFieldGetString (linfo->login_text, &str_len);
	if (str_len)
		{
		OlTextFieldCopyString (linfo->login_text, name);
		if (!strlen (name))
			{
			XBell (XtDisplay (linfo->login_text), 0);
			OlTextEditClearBuffer (linfo->login_text);
			OlTextEditClearBuffer (linfo->password_text);
			XtSetSensitive (linfo->password_text, FALSE);
			SensitiseLogin (FALSE);
			if (!(XtCallAcceptFocus (linfo->login_text, &t)))
				Debug ("Could not set input focus\n");
			}
		else
			{
			OlTextFieldCopyString (linfo->password_text, password);
			code = 0;
			done = 1;
			XtSetSensitive (linfo->password_text, FALSE);
			}
		}
	else
		FailedLogin (d);
	return;
}

Display *
InitGreet (d)
struct display	*d;
{
    Arg		arglist[10];
    int		i;
    static int	argc;
    Screen		*scrn;
    static char	*argv[] = { "dtlogin", 0 };
    Display		*dpy;
    Login_info		*CreateLoginInfo ();
    Dimension   center_x, center_y;
    Dimension	theWidth, theHeight;
    Dimension	w, h;
    Time	t = CurrentTime;

    Debug ("greet %s\n", d->name);
    argc = 1;

    OlToolkitInitialize (&argc, argv, NULL);

    XtToolkitInitialize ();
    context = XtCreateApplicationContext();
    dpy = XtOpenDisplay (context, d->name, "dtlogin", "Dtlogin", 0,0,
			 &argc, argv);

    if (!dpy)
	return 0;

    RegisterCloseOnFork (ConnectionNumber (dpy));

    SecureDisplay (d, dpy);

    i = 0;
    scrn = DefaultScreenOfDisplay(dpy);
    theWidth = WidthOfScreen(scrn);
    theHeight = HeightOfScreen(scrn);

    XtSetArg(arglist[i], XtNscreen, scrn);	i++;
    XtSetArg(arglist[i], XtNargc, argc);	i++;
    XtSetArg(arglist[i], XtNargv, argv);	i++;

    toplevel = XtAppCreateShell ((String) NULL, "Dtlogin",
		    applicationShellWidgetClass, dpy, arglist, i);


    linfo = CreateLoginArea (toplevel, d);
    XtAddCallback (linfo->password_text, XtNverification, GreetDone, 
			(XtPointer)linfo);

    XtSetMappedWhenManaged (toplevel, FALSE);
    Debug ("1\n");
    XtRealizeWidget (toplevel);

    XtVaGetValues (toplevel, XtNwidth, &w, XtNheight, &h, NULL);

    Debug ("2\n");
    center_x = (Dimension)((Dimension)(theWidth - w) / (Dimension)2);
    center_y = (Dimension)((Dimension)(theHeight - h) / (Dimension)2);

    Debug ("3\n");
    XtVaSetValues (toplevel, XtNx, center_x, XtNy, center_y, NULL);

    Debug ("4\n");
    XtSetMappedWhenManaged (toplevel, TRUE);
    XtMapWidget (toplevel);
    Debug ("5\n");

    XDefineCursor (XtDisplay(toplevel), XtWindow(toplevel),
	GetOlStandardCursor(XtScreen(toplevel)));

    if (!(XtCallAcceptFocus (linfo->login_text, &t)))
	Debug ("Could not set input focus\n");
    /*
    if (!(OlCallAcceptFocus (linfo->login_text, CurrentTime)))
	Debug ("Could not set initial focus");
	*/

    XWarpPointer(dpy, None, RootWindowOfScreen (scrn),
		    0, 0, 0, 0,
		    WidthOfScreen(scrn) / 2,
		    HeightOfScreen(scrn) / 2);

    if (d->pingInterval)
    {
    	pingTimeout = XtAppAddTimeOut (context, d->pingInterval * 60 * 1000,
				       GreetPingServer, (XtPointer) d);
    }

    return dpy;
}

CloseGreet (d)
struct display	*d;
{
    Boolean	allow;
    Arg	    arglist[1];

    if (pingTimeout)
    {
	XtRemoveTimeOut (pingTimeout);
	pingTimeout = 0;
    }
    UnsecureDisplay (d, XtDisplay (toplevel));
    XtDestroyWidget (toplevel);
    ClearCloseOnFork (ConnectionNumber (XtDisplay (toplevel)));
    XCloseDisplay (XtDisplay (toplevel));
    Debug ("Greet connection closed\n");
}

Greet (d, greet)
struct display		*d;
struct greet_info	*greet;
{
    XEvent	event;
    Arg		arglist[1];
    Boolean	value = False;
    int		cnt;
    char	buf[64];
    KeySym	keysym;
    XComposeStatus	compose;

    XtSetArg (arglist[0], XtNuserData, (XtArgVal)&value);
    XtSetValues (linfo->login_text, arglist, 1);

    Debug ("dispatching %s\n", d->name);
    done = 0;
    while (!done) {
	    XtAppNextEvent (context, &event);
	    if (event.type == KeyPress) 
		{
		cnt = XLookupString (&event, buf, 64, &keysym, &compose); 
		if (keysym == XK_F1)
			PopupHelp (toplevel, linfo);
		else
			XtDispatchEvent (&event);
		}
	    else
	    	XtDispatchEvent (&event);
    }
    XFlush (XtDisplay (toplevel));
    Debug ("Done dispatch %s\n", d->name);
    if (code == 0)
    {
	greet->name = name;
	greet->password = password;
	/*
	XtSetArg (arglist[0], XtNsessionArgument, (char *) &(greet->string));
	XtGetValues (linfo, arglist, 1);
	Debug ("sessionArgument: %s\n", greet->string ? greet->string : "<NULL>");
	*/
    }
    return code;
}


/*ARGSUSED*/
void
FailedLogin (struct display *d)
{
	char	*fail	=	"Login attempt failed.  Try again.";
	ArgList	args;
	Time	t = CurrentTime;

	XBell (XtDisplay (linfo->login_text), 0);
	OlTextEditClearBuffer (linfo->login_text);
	OlTextEditClearBuffer (linfo->password_text);
	XtSetSensitive (linfo->password_text, FALSE);
	SensitiseLogin (FALSE);
    	if (!(XtCallAcceptFocus (linfo->login_text, &t)))
		Debug ("Could not set input focus\n");
	XtVaSetValues (linfo->login_fail, XtNstring, fail, NULL);
	return;
}

/**/
