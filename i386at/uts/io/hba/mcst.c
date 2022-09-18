/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/hba/mcst.c	1.15"
#ident	"$Header: $"

/*
 * IBM PS/2 Hard Disk Low-Level Controller Interface Routines.
 * For use with Generic Disk Driver.  This file supports the 
 * micro channel ST506 controller, as either primary or secondary controller.
 */

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

#include <util/param.h>
#include <util/types.h>
#include <util/sysmacros.h>
#include <util/cmn_err.h>
#include <fs/buf.h>
#include <svc/errno.h>
#include <svc/sysenvmt.h>
#include <io/vtoc.h>
#include <io/target/alttbl.h>
#include <io/target/scsi.h>
#include <io/target/sdi_edt.h>
#include <io/target/sdi.h>
#include <io/hba/dcd.h>
#include <io/hba/gendev.h>
#include <io/hba/gendisk.h>
#include <io/hba/mcst.h>
#include <io/hba/mcdma.h>
#include <io/ddi.h>
#include <io/ddi_i386at.h>
#include <util/mod/moddefs.h>

MOD_MISC_WRAPPER(mcst_, NULL, NULL, "mcst - loadable DCD driver");

/*
 *	Global variables
 */
unchar	mcst_hdtype[2];		/* hard disk type from CMOS		*/
unchar	mcst_bps;		/* controller encoded bps value		*/

struct	ccb	ST506_CCB;	/* command control block		*/
struct	csb	ST506_CSB;	/* command specify block		*/
struct	ssb	ST506_SSB;	/* sense summary block			*/
struct	fcb	ST506_FCB[MAX_SECTORS];	/* format control block		*/

/*	error message for interrupt status register			*/
static char 	*mcst_msg[] = {
	"Command Invalid",
	"Command Reject",
	"Equipment Checks Fail",
	"Channel Abort or Adapter/Drive Status Error"
};

static ushort 	xlaterr[] = {
	DERR_BADCMD,
	DERR_UNKNOWN,
	DERR_CTLERR,
	DERR_ABORT
};

/*
 *	module definition
 */
unchar	mcst_status();
unchar	mcst_sec_len();
ushort 	mcst_errmsg();

/*
 * mcst_bdinit -- Board Initialization - initialize the hard disk controller.
 *                We return the number of drives which exist on that
 *                controller (or 0 if controller doesn't respond)...
 */

mcst_bdinit(cfgp,dcbp)
register struct gdev_cfg_entry *cfgp;
gdev_dcbp dcbp;
{
	register int	i;
	int		numdrive = 0;
	uint	bus_p;

	/* if not running in a micro-channel machine, skip initialization */
	if (!drv_gethardware(IOBUS_TYPE, &bus_p) && (bus_p != BUS_MCA))
		return(0);

	/*
 	*	set up the constant parameters in the control parameter block
 	*	These define the geometry of the disk.
 	*/
	ST506_CSB.error_control = ECC_ENABLE | RETRIES;
	ST506_CSB.recording_mode = DEVICE_TYPE | TRANSFER_RATE;
	ST506_CSB.gap_1_length = GAP_1_LENGTH;
	ST506_CSB.gap_2_length = GAP_2_LENGTH;
	ST506_CSB.gap_3_length = GAP_3_LENGTH;
	ST506_CSB.sync_length = SYNC_LENGTH;
	ST506_CSB.step_rate = STEP_RATE;

	/* 	reset the adapter	*/
	outb(cfgp->cfg_ioaddr1 + MCST_ACR, RESET | RESERVED);
	outb(cfgp->cfg_ioaddr1 + MCST_ACR, 0);

	for (i = HDTIMOUT; i > 0; i--) {
		if (!(inb(cfgp->cfg_ioaddr1 + MCST_ASR) & ST_BUSY))
			break;
		tenmicrosec();
	}

	/* 	ST506 controller not clearing BUSY when expected   */
	if (i <= 0)
		return(numdrive);

	/*	read CMOS to retrieve the first or second disk type  */
	mcst_hdtype[0] = CMOSread(FDTB);
	mcst_hdtype[1] = CMOSread(SDTB);
	outb(cfgp->cfg_ioaddr1+MCST_ACR, RESERVED | INT_ENABLE | BIT_16);
	/*
 	*  If controller is found return number of drives specified
	*  in cfg struct. Its set to 1 during install and can be increased.
 	*/
	return(cfgp->cfg_drives);
}


/*
 * mcst_drvinit -- Initialize the drive parameters.
 *                 Sets values in dpb for drive (based on info from controller
 *                 or ROM BIOS or defaults).
 */
mcst_drvinit(dcbp,dpbp)
register gdev_dcbp dcbp;
register gdev_dpbp dpbp;
{
	int		drive;
	unsigned long	hd_tabp;
	unsigned char	*pptr;

	/* set up fake inquiry command */
	dpbp->dpb_inqdata.id_type = 0;
	dpbp->dpb_inqdata.id_qualif = 0;
	dpbp->dpb_inqdata.id_rmb = 0;
	dpbp->dpb_inqdata.id_ver = 0x1;		/* scsi 1 */
	dpbp->dpb_inqdata.id_len = 31;
	strncpy(dpbp->dpb_inqdata.id_vendor, "(mcst)  ", 8);
	strncpy(dpbp->dpb_inqdata.id_prod, "MCA ST506 Disk  ", 16);
	strncpy(dpbp->dpb_inqdata.id_revnum, "1.00", 4);
	/*
 *	set up the driver's global variables
 */
	mcst_bps = (unchar) mcst_sec_len(dpbp->dpb_secsiz);


	/* 	set up some defaults in the dpb... 				*/
	dpbp->dpb_flags=(DFLG_RETRY| DFLG_ERRCORR | DFLG_VTOCSUP | DFLG_FIXREC);
	dpbp->dpb_devtype = DTYP_DISK;
	dpbp->dpb_secovhd = 81;
	dpbp->dpb_drvflags |= DPCF_CHGCYLS;
	drive = (dcb_curdriv(dcbp) == 0 ? 0 : 1);

	/*
 * Get disk parameters from the boot information structure
 */
	dpbp->dpb_cyls    = sysenvmtp->hdparamt[drive].hdp_ncyl;
	dpbp->dpb_heads   = sysenvmtp->hdparamt[drive].hdp_nhead;
	dpbp->dpb_wpcyl   = sysenvmtp->hdparamt[drive].hdp_precomp;
	dpbp->dpb_sectors = sysenvmtp->hdparamt[drive].hdp_nsect;

	/*
 * check drive parameters, this should'nt be necessary but some
 * ROMs have a bug and the pointer we got was bad.  If the values are
 * bad then set some reasonable default to allow us to access the
 * pdinfo block with the real values for this drive.
 * The same case applies to disk type in CMOS is 0. This means that
 * the disk is very likely to be an unformatted disk. The SETPARM
 * command will detect whether there is a physical drive conected.
 */
	if ((dpbp->dpb_cyls < 1) || (dpbp->dpb_heads < 2) ||
	    (mcst_hdtype[drive] == 0)) {
		/*	    disk type 1: the smallest disk geometry			*/
		dpbp->dpb_cyls    = 306;
		dpbp->dpb_heads   = 4;
		dpbp->dpb_sectors = 17;	/* standard for ST506/MFM	*/
		dpbp->dpb_wpcyl   = 128;
	}
}



/*
 * mcst_int -- process controller interrupt.
 */

struct gdev_parm_block *
mcst_int(dcbp,intidx)
gdev_dcbp dcbp;
int intidx;
{
	register gdev_dpbp dpbp;
	unchar drive;
	unchar status;


	if (!(inb(dcbp->dcb_ioaddr1+MCST_ASR) & INT_REQUEST))
		return(NULL);

	dpbp = dcbp->dcb_dpbs[dcb_curdriv(dcbp)+dcb_firstdriv(dcbp)];
	if (((dcbp->dcb_flags & CFLG_BUSY) == 0) || (dpbp == NULL))
		return(NULL);   /* Not for us, we aren't doing anything 	*/

	drive = (dcb_curdriv(dcbp) == 0 ? 0 : 1);
	status = inb(0x92) & (~(0x80 >> drive));
	outb(0x92, status);
	status = mcst_status(dcbp);
	/* 	Disable DMA 							*/
	outb(dcbp->dcb_ioaddr1+MCST_ACR,RESERVED|INT_ENABLE|BIT_16);
	if (status & (TERMINATE_ERROR | INVALID_COMMAND | 
	    COMMAND_REJECT | EQUIPMENT_CHECK))  {
		dpbp->dpb_drverror = mcst_errmsg(dcbp,status);
		dpbp->dpb_intret = DINT_ERRABORT;
		return(dpbp);
	}
	switch ((int)dpbp->dpb_state) {
		/* 	    Read/wrote data successfully.  Update dpb. 			*/
	case DSTA_NORMIO:
		/* now call gdev_xferok(dpbp,dpbp->dpb_sectcount) */
		dcbp->dcb_xferok(dpbp,dpbp->dpb_sectcount);
		if (dpbp->dpb_intret == DINT_CONTINUE)
			mcst_exec(dcbp,dpbp);
		break;
	case DSTA_RECAL:
	default:
		break;
	}
	return (dpbp);
}


/*
 * mcst_cmd -- perform command on the ST506 hard disk controller.
 */

mcst_cmd(cmd,dcbp,dpbp)
int cmd;
register gdev_dcbp dcbp;
register gdev_dpbp dpbp;
{
	register int 	secno;
	register int 	j;
	int 	status;
	int	drive;
	int	track;
	int 	head;
	int 	oldpri;


	/*	RETRY :- set command equal to last saved command		*/
	if (cmd == DCMD_RETRY)
		cmd = dpbp->dpb_command;
	else {
		/* 	save command code for retry and perform initialization	 	*/
		dpbp->dpb_command = cmd;	/* save command code for retry 	*/
	}
	drive = (dcb_curdriv(dcbp) == 0 ? 0 : 1);

	switch (cmd) {
	case DCMD_SETPARMS:
		{
			/*
 *		fill in the rest of the control specify block 
 *		using the values in the dpb.
 */
			ST506_CSB.precomp[0] = (unchar) (dpbp->dpb_wpcyl >> 8);
			ST506_CSB.precomp[1] = (unchar) dpbp->dpb_wpcyl;
			ST506_CSB.max_cylinder[0] = (unchar) ((dpbp->dpb_cyls - 1)>>8);
			ST506_CSB.max_cylinder[1] = (unchar) (dpbp->dpb_cyls - 1);
			ST506_CSB.heads = dpbp->dpb_heads;
			ST506_CSB.sectors = dpbp->dpb_sectors;

			oldpri=splhi();
			mcst_wait(dcbp); 	/* wait for controller not busy */
			outb(dcbp->dcb_ioaddr1+MCST_ATTN, CSB | ((drive)*DRIVE_SELECT));
			mcst_out(dcbp, &ST506_CSB, sizeof(struct csb));
			status = mcst_status(dcbp);
			splx(oldpri);

			if (status & (TERMINATE_ERROR | INVALID_COMMAND | 
			    COMMAND_REJECT | EQUIPMENT_CHECK))  {
				dpbp->dpb_flags |= DFLG_OFFLINE;
				return(SDI_HAERR);
			}
			return(0);
		}
	case DCMD_READ:
		dpbp->dpb_drvcmd = ST_READ_DATA;
		dpbp->dpb_state  = DSTA_NORMIO;
		/*		execute the command					*/
		mcst_exec(dcbp,dpbp);
		return(0);
	case DCMD_WRITE:
		dpbp->dpb_drvcmd = ST_WRITE_DATA;
		dpbp->dpb_state  = DSTA_NORMIO;
		/*		execute the command					*/
		mcst_exec(dcbp,dpbp);
		return(0);
	case DCMD_FORMAT:
		track = (int) (dpbp->dpb_cursect / (daddr_t)dpbp->dpb_sectors);
		head = track % (int)dpbp->dpb_heads;

		/*
		 * we always format interleave 1 with skew of 2 sects
		 * per head, per IBM 44MB drive tech ref.
		 * due to skew, we rebuild interleave table for each
		 * track.
		 */

		/* 		(head * 2) % sectors-trk 				*/
		j = (head << 1) % (int)dpbp->dpb_sectors;
		for (secno=1; secno <= (int)dpbp->dpb_sectors; secno++) {
			ST506_FCB[j].sector = (unchar) secno;
			j = (j + 1) %  (int)dpbp->dpb_sectors;
		}

		for (j = 0; j < (int)dpbp->dpb_sectors; j++) {
			ST506_FCB[j].head = (unchar) (head << 4);
			ST506_FCB[j].head += (unchar)((track / (int)dpbp->dpb_heads)>>8);
			ST506_FCB[j].cylinder=(unchar) (track/(int)dpbp->dpb_heads);
			ST506_FCB[j].bps = mcst_bps;
			ST506_FCB[j].fill_byte = FILL_BYTE;
		}

		dpbp->dpb_drvcmd = FORMAT_TRACK;
		dpbp->dpb_state = DSTA_NORMIO;
		dpbp->dpb_sectcount = dpbp->dpb_sectors;
		dpbp->dpb_curaddr = 0;
		dpbp->dpb_memsect = dpbp->dpb_newdrq = NULL;
		dpbp->dpb_flags |= DFLG_BUSY;
		dcbp->dcb_flags |= CFLG_BUSY;
		dpbp->dpb_drverror = 0;

		status = mcst_exec(dcbp,dpbp);
		/*
 *		if unable to set up the controller to perform the format
 *		then clear busy flag, and return with error status set.
 */
		if (!status) {
			dpbp->dpb_flags &= ~DFLG_BUSY;
			dcbp->dcb_flags &= ~CFLG_BUSY;
			return(SDI_HAERR);
		}

		while (dpbp->dpb_flags & DFLG_BUSY)
			sleep ((caddr_t)dpbp,PRIBIO);
		if (dpbp->dpb_drverror)
			return(SDI_HAERR);
		return(0);
	case DCMD_RECAL:
	default:
#ifdef DEBUG
		cmn_err(CE_WARN,"mcst_cmd: invalid command code= %d!",cmd);
#else
		;
#endif
	}
	return(0);
}



/*
 * Build a CCB and send the CCB to the adapter.
 */
mcst_exec(dcbp,dpbp)
register gdev_dcbp dcbp;
register gdev_dpbp dpbp;
{
	unchar 	drive;
	ulong 	cylinder;
	ulong 	head;
	int	oldpri;
	unchar 	status;
	int 	mystatus = 0;

	drive = (dcb_curdriv(dcbp) == 0 ? 0 : 1);
	cylinder = (ulong)(dpbp->dpb_cursect / (daddr_t)(dpbp->dpb_heads * 
	    dpbp->dpb_sectors));
	head = (ulong)((dpbp->dpb_cursect / (daddr_t)dpbp->dpb_sectors) % 
	    (daddr_t)dpbp->dpb_heads);

	ST506_CCB.command = (unchar) (dpbp->dpb_drvcmd | AS | EC);
	ST506_CCB.head = (unchar) ((head << 4)+(cylinder >> 8));
	ST506_CCB.cylinder = (unchar) cylinder;
	if (ST506_CCB.command == FORMAT_TRACK)
		ST506_CCB.sector = 0;
	else 
		ST506_CCB.sector = (unchar)((dpbp->dpb_cursect % 
		    (daddr_t)dpbp->dpb_sectors) + 1);

	ST506_CCB.sectors = (unchar) dpbp->dpb_sectcount;
	ST506_CCB.bps = mcst_bps;

	/*
	 * polling the adapter status register (intr priority >= 5)
	 * waiting for intr bit to come true
	 */
	mcst_wait(dcbp); /* wait for controller not busy 		*/
	outb(dcbp->dcb_ioaddr1+MCST_ACR, RESERVED | INT_ENABLE | BIT_16);
	outb(dcbp->dcb_ioaddr1+MCST_ATTN, CCB | ((drive)*DRIVE_SELECT));
	mcst_out(dcbp,&ST506_CCB, sizeof(struct ccb));
	status = mcst_status(dcbp);
	if (status & (TERMINATE_ERROR | INVALID_COMMAND | 
	    COMMAND_REJECT | EQUIPMENT_CHECK )) {
		dpbp->dpb_drverror = mcst_errmsg(dcbp,status);
		dpbp->dpb_intret = DINT_ERRABORT;
		if (dpbp->dpb_drvcmd != FORMAT_TRACK)
			/* now call gdev_cplerr(dcbp,dpbp) */
			dcb_drvint(dcbp)(dcbp,dpbp);
		return(mystatus);
	} else {
		/* 	    turn on the fixed disk activity light			*/
		status = inb(0x92) | (0x80 >> drive);
		outb(0x92, status);
		/* 	    Perform the appropriate data transfer			*/
		if (dpbp->dpb_drvcmd == FORMAT_TRACK) {
			/* 		set data request ready					*/
			outb(dcbp->dcb_ioaddr1+MCST_ATTN, 
			    DATA_REQUEST | ((drive)*DRIVE_SELECT));
			mcst_out(dcbp, ST506_FCB, 
			    (dpbp->dpb_sectcount * sizeof(struct fcb)) + 
			    (dpbp->dpb_sectcount & 1));
		}
		else {
			oldpri = splhi();
			outb(dcbp->dcb_ioaddr1+MCST_ACR, 
			    RESERVED|INT_ENABLE|DMA_ENABLE|BIT_16);
			mcst_dma((dpbp->dpb_drvcmd == ST_READ_DATA ? 1 : 0), 
			    dpbp->dpb_curaddr,
			    (dpbp->dpb_sectcount * dpbp->dpb_secsiz),
			    dcbp->dcb_dmachan1);
			/* set data request ready		 */
			outb(dcbp->dcb_ioaddr1+MCST_ATTN, 
			    DATA_REQUEST|((drive)*DRIVE_SELECT));
			splx(oldpri);
		}

	}
	return (mystatus = 1);
}


/*
 * Wait for controller BUSY to be cleared.  Will wait
 * approx. 1/4 second if controller BUSY still not cleared,
 * then will panic.
 */
mcst_wait(dcbp)
register gdev_dcbp dcbp;
{
	register int	i;

	for (i = HDTIMOUT; i > 0; i--) {
		if (!(inb(dcbp->dcb_ioaddr1+MCST_ASR) & ST_BUSY))
			return;
		tenmicrosec();
	}
}

/*
 * Routine to write data to the controller buffer.
 */
mcst_out(dcbp, buf, len)
register gdev_dcbp dcbp;
caddr_t	buf;
ulong len;
{
	mcst_wait_data(dcbp); /* wait for controller data request 	*/
	loutw(dcbp->dcb_ioaddr1+MCST_DR, buf, len<<1);
}


/*
 * Routine to read data from the sector buffer.
 */
mcst_in(dcbp, buf, len)
register gdev_dcbp dcbp;
caddr_t	buf;
ulong len;
{
	mcst_wait_data(dcbp); /* wait for controller data request 	*/
	linw(dcbp->dcb_ioaddr1+MCST_DR, buf, len<<1);
}



/*
 * Wait for controller DATA_REQUEST to be set.  Will wait
 * approx. 1 second if controller DATA_REQUEST (or INT_REQUEST on errors)
 * still not set, then will panic.
 */
mcst_wait_data(dcbp)
register gdev_dcbp dcbp;
{
	register int	i;

	for (i = HDTIMOUT; i > 0; i--) {
		if (inb(dcbp->dcb_ioaddr1+MCST_ASR) & DATA_REQUEST)
			return;
		tenmicrosec();
	}
}



/*
 * Wait for controller INTERRUPT_REQUEST to be set.  Will wait
 * approx. 1/4 second if controller INTERRUPT_REQUEST still not set,
 * then will panic.
 * Return the value of the controller interrupt status register.
 */
unchar mcst_status(dcbp)
register gdev_dcbp dcbp;
{
	register int	i;

	for (i = HDTIMOUT; i > 0; i--) {
		if (inb(dcbp->dcb_ioaddr1+MCST_ASR) & INT_REQUEST)
			return (inb(dcbp->dcb_ioaddr1+MCST_ISR));
		tenmicrosec();
	}
	return(0);	/* should not return at this point		*/
}

ushort
mcst_errmsg(dcbp,status)
register gdev_dcbp dcbp;
unchar    status;
{
	int 	error;
	unchar  drive;

	drive = (dcb_curdriv(dcbp) == 0 ? 0 : 1);
	if (status & INVALID_COMMAND)
		error = 0;
	else if (status & COMMAND_REJECT)
		error = 1;
	else if (status & EQUIPMENT_CHECK)
		error = 2;
	else
		error = 3;
	return(xlaterr[error]);
}


/*
 * Routine to set up the DMA controller to do the requested
 * transfer from/to the disk controller.
 */
mcst_dma(rw, buf, len, channel)
int	rw;
ulong	buf;
ulong	len;
unchar	channel;
{
	int	oldpri;

	len >>= 1;				/* len =(len / 2)- 1	*/
	len--;
	oldpri = splhi();
	outb(PS2DMA_CTL,PS2DMA_SMK | channel); 	/* set mask register 	*/
	outb(PS2DMA_CTL,PS2DMA_MAR | channel);	/* set memory addr reg 	*/
	outb(PS2DMA_DAT, buf & 0xff); 		/* set low byte of addr */
	outb(PS2DMA_DAT, (buf >> 8) & 0xff); 	/* set 2nd byte of addr */
	outb(PS2DMA_DAT, (buf >> 16) & 0xff); 	/* set page register 	*/
	outb(PS2DMA_CTL,PS2DMA_TCR | channel);	/* set xfer count reg 	*/
	outb(PS2DMA_DAT, len & 0xff);		/* set low byte of cnt  */
	outb(PS2DMA_DAT, (len >> 8) & 0xff);	/* set high byte of cnt */
	outb(PS2DMA_CTL,PS2DMA_WMR | channel);	/* set transfer cnt reg */
	outb(PS2DMA_DAT, rw ? PS2DMA_WR : PS2DMA_RD);/* R/W 16-bit mode */
	outb(PS2DMA_CTL,PS2DMA_CMK | channel); 	/* reset mask register 	*/
	splx(oldpri);
}


/*
 * module to read back the sense summary block of the given drive
 */

mcst_rd_ssb(dcbp)
register gdev_dcbp dcbp;
{
	register int	i;
	unchar	drive;
	int	status;
	unchar	*ssbp;

	drive = (dcb_curdriv(dcbp) == 0 ? 0 : 1);
	mcst_wait(dcbp);
	outb(dcbp->dcb_ioaddr1+MCST_ATTN, SSB | ((drive)*DRIVE_SELECT));
	mcst_in(dcbp, &ST506_SSB, sizeof(struct ssb));
	status = mcst_status(dcbp);

	printf("mcst_rd_ssb: status= %x\n", status);
	for (i=0, ssbp=(unchar *)&ST506_SSB; i<sizeof(struct ssb); i++,ssbp++)
		printf(" SSB[%d]=%x ", i, *ssbp);

	printf("\n");
}

/*
 *
 * module to convert the sector size to encode sector length 
 * for the controller.
 * sector size		encoded value
 * 128			     0
 * 256			     1
 * 512			     2
 * 1024			     3
 *
 */
unchar
mcst_sec_len(secsiz)
ushort	secsiz;
{
	register unsigned int	i;

	for (i=0; secsiz; i++)
		secsiz >>= 1;
	if (--i >= 7)
		i -= 7;
	return ((unchar) i);
}
