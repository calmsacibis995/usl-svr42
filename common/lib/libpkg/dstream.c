/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
#ident	"@(#)libpkg:common/lib/libpkg/dstream.c	1.9.17.14"
#ident "$Header: $"

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pkgdev.h>
#include <stdlib.h>
#include <sys/types.h>
#include <wait.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/mkdev.h>
#include <sys/statvfs.h>
#include <fcntl.h>
#include <libgenIO.h>

extern int	errno;
extern FILE	*epopen();
extern int	pkgnmchk(),
		epclose(),
		esystem(),
		getvol(),
		ioctl(),
		write(),
		read(),
		close(),
		ckvolseq(),
		atoi();
extern void	ecleanup(),
		cleanup(),
		rpterr(),
		progerr(), 
		logerr(),
		free();
extern char	*devattr();

#define CMDSIZ	2048
#define LSIZE	128
#define	PBLK	512	/* 512 byte "physical" block */
#define DDPROC		"/usr/bin/dd"
#define CPIOPROC	"/usr/bin/cpio"

#define ERR_UNPACK	"attempt to process package from <%s> failed"
#define ERR_DSTREAMSEQ	"datastream sequence corruption"
#define ERR_TRANSFER    "unable to complete package transfer"
#define MSG_MEM		"no memory"
#define MSG_CMDFAIL	"- process <%s> failed, exit code %d"
#define MSG_TOC		"- bad format in datastream table-of-contents"
#define MSG_EMPTY	"- datastream table-of-contents appears to be empty"
#define MSG_POPEN	"- popen of <%s> failed, errno=%d"
#define MSG_OPEN	"- open of <%s> failed, errno=%d"
#define MSG_PCLOSE	"- pclose of <%s> failed, errno=%d"
#define MSG_PKGNAME	"- invalid package name in datastream table-of-contents"
#define MSG_NOPKG	"- package <%s> not in datastream"
#define MSG_STATFS	"- unable to stat filesystem, errno=%d"
#define MSG_NOSPACE	"- not enough tmp space, %d free blocks required"

struct dstoc {
	int	cnt;
	char	pkg[16];
	int	nparts;
	long	maxsiz;
	char    volnos[128];
	struct dstoc *next;
} *ds_head, *ds_toc;

#define	ds_nparts	ds_toc->nparts
#define	ds_maxsiz	ds_toc->maxsiz
	
int	ds_totread;	/* total number of parts read */
int	ds_fd = -1;
int	pkgpipe[2];	/* pipe for dd process */
pid_t	dd_pid = -1;	/* pid of dd process writing to pipe */ 
int	ds_curpartcnt = -1;
int	ds_next();
int	ds_ginit();
int	ds_close();


static FILE	*ds_pp;
static int	ds_realfd = -1;	/* file descriptor for real device */
static int	ds_read;	/* number of parts read for current package */
static int	ds_volno;	/* volume number of current volume */
static int	ds_volcnt;	/* total number of volumes */
static char	ds_volnos[128];	/* parts/volume info */
static char	*ds_device;
static int	ds_volpart;	/* number of parts read in current volume, including skipped parts */
static int	ds_bufsize;
static int	ds_skippart;	/* number of parts skipped in current volume */
static int	ds_getnextvol(), ds_skip();

void
ds_order(list)
char *list[];
{
	struct dstoc *toc_pt;
	register int j, n;
	char	*pt;

	toc_pt = ds_head;
	n = 0;
	while(toc_pt) {
		for(j=n; list[j]; j++) {
			if(!strcmp(list[j], toc_pt->pkg)) {
				/* just swap places in the array */
				pt = list[n];
				list[n++] = list[j];
				list[j] = pt;
			}
		}
		toc_pt = toc_pt->next;
	}
}

static char *pds_header;
static char *ds_header;
static int ds_headsize;

static char *
ds_gets(buf, size)
char *buf;
int size;
{
	char *nextp, *old_ds_header;

	nextp = strchr(pds_header, '\n');
	if(nextp == NULL) {
		old_ds_header = ds_header;
		if((ds_header = (char *)realloc(ds_header, ds_headsize + 512)) == NULL) 
			return 0;
		if(read(ds_fd, ds_header + ds_headsize, 512) < 512)
			return 0;
		pds_header = ds_header + (pds_header - old_ds_header);
		ds_headsize += 512;
		nextp = strchr(pds_header, '\n');
		if(nextp == NULL)
			return 0;
	}
	*nextp = '\0';
	if((int)strlen(pds_header) > size)
		return 0;
	(void)strncpy(buf, pds_header, strlen(pds_header));
	buf[strlen(pds_header)] = '\0';
	pds_header = nextp + 1;
	return buf;
}

/*
 * function to determine if media is datastream or mounted 
 * floppy
 */
int
ds_readbuf(device)
char *device;
{
	char buf[512];

	if(ds_fd >= 0)
		(void)close(ds_fd);
	if((ds_fd = open(device, 0)) >= 0 
	  	&& read(ds_fd, buf, 512) == 512
	  	&& strncmp(buf, "# PaCkAgE DaTaStReAm", 20) == 0) {
		if((ds_header = (char *)calloc(513, 1)) == NULL) {
			progerr(ERR_UNPACK, device);
			logerr(MSG_MEM);
			(void)ds_close(0);
			return 0;
		}
		memcpy(ds_header, buf, 512);
		ds_header[512] = '\0';
		ds_headsize = 512;

		if(ds_ginit(device) < 0) {
			progerr(ERR_UNPACK, device);
			logerr(MSG_OPEN, device, errno);
			(void)ds_close(0);
			return 0;
		}
		return 1;
	} else if(ds_fd >= 0) {
		(void)close(ds_fd);
		ds_fd = -1;
	}
	return 0;
}

/*
 * Determine how many additional volumes are needed for current package.
 *
 * Note: a 0 will occur as first volume number when the package begins
 * on the next volume.
 */
static int
ds_volsum(toc)
struct dstoc *toc;
{
	int curpartcnt, volcnt;
	char volnos[128], tmpvol[128];
	if(toc->volnos[0]) {
		int index, sum;
		sscanf(toc->volnos, "%d %[ 0-9]", &curpartcnt, volnos);
		volcnt = 0;
		sum = curpartcnt;
		while(sum < toc->nparts && sscanf(volnos, "%d %[ 0-9]", &index, tmpvol) >= 1) {
			(void)strcpy(volnos, tmpvol);
			volcnt++;
			sum += index;
		}
		/* side effect - set number of parts read on current volume */
		ds_volpart = index;
		return volcnt;
	}
	ds_volpart += toc->nparts;
	return 0;
}

/* initialize ds_curpartcnt and ds_volnos */
static void
ds_pkginit()
{
	if(ds_toc->volnos[0])
		sscanf(ds_toc->volnos, "%d %[ 0-9]", &ds_curpartcnt, ds_volnos);
	else
		ds_curpartcnt = -1;
}

/* functions to pass current package info to exec'ed program */
void
ds_putinfo(buf)
char *buf;
{
	(void)sprintf(buf, "%d %d %d %d %d %d %d %d %d %d %s", ds_fd, ds_realfd, ds_volcnt, ds_volno, ds_totread, ds_volpart, ds_skippart, ds_bufsize, ds_toc->nparts, ds_toc->maxsiz, ds_toc->volnos);
}

int
ds_getinfo(string)
char *string;
{
	ds_toc = (struct dstoc *)calloc(1, sizeof(struct dstoc));
	(void)sscanf(string, "%d %d %d %d %d %d %d %d %d %d %[ 0-9]", &ds_fd, &ds_realfd, &ds_volcnt, &ds_volno, &ds_totread, &ds_volpart, &ds_skippart, &ds_bufsize, &ds_toc->nparts, &ds_toc->maxsiz, ds_toc->volnos);
	ds_pkginit();
	return ds_toc->nparts;
}

int
ds_init(device, pkg, norewind)
char	*device;
char	**pkg;
char	*norewind;
{
	struct dstoc *tail, *toc_pt; 
	char	*ret;
	char	cmd[CMDSIZ];
	char	line[LSIZE+1];
	int	i, n, count = 0;

	if(!ds_header) {
		if(ds_fd >= 0)
			(void)ds_close(0);

		/* always start with rewind device */
		if((ds_fd = open(device, 0)) < 0) {
			progerr(ERR_UNPACK, device);
			logerr(MSG_OPEN, device, errno);
			return -1;
		}

		if((ds_header = (char *)calloc(512, 1)) == NULL) {
			progerr(ERR_UNPACK, device);
			logerr(MSG_MEM);
			return -1;
		}

		/*
		 * At this point, if pkgdev.bdevice is non-null (for case when the
		 * installation media is diskette), then pkgadd has already initiated
		 * a 'dd' process reading the media in order to determine if it is
		 * datastream media.  This would have been done via a call to the 
		 * function ds_readbuf().  If this is the case, then 'dd_pid' has been
		 * reset from its original value of '-1' to the process id of the 'dd'
		 * process initiated earlier.  To prevent a second 'dd' process from
		 * being started here, do not call ds_ginit() unless 'dd_pid' is still 
		 * '-1'.  Since tape media is always in datastream format, pkgadd never
		 * calls ds_readbuf() for that case.  For tape, dd_pid will still be
		 * '-1' here, so we'll call ds_ginit() to initiate the 'dd' process.
		 */

		if(dd_pid == -1) { 
			if(ds_ginit(device) < 0) {
				(void)ds_close(0);
				progerr(ERR_UNPACK, device);
				logerr(MSG_OPEN, device, errno);
				return -1;
			}
		}
		if(read(ds_fd, ds_header, 512) != 512) {
			rpterr();
			progerr(ERR_UNPACK, device);
			logerr(MSG_TOC);
			(void)ds_close(0);
			return -1;
		}

		while(strncmp(ds_header, "# PaCkAgE DaTaStReAm", 20) != 0) {
			if(!norewind || count++ > 10) {
				progerr(ERR_UNPACK, device);
				logerr(MSG_TOC);
				(void)ds_close(0);
				return -1;
			}
			if(count > 1)
				while(read(ds_fd, ds_header, 512) > 0)
					;
			(void)ds_close(0);
			if((ds_fd = open(norewind, 0)) < 0) {
				progerr(ERR_UNPACK, device);
				logerr(MSG_OPEN, device, errno);
				return -1;
			}
			if(ds_ginit(device) < 0) {
				(void)ds_close(0);
				progerr(ERR_UNPACK, device);
				logerr(MSG_OPEN, device, errno);
				return -1;
			}
			if(read(ds_fd, ds_header, 512) != 512) {
				rpterr();
				progerr(ERR_UNPACK, device);
				logerr(MSG_TOC);
				(void)ds_close(0);
				return -1;
			}
		}
		/*
		 * remember rewind device for ds_close to rewind at
		 * close
		 */
		if(count >= 1) 
			ds_device = device;
		ds_headsize = 512;

	}
	pds_header = ds_header;
	/* read datastream table of contents */
	ds_head = tail = (struct dstoc *)0;
	ds_volcnt = 1;
	
	while(ret=ds_gets(line, LSIZE)) {
		if(strcmp(line, "# end of header") == 0)
			break;
		if(!line[0] || line[0] == '#')
			continue;
		toc_pt = (struct dstoc *) calloc(1, sizeof(struct dstoc));
		if(!toc_pt) {
			progerr(ERR_UNPACK, device);
			logerr(MSG_MEM);
			ecleanup();
			return(-1);
		}
		if(sscanf(line, "%14s %d %d %[ 0-9]", toc_pt->pkg, &toc_pt->nparts, 
		&toc_pt->maxsiz, toc_pt->volnos) < 3) {
			progerr(ERR_UNPACK, device);
			logerr(MSG_TOC);
			free(toc_pt);
			ecleanup();
			return(-1);
		}
		if(tail) {
			tail->next = toc_pt;
			tail = toc_pt;
		} else
			ds_head = tail = toc_pt;
		ds_volcnt += ds_volsum(toc_pt);
	}
	if(!ret) {
		progerr(ERR_UNPACK, device);
		logerr(MSG_TOC);
		return -1;
	}
	sighold(SIGINT);
	sigrelse(SIGINT);
	if(!ds_head) {
		progerr(ERR_UNPACK, device);
		logerr(MSG_EMPTY);
		return(-1);
	}
	/* this could break, thanks to cpio command limit */
	(void) sprintf(cmd, "%s -icdumD -C 512 ", CPIOPROC);

	for(i=0; pkg[i]; i++) {
		if(!strcmp(pkg[i], "all"))
			continue;
		strcat(cmd, pkg[i]);
		strcat(cmd, "'/*' ");
	}

	if(n = esystem(cmd, ds_fd, -1)) {
		rpterr();
		progerr(ERR_UNPACK, device);
		logerr(MSG_CMDFAIL, cmd, n);
		return(-1);
	}

	ds_toc = ds_head;
	ds_totread = 0;
	ds_volno = 1;
	return(0);
}

ds_findpkg(device, pkg)
char	*device, *pkg;
{
	char	*pkglist[2];
	int	nskip, ods_volpart;
	struct	pkgdev	pkgdev;

	if(ds_head == NULL) {
		pkglist[0] = pkg;
		pkglist[1] = NULL;
		if(ds_init(device, pkglist))
			return(-1);
	}

	if(!pkg || pkgnmchk(pkg, "all", 0)) {
		progerr(ERR_UNPACK, device);
		logerr(MSG_PKGNAME);
		return(-1);
	}
		
	nskip = 0;
	ds_volno = 1;
	ds_volpart = 0;
	ds_toc = ds_head;
	while(ds_toc) {
		if(!strcmp(ds_toc->pkg, pkg))
			break;
		nskip += ds_toc->nparts; 
		ds_volno += ds_volsum(ds_toc);
		ds_toc = ds_toc->next;
	}
	if(devtype(device, &pkgdev)) {
		progerr("bad device <%s> specifed", device);
		return(-1);
	}
	if(!ds_toc) {
		if(!pkgdev.bdevice) {
			progerr(ERR_UNPACK, device);
			logerr(MSG_NOPKG, pkg);
		}
		return(-1);
	}

	ds_pkginit();
	ds_skippart = 0;
	if(ds_curpartcnt > 0) {
		ods_volpart = ds_volpart;
		/* skip past archives belonging to last package on current volume */
		if(ds_volpart > 0 && ds_getnextvol(device))
			return -1;
		ds_totread = nskip - ods_volpart;
		if(ds_skip(device, ods_volpart))
			return -1;
	} else if(ds_curpartcnt < 0) {
		if(ds_skip(device, nskip - ds_totread))
			return -1;
	} else
		ds_totread = nskip;
	ds_read = 0;
	return(ds_nparts);
}

/*
 * Get datastream part
 * Call for first part should be preceded by
 * call to ds_findpkg
 */

ds_getpkg(device, n, dstdir) 
char	*device;
int	n;
char	*dstdir;
{
	struct statvfs buf;

	if(ds_read >= ds_nparts)
		return(2);

	if(ds_read == n)
		return(0);
	else if((ds_read > n) || (n > ds_nparts))
		return(2);

	if(ds_maxsiz > 0) {
		if(statvfs(".", &buf)) {
			progerr(ERR_UNPACK, device);
			logerr(MSG_STATFS, errno);
			return(-1);
		}
		if((ds_maxsiz + 50) > (buf.f_bavail * (((buf.f_frsize - 1) / PBLK) + 1))) {
			progerr(ERR_UNPACK, device);
			logerr(MSG_NOSPACE, ds_maxsiz+50);
			return(-1);
		}
	}
	return ds_next(device, dstdir);
}

static int
ds_getnextvol(device)
char *device;
{
	char prompt[128];
	int n;

	if(ds_close(0))
		return -1;
	(void)sprintf(prompt, "Insert %%v %d of %d into %%p", ds_volno, ds_volcnt);
	if(n = getvol(device, NULL, NULL, prompt))
		return n;
	if((ds_fd = open(device, 0)) < 0)
		return -1;
	/*
	 * If no dd process is currently reading from datastream, let's
	 * start one up.  Otherwise, we can use the one that was started
	 * for previous volume of package.
	 */
	if(dd_pid == -1) {
		if(ds_ginit(device) < 0) {
			(void)ds_close(0);
			return -1;
		}
	}
	ds_volpart = 0;
	return 0;
}

/*
 * called by ds_findpkg to skip past archives for unwanted packages 
 * in current volume 
 */
static int
ds_skip(device, nskip)
char *device;
int	nskip;
{
	char	cmd[CMDSIZ];
	int	n, onskip = nskip;
	
	while(nskip--) {
		/* skip this one */
		(void) sprintf(cmd, "%s -ictD -C 512 > /dev/null", CPIOPROC);
		if(n = esystem(cmd, ds_fd, -1)) {
			rpterr();
			progerr(ERR_UNPACK, device);
			logerr(MSG_CMDFAIL, cmd, n);
			nskip = onskip;
			if(ds_volno == 1 || ds_volpart > 0)
				return n;
			if(n = ds_getnextvol(device))
				return n;
		}
	}
	ds_totread += onskip;
	ds_volpart = onskip;
	ds_skippart = onskip;
	return(0);
}

/* skip to end of package if necessary */
void ds_skiptoend(device)
char *device;
{
	if(ds_read < ds_nparts && ds_curpartcnt < 0)
		(void)ds_skip(device, ds_nparts - ds_read);
}


int
ds_next(device, instdir)
char *device;
char *instdir; /* current directory where we are spooling package */
{
	char	cmd[CMDSIZ], tmpvol[128];
	int	nparts, n, index;
	struct	pkgdev	pkgdev;

	while(1) {
		if(ds_read + 1 > ds_curpartcnt && ds_curpartcnt >= 0) {
			ds_volno++;
			if(n = ds_getnextvol(device))
				return n;
			(void)sscanf(ds_volnos, "%d %[ 0-9]", &index, tmpvol);
			(void)strcpy(ds_volnos, tmpvol);
			ds_curpartcnt += index;
		}
		(void) sprintf(cmd, "%s -icdumD -C 512 2>/dev/null", CPIOPROC);
		if(devtype(device, &pkgdev)) {
			progerr("bad device <%s> specified", device);
			return -1;
		}

		if(n = esystem(cmd, ds_fd, -1)) {
			if(!pkgdev.bdevice) {
				rpterr();
				progerr(ERR_UNPACK, device);
				logerr(MSG_CMDFAIL, cmd, n);
			}
		}
		if(ds_read == 0)
			nparts = 0;
		else
			nparts = ds_toc->nparts;
		if(n || (n = ckvolseq(instdir, ds_read + 1, nparts))) {
			if(ds_volno == 1 || ds_volpart > ds_skippart) 
				return -1;
			if(pkgdev.bdevice) {
				char *vol, *pkgname, *pkginst, *alias;
				char *setinst, *setname;
				pkgname = getenv("NAME");
				pkginst = getenv("PKGINST");
				setname = getenv("SETNAME");
				setinst = getenv("SETINST");
				vol = devattr(pkgdev.cdevice, "volume");
				alias = devattr(pkgdev.cdevice, "alias");
				(void) fprintf(stderr, "\npkgadd: WARNING: You may have inserted the wrong %s into <%s>.\n",
						vol, alias); 
				(void) fprintf(stderr, "\nREPROMPTING FOR:\n");
				/*if(setinst && *setinst != '\0')*/
				if(setinst)
					(void) fprintf(stderr, "  Set:     %s (%s)\n", setname, setinst);
				(void) fprintf(stderr, "  Package: %s (%s)\n           %s %d of %d\n",
					pkgname, pkginst, vol, ds_volno, ds_volcnt);
			}
			if(n = ds_getnextvol(device))
				return n;
			continue;
		}
		ds_read++;
		ds_totread++;
		ds_volpart++;
	
		return(0);
	}
}

/*
 * ds_ginit: Determine the device being accessed, set the buffer size,
 * and perform any device specific initialization.  For the 3B2,
 * a device with major number of 17 (0x11) is an internal hard disk,
 * unless the minor number is 128 (0x80) in which case it is an internal
 * floppy disk.  Otherwise, get the system configuration
 * table and check it by comparing slot numbers to major numbers.
 * For the special case of the 3B2 CTC several unusual things must be done.
 * To enable
 * streaming mode on the CTC, the file descriptor must be closed, re-opened
 * (with O_RDWR and O_CTSPECIAL flags set), the STREAMON ioctl(2) command
 * issued, and the file descriptor re-re-opened either read-only or write_only.
 */

static char ds_ctcflg;

int
ds_ginit(device)
char *device;
{
	int oflag, i, size;
	char *pbufsize, cmd[CMDSIZ];
	int fd2, fd, nullfd;
	char bufsz[24];

	if((pbufsize = devattr(device, "bufsize")) != NULL) {
		ds_bufsize = atoi(pbufsize);
		(void)free(pbufsize);
	} else
		ds_bufsize = 512;
	oflag = fcntl(ds_fd, F_GETFL, 0);
	if(ds_bufsize >= 512) {
		if(oflag & O_WRONLY)
			fd = 1;
		else
			fd = 0;
		/* save fd in fd2 - same as fd2=dup(fd) */
		fd2 = fcntl(fd, F_DUPFD, fd);
		(void)close(fd);
		/* datastream is accessible via stdin/stdout */
		fcntl(ds_fd, F_DUPFD, fd);
		if(fd)
			sprintf(cmd, "%s obs=%d 2>/dev/null", DDPROC, ds_bufsize);
		else
			sprintf(cmd, "%s ibs=%d", DDPROC, ds_bufsize);

		/* dd reads/writes from/to stdin (fd_fs) */ 
		if (!fd) {

			/*
			 * The following section replaces the popen() to a pipe(), fork()
			 * and exec() of the dd process so that we can identify the process
			 * doing the dd.  This was done so that we could terminate it before 
			 * closing the stream device during set installation without causing
			 * the "Broken Pipe" message displayed.   We need to terminate the
			 * dd process after each package has been installed off of a datastream
			 * diskette.  This is to prevent multiple dd processes being around.
			 */

			/* We'll read from pipe */
			if(pipe(pkgpipe) < 0) {
				progerr(ERR_TRANSFER);
				logerr(MSG_POPEN, cmd, errno);
				return -1;
			}
			dd_pid = fork();	/* dd_pid identifies the dd process */
			if (dd_pid < 0) {
				progerr("bad fork()");
				return -1;
			} else if(!dd_pid) {
				/*
				 * Child process.
				 * Set up dd process writing to stdout 
				 */
				(void) close(ds_fd);
				(void) close(pkgpipe[0]);
				/* pkgpipe write end to stdout */
				(void) dup2(pkgpipe[1], 1);
				(void) close(pkgpipe[1]);
				/* redirect stderr to /dev/null for dd command */
				if ((nullfd = open("/dev/null", O_WRONLY)) < 0) {
					progerr("error opening /dev/null");
					return -1;
				}
				(void) dup2(nullfd, 2);
				(void) close(nullfd);
				(void) sprintf(bufsz, "ibs=%d", ds_bufsize);
				execl(DDPROC, "dd", bufsz, (char *)0);
				progerr("bad exec() of dd command");
				return -1;
			}
			/* 
			 * Parent continues.
			 * Read from pipe writen to by dd process 
			 */
			close(pkgpipe[1]);	/* close write end of pipe */
			/* associate ds_fd w/pkgpipe[0] also close previous ds_fd */
			ds_realfd = ds_fd;
			(void) dup2(pkgpipe[0], ds_fd);

			if ((ds_pp = fdopen(ds_fd, "r")) == NULL) {
				progerr("error in fdopen");
				return -1;
			}
			(void) close(fd);
			/* restore fd */
			fcntl(fd2, F_DUPFD, fd);
			(void) close(fd2);
		} else {
			/* We'll write to pipe */
			if((ds_pp = popen(cmd, "w")) == NULL) {
				progerr(ERR_TRANSFER);
				logerr(MSG_POPEN, cmd, errno);
				return -1;
			}
			(void) close(fd);
			fcntl(fd2, F_DUPFD, fd);
			(void) close(fd2);
			ds_realfd = ds_fd;
			ds_fd = fileno(ds_pp);
		}
	}
	return(ds_bufsize);
}

int 
ds_close(pkgendflg)
int pkgendflg;
{
	int n, ret = 0;

	if(pkgendflg) {
		if(ds_header)
			(void)free(ds_header);
		ds_header = (char *)NULL;
		ds_totread = 0;
	}

	if(ds_pp) {
		(void)pclose(ds_pp);
		ds_pp = 0;
		(void)close(ds_realfd);
		ds_realfd = -1;
		ds_fd = -1;
	} else if(ds_fd >= 0) {
		(void)close(ds_fd);
		ds_fd = -1;
	}

	if(ds_device) {
		/* rewind device */
		if((n = open(ds_device, 0)) >= 0)
			(void)close(n);
		ds_device = NULL;
	}
	return ret;
}

/*
 * ds_pkgonstream is used to check if the package about to
 * be installed as part of a set installation is already on
 * the current datastream diskette.  If it is, we return a
 * one (1), thus signaling pkgadd not to prompt for the next
 * media.  If it is not, we return a zero (0), thus signaling
 * pkgadd to prompt for the next media.
 */
int
ds_pkgonstream(pkg)
char *pkg;
{

	struct dstoc *ds_sfx;

	ds_sfx = ds_head;
	while(ds_sfx->next) {
		if(!strcmp(ds_sfx->pkg, pkg)) {
			return 1;
			break;
		}
		ds_sfx = ds_sfx->next;
	}
	if(!strcmp(ds_sfx->pkg, pkg))
		return 1;
	else
		return 0;
}
