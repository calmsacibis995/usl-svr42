/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)chmod:chmod.c	1.9.2.5"
#ident  "$Header: chmod.c 1.2 91/06/26 $"

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

/***************************************************************************
 * Command: chmod
 * Inheritable Privileges: P_OWNER,P_MACREAD,P_MACWRITE,P_DACREAD
 *       Fixed Privileges: None
 * Notes: changes the mode of file/directory.
 *
 ***************************************************************************/

/*
 * chmod option mode files
 * where 
 *	mode is [ugoa][+-=][rwxlstugo] or an octal number
 *	option is -R
 */

/*
 *  Note that many convolutions are necessary
 *  due to the re-use of bits between locking
 *  and setgid
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <priv.h>
#include <locale.h>
#include <pfmt.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

#define	USER	05700	/* user's bits */
#define	GROUP	02070	/* group's bits */
#define	OTHER	00007	/* other's bits */
#define	ALL	07777	/* all */

#define	READ	00444	/* read permit */
#define	WRITE	00222	/* write permit */
#define	EXEC	00111	/* exec permit */
#define	SETID	06000	/* set[ug]id */
#define	LOCK	02000	/* lock permit */
#define	STICKY	01000	/* sticky bit */

static	char	*ms;
static	char	*msp;
static	int	rflag;
static	mode_t	newmode(), abs(), who();
static	void	errmsg(long , long , const char *, ... );

static const char badstat[]	= ":5:Cannot access %s: %s\n";
static const char badchmod[]	= ":13:chmod() failed on %s: %s\n";
static const char errfmt[]	= ":12:%s: %s\n";
static const char badmode[]	= ":14:Invalid mode\n";

static	int	dochmod();
static	int	chmodr();
static	int	what();

/*
 * Procedure:     main
 *
 * Restrictions:
                 setlocale: 	none
                 pfmt: 		none
*/
main(argc, argv)
int argc;
char **argv;
{
	register int i;
	register char *p;
	int status = 0;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:chmod");
 
 	argv++, argc--;
	while (argc > 0 && argv[0][0] == '-') {
		for (p = &argv[0][1]; *p; p++)  {
			if (*p == 'R') 
				rflag++;
			else {
				goto done;
			}
		}
		argc--, argv++;
	}

	/* 
	 * Check for sufficient arguments 
	 * or a usage error. 
	 */
done:

	if(argc < 2){
		pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
		pfmt(stderr, MM_ACTION,
			":15:Usage: chmod [-R] [ugoa][+-=][rwxlstugo] file ...\n");
	}	

	ms = argv[0];

	for (i = 1; i < argc; i++)
		status += dochmod(argv[i]);

	exit(status);
	/* NOTREACHED */

}

/*
 * Procedure:     dochmod
 *
 * Restrictions:
                 lstat(2): 	none
                 stat(2): 	none
                 chmod(2): 	none
*/
static int
dochmod(name)
	char *name;
{
	static struct stat st; /* save space, recursion should not reuse */ 
	int errmode;


	if (lstat(name, &st) < 0) {
		errmsg(MM_WARNING, 0, badstat, name, strerror(errno));
		return 1;
	}

	if (rflag && (st.st_mode&S_IFMT) == S_IFDIR)
		return chmodr(name, st.st_mode);


	if ((st.st_mode & S_IFMT) == S_IFLNK) {
		if (stat(name, &st) < 0) {
			errmsg(MM_WARNING, 0, badstat, name, strerror(errno));
			return 1;
		}
	}

	if (chmod(name, newmode(st.st_mode, name, &errmode)) == -1) {
		errmsg(MM_WARNING, 0, badchmod, name, strerror(errno));
		return 1;
	}
	if (errmode)
		return 1;

	return 0;
}

/*
 * Procedure:     chmodr
 *
 * Restrictions:
                 getcwd: 	none
                 chmod(2): 	none
                 chdir(2): 	none
                 opendir: 	none
*/
static int
chmodr(dir, mode)
	char *dir;
	mode_t  mode;
{
	register DIR *dirp;
	register struct dirent *dp;
	char savedir[PATH_MAX];
	int ecode, errmode;


	if (getcwd(savedir, sizeof(savedir)) == 0) { 
		errmsg(MM_WARNING, 255, ":129:Cannot determine current directory");
	}

	/*
	 * Change what we are given before doing it's contents
	 */
	if (chmod(dir, newmode(mode, dir, &errmode)) < 0) {
		errmsg(MM_WARNING, 0, badchmod, dir, strerror(errno));
		return 1;
	}

	if (chdir(dir) < 0) {
		errmsg(MM_WARNING, 0, errfmt, dir, strerror(errno));
		return 1;
	}
	if ((dirp = opendir(".")) == NULL) {
		errmsg(MM_WARNING, 0, errfmt, dir, strerror(errno));
		return 1;
	}
	dp = readdir(dirp);
	dp = readdir(dirp); /* read "." and ".." */
	ecode = 0;
	for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp))
		ecode += dochmod(dp->d_name);
	(void) closedir(dirp);
	if (chdir(savedir) < 0) {
		errmsg(MM_WARNING, 255, ":11:Cannot change back to %s: %s\n",
			savedir, strerror(errno));
	}
	return ecode;
}


static mode_t
newmode(nm, file, errmode)
mode_t  nm;
char  *file;
int *errmode;
{
	/* m contains USER|GROUP|OTHER information
	   o contains +|-|= information
	   b contains rwx(slt) information  */
	mode_t m, b;
	register int o;
	register int lcheck, scheck, xcheck, goon;
	mode_t om = nm;	/* save mode for file mode incompatibility return */

	*errmode = 0;
	msp = ms;
	if (*msp >= '0' && *msp <= '9')
		return abs(om);
	do {
		m = who();
		while (o = what()) {
/*
	this section processes permissions
*/
			b = 0;
			goon = 0;
			lcheck = scheck = xcheck = 0;
			switch (*msp) {
			case 'u':
				b = (nm & USER) >> 6;
				goto dup;
			case 'g':
				b = (nm & GROUP) >> 3;
				goto dup;
			case 'o':
				b = (nm & OTHER);
		    dup:
				b &= (READ|WRITE|EXEC);
				b |= (b << 3) | (b << 6);
				msp++;
				goon = 1;
			}
			while (goon == 0) switch (*msp++) {
			case 'r':
				b |= READ;
				continue;
			case 'w':
				b |= WRITE;
				continue;
			case 'x':
				b |= EXEC;
				xcheck = 1;
				continue;
			case 'l':
				b |= LOCK;
				m |= LOCK;
				lcheck = 1;
				continue;
			case 's':
				b |= SETID;
				scheck = 1;
				continue;
			case 't':
				b |= STICKY;
				continue;
			default:
				msp--;
				goon = 1;
			}

			b &= m;

			switch (o) {
			case '+':

				/* is group execution requested? */
				if ( xcheck == 1 && (b & GROUP & EXEC) == (GROUP & EXEC)) {

					/* not locking, too! */
					if ( lcheck == 1 ) {
						errmsg(MM_ERROR, 3, ":17:Group execution and locking not permitted together\n");
					}

					/* not if the file is already lockable */
					if ( (nm & GROUP & (LOCK | EXEC)) == LOCK ) {
						errmsg(MM_WARNING, 0, ":18:Group execution not permitted\n\ton %s, a lockable file\n", file);
						*errmode = 1;
						return om;
					}
				}

				/* is setgid on execution requested? */
				if ( scheck == 1 && (b & GROUP & SETID) == (GROUP & SETID) ) {

					/* not locking, too! */
					if ( lcheck == 1 && (b & GROUP & EXEC) == (GROUP & EXEC) ) {
						errmsg(MM_ERROR, 4, ":19:Set-group-ID and locking not permitted together\n");
					}

					/* not if the file is already lockable */
					if ( (nm & GROUP & (LOCK | EXEC)) == LOCK ) {
						errmsg(MM_WARNING, 0, ":20:Set-group-ID not permitted\n\ton %s, a lockable file\n", file);
						*errmode = 1;
						return om;
					}
				}

				/* is setid on execution requested? */
				if ( scheck == 1 ) {

					/* the corresponding execution must be requested or already set */
					if ( ((nm | b) & m & EXEC & (USER | GROUP)) != (m & EXEC & (USER | GROUP)) ) {
						errmsg(MM_WARNING, 0, ":21:Execute permission required for set-ID\n\ton execution for %s\n", file);
						*errmode = 1;
						return om;
					}
				}

				/* is locking requested? */
				if ( lcheck == 1 ) {

					/* not if the file has group execution set */
					/* NOTE: this also covers files with setgid */
					if ( (nm & GROUP & EXEC) == (GROUP & EXEC) ) {
						errmsg(MM_WARNING, 0, ":22:Locking not permitted\n\ton %s, a group executable file\n", file);
						*errmode = 1;
						return om;
					}
				}

				/* create new mode */
				nm |= b;
				break;
			case '-':

				/* don't turn off locking, unless it's on */
				if ( lcheck == 1 && scheck == 0 
				     && (nm & GROUP & (LOCK | EXEC)) != LOCK )
					b &= ~LOCK;

				/* don't turn off setgid, unless it's on */
				if ( scheck == 1 && lcheck == 0
				     && (nm & GROUP & (LOCK | EXEC)) == LOCK )
					b &= ~(GROUP & SETID);

				/* if execution is being turned off and the corresponding
				   setid is not, turn setid off, too & warn the user */
				if ( xcheck == 1 && scheck == 0
				     && ((m & GROUP) == GROUP || (m & USER) == USER)
				     && (nm & m & (SETID | EXEC)) == (m & (SETID | EXEC)) ) {
					errmsg(MM_WARNING, 0, ":23:Corresponding set-ID also disabled\n\ton %s since set-ID requires execute permission\n", file);
					if ( (b & USER & SETID) != (USER & SETID)
					     && (nm & USER & (SETID | EXEC)) == (m & USER & (SETID | EXEC)) )
						b |= USER & SETID;
					if ( (b & GROUP & SETID) != (GROUP & SETID)
					     && (nm & GROUP & (SETID | EXEC)) == (m & GROUP & (SETID | EXEC)) )
						b |= GROUP & SETID;
				}

				/* create new mode */
				nm &= ~b;
				break;
			case '=':

				/* is locking requested? */
				if ( lcheck == 1 ) {

					/* not group execution, too! */
					if ( (b & GROUP & EXEC) == (GROUP & EXEC) ) {
						errmsg(MM_ERROR, 3, ":17:Group execution and locking not permitted together\n");
					}

					/* if the file has group execution set, turn it off! */
					if ( (m & GROUP) != GROUP ) {
						nm &= ~(GROUP & EXEC);
					}
				}

/* is setid on execution requested?
   the corresponding execution must be requested, too! */
				if (scheck == 1 
				  && (b & EXEC & (USER | GROUP))  
				    != (m & EXEC & (USER | GROUP)) ) {
					errmsg(MM_ERROR, 2, ":24:Execute permission required for set-ID on execution\n");
				}

/* 
 * The ISGID bit on directories will not be changed when 
 * the mode argument is a string with "=". 
 */

				if ((om & S_IFMT) == S_IFDIR)
					b = (b & ~S_ISGID) | (om & S_ISGID);

				/* create new mode */
				nm &= ~m;
				nm |= b;
				break;
			}
		}
	} while (*msp++ == ',');
	if (*--msp) {
		errmsg(MM_ERROR, 5, badmode);
	}
	return nm;
}

static mode_t
abs(mode)
	mode_t mode;
{
	register c;
	mode_t i;

	for ( i = 0; (c = *msp) >= '0' && c <= '7'; msp++)
		i = (mode_t)((i << 3) + (c - '0'));
	if (*msp)
		errmsg(MM_ERROR, 6, badmode);

/* The ISGID bit on directories will not be changed when the mode argument is
 * octal numeric. Only "g+s" and "g-s" arguments can change ISGID bit when
 * applied to directories.
 */
	if ((mode & S_IFMT) == S_IFDIR)
		return (i & ~S_ISGID) | (mode & S_ISGID);
	return i;
}

static mode_t
who()
{
	register mode_t m;

	m = 0;
	for (;; msp++) switch (*msp) {
	case 'u':
		m |= USER;
		continue;
	case 'g':
		m |= GROUP;
		continue;
	case 'o':
		m |= OTHER;
		continue;
	case 'a':
		m |= ALL;
		continue;
	default:
		if (m == 0)
			m = ALL;
		return m;
	}
}

static int
what()
{
	switch (*msp) {
	case '+':
	case '-':
	case '=':
		return *msp++;
	}
	return 0;
}

/*
 * Procedure:     errmsg
 *
 * Restrictions:
                 pfmt: 	none
*/
/* ARGSUSED3 */
static void
errmsg(long severity, long code, const char *format, ... )
{
	va_list args;
	va_start(args, format);
	(void)vpfmt(stderr, severity, format, args);
	if (code != 0)
		exit(code);
	return;
}
