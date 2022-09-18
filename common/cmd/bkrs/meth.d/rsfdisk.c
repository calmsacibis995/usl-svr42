/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/meth.d/rsfdisk.c	1.12.5.3"
#ident  "$Header: rsfdisk.c 1.2 91/06/21 $"

#include	<limits.h> 	/* get PATH_MAX from here, not stdio */
#include	<sys/types.h>
#include	<sys/vtoc.h>
#include	<stdio.h>
#include	<fcntl.h>
#include	<string.h>
#include	<signal.h>
#include	<backup.h>
#include	<bktypes.h>
#include	<bkrs.h>
#include	<brarc.h>
#include 	"libadmIO.h"
#include	<method.h>
#include	<errno.h>

#define VSTR		"volume \""
#define MAXSLICE	V_NUMPAR

#define NEW_INPUT new_Input(MP, arc_name, &bytes_left, &checksize, &arc_info)

extern int	brlog();
extern int	brsndfname();
extern void	*malloc();
extern GFILE	*new_Input();
extern int	safe_write();
extern int	strfind();

extern int	bklevels;

m_info_t	*MP;

long			bytes_left = 0;
short			checksize = 0;
struct archive_info	arc_info;

static int	fsadd();
static int	send_to_fmthard();
static int	wait_on_fmthard();
static int	do_mkfs();
static void	getvolname();

static char	arc_name[PATH_MAX+1];

static char	fname[513];
static char	cmd[520];
static char	*volname = NULL;	/* fmthard vol name if any */
static int	totslices = 0;
static int	nfilsys = 0;		/* num file systems on this dev */
static int	ndpart = 0;		/* num non fs partitions */
static int	nmntpt = 0;		/* num mount points */
static char	*fsdata[MAXSLICE];	/* nfilsys pointers to fs info */
static char	*mpt[MAXSLICE];		/* nmntpt pointers to mntpt name */
static int	dpart_slices[MAXSLICE];	/* ndpart slice chars */
static char	ronly[MAXSLICE];	/* read only fs chars */

static GFILE	*arc = NULL;		/* archive device/file */
static FILE	*ar;			/* archive fd */
static int	ed_fnum = -1;		/* fmthard file num */
static FILE	*ed_fp;			/* popen fd to fmthard */

do_rsfdisk(mp)
m_info_t	*mp;
{
	int	isfile;
	int	len;
	int	readnum;
	int	ret;
	char	*dmname = NULL;
	char	rec[520];
	char	*buf;
	int	errors = 0;
	int	rserrors = 0;

	MP = mp;

	if (mp->nfsdev && strlen(mp->nfsdev))
		mp->ofsdev = mp->nfsdev;

	arc = NEW_INPUT;

	if (arc == NULL) {
		sprintf(ME(mp), "Job ID %s: g_open of %s failed: %s", mp->jobid, fname, SE);
		brlog(" do_rsfdisk open of archive %s failed %s", fname, SE);
		return(1);
	}
	ar = fdopen(arc->_file, "r");

	/* We assume that low level formatting has already been done
	* using disksetup -I. Now we restore the old vtoc entries.
	*/
	(void) sprintf(cmd, "/sbin/edvtoc -f - %s", MP->ofsdev);
#ifdef        TRACE
	brlog("popen cmd=%s", cmd);
#endif        /* TRACE */
	if ((ed_fp = popen(cmd, "w")) == NULL) {
		brlog("unable to execute %s",cmd);
		return(1);
	}


	for (len = 1, readnum = 0, ret = 1; 1 ; readnum++) {
		BEGIN_CRITICAL_REGION;

		buf = fgets(rec, 512, ar);

		END_CRITICAL_REGION;

		if (!buf) {
			break;
		}
		/* remember that we are padding the output file to
		* full 512 byte blocks. Thus '\0' indicates) EOF.
		*/
		if (*buf == '\0') {
			break;
		}

		len = strlen(rec);

		if (rec[0] == '*') {
			if (!readnum) {
				getvolname(rec, len);
			}
			continue;
		}
		if (rec[0] == '#') {
			if (ret = fsadd(rec, len))
				break;

			continue;
		}
		if (++totslices > MAXSLICE) {
			brlog("Too many slices: %d slices?, rec=\"%s\"",
			 totslices, rec);
			ret = 1;
			break;
		}
		if (send_to_fmthard(rec, len)) {
			(void) wait_on_fmthard();
			sprintf(ME(mp), "Job ID %s: archive format in error", mp->jobid);
			brlog("archive format in error");
			return(1);
		}

	}
	if (ret = wait_on_fmthard()) {
		sprintf(ME(mp), "Job ID %s: fmthard failed", mp->jobid);
		brlog("fmthard failed pclose returned 0x%x", ret);
		return(1);
	}
#ifdef TRACE
	brlog ("Restoring data partitions, n_dpart = %d", ndpart);
#endif
	if (nmntpt != nfilsys) {
		brlog("%d mnt points, %d fs slices in archive",nmntpt,nfilsys);
	}
	for (ret = 0; ret < ndpart; ret++) {
		int	i;

		(void) sprintf(rec, "/bin/restore -P %s", mp->ofsdev);
		len = strlen(rec) - 1;
		(void) sprintf(rec+len, "%x", dpart_slices[ret]);
#ifdef TRACE
		brlog("%s",rec);
#endif
		if ((i = bk_system(rec)) != 0) {
			brlog("restore for %s failed ret=0x%x",mp->ofsdev,i);
			rserrors++;
		}
	}
	if (rserrors) {
		brlog("%d data partition restore errors", rserrors);
	}
#ifdef TRACE
	brlog ("Restoring file systems, n_filsys = %d", nfilsys);
#endif
	for(ret = 0; ret < nfilsys; ret++) {
		errors += do_mkfs(ret);
	}
	if (errors) {
		brlog("%d file system errors", errors);
	}
	if (errors || rserrors) {
		sprintf(ME(mp), "Job ID %s: not all partitions restored", mp->jobid);
		return(1);
	}
	return(0);
} /* do_rsfdisk() */


static int
send_to_fmthard(rec, length)
char	*rec;
int	length;
{
	int	ret;
	int	i;
	int	tag;
	int	flag;
	char	vstring[260];
	int	slice;
	char	*c;

	if ((i=sscanf(rec, " %d 0x%x 0x%x",&slice,&tag,&flag)) != 3) {
		brlog ("send_to_fmthard: bad input format, got %d of %d fields",
			i, 3);
		return (1);
	}
#ifdef TRACE
	brlog("slice=%x, tag=0x%x, flag=0x%x", slice ,tag, flag);
#endif

	if (MP->flags & Vflag) {
		strcpy (vstring, MP->ofsdev);
		i = strlen(vstring) - 1;
		(void) sprintf(vstring+i, "%x", slice);
		brsndfname(vstring);
	}
	if ((flag & V_VALID) && !(flag & V_UNMNT)) {
		ronly[nmntpt++] = (flag & V_RONLY) ? 1 : 0;
	}
	ed_fnum = fileno(ed_fp);
	ret = safe_write(ed_fnum, rec, length);

	if (ret != length) {
		brlog("write error to fmthard - %s",SE);
		return(1);
	}
	if ((flag & V_VALID) == 0)	/* no valid partition */
		return (0);

	if ((flag & V_UNMNT) == 0)	/* mountable fs */
		return(0);

	if (tag == V_BACKUP) {		/* full disk */
		brlog ("slice %x: full disk - supposed to be not used", slice);
		return (0);
	}
	dpart_slices[ndpart++] = slice;	/* must be a data partition */

	return(0);
} /* send_to_fmthard() */


static int
wait_on_fmthard()
{
	if (ed_fp == NULL) {
		brlog("fmthard stdin not established");
		return(-1);
	}
	return (pclose(ed_fp));
} /* wait_on_fmthard() */

static void
getvolname(rec, length)
char	*rec;
int	length;
{
	int	offset;
	int	end;

	offset = strfind(rec, VSTR);

	if (offset < 0)
		return;

	offset += strlen(VSTR);

	end = strfind((rec + offset), "\"");

	if (end < 0)
		return;

	if ((end + offset) > length)
		return;

	volname = (char *) malloc((unsigned) (end + 1));

	if (volname == NULL) {
		brlog("no memory for volume name");
		return;
	}
	(void) strncpy(volname, (rec + offset), end);

	*(volname + end) = 0;
} /* getvolname() */

static int
fsadd(rec, length)
char	*rec;
int	length;
{
	char	*fsd;

	fsd = (char *) malloc((unsigned) (length + 1));

	if (fsd == NULL) {
		brlog("no memory for fsdata");
		return(1);
	}
	fsdata[nfilsys++] = fsd;

	(void) strcpy(fsd, rec);

	return(0);
} /* fsadd() */

static int
do_mkfs(idx)
int	idx;
{
	int	ret;
	int	slice;
	char	*c;
	char	fstype[20];
	char	fsname[20];
	char	volname[20];
	char	cmd[520];
	char	blkdev[256];
	char	mntpt[256];
	int	blocks;
	int	inodes;
	int	i, len;
	char	*fi = fsdata[idx];

	if (fi[0] != '#') {
		brlog ("do_mkfs: input format error: %s", fi);
		return(1);
	}

	if (sscanf(fi+1, " %x", &slice) != 1) {
		brlog ("do_mkfs: input format error: slice");
		return (1);
	}

	if ((i = strfind (fi, "bdevice=")) < 0) {
		brlog ("do_mkfs: input format error: bdevice");
		return (1);
	}
	
	len = 0; c = fi + i + 8;
	while (*c && !(isspace(*c)) && len < sizeof(blkdev)-1)
		blkdev[len++] = *c++;
	blkdev[len] = '\0';

	if ((i = strfind (fi, "mountpt=")) < 0) {
		brlog ("do_mkfs: input format error: mountpt");
		return (1);
	}
	
	len = 0; c = fi + i + 8;
	while (*c && !(isspace(*c)) && len < sizeof(mntpt)-1)
		mntpt[len++] = *c++;
	mntpt[len] = '\0';

	if ((mpt[idx] = strdup(mntpt)) == NULL) {
		brlog ("do_mkfs: out of memory for mountpt");
		return (1);
	}
	if ((i = strfind (fi, "fsname: ")) < 0) {
		brlog ("do_mkfs: input format error: fsname");
		return (1);
	}
	
	c = fi + i + 8;
	/* terminate at ' ' (UFS) and ',' (S5)
	 * The differing formats are a real pain!
	 */
	len = 0;
	while (*c && !(isspace(*c)) && (*c != ',') && len < sizeof(fsname)-1)
		fsname[len++] = *c++;
	fsname[len] = '\0';

	/* at first try UFS labelit format */
	if ((i = strfind (fi, "volume: ")) < 0) {
		/* then try S5 labelit format */
		if ((i = strfind (fi, "volname: ")) < 0) {
			/* then barf */
			brlog ("do_mkfs: input format error: volume");
			return (1);
		}
		len = 9;
	} else	len = 8;
	
	c = fi + i + len;
	/* terminate at ' ' (UFS) and ',' (S5)
	 * The differing formats are a real pain!
	 */
	len = 0;
	while (*c && !(isspace(*c)) && (*c != ',') && len < sizeof(volname)-1)
		volname[len++] = *c++;
	volname[len] = '\0';

#ifdef TRACE
	brlog ("do_mkfs(%d): blkdev=\"%s\" mntpt=\"%s\" fsname=\"%s\" volname=\"%s\"",\
	idx, blkdev, mntpt, fsname, volname);
#endif

	/*
	 * just be sure the partition is not mounted yet
	 */
	(void) sprintf (cmd, "/sbin/umount %s", blkdev);
	/*
	 * ignore errors: the fsys might be simply not mounted.
	 * if the umount fails because it is busy we lose.
	 */
#ifdef TRACE
	brlog ("do_mkfs: cmd=%s", cmd);
#endif
	(void) bk_system(cmd);

	if ((i = strfind (fi, "mkfs -")) < 0) {
		brlog ("do_mkfs: input format error: mkfs");
		return (1);
	}
	
	c = fi + i;
	
	(void) sprintf(cmd, "/sbin/%s >/dev/null 2>&1", c); 
#ifdef TRACE
	brlog("do_mkfs: cmd=%s", cmd);
#endif

	if ((ret = bk_system(cmd)) != 0) {
		brlog("do_mkfs: mkfs for %s failed ret=0x%x", blkdev ,ret);
		return(1);
	}
	(void) sprintf(cmd,
		 "/usr/sbin/labelit %s %s %s", blkdev, fsname, volname);
#ifdef TRACE
	brlog("do_mkfs: cmd=%s",cmd);
#endif

	if ((ret = bk_system(cmd)) != 0) {
		brlog("do_mkfs: labelit for %s failed ret=0x%x", blkdev, ret);
		return(1);
	}
	if (mpt[idx]) {
		(void) sprintf(cmd, "%s %s %s", ronly[idx] ? "/sbin/mount -r" : "/sbin/mount",
						blkdev, mpt[idx]);
#ifdef TRACE
		brlog("do_mkfs: cmd=%s", cmd);
#endif

		if ((ret = bk_system(cmd)) != 0) {
			brlog("do_mkfs: %s failed ret=0x%x", cmd, ret);
			return(1);
		}
	}
	(void) sprintf(cmd,"/sbin/restore -S %s", mntpt);
#ifdef TRACE
	brlog("do_mkfs: cmd=%s", cmd);
#endif

	if ((ret = bk_system(cmd)) != 0) {
		brlog("do_mkfs: %s failed ret=0x%x", cmd, ret);
		return(1);
	}
	return(0);
} /* do_mkfs() */
