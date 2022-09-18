/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XGetDflt.c	1.1"
/*
 * $XConsortium: XGetDflt.c,v 1.27 91/07/09 14:54:15 rws Exp $
 */

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#include "Xlibint.h"
#include <X11/Xos.h>
#include <X11/Xresource.h>
#include <pwd.h>
#include <stdio.h>
#include <ctype.h>

#ifdef X_NOT_STDC_ENV
extern char *getenv();
#endif

static char *GetHomeDir (dest)
	char *dest;
{
#ifndef X_NOT_POSIX
        uid_t uid;
#else
	int uid;
	extern int getuid();
#ifndef SYSV386
	extern struct passwd *getpwuid(), *getpwnam();
#endif
#endif
	struct passwd *pw;
	register char *ptr;

	if((ptr = getenv("HOME")) != NULL) {
		(void) strcpy(dest, ptr);

	} else {
		if((ptr = getenv("USER")) != NULL) {
			pw = getpwnam(ptr);
		} else {
			uid = getuid();
			pw = getpwuid(uid);
		}
		if (pw) {
			(void) strcpy(dest, pw->pw_dir);
		} else {
		        *dest = '\0';
		}
	}
	return dest;
}


static XrmDatabase InitDefaults (dpy)
    Display *dpy;			/* display for defaults.... */
{
    XrmDatabase userdb;
    XrmDatabase xdb;
    char fname[BUFSIZ];                 /* longer than any conceivable size */
    char *xenv;

    XrmInitialize();

    /*
     * See lib/Xtk/Initialize.c
     *
     * First, get the defaults from the server; if none, then load from
     * ~/.Xdefaults.  Next, if there is an XENVIRONMENT environment variable,
     * then load that file.
     */

    if (dpy->xdefaults == NULL) {
	fname[0] = '\0';
	(void) GetHomeDir (fname);
	(void) strcat (fname, "/.Xdefaults");
	xdb = XrmGetFileDatabase (fname);
    } else {
	xdb = XrmGetStringDatabase(dpy->xdefaults);
    }

    if ((xenv = getenv ("XENVIRONMENT")) == NULL) {
	int len;
	fname[0] = '\0';
	(void) GetHomeDir (fname);
	(void) strcat (fname, "/.Xdefaults-");
	len = strlen (fname);
	(void) _XGetHostname (fname+len, BUFSIZ-len);
	xenv = fname;
    }
    userdb = XrmGetFileDatabase (xenv);
    XrmMergeDatabases (userdb, &xdb);
    return (xdb);

#ifdef old
    if (fname[0] != '\0') userdb =  XrmGetFileDatabase(fname);
    xdb = XrmGetStringDatabase(dpy->xdefaults);
    XrmMergeDatabases(userdb, &xdb);
    return xdb;
#endif
}

#if NeedFunctionPrototypes
char *XGetDefault(
	Display *dpy,			/* display for defaults.... */
	char _Xconst *prog,		/* name of program for option	*/
	register _Xconst char *name)	/* name of option program wants */
#else
char *XGetDefault(dpy, prog, name)
	Display *dpy;			/* display for defaults.... */
	char *prog;			/* name of program for option	*/
	register char *name;		/* name of option program wants */
#endif
{					/* to get, for example, "font"  */
	XrmName names[3];
	XrmClass classes[3];
	XrmRepresentation fromType;
	XrmValue result;
	char *progname;

	/*
	 * strip path off of program name (XXX - this is OS specific)
	 */
	progname = rindex (prog, '/');
	if (progname)
	    progname++;
	else
	    progname = (char *)prog;

	/*
	 * see if database has ever been initialized.  Lookups can be done
	 * without locks held.
	 */
	LockDisplay(dpy);
	if (dpy->db == NULL) {
		dpy->db = InitDefaults(dpy);
		}
	UnlockDisplay(dpy);

	names[0] = XrmStringToName(progname);
	names[1] = XrmStringToName(name);
	names[2] = NULLQUARK;
	classes[0] = XrmStringToClass("Program");
	classes[1] = XrmStringToClass("Name");
	classes[2] = NULLQUARK;
	(void)XrmQGetResource(dpy->db, names, classes, &fromType, &result);
	return (result.addr);
}

