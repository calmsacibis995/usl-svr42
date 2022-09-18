/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)shserv:shserv.c	1.12.2.4"
#ident  "$Header: shserv.c 1.2 91/06/27 $"

#include	<stdio.h>
#include	<iaf.h>
#include	<unistd.h>
#include	<errno.h>
#include	<string.h>
#include	<locale.h>
#include	<pfmt.h>

#define	Shell	"/usr/bin/sh"

static	char	minusnam[16] = "-";
extern	char	**environ,
		*basename();

static  const   char
	*noretava = ":731:failed retava in shserv\n",
	*noshell  = ":732:failed getava of SHELL\n",
	*nohome   = ":733:failed getava of HOMEDIR\n",
	*noenv    = ":734:unable to set environment\n",
	*nodir 	  = ":735:unable to change directory to %s\n",
	*noexec   = ":331:No shell\n";

static	char	tmout_env_buf[128]	= "TIMEOUT=0";

extern FILE     *defopen();
extern char     *defread();

main()
{
	FILE	*defltfp;

	char	*dirp,
		**avap,
		**envp,
		*endptr,
		*shellp;

	(void) setlocale(LC_ALL, "");
	setcat("uxcore");
	setlabel("UX:shserv");

	if (( avap = retava(0)) == NULL) {
		pfmt(stderr, MM_ERROR, noretava);
		(void)setava(0,NULL);
		exit(1);
	}
	if ((shellp = getava("SHELL", avap)) == NULL) {
		pfmt(stderr, MM_ERROR, noshell);
		(void)setava(0,NULL);
		exit(1);
	}

	if ((dirp = getava("HOME", avap)) == NULL) {
		pfmt(stderr, MM_ERROR, nohome);
		(void)setava(0,NULL);
		exit(1);
	}

	if (chdir(dirp) < 0) {
		pfmt(stderr, MM_ERROR, nodir, dirp);
		(void)setava(0,NULL);
		exit(1);
	}
	if (set_env())
		pfmt(stderr, MM_ERROR, noenv);

	(void) setava(0,NULL);
	(void) strcat(minusnam, basename(shellp));

	/*
	 * Read the /etc/defaults/sh file to obtain shell TIMEOUT
	 * value, pass this value through the environment.
	 *
	 */
	if((defltfp = defopen("sh")) != NULL)	{
		register char	*ptr;

		if((ptr = defread(defltfp, "TIMEOUT")) != NULL)	{
			strcpy(&tmout_env_buf[8], (const char *)ptr);
		}
		(void)defclose(defltfp);
	}
	putenv(tmout_env_buf);

	(void) execl(shellp, minusnam, (char *) 0);

	if (access(shellp, R_OK|X_OK) == 0) {
		/*
		 * "shellp" was not an executable object file.
		 * Maybe it is  a shell procedure or a command
		 * line with  arguments.  If so, turn off  the
		 * SHELL= environment variable.
		*/
		envp = environ;
		for (; *envp != NULL; envp++) {
			if (!(strncmp(*envp, "SHELL=", 6)) &&
	    		   ((endptr = strchr(*envp, '=')) != NULL)) {
				(*++endptr) = '\0';
			}
		}
		(void) execl(Shell, "sh", shellp, (char *) 0);
	}
	/*
	 * Neither ``exec'' worked so issue a diagnostic and exit.
	*/
	pfmt(stderr, MM_ERROR, noexec);
	exit(1);
	/* NOTREACHED */
}
