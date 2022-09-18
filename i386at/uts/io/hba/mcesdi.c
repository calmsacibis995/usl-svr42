/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/hba/mcesdi.c	1.19"
#ident	"$Header: $"

/*
 * IBM Micro Channel Hard Disk Low-Level Controller Interface Routines.
 * For use with Generic Disk Driver.  This file supports the stock 
 * ESDI controllers as either primary or secondary controller.
 */

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987 INTERACTIVE Systems Corporation
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


#include <util/types.h>
#include <util/cmn_err.h>
#include <svc/errno.h>
#include <svc/sysenvmt.h>
#include <io/dma.h>
#include <io/vtoc.h>
#include <io/target/scsi.h>
#include <io/target/sdi_edt.h>
#include <io/target/sdi.h>
#include <io/hba/dcd.h>
#include <io/hba/mcesdi.h>
#include <io/hba/mcdma.h>
#include <io/hba/gendev.h>
#include <io/hba/gendisk.h>
#include <io/ddi.h>
#include <io/ddi_i386at.h>
#include <util/mod/moddefs.h>

MOD_MISC_WRAPPER(mces_, NULL, NULL, "mcesdi - loadable DCD driver");


/*
 *	The following inb, outb, inw, outw functions were created for the 
 *	MCA ESDI controller must be different to workaround a TLB bug 
 *	in the 386 (B stepping) chip involving I/O addresses above 1000h.
 *	
 *	The following functions were replaced with the asm inline equivalents
 * 	to speed them up and avoid the use of non-DDI compliant routines.
 *
 *esdiinb(reg)
 *unsigned int reg;
 *{
 *	unsigned int val;
 *
 *	intr_disable();
 *	flushtlb();
 *	val = inb(reg);
 *	intr_restore();
 *	return(val);
 *}
 *
 *esdiinw(reg)
 *unsigned int reg;
 *{
 *	unsigned int val;
 *
 *	intr_disable();
 *	flushtlb();
 *	val = inw(reg);
 *	intr_restore();
 *	return(val);
 *}
 *
 *esdioutb(reg,val)
 *unsigned int reg, val;
 *{
 *	intr_disable();
 *	flushtlb();
 *	outb(reg,val);
 *	intr_restore();
 *	return;
 *}
 *
 *esdioutw(reg,val)
 *unsigned int reg, val;
 *{
 *	intr_disable();
 *	flushtlb();
 *	outw(reg,val);
 *	intr_restore();
 *	return;
 *}
 */

asm unsigned char
esdiinb(addr)
{
%mem addr;
	movl	addr, %edx
	pushfl
	cli
	movl    %cr3, %eax
	movl    %eax, %cr3
	subl	%eax, %eax
	inb	(%dx)
	popfl
}

asm unsigned short
esdiinw(addr)
{
%mem addr;
	movl	addr, %edx
	pushfl
	cli
	movl    %cr3, %eax
	movl    %eax, %cr3
	subl	%eax, %eax
	data16
	inl	(%dx)
	popfl
}

asm void
esdioutb(addr, val)
{
%mem addr, val;
	pushfl
	cli
	movl    %cr3, %eax
	movl    %eax, %cr3
	movl	addr, %edx
	movl	val, %eax
	outb	(%dx)
	popfl
}

asm void
esdioutw(addr, val)
{
%mem addr, val;
	pushfl
	cli
	movl    %cr3, %eax
	movl    %eax, %cr3
	movl	addr, %edx
	movl	val, %eax
	data16
	outl	(%dx)
	popfl
}

struct cb ESDI_CB;

ushort ESDI_SB[ESDI_SB_SIZE];
void ESDIwait();
void ESDIstin();
ushort esdierrmsg();
unchar ESDIstatus();

#ifdef DEBUG
int esdiwarning = 0;	/* So we can turn on/off warning messages */
#endif

/*
 * error messages 
 */
static char *ESDIerrmsg[] = {
	"Reserved",
	"Command Completed with Success",
	"Reserved",
	"Command Completed with ECC Applied",
	"Reserved",
	"Command Completed with Retries",
	"Format Command Partially Completed",
	"Command Completed with ECC and Retries",
	"Command Completed with Warning",
	"Abort Complete",
	"Reset Complete",
	"Data Transfer Ready",
	"Command Completed with Failure",
	"DMA Error",
	"Command Block Error",
	"Attention Error",
	"Data Transfer Interrupt Expected",
	"Data Transfer Interrupt NOT Expected",
	"Format Interrupt Expected",
	"Format Interrupt NOT Expected"
};


/* Error codes if error 0xC (failure) use error codes below */
static ushort xlaterr[]= {
	DERR_UNKNOWN,		/* Reservered */
	DERR_NOERR,		/* Success    */
	DERR_UNKNOWN,		/* Reservered */
	DERR_ERRCOR,		/* Success w/ ECC */
	DERR_UNKNOWN,		/* Reservered */
	DERR_ERRCOR,		/* Success w/ retries */
	DERR_FMTERR,		/* Partial Format */
	DERR_ERRCOR,		/* Success w/ ECC retries */
	DERR_ERRCOR,		/* Success w/ warning */
	DERR_NOERR,		/* Abort complete */
	DERR_NOERR,		/* Reset complete */
	DERR_NOERR,		/* Data Xfer ready */
	DERR_DATABAD,		/* Failure */
	DERR_OVERRUN,		/* DMA error */
	DERR_ABORT,		/* Command Block error */
	DERR_CTLERR,		/* Attention Error */
	DERR_DATABAD,		/* Int expected */
	DERR_CTLERR,		/* Int  NOT expected */
	DERR_FMTERR,		/* Format int expected */
	DERR_FMTERR		/* Format int NOT expected */
};

/* These device codes map form 0x00 - 0x18 */
static short xlat2err[] = {
	DERR_NOERR,		/* NO Error */
	DERR_SEEKERR,		/* Seek Fault */
	DERR_CTLERR,		/* Interface Fault */
	DERR_BADSECID,		/* Block Not found (couldn't locate ID )*/
	DERR_BADSECID,		/* AM Not found (Not Formatted ) */
	DERR_DATABAD,		/* Data ECC error */
	DERR_BADSECID,		/* ID ECC Error */
	DERR_PASTEND,		/* RBA Out of Range */
	DERR_TIMEOUT,		/* Time Out Error */
	DERR_DATABAD,		/* Defective Block */
	DERR_MEDCHANGE,		/* Disk Changed */
	DERR_ABORT,		/* Selection Error */
	DERR_WRTPROT,		/* Media Write Protected */
	DERR_WFAULT,		/* Write Fault */
	DERR_DATABAD,		/* Read Fault */
	DERR_SECNOTFND,		/* Bad Format */
	DERR_PASTEND,		/* Volume Overflow */
	DERR_DAMNF,		/* No Data AM found */
	DERR_BADSECID,		/* No ID AM and ID ECC */
	DERR_DRVCONFIG,		/* No Device Configuration Info */
	DERR_BADCMD,		/* Missing first and last RBA flags */
	DERR_SECNOTFND,		/* No ID's found on track */
};

/* These device codes map to 0x80 - 0x86 */
static short xlat3err[] = {
	DERR_UNKNOWN,		/* Reserved */
	DERR_NOSEEKC,		/* time out wiating for SERDES to Stop */
	DERR_TIMEOUT,		/* Time OUT waiting for Data Transfer to end */
	DERR_TIMEOUT,		/* Time Out waiting for FIFO request */
	DERR_ABORT,		/* SERDES has stopped */
	DERR_ABORT,		/* Time Out waiting for Head Switch */
	DERR_TIMEOUT,		/* Timout waiting for DMA complete */
};

/*
 * mces_bdinit -- Initialize AT-class hard disk controller.
 *                We return the number of drives which exist on that
 *                controller (or 0 if controller doesn't respond)...
 */

int
mces_bdinit(cfgp,dcbp)
register struct gdev_cfg_entry *cfgp;
gdev_dcbp dcbp;
{

	unsigned char	status;
	int i;
	uint	bus_p;

	/* if not running in a micro-channel machine, skip initialization */
	if (!drv_gethardware(IOBUS_TYPE, &bus_p) && (bus_p != BUS_MCA))
		return(0);

	/* Reset the adpater */
	esdioutb(cfgp->cfg_ioaddr1 + ESDI_BCR, 0);
	for (i = HDTIMOUT; i > 0; i--) {
		if (!(esdiinb(cfgp->cfg_ioaddr1 + ESDI_BSR) & (BUSY | INTPND)))
			break ;
		tenmicrosec();
	}
	if(i <= 0)
		return 0;
	esdioutb(cfgp->cfg_ioaddr1 + ESDI_ATTN, ADAPTER_ID | AR_RESET);
	status = ESDIstatus(cfgp->cfg_ioaddr1);
	if (status == (ADAPTER_ID | ID_RESET))
		ESDIstin(cfgp->cfg_ioaddr1);
	esdioutb(cfgp->cfg_ioaddr1 + ESDI_ATTN, ADAPTER_ID | AR_EOI);

	if(!mcfind_drive(cfgp->cfg_ioaddr1, 0)){
		return 0;
	}

	if (cfgp->cfg_drives == 1)
	{       /* don't bother to look for second drive */
		return (1);
	}
	if(mcfind_drive(cfgp->cfg_ioaddr1, 1)){
		return 2;	/* got second drive */
	}
	else 
	{        /* Only first drive responded on controller */
		/* don't issue seeks on 1-drive board */
		cfgp->cfg_capab |= CCAP_NOSEEK;
		return (1);
	}
}


/*
 * mces_drvinit -- Initialize drive on controller.
 *                 Sets values in dpb for drive (based on info from controller
 *                 or ROM BIOS or defaults).
 */
mces_drvinit(dcbp,dpbp)
register gdev_dcbp dcbp;
register gdev_dpbp dpbp;
{
	int i;
	unchar status;
	unchar idrive, id;
	ushort drive;
	ulong drivesize;
	void ESDIcmdout();

	dpbp->dpb_inqdata.id_type = 0;
	dpbp->dpb_inqdata.id_qualif = 0;
	dpbp->dpb_inqdata.id_rmb = 0;
	dpbp->dpb_inqdata.id_ver = 0x1;		/* scsi 1 */
	dpbp->dpb_inqdata.id_len = 31;
	strncpy(dpbp->dpb_inqdata.id_vendor, "(mcesdi)", 8);
	strncpy(dpbp->dpb_inqdata.id_prod, "MCA ESDI Disk   ", 16);
	strncpy(dpbp->dpb_inqdata.id_revnum, "1.00", 4);


	esdioutb(dcbp->dcb_ioaddr1 + ESDI_BCR, 0);	/* turn off interupt */
	/* set up some defaults in the dpb... */
	dpbp->dpb_flags=(DFLG_RETRY | DFLG_ERRCORR | DFLG_VTOCSUP|DFLG_FIXREC);
	dpbp->dpb_drvflags = (DPCF_NOTRKFMT | DPCF_BADMAP);
	dpbp->dpb_devtype = DTYP_DISK;

	/* Establish defaults for disk geometry */
	drive = dcb_curdriv(dcbp);
	ESDI_CB.command = GET_DEV_CFG | (drive << DEVICE_SHIFT) | 
	    RESERVED6 | TYPE_2;
	ESDI_CB.blocks = 0;
	ESDI_CB.RBA = 0;



	ESDIwait(dcbp->dcb_ioaddr1); /* wait for controller not busy */
	esdioutb(dcbp->dcb_ioaddr1 + ESDI_ATTN, AR_COMMAND | (drive << DEVICE_SHIFT));
	/* make sure that we don't have an interrupt here already 
	 * if we have an interrupt this means that the drive doesn't exist
	 * and we shouldn't send it the rest of the command
	 */
	for (i = HDTIMOUT/500; i > 0; i--) {
		if (esdiinb(dcbp->dcb_ioaddr1 + ESDI_BSR) & IRPT) {
			if((status=esdiinb(dcbp->dcb_ioaddr1 + ESDI_ISR)) & 0xf)
				return;
			else
				printf("ESDI: unexpected interrupt while initializing drive\n");
		}
		tenmicrosec();
	}

	ESDIcmdout(dcbp->dcb_ioaddr1);
	status = ESDIstatus(dcbp->dcb_ioaddr1);
	idrive = status & DEVICE_SELECT;
	id = status & INTID;
	if (((idrive >> DEVICE_SHIFT) == drive) &&
	    (id == ID_OK) ||
	    (id == ID_ECC) ||
	    (id == ID_RETRY) ||
	    (id == ID_ECC_RETRY) ||
	    (id == ID_WARNING)) {
		ESDIstin(dcbp->dcb_ioaddr1);
		drivesize = ((ulong)(ESDI_SB[3]) << 16) + ESDI_SB[2];
		/*
		 * drivesize now contains the actual size of the drive in
		 * sectors.
		 * To present a standard interface at the BIOS level,
		 * IBM has decided that ESDI drives should have virtual
		 * cylinders of 2048 sectors (1 MB).  This lets them access
		 * 1-GIG drives by having only 1024 cylinders.  We will do
		 * the same in reporting drive geometry.  Note that this may
		 * leave some part of the drive (<1MB at the end) inaccessable.
		 */
		dpbp->dpb_cyls = drivesize / 2048;
		dpbp->dpb_heads = 64;
		dpbp->dpb_sectors = 32;
		dpbp->dpb_secsiz = SECTOR_SIZE;
		dpbp->dpb_drvflags |= (DPCF_CHGCYLS);
	}
	ESDIwait(dcbp->dcb_ioaddr1); /* wait for controller not busy */
	esdioutb(dcbp->dcb_ioaddr1 + ESDI_ATTN, idrive | AR_EOI);

	esdioutb(dcbp->dcb_ioaddr1 + ESDI_BCR, INTEN);
}


/*
 * mces_cmd -- perform command on AT-class hard disk controller.
 */

int
mces_cmd(cmd,dcbp,dpbp)
int cmd;
register gdev_dcbp dcbp;
register gdev_dpbp dpbp;
{
	int error = 0;

	if (cmd == DCMD_RETRY)
		cmd = dpbp->dpb_command;        /* just try again */
	else /* prime for new command */
		/* save command code for later retry */
		dpbp->dpb_command = cmd;

	switch (cmd)
	{
	case DCMD_READ:
		dpbp->dpb_drvcmd = READ_DATA;
		dpbp->dpb_state = DSTA_NORMIO;
		break;
	case DCMD_WRITE:
		dpbp->dpb_drvcmd = WRITE_DATA;
		dpbp->dpb_state = DSTA_NORMIO;
		break;
	case DCMD_FMTDRV:	/* Can only Format Entire Drive */
		dpbp->dpb_drvcmd = FORMAT_PREPARE;
		dpbp->dpb_flags |= DFLG_BUSY;
		dcbp->dcb_flags |= CFLG_BUSY;
		(void)mcesdi_docmd(dcbp,dpbp);
		/* wait for first part of FORMAT to complete */
		while(dpbp->dpb_flags & DFLG_BUSY)
			sleep((caddr_t)dpbp, PRIBIO);
		if(dpbp->dpb_drverror)
			error = SDI_HAERR;
		dpbp->dpb_drvcmd = FORMAT_UNIT;
		dpbp->dpb_flags |= DFLG_BUSY;
		dcbp->dcb_flags |= CFLG_BUSY;
		(void)mcesdi_docmd(dcbp,dpbp);
		/* wait for first part of FORMAT to complete */
		while(dpbp->dpb_flags & DFLG_BUSY)
			sleep((caddr_t)dpbp, PRIBIO);
		if(dpbp->dpb_drverror)
			return (SDI_HAERR);
	default:
		return (SDI_TCERR);
	}
	if(mcesdi_docmd(dcbp,dpbp) == FAILURE)
	{
		/* now call gdev_cplerr(dcbp,dpbp) */
		dcb_drvint(dcbp)(dcbp, dpbp);
	}
	return (error);
}


/*
 * mces_int -- process AT-class controller interrupt.
 */

struct gdev_parm_block *
mces_int(dcbp,intidx)
gdev_dcbp dcbp;
int intidx;
{
	register gdev_dpbp dpbp;
	unchar drive, id;
	unchar sys_ctrl_A;
	int i;

	dpbp = dcbp->dcb_dpbs[dcb_curdriv(dcbp)+dcb_firstdriv(dcbp)];
	if (dpbp == NULL)
		return (NULL);

	if (!(esdiinb(dcbp->dcb_ioaddr1 + ESDI_BSR) & IRPT))
		return (NULL);

	if (!(esdi_chk_isr(dcbp, dpbp, &id))){
		return(dpbp);
	}

	/*	get current drive					*/
	drive = (unchar) dcb_curdriv(dcbp);
	/*	turn the disk light off					*/
	sys_ctrl_A = inb(DISKLIGHT_REG);
	outb(DISKLIGHT_REG, sys_ctrl_A & (~(0x80 >> drive)));
        for (i = HDTIMOUT; i > 0; i--) {
                if (!(esdiinb(dcbp->dcb_ioaddr1 + ESDI_BSR) & (BUSY)))
                        break;
                tenmicrosec();
        }
	esdioutb(dcbp->dcb_ioaddr1+ESDI_ATTN, (drive << DEVICE_SHIFT) | AR_EOI);

	switch ((int)dpbp->dpb_state)
	{
	case DSTA_RECAL:
		dpbp->dpb_intret = DINT_COMPLETE;
		break;
	case DSTA_NORMIO:
		switch(id){
		case ID_ECC:
			if(!(dpbp->dpb_flags & DFLG_ERRCORR))
			{
				dpbp->dpb_drverror = DERR_DATABAD;
				return (dpbp);
			}
		case ID_RETRY:
			if(!(dpbp->dpb_flags & DFLG_RETRY))
			{
				dpbp->dpb_drverror = DERR_DATABAD;
				return (dpbp);
			}
		case ID_ECC_RETRY:
			if(!(dpbp->dpb_flags & (DFLG_ERRCORR|DFLG_RETRY)))
			{
				dpbp->dpb_drverror = DERR_DATABAD;
				return (dpbp);
			}
		}
		/* now call gdev_xferok(dpbp, dpbp->dpb_sectcount) */
		dcbp->dcb_xferok(dpbp, dpbp->dpb_sectcount);
		if(dpbp->dpb_intret == DINT_CONTINUE)
			if(mcesdi_docmd(dcbp,dpbp) == FAILURE)
			{
				/* now  call gdev_cplerr(dcbp,dpbp) */
				dcb_drvint(dcbp)(dcbp, dpbp);
			}
		break;
	case DSTA_FORMAT:
		if(id == ID_FORMAT){
			dpbp->dpb_drverror = DERR_FMTERR;

		}
		else
			dpbp->dpb_drverror = DERR_NOERR;
		break;
	default:
		break;
	}
	return (dpbp);  /* let caller know we did something... */
}

/*
 * mcesdi_docmd -- output a command to the controller.
 *               Generate all the bytes for the task file & send them.
 */

mcesdi_docmd(dcbp,dpbp)
gdev_dcbp dcbp;
register gdev_dpbp dpbp;
{

	int curdriv = dcb_curdriv(dcbp);
	unchar sys_ctrl_A;
	void ESDIcmdout();
	unchar	int_id;
	int ret_status = SUCCESS;

	if (dpbp->dpb_command == DCMD_FMTDRV) {
		if (dpbp->dpb_drvcmd == FORMAT_UNIT) {
			ESDI_CB.command = dpbp->dpb_drvcmd | 
			    (curdriv << DEVICE_SHIFT) | RESERVED6 | TYPE_2;
			ESDI_CB.blocks = 0x0;
			dpbp->dpb_state |= DSTA_FORMAT;
		} else if (dpbp->dpb_drvcmd == FORMAT_PREPARE) {
			ESDI_CB.command = dpbp->dpb_drvcmd | 
			    (curdriv << DEVICE_SHIFT) | RESERVED6 | TYPE_2;
			ESDI_CB.blocks = 0x55AA;
			dpbp->dpb_state |= DSTA_FORMAT;
		}
	} else {
		dpbp->dpb_state |= DSTA_NORMIO;
		ESDI_CB.command = dpbp->dpb_drvcmd | (curdriv << DEVICE_SHIFT)
		    | RESERVED2 | TYPE_4;
		ESDI_CB.blocks = dpbp->dpb_sectcount;
		ESDI_CB.RBA = dpbp->dpb_cursect;
	}

	/*	turn the disk light on						*/
	sys_ctrl_A = inb(DISKLIGHT_REG);
	outb(DISKLIGHT_REG, sys_ctrl_A | (0x80 >> curdriv));
	ESDIwait(dcbp->dcb_ioaddr1); /* wait for controller not busy */
	esdioutb(dcbp->dcb_ioaddr1 + ESDI_ATTN, (curdriv << DEVICE_SHIFT) | AR_COMMAND);
	ESDIcmdout(dcbp->dcb_ioaddr1);
	if ((dpbp->dpb_drvcmd == READ_DATA) ||
	    (dpbp->dpb_drvcmd == WRITE_DATA)) {
		/*
	 	 * wait for the esdi command to complete
		 * and then set up the DMA.
		 */
		while ( ret_status = esdi_chk_isr(dcbp, dpbp, &int_id)) {
			if ( int_id != ID_XFER)
				continue;
			else {
				/* setup DMA for the data transfer */
				ESDIdma(dcbp,dpbp);
				break;
			}
		}
	}
	return ret_status;
}

/*
 * Routine to set up the DMA controller to do the requested
 * transfer from/to the disk controller.
 */
ESDIdma(dcbp,dpbp)
register gdev_dcbp dcbp;
register gdev_dpbp dpbp;
{
	int	oldpri;
	ushort 	channel = dcbp->dcb_dmachan1;
	int 	rw = dpbp->dpb_flags & DFLG_READING;
	ulong	addr = dpbp->dpb_curaddr;
	ulong	len = dpbp->dpb_sectcount * dpbp->dpb_secsiz / 2;

	oldpri = splhi();
	outb(PS2DMA_CTL,PS2DMA_SMK + channel); 	/* set mask register */
	outb(PS2DMA_CTL,PS2DMA_MAR + channel);	/* set memory address reg */
	outb(PS2DMA_DAT, addr & 0xff); 		/* set low byte of address */
	outb(PS2DMA_DAT, (addr >> 8) & 0xff); 	/* set 2nd byte of address */
	outb(PS2DMA_DAT, (addr >> 16) & 0xff); 	/* set page register */
	outb(PS2DMA_CTL,PS2DMA_TCR + channel);	/* set transfer count reg */
	outb(PS2DMA_DAT, (len-1) & 0xff);	/* set low byte of count */
	outb(PS2DMA_DAT, ((len-1) >> 8) & 0xff);/* set high byte of count */
	outb(PS2DMA_CTL,PS2DMA_WMR + channel);	/* set transfer count reg */
	outb(PS2DMA_DAT, rw ? PS2DMA_WR : PS2DMA_RD);/* set R/W 16-bit mode */
	outb(PS2DMA_CTL,PS2DMA_CMK + channel); 	/* reset mask register */
	splx(oldpri);
	esdioutb(dcbp->dcb_ioaddr1  + ESDI_BCR, INTEN | DMAEN);
}


/*
 * Wait for controller CMDIN to be cleared.  Will wait
 * approx. 1/4 second if controller CMDIN still not cleared,
 * then will panic.
 * If cleared then output command word.
 */
void
ESDIcmdout(addr)
ushort addr;
{
	int	i, j, len;
	ushort *word;

	word = (ushort *) &ESDI_CB;
	len = (((uint)(ESDI_CB.command & TYPE) >> TYPE_SHIFT) + 1) * 2;
	for (j = 0; j < len; j++) {
		for (i = HDTIMOUT; i > 0; i--) {
			if (!(esdiinb(addr + ESDI_BSR) & CMDIN))
				break;
			tenmicrosec();
		}
		esdioutw((unsigned int)(addr+ESDI_CIR), (unsigned int)word[j]);
	}
}

mcfind_drive(ioaddr, drive)
ushort ioaddr;
int drive;
{
	int i;
	unchar status;
	unchar idrive, id;
	int mystatus = 0;

	ESDI_CB.command = GET_DEV_CFG | (drive << DEVICE_SHIFT) | 
	    RESERVED6 | TYPE_2;
	ESDI_CB.blocks = 0;
	ESDI_CB.RBA = 0;

	ESDIwait(ioaddr); /* wait for controller not busy */
	esdioutb(ioaddr + ESDI_ATTN, AR_COMMAND | (drive << DEVICE_SHIFT));

	/* make sure that we don't have an interrupt here already 
	 * if we have an interrupt this means that the drive doesn't exist
	 * and we shouldn't send it the rest of the command
	 */
	for (i = HDTIMOUT/500; i > 0; i--) {
		if (esdiinb(ioaddr + ESDI_BSR) & IRPT) {
			if( (status=esdiinb(ioaddr + ESDI_ISR)) & 0xf) {
				return (mystatus);
			} else
				printf("ESDI unexpected interrupt while initializing drive\n");
		}
		tenmicrosec();
	}
	ESDIcmdout(ioaddr);
	status = ESDIstatus(ioaddr);
	idrive = status & DEVICE_SELECT;
	id = status & INTID;
	if (((idrive >> DEVICE_SHIFT) == drive) &&
	    (id == ID_OK) ||
	    (id == ID_ECC) ||
	    (id == ID_RETRY) ||
	    (id == ID_ECC_RETRY) ||
	    (id == ID_WARNING)) {
		ESDIstin(ioaddr);
		mystatus = 1;
	}
	ESDIwait(ioaddr);
	esdioutb(ioaddr + ESDI_ATTN, (drive << DEVICE_SHIFT) | AR_EOI);
	return (mystatus);
}

/*
 * Wait for controller IRPT to be set.  Will wait
 * approx. 1/4 second if controller IRPT still not set,
 * then will panic.
 * Return the value of the controller interrupt status register.
 */
unchar
ESDIstatus(addr)
ushort addr;
{
	register int		i;

	for (i = HDTIMOUT; i > 0; i--) {
		if (esdiinb(addr + ESDI_BSR) & IRPT)
			return (esdiinb(addr + ESDI_ISR));
		tenmicrosec();
	}
}


/*
 * Wait for controller BUSY and INTPND to be cleared.  Will wait
 * approx. 1/4 second if controller BUSY and INTPND still not cleared,
 * then will panic.
 */
void
ESDIwait(addr)
ushort addr;
{
	register int	i;

	for (i = HDTIMOUT; i > 0; i--) {
		if (!(esdiinb(addr + ESDI_BSR) & (BUSY | INTPND)))
			return;
		tenmicrosec();
	}
}

/*
 * Wait for controller STOUT to be set.  Will wait
 * approx. 1/4 second if controller STOUT still not set,
 * then will panic.
 * If set then input status word.
 */
void 
ESDIstin(addr)
ushort addr;
{
	int	i, j, len;
	ushort *word;

	word = ESDI_SB;
	len = 1;
	for (j = 0; j < len; j++) {
		for (i = HDTIMOUT; i > 0; i--) {
			if (esdiinb(addr + ESDI_BSR) & STOUT)
				break;
			tenmicrosec();
		}
		word[j] = esdiinw(addr + ESDI_SIR);
		if (j == 0) {
			len = (uint)(ESDI_SB[0] & WORD_COUNT) >> WORD_SHIFT;
		}
	}
}


ushort
esdierrmsg(dcbp, dpbp, status)
gdev_dcbp dcbp;
gdev_dpbp dpbp;
unchar    status;
{
	int i, id, oid;
	unchar    drive = dcb_curdriv(dcbp);

	id = status & INTID;
	oid = id;
	if ((id != ID_XFER) && (dpbp->dpb_state ==  DSTA_NORMIO))
		id = 16;
	else if ((id == ID_XFER) && (dpbp->dpb_state != DSTA_NORMIO))
		id = 17;
	else if ((id != ID_FORMAT) && (dpbp->dpb_state == DSTA_FORMAT))
		id = 18;
	else if ((id == ID_FORMAT) && (dpbp->dpb_state != DSTA_FORMAT))
		id = 19;
#ifdef DEBUG
	if(esdiwarning)
		printf("ESDI error: %s\n          drive %d, RBA %uld    status 0x%x\n",
		    ESDIerrmsg[id], drive, ESDI_CB.RBA, status);

	ESDIstin(dcbp->dcb_ioaddr1);
	if(esdiwarning){
		printf("            status block ");
		for (i = 0; i < ((uint)(ESDI_SB[0] & WORD_COUNT)>>WORD_SHIFT); i++)
			printf(" 0x%x", ESDI_SB[i]);
		printf("\n");
	}
#endif
	if((oid & 0x0F) == 0xC){	/* Command Complete with Failure */
		return esdi2errmsg(ESDI_SB[2]);
	} else
		return xlaterr[id];
}

esdi2errmsg(status)
ushort status;
{
	unchar id;
	id  = status & 0xff;

	if(id & 0x80)
		return xlat3err[id & 0x0f];
	else if ((id & 0xf0) == 0  || (id & 0x10) == 1)
		return xlat2err[id];
	else	/* codes 0x19 - 80  and 0x87 - 0xff are reserved */
		return DERR_UNKNOWN;
}


esdi_chk_isr(dcbp, dpbp, int_id)
gdev_dcbp dcbp;
gdev_dpbp dpbp;
unchar *int_id;
{
	unchar drive, id, status;
	unchar sys_ctrl_A;

	status = ESDIstatus(dcbp->dcb_ioaddr1);
	drive = (uint)(status & DEVICE_SELECT) >> DEVICE_SHIFT;
	id = status & INTID;
	*int_id = id;

	if (((dcbp->dcb_flags & CFLG_BUSY) == 0) || 
	    (drive != dcb_curdriv(dcbp))) {
		if ((id != ID_XFER) && (id != ID_ATTN_ERROR)) {
			/* wait for controller not busy			*/
			ESDIwait(dcbp->dcb_ioaddr1);
			esdioutb(dcbp->dcb_ioaddr1 + ESDI_ATTN, drive | AR_EOI);
		}
	}

	if (!((id == ID_OK) ||
	    (id == ID_ECC) ||
	    (id == ID_RETRY) ||
	    (id == ID_ECC_RETRY) ||
	    (id == ID_WARNING) ||
	    ((id == ID_XFER)  && (dpbp->dpb_state == DSTA_NORMIO ))||
	    ((id == ID_FORMAT) && (dpbp->dpb_state == DSTA_FORMAT)))) {
		/*		turn the disk light off					*/
		sys_ctrl_A = inb(DISKLIGHT_REG);
		outb(DISKLIGHT_REG, sys_ctrl_A & (~(0x80 >> drive)));
		dpbp->dpb_drverror = esdierrmsg(dcbp, dpbp, status);
		if ((id != ID_XFER) && (id != ID_ATTN_ERROR)) {
			/* wait for controller not busy			*/
			ESDIwait(dcbp->dcb_ioaddr1);
			esdioutb(dcbp->dcb_ioaddr1 + ESDI_ATTN, drive | AR_EOI);
		}
		dpbp->dpb_intret = DINT_ERRABORT;
		return (FAILURE);
	}
	return (SUCCESS);
}
