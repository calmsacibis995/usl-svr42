/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_STAT_H	/* wrapper symbol for kernel use */
#define _FS_STAT_H	/* subject to change without notice */
#define _SYS_STAT_H

#ident	"@(#)uts-x86:fs/stat.h	1.12"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#ifndef _SVC_TIME_H
#include <svc/time.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */
#include <sys/time.h>	/* REQUIRED */

#else

#include <sys/time.h>	/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

#define _ST_FSTYPSZ 16		/* array size for file system type name */

/*
 * stat structure, used by stat(2) and fstat(2)
 *
 * With the Expansion of the Fundamental Types (EFT), two versions of
 * the stat structures are maintained.  In the kernel, "struct stat"
 * is still used to represent the old (SVID Issue 2) stat structure.
 * A new structure, "struct xstat", is used to represent the EFT stat
 * structure. At the user-level, "struct stat" is defined as the
 * old stat structure if _STYPES is defined.  Otherwise, the EFT stat
 * structure is assumed.
 */

#if defined(_KERNEL)

	/* SVID Issue 2 stat structure */
struct	stat {
	o_dev_t	st_dev;		/* id of device containing */
				/* a directory entry for this file */
	o_ino_t	st_ino;		/* inode number */
	o_mode_t st_mode;	/* file mode */
	o_nlink_t st_nlink;	/* number of links */
	o_uid_t	st_uid;		/* user id of the file's owner */
	o_gid_t	st_gid;		/* group id of the file's group */
	o_dev_t	st_rdev;	/* id of device, only defined for */
				/* character and block special files */
	off_t	st_size;	/* file size in bytes */
	time_t	st_atime;	/* time of last access */
	time_t	st_mtime;	/* time of last data modification */
	time_t	st_ctime;	/* time of last file status change */
};

	/* Expanded stat structure */ 
struct	xstat {
	dev_t	st_dev;		/* id of device containing */
				/* a directory entry for this file */
	long	st_pad1[3];	/* reserve for dev expansion, sysid definition */
	ino_t	st_ino;		/* inode number */
	mode_t	st_mode;	/* file mode */
	nlink_t	st_nlink;	/* number of links */
	uid_t	st_uid;		/* user id of the file's owner */
	gid_t	st_gid;		/* group id of the file's group */
	dev_t	st_rdev;	/* id of device, only defined for */
				/* character and block special files */
	long	st_pad2[2];	/* dev and off_t expansion */
	off_t	st_size;	/* file size in bytes */
	long	st_pad3;	/* reserve pad for future off_t expansion */
	timestruc_t st_atime;	/* time of last access */
	timestruc_t st_mtime;	/* time of last data modification */
	timestruc_t st_ctime;	/* time of last file status change */
	long	st_blksize;	/* preferred I/O block size */
	long	st_blocks;	/* number st_blksize blocks allocated */
	char	st_fstype[_ST_FSTYPSZ];
				/* type of file system on which file resides */
	int	st_aclcnt;	/* number of ACL entries */
	lid_t	st_level;	/* MAC level */
	ulong_t	st_flags;	/* general purpose flag */
	lid_t	st_cmwlevel;	/* MAC level for future use */
	long	st_pad4[4];	/* expansion area */
};

#else /* !defined(_KERNEL) */
#if !defined(_STYPES)

	/* User level 4.0 stat structure; maps to kernel struct xstat */
struct	stat {
	dev_t	st_dev;		/* id of device containing */
				/* a directory entry for this file */
	long	st_pad1[3];	/* reserved for network id */
	ino_t	st_ino;		/* inode number */
	mode_t	st_mode;	/* file mode */
	nlink_t	st_nlink;	/* number of links */
	uid_t	st_uid;		/* user id of the file's owner */
	gid_t	st_gid;		/* group id of the file's group */
	dev_t	st_rdev;	/* id of device, only defined for */
				/* character and block special files */
	long	st_pad2[2];	/* dev and off_t expansion */
	off_t	st_size;	/* file size in bytes */
	long	st_pad3;	/* future off_t expansion */
	union {
		time_t		st__sec; /* compatible: time in seconds */
		timestruc_t	st__tim; /* secs+nanosecs; first is time_t */
	}	st_atim,	/* time of last access */
		st_mtim,	/* time of last data modification */
		st_ctim;	/* time of last file status change */
	long	st_blksize;	/* preferred I/O block size */
	long	st_blocks;	/* number st_blksize blocks allocated */
	char	st_fstype[_ST_FSTYPSZ];
				/* type of file system on which file resides */
	int	st_aclcnt;	/* number of ACL entries */
	level_t	st_level;	/* MAC level */
	ulong_t	st_flags;	/* general purpose flag */
	lid_t	st_cmwlevel;	/* MAC level for future use */
	long	st_pad4[4];	/* expansion area */
};
#define st_atime	st_atim.st__sec
#define st_mtime	st_mtim.st__sec
#define st_ctime	st_ctim.st__sec

#else /* defined(_STYPES) */

	/* SVID Issue 2 stat structure */
struct	stat {
	o_dev_t	st_dev;		/* id of device containing */
				/* a directory entry for this file */
	o_ino_t	st_ino;		/* inode number */
	o_mode_t st_mode;	/* file mode */
	o_nlink_t st_nlink;	/* number of links */
	o_uid_t	st_uid;		/* user id of the file's owner */
	o_gid_t	st_gid;		/* group id of the file's group */
	o_dev_t	st_rdev;	/* id of device, only defined for */
				/* character and block special files */
	off_t	st_size;	/* file size in bytes */
	time_t	st_atime;	/* time of last access */
	time_t	st_mtime;	/* time of last data modification */
	time_t	st_ctime;	/* time of last file status change */
};
#endif	/* end !defined(_STYPES) */
#endif	/* end defined(_KERNEL) */


/* MODE MASKS */

/* de facto standard definitions */

#if !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE) 
#define	S_IFMT		0xF000	/* type of file */
#endif /*!defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE)*/ 
#if !defined(_POSIX_SOURCE) 
#define S_IAMB		0x1FF	/* access mode bits */
#endif /* !defined(_POSIX_SOURCE) */
#if !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE) 
#define	S_IFIFO		0x1000	/* fifo */
#define	S_IFCHR		0x2000	/* character special */
#define	S_IFDIR		0x4000	/* directory */
#endif /*!defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE)*/ 
#if !defined(_POSIX_SOURCE) 
#define	S_IFNAM		0x5000  /* XENIX special named file */
#define		S_INSEM 0x1	/* XENIX semaphore subtype of IFNAM */
#define		S_INSHD 0x2	/* XENIX shared data subtype of IFNAM */
#endif /* !defined(_POSIX_SOURCE) */

#if !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE) 
#define	S_IFBLK		0x6000	/* block special */
#define	S_IFREG		0x8000	/* regular */
#endif /*!defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE)*/ 
#if !defined(_POSIX_SOURCE) 
#define	S_IFLNK		0xA000	/* symbolic link */
#define	S_IFSOCK	0xC000	/* socket */
#endif /* !defined(_POSIX_SOURCE) */

#define	S_ISUID		0x800	/* set user id on execution */
#define	S_ISGID		0x400	/* set group id on execution */

#if !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE) 
#define	S_ISVTX		0x200	/* save swapped text even after use */
#endif /* !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE) */

#if !defined(_POSIX_SOURCE) 
#define	S_IREAD		00400	/* read permission, owner */
#define	S_IWRITE	00200	/* write permission, owner */
#define	S_IEXEC		00100	/* execute/search permission, owner */
#define	S_ENFMT		S_ISGID	/* record locking enforcement flag */
#endif /* !defined(_POSIX_SOURCE) */


/* the following macros are for POSIX conformance */

#define	S_IRWXU	00700		/* read, write, execute: owner */
#define	S_IRUSR	00400		/* read permission: owner */
#define	S_IWUSR	00200		/* write permission: owner */
#define	S_IXUSR	00100		/* execute permission: owner */
#define	S_IRWXG	00070		/* read, write, execute: group */
#define	S_IRGRP	00040		/* read permission: group */
#define	S_IWGRP	00020		/* write permission: group */
#define	S_IXGRP	00010		/* execute permission: group */
#define	S_IRWXO	00007		/* read, write, execute: other */
#define	S_IROTH	00004		/* read permission: other */
#define	S_IWOTH	00002		/* write permission: other */
#define	S_IXOTH	00001		/* execute permission: other */


#define S_ISFIFO(mode)	((mode&0xF000) == 0x1000)
#define S_ISCHR(mode)	((mode&0xF000) == 0x2000)
#define S_ISDIR(mode)	((mode&0xF000) == 0x4000)
#define S_ISBLK(mode)	((mode&0xF000) == 0x6000)
#define S_ISREG(mode)	((mode&0xF000) == 0x8000) 

/* FLAGS MASKS */

#if !defined(_POSIX_SOURCE) 
#define	S_ISMLD		0x1	/* denotes a multi-level directory (MLD) */
#endif /* !defined(_POSIX_SOURCE) */

/* general purpose stat flags */
#define	_S_ISMLD	0x1	/* denotes a multi-level directory (MLD) */
#define _S_ISMOUNTED	0x2	/* denotes a mounted b|c device node */

/* a version number is included in the SVR4 stat and mknod interfaces. */

#define _R3_MKNOD_VER 1		/* SVR3.0 mknod */
#define _MKNOD_VER 2		/* current version of mknod */
#define _R3_STAT_VER 1		/* SVR3.0 stat */
#define _STAT_VER 2		/* current version of stat */

#if !defined(_KERNEL)
#if defined(__STDC__)

/*			Function prototypes
 *			___________________
 *
 * When _STYPES is defined the pre-SVR4 routines for fstat()/stat()/mknod()
 * are used. When _STYPES is NOT defined, calls to the stat family and
 * mknod are mapped to the system call prefixed by "_".
 * 
 * NOTE: user code should NOT program to the "_" routines.
 *
 */

#if !defined(_STYPES)
static int fstat(int, struct stat *);
static int stat(const char *, struct stat *);
#if !defined(_POSIX_SOURCE) 
static int lstat(const char *, struct stat *);
static int mknod(const char *, mode_t, dev_t);
#endif /* !defined(_POSIX_SOURCE) */
#else
int fstat(int, struct stat *);
int stat(const char *, struct stat *);
#if !defined(_POSIX_SOURCE) 
int lstat(const char *, struct stat *);
int mknod(const char *, mode_t, dev_t);
#endif /* !defined(_POSIX_SOURCE) */
#endif

int _fxstat(const int, int, struct stat *);
int _xstat(const int, const char *, struct stat *);
#if !defined(_POSIX_SOURCE) 
int _lxstat(const int, const char *, struct stat *);
int _xmknod(const int, const char *, mode_t, dev_t);
#endif /* !defined(_POSIX_SOURCE) */
extern int chmod(const char *, mode_t);
extern int mkdir(const char *, mode_t);
extern int mkfifo(const char *, mode_t);
extern mode_t umask(mode_t);
extern int fchmod(int, mode_t);

#else	/* !__STDC__ */

#if !defined(_STYPES)
static int fstat(), stat();
#if !defined(_POSIX_SOURCE) 
static int mknod(), lstat();
#endif /* !defined(_POSIX_SOURCE) */
#else
int fstat(), stat();
#if !defined(_POSIX_SOURCE) 
int mknod(), lstat();
#endif /* !defined(_POSIX_SOURCE) */
#endif

int _fxstat(), _xstat();
#if !defined(_POSIX_SOURCE) 
int _xmknod(), _lxstat();
#endif /* !defined(_POSIX_SOURCE) */
extern int chmod();
extern int mkdir();
extern int mkfifo();
extern mode_t umask();
extern int fchmod();

#endif /* defined(__STDC__) */
#endif /* !defined(_KERNEL) */

/*
 * The static function implementation for
 * the stat(2) switch call provided both 
 * source and binary compatibility in SVR4.
 * While this implementation has some pitfalls
 * it was considered the best compromise given
 * the compatibility issues that exist with this 
 * POSIX/XOPEN interface. The static function
 * implementation adds incremental a.out space
 * and it generates lint warnings for unused
 * static functions. The lint problem can be worked
 * around by including a stub call for the unused 
 * static functions when lint is run (#ifdef LINT).
 *
 */

#if !defined(_STYPES) && !defined(_KERNEL)
static int
stat(_path, _buf)
const char *_path;
struct stat *_buf;
{
int ret;
	ret = _xstat(_STAT_VER, _path, _buf);
	return ret; 
}

#if !defined(_POSIX_SOURCE) 
static int
lstat(path, buf)
const char *path;
struct stat *buf;
{
int ret;
	ret = _lxstat(_STAT_VER, path, buf);
	return ret;
}
#endif /* !defined(_POSIX_SOURCE) */

static int
fstat(_fd, _buf)
int _fd;
struct stat *_buf;
{
int ret;
	ret = _fxstat(_STAT_VER, _fd, _buf);
	return ret;
}

#if !defined(_POSIX_SOURCE) 
static int
mknod(path, mode, dev)
const char *path;
mode_t mode;
dev_t dev;
{
int ret;
	ret = _xmknod(_MKNOD_VER, path, mode, dev);
	return ret;
}
#endif /* !defined(_POSIX_SOURCE) */

#endif



#endif	/* _FS_STAT_H */
