/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/utils.c	1.9"
#endif

#include <sys/utsname.h>
#include <pwd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <string.h>
#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Error.h>
#include "dtcopy.h"
#include "uucp.h"

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

char *
GetNodeName(node)
char *node;
{
	struct utsname  sname;

	uname(&sname);
	(void)strcpy(node, sname.nodename);
	return(sname.nodename);
} /* GetNodeName */

/*
 * get the login name of a user
 */

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
}

int
file_type(file)
char	*file;
{
	struct	stat	sb;
	int	mode;

	if (stat(file, &sb) < 0)
		return(-1);

	mode = sb.st_mode & S_IFMT;

	switch (mode) {
		case S_IFREG:
			return(0);
		case S_IFDIR:
			return(1);
		default:
			return(-1);
	}
}

char  *
_cleanline(s)
char *s;
{
	char  *ns;

	*(s + strlen((char *)s) -1) = (char) 0; /* delete newline */
	if (!strlen((char *)s))
		return(s);
	ns = s + strlen((char *)s) - 1; /* s->start; ns->end */
	while ((ns != s) && (isspace((char)*ns))) {
		*ns = (char)0;	/* delete terminating spaces */
		--ns;
		}
	while (*ns)             /* delete beginning white spaces */
		if (isspace((char)*s))
			++s;
		else
			break;
	return(s);
}

void
rexit(i, description, additional)
int	i;
char	*description;
char	*additional;
{

	char *	x;
	char	err_buf[BUF_SIZE];

	x = getenv("XWINHOME");
	(void)sprintf( err_buf, "%s/desktop/rft/dtmsg \"%s %s\"&",
		x, GGT(description), additional); 
	system(err_buf);
	(void)exit( i );
} /* rexit */

/*
 * append f1 to f2
 *	f1	-> source FILE pointer
 *	f2	-> destination FILE pointer
 * return:
 *	True	-> ok
 *	False	-> failed
 */
Boolean
F1AppendF2(fp1, fp2)
register FILE	*fp1, *fp2;
{
	register int nc;
	char	buf[BUFSIZ];

	while ((nc = fread(buf, sizeof (char), BUFSIZ, fp1)) > 0)
		(void) fwrite(buf, sizeof (char), nc, fp2);

	return(ferror(fp1) || ferror(fp2) ? False : True);
} /*F1AppendF2 */


void
MailUser(user, str, errfile)
char *user, *str, *errfile;
{
	register FILE *fp, *fi;
	char cmd[BUFSIZ];
	char *c;

	(void) sprintf(cmd, "%s mail '%s'", BIN, user);
	if ((fp = popen(cmd, "w")) == NULL)
		return;
	(void) fprintf(fp, "\n%s\n", str);

	/* copy back stderr */
	if (*errfile != '\0' && (fi = fopen(errfile, "r")) != NULL) {
		if (F1AppendF2(fi, fp) != True)
			fputs("\n\t===== well, i tried =====\n", fp);
		(void) fclose(fi);
		fputc('\n', fp);
	}
	pclose(fp);
} /* MailUser */

int
ExecProgram(eip)
ExecItem *eip;
{
	switch (eip->pid = fork()) {
		case 0: /* child */
			execvp(eip->exec_argv[0], eip->exec_argv);
			perror(eip->exec_argv[0]);
			exit(255);
		case -1:
			return (-1);
		default:
			/* maybe set the button insensitive */
			return (0);
	}
			
} /* ExecProgram */
