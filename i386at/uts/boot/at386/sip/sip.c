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

#ident	"@(#)uts-x86at:boot/at386/sip/sip.c	1.3"
#ident	"$Header: $"

#include "util/types.h"
#include "svc/bootinfo.h"
#include "util/param.h"
#include "util/sysmacros.h"
#include "mem/immu.h"
#include "io/cram/cram.h"

#include "boot/boot.h"
#include "boot/initprog.h"
#include "boot/libfm.h"

paddr_t	getfontptr();

struct  bootenv		*btep;
struct  bootfuncs       *bfp;

sip_start(binp, bfup, cmd, argp)
struct  bootenv        *binp;
struct  bootfuncs       *bfup;
int	cmd;
char	*argp;
{

	btep = binp;
	bfp = bfup;

	switch (cmd) {
		case SIP_INIT:
			sip_init();
			break;
		case SIP_KPREP:
			sip_kprep();
			break;
		case SIP_KSTART:
			sip_kstart(argp);
			break;
	}
}

sip_init()
{
	int     i;
	struct	sysenvmt	*sep;
	struct	int_pb		ic;

	sep = &btep->sysenvmt;

#ifdef BOOT_DEBUG
	if (btep->db_flag & BOOTTALK)
		printf("Begin sip_init\n");
#endif
/*	get ROM BIOS id's					*/
	memcpy(physaddr(BTEP_INFO.id), 0xfed00, sizeof(BTEP_INFO.id));

/* 	Get the ega font pointer locations. This has to be collected
 * 	after shadow ram has been turned off.
 */
	btep->sysenvmt.font_ptr[0] = getfontptr(0x0300);	/* 8 x 8  */
	btep->sysenvmt.font_ptr[1] = getfontptr(0x0200);	/* 8 x 14 */
	btep->sysenvmt.font_ptr[2] = getfontptr(0x0500);	/* 9 x 14 */
	btep->sysenvmt.font_ptr[3] = getfontptr(0x0600);	/* 8 x 16 */
	btep->sysenvmt.font_ptr[4] = getfontptr(0x0700);	/* 9 x 16 */

#ifdef BOOT_DEBUG
	if (btep->db_flag & BOOTTALK) {
		for (i=0; i<5; i++)
			printf("egafont[%d] 0x%x\n", i, btep->sysenvmt.font_ptr[i]);
		goany();
	}
#endif

	/* Gather memory size info from CMOS and BIOS ram */
	sep->CMOSmembase = CMOSreadwd(BMLOW);
	sep->CMOSmemext  = CMOSreadwd(EMLOW);
	sep->CMOSmem_hi  = CMOSreadwd(EMLOW2);
	sep->base_mem    = MEM_BASE();
	ic.intval = 0x15;
	ic.ax = 0x8800;
	doint(&ic);
	sep->sys_mem = ic.ax;

/* 	set serial and printer port addresses if any 		*/
	for ( i = 0; i < 4; i++) {
		sep->COMM_base[i] = COMM_B(i);
		sep->LPT_base[i] = LPT_B(i);
	}

}

sip_kprep()
{
	int	i;

/* 	set up the floppy and hard disk parameters for the kernel 
 *	has to be done after bus architecture has been identified
 */
	for (i = 0; i < 2; i++)	{	/* drives 0 and 1 */
		bfdparam(i);
		bhdparam(i);
	}

/* 	size memory 						*/
	if (!(BTEP_INFO.bootflags & BF_MEMAVAILSET))
		memtest();
#ifdef BOOT_DEBUG
	else if (btep->db_flag & BOOTTALK)
		printf("sip: Skipping memory test - flags %x\n",
			BTEP_INFO.bootflags);
#endif
}

sip_kstart(argp)
char	*argp;
{
	int	i;
	char	*btinfop;

/*	check for machines support shadow ram				*/
	if (btep->sysenvmt.machflags & MEM_SHADOW) {
/*
 *		re-get ega font pointer locations after shadow ram 
 *		was disable.
 */
		btep->sysenvmt.font_ptr[0] = getfontptr(0x0300);/* 8 x 8  */
		btep->sysenvmt.font_ptr[1] = getfontptr(0x0200);/* 8 x 14 */
		btep->sysenvmt.font_ptr[2] = getfontptr(0x0500);/* 9 x 14 */
		btep->sysenvmt.font_ptr[3] = getfontptr(0x0600);/* 8 x 16 */
		btep->sysenvmt.font_ptr[4] = getfontptr(0x0700);/* 9 x 16 */

#ifdef BOOT_DEBUG
	if (btep->db_flag & BOOTTALK) {
		for (i=0; i<5; i++)
			printf("egafont[%d] 0x%x\n", i, btep->sysenvmt.font_ptr[i]);
		goany();
	}
#endif
		tst_shdram();
	}

/* 	Now mark the bootstrap space available 				*/
	BTEP_INFO.memused[0].extent = RESERVED_SIZE;
	for (i=0; i < BTEP_INFO.memavailcnt; i++) 
		BTEP_INFO.memavail[i].flags &= ~B_MEM_BOOTSTRAP;

#ifdef BOOT_DEBUG
	if (btep->db_flag & BOOTTALK) {
		shomem(0,"avail final",BTEP_INFO.memavailcnt,BTEP_INFO.memavail);
		shomem(1,"used final",BTEP_INFO.memusedcnt, BTEP_INFO.memused);
	}
#endif

/* 	calculate bootinfo checksum					*/
	BTEP_INFO.checksum = 0;
	btinfop = (char *)&BTEP_INFO;
	for ( i = 0; i < (sizeof(struct bootinfo) - sizeof(int)); i++ )
		BTEP_INFO.checksum += btinfop[i];

/*	Start up the bridge manager					*/
	brgmgr(argp);
}


/*
 *	Fill in the floppy disk drive parameters for the selected drive.
 */
#ifdef BOOT_DEBUG
char	*fd_type[8] = {
		"None",
		"360KB",
		"1.2MB",
		"720KB",
		"1.44MB",
		"Unknown",
		"Unknown",
		"Unknown"
	};
#endif

bfdparam(drv)
int drv;
{
	register struct	b_fdparam	*fdp = &btep->sysenvmt.fdparamt[drv];
	register struct	fdpt	*fdpp;
	struct	int_pb	ic;

	fdp->fdp_type = CMOSread(DDTB);
	if (drv)
		fdp->fdp_type &= 0x0F;
	else
		fdp->fdp_type = (fdp->fdp_type >> 4 ) & 0x0F;

	if (fdp->fdp_type ) { /* if drive present */
		if (drv) {	/* for drive one */
			ic.intval = 0x13;
			ic.ax = 0x800;		/* get drive params thru BIOS */
			ic.dx = drv;
			if (doint(&ic))
				return(-1);
			fdpp = (struct fdpt *)segoftop(ic.es, ic.dx);
		}
		else /* For drive 0 use disk base table */
			fdpp = (struct fdpt *) ftop(FDBvect);

		fdp->fdp_ncyl    = fdpp->mxtrk;
		fdp->fdp_nsect   = fdpp->spt;
		fdp->fdp_secsiz  = fdpp->secsiz;
		fdp->fdp_RWgap   = fdpp->dgap;
		fdp->fdp_FORMgap = fdpp->fgap;
		fdp->fdp_steprate= fdpp->step;
		fdp->fdp_xferate = fdpp->dtr;
#ifdef BOOT_DEBUG
		if (btep->db_flag & BOOTTALK )
		       printf("flpy %d - type %d %s, trks %d, spt %d, bps %d\n",
				drv, fdp->fdp_type, fd_type[fdp->fdp_type],
				fdp->fdp_ncyl + 1, fdp->fdp_nsect,
				0x80 << fdp->fdp_secsiz);
#endif
	}
}


/*
 *	Fill in the hard disk drive parameters for the selected drive.
 */

bhdparam(drv)
int drv;
{
	register struct	hdparam	*hdp = &btep->sysenvmt.hdparamt[drv];
	register struct	hdpt	*hdpp;
	struct 	hdparams *bhdp = &BTEP_INFO.hdparams[drv];
	struct	int_pb	ic;
	ulong	temp;

	if (btep->sysenvmt.machflags & MC_BUS)
		hdp->hdp_type = (CMOSread( (drv ? MC_FD1TB : MC_FD0TB)));
	else {
		hdp->hdp_type = (CMOSread(FDTB) & (HINBL >> (4 * drv)));
		/* if CMOS byte 12 is F0 for drive 0 or 0F for drive 1  */
		/* OR in the extended byte value at 19 or 1A		*/
		if (hdp->hdp_type == (HINBL >> (4 * drv)))
			hdp->hdp_type |= (CMOSread(DCEB+drv) << 8);
	}

	/***
	** Since the type byte is 0, indicating no drive,
	** let's NULL out the parameters.
	***/
	if ((btep->sysenvmt.machflags & AT_BUS) && (hdp->hdp_type == 0)) {
		hdpp = (struct hdpt *)ftop(temp);
		hdp->hdp_ncyl    = 0;
		hdp->hdp_nhead   = 0;
		hdp->hdp_nsect   = 0;
		hdp->hdp_precomp = 0;
		hdp->hdp_lz      = 0;
	}
	else {
		/***
		** We have a drive byte, thus a valid drive
		** defined in CMOS, let's load up the parameters.
		***/
		temp = (ulong)*(drv ? HD1p : HD0p);
		hdpp = (struct hdpt *)ftop(temp);
		hdp->hdp_ncyl    = hdpp->mxcyl;
		hdp->hdp_nhead   = hdpp->mxhead;
		hdp->hdp_nsect   = hdpp->spt;
		hdp->hdp_precomp = hdpp->precomp;
		hdp->hdp_lz      = hdpp->lz;
	}

	if ((btep->sysenvmt.machflags & AT_BUS) && !(hdp->hdp_type&0xff))
		bhdp->hdp_ncyl	= 0;
	else {
		bhdp->hdp_ncyl	= hdp->hdp_ncyl;
		bhdp->hdp_nhead	= hdp->hdp_nhead;
		bhdp->hdp_nsect	= hdp->hdp_nsect;
		bhdp->hdp_precomp = hdp->hdp_precomp;
		bhdp->hdp_lz	= hdp->hdp_lz;
	}

#ifdef BOOT_DEBUG
	if (btep->db_flag & BOOTTALK )
		printf("disk %d - ncyl 0x%x, heads 0x%x, spt 0x%x CMOStype= 0x%x\n",
			drv, hdp->hdp_ncyl, hdp->hdp_nhead, 
			hdp->hdp_nsect, hdp->hdp_type);
#endif

	return(0);
}


paddr_t
getfontptr(which)
unsigned short which;
{
	struct	int_pb		ic;

	ic.intval = 0x10;
	ic.ax = 0x1130;
	ic.bx = which;
	ic.cx = ic.dx = ic.es = ic.bp = 0;
	if (doint(&ic)) {
		printf("doint error getting font pointers, may be trouble\n");
		return(0);
	}
	else
		return(phystokv( (uint)((ic.es) << 4) + ic.bp));
}

