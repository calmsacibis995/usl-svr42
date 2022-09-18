#ident	"@(#)r4spider:util.c	1.3"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 *	Spider
 *
 *	(c) Copyright 1989, Donald R. Woods and Sun Microsystems, Inc.
 *	(c) Copyright 1990, David Lemke and Network Computing Devices Inc.
 *
 *	See copyright.h for the terms of the copyright.
 *
 *	@(#)util.c	2.1	90/04/27
 *
 */

/*
 * Sun libc has strdup() and usleep(), but other OS's need
 * them
 */
#include <X11/Xos.h>
#ifndef att
#ifndef sun 
#define	LOCAL_USLEEP
#define	LOCAL_STRDUP
#endif
#endif
/*
 * misc utility funcs
 */

#include	"defs.h"
#include	"globals.h"
#ifndef KITLESS
#include	<sys/file.h>
#endif /* KITLESS */
#ifdef XAW
#include	"xaw_ui.h"
#endif /* XAW */
#include	<ctype.h>
#include	<string.h>
#include	<pwd.h>

#define	NUM_RETRIES	5

int	replayTime = 200;

#ifndef XVIEW
/*
 * gets current PRIMARY selection
 *
 * this is a pretty gross hack, but it works...
 */
char	*
get_selection()
{
static Atom	selection = (Atom) 0;
static Atom	target = (Atom) 0;
Window	win;
unsigned char	*prop;
XSelectionEvent	ev;
Atom	type;
int	format;
unsigned long	elmts, left;
int	retry = 0;

	if (!selection)	{
		selection = XInternAtom(dpy, "PRIMARY", False);
		target = XInternAtom(dpy, "STRING", False);
	}

	win = XGetSelectionOwner(dpy, selection);

	if (win == None)	/* nobody owns it */
		return (NULL);

#ifdef XAW
	{
	String	str;
	XawTextPosition	start, end;
	Arg	args[1];
	XawTextBlock	text;

	XtSetArg(args[0], XtNstring, &str);
	if (helptext && win == XtWindow(helptext))	{
		XawTextGetSelectionPos(helptext, &start, &end);
		XawTextSourceRead(XawTextGetSource(helptext),
			start, &text, end - start);
	} else if (win == XtWindow(file))	{
		XawTextGetSelectionPos(file, &start, &end);
		XawTextSourceRead(XawTextGetSource(file),
			start, &text, end - start);
	} else	{
		return(NULL);
	   /*
		goto skip;
	   */
	}
	prop = (unsigned char *)malloc(end - start + 1);
	(void)strncpy((char *)prop, text.ptr, end - start);
	prop[end - start] = '\0';
	return ((char *)prop);
	}
/*
 *  WIPRO : Neeti
 *  CHANGE # UNKNOWN
 *  FILE # util.c
 *  1. prop has been type cast to (char *) in strncpy.
 *  2. XLib code after this point in this function should NOT be used
 *     for XAW code. 'skip' label is hence not required .
 *  ENDCHANGE # UNKNOWN
 */

/*
	skip:
*/
#else /* XAW */

	XConvertSelection(dpy, selection, target, None, table, CurrentTime);

	XSync(dpy, 0);

	/* wait for notification */
	while(XCheckTypedEvent(dpy, SelectionNotify, (XEvent*)&ev) == False)	{
		XSync(dpy, 0);
		if (retry++ == NUM_RETRIES)
			return (NULL);
		sleep(1);
	}

	if (ev.property == None)	/* nothing to get */
		return (NULL);

	(void)XGetWindowProperty(dpy, table, ev.property, 0L, 1024L,
		False, AnyPropertyType, &type, &format,
		&elmts, &left, &prop);
	
	assert(type == target);

	if (format != 8)	/* only want chars */
		return (NULL);

	return ((char *)prop);
#endif /*XAW */
}
#endif /* XVIEW */

#ifdef XAW
char	*helpDir;

/*
 * see if all the help files are there
 */
Bool
can_get_help_files(helpfiles)
char	helpfiles[6][256];
{
int	i;

	(void)sprintf(helpfiles[0], "%s/doc.intro", helpDir);
	(void)sprintf(helpfiles[1], "%s/doc.rules", helpDir);
	(void)sprintf(helpfiles[2], "%s/doc.controls", helpDir);
	(void)sprintf(helpfiles[3], "%s/doc.examples", helpDir);
	(void)sprintf(helpfiles[4], "%s/doc.misc", helpDir);
	(void)sprintf(helpfiles[5], "%s/doc.summary", helpDir);

	for (i = 0; i < 6; i++)	{
		if (access(helpfiles[i], R_OK) == -1)	{
			return False;
		}
	}
	return True;
}
#endif /* XAW */


char	*
remove_newlines(str)
char	*str;
{
char	*newstr;
char	*n;
extern char	*getenv();

	/* pad it generously to provide for tilde expansion */
	n = newstr = (char *)calloc((unsigned)(strlen(str) + 256), 1);

	/* remove leading whitespace */
	while (isspace(*str))	{
		str++;
	}

	/* tilde expansion */
	if (*str == '~')	{
		/* user */
		if (*(str + 1) == '/')	{
			(void)strcpy(newstr, getenv("HOME"));
		} else	{
			char	uname[20], *t;
			struct passwd	*pwd;
			int	len;

			t = strchr(str + 1, '/');
			if (t)	{
				len = t - str - 1;
			} else	{
				len = strlen(str);
			}
			(void)strncpy(uname, str + 1, len);
			uname[len] = '\0';
			if (pwd = getpwnam(uname))	{
				(void)strcpy(newstr, pwd->pw_dir);
				str += len;
			}
		}
		n += strlen(newstr);
		str++;
	}

	/* strip newlines in selection */
	while (*str)	{
		if (*str != '\n')
			*n++ = *str;
		str++;
	}
	*n = '\0';
	return (newstr);
}

#ifndef XVIEW
void
delay()
{
	if (replayTime)
		usleep((unsigned)replayTime);
}

#ifdef	LOCAL_USLEEP

#include <signal.h>
#include <time.h>

usleep(value)
long value;
{
	void stopme();
	struct itimerval ntval, otval;
	long		tmp;

	getitimer(ITIMER_REAL,&otval);
	ntval.it_interval.tv_sec = 0;
	ntval.it_interval.tv_usec = 0;
	ntval.it_value.tv_sec = otval.it_value.tv_sec ;
	tmp = otval.it_value.tv_usec + value;
	ntval.it_value.tv_sec += tmp/1000000;
	ntval.it_value.tv_usec = tmp % 1000000;
	signal(SIGALRM, stopme);
	if ( (otval.it_value.tv_sec < ntval.it_value.tv_sec) ||
		((otval.it_value.tv_sec == ntval.it_value.tv_sec) &&
		 (otval.it_value.tv_usec < ntval.it_value.tv_usec)))
	{
			setitimer(ITIMER_REAL, &ntval, &otval);
			pause();
	}
/*
 *	WIPRO : Neeti
 *	CHANGE # UNKNOWN
 *	FILE # util.c
 * 	usleep function has been improved. 
 *	1. A very large 'value' is now correctly handled.
 *	2. However, in case of a very low 'value' the possiblity  
 *	   of SIGALRM coming before the pause function still
 *	   exists. Then the fuction waits forever and spider will hang.
 *	ENDCHANGE # UNKNOWN
 */
}

void
stopme()
{
	signal(SIGALRM, SIG_DFL);
}
#endif	/* LOCAL_USLEEP */

#endif /* XVIEW */

#ifndef MEMUTIL
#ifdef	LOCAL_STRDUP
char	*
strdup(s)
char	*s;
{
	return strcpy(malloc((unsigned) strlen(s) + 1), s);
}
#endif	/* LOCAL_STRDUP */
#endif /* MEMUTIL */
