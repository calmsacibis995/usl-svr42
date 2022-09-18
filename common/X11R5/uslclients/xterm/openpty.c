/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xterm:openpty.c	1.1.1.29"
#endif

/*
 openpty.c (C source file)
	Acc: 601052337 Tue Jan 17 09:58:57 1989
	Mod: 601054121 Tue Jan 17 10:28:41 1989
	Sta: 601054121 Tue Jan 17 10:28:41 1989
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

#include <stdio.h>
#include "ptyx.h"		/* EHR3 - port */
#include <signal.h>
#include <poll.h>
#include <stropts.h>
#include <sys/termio.h>		/* for TIOCSWINSZ */
#ifndef SVR4
#include	<sys/stream.h>
#include	<sys/ptem.h>
#endif /* SVR4 */

#include "Strings.h"
#include "messages.h"

extern	char	*getenv();

#ifdef SVR4
extern  char    *ptsname();
#endif /* SVR4 */
/* EHR3 - end port */

#ifdef SVR4 				/* includes for console logging */
#include <unistd.h>			/* for ioctl */
#include <sys/strlog.h>
#include <syslog.h>
#endif

#define NFDS 20

extern int errno;


#ifdef SVR4
char console_has_input = 0;
struct pollfd pfd[1];

static void sigpoll(dummy)
int dummy;
{
	
	if (poll(pfd, 1, 0) < 0){
#ifdef DEBUG
		printf("poll of console log driver failed
#endif
		return;
	}	
	
			/* check for input from the console log driver */
	if (pfd[0].revents & POLLIN){
		pfd[1].revents = 0;
		console_has_input = 1;	
	}	
	(void) signal(SIGPOLL, sigpoll);
}
#endif

openpty(command_to_exec, screen)
char	**command_to_exec;
TScreen	*screen;
{
	register char *shell;
	register int fd;
	extern char *getenv();
	int fds[2];
/* SS-port */
	extern XtermWidget term;
/* SS-end-port */
/* ehr3 - TIOCSWINSZ */
	struct	winsize ws;
/* ehr3 - end TIOCSWINSZ */
#ifdef I18N
Arg args[5];
int i;
String xnlLanguage;
String inputLang;
String displayLang;
String timeFormat;
String numeric;

char xnlLanguage_announce[64];
char inputLang_announce[64];
char displayLang_announce[64];
char timeFormat_announce[64];
char numeric_announce[64];
#endif

	extern struct termios d_tio;

	errno = 0;
	if ((fds[0] = open("/dev/ptmx", O_RDWR)) < 0)
	{
#if !defined(I18N)
		printf("spipe: open 0 failed, errno=%d\n",errno);
#else
		OlVaDisplayWarningMsg(screen->display, OleNopen, OleTopenpty1,
					OleCOlClientXtermMsgs, OleMopen_openpty1,
					errno, NULL);
#endif
		return(-1);
	}

/* SS - port */
	if ((term->screen.pid = fork()) == 0) {
/* SS - end port */
		setpgrp();
		
		grantpt(fds[0]);
		unlockpt(fds[0]);

		errno = 0;
		if ((fds[1] = open(ptsname(fds[0]), O_RDWR)) < 0)
		{
#if !defined(I18N)
			printf("spipe: open 1 failed, errno=%d\n",errno);
#else
			OlVaDisplayWarningMsg(screen->display, OleNopen, OleTopenpty2,
					OleCOlClientXtermMsgs, OleMopen_openpty2,
					errno, NULL);
#endif
			return(-1);
		}

		if ( ioctl(fds[1], I_PUSH, "ptem") < 0)
		{
#if !defined(I18N)
			fprintf(stderr,"ptem failed\n");
#else
			OlVaDisplayWarningMsg(screen->display, OleNioctl, OleTptem,
					OleCOlClientXtermMsgs, OleMioctl_ptem,
					NULL);
#endif
			return -1;
		}

#ifndef u3b2
		if (!getenv("CONSEM") && ioctl(fds[1], I_PUSH, "consem") < 0)
		{
#if !defined(I18N)
			fprintf(stderr, "consem failed\n");
#else
			OlVaDisplayWarningMsg(screen->display, OleNioctl, OleTconsem,
					OleCOlClientXtermMsgs, OleMioctl_consem,
					NULL);
#endif
			return -1;
		}
#endif /* u3b2 */

		if ( ioctl(fds[1],I_PUSH,"ldterm") < 0)
		{
#if !defined(I18N)
			fprintf(stderr,"ldterm failed\n");
#else
			OlVaDisplayWarningMsg(screen->display, OleNioctl, OleTldterm,
					OleCOlClientXtermMsgs, OleMioctl_ldterm,
					NULL);
#endif
			return -1;
		}

#if defined(SVR4)
		if ( ioctl(fds[1],I_PUSH,"ttcompat") < 0)
		{
#if !defined(I18N)
			fprintf(stderr,"ttcompat failed\n");
#else
			OlVaDisplayWarningMsg(screen->display, OleNioctl, OleTttcompat,
					OleCOlClientXtermMsgs, OleMioctl_ttcompat,
					NULL);
#endif
			return -1;
		}
#endif /* SVR4 */

		if (!(shell = getenv("SHELL")))
			shell = "/bin/sh";

		(void)dup2(fds[1], 0);
		(void)dup2(0, 1);
		(void)dup2(0, 2);

#ifdef TIOCSWINSZ	/* ehr3 */
		/* tell tty how big window is */	/* ehr3 */
#ifdef TEK
		if (screen->TekEmu)
		{
		    ws.ws_row = 35;
		    ws.ws_col = 74;
		    ws.ws_xpixel = TFullWidth(screen);
		    ws.ws_ypixel = TFullHeight(screen);
		}
		else
#endif /* TEK */
		{
		    ws.ws_row = screen->max_row + 1; /* ehr3 */
		    ws.ws_col = screen->max_col + 1; /* ehr3 */
		    ws.ws_xpixel = FullWidth(screen); /* ehr3 */
		    ws.ws_ypixel = FullHeight(screen); /* ehr3 */
		}
	
		if (ioctl (1, TIOCSWINSZ, &ws) == -1) {
#if !defined(I18N)
			(void) fprintf(stderr, "TIOCSWINSZ failed in main.c\n");
			perror("	Reason");
#else
			OlVaDisplayWarningMsg(screen->display, OleNioctl, OleTwinSz2,
					OleCOlClientXtermMsgs, OleMioctl_winSz2,
					NULL);
			perror( OlGetMessage(screen->display, NULL, 0, OleNperror,
				OleTreason, OleCOlClientXtermMsgs,
				OleMperror_reason, NULL));

#endif
		}
#endif	/* TIOCSWINSZ */	/* ehr3 */

		for (fd=3; fd < NFDS; fd++)
			(void)close(fd);

		if (ioctl (1, TCSETS, &d_tio) == -1)
#if !defined(I18N)
		        fprintf(stderr,"ioctl TCSETA failed in openpty()\n");
#else
			OlVaDisplayWarningMsg(screen->display, OleNioctl, OleTtcseta,
					OleCOlClientXtermMsgs, OleMioctl_tcseta,
					NULL);
#endif


		signal(SIGINT, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
#ifdef I18N
		i = 0;
		XtSetArg(args[i], XtNxnlLanguage, &xnlLanguage);i++;
		XtSetArg(args[i], XtNinputLang, &inputLang);i++;
		XtSetArg(args[i], XtNdisplayLang, &displayLang);i++;
		XtSetArg(args[i], XtNtimeFormat, &timeFormat);i++;
		XtSetArg(args[i], XtNnumeric, &numeric);i++;

		OlGetApplicationValues((Widget) NULL, args, i);	
		if (xnlLanguage && *xnlLanguage != NULL){
			strcpy(xnlLanguage_announce, "LANG=");
			strcat(xnlLanguage_announce, xnlLanguage);
			if (putenv(xnlLanguage_announce) != 0){
				OlVaDisplayWarningMsg(screen->display,
				 OleNputenv, OleTbadPutenv,
				 OleCOlClientXtermMsgs, OleMputenv_badPutenv,
				 NULL);
			}
		}	
		if (inputLang && *inputLang != NULL){
			strcpy(inputLang_announce, "LC_CTYPE=");
			strcat(inputLang_announce, inputLang);
			if (putenv(inputLang_announce) != 0){
				OlVaDisplayWarningMsg(screen->display,
				 OleNputenv, OleTbadPutenv,
				 OleCOlClientXtermMsgs, OleMputenv_badPutenv,
				 NULL);
			}
		}	
		if (displayLang && *displayLang != NULL){
			strcpy(displayLang_announce, "LC_MESSAGES=");
			strcat(displayLang_announce, displayLang);
			if (putenv(displayLang_announce) != 0){
				OlVaDisplayWarningMsg(screen->display,
				 OleNputenv, OleTbadPutenv,
				 OleCOlClientXtermMsgs, OleMputenv_badPutenv,
				 NULL);
			}
		}	
		if (timeFormat && *timeFormat != NULL){
			strcpy(timeFormat_announce, "LC_TIME=");
			strcat(timeFormat_announce, timeFormat);
			if (putenv(timeFormat_announce) != 0){
				OlVaDisplayWarningMsg(screen->display,
				 OleNputenv, OleTbadPutenv,
				 OleCOlClientXtermMsgs, OleMputenv_badPutenv,
				 NULL);
			}
		}	
		if (numeric && *numeric != NULL){
			strcpy(numeric_announce, "LC_NUMERIC=");
			strcat(numeric_announce, numeric);
			if (putenv(numeric_announce) != 0){
				OlVaDisplayWarningMsg(screen->display,
				 OleNputenv, OleTbadPutenv,
				 OleCOlClientXtermMsgs, OleMputenv_badPutenv,
				 NULL);
			}
		}	
#endif

		if (command_to_exec)
			execvp(*command_to_exec, command_to_exec);
		else  {
			extern	char	*getenv();
			char	*shell, *shname, *shname_minus;

			if (!(shell = getenv("SHELL")))
                                shell = "/bin/sh";

			if (shname = rindex(shell, '/'))
                            shname++;
	 	 	else
			    shname = shell;
			shname_minus = (char *) malloc(strlen(shname) + 2);
			(void) strcpy(shname_minus, "-");
			(void) strcat(shname_minus, shname);


                        execl(shell, term->misc.login_shell ? shname_minus : shname, (char *)0);
                }

		_exit(1);
	}
	return(fds[0]);
}


#ifdef SVR4
/*
 *		init_console:
 *							1) open console log device
 *							2) register to receive console messages
 *							3) store fd in screen->console
 *							4) return
 *							
 *							**) if an error occurs, exit
 */

void
init_console(screen)
TScreen *screen;
{
	struct strioctl ioc;
	int console;

                /* open logger device */
	console = open("/dev/log",O_RDONLY);
		if (console <= 0){
#if !defined(I18N)
			XtError("could not open console");
#else
			/* fatal error */
			OlVaDisplayErrorMsg(screen->display, OleNopen, OleTbadConsole2,
					OleCOlClientXtermMsgs, OleMopen_badConsole2,
					NULL);
#endif
		}
			/* register to receive console messages */
	ioc.ic_cmd = I_CONSLOG;
	ioc.ic_timout = 0;
	ioc.ic_len = 0;
	ioc.ic_dp = (char *) NULL;
	if (ioctl(console, I_STR, &ioc) < 0){
#if !defined(I18N)
		XtError("Cannot register to receive console log messages");
#else
			OlVaDisplayErrorMsg(screen->display, OleNioctl, OleTbadConsole,
					OleCOlClientXtermMsgs, OleMioctl_badConsole,
					NULL);
#endif
	}
	if(ioctl(console, I_SETSIG, S_INPUT) < 0){
#if !defined(I18N)
		XtError("Cannot register to receive console log messages");
#else
			OlVaDisplayErrorMsg(screen->display, OleNioctl, OleTbadConsole,
					OleCOlClientXtermMsgs, OleMioctl_badConsole,
					NULL);
#endif
	}
		/* register event handler */
	signal(SIGPOLL, sigpoll);
		/* store file descriptor in screen structure */
	screen->console = console;
		/* store fd in pfd struct for poll() */
	pfd[0].fd = console;
	pfd[0].events = POLLIN;
	return;
}

/*
 *	console_input: read input from console logger into buffer
 *						return number of bytes read
 *						if error, return -1
 */
struct log_ctl ctl_buf;	
struct strbuf ctl,dat;

int
console_input(fd,buf,size)
int fd;
Char *buf;
int size;
{
	int flags = 0;
	static char initialized = 0;
	

			/* initialize data structs for reading from stream */
	if (!initialized){
      initialized = (char) 1;
      ctl.maxlen = sizeof(struct log_ctl);   
      ctl.buf = (void *) &ctl_buf;
   }

			/* set up to read into caller's buffer */
	dat.len = 0;
   dat.maxlen = size;   /* leave room for null pad */
	dat.buf =  (char *) buf;
			/* read input from console log device */
	if ((getmsg(fd,&ctl,&dat,&flags)) < 0){
      return(-1);
   }
			/* null pad the message */
	dat.buf[dat.len] = (char) NULL;
#ifdef DEBUG
	printf("%s",dat.buf);
#endif
			/* return the number of bytes read */
	return(dat.len);
}
#endif
