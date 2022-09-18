/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)lp:cmd/lpsched/lpsched/lpfsck.c	1.3.8.2"
#ident  "$Header: lpfsck.c 1.2 91/06/27 $"

#ifdef	__STDC__
#include "stdarg.h"
#else
#include "varargs.h"
#endif

#include "stdlib.h"
#include "fcntl.h"

#include "lpsched.h"


/**
 ** main()
 **/

#ifdef	UNIT_TEST

#include "sys/utsname.h"

static char		*Local_System;

main ()
{
	struct utsname		utsbuf;

	(void) uname (&utsbuf);
	Local_System = utsbuf.nodename;

	lpfsck();
}

#undef	Stat
#undef	Lstat
#undef	Readlink
#undef	Symlink
#undef	Unlink
#undef	Mkdir
#undef	Close
#undef	Creat
#undef	Chmod
#undef	Chown
#undef	Lchown
#undef	Rename

#define	Stat		stat
#define Lstat		lstat
#define Readlink	readlink
#define	Symlink(A,B)	(printf("symlink(%s, %s)\n", A, B), 0)
#define Unlink(A)	(printf("unlink(%s)\n", A), 0)
#define	Mkdir(A,M)	(printf("mkdir(%s, %04o)\n", A, M), 0)
#define Close(X)	X
#define Creat(A,M)	(printf("creat(%s, %04o)\n", A, M), 0)
#define Chmod(A,M)	(printf("chmod(%s, %04o)\n", A, M), 0)
#define Chown(A,U,G)	(printf("chown(%s, %d, %d)\n", A, U, G), 0)
#define Lchown(A,U,G)	(printf("lchown(%s, %d, %d)\n", A, U, G), 0)
#define Rename(A,B)	(printf("rename(%s, %s)\n", A, B), 0)
#define schedlog	printf
#define note		printf

#define Lp_Uid		98
#define Lp_Gid		99

void
#ifdef	__STDC__
fail (char *format, ...)
#else
fail (format, va_alist)
	char *	format;
	va_dcl
#endif
{
	va_list	ap;

#ifdef	__STDC__
	va_start (ap, format);
#else
	va_start (ap);
#endif
	vprintf (format, ap);
	va_end (ap);
	exit (1);
}

int			am_in_background;

#endif	/*  UNIT_TEST  */

/**
 ** lpfsck()
 **/

#define	F	0
#define D	1
#define P	2
#define S	3
#define	PUBLIC	LPSCHED_SYS_PUBLIC
#define	PRIVATE	LPSCHED_SYS_PRIVATE


#ifdef	__STDC__
static void		proto (int, int, ...);
static char *		va_makepath(va_list *);
static void		_rename (char *, char *, ...);
#else
static void		proto();
static char *		va_makepath();
static void		_rename();
#endif

/*
 * Procedure:     lpfsck
 *
 * Restrictions:
 *               Lstat: None
 *               Unlink: None
 *               rmdir(2): None
 *               Readlink: None
*/
void
#ifdef	__STDC__
lpfsck (void)
#else
lpfsck ()
#endif
{
	DEFINE_FNNAME (lpfsck)

	char *			cmd;
	char *			symbolic	= 0;
	char *			real_dir;
	char *			old_system;

	struct stat		stbuf;

	int			len;
	int			real_am_in_background = am_in_background;


	/*
	 * Force log messages to go into the log file instead of stdout.
	 */
	am_in_background = 1;

	/*
	 * These lines should match what is in the prototype file
	 * for the packaging! (In fact, it probably ought to be
	 * generated from that file, but that work is for a rainy day...)
	 */

	/*
	 * DIRECTORIES:
	 */
proto (D, 0,  Lp_A, NULL,		       0775, Lp_Uid, Lp_Gid, PUBLIC);
proto (D, 1,  Lp_A_Classes, NULL,	       0775, Lp_Uid, Lp_Gid, PUBLIC);
proto (D, 1,  Lp_A_Forms, NULL,		       0775, Lp_Uid, Lp_Gid, PUBLIC);
proto (D, 1,  Lp_A_Interfaces, NULL,	       0775, Lp_Uid, Lp_Gid, PUBLIC);
proto (D, 1,  Lp_A_Printers, NULL,	       0775, Lp_Uid, Lp_Gid, PUBLIC);
proto (D, 1,  Lp_A_PrintWheels, NULL,	       0775, Lp_Uid, Lp_Gid, PUBLIC);
proto (D, 0,  "/var/lp", NULL,		       0775, Lp_Uid, Lp_Gid, PUBLIC);
proto (D, 1,  Lp_Logs, NULL,		       0775, Lp_Uid, Lp_Gid, PUBLIC);
proto (D, 1,  Lp_Spooldir, NULL,	       0775, Lp_Uid, Lp_Gid, PUBLIC);
proto (D, 1,  Lp_Admins, NULL,		       0775, Lp_Uid, Lp_Gid, PUBLIC);
proto (D, 0,  Lp_Spooldir, FIFOSDIR, NULL,     0775, Lp_Uid, Lp_Gid, PUBLIC);
proto (D, 1,  Lp_Private_FIFOs, NULL,	       0771, Lp_Uid, Lp_Gid, PUBLIC);
proto (D, 1,  Lp_Public_FIFOs, NULL,	       0773, Lp_Uid, Lp_Gid, PUBLIC);
proto (D, 1,  Lp_Requests, NULL,	       0775, Lp_Uid, Lp_Gid, PRIVATE);
proto (D, 1,  Lp_Requests, Local_System, NULL, 0770, Lp_Uid, Lp_Gid, PRIVATE);
proto (D, 1,  Lp_System, NULL,		       0775, Lp_Uid, Lp_Gid, PUBLIC);
proto (D, 1,  Lp_Tmp, NULL,		       0771, Lp_Uid, Lp_Gid, PUBLIC);
proto (D, 1,  Lp_Tmp, Local_System, NULL,      0775, Lp_Uid, Lp_Gid, PUBLIC);
proto (D, 1,  Lp_NetTmp, NULL,		       0770, Lp_Uid, Lp_Gid, PUBLIC);

	/*
	 * The lpNet <-> lpsched job transfer directories.
	 * Strictly used for temporary file transfer, on start-up
	 * we can safely clean them out. Indeed, we should clean
	 * them out in case we had died suddenly and are now
	 * restarting. The directories should never be very big,
	 * so we are not in danger of getting ``arglist too big''.
	 */
proto (D, 1,  Lp_NetTmp, "tmp", NULL,
	0770, Lp_Uid, Lp_Gid, PRIVATE);
proto (D, 1,  Lp_NetTmp, "tmp", Local_System, NULL,
	0770, Lp_Uid, Lp_Gid, PRIVATE);
proto (D, 1,  Lp_NetTmp, "requests", NULL,
	0770, Lp_Uid, Lp_Gid, PRIVATE);
proto (D, 1,  Lp_NetTmp, "requests", Local_System, NULL,
	0770, Lp_Uid, Lp_Gid, PRIVATE);
	cmd = makestr(RMCMD, " ", Lp_NetTmp, "/tmp/*/*", (char *)0);
	(void) system (cmd);
	Free (cmd);
	cmd = makestr(RMCMD, " ", Lp_NetTmp, "/requests/*/*", (char *)0);
	(void) system (cmd);
	Free (cmd);

	/*
	 * THE MAIN FIFO:
	 */
proto (P, 1,  Lp_FIFO, NULL, 0666, Lp_Uid, Lp_Gid, PUBLIC);

	/*
	 * SYMBOLIC LINKS:
	 * Watch out! These names are given in the reverse
	 * order found in the prototype file (sorry!)
	 */
proto (S, 1,  Lp_Model, NULL, "/etc/lp/model", NULL,
	0777, Lp_Uid, Lp_Gid, PUBLIC);
proto (S, 1,  Lp_Logs, NULL,  "/etc/lp/logs", NULL,
	0777, Lp_Uid, Lp_Gid, PUBLIC);
/*     S, 1,  Lp_Tmp, Local_System, ...    DONE BELOW */
proto (S, 1,  Lp_Bin, NULL,   Lp_Spooldir, "bin", NULL,
	0777, Lp_Uid, Lp_Gid, PUBLIC);
proto (S, 1,  Lp_A, NULL,     Lp_Admins, "lp", NULL,
	0777, Lp_Uid, Lp_Gid, PUBLIC);

	/*
	 * OTHER FILES:
	 */
proto (F, 1,  Lp_NetData, NULL, 0664, Lp_Uid, Lp_Gid, PRIVATE);

	/*
	 * SPECIAL CASE:
	 * If the "temp" symbolic link already exists,
	 * but is not correct, assume the machine's nodename changed.
	 * Rename directories that include the nodename, if possible,
	 * so that unprinted requests are saved. Then change the
	 * symbolic link.
	 * Watch out for a ``symbolic link'' that isn't!
	 */
	if (Lstat(Lp_Temp, &stbuf) == 0)
	    switch (stbuf.st_mode & S_IFMT) {

	    default:
		(void) Unlink (Lp_Temp);
		break;

	    case S_IFDIR:
		(void) Rmdir (Lp_Temp);
		break;

	    case S_IFLNK:
		symbolic = Malloc(stbuf.st_size + 1);
		if ((len = Readlink(Lp_Temp, symbolic, stbuf.st_size))) {
			symbolic[len] = 0;
			real_dir = makepath(Lp_Tmp, Local_System, NULL);
			if (!STREQU(real_dir, symbolic)) {
				if (!(old_system = strrchr(symbolic, '/')))
					old_system = symbolic;
				else
					old_system++;

				/*
				 * The "rename()" system call (buried
				 * inside the "_rename()" routine) should
				 * succeed, even though we blindly created
				 * the new directory earlier, as the only
				 * directory entries should be . and ..
				 * (although if someone already created
				 * them, we'll note the fact).
				 */
	_rename (old_system, Local_System,   Lp_Tmp, NULL);
	_rename (old_system, Local_System,   Lp_Requests, NULL);
	_rename (old_system, Local_System,   Lp_NetTmp, "tmp", NULL);
	_rename (old_system, Local_System,   Lp_NetTmp, "requests", NULL);

				(void) Unlink (Lp_Temp);

			}
			Free (real_dir);
		}
		Free (symbolic);
		break;
	    }

proto (S, 1,  Lp_Tmp, Local_System, NULL, Lp_Temp, NULL,
	0777, Lp_Uid, Lp_Gid, PUBLIC);

	am_in_background = real_am_in_background;
	return;
}

/*
 * Procedure:     proto
 *
 * Restrictions:
 *               stat(2): None
 *               Symlink: None
 *               Unlink: None
 *               mkdir(2): None
 *               Creat: None
 *               lchown(2): None
 *               Chmod: None
 *               Chown: None
 *               lvlfile(2): None
*/

static void
#ifdef	__STDC__
proto (
	int			type,
	int			rm_ok,
	...
)
#else
proto (type, rm_ok, va_alist)
	int			type,
				rm_ok;
	va_dcl
#endif
{
	DEFINE_FNNAME (proto)

	va_list			ap;
	char			*path,
				*symbolic;
	int			exist;
	mode_t			mode;
	uid_t			uid;
	gid_t			gid;
	level_t			lid;
	struct stat		stbuf;


#ifdef	__STDC__
	va_start (ap, type);
#else
	va_start (ap);
#endif

	path = va_makepath(&ap);

	exist = (stat(path, &stbuf) == 0);

	switch (type) {

	case S:
		if (!exist)
			fail ("%s is missing!\n", path);
		symbolic = va_makepath(&ap);
		(void) Symlink (path, symbolic);
		break;

	case D:
		if (exist && (stbuf.st_mode & S_IFDIR) == 0)
			if (!rm_ok)
				fail ("%s is not a directory!\n", path);
			else {
				(void) Unlink (path);
				exist = 0;
			}
		if (!exist)
			(void) Mkdir (path, 0);
		break;

	case F:
		if (exist && (stbuf.st_mode & S_IFREG) == 0)
			if (!rm_ok)
				fail ("%s is not a file!\n", path);
			else {
				(void) Unlink (path);
				exist = 0;
			}
		if (!exist)
			(void) Close(Creat(path, 0));
		break;

	case P:
		/*
		 * Either a pipe or a file.
		 */
		if (exist && (stbuf.st_mode & (S_IFREG|S_IFIFO)) == 0)
			if (!rm_ok)
				fail ("%s is not a file or pipe!\n", path);
			else {
				(void) Unlink (path);
				exist = 0;
			}
		if (!exist)
			(void) Close(Creat(path, 0));
		break;

	}

	mode = va_arg(ap, mode_t);
	uid = va_arg(ap, uid_t);
	gid = va_arg(ap, gid_t);
	lid = va_arg(ap, level_t);

	switch (type) {
	case	S:
		(void) lchown (symbolic, uid, gid);
		Free (symbolic);
		break;

	default:
		(void) Chmod (path, mode);
		(void) Chown (path, uid, gid);
		(void) lvlfile (path, MAC_SET, &lid);
	}
	Free (path);
	return;
}

static char *
#ifdef	__STDC__
va_makepath (
	va_list *		pap
)
#else
va_makepath (pap)
	va_list			*pap;
#endif
{
	DEFINE_FNNAME (va_makepath)

	va_list			ap_start	= *pap;

	char			*component,
				*p,
				*q;

	int			len;

	char			*ret;


	for (len = 0; (component = va_arg((*pap), char *)); )
		len += strlen(component) + 1;

	if (!len) {
		errno = 0;
		return (0);
	}

	ret = Malloc(len);

	*pap = ap_start;
	for (p = ret; (component = va_arg((*pap), char *)); ) {
		for (q = component; *q; )
			*p++ = *q++;
		*p++ = '/';
	}
	p[-1] = 0;

	return (ret);
}

/*
 * Procedure:     _rename
 *
 * Restrictions:
 *               Rename: None
*/

static void
#ifdef	__STDC__
_rename (
	char *			old_system,
	char *			new_system,
	...
)
#else
_rename (old_system, new_system, va_alist)
	char *			old_system;
	char *			new_system;
	va_dcl
#endif
{
	DEFINE_FNNAME (_rename)

	va_list			ap;

	char *			prefix;
	char *			old;
	char *			new;


#ifdef	__STDC__
	va_start (ap, new_system);
#else
	va_start (ap);
#endif
	prefix = va_makepath(&ap);
	va_end (ap);

	old = makepath(prefix, old_system, (char *)0);
	new = makepath(prefix, new_system, (char *)0);

	if (Rename(old, new) == 0)
		note ("Renamed %s to %s.\n", old, new);
	else
	if (errno == EEXIST)
		note (
			"Rename of %s to %s failed because %s exists.\n",
			old,
			new,
			new
		);
	else
		fail (
			"Rename of %s to %s failed (%s).\n",
			old,
			new,
			PERROR
		);

	Free (new);
	Free (old);
	Free (prefix);

	return;
}
