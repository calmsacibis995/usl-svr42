/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#ident	"@(#)uts-x86at:boot/at386/disk.c	1.4"
#ident	"$Header: $"
#ident "@(#) (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990"

/*
 *	disk.c - disk driver for the boot program.
 *	         this is the only module in the boot program compiled with 
 *		 ifdef WINI to separate between hard disk and floppy disk boot.
 */
#include "util/types.h"
#include "svc/bootinfo.h"
#include "util/param.h"
#include "util/sysmacros.h"
#include "io/vtoc.h"
#include "io/target/alttbl.h"
#include "io/target/fdisk.h"
#include "fs/vnode.h"
#include "fs/vfs.h"
#include "fs/bfs/bfs.h"

#include "boot/boot.h"
#include "boot/bootlink.h"
#include "boot/s51kconf.h"
#include "boot/bfsconf.h"
#include "boot/libfm.h"

#ifdef WINI
ushort	bootdriv = BOOTDRV_HD;	/* boot drive indicator		*/
struct	ipart	*actpart_ptr;	/* pointer to active partition	*/
#else
ushort	bootdriv = BOOTDRV_FP;
#endif

struct	gcache gcache;		/* disk cache header		*/
short	fd_adapt_lev;

extern	int	bfs_rdblk_cnt;
extern 	char	gbuf[];
extern 	paddr_t	act_part;	/* address of active partition 	*/
extern 	paddr_t	entry_ds_si;	/* ds:si bios entry pointer	*/
extern	short	bps;		/* bytes per sector 		*/
extern	short	spt;		/* disk sectors per track 	*/
extern	short	spc;		/* disk sectors per cylinder 	*/
extern 	struct 	bootenv bootenv;


/*
 * the following must be set up for use by the high level filesystem 
 * routines
 */

off_t	boot_delta;		/* sector offset to filesystem 	*/
off_t	root_delta;		/* sector offset to filesystem 	*/
bfstyp_t boot_fs_type = UNKNOWN;/* boot file system type	*/
bfstyp_t root_fs_type = UNKNOWN;/* root file system type	*/
char	buf[512];

/*
 * get_fs: 	initialize the driver; open the disk, find
 *		the root slice and fill in the global 
 *		variables and tables.
 */

#ifdef WINI
#ifdef HDTST
char	partbuf[512];
#endif

get_fs()
{
	extern	bd_getalts();
	struct pdinfo	*pip;
	struct vtoc	*vtp;
	struct mboot 	*mb;
	struct ipart	*ip;
	daddr_t		secno;
	int		i;
	int		partid = 0;
	int		rootfs;
	int		bootfs;

#ifdef HDTST
	struct		int_pb ic;

/*	The HDTST is used to allow the system to boot up from
 *	the hard disk without installing the boot program onto
 * 	the hard disk. Instead, the boot program is a combination 
 *	of the floppy primary boot and the hard disk boot programs.
 *	This speeds up the testing of hard disk boot by installing
 *	this mixed boot program in the floppy, and starting the
 *	machine up from floppy.
 */
	ic.ax = 0x0800;
	ic.dx = bootdriv;
	ic.intval = 0x13;
	if (doint(&ic)) {
		printf("hd:get_fs: Can't get hard disk drive parameters\n");
		bootabort();
	}
	spt = ic.cx &0x3f;
	spc = (((ushort)(ic.dx & 0xff00) >> 8) + 1) * spt;
	printf("hd:get_fs: bps= 0x%x spt=0x%x spc=0x%x\n", bps, spt,spc);

	disk( 0, physaddr(partbuf), (short)1 );
	actpart_ptr = (struct ipart *)&(partbuf[BOOTSZ]);
	for (i=0; i<FD_NUMPART; i++, actpart_ptr++) {
		if (actpart_ptr->bootid == B_ACTIVE)
			break;
	}
	if (i< FD_NUMPART) {
		act_part = (ulong) actpart_ptr;
		entry_ds_si = (ulong) actpart_ptr;
	} else {
		printf("disk: unable to locate active boot partition\n");
		bootabort();
	}
	printf("fdisk info: bootid= 0x%x systid=0x%x relsect= 0x%x begcyl= 0x%x\n",
	actpart_ptr->bootid, actpart_ptr->systid, actpart_ptr->relsect,
	actpart_ptr->begcyl);
#endif

/* 	setup the pointer to the active partition 			*/
	if (entry_ds_si)
		actpart_ptr = (struct ipart *)entry_ds_si;
	else
		actpart_ptr = (struct ipart *)act_part;

/* 	read in pdinfo 							*/
	secno = actpart_ptr->relsect + HDPDLOC;
#ifdef BOOT_DEBUG
	if (bootenv.db_flag & BOOTDBG)
		printf("\nReading disk info (pdinfo) from sector %ld\n",secno);
#endif
	disk( secno, physaddr(buf), (short)1 );
	pip = (struct pdinfo *)buf;

/*
 * 	look in vtoc to find start of stand|root partition
 */
	rootfs=-1;
	bootfs=-1;
	vtp = (struct vtoc *)&buf[pip->vtoc_ptr % bps];
	for (i = 0; i < (int)vtp->v_nparts; i++) {
		if (vtp->v_part[i].p_tag == V_STAND) 
		{
			bootfs = i;	
			boot_fs_type = BFS;
			boot_delta = (off_t) vtp->v_part[i].p_start;	/* start of fs 	*/
		}
		if (vtp->v_part[i].p_tag == V_ROOT)
		{
			rootfs = i;	
			root_delta = (off_t) vtp->v_part[i].p_start;	/* start of fs 	*/
		}
	}

	if ((rootfs == -1) && (bootfs == -1)) {
		printf("\nboot: No file system to boot from.\n");
		bootabort();
	}

/* 	read in alternate sector table 					*/
	bd_getalts(vtp, pip);
	BTE_INFO.bootflags &= ~BF_FLOPPY;

	if (boot_fs_type == BFS) {
		bfs_rdblk_cnt = BLK_GBUF;
		bfsinit();
	}

#ifdef BOOT_DEBUG
	if (bootenv.db_flag & BOOTDBG)
		printf("boot filesystem start sector %ld\n",boot_delta);
#endif
	return(root_delta);
}
#else	/* WINI */
get_fs()
{
	struct	bdsuper *bdsup;

/*
	BTE_INFO.bootflags |= BF_MFLOP_BOOT | BF_FLOPPY;
	to make boot prompts for the second floppy.
*/
	BTE_INFO.bootflags |= BF_FLOPPY;

/*	set start of filesystem at the start of the second cylinder	*/
	root_delta = (off_t) spc;
/* 	select minor number for boot device 				*/
	switch (spt) {
	default:	/* 5.25" disks, usually 15 spt */
		bootenv.bf_minor = 4;
		break;
	case 18:
		bootenv.bf_minor = 84;
		break;
	case 9:
/*
 *		for 3.5", 720K ds/ds floppy drive		
 *		reset start of filesystem at the third cylinder		
 */
		root_delta = (off_t) (2 * spc);
		bootenv.bf_minor = 100;
		break;
	}

/*	determine the filesystem type					*/
	disk(root_delta, physaddr(buf), (short)1 );

/*	get first sector at the beginning of fs
 *	and check for bfs magic number
 */
	bdsup = (struct bdsuper *)buf;
	if (bdsup->bdsup_bfsmagic == BFS_MAGIC) {
		boot_fs_type = BFS;

/*		setup bfs logical block read count to be 1 track long	
 *		check for maximum blocks available in global disk buf
 */
		bfs_rdblk_cnt = (spt * bps / BFS_BSIZE);
		if (bfs_rdblk_cnt > BLK_GBUF)
			bfs_rdblk_cnt = BLK_GBUF;

		bfsinit();
	} else 
		boot_fs_type = s5;

	return(root_delta);
}


#endif /* WINI */


/*
 * dread(): 	convert block number to sector number.
 *		read the given number of blocks into the global buffer.
 *		A guaranteed full-track read algorithm is applied for
 *		floppy read to increase the performance.
 */

dread(bno, bnocnt)
daddr_t	bno;
ulong	bnocnt;
{
	register int	offset;
	daddr_t		secno;
	int		tcount;
	ulong		totcnt;
	ulong		secpbno;
	int		xfer_bnocnt = bnocnt;

/*	return if the begin block number and the amount of cached data
 *	covers the requested amount
 */
	if ((bno == gcache.gc_bno) && (bnocnt <= gcache.gc_cnt))
		return(xfer_bnocnt);

#ifdef BOOT_DEBUG2
	if (bootenv.db_flag & LOADDBG)
		printf(" dread: bno= 0x%x bnocnt= 0x%x ",
			bno, bnocnt);
#endif

/*	xlate block # to sector number				*/
	if (boot_fs_type == s5)
		secpbno = BSIZE / NBPSCTR;
	else
		secpbno = BFS_BSIZE / NBPSCTR;
	totcnt = bnocnt * secpbno;
	secno  = bno * secpbno + (ulong) boot_delta;

#ifdef WINI
/*	read what has been requested 
 *	neglect any cached data since the hard disk is a faster device
 */
	bd_rd_sec(secno,totcnt);
#else /* WINI */
/*	deduct the total blocks that have been cache			*/
	if (bno == gcache.gc_bno) {
		tcount  = gcache.gc_cnt * secpbno;
		secno  = secno + tcount;
		offset = tcount * NBPSCTR;
		totcnt = (bnocnt - gcache.gc_cnt) * secpbno;
	} else 
		offset = 0;

/*	limit read count if not starting at track boundary
 *	ensure subsequent reads to fall at track boundary
 *	-- apply only to filesystem with buffer size that can adjust
 *	   to end at the track boundary
 */
	tcount = spt - (secno % spt);
	if (!(spt % secpbno) && !(tcount % secpbno)) {
/*		if NOT start at track boundary and spanning cross tracks*/
		if ((tcount != spt) && (tcount < totcnt)) {
			totcnt = tcount;	
/*			modify the requested transferred block count	*/
			xfer_bnocnt = totcnt / secpbno;
			if (bno == gcache.gc_bno)
				xfer_bnocnt += gcache.gc_cnt;
		}
	}

/*	sector transfer count - tcount has been calculated		*/
	for (;;) {
		if (tcount > totcnt)
			tcount = totcnt;
		disk(secno, physaddr(&gbuf[offset]), (short) tcount);
		if ((totcnt -= tcount) == 0)
			break;
		secno += tcount;
		offset += tcount * NBPSCTR;
		tcount = spt - (secno % spt);
	}

#endif /* WINI */

/*	update cache record					*/
	gcache.gc_bno = bno;
	gcache.gc_cnt = xfer_bnocnt;
	return(xfer_bnocnt);	
}



