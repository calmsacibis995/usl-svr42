/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)xdm:verify.c	1.25"
/*
 * xdm - display manager daemon
 *
 * $XConsortium: verify.c,v 1.24 91/07/18 22:22:45 rws Exp $
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
 * verify.c
 *
 * typical unix verification routine.
 */

#include	"dm.h"

#ifdef USE_IAF
#include <sys/types.h>
#include <utmpx.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>			/* For logfile locking */
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/utsname.h>
#include <sys/systeminfo.h>
#include <utime.h> 
#include <termio.h>
#include <sys/stropts.h>
#include <shadow.h>			/* shadow password header file */
#include <time.h>
#include <sys/param.h> 
#include <sys/fcntl.h>
#include <deflt.h>
#include <grp.h>
#include <ia.h>
#include <sys/vnode.h>
#include <errno.h>
#include <lastlog.h>
#include <iaf.h>
#include <audit.h>
#include <mac.h>
#include <priv.h>
#include <sys/secsys.h>
#include <locale.h>
#include <sys/stream.h>
#include <sys/resource.h>

static char	*encryptPass;
#endif

#include	<crypt.h>
#ifdef X_NOT_STDC_ENV
char *getenv();
#endif

int	xset_id();
static	int	xset_env();

extern	char	**environ;

int
CreateEnv (user, verify, d, fd)
char *user;
struct verify_info	*verify;
struct display *d;
int	fd;
{
	int		ret = 0;
	char		**userEnv (), **systemEnv (), **parseArgs ();
	char		**argv;

	argv = 0;
	if (d->session)
		argv = parseArgs (argv, d->session);
	verify->argv = argv;
	verify->userEnviron = userEnv (d, 0);
	if ((ret = xset_id(verify, d->name, fd)) != 0) {
		Debug ("Failure in xset_id() = %d\n", ret);
		return 1;
	}
	Debug ("user environment:\n");
	printEnv (verify->userEnviron);
	verify->systemEnviron = systemEnv (d, user, getenv ("HOME"));
	Debug ("system environment:\n");
	printEnv (verify->systemEnviron);
	Debug ("end of environments\n");

	return 0;
}

extern char **setEnv ();

char **
defaultEnv ()
{
    char    **env, **exp, *value;

    env = 0;
    for (exp = exportList; exp && *exp; ++exp)
    {
	value = getenv (*exp);
	if (value)
	    env = setEnv (env, *exp, value);
    }
    return env;
}

char **
userEnv (d, useSystemPath)
struct display	*d;
int	useSystemPath;
{
    char	**env;
    char	**envvar;
    char	*str;
    
    env = defaultEnv ();
    env = setEnv (env, "TERM", "xterm");
    env = setEnv (env, "PATH", useSystemPath ? d->systemPath : d->userPath);
    env = setEnv (env, "DISPLAY", d->name);
    env = setEnv (env, "CONSEM", "no");
    return env;
}

char **
systemEnv (d, user, home)
struct display	*d;
char	*user, *home;
{
    char	**env;
    
    env = defaultEnv ();
    env = setEnv (env, "DISPLAY", d->name);
    if (home)
	env = setEnv (env, "HOME", home);
    if (user)
	env = setEnv (env, "LOGNAME", user);
    env = setEnv (env, "PATH", d->systemPath);
    env = setEnv (env, "SHELL", d->systemShell);
    if (d->authFile)
	    env = setEnv (env, "XAUTHORITY", d->authFile);
    return env;
}

/*
 * Procedure:	xset_id
 *
 * Notes:	called to set the necessary attributes for this user
 *		based on information retrieved via the "ava" stream
 *		or the I&A database file.
 */
int
xset_id(verify, display, fd)
struct verify_info	*verify;
char			*display;
int			fd;
{
	int	i;
	char	**avap;
	char	*p, *tp;

	avap = retava (fd);

	if ((p = getava("UID", avap)) != NULL)
		verify->uid = atol(p);
	else
		return 1;

	if ((p = getava("GID", avap)) != NULL) 
		verify->gid = atol(p);
	else
		return 1;

	if ((p = getava("GIDCNT", avap)) != NULL) {
		verify->ngroups = atol(p);

		if (verify->ngroups) {
			if ((p = getava("SGID", avap)) == NULL) 
				return 1;
			for (i = 0; i < verify->ngroups; i++) {
				verify->groups[i] = strtol(p, &tp, 10);
				p = ++tp;
			}
		}
	} else
		return 1;

	if (set_id(NULL) != 0) {
		Debug ("Problem with set_id\n");
		return 1;
	}

	if (xset_env (display, fd) != 0) {
		Debug ("Problem with set_env\n");
		return 1;
	}
	return 0;
}

static	int
xset_env(display, fd)
char	*display;
int	fd;
{
	char	*ptr;
	char	**envp;
	char	**avap;
	char	env[BUFSIZ] = { "ENV=" };
	char	disp[1024];

	extern	char	**environ;
	extern	char	*argvtostr();
	extern	char	**strtoargv();

	if ((avap = retava(fd)) == NULL)
		return 1;

	if ((ptr = getava("ENV", avap)) == NULL)
		return 1;

	if ((envp = strtoargv(ptr)) == NULL)
		return 1;

	environ = envp;

	(void) putenv("XDM_LOGIN=yes");
	(void) putenv("CONSEM=no");
	(void) sprintf (disp, "DISPLAY=%s", display);
	(void) putenv(disp);

	if ((ptr = argvtostr(environ)) == NULL)
		return 1;

	(void) strcat(env, ptr);

	if ((avap = putava(env, avap)) == NULL)
		return 1;

	(void) setava (fd, avap);

	return 0;
}
