/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)acct:common/cmd/acct/vxdiskusg.c	1.3"
#ident "$Header: $"

/*
 * Copyright (c) 1991, 1992 VERITAS Software Corporation.  ALL RIGHTS RESERVED.
 * UNPUBLISHED -- RIGHTS RESERVED UNDER THE COPYRIGHT
 * LAWS OF THE UNITED STATES.  USE OF A COPYRIGHT NOTICE
 * IS PRECAUTIONARY ONLY AND DOES NOT IMPLY PUBLICATION
 * OR DISCLOSURE.
 * 
 * THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 * TRADE SECRETS OF VERITAS SOFTWARE.  USE, DISCLOSURE,
 * OR REPRODUCTION IS PROHIBITED WITHOUT THE PRIOR
 * EXPRESS WRITTEN PERMISSION OF VERITAS SOFTWARE.
 * 
 *               RESTRICTED RIGHTS LEGEND
 * USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT IS
 * SUBJECT TO RESTRICTIONS AS SET FORTH IN SUBPARAGRAPH
 * (C) (1) (ii) OF THE RIGHTS IN TECHNICAL DATA AND
 * COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013.
 *               VERITAS SOFTWARE
 * 4800 GREAT AMERICA PARKWAY, SUITE 420, SANTA CLARA, CA 95054
 */

/*
 * Portions Copyright 1992, 1991 UNIX System Laboratories, Inc.
 * Portions Copyright 1990 - 1984 AT&T
 * All Rights Reserved
 */

/*
 * The diskusg program for FSType vxfs.  This allows running against a
 * vxfs snapshot file system - though the script that usually drives
 * diskusg (dodisk) should have no call to do so.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <pwd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mnttab.h>
#include <sys/param.h>
#include <sys/vnode.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/fs/vx_fs.h>
#include <sys/fs/vx_inode.h>
#include <sys/fs/vx_param.h>
#include <sys/fs/vx_ioctl.h>
#include "acctdef.h"


/*
 * a bit pattern for fs_open() and fs_cksblock() to control
 * what fields are actually validated in the superblock.
 */

#define	FS_CKMAGIC	0x01
#define	FS_CKVERSION	0x02
#define	FS_CKSUM	0x04

/*
 * holds all the information about an open file system.  
 * if the file system is a snapshot, fs_mntpt and fs_mntfd
 * are also filled in
 */

struct fs_dat {
	struct fs	fsd_fs;		/* superblock data */
	char		*fsd_spec;	/* special file */
	int		fsd_fd;		/* file descriptor for special */
	off_t		fsd_off;	/* current offset */
	int		fsd_issnapshot;	/* true if snapshot */
	char 		*fsd_mntpt;	/* mount point if snapshot */
	int		fsd_mntfd;	/* fd for mount point if snapshot */
};

struct fs_dat	*fs_datalloc();
void	fs_datfree(struct fs_dat *);
char	*fs_open(struct fs_dat *, char *, int, int);
int	fs_close(struct fs_dat *);
off_t	fs_lseek(struct fs_dat *, off_t, int);
int	fs_read(struct fs_dat *, void *, unsigned);
int	fs_write(struct fs_dat *, void *, unsigned);
char	*fs_cksblock(struct fs *, int);
char	*fs_findbspec(char *);
char	*fs_convto_cspec(char *);
char	*fs_convto_bspec(char *);
int	fs_ckspecmnt(char *, char *);


#define max(a,b)	((int)(a) > (int)(b) ? (a) : (b))
#define IBUFSIZE 32768

ino_t	ino;			/* used by ilist() and count() */
long	lseek();
int	VERBOSE = 0;
FILE	*ufd = 0;	/* file ptr for file where unacct'd for fileusg goes */
char	*ignlist[MAXIGN];	/* ignore list of filesystem names */
int	igncnt = {0};
char	*cmd;
unsigned hash();
struct	fs *fs;
struct	fs_dat *fs_datp;
void	getblk();

struct acct  {
	uid_t	uid;
	long	usage;
	char	name [MAXNAME+1];
} userlist [MAXUSERS];

main(argc, argv)
int argc;
char **argv;
{
	extern	int	optind;
	extern	char	*optarg;
	register c;
	register FILE	*fd;
	int	sflg = {FALSE};
	char 	*pfile = NULL;
	int	errfl = {FALSE};
	int	i; 
	size_t	len;
	char 	*errmsg;

	cmd = argv[0];
	while((c = getopt(argc, argv, "vu:p:si:")) != EOF) switch(c) {
	case 's':
		sflg = TRUE;
		break;
	case 'v':
		VERBOSE = 1;
		break;
	case 'i':
		ignore(optarg);
		break;
	case 'u':
		ufd = fopen(optarg, "a");
		break;
	case 'p':
		pfile = optarg;
		break;
	case '?':
		errfl++;
		break;
	}
	if(errfl) {
		fprintf(stderr, "Usage: %s [-sv] [-p pw_file] [-u file] [-i ignlist] [file ...]\n", cmd);
		exit(10);
	}

	hashinit();
	if(sflg == TRUE) {
		if(optind == argc){
			adduser(stdin);
		} else {
			for( ; optind < argc; optind++) {
				if( (fd = fopen(argv[optind], "r")) == NULL) {
					fprintf(stderr, "%s: Cannot open %s\n", cmd, argv[optind]);
					continue;
				}
				adduser(fd);
				fclose(fd);
			}
		}
	} else {
		setup(pfile);
		for( ; optind < argc; optind++) {
			if ((fs_datp = fs_datalloc()) == NULL) {
				fprintf(stderr, 
					"%s: Cannot allocate space for super block.\n", 
					cmd);
				exit(15);
			}
			errmsg = fs_open(fs_datp, argv[optind], O_RDONLY,
					  FS_CKMAGIC | FS_CKVERSION | FS_CKSUM);
			if (errmsg) {
				fprintf(stderr, "%s: %s", cmd, errmsg);
				continue;
			}
			fs = &fs_datp->fsd_fs;
			for(i = 0; i < igncnt; i++) {
				len = max(strlen(fs->fs_fname), strlen(ignlist[i])); 
				if(strncmp(fs->fs_fname, ignlist[i], len) == 0) {
					goto skip;
				}
			}
			ilist(argv[optind]);
skip:
			fs_close(fs_datp);
			fs_datfree(fs_datp);
			if (ufd)  {
				fclose(ufd);
			}
		}
	}
	output();
	exit(0);
	/*NOTREACHED*/
}

ilist(file)
char *file;
{
	int	aun, i, nreads, relino;
	char	ibuf[IBUFSIZE];
	daddr_t	bno;
	struct	dinode *dp;

	sync();

	/*
	 * traverse the ilist by allocation unit
	 */
	ino = 0;
	nreads = (fs->fs_inopau * sizeof (struct dinode) + IBUFSIZE - 1) /
		 sizeof (ibuf);

	for (aun = 0; aun < fs->fs_nau; aun++) {
		bno = fs->fs_fistart + aun * fs->fs_aulen;
		relino = 0;
		for (i = 0; i < nreads; i++) {
			getblk(ibuf, bno, sizeof (ibuf));
			for (dp = (struct dinode *)ibuf;
			     dp < (struct dinode *)(ibuf + IBUFSIZE) &&
			     relino < fs->fs_inopau; dp++, ino++, relino++) {
				if ((dp->di_mode & IFMT) == 0)
					continue;
				if(count(dp) == FAIL) {
					if(VERBOSE) {
						fprintf(stderr, 
							"BAD UID: file system = %s, inode = %u, uid = %ld\n",
							file, ino, dp->di_uid);
					}
					if(ufd) {
						fprintf(ufd, "%s %u %ld\n", file, ino, dp->di_uid);
					}
				}
			}
			bno += IBUFSIZE / fs->fs_bsize;
		}
	}

	return (0);
}

count(ip)
struct dinode *ip;
{
	int index;
	static int	secperblk = 0;

	if (!secperblk)
		secperblk = fs->fs_bsize / DEV_BSIZE;

	if ( ip->di_nlink == 0 || ip->di_mode == 0 )
		return(SUCCEED);
	if( (index = hash(ip->di_uid)) == FAIL || 
		userlist[index].uid == UNUSED )
		return (FAIL);
	userlist[index].usage += ip->di_blocks * secperblk;
	return (SUCCEED);
}


adduser(fd)
register FILE	*fd;
{
	uid_t	usrid;
	long	blcks;
	char	login[MAXNAME+10];
	int 	index;

	while(fscanf(fd, "%ld %s %ld\n", &usrid, login, &blcks) == 3) {
		if( (index = hash(usrid)) == FAIL) return(FAIL);
		if(userlist[index].uid == UNUSED) {
			userlist[index].uid = usrid;
			strncpy(userlist[index].name, login, MAXNAME);
		}
		userlist[index].usage += blcks;
	}
}

ignore(str)
register char	*str;
{
	char	*skip();

	for( ; *str && igncnt < MAXIGN; str = skip(str), igncnt++)
		ignlist[igncnt] = str;
	if(igncnt == MAXIGN) {
		fprintf(stderr, "%s: ignore list overflow. Recompile with larger MAXIGN\n", cmd);
	}
}


output()
{
	int index;

	for (index=0; index < MAXUSERS ; index++)
		if ( userlist[index].uid != UNUSED && userlist[index].usage != 0 )
			printf("%ld	%s	%ld\n",
			    userlist[index].uid,
			    userlist[index].name,
			    userlist[index].usage);
}

unsigned
hash(j)
uid_t j;
{
	register unsigned start;
	register unsigned circle;
	circle = start = (unsigned)j % MAXUSERS;
	do
	{
		if ( userlist[circle].uid == j || userlist[circle].uid == UNUSED )
			return (circle);
		circle = (circle + 1) % MAXUSERS;
	} while ( circle != start);
	return (FAIL);
}

hashinit() 
{
	int index;

	for(index=0; index < MAXUSERS ; index++) {
		userlist[index].uid = UNUSED;
		userlist[index].usage = 0;
		userlist[index].name[0] = '\0';
	}
}

static FILE *pwf = NULL;

setup(pfile)
char	*pfile;
{
	register struct passwd	*pw;
	void end_pwent();
	struct passwd *	(*getpw)();
	void	(*endpw)();
	int index;

	if (pfile) {
		if( !stpwent(pfile)) {
			fprintf(stderr, "%s: Cannot open %s\n", cmd, pfile);
			exit(5);
		}
		getpw = fgetpwent;
		endpw = end_pwent;
	} else {
		setpwent();
		getpw = getpwent;
		endpw = endpwent;
	}
	while ( (pw=getpw(pwf)) != NULL ) {
		if ( (index=hash(pw->pw_uid)) == FAIL ) {
			fprintf(stderr,"%s: INCREASE SIZE OF MAXUSERS\n", cmd);
			return (FAIL);
		}
		if ( userlist[index].uid == UNUSED ) {
			userlist[index].uid = pw->pw_uid;
			strncpy( userlist[index].name, pw->pw_name, MAXNAME);
		}
	}

	endpw();
}

char	*
skip(str)
register char	*str;
{
	while(*str) {
		if(*str == ' ' ||
		    *str == ',') {
			*str = '\0';
			str++;
			break;
		}
		str++;
	}
	return(str);
}

stpwent(pfile)
register char *pfile;
{
	if(pwf == NULL)
		pwf = fopen(pfile, "r");
	else
		rewind(pwf);
	return(pwf != NULL);
}

void
end_pwent()
{
	if(pwf != NULL) {
		(void) fclose(pwf);
		pwf = NULL;
	}
}


void
getblk(buf, bno, siz)
	char	*buf;
	daddr_t	bno;
	off_t	siz;
{
	fs_lseek(fs_datp, bno * fs->fs_bsize, 0);
	if (fs_read(fs_datp, (void *) buf, siz) != siz) {
		fprintf(stderr, "read failed in getblk()\n");
	}
}



/*
 * Below this point are the routines taken from the standard vxfs fs
 * library.  These routines, in the main, are used for encapsulating
 * the handling of snapshot mounted file systems.
 */


#undef	bzero
#define bzero(cp, size)	memset((void *) cp, 0, size)

static char	errbuf[MAXPATHLEN + 80];	/* for generated errors */

char		*fs_findmntpt(char *);
static char	*fs_cksnapmnt(struct fs_dat *);


/*
 * read the superblock of the target file system.  it may
 * be of type vxfs.  if it's a snapshot file system, try to
 * figure out the mountpoint and setup values for future
 * snapreads.
 *
 * if the routine returns successfully, datp is filled  with valid information,
 * including superblock of the file system (snapshot or not).
 *
 * this routine will not necessarily work correctly if spec is
 * a tape, and will leave spec open (until an fs_close() is done),
 * inhibiting rewind.
 *
 * fs_read() and fs_write() won't work on tapes (since they attempt to seek).
 * snapshot file systems on tape don't work.
 *
 * returns NULL on success, or an error message on failure.
 */

char *
fs_open(datp, spec, oflag, ckflags)
	struct fs_dat	*datp;
	char	 *spec;
	int	oflag;
	int	ckflags;
{
	int	fd, len;
	int	fsopt;
	char	*buf;
	char	*bspec;
	char	*errmsg;
	struct stat	statbuf;
	struct fs	*fs;

	if (datp == NULL) {
		return "fs_open: internal error";
	}
	if (stat(spec, &statbuf) != 0) {
		sprintf(errbuf, "stat of %s failed: %s\n",
			spec, strerror(errno));
		return errbuf;
	}

	if (oflag == O_WRONLY) {
		oflag = O_RDWR;
	}
	if ((fd = open(spec, oflag)) < 0) {
		sprintf(errbuf, "open of %s failed: %s\n",
			spec, strerror(errno));
		return errbuf;
	}

	buf = (char *) malloc(VX_MAXBSIZE);
	if (!buf) {
		close(fd);
		return "malloc of space for super block failed\n";
	}
	len = read(fd, buf, VX_MAXBSIZE);
	if (len != VX_MAXBSIZE) {
		if (len < 0) {
			sprintf(errbuf, "read of superblock on %s failed: %s\n",
				spec, strerror(errno));
		} else {
			sprintf(errbuf,
			"read of superblock on %s failed: not enough bytes\n",
				spec);
		}
		goto errout;
	}

	/*
	 * note that ckflags may be 0, and the magic number
	 * may not be valid, even though no error is returned
	 */
	 
	fs = (struct fs *) (buf + VX_SUPERBOFF);
	errmsg = fs_cksblock(fs, ckflags);
	if (errmsg) {
		sprintf(errbuf, "%s: %s\n", spec, errmsg);
		goto errout;
	}

	datp->fsd_fd = fd;
	datp->fsd_spec = strdup(spec);
	datp->fsd_fs = *fs;
	fs_lseek(datp, 0, 0);

	if (fs->fs_magic != VX_SNAPMAGIC) {
		
		/*
		 * it's not a snapshot; we're done
		 */

		datp->fsd_issnapshot = 0;
		free(buf);
		return NULL;
	}

	/*
	 * it's a snapshot file system; try to determine the mountpoint.
	 */

	datp->fsd_issnapshot = 1;
	if ((statbuf.st_mode & S_IFMT) == S_IFCHR) {
		bspec = fs_convto_bspec(spec);
	} else if ((statbuf.st_mode & S_IFMT) == S_IFBLK) {
		bspec = spec;
	} else {
		sprintf(errbuf,
			"%s is not a block or character special file\n", spec);
		goto errout;
	}
	datp->fsd_mntpt = fs_findmntpt(bspec);
	if (!datp->fsd_mntpt) {
		sprintf(errbuf, "snapshot mountpoint for %s (%s) not found\n",
			spec, bspec);
		goto errout;
	}

	if (fs_cksnapmnt(datp)) {
		sprintf(errbuf,
			"mountpoint %s for %s (%s) not snapshot file system\n",
			datp->fsd_mntpt, spec, bspec);
		goto errout;
	}

	/*
	 * now we get the "real" superblock for the snapshot.
	 */

	len = fs_read(datp, (void *) buf, VX_MAXBSIZE);
	if (len != VX_MAXBSIZE) {
		if (len < 0) {
			sprintf(errbuf,
			"snapshot read of superblock on %s (%s) failed: %s\n",
				datp->fsd_mntpt, spec, strerror(errno));
		} else {
			sprintf(errbuf,
	"snapshot read of superblock on %s (%s) failed: not enough bytes\n",
				datp->fsd_mntpt, spec);
		}
		goto errout;
	}
	fs_lseek(datp, 0, 0);
	fs = (struct fs *) (buf + VX_SUPERBOFF);

	errmsg = fs_cksblock(fs, FS_CKMAGIC | FS_CKVERSION | FS_CKSUM);
	if (errmsg) {
		sprintf(errbuf, "%s: %s\n", datp->fsd_mntpt, errmsg);
		goto errout;
	}

	datp->fsd_fs = *fs;
	free(buf);
	return NULL;

errout:
	free(buf);
	close(fd);
	return errbuf;
}


/*
 * Check the putative snapshot mountpoint for perjury.
 *
 * We "round up" the checks since we can't check the version number
 * 'till we have a valid magic number, and can't check the checksums
 * until we have a valid version number.
 */

char *
fs_cksblock(fsp, ckflags)
	struct fs	*fsp;
	int		ckflags;
{
	if (ckflags & (FS_CKMAGIC | FS_CKVERSION | FS_CKSUM)
	    && fsp->fs_magic != VX_MAGIC && fsp->fs_magic != VX_SNAPMAGIC) {
		return "invalid vxfs super block magic";
	}
	if (ckflags & (FS_CKVERSION | FS_CKSUM)
	    && ((fsp->fs_magic == VX_MAGIC && fsp->fs_version != VX_VERSION)
		|| (fsp->fs_magic == VX_SNAPMAGIC
		    && fsp->fs_version != VX_SNAPVERSION))) {
		return "unrecognized vxfs version number";
	}
	if (ckflags & FS_CKSUM && fsp->fs_checksum != VX_FSCHECKSUM(fsp)) {
		return "invalid vxfs superblock checksum";
	}

	return NULL;
}


/*
 * verify that the special file and the mounted snapshot filesystem 
 * correspond to the same file system.
 *
 * return NULL if they match, an errmsg on failure.
 */

static char *
fs_cksnapmnt(datp)
	struct fs_dat *datp;
{
	int	fsopt;
	struct statvfs	vfsbuf;

	if ((datp->fsd_mntfd = open(datp->fsd_mntpt, O_RDONLY)) < 0) {
		return "open of snapshot mountpoint %s failed:";
	}
	if (fstatvfs(datp->fsd_mntfd, &vfsbuf) != 0) {
		return "fstatvfs of snapshot mountpoint %s failed:";
	}
	if (strcmp(vfsbuf.f_basetype, "vxfs") != 0) {
		return "snapshot mountpoint %s is not a vxfs file system";
	}
	if (ioctl(datp->fsd_mntfd, VX_GETFSOPT, &fsopt) != 0) {
		return "VX_GETFSOPT ioctl on snapshot mountpoint %s failed:";
	}
	if (!(fsopt & VX_FSO_SNAPSHOT)) {
		return "file system %s is not a snapshot!";
	}
	if (fs_ckspecmnt(datp->fsd_mntpt, datp->fsd_spec)) {
		return "special file %s doesn't match file system %s";
	}
	return NULL;
}


/*
 * Try to convert a character special pathname of the
 * form /dev/rdsk/0s7, /dev/rroot, or /dev/sd01/rc0t0s1
 * to the corresponding block special pathname.
 * Essentially strip the 'r' from the first '/r' 
 * combination found, and verify that it's a block device.
 * If we're passed a relative pathname, first convert
 * it to absolute before doing our thing.
 *
 * return malloc'ed space holding the path of the block special.
 * return NULL on failure.
 */

char *
fs_convto_bspec(spec)
	char *spec;
{
	char *sp, *bp, *bspec;
	char cdp[2 * MAXPATHLEN];
	struct stat statbuf;

	if (strlen(spec) >= (size_t) MAXPATHLEN) {
		return NULL;
	}
	sp = spec;
	if (*sp != '/') {
		if (getcwd(cdp, 2 * MAXPATHLEN) == NULL) {
			return NULL;
		}
		strcat(cdp, "/");
		strcat(cdp, sp);
		sp = cdp;
	}
		
	bspec = malloc(2 * MAXPATHLEN);
	if (bspec == NULL) {
		return NULL;
	}
	bp = bspec;
	while (*sp) {
		*bp = *sp;
		if (*sp == '/' && *(sp + 1) == 'r') {
			sp += 2;
			bp++;
			break;
		}
		sp++;
		bp++;
	}
	if (!*sp) {
		goto errout;
	}
	while (*bp++ = *sp++) {
	}
	if (stat(bspec, &statbuf) != 0) {
		goto errout;
	}
	if ((statbuf.st_mode & S_IFMT) == S_IFBLK) {
		return bspec;
	}

errout:
	free(bspec);
	return NULL;
}


/*
 * Try to convert a block special pathname of the
 * form /dev/dsk/0s7, /dev/root, or /dev/sd01/c0t0s1
 * to the corresponding character special pathname, e.g.
 * /dev/rdsk/0s7, /dev/rroot, or /dev/sd01/rc0t0s1.
 * Essentially try adding an 'r' after the 2nd to
 * last '/' and after the last '/' and see if we've
 * found a character special file.
 * If we're passed a relative pathname, first convert
 * it to absolute before doing our thing.
 *
 * This should really parse out things like "/dev//vol/./v0",
 * but it doesn't.
 *
 * return malloc'ed space holding the path of the character special.
 * return NULL on failure.
 */

char *
fs_convto_cspec(bspec)
	char *bspec;
{
	char *tp, *cp, *cspec;
	char c;
	struct stat statbuf;

	if (strlen(bspec) >= (size_t) MAXPATHLEN) {
		return NULL;
	}
	cspec = malloc(2 * MAXPATHLEN);
	if (cspec == NULL) {
		return NULL;
	}
	if (*bspec != '/') {
		if (getcwd(cspec, 2 * MAXPATHLEN) == NULL) {
			free(cspec);
			return NULL;
		}
		strcat(cspec, "/");
		strcat(cspec, bspec);
	} else {
		strcpy(cspec, bspec);
	}
		
	/*
	 * try adding an 'r' after the second-to-last '/'
	 */

	cp = strrchr(cspec, '/');
	if (cp) {
		*cp = 0;
		tp = strrchr(cspec, '/');
		*cp = '/';
		cp = tp;
	}
	if (cp) {
		cp++;
		tp = cp + strlen(cp);
		while (tp >= cp) {
			*(tp + 1) = *tp;
			tp--;
		}
		*cp = 'r';

		if (stat(cspec, &statbuf) == 0
		    && (statbuf.st_mode & S_IFMT) == S_IFCHR) {
			return cspec;
		}
	}

	/*
	 * That didn't work.  Fix up the string, and try adding
	 * a 'r' after the last '/'.  cp (if set) still points
	 * at the 'r'.
	 */

	while (*cp) {
		*cp = *(cp + 1);
		cp++;
	}
	cp = strrchr(cspec, '/');
	if (cp) {
		cp++;
		tp = cp + strlen(cp);
		while (tp >= cp) {
			*(tp + 1) = *tp;
			tp--;
		}
		*cp = 'r';

		if (stat(cspec, &statbuf) == 0
		    && (statbuf.st_mode & S_IFMT) == S_IFCHR) {
			return cspec;
		}
	}

	free(cspec);
	return NULL;
}


/*
 * Verify that the passed special corresponds to the passed
 * file system by reading the superblock from the special
 * device and comparing the unchanging fields.  This is only
 * an approximate test.  We remember that the writable fields 
 * may be changing, and that snapshot file systems have a
 * slightly different superblock.
 *
 * Return 0 if they match, 1 if they don't, and -1 if some
 * kind of error prevents us from figuring it out.
 */

int
fs_ckspecmnt(mntpt, spec)
	char *mntpt;
	char *spec;
{
	char *mbufp, *cp;
	struct fs *fs;
	struct statvfs stvbuf;
	int fd;
	int error;

	if ((mbufp = malloc(VX_MAXBSIZE)) == NULL) {
		return -1;
	}
	error = -1;
	if ((fd = open(spec, O_RDONLY)) < 0) {
		goto out;
	}
	if (read(fd, mbufp, VX_MAXBSIZE) != VX_MAXBSIZE) {
		close(fd);
		goto out;
	}
	(void) close(fd);
	if (statvfs(mntpt, &stvbuf) < 0) {
		goto out;
	}

	/*
	 * we've collected all the information for our tests,
	 * now perform them
	 */

	error = 1;
	fs = (struct fs *) (mbufp + VX_SUPERBOFF);
	if (strcmp("vxfs", stvbuf.f_basetype) != 0
	    || fs_cksblock(fs, FS_CKMAGIC)) {
		goto out;
	}

	if (stvbuf.f_blocks != fs->fs_size
	    || stvbuf.f_files != fs->fs_ninode
	    || stvbuf.f_frsize != fs->fs_bsize) {
		goto out;
	}

	if (strncmp(stvbuf.f_fstr, fs->fs_fname, sizeof(fs->fs_fname)) != 0) {
		goto out;
	}
	cp = stvbuf.f_fstr + strlen(stvbuf.f_fstr) + 1;
	if (strncmp(cp, fs->fs_fpack, sizeof(fs->fs_fpack)) != 0) {
		goto out;
	}
	error = 0;

out:
	free(mbufp);
	return error;
}


/*
 * Verify that the passed block and character special files
 * correspond to the same file system device by reading the
 * superblock from each and comparing the contents of fields.  
 *
 * vxfs writes to the file system directly, bypassing the 
 * page cache used by the block device, so we expect the
 * character device to be more current than the block device.
 * The file system can write the superblock at any time.
 *
 * Return 0 if they match, 1 if they don't, and -1 if some
 * kind of error prevents us from figuring it out.
 */

int
fs_ckspecs(cspec, bspec)
	char *cspec, *bspec;
{
	struct fs *cfs, *bfs;
	char *cbufp, *bbufp;
	char *mbufp, *cp;
	int fd;
	int error;

	if ((cbufp = malloc(VX_MAXBSIZE)) == NULL) {
		return -1;
	}
	if ((bbufp = malloc(VX_MAXBSIZE)) == NULL) {
		free(cbufp);
		return -1;
	}

	error = -1;
	if ((fd = open(bspec, O_RDONLY)) < 0) {
		goto out;
	}
	if (read(fd, bbufp, VX_MAXBSIZE) != VX_MAXBSIZE) {
		close(fd);
		goto out;
	}
	(void) close(fd);
	bfs = (struct fs *) (bbufp + VX_SUPERBOFF);

	if ((fd = open(cspec, O_RDONLY)) < 0) {
		goto out;
	}
	if (read(fd, cbufp, VX_MAXBSIZE) != VX_MAXBSIZE) {
		close(fd);
		goto out;
	}
	(void) close(fd);
	cfs = (struct fs *) (cbufp + VX_SUPERBOFF);

	/*
	 * we've collected all the information for our tests,
	 * now perform them
	 */

	error = 1;
	if (cfs->fs_magic != bfs->fs_magic
	    || VX_FSCHECKSUM(cfs) != VX_FSCHECKSUM(bfs)
	    || cfs->fs_size != bfs->fs_size
	    || cfs->fs_ninode != bfs->fs_ninode) {
		goto out;
	}
	if (cfs->fs_ctime != bfs->fs_ctime
	    || cfs->fs_ectime != bfs->fs_ectime) {
		goto out;
	}
	if (cfs->fs_time < bfs->fs_time
	    || (cfs->fs_time == bfs->fs_time
		&& cfs->fs_etime < bfs->fs_etime)) {
		goto out;
	}
	if (memcmp(bfs->fs_fname, cfs->fs_fname, sizeof(bfs->fs_fname)) != 0
	    || memcmp(bfs->fs_fpack, cfs->fs_fpack,
		      sizeof(bfs->fs_fpack)) != 0) {
		goto out;
	}
	error = 0;

out:
	free(cbufp);
 	free(bbufp);
	return error;
}


/*
 * set a specified offset for reading or writing to the
 * the file system; undefined when used on tape drives.
 */
off_t
fs_lseek(datp, off, whence)
	struct fs_dat	*datp;
	off_t	off;
	int	whence;
{
	if (!datp->fsd_issnapshot) {
		datp->fsd_off = lseek(datp->fsd_fd, off, whence);
	} else {
		switch (whence) {
		case 0:
			datp->fsd_off = off;
			break;
		case 1:
			datp->fsd_off += off;
			break;
		case 2:
			
			/*
			 * special files seem to have a size of 0
			 */
			datp->fsd_off = off;
			break;
		default:
			datp->fsd_off = -1;
			break;
		}
	}

	return datp->fsd_off;
}


/*
 * read the specified blocks from a file system; handle snapshot appropriately.
 * off and len are in bytes.
 */
int
fs_read(datp, buf, len)
	struct fs_dat *datp;
	void *buf;
	unsigned len;
{
	struct vx_snapread args;
	int err = 0;

	if (datp->fsd_issnapshot) {
		args.sr_buf = (char *) buf;
		args.sr_off = datp->fsd_off;
		args.sr_len = len;
		err = ioctl(datp->fsd_mntfd, VX_SNAPREAD, &args);
		if (!err) {
			datp->fsd_off += len;
			return len;
		}
		return -1;
	}
	return read(datp->fsd_fd, buf, len);
}

/*
 * write the specified blocks to a file system; snapshots are read-only.
 * off and len are in bytes.
 */
int
fs_write(datp, buf, len)
	struct fs_dat *datp;
	void *buf;
	unsigned len;
{
	struct vx_snapread args;
	int err = 0;

	if (datp->fsd_issnapshot) {
		errno = EROFS;
		return -1;
	}
	return write(datp->fsd_fd, buf, len);
}


/*
 * Given the pathname of a block special file, try to find where
 * its mounted based on the mount table.
 *
 * Space for the returned mountpoint is malloc'ed.
 */

char *
fs_findmntpt(bspec)
	char *bspec;
{
	FILE *fp;
	struct mnttab	mntent;
	struct mnttab	mntpref;

	fp = fopen(MNTTAB, "r");
	if (!fp) {
		return NULL;
	}

	bzero(&mntpref, sizeof(struct mnttab));
	mntpref.mnt_special = bspec;
	if (getmntany(fp, &mntent, &mntpref)) {
		fclose(fp);
		return NULL;
	}

	fclose(fp);
	return strdup(mntent.mnt_mountp);
}


/*
 * Given the mountpoint, try to find the block special file 
 * in the mount table.
 *
 * Space for the returned pathname is malloc'ed.
 */

char *
fs_findbspec(mntpt)
	char *mntpt;
{
	FILE *fp;
	struct mnttab	mntent;
	struct mnttab	mntpref;

	fp = fopen(MNTTAB, "r");
	if (!fp) {
		return NULL;
	}

	bzero(&mntpref, sizeof(struct mnttab));
	mntpref.mnt_mountp = mntpt;
	if (getmntany(fp, &mntent, &mntpref)) {
		fclose(fp);
		return NULL;
	}

	fclose(fp);
	return strdup(mntent.mnt_special);
}


/*
 * allocate an fs_dat structure and initialize it for use by our callers
 */

struct fs_dat *
fs_datalloc()
{
	struct fs_dat	*datp;

	datp = (struct fs_dat *) malloc(sizeof(struct fs_dat));
	if (datp) {
		bzero((char *) datp, sizeof(struct fs_dat));
		datp->fsd_fd = -1;
		datp->fsd_mntfd = -1;
	}
	return datp;
}

/*
 * close an open file system; and clear the snapshot mountpoint.
 */
int
fs_close(datp)
	struct fs_dat	*datp;
{
	close(datp->fsd_fd);
	datp->fsd_fd = -1;
	free(datp->fsd_spec);
	datp->fsd_spec = NULL;
	if (datp->fsd_issnapshot) {
		datp->fsd_issnapshot = 0;
		close(datp->fsd_mntfd);
		datp->fsd_mntfd = -1;
	}
	if (datp->fsd_mntpt) {
		free(datp->fsd_mntpt);
		datp->fsd_mntpt = NULL;
	}

	return 0;
}


/*
 * free an allocated fs_dat structure; it's assumed to have
 * already been closed.
 */
void
fs_datfree(datp)
	struct fs_dat *datp;
{
	free(datp);
}
