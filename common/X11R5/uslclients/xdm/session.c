/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)xdm:session.c	1.21"
/*
 * xdm - display manager daemon
 *
 * $XConsortium: session.c,v 1.55 91/09/19 16:25:56 keith Exp $
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
 * session.c
 */

# include "dm.h"
# include "dtlogin.h"
# include <X11/Xlib.h>
# include <X11/keysym.h>
# include <signal.h>
# include <X11/Xatom.h>
# include <errno.h>
# include <stdio.h>
# include <stdlib.h>
# include <ctype.h>
# include <iaf.h>
# include <priv.h>
# include <deflt.h>
# include <sys/types.h>
# include <sys/param.h>
# include <sys/stat.h>
#ifdef SECURE_RPC
# include <rpc/rpc.h>
# include <rpc/key_prot.h>
#endif

extern int  		errno;
extern Widget		toplevel;
extern XtAppContext	context;
extern Login_info	*linfo;
extern int		Ptty_fd;
extern int		loginProblem;
extern int		Slaveptty_fd;
extern void		Sys_Shutdown ();
extern void		PopupPasshelp ();
extern SIGVAL		StopAll ();
extern void		Sys_Shutdown ();
extern char		*getEnv ();
extern char		**environ;
extern int		CreateEnv ();
extern int		RemovePidFile ();
extern int		StoreUserPid ();

int	passwdStored = 0;
int	pwNeedDone = 0;
int	pwSuccess = 0;
int	pwMaxTrys = 0;
char	*errStr;
char	oldPassword[128];
char	newPassword[128];

static int			clientPid;
static struct greet_info	greet;
static struct verify_info	verify;
static char			*BINPASSWD = "/usr/bin/passwd";

static Jmp_buf	abortSession;

/* ARGSUSED */
static SIGVAL
catchTerm (n)
    int n;
{
    Longjmp (abortSession, 1);
}

static Jmp_buf	pingTime;

/* ARGSUSED */
static SIGVAL
catchAlrm (n)
    int n;
{
    Longjmp (pingTime, 1);
}

SessionPingFailed (d)
    struct display  *d;
{
    if (clientPid > 1)
    {
    	AbortClient (clientPid);
	source (verify.systemEnviron, d->reset);
    }
    SessionExit (d, RESERVER_DISPLAY, TRUE);
}

/*
 * We need our own error handlers because we can't be sure what exit code Xlib
 * will use, and our Xlib does exit(1) which matches REMANAGE_DISPLAY, which
 * can cause a race condition leaving the display wedged.  We need to use
 * RESERVER_DISPLAY for IO errors, to ensure that the manager waits for the
 * server to terminate.  For other X errors, we should give up.
 */

/*ARGSUSED*/
static
IOErrorHandler (dpy)
    Display *dpy;
{
    extern char *sys_errlist[];
    extern int sys_nerr;
    char *s = ((errno >= 0 && errno < sys_nerr) ? sys_errlist[errno]
						: "unknown error");

    LogError("fatal IO error %d (%s)\n", errno, s);
    exit(RESERVER_DISPLAY);
}

static int
ErrorHandler(dpy, event)
    Display *dpy;
    XErrorEvent *event;
{
    LogError("X error\n");
    if (XmuPrintDefaultErrorMessage (dpy, event, stderr) == 0) return 0;
    exit(UNMANAGE_DISPLAY);
    /*NOTREACHED*/
}

ManageSession (d)
struct display	*d;
{
    int			pid, code, i;
    int			notDone = 1, ret;
    int			reinvoke = 0;
    char		**ava, *tty;
    char		*p;
    char		xlogname[137];
    char		xpasswd[137];
    char		xtty[137];
    waitType		status;
    Display		*dpy, *InitGreet ();
    XEvent		event;
    int         	cnt;
    char        	buf[64];
    KeySym      	keysym;
    XComposeStatus      compose;


    if (account (ttyname (Slaveptty_fd)))
	Debug ("Could not create utmp entry\n");

    Debug ("ManageSession %s\n", d->name);
    (void)XSetIOErrorHandler(IOErrorHandler);
    (void)XSetErrorHandler(ErrorHandler);
    SetTitle(d->name, (char *) 0);

    dpy = InitGreet (d);
    /*
     * Run the setup script - note this usually will not work when
     * the server is grabbed, so we don't even bother trying.
     */
    if (!d->grabServer)
	SetupDisplay (d);
    if (!dpy) {
	LogError ("Cannot reopen display %s for greet window\n", d->name);
	exit (RESERVER_DISPLAY);
    }
    while (notDone) {
	/*
	 * Greet user, requesting name/password
	 */
	code = Greet (d, &greet);
	if (code != 0)
	{
	    CloseGreet (d);
	    SessionExit (d, code, FALSE);
	}

	/*
	 * Put the logname and the passwd on the ava stream
	 */
	if ((ret = dup2 (Slaveptty_fd, 0)) == -1)
		Debug ("Problem with dup2\n");

	ava = retava(Slaveptty_fd);

	(void) bzero (xlogname, strlen (xlogname));
	(void) sprintf (xlogname, "XLOGNAME=%s", greet.name);
	if ((ava = putava (xlogname, ava)) == NULL)
		{
		Debug ("Could not set logname ava\n");
		}

	(void) bzero (xpasswd, strlen (xpasswd));
	(void) sprintf (xpasswd, "XPASSWD=%s", greet.password);
	if ((ava = putava (xpasswd, ava)) == NULL)
		{
		Debug ("Could not set passwd ava\n");
		}

	(void) bzero (xtty, strlen (xtty));
	(void) sprintf (xtty, "XTTY=%s", ttyname(Slaveptty_fd));
	if ((ava = putava (xtty, ava)) == NULL)
		{
		Debug ("Could not set tty ava\n");
		}

	if (setava (Slaveptty_fd, ava) != 0)
		{
		Debug ("Could not do setava\n");
		}

	/*
	 * invoke identification and authorizarion scheme
	 */
	switch (ret = invoke (Slaveptty_fd, "login"))
		{
		case LOGIN_SUCCESS:
			if (CreateEnv (greet.name, &verify, d, Slaveptty_fd))
				Debug ("CreateEnv failed\n");
			if (StoreUserPid (d->serverPid))
				Debug ("Could not store user's server pid\n");
			notDone = 0; 
			break;
		case LOGIN_FAIL:
			FailedLogin (d); 
			break;
		case INACTIVE:
			XBell (XtDisplay (linfo->login_fail), 0);
			PasswdAged (linfo); 
			break;
		case EXPIRED:
			XBell (XtDisplay (linfo->login_fail), 0);
			AccountAged (linfo); 
			break;
		case IDLEWEEKS:
			XBell (XtDisplay (linfo->login_fail), 0);
			PasswdAged (linfo); 
			break;
		case MANDATORY:
		case PFLAG:
		case AGED:
			errStr = (char *)malloc (512);
			(void) bzero (oldPassword, strlen (oldPassword));
			(void) strcpy (oldPassword, greet.password);
			XBell (XtDisplay (toplevel), 0);
			XtSetSensitive (linfo->login_text, FALSE);
			ava = retava(Slaveptty_fd);
			if ((p = getava ("XLOGNAME", ava)) == NULL) {
				Debug ("Could not get XLOGNAME\n");
				p = greet.name;
			}
			NewPasswd (p, ret);
			/* loop until correct passwd entered or to many trys */
			pwSuccess = 0;
			pwNeedDone = 0;
			pwMaxTrys = 0;
			passwdStored = 0;
			while (!pwNeedDone) {
				XtAppNextEvent (context, &event);
            			if (event.type == KeyPress)
                		    {
                		    cnt = XLookupString (&event, buf, 64, 
						&keysym, &compose);
		                    if (keysym == XK_F1)
                        		PopupPasshelp (toplevel, linfo);
                		    else
                		        XtDispatchEvent (&event);
                		    }
            			else
				    XtDispatchEvent (&event);
			}
			XFlush (XtDisplay (toplevel));
			if (pwSuccess && passwdStored)
				{
				reinvoke = 1;
				notDone = 0;
				}
			else if (pwMaxTrys == PW_MAXTRYS)
				{
				XtSetSensitive (linfo->login_text, TRUE);
				XtVaSetValues (linfo->login_fail, XtNstring,
					(XtArgVal)errStr, NULL);
				OlSetInputFocus (linfo->login_text,
					RevertToParent, CurrentTime);
				}
			else if (!passwdStored)
				{
				XtSetSensitive (linfo->login_text, TRUE);
				XBell (XtDisplay (linfo->login_fail), 0);
				XtVaSetValues (linfo->login_fail, XtNstring,
					(XtArgVal)errStr, NULL);
				OlSetInputFocus (linfo->login_text,
					RevertToParent, CurrentTime);
				}
			free (errStr);
			break;
		default:
			FailedLogin (d); 
			break;
		}
    }
    /* do we need to reinvoke login scheme */
    if (reinvoke)
	{
	/*
	 * Put the logname and the passwd on the ava stream
	 */
	ava = retava(Slaveptty_fd);

	(void) bzero (xlogname, strlen (xlogname));
	(void) sprintf (xlogname, "XLOGNAME=%s", p);
	if ((ava = putava (xlogname, ava)) == NULL)
		{
		Debug ("Could not set logname ava\n");
		}

	(void) bzero (xpasswd, strlen (xpasswd));
	(void) sprintf (xpasswd, "XPASSWD=%s", newPassword);
	if ((ava = putava (xpasswd, ava)) == NULL)
		{
		Debug ("Could not set passwd ava\n");
		}

	(void) bzero (xtty, strlen (xtty));
	(void) sprintf (xtty, "XTTY=%s", ttyname(Slaveptty_fd));
	if ((ava = putava (xtty, ava)) == NULL)
		{
		Debug ("Could not set tty ava\n");
		}

	if (setava (Slaveptty_fd, ava) != 0)
		{
		Debug ("Could not do setava\n");
		}

	ret = invoke (Slaveptty_fd, "login");
	if (ret == 0) 
		{
		if (CreateEnv (greet.name, &verify, d, Slaveptty_fd))
			Debug ("CreateEnv failed\n");
		if (StoreUserPid (d->serverPid))
			Debug ("Could not store user's server pid\n");
		}
	reinvoke = 0;
	}
    (void) bzero (oldPassword, strlen (oldPassword));
    (void) bzero (newPassword, strlen (newPassword));

#ifdef SECURE_RPC
    for (i = 0; i < d->authNum; i++)
    {
	if (d->authorizations[i]->name_length == 9 &&
	    bcmp (d->authorizations[i]->name, "SUN-DES-1", 9) == 0)
	{
	    XHostAddress	addr;
	    char		netname[MAXNETNAMELEN+1];
	    char		domainname[MAXNETNAMELEN+1];
    
	    getdomainname(domainname, sizeof domainname);
	    user2netname (netname, verify.uid, domainname);
	    addr.family = FamilyNetname;
	    addr.length = strlen (netname);
	    addr.address = netname;
	    XAddHost (dpy, &addr);
	    break;
	}
    }
#endif
    CloseGreet (d);
    Debug ("Greet loop finished\n");
    /*
     * Run system-wide initialization file
     */
    if (d->startup) {
	    if (source (verify.systemEnviron, d->startup) != 0)
    		{
		Debug ("Startup program %s exited with non-zero status\n",
			d->startup);
		SessionExit (d, OBEYSESS_DISPLAY, FALSE);
    		}
    }
    if (!Setjmp (abortSession)) {
	(void) Signal (SIGTERM, catchTerm);
	/*
	 * Start the clients, changing uid/groups
	 *	   setting up environment and running the session
	 */
	if (StartClient (&verify, d, &clientPid, greet.password)) {
	    Debug ("Client Started\n");

	    /*
	     * Wait for session to end,
	     */
	    for (;;) {
		if (d->pingInterval)
		{
		    if (!Setjmp (pingTime))
		    {
			(void) Signal (SIGALRM, catchAlrm);
			(void) alarm (d->pingInterval * 60);
			pid = wait (&status);
			(void) alarm (0);
		    }
		    else
		    {
			(void) alarm (0);
		    	if (!PingServer (d, (Display *) NULL))
			    SessionPingFailed (d);
		    }
		}
		else
		{
		    Debug ("waiting in ManageSession\n");
		    pid = wait (&status);
		    Debug ("waiting done.\n");
		}
		if (pid == clientPid)
		    break;
	    }
	} else {
	    LogError ("session start failed\n");
	}
    } else {
	/*
	 * when terminating the session, nuke
	 * the child and then run the reset script
	 */
	AbortClient (clientPid);
    }
    /*
     * run system-wide reset file
     */
    switch (waitVal (status))
	{
	case SHUTDOWN_FLAG:
		if (d->reset) {
			Debug ("Source reset program %s\n", d->reset);
			source (verify.systemEnviron, d->reset);
		}
		setuid (verify.uid);
		Sys_Shutdown ();
		SessionExit (d, SHUTDOWN_FLAG, TRUE);
		break;
	default:
		if (d->reset) {
			Debug ("Source reset program %s\n", d->reset);
			source (verify.systemEnviron, d->reset);
		}
		SessionExit (d, waitVal(status), TRUE);
		break;
	}
}

/*
 * Procedure:	StorePasswd
 *
 * Restrictions:
 *		fork(2):	none
 *		setuid:		none
 *		execl(2):	P_ALLPRIVS
 *
 * Notes:	This routine forks, changes the uid of the forked process
 *		to the user logging in, and execs the "/usr/bin/passwd"
 *		command.  It returns the status of the "exec" to the
 *		parent process.  All "working" privileges of the forked
 *		(child) process are cleared.  Also, P_SYSOPS is cleared
 *		from the maximum set to indicate to "passwd" that this
 *		"exec" originated from the login scheme.
*/
int
StorePasswd (char *usernam, char *password)
	{
	int	status, w;
	pid_t	pid;
	char	**ava;
	char	xlogname[128];
	char	xnpass[128];
	char	xopass[128];

	
	/*
	 * Put the logname and the passwd on the ava stream
	 */
	ava = retava(Slaveptty_fd);

	(void) sprintf (xlogname, "XLOGNAME=%s\0", usernam);
	if ((ava = putava (xlogname, ava)) == NULL)
		{
		Debug ("Could not set xlogname ava\n");
		}

	(void) sprintf (xnpass, "XNPASS=%s\0", password);
	if ((ava = putava (xnpass, ava)) == NULL)
		{
		Debug ("Could not set xnpass ava\n");
		}

	(void) sprintf (xopass, "XOPASS=%s\0", oldPassword);
	if ((ava = putava (xopass, ava)) == NULL)
		{
		Debug ("Could not set xopass ava\n");
		}


	if (setava (Slaveptty_fd, ava) != 0)
		{
		Debug ("Could not do setava\n");
		}

	w = invoke (Slaveptty_fd, BINPASSWD);
	Debug ("return from invoke is %d\n", w);
	return w;
	}

SetupDisplay (d)
struct display	*d;
{
    char	**env = 0, **setEnv(), **systemEnv();

    if (d->setup && d->setup[0])
    {
    	env = systemEnv (d, (char *) 0, (char *) 0);
    	(void) source (env, d->setup);
    	freeEnv (env);
    }
}

static Jmp_buf syncJump;

/* ARGSUSED */
static SIGVAL
syncTimeout (n)
    int n;
{
    Longjmp (syncJump, 1);
}

SecureDisplay (d, dpy)
struct display	*d;
Display		*dpy;
{
    Debug ("SecureDisplay %s\n", d->name);
    (void) Signal (SIGALRM, syncTimeout);
    if (Setjmp (syncJump)) {
	LogError ("WARNING: display %s could not be secured\n",
		   d->name);
	SessionExit (d, RESERVER_DISPLAY, FALSE);
    }
    (void) alarm ((unsigned) d->grabTimeout);
    Debug ("Before XGrabServer %s\n", d->name);
    XGrabServer (dpy);
    if (XGrabKeyboard (dpy, DefaultRootWindow (dpy), True, GrabModeAsync,
		       GrabModeAsync, CurrentTime) != GrabSuccess)
    {
	(void) alarm (0);
	(void) Signal (SIGALRM, SIG_DFL);
	LogError ("WARNING: keyboard on display %s could not be secured\n",
		  d->name);
	SessionExit (d, RESERVER_DISPLAY, FALSE);
    }
    Debug ("XGrabKeyboard succeeded %s\n", d->name);
    (void) alarm (0);
    (void) Signal (SIGALRM, SIG_DFL);
    pseudoReset (dpy);
    if (!d->grabServer)
    {
	XUngrabServer (dpy);
	XSync (dpy, 0);
    }
    Debug ("done secure %s\n", d->name);
}

UnsecureDisplay (d, dpy)
struct display	*d;
Display		*dpy;
{
    Debug ("Unsecure display %s\n", d->name);
    if (d->grabServer)
    {
	XUngrabServer (dpy);
	XSync (dpy, 0);
    }
}

SessionExit (d, status, removeAuth)
    struct display  *d;
{
    /* make sure the server gets reset after the session is over */
    if (d->serverPid >= 2 && d->resetSignal)
	kill (d->serverPid, d->resetSignal);
    else
	ResetServer (d);
    if (removeAuth)
    {
	(void) setgid (verify.groups[0]);
	(void) setgid (verify.gid);
	(void) setuid (verify.uid);
	RemoveUserAuthorization (d, &verify);
    }
    Debug ("Display %s exiting with status %d\n", d->name, status);
    exit (status);
}

StartClient (verify, d, pidp, passwd)
struct verify_info	*verify;
struct display		*d;
int			*pidp;
char			*passwd;
{
    char	**f, *home, *getEnv (), **setEnv ();
    char	*failsafeArgv[2];
    char	disp[512];
    struct stat	buf;
    int		pid;

    switch (pid = fork ()) {
    case 0:
	CleanUpChild ();
#ifdef SECURE_RPC
	{
	    char    netname[MAXNETNAMELEN+1], secretkey[HEXKEYBYTES+1];
	    int	    ret;
	    int	    len;

	    getnetname (netname);
	    Debug ("User netname: %s\n", netname);
	    len = strlen (passwd);
	    if (len > 8)
		bzero (passwd + 8, len - 8);
	    ret = getsecretkey(netname,secretkey,passwd);
	    Debug ("getsecretkey returns %d, key length %d\n",
		    ret, strlen (secretkey));
	    ret = key_setsecret(secretkey);
	    Debug ("key_setsecret returns %d\n", ret);
	}
#endif
	bzero(passwd, strlen(passwd));
	SetUserAuthorization (d, verify);
	home = getenv ("HOME");
	if (home) {
		if (chdir (home) == -1) {
			if (RemovePidFile ())
				Debug ("Could not remove user's pid file\n");
			LogError ("No home directory %s for user %s\n", home,
				getEnv (verify->userEnviron, "LOGNAME"));
			exit (NOHOME);
		}
		if (stat (".olsetup", &buf)) {
			if (RemovePidFile ())
				Debug ("Could not remove user's pid file\n");
			failsafeArgv[0] = d->failsafeClient;
			failsafeArgv[1] = "-motif";
			verify->userEnviron = setEnv (verify->userEnviron, "LD_LIBRARY_PATH", d->linkPath);
			verify->userEnviron = setEnv (verify->userEnviron, "SHELL", "/sbin/sh");
			execute (failsafeArgv, verify->userEnviron);
			exit (OBEYSESS_DISPLAY);
		}
		if (dup2 (Slaveptty_fd, 0) != 0)
			Debug ("dup2 failed\n");
		failsafeArgv[0] = "/usr/bin/shserv";
		failsafeArgv[1] = 0;
		execve (failsafeArgv[0], failsafeArgv, environ);
		if (RemovePidFile ())
			Debug ("Could not remove user's pid file\n");
		LogError ("Session execution failed xshserv\n");
		exit (BADSHELL);
	}
	if (RemovePidFile ())
		Debug ("Could not remove user's pid file\n");
	LogError ("No home directory for user %s\n", getEnv (verify->userEnviron, "LOGNAME"));
	exit (NOHOME);
    case -1:
	bzero(passwd, strlen(passwd));
	Debug ("StartSession, fork failed\n");
	LogError ("can't start session for %d, fork failed\n", d->name);
	return 0;
    default:
	bzero(passwd, strlen(passwd));
	Debug ("StartSession, fork suceeded %d\n", pid);
	*pidp = pid;
	return 1;
    }
}

static Jmp_buf	tenaciousClient;

/* ARGSUSED */
static SIGVAL
waitAbort (n)
    int n;
{
	Longjmp (tenaciousClient, 1);
}

#if defined(_POSIX_SOURCE) || defined(SYSV) || defined(SVR4)
#define killpg(pgrp, sig) kill(-(pgrp), sig)
#endif

AbortClient (pid)
int	pid;
{
    int	sig = SIGTERM;
#if __STDC__
    volatile int	i;
#else
    int	i;
#endif
    int	retId;
    for (i = 0; i < 4; i++) {
	if (killpg (pid, sig) == -1) {
	    switch (errno) {
	    case EPERM:
		LogError ("xdm can't kill client\n");
	    case EINVAL:
	    case ESRCH:
		return;
	    }
	}
	if (!Setjmp (tenaciousClient)) {
	    (void) Signal (SIGALRM, waitAbort);
	    (void) alarm ((unsigned) 10);
	    retId = wait ((waitType *) 0);
	    (void) alarm ((unsigned) 0);
	    (void) Signal (SIGALRM, SIG_DFL);
	    if (retId == pid)
		break;
	} else
	    (void) Signal (SIGALRM, SIG_DFL);
	sig = SIGKILL;
    }
}

int
source (environ, file)
char			**environ;
char			*file;
{
    char	**args, *args_safe[2];
    extern char	**parseArgs ();
    int		ret;

    if (file && file[0]) {
	Debug ("source %s\n", file);
	args = parseArgs ((char **) 0, file);
	if (!args)
	{
	    args = args_safe;
	    args[0] = file;
	    args[1] = NULL;
	}
	ret = runAndWait (args, environ);
	freeArgs (args);
	return ret;
    }
    return 0;
}

int
runAndWait (args, environ)
    char	**args;
    char	**environ;
{
    int	pid;
    extern int	errno;
    waitType	result;

    switch (pid = fork ()) {
    case 0:
	CleanUpChild ();
	execute (args, environ);
	LogError ("can't execute %s\n", args[0]);
	exit (1);
    case -1:
	Debug ("fork failed\n");
	LogError ("can't fork to execute %s\n", args[0]);
	return 1;
    default:
	while (wait (&result) != pid)
		/* SUPPRESS 530 */
		;
	break;
    }
    return waitVal (result);
}

execute (argv, environ)
char	**argv;
char	**environ;
{
    /* make stdout follow stderr to the log file */
    dup2 (2,1);
    execve (argv[0], argv, environ);
    /*
     * In case this is a shell script which hasn't been
     * made executable (or this is a SYSV box), do
     * a reasonable thing
     */
    if (errno != ENOENT) {
	char	program[1024], *e, *p, *optarg;
	FILE	*f;
	char	**newargv, **av;
	int	argc;

	/*
	 * emulate BSD kernel behaviour -- read
	 * the first line; check if it starts
	 * with "#!", in which case it uses
	 * the rest of the line as the name of
	 * program to run.  Else use "/bin/sh".
	 */
	f = fopen (argv[0], "r");
	if (!f)
	    return;
	if (fgets (program, sizeof (program) - 1, f) == NULL)
 	{
	    fclose (f);
	    return;
	}
	fclose (f);
	e = program + strlen (program) - 1;
	if (*e == '\n')
	    *e = '\0';
	if (!strncmp (program, "#!", 2)) {
	    p = program + 2;
	    while (*p && isspace (*p))
		++p;
	    optarg = p;
	    while (*optarg && !isspace (*optarg))
		++optarg;
	    if (*optarg) {
		*optarg = '\0';
		do
		    ++optarg;
		while (*optarg && isspace (*optarg));
	    } else
		optarg = 0;
	} else {
	    p = "/bin/sh";
	    optarg = 0;
	}
	Debug ("Shell script execution: %s (optarg %s)\n",
		p, optarg ? optarg : "(null)");
	for (av = argv, argc = 0; *av; av++, argc++)
	    /* SUPPRESS 530 */
	    ;
	newargv = (char **) malloc ((argc + (optarg ? 3 : 2)) * sizeof (char *));
	if (!newargv)
	    return;
	av = newargv;
	*av++ = p;
	if (optarg)
	    *av++ = optarg;
	/* SUPPRESS 560 */
	while (*av++ = *argv++)
	    /* SUPPRESS 530 */
	    ;
	execve (newargv[0], newargv, environ);
    }
}

#define	MINLENGTH	6

int
GetPasswdMinLen ()
{
	FILE	*defltfp;
	int	temp, minlen = MINLENGTH;

	/*
	 * Read the /etc/defaults/passwd file to obtain passwd MINLENGTH
	 * value, return this value or MINLENGTH.
	 *
	 */
	if((defltfp = defopen("passwd")) != NULL)	{
		register char	*ptr;

		if((ptr = defread(defltfp, "PASSLENGTH")) != NULL)	{
			if (ptr) {
				temp = atoi (ptr);
				if (temp > 1 && temp < 9)
					minlen = temp;
			}
		}
		(void)defclose(defltfp);
	}
	return minlen;
}
