/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/lckpwdf.c	1.6"

#ifdef __STDC__
	#pragma weak lckpwdf = _lckpwdf
	#pragma weak ulckpwdf = _ulckpwdf
#endif
#include "synonyms.h"
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mac.h>

#define S_WAITTIME	15

static struct flock flock =	{
			0,	/* l_type */
			0,	/* l_whence */
			0,	/* l_start */
			0,	/* l_len */
			0,	/* l_sysid */
			0	/* l_pid */
			} ;

/*	lckpwdf() returns a 0 for a successful lock within S_WAITTIME
	and -1 otherwise
*/

static	int	fildes = -1;
extern	void	(*sigset())();
extern	int	close(),
		lvlin(),
		fchown(),
		access(),
		unlink(),
		lvlfile();

extern	unsigned	alarm();


/*ARGSUSED*/
static void
almhdlr(sig)
int	sig;
{
}

/*
 * This routine sets a lock on the _LOCKFILE.
*/
int
lckpwdf()
{
	int	retval,
		no_lock = 0,
		had_shad = 1;
	level_t	f_lid = 0;
	struct	stat	buf;
	const	char	*_PWDFILE = "/etc/shadow",
			*_LOCKFILE = "/etc/security/ia/.pwd.lock";

	/*
	 * check if the MAC feature is installed.
	 * If it isn't, set ``f_lid'' to an invalid LID.
	*/
	if (lvlin("SYS_PRIVATE", &f_lid) < 0) {
		f_lid = 0;
	}
	/*
	 * determine if _LOCKFILE exists.  If it doesn't, set "no_lock"
	 * to 1 so that the stat, chown and lvlfile will be done.
	*/
	if (stat(_LOCKFILE, &buf) < 0) {
		no_lock = 1;
		/*
		 * Only stat the password file to get owner, group and
		 * level if the _LOCKFILE did not exist.
		*/
		if (stat(_PWDFILE, &buf) < 0) {
			had_shad = 0; 		/* stat() FAILED! */
			buf.st_level = 0;
		}
	}
	/*
	 * If either of the two stat() call above succeeded and
	 * either returned a valid level, compare that level to
	 * "SYS_PRIVATE".  If they're not equal, set the lock file
	 * level to the level of the successful stat().
	*/
	if (buf.st_level && (buf.st_level != f_lid)) {
		f_lid = buf.st_level;
	}
	/*
	 * the creat() succeeds if MAC is installed and the process
	 * is either at the same level as the _LOCKFILE, or the process
	 * has P_MACWRITE turned on in its working set.
	 *
	 * Regardless of the status of MAC, the process must still pass
	 * the discretionary checks for a successful creat() so the
	 * process may also require P_DACWRITE.
	*/
	if ((fildes = creat(_LOCKFILE, 0400)) == -1)
		return -1;
	else {
		if (no_lock) {
			/*
			 * if the _LOCKFILE didn't exist and MAC is installed,
			 * set the level of the _LOCKFILE to the value in
			 * ``f_lid''.
			*/
			if (f_lid) {
				(void) close(fildes);
				(void) lvlfile(_LOCKFILE, MAC_SET, &f_lid);
				if ((fildes = creat(_LOCKFILE, 0400)) == -1) {
					(void) unlink(_LOCKFILE);
					return -1;
				}
			}
			if (had_shad) {
				/*
				 * change the owner and group of the _LOCKFILE
				 * to the _PWDFILE owner.  On failure, remove
				 * the file.
				*/
				if (fchown(fildes, buf.st_uid, buf.st_gid) < 0) {
					(void) unlink(_LOCKFILE);
					return -1;
				}
			}
		}
		flock.l_type = F_WRLCK;
		(void) sigset(SIGALRM, (void (*)())almhdlr);
		(void) alarm(S_WAITTIME);
		retval = fcntl(fildes, F_SETLKW, (int)&flock); 
		(void) alarm(0);
		(void) sigset(SIGALRM, SIG_DFL);
		return retval;
	}
}

/* 	ulckpwdf() returns 0 for a successful unlock and -1 otherwise
*/
int
ulckpwdf()
{
	if (fildes == -1) 
		return -1;
	else {
		flock.l_type = F_UNLCK;
		(void) fcntl(fildes, F_SETLK, (int)&flock);
		(void) close(fildes);
		fildes = -1;
		return 0;
	}
}
