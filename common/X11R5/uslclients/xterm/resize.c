/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xterm:resize.c	1.2.1.16"
#endif

/*
 resize.c (C source file)
	Acc: 601052375 Tue Jan 17 09:59:35 1989
	Mod: 601054123 Tue Jan 17 10:28:43 1989
	Sta: 605994800 Wed Mar 15 14:53:20 1989
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
#include <stdio.h>
#include <ctype.h>

/*
 * Release 5 : SYSV is not defined on cc line any more in R5
 */
#ifdef SVR4
#define SYSV YES
#endif

#ifdef SYSV
#include <sys/termio.h>
#else	/* SYSV */
#include <sgtty.h>
#endif	/* SYSV */

#ifndef SVR4
#include	<sys/stream.h>	/* ehr3 - for typedef used in ptem.h */
#include	<sys/ptem.h>	/* ehr3 - for struct winsize */
#endif  /* SVR4 */

#include <signal.h>
#include <pwd.h>
#ifdef SYSV
extern struct passwd *getpwent();
extern struct passwd *getpwuid();
extern struct passwd *getpwnam();
extern void setpwent();
extern void endpwent();
extern struct passwd *fgetpwent();
#define	bzero(s, n)	memset(s, 0, n)

#endif	/* SYSV */

#define	EMULATIONS	2
#define	SUN		1
#define	TIMEOUT		10
#define	VT100		0

#define	SHELL_UNKNOWN	0
#define	SHELL_C		1
#define	SHELL_BOURNE	2
struct {
	char *name;
	int type;
} shell_list[] = {
	"csh",		SHELL_C,	/* vanilla cshell */
	"tcsh",		SHELL_C,
	"sh",		SHELL_BOURNE,	/* vanilla Bourne shell */
	"ksh",		SHELL_BOURNE,	/* Korn shell (from AT&T toolchest) */
	"ksh-i",	SHELL_BOURNE,	/* other name for latest Korn shell */
	(char *) 0,	SHELL_BOURNE,	/* last effort default (same as
					 * xterm's)
					 */
};

char *emuname[EMULATIONS] = {
	"VT100",
	"Sun",
};
char *myname;
int shell_type = SHELL_UNKNOWN;
char *getsize[EMULATIONS] = {
	"\0337\033[r\033[999;999H\033[6n",
	"\033[18t",
};
#ifndef sun
#ifdef TIOCSWINSZ
char *getwsize[EMULATIONS] = {
	0,
	"\033[14t",
};
#endif	/* TIOCSWINSZ */
#endif	/* sun */
char *restore[EMULATIONS] = {
	"\0338",
	0,
};
char *setname = "";
char *setsize[EMULATIONS] = {
	0,
	"\033[8;%s;%st",
};
#ifdef SYSV
struct termio tioorig;
#else	/* !SYSV */
struct sgttyb sgorig;
#endif	/* !SYSV */
char *size[EMULATIONS] = {
	"\033[%d;%dR",
	"\033[8;%d;%dt",
};
char sunname[] = "sunsize";
int tty;
FILE *ttyfp;
#ifndef sun
#ifdef TIOCSWINSZ
char *wsize[EMULATIONS] = {
	0,
	"\033[4;%hd;%hdt",
};
#endif	/* TIOCSWINSZ */
#endif	/* sun */

char *strindex (), *index (), *rindex();

static void onintr(dummy)
int dummy;
{
#ifdef SYSV
	ioctl (tty, TCSETAW, &tioorig);
#else	/* !SYSV */
	ioctl (tty, TIOCSETP, &sgorig);
#endif	/* SYSV */
	exit(1);
}


static void timeout(dummy)
int dummy;
{
	fprintf(stderr, "%s: Time out occurred\r\n", myname);
	onintr();
}


main (argc, argv)
char **argv;
/*
   resets termcap string to reflect current screen size
 */
{
	register char *ptr, *env;
	register int emu = VT100;
	char *shell;
	struct passwd *pw;
	int i;
	int rows, cols;
#ifdef SYSV
	struct termio tio;
#else	/* !SYSV */
	struct sgttyb sg;
	char termcap [1024];
	char newtc [1024];
#endif	/* !SYSV */
	char buf[BUFSIZ];
#ifdef sun
#ifdef TIOCSSIZE
	struct ttysize ts;
#endif	/* TIOCSSIZE */
#else	/* sun */
#ifdef TIOCSWINSZ
	struct winsize ws;
#endif	/* TIOCSWINSZ */
#endif	/* sun */
	char *getenv();

	if(ptr = rindex(myname = argv[0], '/'))
		myname = ptr + 1;
	if(strcmp(myname, sunname) == 0)
		emu = SUN;
	for(argv++, argc-- ; argc > 0 && **argv == '-' ; argv++, argc--) {
		switch((*argv)[1]) {
		 case 's':	/* Sun emulation */
			if(emu == SUN)
				Usage();	/* Never returns */
			emu = SUN;
			break;
		 case 'u':	/* Bourne (Unix) shell */
			shell_type = SHELL_BOURNE;
			break;
		 case 'c':	/* C shell */
			shell_type = SHELL_C;
			break;
		 default:
			Usage();	/* Never returns */
		}
	}

	if (SHELL_UNKNOWN == shell_type) {
		/* Find out what kind of shell this user is running.
		 * This is the same algorithm that xterm uses.
		 */
		if (((ptr = getenv("SHELL")) == NULL || *ptr == 0) &&
		 (((pw = getpwuid(getuid())) == NULL) ||
		 *(ptr = pw->pw_shell) == 0))
			/* this is the same default that xterm uses */
			ptr = "/bin/sh";

		if (shell = rindex(ptr, '/'))
			shell++;
		else
			shell = ptr;

		/* now that we know, what kind is it? */
		for (i = 0; shell_list[i].name; i++)
			if (!strcmp(shell_list[i].name, shell))
				break;
		shell_type = shell_list[i].type;
	}

	if(argc == 2) {
		if(!setsize[emu]) {
			fprintf(stderr,
			 "%s: Can't set window size under %s emulation\n",
			 myname, emuname[emu]);
			exit(1);
		}
		if(!checkdigits(argv[0]) || !checkdigits(argv[1]))
			Usage();	/* Never returns */
	} else if(argc != 0)
		Usage();	/* Never returns */
	if((ttyfp = fopen("/dev/tty", "r+")) == NULL) {
		fprintf(stderr, "%s: Can't open /dev/tty\n", myname);
		exit(1);
	}
	tty = fileno(ttyfp);
#ifdef SYSV
	if(!(env = getenv("TERM")) || !*env) {
		env = "xterm";
		if(SHELL_BOURNE == shell_type)
			setname = "TERM=xterm;\nexport TERM;\n";
		else	setname = "setenv TERM xterm;\n";
	}
#else	/* !SYSV */
	if((env = getenv("TERMCAP")) && *env)
		strcpy(termcap, env);
	else {
		if(!(env = getenv("TERM")) || !*env) {
			env = "xterm";
			if(SHELL_BOURNE == shell_type)
				setname = "TERM=xterm;\nexport TERM;\n";
			else	setname = "setenv TERM xterm;\n";
		}
		if(tgetent (termcap, env) <= 0) {
			fprintf(stderr, "%s: Can't get entry \"%s\"\n",
			 myname, env);
			exit(1);
		}
	}
#endif	/* !SYSV */

#ifdef SYSV
	ioctl (tty, TCGETA, &tioorig);
	tio = tioorig;
	tio.c_iflag &= ~(ICRNL | IUCLC);
	tio.c_lflag &= ~(ICANON | ECHO);
	tio.c_cflag |= CS8;
	tio.c_cc[VMIN] = 6;
	tio.c_cc[VTIME] = 1;
#else	/* !SYSV */
 	ioctl (tty, TIOCGETP, &sgorig);
	sg = sgorig;
	sg.sg_flags |= RAW;
	sg.sg_flags &= ~ECHO;
#endif	/* !SYSV */
	signal(SIGINT, onintr);
	signal(SIGQUIT, onintr);
	signal(SIGTERM, onintr);
#ifdef SYSV
	ioctl (tty, TCSETAW, &tio);
#else	/* !SYSV */
	ioctl (tty, TIOCSETP, &sg);
#endif	/* !SYSV */

	if (argc == 2) {
		sprintf (buf, setsize[emu], argv[0], argv[1]);
		write(tty, buf, strlen(buf));
	}
	write(tty, getsize[emu], strlen(getsize[emu]));
	readstring(ttyfp, buf, size[emu]);
	if(sscanf (buf, size[emu], &rows, &cols) != 2) {
		fprintf(stderr, "%s: Can't get rows and columns\r\n", myname);
		onintr();
	}
	if(restore[emu])
		write(tty, restore[emu], strlen(restore[emu]));
#ifdef sun
#ifdef TIOCGSIZE
	/* finally, set the tty's window size */
	if (ioctl (tty, TIOCGSIZE, &ts) != -1) {
		ts.ts_lines = rows;
		ts.ts_cols = cols;
		ioctl (tty, TIOCSSIZE, &ts);
	}
#endif	/* TIOCGSIZE */
#else	/* sun */
#ifdef TIOCGWINSZ
	/* finally, set the tty's window size */
	if(getwsize[emu]) {
	    /* get the window size in pixels */
	    write (tty, getwsize[emu], strlen (getwsize[emu]));
	    readstring(ttyfp, buf, wsize[emu]);
	    if(sscanf (buf, wsize[emu], &ws.ws_xpixel, &ws.ws_ypixel) != 2) {
		fprintf(stderr, "%s: Can't get window size\r\n", myname);
		onintr();
	    }
	    ws.ws_row = rows;
	    ws.ws_col = cols;
	    ioctl (tty, TIOCSWINSZ, &ws);
	} else if (ioctl (tty, TIOCGWINSZ, &ws) != -1) {
	    /* we don't have any way of directly finding out
	       the current height & width of the window in pixels.  We try
	       our best by computing the font height and width from the "old"
	       struct winsize values, and multiplying by these ratios...*/
	    if (ws.ws_xpixel != 0)
	        ws.ws_xpixel = cols * (ws.ws_xpixel / ws.ws_col);
	    if (ws.ws_ypixel != 0)
	        ws.ws_ypixel = rows * (ws.ws_ypixel / ws.ws_row);
	    ws.ws_row = rows;
	    ws.ws_col = cols;
	    ioctl (tty, TIOCSWINSZ, &ws);
	}
#endif	/* TIOCGWINSZ */
#endif	/* sun */

#ifdef SYSV
	ioctl (tty, TCSETAW, &tioorig);
#else	/* !SYSV */
	ioctl (tty, TIOCSETP, &sgorig);
#endif	/* !SYSV */
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGTERM, SIG_DFL);

#ifndef SYSV
	/* update termcap string */
	/* first do columns */
	if ((ptr = strindex (termcap, "co#")) == NULL) {
		fprintf(stderr, "%s: No `co#'\n", myname);
		exit (1);
	}
	strncpy (newtc, termcap, ptr - termcap + 3);
	sprintf (newtc + strlen (newtc), "%d", cols);
	ptr = index (ptr, ':');
	strcat (newtc, ptr);

	/* now do lines */
	if ((ptr = strindex (newtc, "li#")) == NULL) {
		fprintf(stderr, "%s: No `li#'\n", myname);
		exit (1);
	}
	strncpy (termcap, newtc, ptr - newtc + 3);
	sprintf (termcap + ((int) ptr - (int) newtc + 3), "%d", rows);
	ptr = index (ptr, ':');
	strcat (termcap, ptr);
#endif	/* !SYSV */

	if(SHELL_BOURNE == shell_type)
#ifdef SYSV
		printf ("%sCOLUMNS=%d;\nLINES=%d;\nexport COLUMNS LINES;\n",
		 setname, cols, rows);
#else	/* !SYSV */
		printf ("%sTERMCAP='%s'\n",
		 setname, termcap);
#endif	/* !SYSV */
	else
#ifdef SYSV
		printf ("set noglob;\n%ssetenv COLUMNS '%d';\nsetenv LINES '%d';\nunset noglob;\n",
		 setname, cols, rows);
#else	/* !SYSV */
		printf ("set noglob;\n%ssetenv TERMCAP '%s';\nunset noglob;\n",
		 setname, termcap);
#endif	/* !SYSV */
	exit(0);
}

char *strindex (s1, s2)
/*
   returns a pointer to the first occurrence of s2 in s1, or NULL if there are
   none.
 */
register char *s1, *s2;
{
	register char *s3;

	while ((s3 = index (s1, *s2)) != NULL)
	{
		if (strncmp (s3, s2, strlen (s2)) == 0) return (s3);
		s1 = ++s3;
	}
	return (NULL);
}

checkdigits(str)
register char *str;
{
	while(*str) {
		if(!isdigit(*str))
			return(0);
		str++;
	}
	return(1);
}

readstring(fp, buf, str)
register FILE *fp;
register char *buf;
char *str;
{
	register int last;
/*FLH
	struct itimerval it;
*/

	signal(SIGALRM, timeout);
/*FLH
	bzero((char *)&it, sizeof(struct itimerval));
	it.it_value.tv_sec = TIMEOUT;
*/

	/* ehr3 - We don't have ITIMER_REAL
		setitimer(ITIMER_REAL, &it, (struct itimerval *)NULL);
	*/

	if((*buf++ = getc(fp)) != *str) {
		fprintf(stderr, "%s: unknown character, exiting.\r\n", myname);
		onintr();
	}
	last = str[strlen(str) - 1];
	while((*buf++ = getc(fp)) != last);
/*FLH
	bzero((char *)&it, sizeof(struct itimerval));
*/
	/* ehr3 - We don't have ITIMER_REAL
		setitimer(ITIMER_REAL, &it, (struct itimerval *)NULL);
	*/
	*buf = 0;
}

Usage()
{
	fprintf(stderr, strcmp(myname, sunname) == 0 ?
	 "Usage: %s [rows cols]\n" :
	 "Usage: %s [-u] [-c] [-s [rows cols]]\n", myname);
	exit(1);
}
