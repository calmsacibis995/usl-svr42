/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/meth.d/bkfdisk.c	1.13.5.3"
#ident  "$Header: bkfdisk.c 1.2 91/06/21 $"

#include	<limits.h> 	/* get PATH_MAX from here, not stdio */
#include	<sys/types.h>
#include	<sys/vtoc.h>
#include	<devmgmt.h>
#include	<ctype.h>
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
#include	<setjmp.h>
#include	<errno.h>

extern int		bknewvol();
extern struct br_arc	*bld_hdr();
extern int		brlog();
extern int		brsndfname();
extern int		chk_vol_sum();
extern int		close();
extern void		do_history();
extern GFILE		*g_open();
extern int		g_write();
extern int		strfind();
extern void		sum();
extern time_t		time();

/* When doing backups on tape the byte count for write() must always
* be an exact multiple of 512. So we have to do some buffering...
*/
int pcs_write (m_info_t *mp, GFILE *Fdisk, char *buf, int len);
int pcs_flush (m_info_t *mp, GFILE *Fdisk);


extern int	bklevels;


short		reuse_dmname = 0;
char		fname[1025];
char		tocname[] = "";
unsigned	Vol_sum = 0;		/* for -v option */
jmp_buf		env;
m_info_t	*MP;

static int	fsinfo();
static GFILE	*openfd();

static int		bytes_summed = 0;
static GFILE		*Fdisk = NULL;
static media_info_t	IMM;

do_bkfdisk(mp)
m_info_t *mp;
{
	FILE	*pv;
	FILE	*lv;
	char	pvtoc[512];
	char	fsdev[128];
	char	mntpt[256];
	char	fn[256];
	char	*wrk;
	int	slice;
	int	flags;
	int	ret;
	int	isfile;
	int	devlen;
	int	len;
	int	errors = 0;
	short	numfs = 0;

	mp->bkdate = time((long *) 0);
	MP = mp;
	ret = setjmp(env);

	if (ret)  {				  /* error has occurred */
		brlog(" do_bkfdisk ret=%d ", ret);
		mp->blk_count >>= 9;
		return(1);
	}
newarc:
	if ((isfile = bknewvol(mp, fname, &reuse_dmname, &env, &IMM)) < 0) {
		brlog(" bknewvol failed for %s", fname);
		sprintf(ME(mp), "Job ID %s: new volume failed for %s", mp->jobid, fname);
		return(1);
	}
	BEGIN_CRITICAL_REGION;
	if (isfile) {
		Fdisk = g_open(fname, (O_WRONLY|O_CREAT|O_TRUNC), 0644);

		if (Fdisk == NULL) {
			brlog(" openfd: cannot create %s %s ", fname, SE);
			sprintf(ME(mp), "Job ID %s: g_open failed for %s: %s", mp->jobid, fname, SE);
			return(1);
		}
	}
	/* else wait until doing the br_write_hdr to open */
	/* This is done since the header needs to be read */
	/* and verified prior to writing and some devices */
	/* will not allow closing and reopenning the      */
	/* device without changing the media. (e.g. the   */
	/* tapes will write a file mark on close.)        */
	END_CRITICAL_REGION;

	ret = wr_bkrs_hdr();
	if (ret < 0) {
		brlog("unable to write bkrs hdr on archive");
		sprintf(ME(mp), "Job ID %s: write of archive header failed for %s", mp->jobid, fname);
		return(1);
	}
	(void) strcpy(fsdev, mp->ofsdev);
	devlen = strlen(fsdev) - 1;

	(void) sprintf(pvtoc, "/usr/sbin/prtvtoc -f - %s", mp->ofsdev);
#ifdef TRACE
	brlog("pvtoc = %s", pvtoc);
#endif
	BEGIN_CRITICAL_REGION;

	pv = popen(pvtoc, "r");

	END_CRITICAL_REGION;

	if (pv == NULL) {
		brlog("cannot prtvtoc of %s", mp->ofsdev);
		sprintf(ME(mp), "Job ID %d: prtvtoc failed for %s", mp->jobid, mp->ofsdev);
		return(1);
	}
	mntpt[0] = 0;

	while(1) {
		BEGIN_CRITICAL_REGION;

		wrk = fgets(pvtoc, 512, pv);

		END_CRITICAL_REGION;

		/* no need to copy comment lines */
		if (pvtoc[0] == '#')
			continue;

		if (wrk == NULL)
			break;

		len = strlen(pvtoc);

		ret = pcs_write (mp, Fdisk, pvtoc, len);

		bytes_summed += len;
		sum (pvtoc, (long) len, &Vol_sum);

		mp->blk_count += len;

		/* prtvotc really uses _this_ format.
		 * Unfortunately, there is no mountpoint information there
		 * (maybe once there was?).
		 * Some error checking added
		 */
		if (sscanf(pvtoc, " %d 0x%*x 0x%x",&slice,&flags) != 2) {
			brlog("do_bkfdisk: scanf of prtvtoc output failed");
			sprintf(ME(mp), "Job ID %s: bad format of %s output",
				mp->jobid, "/usr/sbin/prtvtoc");
			return (1);
		}
#ifdef	TRACE
		brlog ("slice=%x, flags=0x%x", slice, flags);
#endif	/* TRACE */
		/* skip unvalid entries, use symbolic names for flags */
		if ((flags & V_VALID) == 0)
			continue;
	  
		/* build device name from constant part and slice number */
		sprintf (fsdev+devlen, "%x", slice);

		if (flags & V_UNMNT) {  /* NOT mountable */
			brlog("slice %x NOT mountable", slice);
			(void) sprintf(fn, "dp  %s", fsdev);
		} else {
			++numfs;
			errors += fsinfo(fsdev, mp, slice, mntpt);
			(void) sprintf(fn, "fs  %s  %s", fsdev, mntpt);
		}

		if (mp->flags & Vflag) {
			(void) brsndfname(fn);
		}
		mntpt[0] = NULL;
	}
	BEGIN_CRITICAL_REGION;

	ret = pclose(pv);

	END_CRITICAL_REGION;

	if ((ret != 0) && !numfs) {	/* did popen already wait for prtvtoc */
		brlog("prtvtoc exit code 0x%x %s",ret,SE);
		sprintf(ME(mp), "Job ID %s: prtvtoc failed for %s", mp->jobid, mp->ofsdev);
		return(1);
	}
	if (errors) {
		brlog("fsinfo errors - terminating");
		sprintf(ME(mp), "Job ID %s: fsinfo failed for %s",
			mp->jobid, mp->ofsdev);
		return(1);
	}
	/* flush internal buffer */
	bytes_summed += pcs_flush (mp, Fdisk);
	if (mp->flags & vflag) {
		if (chk_vol_sum(mp, &Fdisk,
				(long) bytes_summed, fname, Vol_sum)) {
			(void) g_close(Fdisk);
			goto newarc;
		}
	}
	(void) g_close(Fdisk);

	if (IMM.cur) {			/* last vol was good */
		if (IMM.first == NULL) 
			IMM.first = IMM.cur;
		if (IMM.last)
			(IMM.last)->next = IMM.cur;
		IMM.last = IMM.cur;
	}
	mp->blk_count += 511;
	mp->blk_count >>= 9;

	do_history(mp, &IMM, -1);

	return(0);
} /* do_bkfdisk() */

#ifdef XXXXXXXX
	/* see below endif for replacement fsinfo */
static int
fsinfo(dev, mp, slice, mntpt)
char	*dev;
m_info_t *mp;
char	slice;
char	*mntpt;
{
	char	cmd[64];
	char	buf[512];
	char	*mntnm;
	FILE	*la;
	int	len;
	int	ret;
	int	i;

	(void) sprintf(cmd, "/sbin/labelit %s", dev);
#ifdef TRACE
	brlog("labelit command=%s", cmd);
#endif
	BEGIN_CRITICAL_REGION;

	la = popen(cmd, "r");

	END_CRITICAL_REGION;

	if (la == NULL) {
		brlog("cannot read label of %s", dev);
		return(1);
	}
	buf[0] = '#';
	buf[1] = slice;
	buf[2] = NULL;

	BEGIN_CRITICAL_REGION;

	len = strlen(buf);

	while (fgets((buf+len+1), 512-len-1, la) != NULL) {
		buf[len] = ' ';
		len = strlen(buf);
		buf[len-1] = ' ';
		buf[len] = '\n';
		buf[len+1] = NULL;
	}
	if ((i = strfind (buf, "Current fsname: ")) >= 0) {
		int	j = 0;
		char	*label;

		i += 16;
		label = &buf[i];

		while ((j < 9) && (label[j++] != ','));

		label[--j] = NULL;
		brlog("label %s found for slice %c", label, slice);

		mntnm = strrchr (mntpt, '/');

		if (strcmp (mntnm+1, label) != 0) {
			brlog("labelit mismatch for slice %c",slice);
			brlog("label=%s, mntpt=%s, mntnm=%s",label, mntpt, mntnm);
		}
		label[j] = ',';
	}
	(void) pclose(la);

	END_CRITICAL_REGION;

	(void) sprintf(cmd, "/sbin/mkfs -m %s", dev);
#ifdef TRACE
	brlog("mkfs command=%s", cmd);
#endif
	BEGIN_CRITICAL_REGION;

	la = popen(cmd, "r");

	END_CRITICAL_REGION;

	if (la == NULL) {
		brlog("cannot read mkfs -m of %s", dev);
		return(1);
	}
	BEGIN_CRITICAL_REGION;

	if (fgets((buf+len+1), 512-len-1, la) == NULL) {
		brlog("cannot read mkfs output from %s - Hopefully we won't need it!", dev);
	}
	buf[len] = ' ';
	len = strlen(buf);
	buf[len-1] = ' ';
	buf[len] = '\n';
	buf[len+1] = NULL;

	if (fgets(cmd, 512, la) != NULL) {
		brlog("mkfs output from %s more than one line? - Hopefully this won't screw me up!", dev);
	}
	END_CRITICAL_REGION;
	len = strlen(buf);
	ret = g_write(Fdisk, buf, len);

	if (ret != len) {
		brlog("archive write error %s",SE);
		return(1);
	}
	bytes_summed += len;
	sum (buf, (long) len, &Vol_sum);
	mp->blk_count += len;

	BEGIN_CRITICAL_REGION;

	(void) pclose(la);

	END_CRITICAL_REGION;

	return(0);
} /* fsinfo() */
#endif /* XXXXXXXXXX */

/* replacement fsinfo */
static int
fsinfo(dev, mp, slice, mntpt)
char	*dev;
m_info_t *mp;
int	slice;
char	*mntpt;
{
	char	cmd[64];
	char	buf[512];
	char	*cdev;
	char	*bdev;
	char	*fstype;
	char	*c;
	FILE	*la;
	int	len;
	int	ret;
	int	i;

	if ((c = devattr (dev, "cdevice")) == NULL || strlen(c) == 0) {
		brlog ("no attr \"cdevice\" for device %s", dev);
		return (1);
	}
	if ((cdev = strdup(c)) == NULL) {
		brlog ("out of memory for cdevice");
		return (1);
	}

	if ((c = devattr (dev, "bdevice")) == NULL || strlen(c) == 0) {
		brlog ("no attr \"bdevice\" for device %s", dev);
		free (cdev);
		return (1);
	}
	if ((bdev = strdup(c)) == NULL) {
		brlog ("out of memory for bdevice");
		free (cdev);
		return (1);
	}

	if ((c = devattr (dev, "mountpt")) == NULL || strlen(c) == 0) {
		brlog ("no attr \"mountpt\" for device %s", dev);
		free (cdev);	free (bdev);
		return (1);
	}
	if ((mntpt = strdup(c)) == NULL) {
		brlog ("out of memory for mountpt");
		free (cdev);	free (bdev);
		return (1);
	}

	if ((c = devattr (dev, "fstype")) == NULL || strlen(c) == 0) {
		brlog ("no attr \"fstype\" for device %s", dev);
		free (cdev);	free (bdev);	free (mntpt);
		return (1);
	}
	if ((fstype = strdup(c)) == NULL) {
		brlog ("out of memory for fstype");
		free (cdev);	free (bdev);	free (mntpt);
		return (1);
	}

	len = sprintf (buf, "# %x bdevice=%s cdevice=%s mountpt=%s fstype=%s ",
		slice, bdev, cdev, mntpt, fstype);
	ret = pcs_write(mp, Fdisk, buf, len);

	if (ret != len) {
		brlog("archive write error %s (len=%d, ret=%d)",
			SE, len, ret);
		free (cdev);	free (bdev);
		free (mntpt);	free (fstype);
		return(1);
	}
	bytes_summed += len;

	/*
	 * may be paranoid - but verify fstype
	 */

	(void) sprintf (cmd, "/sbin/fstyp %s", dev);
#ifdef TRACE
	brlog("fstyp command=%s", cmd);
#endif
	BEGIN_CRITICAL_REGION;

	la = popen(cmd, "r");

	END_CRITICAL_REGION;

	if (la == NULL) {
		brlog("cannot read fstyp of %s", dev);
		free (cdev);	free (bdev);
		free (mntpt);	free (fstype);
		return(1);
	}
	BEGIN_CRITICAL_REGION;

	if (fgets(buf, sizeof(buf), la) == NULL) {
		brlog("cannot read fstyp output from %s", dev);
	}
	(void) pclose(la);

	END_CRITICAL_REGION;

	i = strlen(buf) - 1;
	if (i >= 0)		/* remove trailing \n */
		buf[i] = '\0';

	if (strcmp (buf, fstype) != 0) {
		brlog ("fstype inconsistent: device.tab=%s, fstyp=%s",
			fstype, buf);
		free (cdev);	free (bdev);
		free (mntpt);	free (fstype);
		return (1);
	}

	(void) sprintf(cmd, "/usr/sbin/labelit %s", dev);
#ifdef TRACE
	brlog("labelit command=%s", cmd);
#endif
	BEGIN_CRITICAL_REGION;

	la = popen(cmd, "r");

	END_CRITICAL_REGION;

	if (la == NULL) {
		brlog("cannot read label of %s", dev);
		return(1);
	}

	BEGIN_CRITICAL_REGION;

	len = 0;
	while (fgets(buf+len, sizeof(buf)-len, la) != NULL) {
		len = strlen(buf);
		if (len > 0)
			buf[len-1] = ' ';
		if (len >= sizeof(buf)-1) {
			brlog ("more than %d bytes output from labelit - ignored",
				sizeof(buf));
			break;
		}
	}
	(void) pclose(la);

	END_CRITICAL_REGION;

	ret = pcs_write(mp, Fdisk, buf, len);

	if (ret != len) {
		brlog("archive write error %s (len=%d, ret=%d)",
			SE, len, ret);
		free (cdev);	free (bdev);
		free (mntpt);	free (fstype);
		return(1);
	}
	bytes_summed += len;

	(void) sprintf(cmd, "/sbin/mkfs -m %s", dev);
#ifdef TRACE
	brlog("mkfs command=%s", cmd);
#endif
	BEGIN_CRITICAL_REGION;

	la = popen(cmd, "r");

	END_CRITICAL_REGION;

	if (la == NULL) {
		brlog("cannot read mkfs -m of %s", dev);
		free (cdev);	free (bdev);
		free (mntpt);	free (fstype);
		return(1);
	}
	BEGIN_CRITICAL_REGION;

	if (fgets(buf, sizeof(buf), la) == NULL) {
		brlog("cannot read mkfs output from %s - Hopefully we won't need it!", dev);
	}

	c = buf;
	while (*c && strncmp(c, "mkfs", 4) != 0)
		++c;
	if (*c == 0)
		brlog ("bad format of mkfs output: %s", buf);

	len = strlen(c);
	ret = pcs_write(mp, Fdisk, c, len);

	if (ret != len) {
		brlog("archive write error %s (len=%d, ret=%d)",
			SE, len, ret);
		free (cdev);	free (bdev);
		free (mntpt);	free (fstype);
		return(1);
	}
	bytes_summed += len;
	if (fgets(cmd, 512, la) != NULL) {
		brlog("mkfs output from %s more than one line? - Hopefully this won't screw me up!", dev);
	}
	END_CRITICAL_REGION;
	sum (buf, (long) len, &Vol_sum);
	mp->blk_count += len;

	BEGIN_CRITICAL_REGION;

	(void) pclose(la);

	END_CRITICAL_REGION;

	free (cdev);	free (bdev);	free (mntpt);	free (fstype);
	return(0);
} /* fsinfo() */



int
wr_bkrs_hdr()
{
	int	i;
	int	hdrsize;
	long	nbytes;
	char	*typstrng;
	struct br_arc	*hdr;
	struct wr_archive_hdr	brhd;
	struct wr_archive_hdr	*b = &brhd;
	struct bld_archive_info	brai;
	struct bld_archive_info	*ai = &brai;

	Vol_sum = 0;		/* for -v option */
	bytes_summed = 0;

	ai->br_method    = MN(MP);		/* method name */
	ai->br_fsname    = OFS(MP);		/* file system name */
	ai->br_dev       = ODEV(MP);		/* backup object device */
	ai->br_date      = MP->bkdate;		/* date-time of backup */
	ai->br_seqno     = 1;			/* sequence num of this vol */
	ai->br_media_cap = MP->blks_per_vol;	/* capacity in 512 byte blks */
	ai->br_blk_est   = 10;			/* num of blks in archive */
	ai->br_flags     = 0;

	/* ai->br_fstype and ai->br_mname are used in libbrmeth.d/bkhdr.c
	* We better initialize these fields...
	*/
	ai->br_mname    = NULL;
	ai->br_fstype   = NULL;

	if (IMM.cur) {
		ai->br_mname = (IMM.cur)->label;
	}
	else {
		ai->br_mname = NULL;
	}
	hdr = bld_hdr (ai, &hdrsize);

	if (hdr == NULL) {
		brlog("unable to build archive hdr");
		sprintf(ME(MP), "Job ID %s: unable to build archive header", MP->jobid);
		return(-1);
	}
	b->br_hdr = hdr;
	b->br_hdr_len = hdrsize;
	typstrng = "\0";

	if (MP->dtype == IS_DPART) {
		typstrng = "dpart";
		nbytes = (MP->blks_per_vol) << 9;
	}
	else {
		if (MP->dtype == IS_FILE)
			typstrng = "file";
		else if (MP->dtype == IS_DIR)
			typstrng = "dir";
		nbytes = 0;
	}
#ifdef TRACE
	brlog ("wr_bkrs_hdr: before br_write_hdr() Fdisk=0x%x typstrng=%s fname=%s",
		Fdisk, typstrng, fname);
#endif
	i = br_write_hdr(&Fdisk, typstrng, MP->volpromt, b, nbytes, &fname);

	if (i < 0 ) {
		brlog("unable to write bkrs hdr on archive");
		sprintf(ME(MP), "Job ID %s: unable to write archive header to %s", MP->jobid, fname);
		return(-1);
	}
#ifdef TRACE
	brlog ("wr_bkrs_hdr: after br_write_hdr(): Fdisk = 0x%x, Fdisk->_file=%d",
		Fdisk, Fdisk->_file);
#endif
	if (MP->flags & vflag) {
		if (b->br_lab_len) {
			bytes_summed += (b->br_lab_len);
			sum (b->br_labelit_hdr, (long) (b->br_lab_len), &Vol_sum);
		}
		if (MP->dtype != IS_DPART)
			bytes_summed += (long) hdrsize;
			sum ((char *)hdr, (long) hdrsize, &Vol_sum);
	}
	if (MP->dtype == IS_DPART) {
		IMM.bytes_left -= hdrsize;
	}
	else {
		if (MP->blks_per_vol > 0) {  /* o/w go to eof */
			IMM.bytes_left -= bytes_summed;
		}
	}
	return(0);
} /* wr_bkrs_hdr() */

#define	IO_BSIZE	512
#define	IO_BMASK	(IO_BSIZE-1)

char	pcs_buf[IO_BSIZE];
char	*pcs_b = pcs_buf;

int
pcs_write (m_info_t *mp, GFILE *Fdisk, char *buf, int len)
{
	int	free;
	int	out = 0;

	while ((free = &pcs_buf[IO_BSIZE] - pcs_b) < len) {
		memcpy (pcs_b, buf, free);
		if (g_write(Fdisk, pcs_buf, IO_BSIZE) != IO_BSIZE) {
			brlog("do_bkfdisk: g_write failed, errno=%d", errno);
			sprintf(ME(mp), "Job ID %s: write error for %s",
				mp->jobid, mp->ofsdev);
			return(1);
		}
		memset (pcs_b, 0, IO_BSIZE);
		buf += free;
		len -= free;
		out += free;
		pcs_b  = pcs_buf;
	}
	if (len > 0) {
		memcpy (pcs_b, buf, len);
		pcs_b += len;
		out += len;
	}
	return (out);
}

int
pcs_flush (m_info_t *mp, GFILE *Fdisk)
{
	int tail = pcs_b - pcs_buf;

	if (tail > 0) {
		memset (pcs_b, 0, IO_BSIZE-tail);
		if (g_write(Fdisk, pcs_buf, IO_BSIZE) != IO_BSIZE) {
			brlog("do_bkfdisk: g_write failed, errno=%d", errno);
			sprintf(ME(mp), "Job ID %s: write error for %s",
				mp->jobid, mp->ofsdev);
			return(1);
		}
	}
	pcs_b  = pcs_buf;

	return (tail);
}
