/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ucb:common/ucbcmd/chown/chown.c	1.3"
#ident	"$Header: $"

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

/*
 * chown [-fhR] uid[.gid] file ...
 */

	/* Macros to mimic the berkeley chown behaviour */
#define	c_chown(n,u,g)	((cuid!=0&&cuid!=u)?(errno=EPERM,-1):chown(n,u,g))
#define	c_lchown(n,u,g)	((cuid!=0&&cuid!=u)?(errno=EPERM,-1):lchown(n,u,g))

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <dirent.h>
#include <grp.h>
#include <errno.h>

struct	passwd *pwd;
struct	passwd *getpwnam();
struct	stat stbuf;
int	uid;
int	status;
int	fflag;
int	hflag;
int	rflag;
int	cuid;		/* Used by c_{l}chown macros */
extern	int	errno;

main(argc, argv)
	char *argv[];
{
	register int c, gid;
	register char *cp, *group;
	struct group *grp;
	extern char *strchr();

	cuid = getuid();	/* Used by c_{l}chown macros */
	argc--, argv++;
	while (argc > 0 && argv[0][0] == '-') {
		for (cp = &argv[0][1]; *cp; cp++) switch (*cp) {

		case 'f':
			fflag++;
			break;

		case 'h':
			hflag++;
			break;

		case 'R':
			rflag++;
			break;

		default:
			fatal(255, "unknown option: %c", *cp);
		}
		argv++, argc--;
	}
	if (argc < 2) {
		fprintf(stderr, "usage: chown [-fhR] owner[.group] file ...\n");
		exit(-1);
	}
	gid = -1;
	group = strchr(argv[0], '.');
	if (group != NULL) {
		*group++ = '\0';
		if (!isnumber(group)) {
			if ((grp = getgrnam(group)) == NULL)
				fatal(255, "unknown group: %s",group);
			gid = grp -> gr_gid;
			(void) endgrent();
		} else if (*group != '\0')
			gid = atoi(group);
	}
	if (!isnumber(argv[0])) {
		if ((pwd = getpwnam(argv[0])) == NULL)
			fatal(255, "unknown user id: %s",argv[0]);
		uid = pwd->pw_uid;
	} else
		uid = atoi(argv[0]);
	for (c = 1; c < argc; c++) {
		/* do stat for directory arguments */
		if (lstat(argv[c], &stbuf) < 0) {
			status += Perror(argv[c]);
			continue;
		}
		if (rflag && ((stbuf.st_mode&S_IFMT) == S_IFDIR)) {
			status += chownr(argv[c], uid, gid);
			continue;
		}
		else if (hflag) {
			if (c_lchown(argv[c], uid, gid)) {
				status += Perror(argv[c]);
				continue;
			}
		}
		else if (c_chown(argv[c], uid, gid)) {
			status += Perror(argv[c]);
			continue;
		}
	}
	exit(status);
}

isnumber(s)
	char *s;
{
	register c;

	while(c = *s++)
		if (!isdigit(c))
			return (0);
	return (1);
}

chownr(dir, uid, gid)
	char *dir;
	int uid, gid;
{
	register DIR *dirp;
	register struct dirent *dp;
	struct stat st;
	char savedir[1024];
	int ecode;

	if (getcwd(savedir,1024) == NULL)
		fatal(255, "%s", savedir);
	/*
	 * Change what we are given before doing it's contents.
	 */
	if (c_chown(dir, uid, gid) < 0 && Perror(dir))
		return (1);
	if (chdir(dir) < 0) {
		Perror(dir);
		return (1);
	}
	if ((dirp = opendir(".")) == NULL) {
		Perror(dir);
		return (1);
	}
	dp = readdir(dirp);
	dp = readdir(dirp); /* read "." and ".." */
	ecode = 0;
	for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
		if (lstat(dp->d_name, &st) < 0) {
			ecode = Perror(dp->d_name);
			if (ecode)
				break;
			continue;
		}
		if ((st.st_mode&S_IFMT) == S_IFDIR) {
			ecode = chownr(dp->d_name, uid, gid);
			if (ecode)
				break;
			continue;
		}
		else if (hflag) {
			if (c_lchown(dp->d_name, uid, gid) < 0 &&
		   	 (ecode = Perror(dp->d_name)))
				break;
		}
		else if (c_chown(dp->d_name, uid, gid) < 0 &&
		    (ecode = Perror(dp->d_name)))
			break;
	}
	closedir(dirp);
	if (chdir(savedir) < 0)
		fatal(255, "can't change back to %s", savedir);
	return (ecode);
}

error(fmt, a)
	char *fmt, *a;
{

	if (!fflag) {
		fprintf(stderr, "chown: ");
		fprintf(stderr, fmt, a);
		putc('\n', stderr);
	}
	return (!fflag);
}

fatal(status, fmt, a)
	int status;
	char *fmt, *a;
{

	fflag = 0;
	(void) error(fmt, a);
	exit(status);
}

Perror(s)
	char *s;
{

	if (!fflag) {
		fprintf(stderr, "chown: ");
		perror(s);
	}
	return (!fflag);
}
