/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma	ident	"@(#)libDtI:diagnose.c	1.19"
#endif

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <archives.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <limits.h>
#include <sys/fs/s5filsys.h>
#undef	getfs
#include <sys/fs/ufs_fs.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>

#include <Memutil/memutil.h>
#include "DtI.h"

#define	MOUNT_TABLE	"/etc/mnttab"

extern	char	BDEVICE[];
extern	char	CDEVICE[];
extern	char	DISKETTE[];

char	*_dtam_mntpt = NULL;
char	*_dtam_mntbuf= NULL;
char	*_dtam_fstyp = NULL;
long	_dtam_flags  = 0;

#ifdef	DEBUG
static	void	prtdiag(int n)
{
	char	*ptr;

	switch (n) {
	case DTAM_S5_FILES:		ptr = "S5_FILES";	break;
	case DTAM_UFS_FILES:		ptr = "UFS_FILES";	break;
	case DTAM_FS_TYPE:		ptr = "FS_TYPE";	break;
	case (DTAM_CPIO|DTAM_PACKAGE):	ptr = "PACKAGE";	break;
	case (DTAM_CPIO|DTAM_INSTALL):	ptr = "INSTALL";	break;
	case DTAM_BACKUP:		ptr = "BACKUP";		break;
	case DTAM_CPIO:			ptr = "CPIO";		break;
	case DTAM_CPIO_BINARY:		ptr = "CPIO_BINARY";	break;
	case DTAM_CPIO_ODH_C:		ptr = "CPIO_ODC_H";	break;
	case DTAM_TAR:			ptr = "TAR";		break;
	case DTAM_CUSTOM:		ptr = "CUSTOM";		break;
	case DTAM_DOS_DISK:		ptr = "DOS_DISK";	break;
	case DTAM_UNFORMATTED:		ptr = "UNFORMATTED";	break;
	case DTAM_NO_DISK:		ptr = "NO_DISK";	break;
	case DTAM_UNREADABLE:		ptr = "UNREADABLE";	break;
	case DTAM_BAD_ARGUMENT:		ptr = "BAD_ARGUMENT";	break;
	case DTAM_BAD_DEVICE:		ptr = "BAD_DEVICE";	break;
	case DTAM_DEV_BUSY:		ptr = "DEV_BUSY";	break;
	case DTAM_UNKNOWN:		ptr = "UNKNOWN";	break;
	default:			ptr = "?";		break;
	}
	fprintf(stderr, "CheckMedia ->  %s\n", ptr);
}
#endif

int	DtamCheckMedia(char *alias)
{
struct	stat	st_buf;
	char	*device;
	char	*ptr;
	int	mntfd;
	int	n;
 
	_dtam_flags = 0;
	if (alias == NULL)
		return 0;
	else if ((ptr = DtamGetDev(alias,DTAM_FIRST)) == NULL) {
		n = diagnose(alias); 
#ifdef	DEBUG
		prtdiag(n);
#endif
		return n;
	}
	else if ((device = DtamDevAttr(ptr,BDEVICE)) == NULL) {
		if ((device = DtamDevAttr(ptr,CDEVICE)) == NULL)
			return DTAM_BAD_DEVICE;
		else {
			n = diagnose(device);
#ifdef	DEBUG
			prtdiag(n);
#endif
			FREE(ptr);
			FREE(device);
			return n;
		}
	}
	else
		FREE(ptr);
/*
 *	the following checks to see if /dev/dsk/fn[t] is mounted;
 *	if so, that is the target of the diagnostic, and only if checks
 *	on this device fail will the other option be tried.  If neither
 *	is (claimed to be) mounted, then the device checked first is the
 *	one originally specified, and 't' is deleted or added for a second
 *	diagnostic pass if the first one fails.
 */
	n = strlen(device)-1;
	if (strstr(alias,DISKETTE) && device[n] == 't') {
	/*
	 *	(temporarily) remove the final t, to match both possibilities
	 */
		device[n] = '\0';
		_dtam_flags = DTAM_TFLOPPY;
	}
	else
		_dtam_flags = 0;
	if (_dtam_mntpt) {
		FREE(_dtam_mntpt);
		_dtam_mntpt = NULL;
	}
	if (stat(MOUNT_TABLE, &st_buf) != -1
	&& (mntfd = open(MOUNT_TABLE,O_RDONLY)) != -1) {
		_dtam_mntbuf = mmap((caddr_t)0, st_buf.st_size, PROT_READ,
				MAP_SHARED, mntfd, 0);
	}
	if (_dtam_mntbuf != NULL && _dtam_mntbuf != (char *)-1) {
		if ((ptr=strstr(_dtam_mntbuf, device)) != NULL) {
				char	*ptr2;
				int	i = 0;
			for (ptr2 = strchr(ptr,'\t')+1; *ptr2 != '\t'; ptr2++)
				i++;
			_dtam_mntpt = (char *)malloc(i+1);
			_dtam_mntpt[i] = 0;
			strncpy(_dtam_mntpt, ptr2-i, i);
			if (ptr[n] == 't' && _dtam_flags)
			/*
			 *	retore the final t, as that is what is mounted
			 */
				device[n] = 't';
			else
				_dtam_flags = 0;
			_dtam_flags |= DTAM_MOUNTED;
		}
		else if (_dtam_flags & DTAM_TFLOPPY)
			device[n] = 't';
		munmap(_dtam_mntbuf, st_buf.st_size);
		close (mntfd);
		_dtam_mntbuf = NULL;
	}
/*
 *	Now run through actual checks on the disk 
 */
	n = diagnose(device);
	if (n & DTAM_FS_TYPE == 0 && _dtam_flags & DTAM_MOUNTED) {
		_dtam_flags |= DTAM_MIS_MOUNT;
		DtamUnMount(_dtam_mntpt);
	}
	if (n == DTAM_UNKNOWN) {
	/*
	 *	make one more attempt, using fstyp(1) to check file systems
	 */
		FILE	*pipefp;
		char	devbuf[BUFSIZ];

		sprintf(devbuf, "/sbin/fstyp %s 2>&1", device);
		if (pipefp=popen(devbuf,"r")) {
			while (fgets(devbuf, BUFSIZ, pipefp))
				;
			if (pclose(pipefp) == 0) {
				if (_dtam_fstyp)
					FREE(_dtam_fstyp);

				/* Only use the 1st "token" for now.  In the
				   case of a CD-ROM, for instance, additional
				   info is provided.  This is currently
				   unused so must be discarded.  (While we're
				   at it, clobber any trailing newline)
				*/
				(void)strtok(devbuf, " \t\n");

				_dtam_fstyp = STRDUP(devbuf);
				n = DTAM_FS_TYPE;
			}
		}
	}
	if (n == DTAM_UNKNOWN && (_dtam_flags & DTAM_TFLOPPY) != 0) {
	/*
	 *	try again, with the non-t version of the device
	 */
		_dtam_flags ^= DTAM_TFLOPPY;
		device[strlen(device)-1] = '\0';
		if ((n = diagnose(device)) == DTAM_UNKNOWN)
			_dtam_flags |= DTAM_TFLOPPY;
	}
#ifdef	DEBUG
	prtdiag(n);
#endif
	FREE(device);
	return n;
}

int StoreIndexNumber(char *ascNum)
{
static int flpIndexNumber;
	if (ascNum)
	{
		flpIndexNumber = atoi (ascNum);
		return (0);
	}
	else 
	{
		return (flpIndexNumber);
	}
}

int	diagnose( char *dev)
{
extern	int	errno;
	char	devbuf[2*BBSIZE];
	char	cmdbuf[PATH_MAX+33];
	int	devfd;
	int	n;
	long	*l;
	struct	filsys		*s5_files;
	struct	fs		*ufs_files;
	struct	Exp_cpio_hdr	*cpio_hdr;
	struct	c_hdr		*char_hdr;
	struct	hdr_cpio	*bin_cpio;
	struct	tar_hdr		*tarbuf;


	if (access(dev,R_OK) == -1)
		return(DTAM_BAD_DEVICE);
	if ((devfd = open(dev,
			_dtam_flags&DTAM_READ_ONLY? O_RDONLY: O_RDWR)) == -1) {
tsterr:		switch (errno) {
			case ENODEV:
			case EACCES:
			case EROFS:	if (_dtam_flags & DTAM_READ_ONLY)
						return DTAM_UNREADABLE;
					_dtam_flags |= DTAM_READ_ONLY;
					if ((devfd=open(dev,O_RDONLY)) == -1)
						goto tsterr;
					break;
			case EIO:	return DTAM_NO_DISK;
			case ENXIO:	return DTAM_UNFORMATTED;
			case EBUSY:	return DTAM_DEV_BUSY;
			default:	return DTAM_UNREADABLE;
		}
	}
/*
 *	map out 2 ufs-sized blocks (8192 each)
 */
	n = read(devfd,devbuf,2*BBSIZE);
	close (devfd);
	if (n == -1) {
		return(DTAM_UNREADABLE);
	}
	else {
/*
 *		try files systems first (s5 then ufs)
 */
		s5_files = (struct filsys *)(devbuf + 512);
		if (s5_files->s_magic == FsMAGIC) {
			return(DTAM_S5_FILES);
		}
		ufs_files = (struct fs *)(devbuf+BBSIZE);
		if (ufs_files->fs_magic == FS_MAGIC) {
			return(DTAM_UFS_FILES);
		}
	}
/*
 *	check for cpio formats (cf. archive.h)
 *
 *	all character formats for cpio have initial string "07070" -- they
 *	differ in the 6th byte, but for input that can safely be left to cpio
 */
	if (strncmp(devbuf,"# PaCkAgE DaTaStReAm",20) == 0) {
		return(DTAM_PACKAGE|DTAM_CPIO);
	}
	if (strncmp(devbuf,"07070",5) == 0) {
		if (devbuf[5] == '1') {
			cpio_hdr = (struct Exp_cpio_hdr *)devbuf;
			if (strncmp(cpio_hdr->E_name,"/tmp/flp_index",14)==0) {
				StoreIndexNumber(cpio_hdr->E_name + 15);
				return(DTAM_BACKUP);
			}
		}
		else if (devbuf[5] == '7') {
			char_hdr = (struct c_hdr *)devbuf;
			if (strcmp(char_hdr->c_name,".") == 0) {
				char_hdr=(struct c_hdr *)&(char_hdr->c_name[2]);
				if (strcmp(char_hdr->c_name,"Size") == 0) {
					return(DTAM_INSTALL|DTAM_CPIO);
				}
			}
		}
		return(DTAM_CPIO);
	}
	bin_cpio = (struct hdr_cpio *)devbuf;
	if (bin_cpio->h_magic == CMN_BIN) {
		return(DTAM_CPIO_BINARY);
	}
	/*
	 * Use dosdir to check for a DOS diskette
	 */
	(void)sprintf(cmdbuf, "/usr/bin/dosdir %s >/dev/null 2>&1", dev);
	if (system(cmdbuf) == 0) {
		return(DTAM_DOS_DISK);
	}

/*
 *	the tar structure provides for a "magic number" but this is not
 *	unique, but rather dependent on the implementation of tar.  The
 *	following is an ad hoc and fallible attmept to recognize a tar file.
 */
	tarbuf = (struct tar_hdr *)devbuf;
	if ((n=strlen(tarbuf->t_name)) > 0 && n < TNAMLEN
	&&  strlen(tarbuf->t_mode) == TMODLEN-1
	&&  strlen(tarbuf->t_uid)  == TUIDLEN-1
	&&  strlen(tarbuf->t_gid)  == TGIDLEN-1) {
		/*
		 *	should check for syntactical validity of these
		*/
		if (strncmp(tarbuf->t_name, "/etc/perms/", 11) == 0
		|| strstr(tarbuf->t_name, "/prd=")) {
			return(DTAM_CUSTOM);
		}
		else {
			return(DTAM_TAR);
		}
	}
/*
 *	if all tests fail, ...
 */
	return(DTAM_UNKNOWN);

	/*
	 *	for file system types, there should be a further check
	 *	when mounted to see if we have the right diskette.
	 */
}
