/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)s5.cmds:s5/mount.c	1.15.12.12"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/fs.d/s5/mount.c,v 1.1 91/02/28 17:26:24 ccs Exp $"

#include	<stdio.h>
#include	<sys/signal.h>
#include	<unistd.h>	/* defines F_LOCK for lockf */
#include	<sys/errno.h>
#include	<sys/mnttab.h>
#include	<sys/mount.h>	/* exit code definitions */
#include	<sys/types.h>
#include	<sys/statvfs.h>

#define	TIME_MAX	16
#define	NAME_MAX	64	/* sizeof "fstype myname" */

#define	FSTYPE		"s5"

#define	RO_BIT		1

#define	READONLY	0
#define	READWRITE	1
#define SUID		2
#define NOSUID		3
#define REMOUNT		4

extern int	errno;
extern int	optind;
extern char	*optarg;

extern char	*strrchr();
extern time_t	time();

/* Forward definitions */
void	do_mount();

char	typename[NAME_MAX], *myname;
char	mnttab[] = MNTTAB;
char	temp[] = "/etc/mnttab.tmp";
char	fstype[] = FSTYPE;
char	*myopts[] = {
	"ro",
	"rw",
	"suid",
	"nosuid",
	"remount",
	NULL
};

main(argc, argv)
	int	argc;
	char	**argv;
{
	FILE	*fwp,*frp;
	char	*special, *mountp;
	char	*options, *value;
	char	tbuf[TIME_MAX];
	int	errflag = 0;
	int	confflag = 0;
	int 	suidflg = 0; 
	int 	nosuidflg = 0;
	int	remountflg = 0;
	int	mntflag =0;
	int	cc, ret, rwflag = 0;
	struct mnttab	mm,mget;
	int	roflag = 0;

	myname = strrchr(argv[0], '/');
	if (myname)
		myname++;
	else
		myname = argv[0];
	sprintf(typename, "%s %s", fstype, myname);
	argv[0] = typename;

	/*
	 *	check for proper arguments
	 */

	while ((cc = getopt(argc, argv, "?o:r")) != -1)
		switch (cc) {
		case 'r':
			if ((roflag & RO_BIT) || rwflag )
				confflag = 1;
			else if (rwflag)
				confflag = 1;
			else {
				roflag |= RO_BIT;
				mntflag |= MS_RDONLY;
			}
			break;
		case 'o':
			options = optarg;
			while (*options != '\0')
				switch (getsubopt(&options, myopts, &value)) {
				case READONLY:
					if (rwflag || remountflg)
						confflag = 1;
					else {
						roflag |= RO_BIT;
						mntflag |= MS_RDONLY;
					}
					break;
				case READWRITE:
					if (roflag & RO_BIT)
						confflag = 1;
					else if (rwflag)
						errflag = 1;
					else
						rwflag = 1;
					break;
				case SUID:
					if (nosuidflg)
						confflag = 1;
					else if (suidflg)
						errflag = 1;
					else 
						suidflg++; 
					break;
				case NOSUID:
					if (suidflg)
						confflag = 1;
					else if (nosuidflg)
						errflag = 1;
					else {
						mntflag |= MS_NOSUID;
						nosuidflg++; 
					}
					break;
				case REMOUNT:
					if (roflag)
						confflag = 1;
					else if (remountflg)
						errflag = 1;
					else if (roflag & RO_BIT)
						confflag = 1;
					else {
						remountflg++;
						mntflag |= MS_REMOUNT;
					}
					break;
				default:
					fprintf(stderr, "%s: illegal -o suboption -- %s\n", typename, value);
					errflag++;
				}
			break;
		case '?':
			errflag = 1;
			break;
		}


	/*
	 *	There must be at least 2 more arguments, the
	 *	special file and the directory.
	 */

	if (confflag) {
		fprintf(stderr, "s5 %s: warning: conflicting suboptions\n", myname);
		usage();
	}
		
	if ( ((argc - optind) != 2) || (errflag) )
		usage();

	special = argv[optind++];
	mountp = argv[optind++];


	mm.mnt_special = special;
	mm.mnt_mountp = mountp;
	mm.mnt_fstype = fstype;
	if (roflag & RO_BIT)
		mm.mnt_mntopts = "ro";
	else
		mm.mnt_mntopts = "rw";
	if (nosuidflg)
		strcat(mm.mnt_mntopts, ",nosuid");
	else
		strcat(mm.mnt_mntopts, ",suid");
	sprintf(tbuf, "%ld", time(0L));	/* assumes ld == long == time_t */
	mm.mnt_time = tbuf;

	if ((fwp = fopen(mnttab, "r")) == NULL) {
		fprintf(stderr, "%s: warning: cannot open mnttab\n", myname);
	}

	/* Open /etc/mnttab read-write to allow locking the file */
	if ((fwp = fopen(mnttab, "r+")) == NULL) {
		fprintf(stderr, "UX:s5 %s: cannot open mnttab\n", myname);
		exit(RET_MNT_OPEN);
	}

	/*
	 * Lock the file to prevent many updates at once.
	 * This may sleep for the lock to be freed.
	 * This is done to ensure integrity of the mnttab.
	 */
	if (lockf(fileno(fwp), F_LOCK, 0L) < 0) {
		fprintf(stderr, "UX:s5 %s: cannot lock mnttab\n", myname);
		perror(myname);
		exit(RET_MNT_LOCK);
	}

	signal(SIGHUP,  SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGINT,  SIG_IGN);

	/*
	 *	Perform the mount.
	 *	Only the low-order bit of "roflag" is used by the system
	 *	calls (to denote read-only or read-write).
	 */

	do_mount(special, mountp, mntflag);
	if (remountflg) {
		if ((frp = fopen(temp, "w")) == NULL) {
			fprintf(stderr, "UX:s5 %s: cannot open %s for writing\n", myname, temp);
			exit(RET_TMP_OPEN);
		}
	
		rewind(fwp);
		/* Make sure writes happen right away, so we see errors.*/
		setbuf(frp, NULL);
		/* for each entry ... */
		while ((ret = getmntent(fwp, &mget)) != -1)  {
			/* if it's a valid entry and not the one we got above ... */
			if ( ret == 0 && (strcmp(mm.mnt_special, mget.mnt_special) != 0))
				/* put it out */
				if (putmntent(frp, &mget) <0) {
					fprintf(stderr,
					"UX: s5 %s: cannot write to %s\n",
					myname, temp);
					exit(RET_TMP_WRITE);
				}
		}

		putmntent(frp, &mm);
		fclose(frp);
		rename(temp,mnttab);
		fclose(fwp);
	}
	else {
		fseek(fwp, 0L, 2);
		putmntent(fwp, &mm);
		fclose(fwp);
	}

	exit(RET_OK);
	/* NOTREACHED */
}

rpterr(bs, mp)
	register char *bs, *mp;
{
	switch (errno) {
	case EPERM:
		fprintf(stderr, "UX:s5 %s: permission denied\n", myname);
		return(RET_EPERM);
	case ENXIO:
		fprintf(stderr, "UX:s5 %s: %s no such device\n", myname, bs);
		return(RET_ENXIO);
	case ENOTDIR:
		fprintf(stderr,
			"UX:s5 %s: %s not a directory\n\tor a component of %s is not a directory\n",
			myname, mp, bs);
		return(RET_ENOTDIR);
	case ENOENT:
		fprintf(stderr, "UX: s5 %s: %s or %s, no such file or directory or no previous mount was performed\n", myname, bs, mp);
		fprintf(stderr, "UX:s5 %s: %s or %s, no such file or directory\n",
			myname, bs, mp);
		return(RET_ENOENT);
	case EINVAL:
		fprintf(stderr, "UX:s5 %s: %s is not an s5 file system,\n\tor %s is busy.\n", myname, bs, mp);
		return(RET_EINVAL);
	case EBUSY:
		fprintf(stderr,
			"UX:s5 %s: %s is already mounted, %s is busy,\n\tor allowable number of mount points exceeded\n",
			myname, bs, mp);
		return(RET_EBUSY);
	case ENOTBLK:
		fprintf(stderr, "UX:s5 %s: %s not a block device\n", myname, bs);
		return(RET_ENOTBLK);
	case EROFS:
		fprintf(stderr, "UX:s5 %s: %s write-protected\n", myname, bs);
		return(RET_EROFS);
	case ENOSPC:
		fprintf(stderr, "UX:s5 %s: %s is corrupted. needs checking\n", myname, bs);
		return(RET_ENOSPC);
	case ENODEV:
		fprintf(stderr, "UX:s5 %s: %s no such device or device is write-protected\n", myname, bs);
		return(RET_ENODEV);
	case ENOLOAD:
		fprintf(stderr, "UX:s5 %s: s5 file system module cannot be loaded\n", myname);
		return(RET_ENOLOAD);
	default:
		perror(myname);
		fprintf(stderr, "UX:s5 %s: cannot mount %s\n", myname, bs);
		return(RET_MISC);
	}
}

void
do_mount(special, mountp, flag)
	char	*special, *mountp;
	int	flag;
{
	register char *ptr;
	struct statvfs stbuf;

	if (mount(special, mountp, flag | MS_DATA, fstype, NULL, 0))
 		exit(rpterr(special, mountp));

	/*
	 *	compare the basenames of the mount point
	 *	and the volume name, warning if they differ.
	 */

	if (statvfs(mountp, &stbuf) == -1)
		return;

	ptr = stbuf.f_fstr;
	while (*ptr == '/')
		ptr++;

}

usage()
{
	fprintf(stderr,
		"%s Usage:\n%s [-F %s] [generic_options] [-r] [-o {[rw|ro],[suid|nosuid],[remount]}] {special | mount_point}\n%s [-F %s] [generic_options] [-r] [-o {[rw|ro],[suid|nosuid],[remount]}] special mount_point\n",
		fstype, myname, fstype, myname, fstype);
	exit(RET_USAGE);
	/* NOTREACHED */
}
