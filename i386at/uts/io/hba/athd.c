/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/hba/athd.c	1.21"
#ident	"$Header: miked 1/23/92 $"

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1989 INTERACTIVE Systems Corporation
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

/*
 * IBM PC AT Hard Disk Low-Level Controller Interface Routines.
 * For use with Generic Disk Driver.  This file supports the stock AT
 * controller (and clones), the Adaptec RLL and ESDI controllers, and
 * the Western Digital WD1005-WAH ESDI controller, as either primary or
 * secondary controller.
 */

#include <util/param.h>
#include <util/types.h>
#include <mem/kmem.h>
#include <fs/buf.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/sysenvmt.h>
#include <io/iobuf.h>
#include <util/cmn_err.h>
#include <io/hba/athd.h>
#include <io/target/alttbl.h>
#include <io/vtoc.h>
#include <io/target/sdi_edt.h>
#include <io/target/sdi.h>
#include <io/hba/dcd.h>
#include <io/target/scsi.h>
#include <io/hba/gendev.h>
#include <io/ddi.h>
#include <util/mod/moddefs.h>

MOD_MISC_WRAPPER(athd_, NULL, NULL, "athd - loadable DCD driver");

/*
 * The following table is used to translate the bits in the controller's
 * 'error' register (AT_ERROR) to Generic error codes.  It is indexed by
 * the bit number of the first bit to be found ON in the register (or 8
 * if NO bits are found to be on).
 */

static ushort xlaterr [9] =
{
	DERR_DAMNF,
	DERR_TRK00,
	DERR_ABORT,
	DERR_UNKNOWN,
	DERR_SECNOTFND,
	DERR_UNKNOWN,
	DERR_DATABAD,
	DERR_BADMARK,
	DERR_UNKNOWN
};
/* reset_count is used to monitor the number of consecutive resets */
int	reset_count = 0;

/* local timeout routine */
void athdtimeout();

/*
 * athd_bdinit -- Initialize AT-class hard disk controller.
 *                We return the number of drives which exist on that
 *                controller (or 0 if controller doesn't respond)...
 */

int
athd_bdinit(cfgp,dcbp)
register struct gdev_cfg_entry *cfgp;
gdev_dcbp dcbp;
{
	register int i,drives = 0;

	/***
	** If we are running on a micro-channel platform,
	** then skip the AT Class HDC Initialization.
	***/
	if (sysenvmtp->machflags & MC_BUS) {
		return(0);
	}

	/***
	** We differentiate between controllers configured at the Primary
	** and Secondary IOA in order to avoid a controller reset if
	** possible. The DPT 2012/A responds to a reset on the Primary
	** IOA even though it is configured on Secondary IOA. When this
	** happens the DPT HBA Driver will panic on a 2011 during it's
	** initialization. Thus we attempt to avoid the reset if at all
	** possible. Our primary concern is on the boot floppy when all
	** the HBA init's are run.
	***/
	if( dcbp->dcb_ioaddr1 == ATHD_PRIMEADDR ) {

		/***
		** If the CMOS has bad, NULL, values for heads,
		** sectors or cylinders, fail the drive.
		***/
		if( sysenvmtp->hdparamt[0].hdp_ncyl == 0 ||
		    sysenvmtp->hdparamt[0].hdp_nhead == 0 ||
		    sysenvmtp->hdparamt[0].hdp_nsect == 0 ) {
			return( 0 );
		}

		/***
		** Issue Drive Select 0 Command to verify the drive exists.
		***/
		outb(cfgp->cfg_ioaddr1+AT_DRVHD, ATDH_DRIVE0);
		drv_usecwait( 10 );

		/***
		** See if Drive 0 responded.
		***/
		if (!athd_wait(cfgp->cfg_ioaddr1,
		    (ATS_BUSY|ATS_READY|ATS_SEEKDONE|ATS_ERROR),
		    (ATS_READY|ATS_SEEKDONE), (ATS_BUSY|ATS_ERROR)) ) {
			/***
			** DCD Drive 0 did not respond, now we'll have
			** to attempt a controller reset to verify if the
			** drive is really present. The side effect is that
			** if a DPT 2012/A is present in the BUS, this reset
			** will cause the DPT init panic 8-(. The good news
			** is that the CMOS indicates the system is booting
			** DCD, so at best the system may have a DPT 2012/A
			** as a secondary controller. This is a real slim
			** possibility, but if it is, we're setting up
			** for a panic.
			***/
			outb(cfgp->cfg_ioaddr2,AT_CTLRESET);
			drv_usecwait( 20 );
			outb(cfgp->cfg_ioaddr2,AT_INTRDISAB);
			outb((cfgp->cfg_ioaddr2 | 0x80),AT_INTRDISAB);

			if (athd_wait(cfgp->cfg_ioaddr1,
			    (ATS_BUSY|ATS_READY|ATS_SEEKDONE|ATS_ERROR),
			    (ATS_READY|ATS_SEEKDONE), (ATS_BUSY|ATS_ERROR))) {

				/***
				** The DCD Disk 0 did not respond even after
				** the reset. The most probable cause is
				** an incorrectly configured machine.
				***/
				return( 0 );
			}
			else {
				/***
				** The DCD Disk 0 responded after the reset,
				** thus we have at least 1 DCD Disk.
				***/
				drives = 1;
			}
		}
		else {
			/***
			** The DCD Disk 0 responded to the Drive Select,
			** thus we have at least one DCD Disk.
			***/
			drives = 1;
		}

		if (cfgp->cfg_drives == 1) {
			/***
			** According to the configuration information
			** we are only using 1 drive on this controller.
			** Mark CCAP_NOSEEK since we do not perform
			** explicite seeks on single drive configuration.
			***/
			cfgp->cfg_capab |= CCAP_NOSEEK;
			timeout(athdtimeout, (caddr_t)dcbp, (30 * HZ));
			return (drives);
		}

		/***
		** Let's see if we have a second disk
		***/

		/***
		** If the CMOS has bad, NULL, values for heads,
		** sectors or cylinders, fail the drive.
		***/
		if( sysenvmtp->hdparamt[1].hdp_ncyl != 0 &&
		    sysenvmtp->hdparamt[1].hdp_nhead != 0 &&
		    sysenvmtp->hdparamt[1].hdp_nsect != 0 ) {

			/***
			** Issue Drive 1 Select Command to verify the
			** drive exists.
			***/
			outb(cfgp->cfg_ioaddr1+AT_DRVHD, ATDH_DRIVE1);
			drv_usecwait( 10 );

			if (!athd_wait(cfgp->cfg_ioaddr1,
			    (ATS_BUSY|ATS_READY|ATS_SEEKDONE|ATS_ERROR),
			    (ATS_READY|ATS_SEEKDONE), (ATS_BUSY|ATS_ERROR)) ) {

				/***
				** DCD Disk Drive 1 responded to the Drive
				** Select, thus we have another drive.
				***/
				drives++;
			}
			else {
				/***
				** Since only the first drive responded, we
				** have a single drive configuration. Mark
				** CCAP_NOSEEK since we do not perform
				** explicite seeks on single drive
				** configurations.
				***/
				cfgp->cfg_capab |= CCAP_NOSEEK;
			}
		}

		timeout(athdtimeout, (caddr_t)dcbp, (30 * HZ));

		/***
		** See if this is a DPT 3011 controller with drives.
		** Note that we must do these checks first since a
		** DPT controller will see two drives even when there
		** are none attached with the previous tests.
		***/
		if ( (i=athd_DPTinit(cfgp,dcbp)) < 0 ) {
			/***
			** Not on a DPT 3011, use the number we calculated.
			***/
			return(drives);
		} else {
			/***
			** We are on a DPT 3011, use the drive count that
			** the DPT style code found.
			***/
			return(i);
		}
	}
	else {
		/***
		** We are initializing a secondary DCD Disk Controller,
		** this excludes support for a DPT 2012/A in this system
		** do to the reset being done. See above for more information.
		**/
		outb(cfgp->cfg_ioaddr2,AT_CTLRESET);
		drv_usecwait( 20 );
		outb(cfgp->cfg_ioaddr2,AT_INTRDISAB);

		/*
		 * The outb to the address "or"ed with 0x80 below is a HACK!!!
		 * On Adaptec controllers set up for the secondary position,
		 * once you reset them, they forget that they're supposed
		 * to respond to secondary addresses until you un-do the
		 * reset!!! Thus, you have to set the reset bit in port
		 * 0x376 and clear it in port 0x3F6!!! Gross, but this
		 * shouldn't hurt, as long as we're still in initialization.
		 */
		outb((cfgp->cfg_ioaddr2 | 0x80),AT_INTRDISAB);

		if (athd_wait(cfgp->cfg_ioaddr1,
		    (ATS_BUSY|ATS_READY|ATS_SEEKDONE|ATS_ERROR),
		    (ATS_READY|ATS_SEEKDONE), (ATS_BUSY|ATS_ERROR))) {
			return (drives);
		}

		/***
		** At this point we have a controller and at least one drive.
		***/
		drives = 1;
		if (cfgp->cfg_drives == 1) {
			/***
			** According to the configuration information
			** we are only using 1 drive on this controller.
			** Mark CCAP_NOSEEK since we do not perform
			** explicite seeks on single drive configuration.
			***/
			cfgp->cfg_capab |= CCAP_NOSEEK;
			timeout(athdtimeout, (caddr_t)dcbp, (30 * HZ));
			return (drives);
		}

		/***
		** Let's see if we have a second disk
		***/
		drv_usecwait( cfgp->cfg_delay*100 );
		outb(cfgp->cfg_ioaddr1+AT_DRVHD, ATDH_DRIVE1);
		drv_usecwait( 10 );

		if (!athd_wait(cfgp->cfg_ioaddr1,
		    (ATS_BUSY|ATS_READY|ATS_SEEKDONE|ATS_ERROR),
		    (ATS_READY|ATS_SEEKDONE), (ATS_BUSY|ATS_ERROR)) ) {
			/***
			** The disk is there.
			***/
			drives++;
		}
		else {
			/***
			** Since only the first drive responded, we have
			** a single drive configuration. Mark CCAP_NOSEEK
			** since we do not perform explicite seeks on
			** single drive configuration.
			***/
			cfgp->cfg_capab |= CCAP_NOSEEK;
		}
		timeout(athdtimeout, (caddr_t)dcbp, (30 * HZ));

		/***
		** See if this is a DPT type controller with drives.
		** Note that we must do these checks first since a
		** DPT controller will see two drives even when there
		** are none with the tests below
		***/
		if ( (i=athd_DPTinit(cfgp,dcbp)) < 0 ) {
			/***
			** Not a DPT, use the number we calculated.
			***/
			return(drives);
		} else {
			/***
			** We are on a DPT, use the drive count that
			** the DPT style code found.
			***/
			return(i);
		}
	}
}

/*
 * We must use special commands to check for drives on a DPT type
 * controller. This must be done since the code in athd_bdinit()
 * will detect two drives on a DPT even with none connected. The
 * cmds in this routine will only work on a DPT controllers.
 * If this is a DPT type controller then return # of drives else -1.
 */
int
athd_DPTinit(cfgp,dcbp)
register struct gdev_cfg_entry *cfgp;
gdev_dcbp dcbp;
{
	register int i, DPTerror=0;
	unchar  laststat,tmpbuf[32];

	/* First we will use a DPT INQUIRY cmnd to make shure this is a DPT*/
	for(i=cfgp->cfg_delay*10; i!=0; i--) tenmicrosec();	 
	outb(cfgp->cfg_ioaddr1+AT_DRVHD, 0x00);   /* indicate 1st drive */
	outb(cfgp->cfg_ioaddr1+AT_HCYL, 0x0);
	outb(cfgp->cfg_ioaddr1+AT_LCYL, 0x0);
	outb(cfgp->cfg_ioaddr1+AT_SECT, 0x10); /* amount of data we expect */
	outb(cfgp->cfg_ioaddr1+AT_COUNT, 0x0);
	outb(cfgp->cfg_ioaddr1+AT_CMD, 0xd2); /* isu a inquiry cmd */
	for(i=cfgp->cfg_delay*10; i!=0; i--) tenmicrosec();	 

	/* Wait for it to complete */
	/* Wait for DRQ or error status */
	for (i=0; i<100; i++) {
		laststat = inb(cfgp->cfg_ioaddr1+AT_STATUS);
		if ( (laststat & (ATS_DATARQ|ATS_ERROR)) != 0)
			break;
		tenmicrosec();
	}

	/* We took too long or got an error so this must not be DPT*/
	if (i==100 || laststat & ATS_ERROR) {
		DPTerror++;
		goto dptdone;
	}
	linw(cfgp->cfg_ioaddr1+AT_DATA,&tmpbuf, 16);

	/* Now check that this is really a DPT vender="DPT" */
	if (tmpbuf[8] != 'D' || tmpbuf[9] != 'P' || tmpbuf[10] != 'T') {
		DPTerror++;
		goto dptdone;
	}

	/* Yes this is really a DPT so now on with drive tests */
	/* Try DPT type Test Unit ready for second drive */
	tenmicrosec();   /* kill some time */
	outb(cfgp->cfg_ioaddr1+AT_DRVHD, 0x20);   /* indicate second drive */
	outb(cfgp->cfg_ioaddr1+AT_HCYL, 0x0);
	outb(cfgp->cfg_ioaddr1+AT_LCYL, 0x0);
	outb(cfgp->cfg_ioaddr1+AT_SECT, 0x0);
	outb(cfgp->cfg_ioaddr1+AT_COUNT, 0x0);
	outb(cfgp->cfg_ioaddr1+AT_CMD, 0xc0); /* isu a test unit ready cmd */
	for(i=cfgp->cfg_delay; i!=0; i--) tenmicrosec();	 

	if (!athd_wait(cfgp->cfg_ioaddr1,
	    (ATS_BUSY|ATS_READY|ATS_SEEKDONE|ATS_ERROR),
	    (ATS_READY|ATS_SEEKDONE),
	    (ATS_BUSY|ATS_ERROR)))
	{       /* Have second drive with good status */
		return (2);
	}

	/* look for first drive DPT way  since no second drive*/
	tenmicrosec();   /* kill some time */
	outb(cfgp->cfg_ioaddr1+AT_DRVHD, 0x0);   /* indicate first drive */
	outb(cfgp->cfg_ioaddr1+AT_HCYL, 0x0);
	outb(cfgp->cfg_ioaddr1+AT_LCYL, 0x0);
	outb(cfgp->cfg_ioaddr1+AT_SECT, 0x0);
	outb(cfgp->cfg_ioaddr1+AT_COUNT, 0x0);
	outb(cfgp->cfg_ioaddr1+AT_CMD, 0xc0); /* isu a test unit ready cmd */
	for(i=cfgp->cfg_delay; i!=0; i--) tenmicrosec();	 

	if (!athd_wait(cfgp->cfg_ioaddr1,
	    (ATS_BUSY|ATS_READY|ATS_SEEKDONE|ATS_ERROR),
	    (ATS_READY|ATS_SEEKDONE),
	    (ATS_BUSY|ATS_ERROR)))
	{       /* Have first drive with good status */
		cfgp->cfg_capab |= CCAP_NOSEEK; /* don't issue seeks on 1-drive board */
		return (1);
	}

	/* this must  be a DPT with no drive reset it for normal use */
dptdone:
	outb(cfgp->cfg_ioaddr2,AT_CTLRESET);    /* Reset controller */
	tenmicrosec();                          /* Allow it to notice reset */
	tenmicrosec();
	outb(cfgp->cfg_ioaddr2,AT_INTRDISAB);   /* Turn off interrupts */
	/*
	 * The outb to the address "or"ed with 0x80 below is dirty. On
	 * Adaptec controllers set up for the secondary position, once you reset
	 * them, they forget that they're supposed to respond to secondary 
	 * addresses until you un-do the reset. Thus, you have to set the reset
	 * bit in port 0x376 and clear it in port 0x3F6. But this shouldn't
	 * hurt, as long as we're still in initialization.
	 */
	outb((cfgp->cfg_ioaddr2 | 0x80),AT_INTRDISAB);   /* Turn off interrupts */

	if (DPTerror)
		return (-1);	/* tell the world this is not DPT */
	else
		return (0);     /* tell the world we have no drives */
}


/*
 * athd_drvinit -- Initialize drive on controller.
 *                 Sets values in dpb for drive (based on info from controller
 *                 or ROM BIOS or defaults).
 */
athd_drvinit(dcbp,dpbp)
register gdev_dcbp dcbp;
register gdev_dpbp dpbp;
{
	struct admsbuf ab;
	struct wdrpbuf wb;
	unchar laststat;
	int i;
	int current_drive;


	/* set up some defaults in the dpb... */
	dpbp->dpb_flags = (DFLG_RETRY | DFLG_ERRCORR | DFLG_VTOCSUP | DFLG_FIXREC);
	dpbp->dpb_drvflags = (ATF_FMTRST | ATF_RESTORE);
	dpbp->dpb_devtype = DTYP_DISK;

	/* Still initializing, make drivesel timeout big... */
	dcbp->dcb_delay *= 10;
	athd_drivesel(dcbp);    /* select the proper drive */
	/* Switch it back. */
	dcbp->dcb_delay /= 10;

	/* Establish defaults for disk geometry */
	dpbp->dpb_secovhd = 81;
	dpbp->dpb_ressecs = 0;
	dpbp->dpb_pbytes = 512;
	dpbp->dpb_drvflags |= (DPCF_CHGCYLS | DPCF_CHGHEADS | DPCF_CHGSECTS);
	dpbp->dpb_inqdata.id_type = 0;
	dpbp->dpb_inqdata.id_qualif = 0;
	dpbp->dpb_inqdata.id_rmb = 0;
	dpbp->dpb_inqdata.id_ver = 0x1;		/* scsi 1 */
	dpbp->dpb_inqdata.id_len = 31;
	strncpy(dpbp->dpb_inqdata.id_vendor, "(athd)  ", 8);
	strncpy(dpbp->dpb_inqdata.id_prod, "ISA Disk Drive  ", 16);
	strncpy(dpbp->dpb_inqdata.id_revnum, "1.00", 4);
	current_drive = (dcb_curdriv(dcbp) == 0 ? 0 : 1);

	if (dcbp->dcb_ioaddr1 == ATHD_PRIMEADDR)
	{       /* Get default BIOS parameters for primary drive */
		dpbp->dpb_cyls = sysenvmtp->hdparamt[current_drive].hdp_ncyl;
		dpbp->dpb_heads = sysenvmtp->hdparamt[current_drive].hdp_nhead;
		dpbp->dpb_sectors = sysenvmtp->hdparamt[current_drive].hdp_nsect;
		if (dpbp->dpb_heads > 16) {
			cmn_err(CE_WARN,"Disk Driver: The hard disk parameters are not valid. The hard disk");
			cmn_err(CE_CONT,"is defined with %d heads. The maximum valid number is 16. You must re-run\n", dpbp->dpb_heads);
			cmn_err(CE_CONT,"your setup utilities to chose a valid disk type parameter. Please\n");
			cmn_err(CE_CONT,"refer to the installation guide for more information on this problem.\n\n");
			cmn_err(CE_CONT,"Disk Driver: Now halting the system due to the problem.");
			oemreboot(0);

		}
	}
	else {       /* Pick something that will let everything be discovered */
		dpbp->dpb_cyls = 1024;
		dpbp->dpb_heads = 16;   /* updated at open time, if possible */
		dpbp->dpb_sectors = 255; /* updated at open time, if possible */
	}
	outb(dcbp->dcb_ioaddr2,((dpbp->dpb_heads > 8 ? AT_EXTRAHDS
	    : AT_NOEXTRAHDS) |
	    AT_INTRDISAB)); /* no interrupt on this */

	/*  include fix for dtc controller here !  */
	outb(dcbp->dcb_ioaddr1+AT_DRVHD, current_drive == 0 ? ATDH_DRIVE0 : ATDH_DRIVE1);
	outb(dcbp->dcb_ioaddr1+AT_HCYL, 0x0);
	outb(dcbp->dcb_ioaddr1+AD_SPCLCMD,ADSC_MODESENSE);
	outb(dcbp->dcb_ioaddr1+AT_CMD,ADC_SPECIAL);  /* try modesense command */
	tenmicrosec();
	laststat = athd_wait(dcbp->dcb_ioaddr1,ATS_BUSY,0,ATS_BUSY);

	/* Wait for DRQ or error status */

	for (i=0; i<10000; i++) {
		if (((laststat=inb(dcbp->dcb_ioaddr1+AT_STATUS)) &
		    (ATS_DATARQ|ATS_ERROR)) != 0)
			break;
		if (((laststat & (ATS_READY|ATS_SEEKDONE)) && (~laststat & ATS_BUSY))  != 0)
			break;
		tenmicrosec();
	}
	if (i==10000)
		cmn_err(CE_NOTE,"!athd_drvinit modesense never got DRQ or error, got 0x%x\n",laststat);

	/* See if error or never got DRQ */
	if ((laststat & ATS_ERROR) || !(laststat & (ATS_DATARQ|ATS_READY))) {
	        /* Controller didn't like modesense command -- not Adaptec */
		/* Try for WD1007 Read Parameters command */
		tenmicrosec();
		tenmicrosec();
		outb(dcbp->dcb_ioaddr1+AT_CMD,AWC_READPARMS);
		laststat = athd_wait(dcbp->dcb_ioaddr1,ATS_BUSY,0,ATS_BUSY);
		/* Wait for DRQ or error status */
		for (i=0; i<10000; i++)
		{
			if (((laststat=inb(dcbp->dcb_ioaddr1+AT_STATUS)) &
			    (ATS_DATARQ|ATS_ERROR)) != 0)
				break;
			tenmicrosec();
		}
		if (i==10000)
			cmn_err(CE_NOTE,"!athd_drvinit read parameters never got DRQ or error, got 0x%x\n",laststat);

		/* See if DRQ or error */
		if (laststat & ATS_ERROR)
			return;
		/* Read Parameters worked.  Get data */
		linw(dcbp->dcb_ioaddr1+AT_DATA,&wb,WDRPBUF_LEN);
		dpbp->dpb_pcyls = wb.wdrp_fixcyls;
		dpbp->dpb_pheads = wb.wdrp_heads;
		dpbp->dpb_psectors = wb.wdrp_sectors;
		dpbp->dpb_secovhd = wb.wdrp_secsiz - dpbp->dpb_secsiz;
		/* If we aren't on the primary controller, there are no BIOS 
		 * values to use as operating values, so use the physical ones 
		 * from the controller.
		 */
		if (dcbp->dcb_ioaddr1 != ATHD_PRIMEADDR) {
			dpbp->dpb_cyls = wb.wdrp_fixcyls;
			dpbp->dpb_heads = wb.wdrp_heads;
			dpbp->dpb_sectors = wb.wdrp_sectors;
			dpbp->dpb_rescyls = 2;
		} else 
		/* Otherwise, be sure to report the true number of cylinders.  
	 	 * The values obtained from the BIOS may be wrong when we are 
	 	 * using the true geometry, and the number of cylinders
	 	 * reported by the BIOS is less than the number of physical
	 	 * ones.  We determine that we are in true geometry by
	 	 * making sure that the heads match and the sectors are close
	 	 * (to allow for slipped sectoring).
	 	 */
			if (dpbp->dpb_heads == dpbp->dpb_pheads &&
		           (!(i = dpbp->dpb_sectors - dpbp->dpb_psectors) ||
		           i == -1 || i == -2) &&
		           (dpbp->dpb_pcyls > 1023) &&
		           (dpbp->dpb_pcyls > dpbp->dpb_cyls)) {
				dpbp->dpb_cyls = dpbp->dpb_pcyls;
				dpbp->dpb_rescyls = 2;
			}
		strncpy(dpbp->dpb_inqdata.id_prod, "ESDI            ", 16);
		return;
	}
	linw(dcbp->dcb_ioaddr1+AT_DATA, &ab, 6);        /* Get modesense data */

	if (ab.adms_valid != ADMS_VALID)
	{       /* Not valid modesense return -- not Adaptec */
		return;
	}
	dpbp->dpb_secovhd = ab.adms_secovhd;
	dpbp->dpb_drvflags &= ~(DPCF_CHGHEADS | DPCF_CHGSECTS);
	dpbp->dpb_drvflags |= (ATF_ADAPTEC | DPCF_CHGCYLS);  /* adaptec cntlr */
	if ((ab.adms_flags & ADMSF_CIDMSK) == ADCID_ESDI)
	{       /* ESDI controller -- 2 reserved cyls and BADMAP avail */
		dpbp->dpb_rescyls = 2;
		dpbp->dpb_drvflags |= DPCF_BADMAP;
		strncpy(dpbp->dpb_inqdata.id_prod, "ESDI            ", 16);
	}
	else {       /* RLL controller -- 1 reserved cyl and no BADMAP */
		dpbp->dpb_rescyls = 1;
		dpbp->dpb_drvflags |= (DPCF_CHGHEADS | DPCF_CHGCYLS);
		strncpy(dpbp->dpb_inqdata.id_prod, "MFM             ", 16);
	}

	/* If we have valid modesense data, use it */
	if (ab.adms_flags & (ADMSF_VAC | ADMSF_VMS)) {
		dpbp->dpb_pcyls = ((ushort)ab.adms_cylh << 8) | ab.adms_cyll;
		dpbp->dpb_pheads = ab.adms_nhds;
		dpbp->dpb_psectors = ab.adms_nsect;
		dpbp->dpb_secovhd = ab.adms_secovhd;
		dpbp->dpb_pdsect = dpbp->dpb_sectors;

		/* If we aren't on the primary controller, there are no BIOS 
	 	 * valuesto use as operating values, so use the physical ones 
	 	 * from the controller.
	 	 */
		if (dcbp->dcb_ioaddr1 != ATHD_PRIMEADDR) {
			dpbp->dpb_cyls = dpbp->dpb_pcyls;
			dpbp->dpb_heads = dpbp->dpb_pheads;
			dpbp->dpb_sectors = dpbp->dpb_psectors;
		} else 
		/* Otherwise, be sure to report the true number of cylinders.  
	 	 * The values obtained from the BIOS may be wrong when we are 
	 	 * using the true geometry, and the number of cylinders
	 	 * reported by the BIOS is less than the number of physical
	 	 * ones.  We determine that we are in true geometry by
	 	 * making sure that the heads match and the sectors are close
	 	 * (to allow for slipped sectoring).
	 	 */
			if (dpbp->dpb_heads == dpbp->dpb_pheads &&
		    	   ( !(i = dpbp->dpb_sectors - dpbp->dpb_psectors) ||
		    	   i == -1 || i == -2) &&
		    	   (dpbp->dpb_pcyls > 1023) &&
		    	   (dpbp->dpb_pcyls > dpbp->dpb_cyls)) {
				dpbp->dpb_cyls = dpbp->dpb_pcyls;
			}
	}

}


/*
 * athd_cmd -- perform command on AT-class hard disk controller.
 */

int
athd_cmd(cmd,dcbp,dpbp)
int cmd;
register gdev_dcbp dcbp;
register gdev_dpbp dpbp;
{
	int i;
	int error = 0;

	if (cmd == DCMD_RETRY)
		cmd = dpbp->dpb_command;        /* just try again */
	else {       /* prime for new command */
		dpbp->dpb_command = cmd;   /* save command for later retry */
		dpbp->dpb_oddbyte = 0;   /* initialize some stuff */
	}

	if ((dpbp->dpb_drvflags & ATF_RESTORE) &&
	    ((cmd == DCMD_READ) || (cmd == DCMD_WRITE)))
	{       /* we have to recalibrate heads before actual I/O command */
		cmd = DCMD_RECAL;
	}

	switch (cmd)
	{
	case DCMD_READ:
		dpbp->dpb_drvcmd = ATC_RDSEC;
		/* enable controller ECC & RETRY only if we're supposed to. */
		if (dpbp->dpb_flags & (DFLG_RETRY | DFLG_ERRCORR))
			dpbp->dpb_drvcmd &= ~ATCM_ECCRETRY; /* inverse logic */
		dpbp->dpb_state = DSTA_NORMIO;
		break;
	case DCMD_WRITE:
		dpbp->dpb_drvcmd = ATC_WRSEC;
		/* enable controller ECC & RETRY only if we're supposed to. */
		if (dpbp->dpb_flags & (DFLG_RETRY | DFLG_ERRCORR))
			dpbp->dpb_drvcmd &= ~ATCM_ECCRETRY; /* inverse logic */
		dpbp->dpb_state = DSTA_NORMIO;
		break;
	case DCMD_FORMAT:
		{
			unchar *itablep;
			int i, secno;
			int cyl, head, sect, sectsize;

			cyl = dpbp->dpb_pcyls? dpbp->dpb_pcyls : dpbp->dpb_cyls;
			head = dpbp->dpb_pheads? dpbp->dpb_pheads : dpbp->dpb_heads;
			sect = dpbp->dpb_psectors? dpbp->dpb_psectors : dpbp->dpb_sectors;
			sectsize = dpbp->dpb_pbytes? dpbp->dpb_pbytes : dpbp->dpb_secsiz;

			/* Get memory for interleave table */
			if ((itablep = (unchar *)kmem_zalloc(sectsize, KM_SLEEP)) == NULL)
			{       /* no memory available now */
				return (ENOSPC);
			}

			/* build the interleave table.  If we're running a
		 	 * with interleave 1, skew the first sector by the head
		 	 * number of the track we're formatting times skew
		 	 * factor. We do this by setting 'i' (the starting index
		 	 * for loading sector #s) to something other than 1 for 			 * this case.
		 	 */
			athd_setskew(dpbp);  /* set skew factor appropriately */
			if (dpbp->dpb_interleave == 1)
			{       /* do the skew and let the world know... */
				i = ((int)(((dpbp->dpb_cursect/sect) % head)
				    * dpbp->dpb_skew) % sect) * 2 + 1;
			}
			else 
				i = 1;
			for (secno=1;secno<=sect;)
			{       /* build interleave table */
				if (itablep[i] == 0)
				{       /* found an unused slot */
					itablep[i] = secno++;
					i = (int)(i + dpbp->dpb_interleave * 2)%(sect * 2);
				}
				else        /* skip to next empty slot */
					i += 2;
			}
			athd_drivesel(dcbp);
			if (dpbp->dpb_drvflags & ATF_FMTRST)
			{       /* Have to do RESTORE, first */
				dpbp->dpb_drvflags &= ~ATF_FMTRST;
				dpbp->dpb_drvcmd = ATC_RESTORE;
				dpbp->dpb_state = DSTA_RECAL;
				/* save time when command was started */
				drv_getparm(LBOLT, &dcbp->dcb_laststart);
				dpbp->dpb_flags |= DFLG_BUSY;
				dcbp->dcb_flags |= CFLG_BUSY;
				athd_docmd(dcbp,dpbp);
				/* wait for restore to complete */
				while (dpbp->dpb_flags & DFLG_BUSY)
					sleep((caddr_t)dpbp,PRIBIO);
				if (dpbp->dpb_drverror)
					return (EIO);
			}

			/* Now we must perform a set params for the physical
		 	 * parameters--otherwise the format will fail. After
		 	 * completion, we must reprogram the controller
		 	 * for the operating parameters.
		 	 */
			outb(dcbp->dcb_ioaddr1+AT_DRVHD,
			    (dcb_curdriv(dcbp) == 0 ? ATDH_DRIVE0 : ATDH_DRIVE1)
			    | (head-1));
			outb(dcbp->dcb_ioaddr1+AT_COUNT,sect+dpbp->dpb_ressecs);
			outb(dcbp->dcb_ioaddr1+AT_LCYL, cyl & 0xff);
			outb(dcbp->dcb_ioaddr1+AT_HCYL, cyl >> 8);
			outb(dcbp->dcb_ioaddr1+AT_CMD,ATC_SETPARAM);

			/* set write precomp cylinder as follows:
		 	 * If number of heads is odd, use number of cyls.
		 	 * If even, use half the number of cyls.
		 	 */
			dpbp->dpb_wpcyl = ((head & 1) ? cyl : cyl / 2);
			athd_wait(dcbp->dcb_ioaddr1,ATS_BUSY,0,ATS_BUSY);
			for (i=100; i>0; i--)
				tenmicrosec();
			dpbp->dpb_drvcmd = ATC_FORMAT;
			dpbp->dpb_sectcount = 1; /* 1 sector's worth of data */
			dpbp->dpb_flags &= ~DFLG_READING;
			dpbp->dpb_state = DSTA_NORMIO;  /* More or less... */
			dpbp->dpb_newcount = sectsize;
			dpbp->dpb_memsect = dpbp->dpb_newdrq = NULL;
			/* write from interleave table... */
			dpbp->dpb_newvaddr = dpbp->dpb_virtaddr = (ulong)itablep;
			/* save time when command was started */
			drv_getparm(LBOLT, &dcbp->dcb_laststart);
			dpbp->dpb_flags |= DFLG_BUSY;
			dcbp->dcb_flags |= CFLG_BUSY;
			athd_docmd(dcbp,dpbp);  /* issue the command */
			/* wait for FORMAT command to complete */
			while (dpbp->dpb_flags & DFLG_BUSY) {
				sleep ((caddr_t)dpbp,PRIBIO);
			}
			if (dpbp->dpb_drverror)
				error = EIO;
			kmem_free(itablep,sectsize);
			itablep = NULL;

			/* Reset the virtual parameters */
			/*FALL THRU*/
		}
	case DCMD_SETPARMS:
		{
		struct admsbuf ab;      /* for mode select */
		athd_setskew(dpbp);     /* set skew factor appropriately */
		athd_drivesel(dcbp);    /* select the proper drive */
		/* no interrupt on this */
		outb(dcbp->dcb_ioaddr2,((dpbp->dpb_heads > 8 ? AT_EXTRAHDS
			    : AT_NOEXTRAHDS) | AT_INTRDISAB)); 
		if (dpbp->dpb_drvflags & ATF_ADAPTEC && dpbp->dpb_rescyls < 2) {
			/* Adaptec 237x (RLL or MFM) device.  Must use Adaptec
			 * Mode Select command to program the controller.  Do
			 * Adaptec 232x (ESDI) as a normal controller,
			 * programing it with a Set Parameters command.  The
			 * Mode Select command messes up any translate mode
			 * that an Adaptec ESDI drive might be using.
			 */
			register ushort temp;

			ab.adms_valid = ADMS_VALID;
			temp = dpbp->dpb_cyls + dpbp->dpb_rescyls;
			ab.adms_cylh = temp >> 8;
			ab.adms_cyll = temp & 0xff;
			ab.adms_nhds = dpbp->dpb_heads;
			temp = dpbp->dpb_sectors + dpbp->dpb_ressecs;
			ab.adms_nsect = temp;
			outb(dcbp->dcb_ioaddr1+AD_SPCLCMD,ADSC_MODESEL);
			outb(dcbp->dcb_ioaddr1+AT_CMD,ADC_SPECIAL);
			athd_wait(dcbp->dcb_ioaddr1,ATS_DATARQ,ATS_DATARQ,0);
			loutw(dcbp->dcb_ioaddr1+AT_DATA, &ab, 6);
		} else {       /* Not Adaptec RLL-- use WD approach */
			outb(dcbp->dcb_ioaddr1+AT_DRVHD,
			    (dcb_curdriv(dcbp) == 0 ? ATDH_DRIVE0 : ATDH_DRIVE1)
			    | (dpbp->dpb_heads-1));
			outb(dcbp->dcb_ioaddr1+AT_COUNT,
			    dpbp->dpb_sectors+dpbp->dpb_ressecs);
			outb(dcbp->dcb_ioaddr1+AT_LCYL, dpbp->dpb_cyls & 0xff);
			outb(dcbp->dcb_ioaddr1+AT_HCYL, dpbp->dpb_cyls >> 8);
			outb(dcbp->dcb_ioaddr1+AT_CMD,ATC_SETPARAM);
			/* set write precomp cylinder as follows:
			 * If number of heads is odd, use number of cyls.
			 * If even, use half the number of cyls.
			 */
			dpbp->dpb_wpcyl = ((dpbp->dpb_heads & 1) ? dpbp->dpb_cyls : dpbp->dpb_cyls / 2);
		}
		athd_wait(dcbp->dcb_ioaddr1,ATS_BUSY,0,ATS_BUSY);
		for (i=100; i>0; i--)
			tenmicrosec();
		return(0);
		}
	case DCMD_RECAL:
		dpbp->dpb_drvcmd = ATC_RESTORE;
		dpbp->dpb_state = DSTA_RECAL;
		break;
	case DCMD_SEEK:
		dpbp->dpb_drvcmd = ATC_SEEK;
		dpbp->dpb_state = DSTA_SEEKING;
		break;
	default:
#ifdef DEBUG
		cmn_err(CE_WARN,"athd_cmd unimplemented command %d!",cmd);
#endif
		break;
	}
	athd_docmd(dcbp,dpbp);
	return (error);
}


/*
 * athd_int -- process AT-class controller interrupt.
 */

struct gdev_parm_block *
athd_int(dcbp,intidx)
gdev_dcbp dcbp;
int intidx;
{
	register gdev_dpbp dpbp;
	register struct buf *bp;
	register struct drq_entry *drqp;
	unchar cmdstat;
	unchar drverr;
	int i;

	dpbp = dcbp->dcb_dpbs[dcb_curdriv(dcbp)+dcb_firstdriv(dcbp)];

	/*
 * If we have no dpb or if the one we have says it isn't an Adaptec,
 * get the contents of the status (and error, if necessary) register(s).
 * This will guarantee that the controllers on Compaqs drop their interrupt
 * line.  We do this even if the controller isn't supposed to be busy so
 * we don't get out of sync on leftover interrupts...
 */
	if ((dpbp == NULL) || ((dpbp->dpb_drvflags & ATF_ADAPTEC) == 0))
		cmdstat = inb(dcbp->dcb_ioaddr1+AT_STATUS);

	if ((dcbp->dcb_flags & CFLG_BUSY) == 0)
	{
		return(NULL);   /* Not for us, we aren't doing anything */
	}

	switch ((int)dpbp->dpb_state)
	{
	case DSTA_RECAL:
		if (dpbp->dpb_drvflags & ATF_RESTORE)
		{       /* restored, just do normal op as a retry */
			dpbp->dpb_drvflags &= ~ATF_RESTORE;
			athd_cmd(DCMD_RETRY,dcbp,dpbp);
			dpbp->dpb_intret = DINT_CONTINUE;
			break;
		}
	case DSTA_SEEKING:
		if (dpbp->dpb_drvflags & ATF_ADAPTEC)
			cmdstat = inb(dcbp->dcb_ioaddr1+AT_STATUS);
		if (cmdstat & ATS_ERROR)
		{       /* Error on seek/restore... tell guys above */
			/* wait for non-busy status */
			for (i = 0; i < 1000; i++) {
				if (((cmdstat = inb(dcbp->dcb_ioaddr1 + AT_STATUS)) & ATS_BUSY) == 0)
					break;
				tenmicrosec();
			}
			if (i == 1000)
				cmn_err(CE_NOTE,"!athd_int never got non-busy, got 0x%x\n",cmdstat);
			drverr = inb(dcbp->dcb_ioaddr1+AT_ERROR);
			dpbp->dpb_drverror = athd_err(drverr);
			dpbp->dpb_intret = DINT_ERRABORT;
		}
		else {       /* seek/restore completed normally. */
			dpbp->dpb_intret = DINT_COMPLETE;
			/* successful I/O, if following a reset we need to */
			/* set reset_count, so zero rather than test and set */
			reset_count = 0;
		}
		break;
	case DSTA_NORMIO:
		if (dpbp->dpb_flags & DFLG_READING)
			athd_recvdata(dcbp,dpbp);       /* Get data */
		if (dpbp->dpb_drvflags & ATF_ADAPTEC)
			cmdstat = inb(dcbp->dcb_ioaddr1+AT_STATUS);
		/* For a Write Fault, reset controller, and issue retries */
		if (cmdstat & ATS_WRFAULT) {
			cmn_err(CE_WARN,"Disk Driver: controller error, resetting controller");
			athd_reset(dcbp);
			break;
		}
		/* check for ECC correction when ECC is off */
		if ((cmdstat & ATS_ECC) && !(dpbp->dpb_flags & DFLG_ERRCORR)) {
			dpbp->dpb_drverror = DERR_DATABAD;
			dpbp->dpb_intret = DINT_ERRABORT;
			break;
			/* check for real error */
		} else 
			if (cmdstat & ATS_ERROR) {
		        /* Error on I/O... tell guys above */
			/* wait for non-busy status */
				for (i = 0; i < 1000; i++) {
					if (((cmdstat = inb(dcbp->dcb_ioaddr1 + AT_STATUS)) & ATS_BUSY) == 0)
						break;
					tenmicrosec();
				}
				if (i == 1000)
					cmn_err(CE_NOTE,"!athd_int never got non-busy, got 0x%x\n",cmdstat);
				drverr = inb(dcbp->dcb_ioaddr1+AT_ERROR);
				dpbp->dpb_drverror = athd_err(drverr);
				dpbp->dpb_intret = DINT_ERRABORT;
				break;
			}
		/* Read/wrote data successfully.  Update dpb. */

		/* successful I/O, if following a reset we need to */
		/* set reset_count, so just zero rather than test and set */
		reset_count = 0;
		dpbp->dpb_oddbyte = dpbp->dpb_newodd;   /* and oddbyte status */
		/* now call gdev_xferok(dpbp,1) */
		dcbp->dcb_xferok(dpbp,1);
		if (dpbp->dpb_intret == DINT_CONTINUE)
		{       /* see if we actually have to do anything */
			if (!(dpbp->dpb_flags & DFLG_READING))
				athd_senddata(dcbp,dpbp);       /* send next sector */
		}
		break;

	default:
#ifdef DEBUG
		cmn_err(CE_PANIC,"athd_int: invalid state %d!",dpbp->dpb_state);
#endif
		break;
	}
	return (dpbp);  /* let caller know we did something... */
}

void
athdtimeout(dcbp)
gdev_dcbp dcbp;
{
	long lbolt_val;
	register gdev_dpbp dpbp;

	if (drv_getparm(LBOLT,&lbolt_val) == -1) {
		timeout(athdtimeout, (caddr_t)dcbp, (30 * HZ));
		return;
	}

	if (((dcbp->dcb_flags & CFLG_BUSY) == 0) || ((dcbp->dcb_laststart + (30 * HZ)) > lbolt_val)) {
		timeout( athdtimeout, (caddr_t)dcbp, (30 * HZ));
		return;
	}
	if (reset_count > 3) {
		cmn_err(CE_WARN,"Disk Driver: Cannot reset controller successfully");
		cmn_err(CE_PANIC, "Disk Driver: Disk controller or hard disk has failed");
	}

	cmn_err(CE_WARN,"Disk Driver Request Timed Out, Resetting Controller");
	athd_reset(dcbp);

	timeout(athdtimeout, (caddr_t)dcbp, (30 * HZ));

	/* now call gdev_cplerr so retries are started */
	dpbp = dcbp->dcb_dpbs[dcb_firstdriv(dcbp) + dcb_curdriv(dcbp)];
	/* this prevents retries, since the dpb is not staying sane */
	dcb_drvint(dcbp)(dcbp, dpbp);
	/* increment reset_count so only 4 consecutive resets done */
	reset_count++;
}

/* General reset mechanism to reset controller and reset driver params */
athd_reset(dcbp)
gdev_dcbp dcbp;
{
	register gdev_dpbp dpbp, curdpbp;
	int curdrive, drive;

	/* Save the dpbp and the current to restore state later */
	curdpbp = dcbp->dcb_dpbs[dcb_firstdriv(dcbp) + dcb_curdriv(dcbp)];
	curdrive = dcb_curdriv(dcbp);

	/* Reset controller, then set params for drives */
	outb(dcbp->dcb_ioaddr2, AT_CTLRESET);
	drv_usecwait(20);
	outb(dcbp->dcb_ioaddr2,AT_NOEXTRAHDS);
	drv_usecwait(50);
	if (athd_wait(dcbp->dcb_ioaddr1, (ATS_BUSY|ATS_READY), ATS_READY, ATS_BUSY))
		cmn_err(CE_PANIC,"Disk Driver: Reset failure please check your disk controller");
	for (drive=0; drive < (int)dcb_drives(dcbp); drive++) {
		dpbp = dcbp->dcb_dpbs[dcb_firstdriv(dcbp) + drive];
		dcb_curdriv(dcbp) = drive;
		athd_cmd(DCMD_SETPARMS, dcbp, dpbp);
		dpbp->dpb_drvflags |= ATF_RESTORE;
		if (athd_wait(dcbp->dcb_ioaddr1, (ATS_BUSY|ATS_READY), ATS_READY, ATS_BUSY))
			cmn_err(CE_WARN,"!Disk Driver: reset disk params failed\n");
	}
	dcb_curdriv(dcbp) = curdrive;

	curdpbp->dpb_drverror = athd_err(4);
	curdpbp->dpb_intret = DINT_ERRABORT;
	curdpbp->dpb_retrycnt = 11; 
#ifdef DEBUG
	cmn_err(CE_CONT,"SECTCNT %d TOTCNT %d NEWCOUNT %d\n",curdpbp->dpb_sectcount,
		curdpbp->dpb_totcount, curdpbp->dpb_newcount);

	cmn_err(CE_CONT,"MEMSECT %x NEWDRQ %x NEWADDR %x NEWVADDR %x\n",curdpbp->dpb_memsect,
		curdpbp->dpb_newdrq, curdpbp->dpb_newaddr, curdpbp->dpb_newvaddr);
	if ((ulong)curdpbp->dpb_memsect > 0xC0000000L) {
		cmn_err(CE_CONT,"MEMSECT->count %d ",curdpbp->dpb_memsect->drq_count);
		cmn_err(CE_CONT,"MEMSECT->link  %x ",curdpbp->dpb_memsect->drq_link);
		cmn_err(CE_CONT,"MEMSECT->vaddr %x \n",curdpbp->dpb_memsect->drq_vaddr);
		cmn_err(CE_CONT,"MEMSECT->flags %x ",curdpbp->dpb_memsect->drq_word1.drq_flgint);
		cmn_err(CE_CONT,"MEMSECT->addr1 %x ",curdpbp->dpb_memsect->drq_addr1);
		cmn_err(CE_CONT,"MEMSECT->addr2 %x \n",curdpbp->dpb_memsect->drq_addr2);
	}
#endif
}

/*
 * athd_wait -- wait for the status register of a controller to achieve a
 *              specific state.  Arguments are a mask of bits we care about,
 *              and two sub-masks.  To return normally, all the bits in the
 *              first sub-mask must be ON, all the bits in the second sub-
 *              mask must be OFF.  If 10 seconds pass without the controller
 *              achieving the desired bit configuration, we return 1, else
 *              0.
 */
int
athd_wait(baseport,mask,onbits,offbits)
ushort baseport;
uint mask;
uint onbits;
uint offbits;
{
	register int i;
	ushort portno = baseport+AT_STATUS;
	register uint maskval = 0;

	for (i=500000; i; i--)
	{
		maskval = inb(portno) & mask;
		if (((maskval & onbits) == onbits) &&
		    ((maskval & offbits) == 0))
			return(0);
		tenmicrosec();
	}
	return(1);
}


/*
 * athd_drivesel -- cause drive indicated by dcb_curdriv(dcbp) to be selected.
 *    Indicate which was last selected in dcbp->dcb_lastdriv.
 *    if we changed drives, kill dcbp->dcb_delay ten-microsecond
 *    units of time.  Also set ATF_FMTRST bit in last dpb_drvflags
 *    to indicate that a RESTORE will be needed prior to formatting
 *    that drive again...
 */

athd_drivesel(dcbp)
register gdev_dcbp dcbp;
{
	register ushort i;

	outb(dcbp->dcb_ioaddr1+AT_DRVHD,
	    (dcb_curdriv(dcbp) == 0 ? ATDH_DRIVE0 : ATDH_DRIVE1));
	if (dcb_curdriv(dcbp) != dcbp->dcb_lastdriv)
	{       /* Delay to allow status to settle */
		register gdev_dpbp dpbp = dcbp->dcb_dpbs[dcbp->dcb_lastdriv];
		if (dpbp != NULL)
			dpbp->dpb_drvflags |= ATF_FMTRST;
		dcbp->dcb_lastdriv = dcb_curdriv(dcbp);
	}
}


/*
 * athd_docmd -- output a command to the controller.
 *               Generate all the bytes for the task file & send them.
 */

athd_docmd(dcbp,dpbp)
gdev_dcbp dcbp;
register gdev_dpbp dpbp;
{
	register ulong resid;
	uint  headval;
	uint head, sector;
	register ushort baseaddr = dcbp->dcb_ioaddr1;

	athd_drivesel(dcbp);
	outb(baseaddr+AT_PRECOMP, dpbp->dpb_wpcyl >> 2);
	if (dpbp->dpb_drvcmd == ATC_FORMAT)
	{       /* Really doing FORMAT, play some funny games */
		head = dpbp->dpb_pheads? dpbp->dpb_pheads : dpbp->dpb_heads;
		sector = dpbp->dpb_psectors? dpbp->dpb_psectors : dpbp->dpb_sectors;

		outb(baseaddr+AT_COUNT, sector);     /* Number of sectors */
		outb(baseaddr+AT_SECT, 35);    /* for 38-byte WD gap length */
	}
	else {       /* doing normal I/O, use real values */
		head = dpbp->dpb_heads;
		sector = dpbp->dpb_sectors;

		outb(baseaddr+AT_COUNT, dpbp->dpb_sectcount);
		outb(baseaddr+AT_SECT, (dpbp->dpb_cursect % sector) + 1);
	}
	outb(dcbp->dcb_ioaddr2,(head > 8 ? AT_EXTRAHDS : AT_NOEXTRAHDS));
	resid = dpbp->dpb_cursect / sector;
	headval = resid % head;
	headval |= (dcb_curdriv(dcbp) == 0 ? ATDH_DRIVE0 : ATDH_DRIVE1);
	outb(baseaddr+AT_DRVHD, headval);
	resid = resid / head;
	dpbp->dpb_curcyl = resid;       /* save current cylinder number */
	outb(baseaddr+AT_LCYL, resid & 0xff);
	outb(baseaddr+AT_HCYL, resid >> 8);
	outb(baseaddr+AT_CMD, dpbp->dpb_drvcmd);
	dpbp->dpb_drverror = 0;
	/* If we're doing some output, fill the controller's buffer */
	if (((dpbp->dpb_drvcmd & ~(ATCM_ECCRETRY|ATCM_LONGMODE)) ==
	    (ATC_WRSEC & ~(ATCM_ECCRETRY|ATCM_LONGMODE))) ||
	    (dpbp->dpb_drvcmd == ATC_FORMAT))
	{
		athd_wait(baseaddr,ATS_DATARQ,ATS_DATARQ,0);
		athd_senddata(dcbp,dpbp);
	}
}


/*
 * athd_senddata -- send a sector's worth of data to the controller.  We have
 *                  to look out for memory section breaks along the way.  In
 *                  addition, since the controller only takes data a word at
 *                  a time, we have to buffer any memory sections which cross
 *                  at non-word boundaries.  Since we don't know if the data
 *                  we're sending to the controller will be sucessfully sent
 *                  to the disk until after the next interrupt comes in, we
 *                  save our state so we can be restarted.  Updated counts
 *                  and pointers are left in temporary areas until interrupt
 *                  and are adjusted there if I/O completed normally...
 *                  (whew!)
 */

athd_senddata(dcbp,dpbp)
gdev_dcbp dcbp;
register gdev_dpbp dpbp;
{
	register ulong  bytesleft = dpbp->dpb_secsiz;   /* Number of bytes to send to controller */
	register struct drq_entry *drqp = dpbp->dpb_newdrq;
	struct oddbyte *odbp = (struct oddbyte *)&dpbp->dpb_oddbyte;    /*old one */

	while (bytesleft)
	{       /* do this until a full sector is transferred */
		ulong  curcnt;  /* the count for the current memory section */
		unchar *bptr = (unchar *)(dpbp->dpb_newvaddr);  /* virt addr */
		/* of data */
		ulong  bcnt;

		/* set curcnt to min(remaining in memsect, bytesleft) */

		if (dpbp->dpb_newcount >= bytesleft)
			curcnt = bytesleft;     /* room left in memsect */
		else curcnt = dpbp->dpb_newcount;    /* memsect < what's left */

		/* Now we check for an odd byte from the last transfer, and 
	 	 * merge it with our first byte for sending if we had one.
	 	 */
		bcnt = curcnt;  /* local copy for playing with */
		if (odbp->usedp)
		{       /* have leftover byte.  merge away */
			ushort tmpbuf;

			tmpbuf = (ushort)odbp->datab << 8;   /* always high byte left */
			tmpbuf |= *bptr++;
			bcnt -= 1;
			outw(dcbp->dcb_ioaddr1+AT_DATA,tmpbuf);
		}
		odbp = (struct oddbyte *)&dpbp->dpb_newodd;     /* switch to new one. */

		/* Transfer the remaining words */
		if (bcnt >= 2)
			loutw(dcbp->dcb_ioaddr1+AT_DATA,bptr,bcnt/2);

		/* See if we have a new odd byte at the end of this section */
		if (bcnt & 1)
		{       /* yep, save it away */
			bptr += (bcnt/2) * 2;   /* point at the oddie */
			odbp->usedp = 1;
			odbp->datab = *bptr;
		}
		else odbp->usedp = 0;

		if (bytesleft-=curcnt)
		{       /* need to skip to new memory section */
			dpbp->dpb_newvaddr = drq_vaddr(drqp);  /* update buf ptr */
			/* check link, in controller reset cases, next drq */
			/* may not be there so advancing will cause problems */
			if (drqp->drq_link != NULL) {
				drqp = drqp->drq_link;
			}

#ifdef DEBUG
			if ((drqp == NULL) )
				cmn_err(CE_WARN,"athd_senddata: ERROR drqp is NULL");
			if (drqp->drq_type != DRQT_MEMBREAK)
				cmn_err(CE_WARN,"athd_senddata: missing MEMBREAK, type is %x",drqp->drq_type);
#endif
			dpbp->dpb_newcount = drqp->drq_count;
		}
		else {       /* don't need new memsect, may have used one up. */
			dpbp->dpb_newvaddr += curcnt;   /* update buf ptr */
			dpbp->dpb_newcount -= curcnt;   /* and decr count */
		}
	}
	/* All done with transfer.  Save drqp in dpb_newdrq for later. */
	dpbp->dpb_newdrq = drqp;
}


/*
 * athd_recvdata -- like athd_senddata, but for getting data out of the
 *                  controller.  In case this controller doesn't give us
 *                  an error indicator until we've grabbed an entire sector
 *                  full of data, we save any state we change along the
 *                  way so it can be restarted (if legal) after status is
 *                  checked...
 */

athd_recvdata(dcbp,dpbp)
gdev_dcbp dcbp;
register gdev_dpbp dpbp;
{
	register ulong  bytesleft = dpbp->dpb_secsiz;   /* Number of bytes to get from controller */
	register struct drq_entry *drqp = dpbp->dpb_newdrq;
	struct oddbyte *odbp = (struct oddbyte *)&dpbp->dpb_oddbyte;

	while (bytesleft)
	{       /* do this until a full sector is transferred */
		ulong  curcnt;  /* the count for the current memory section */
		/* virt addr of buffer */
		unchar *bptr = (unchar *)(dpbp->dpb_newvaddr);  
		ulong  bcnt;

		/* set curcnt to min(remaining in memsect, bytes left) */
		if (dpbp->dpb_newcount >= bytesleft)
			curcnt = bytesleft;     /* room left in memsect */
		else curcnt = dpbp->dpb_newcount;    /* memsect < what's left */

		/* Now we check for an odd byte from the last transfer, and 
	 	 * stuff itinto the buffer if so.
	 	 */
		bcnt = curcnt;  /* local copy for playing with */
		if (odbp->usedp)
		{       /* have leftover byte.  stuff it. */
			*bptr++ = odbp->datab;
			bcnt -= 1;
		}
		odbp = (struct oddbyte *)&dpbp->dpb_newodd;     /* change to new one. */

		/* Transfer the remaining words */
		if (bcnt >= 2)
		{
			linw(dcbp->dcb_ioaddr1+AT_DATA,bptr,bcnt/2);
		}

		/* See if we have a new odd byte at the end of this section */

		if (bcnt & 1)
		{       /* yep, break up the last word. */
			ushort tmpbuf;

			bptr += (bcnt/2) * 2;   /* point at the oddie */
			tmpbuf = inw(dcbp->dcb_ioaddr1+AT_DATA);
			*bptr = (unchar)(tmpbuf >> 8);  /* Always the high byte left */
			odbp->usedp = 1;
			odbp->datab = tmpbuf & 0xff;    /* save the low byte */
		}
		else 
			odbp->usedp = 0;
		if (bytesleft-=curcnt)
		{       /* need to skip to new memory section */
			dpbp->dpb_newvaddr = drq_vaddr(drqp);  /* update buf ptr */
			/* check link, in controller reset cases, next drq */
			/* may not be there so advancing will cause problems */
			if (drqp->drq_link != NULL) {
				drqp = drqp->drq_link;
			}

#ifdef DEBUG
			if ((drqp == NULL) || (drqp->drq_type != DRQT_MEMBREAK))
				cmn_err(CE_WARN,"athd_recvdata ERROR missing MEMBREAK");
#endif
			dpbp->dpb_newcount = drqp->drq_count;
		}
		else {       /* don't need new memsect, tho we may have used one up. */
			dpbp->dpb_newvaddr += curcnt;   /* just update buf ptr */
			dpbp->dpb_newcount -= curcnt;   /* and decr count */
		}
	}
	/* All done with transfer.  Save drqp in dpb_newdrq for later. */
	dpbp->dpb_newdrq = drqp;
}


/*
 * athd_err -- Translate the AT_ERROR register into a Generic error #
 */

athd_err(errcod)
register unchar errcod;
{
	register int i;

	for (i = 0; i<8; i++)
	{
		if (errcod & 1) break;
		errcod >>= 1;
	}
	return (xlaterr[i]);
}


/*
 * athd_setskew -- set the dpb_skew value based on interleave and sectors
 *                 per track.  If interleave isn't 1, set to 0 (which means
 *                 it's either irrelevant or unknown).  Otherwise, if we
 *                 are on an RLL controller, we use a skew of 1.  Else,
 *                 use skew of 2 (ESDI -- ST506 isn't interleave 1).
 */
athd_setskew(dpbp)
register gdev_dpbp dpbp;
{
	dpbp->dpb_skew = ((dpbp->dpb_sectors == RLL_NSECS) ? 1 : 2);
}
