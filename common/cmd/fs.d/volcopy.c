/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)fs.cmds:common/cmd/fs.d/volcopy.c	1.10.9.3"
#ident  "$Header: volcopy.c 1.2 91/06/27 $"


/***************************************************************************
 * Command: volcopy
 * Inheritable Privileges: P_SYSOPS P_MACREAD,P_MACWRITE,P_DACREAD,
 *				P_DACWRITE,P_DEV,P_SETFLEVEL
 *       Fixed Privileges: None
 * Notes:
 *
 ***************************************************************************/

#include	<stdio.h>
#include 	<limits.h>
#include	<errno.h>
#include	<varargs.h>
#include	<sys/vfstab.h>
#include	<sys/types.h>
#include	<priv.h>

#define	ARGV_MAX	1024
#define	FSTYPE_MAX	8

#define	VFS_PATH	"/usr/lib/fs"
#define usage	\
		"Usage:\nvolcopy [-F FSType] [-V] [current_options] [-o specific_options] operands\n"

#define	EQ(X,Y,Z)	!strncmp(X,Y,Z)
#define	NEWARG()\
	(nargv[nargc++] = &argv[1][0],\
	 nargc == ARGV_MAX ? perr("volcopy:  too many arguments.\n") : 1)

extern int	errno;

char	*nargv[ARGV_MAX];
int	nargc = 2;

char	vfstab[] = VFSTAB;


/*
 * Procedure:     main
 *
 * Restrictions:
 *                fopen: P_MACREAD;
 *                getvfsany: none
 *                rewind: none
 *                fclose: none
 *                printf: none
 */

main(argc, argv)
	int	argc;
	char	**argv;
{
	register char	cc;
	register int	ii, Vflg = 0, Fflg = 0;
	register char	*fstype = NULL;
	register FILE	*fd;
	struct vfstab	vget, vref;

	while (argc > 1 && argv[1][0] == '-') {
		if (EQ(argv[1], "-a", 2)) {
			NEWARG();
		} else if (EQ(argv[1], "-e", 2)) {
			NEWARG();
		} else if (EQ(argv[1], "-s", 2)) {
			NEWARG();
		} else if (EQ(argv[1], "-y", 2)) {
			NEWARG();
		} else if (EQ(argv[1], "-buf", 4)) {
			NEWARG();
		} else if (EQ(argv[1], "-bpi", 4)) {
			NEWARG();
			if ((cc = argv[1][4]) < '0' || cc > '9') {
				++argv;
				--argc;
				NEWARG();
			}
		} else if (EQ(argv[1], "-feet", 5)) {
			NEWARG();
			if ((cc = argv[1][5]) < '0' || cc > '9') {
				++argv;
				--argc;
				NEWARG();
			}
		} else if (EQ(argv[1], "-reel", 5)) {
			NEWARG();
			if ((cc = argv[1][5]) < '0' || cc > '9') {
				++argv;
				--argc;
				NEWARG();
			}
		} else if (EQ(argv[1], "-r", 2)) { /* 3b15 only */
			NEWARG();
			if ((cc = argv[1][2]) < '0' || cc > '9') {
				++argv;
				--argc;
				NEWARG();
			}
		} else if (EQ(argv[1], "-block", 6)) { /* 3b15 only */
			NEWARG();
			if ((cc = argv[1][6]) < '0' || cc > '9') {
				++argv;
				--argc;
				NEWARG();
			}
		} else if (EQ(argv[1], "-V", 2)) {
			Vflg++;
		} else if (EQ(argv[1], "-F", 2)) {
			if (Fflg) 
				perr("volcopy: More than one FSType specified.\n%s", usage);
			Fflg++;
			if (argv[1][2] == '\0') {
				++argv;
				--argc;
				fstype = &argv[1][0];
			} else
				fstype = &argv[1][2];
			if(!fstype)
				perr(usage);
			else
				 if (strlen(fstype) > FSTYPE_MAX)
					perr("volcopy: FSType %s exceeds %d characters\n", fstype, FSTYPE_MAX);
		} else if (EQ(argv[1], "-o", 2)) {
			NEWARG();
			if (argv[1][2] == '\0') {
				++argv;
				--argc;
				NEWARG();
			}
		} else if (EQ(argv[1], "-nosh", 5)) { /* 3b15 only */
			NEWARG();
		} else if (EQ(argv[1], "-?", 2)) { 
			if (Fflg) {
				nargv[2] = "-?";
				doexec(fstype, nargv);
			}
			else 
				perr(usage);
		} else 
			perr("<%s> invalid option\n%s",argv[1], usage);
		++argv;
		--argc;
	} /* argv[1][0] == '-' */

	if (argc != 6) /* if mandatory fields not present */
		perr(usage);

	if (nargc + 5 >= ARGV_MAX)
		perr("volcopy: too many arguments.\n");

	for (ii = 0; ii < 5; ii++)
		nargv[nargc++] = argv[ii+1];

	if (fstype == NULL) {
		procprivl(CLRPRV, MACREAD_W,0);
		if ((fd = fopen(vfstab, "r")) == NULL)
			perr("volcopy: cannot open %s.\n", vfstab);
		procprivl(SETPRV, MACREAD_W,0);

		vfsnull(&vref);
		vref.vfs_special = argv[2];
		ii = getvfsany(fd, &vget, &vref);
		if (ii == -1) {
			rewind(fd);
			vfsnull(&vref);
			vref.vfs_fsckdev = argv[2];
			ii = getvfsany(fd, &vget, &vref);
		}

		fclose(fd);

		switch (ii) {
		case -1:
			perr("volcopy: File system type cannot be identified.\n");
			break;
		case 0:
			fstype = vget.vfs_fstype;
			break;
		case VFS_TOOLONG:
			perr("volcopy: line in vfstab exceeds %d characters\n", VFS_LINE_MAX-2);
			break;
		case VFS_TOOFEW:
			perr("volcopy: line in vfstab has too few entries\n");
			break;
		case VFS_TOOMANY:
			perr("volcopy: line in vfstab has too many entries\n");
			break;
		}
	}

	if (Vflg) {
		printf("volcopy -F %s", fstype);
		for (ii = 2; nargv[ii]; ii++)
			printf(" %s", nargv[ii]);
		printf("\n");
		exit(0);
	}

	doexec(fstype, nargv);
}


/*
 * Procedure:     doexec
 *
 * Restrictions:
 *                sprintf: none
 *                execv(2): P_MACREAD;
 */

doexec(fstype, nargv)
	char	*fstype, *nargv[];
{
	char	full_path[PATH_MAX];
	char	*vfs_path = VFS_PATH;

	/* build the full pathname of the fstype dependent command. */
	sprintf(full_path, "%s/%s/volcopy", vfs_path, fstype);

	/* set the new argv[0] to the filename */
	nargv[1] = "volcopy";

	/* Try to exec the fstype dependent portion of the mount. */
	procprivl(CLRPRV,MACREAD_W,0);
	execv(full_path, &nargv[1]);
	if (errno == EACCES) {
		perr("volcopy: cannot execute %s - permission denied\n", full_path);
		exit(1);
	}
	if (errno == ENOEXEC) {
		nargv[0] = "sh";
		nargv[1] = full_path;
		execv("/sbin/sh", &nargv[0]);
	}
	perr("volcopy: Operation not applicable for FSType %s\n", fstype);
	exit(1);
}


/*
 * Procedure:     perr
 *
 * Restrictions:
 *               vfprintf:  none
 * Notes:
 *	Print error messages.
 */

int
perr(va_alist)
va_dcl
{
	register char *fmt_p;
	va_list v_Args;

	va_start(v_Args);
	fmt_p = va_arg(v_Args, char *);
	(void)vfprintf(stderr, fmt_p, v_Args);
	va_end(v_Args);
	exit(1);
}
