/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:user/homedir.c	1.3.17.6"
#ident  "$Header: homedir.c 2.0 91/07/13 $"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <userdefs.h>
#include <errno.h>
#include <mac.h>
#include <sys/stat.h>
#include <priv.h>
#include "messages.h"

extern	int	execl(),
		chown(),
		mkdir(),
		unlink(),
		setgid(),
		lvlfile(),
		rm_files();

extern	size_t	strlen();

extern	pid_t	fork(),
		wait();

extern	char	*strrchr(),
		*dirname(),
		*strerror();

extern	void	exit(),
		errmsg();

static	int	_mk_skel();

/*
 * Procedure:	create_home
 *
 * Restrictions:
 *		mkdir(2):	none
 *		chown(2):	none
 *		unlink(2):	none
 *		lvlfile(2):	none
 *
 * Notes:	Create a home directory and populate with files from skeleton
 *		directory.
*/
int
create_home(hdir, skeldir, logname, existed, uid, gid, def_lvl)
	char	*hdir,			/* home directory to create */
		*skeldir,		/* skel directory to copy if indicated */
		*logname;		/* login name of user */
	int	existed;		/* determine if directory existed */
	uid_t	uid;			/* uid of user */
	gid_t	gid;			/* group id of user */
	level_t	def_lvl;		/* level for user's files */
{
	if (*skeldir) {
		return _mk_skel(hdir, skeldir, logname, existed, gid, def_lvl);
	}
	else {
		if (!existed) {
			if (mkdir(hdir, 0755) != 0) {
				errmsg(M_OOPS, "create the home directory",
					strerror(errno));
				return EX_HOMEDIR;
			}
		}

		(void) lvlfile(hdir, MAC_SET, &def_lvl);

		if (chown(hdir, uid, gid) != 0) {
			errmsg(M_OOPS, "change ownership of home directory", 
				strerror(errno));
			(void) unlink(hdir);
			return EX_HOMEDIR;
		}
	}
	return EX_SUCCESS;
}


#define	f_c	"/usr/bin/find . -print | /usr/bin/cpio -pdum %s > /dev/null 2>&1"
#define	chl	"/usr/bin/find %s -print | /usr/bin/xargs $TFADMIN /sbin/chlvl %s > /dev/null 2>&1"

/*
 * Procedure:	_mk_skel
 *
 * Restrictions:
 *		lvlout:		none
 *		setgid(2):	none
 *		stat(2):	none
 *		chdir(2):	none
 *		mkdir(2):	none
 *		lvlfile(2):	none
 *		setgid():	none
 *		execl():	P_ALLPRIVS
 *
 * Notes:	This routine forks and execs a command with the specified
 *		option to copy ALL files in the named directory to the new
 *		home directory.
 *
 *		Also forks and execs the "/usr/bin/chown" command with
 *		the "-R" option to change the owner of ALL the files
 *		in the new directory to the new user.
*/
static	int
_mk_skel(hdir, skeldir, logname, existed, gid, u_lvl)
	char	*hdir,			/* real home directory */
		*skeldir,		/* skeleton directory to copy from */
		*logname;		/* login name of user */
	int	existed;		/* determine if directory existed */
	gid_t	gid;			/* the new group ID for all files */
	level_t	u_lvl;			/* level for user's files */
{
	char	*bufp,
		*own_opt_h = "-h",
		*own_opt_R = "-R",
		*sh_cmd = "/sbin/sh",
		*cmd = NULL,
		*own_cmd = "/usr/bin/chown";

	int	i,
		mac = 0,
		status = 0;
	pid_t	pid;

	if ((i = lvlout(&u_lvl, bufp, 0, LVL_ALIAS)) != -1) {
		mac = 1;
		bufp = (char *)malloc((unsigned int) i);
		if (bufp != NULL) {
			(void) lvlout(&u_lvl, bufp, i, LVL_ALIAS);
		}
	}

	if ((pid = fork()) < 0) {
		errmsg(M_OOPS, "fork.  Try again later", strerror(errno));
		return EX_HOMEDIR;
	}
	if (pid == (pid_t) 0) {
		/*
		 * in the child
		*/
		if (existed) {
			/*
	 		 * Copy all files from "skeldir" to the new "homedir".
	 		 * Notice that the group ID is changed in the child
	 		 * so that the new files will be at the new user's
			 * default level and group ID.  The user ID will be
			 * done if this is succesful.
			*/
			if (setgid(gid) != 0) {
				errmsg(M_OOPS, "setgid", strerror(errno));
				exit(1);
			}
		}
		else {
			/*
	 		 * Fork and exec "/sbin/sh" with the "-c" option and the
	 		 * command string specified in the define "cmd".  This
	 		 * copies all files from "skeldir" to the new "homedir".
			*/
			struct	stat	statbuf;

			if (stat(skeldir, &statbuf) < 0) {
				errmsg(M_OOPS, "find source directory", strerror(errno));
				exit(1);
			}
			if (setgid(gid) != 0) {
				errmsg(M_OOPS, "setgid", strerror(errno));
				exit(1);
			}
			if (mkdir(hdir, 0755) != 0) {
				errmsg(M_OOPS, "create the home directory", strerror(errno));
				exit(1);
			}
			(void) lvlfile(hdir, MAC_SET, &u_lvl);
		}
		/*
		 * set up the command string exec'ed by the shell
		 * so it contains the correct directory name where
		 * the files are copied.
		*/
		cmd = (char *)malloc(strlen(f_c) + strlen(hdir) + (size_t)1);
		(void) sprintf(cmd, f_c, hdir);

		/*
		 * change directory to the source directory.  If
		 * the chdir fails, print out a message and exit.
		*/
		if (chdir(skeldir) < 0) {
			errmsg(M_OOPS, "cd to source directory", strerror(errno));
			exit(1);
		}
		/*
		 * clear all privileges in the working set.
		*/
		(void) procprivl(CLRPRV, ALLPRIVS_W, 0);
		(void) execl(sh_cmd, sh_cmd, "-c", cmd, (char *)NULL);
		(void) procprivl(SETPRV, ALLPRIVS_W, 0);
		exit(1);
	}

	/*
	 * the parent sits quietly and waits for the child to terminate.
	*/
	else {
		(void) wait(&status);
	}
	if (existed) {
		(void) lvlfile(hdir, MAC_SET, &u_lvl);
	}
	if (((status >> 8) & 0377) != 0) {
		return EX_HOMEDIR;
	}

	cmd = '\0';
	status = 0;

	/*
	 * if MAC is installed, fork and exec "/usr/bin/find" and pipe
	 * the output to /sbin/chlvl to change the level of the files
	 * just copied to the new user's default level.
	*/
	if (mac) {
		if ((pid = fork()) < 0) {
			errmsg(M_OOPS, "fork.  Try again later", strerror(errno));
			return EX_HOMEDIR;
		}
		if (pid == (pid_t) 0) {
			/*
			 * in the child
			*/
			cmd = (char *) malloc(strlen(chl) + strlen(bufp) +
				strlen(hdir) + (size_t) 1);
			(void) sprintf(cmd, chl, hdir, bufp);
			(void) procprivl(CLRPRV, ALLPRIVS_W, 0);
			(void) execl(sh_cmd, sh_cmd, "-c", cmd, (char *)NULL);
			(void) procprivl(SETPRV, ALLPRIVS_W, 0);
			exit(1);
		}
	
		/*
		 * the parent sits quietly and waits for the child to terminate.
		*/
		else {
			(void) wait(&status);
		}
	
		if (((status >> 8) & 0377) != 0) {
			errmsg(M_OOPS, "chlvl new files");
			return EX_HOMEDIR;
		}
	 
		cmd = '\0';
		status = 0;
	}

	/*
	 * Now fork and exec "/usr/bin/chown" with the "-R" flag to
	 * change the owner of the files just copied to the new user.
	 * Doing it this way means that "/usr/bin/cp" does not need
	 * P_MACWRITE as a fixed privilege in a system that is running
	 * an ID based privilege mechanism.
	*/
	if ((pid = fork()) < 0) {
		errmsg(M_OOPS, "fork.  Try again later", strerror(errno));
		return EX_HOMEDIR;
	}
	if (pid == (pid_t) 0) {
		/*
		 * in the child
		*/
		(void) procprivl(CLRPRV, ALLPRIVS_W, 0);
		(void) execl(own_cmd, own_cmd, own_opt_h, own_opt_R, logname, hdir,
			(char *)NULL);
		(void) procprivl(SETPRV, ALLPRIVS_W, 0);
		exit(1);
	}

	/*
	 * the parent sits quietly and waits for the child to terminate.
	*/
	else {
		(void) wait(&status);
	}

	if (((status >> 8) & 0377) != 0) {
		errmsg(M_OOPS, "chown new home directory");
		return EX_HOMEDIR;
	}
	return EX_SUCCESS;
}
