/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:internet/utils.c	1.6"
#endif

#include <stdio.h>
#include <pwd.h>
#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Error.h>
#include "error.h"

extern char *	getenv();
/*
 * DisallowPopdown
 *
 */

void
DisallowPopdown(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	Boolean * flag = (Boolean *)call_data;

	if (flag) *flag = False;

} /* DisallowPopdown */

XtArgVal GetValue(wid, resource)
	Widget	wid;
	String	resource;
{
	static XtArgVal	value;
	Arg	arg[1];

	XtSetArg(arg[0], resource, &value);
	XtGetValues(wid, arg, 1);
	return(value);
}

void SetValue(wid, resource, value)
	Widget		wid;
	String		resource;
	XtArgVal	value;
{
	Arg	arg[1];

	XtSetArg(arg[0], resource, value);
	XtSetValues(wid, arg, 1);
	return;
}

Boolean
AcceptFocus(w)
Widget	w;
{

	Time	time;
	time = XtLastTimestampProcessed(XtDisplay(w));
	if (OlCanAcceptFocus(w, time))
	{
	 	OlSetInputFocus(w, RevertToNone, time);
		return (True);
	}

    return (False);

} /* AcceptFocus() */

void
rexit(i, description, additional)
int	i;
char	*description;
char	*additional;
{

	char *	x;
	char	err_buf[BUFSIZ];

	x = getenv("XWINHOME");
	(void)sprintf( err_buf, "%s/desktop/rft/dtmsg \"%s %s\"",
		x, GGT(description), additional); 
	system(err_buf);
	(void)exit( i );
} /* rexit */

/* Get the user's UNIX login name */
char *
GetUser(login)
char *login;
{
	int	userid;
	struct	passwd	*nptr;

	userid = getuid();
	if ((nptr = getpwuid(userid)) == (struct passwd *)NULL)
		return((char *)NULL);
	(void)strcpy(login, nptr->pw_name);
	return(nptr->pw_name);
} /* GetUser */
