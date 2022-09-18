/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1991
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

#ident	"@(#)uts-x86at:io/hba/mcis.cf/Space.c	1.9"
#ident	"$Header: $"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/immu.h>
#include <sys/errno.h>
#include <sys/cmn_err.h>
#include <sys/buf.h>
#include <sys/signal.h>
#include <sys/user.h>
#include <sys/bootinfo.h>
#include <sys/dma.h>
#include <sys/vtoc.h>
#include <sys/sdi_edt.h>
#include <sys/scsi.h>
#include <sys/sdi.h>
#include <sys/mcis.h>
#include "config.h"

/*
** Control the use of cache on the adapter:
**
** - if 1, read and write through the adapter cache
** - if 0, read and write bypass the cache
**
** NOTE: this flag applies only to read and write commands on direct access
** devices.  The cache is not used for tape devices because of their variable
** block size value; for other scsi commands, they all bypass the adapter
** cache
*/

int	mcis_cache = 0;	
int	mcis_global_tout = 600;	/* Number of seconds used for global	*/
				/* timeout on the mcis SCSI HBA	board.	*/
				/* The timeout value is increased from	*/
				/* the default of 45 sec. to account for*/
				/* the length of time needed for SCSI	*/
				/* tape retensioning.			*/

struct	hba_idata	mcis_idata[MCIS__CNTLS]	= {
#ifdef	MCIS__0
	{ 1, "(mcis,1st) MCIS SCSI HBA",
	  7, MCIS__0_SIOA, MCIS__0_CHAN, MCIS__0_VECT, MCIS__0, 0 }
#endif
#ifdef	MCIS__1
	,
	{ 1, "(mcis,2nd) MCIS SCSI HBA",
	  7, MCIS__1_SIOA, MCIS__1_CHAN, MCIS__1_VECT, MCIS__1, 0 }
#endif
#ifdef	MCIS__2
	,
	{ 1, "(mcis,3rd) MCIS SCSI HBA",
	  7, MCIS__2_SIOA, MCIS__2_CHAN, MCIS__2_VECT, MCIS__2, 0 }
#endif
#ifdef	MCIS__3
	,
	{ 1, "(mcis,4th) MCIS SCSI HBA",
	  7, MCIS__3_SIOA, MCIS__3_CHAN, MCIS__3_VECT, MCIS__3, 0 }
#endif
};

int	mcis__cntls	= MCIS__CNTLS;

/*
** The remainder of this space.c file is strictly for debugging
*/

/*
#define MCIS_PRDEBUG
*/

#ifdef MCIS_PRDEBUG
#define WATERM_H        100
#define WATERM_L        -100

mcisp_index(addr)
int	addr;
{
	printf("mcis: print list\n");
	printf("mcisp_xsb 	mcisp_scs	mcisp_scm\n");
	printf("mcisp_dcb	mcisp_dev	mcisp_scb\n");
}

struct dev_blk *mcis_devp;
mcisp_dev(addr)
int	addr;
{
	struct dev_blk *devp;

	if (addr==NULL) {
		if (mcis_devp == NULL)
			return;
		addr = (int)mcis_devp;
	}

	mcis_devp = devp = (struct dev_blk *)addr;
	printf("dev_blk (size= 0x%x)\n", sizeof(struct dev_blk));
	printf("DEV_BLK tp_flag= 0x%x tp_devtype= 0x%x tp_devsubtype= 0x%x\n",
		devp->DTP.tp_flag, devp->DTP.tp_devtype, 
		devp->DTP.tp_devsubtype);
	printf("secsiz= 0x%x maxblk= 0x%x minblk= 0x%x density= 0x%x\n",
		devp->DTP.tp_secsiz, devp->DTP.tp_maxblk, devp->DTP.tp_minblk, 
		devp->DTP.tp_density);
	printf("tp_msp= 0x%x tp_opsp= 0x%x \n", 
		devp->DTP.tp_msp, devp->DTP.tp_opsp);
}

gdev_dcbp mcis_dcbp;
mcisp_dcb(addr)
int	addr;
{
	gdev_dcbp dcbp;
	struct	mcis_blk *blkp;
	int	i;

	if ((addr>WATERM_L) && (addr < WATERM_H)) {
		if (mcis_dcbp == NULL)
			return;
		addr = (int)mcis_dcbp + (addr * sizeof(struct gdev_ctl_block));
	}

	mcis_dcbp = dcbp = (gdev_dcbp)addr;
	printf("DCB conf=0x%x ops=0x%x private=0x%x targetid=0x%x\n",
		dcbp->dcb_hba_conf, dcbp->dcb_hba_ops, dcbp->dcb_hba_private,
		dcbp->dcb_hba_targetid);
	blkp = dcb_mcis_blkp(dcbp);
	printf("MCIS intr_code= 0x%x intr_dev= 0x%x allocmem= 0x%x\n",
		blkp->b_intr_code, blkp->b_intr_dev, blkp->b_allocmem);
	printf("targetid= %d btarg= %d ioaddr= 0x%x dmachan= 0x%x b_intr= %d\n",
		blkp->b_targetid, blkp->b_boottarg, blkp->b_ioaddr, 
		blkp->b_dmachan, blkp->b_intr);
	printf("ldcnt= %d numdev= %d scb_cnt= 0x%x b_scbp= 0x%x\n",
		blkp->b_ldcnt, blkp->b_numdev, blkp->b_scb_cnt, blkp->b_scbp);
	printf("LDEV HEAD INDEX=(%d) ld_avfw= 0x%x ld_avbk= 0x%x\n",
		((int)ldmhd_fw(blkp)-(int)(blkp->b_ldm))/
		sizeof(struct mcis_ldevmap), ldmhd_fw(blkp), ldmhd_bk(blkp));
	for (i=0; i<blkp->b_ldcnt; i++) {
		printf("LD[%d,x%x] avfw= 0x%x avbk= 0x%x, scbp= 0x%x <%d,%d>\n",
			i, &blkp->b_ldm[i], blkp->b_ldm[i].ld_avfw, 
			blkp->b_ldm[i].ld_avbk, blkp->b_ldm[i].ld_scbp, 
			blkp->b_ldm[i].LD_CB.a_targ,blkp->b_ldm[i].LD_CB.a_lun);
	}
}


struct	gdev_parm_block *mcis_dpbp;
mcisp_xsb(addr)
int	addr;
{
	struct	gdev_parm_block *dpbp;
	struct	xsb *xsbp;

	printf("xsb (size= 0x%x)\n", sizeof(struct xsb));
	if ((addr>WATERM_L) && (addr < WATERM_H)) {
		if (mcis_dpbp == NULL)
			return;
		addr = (int)mcis_dpbp + (addr * sizeof(struct gdev_parm_block));
	}

	mcis_dpbp = dpbp = (struct gdev_parm_block *)addr;
	printf("DPB=0x%x scep=0x%x cmdxsbp= 0x%x fltxsbp= 0x%x sc_addr= 0x%x\n",
		dpbp, dpbp->dpb_hba_sce, dpbp->dpb_hba_cmdxsb, dpbp->dpb_hba_fltxsb, dpbp->dpb_hba_addr);
	printf("DPB xfer= 0x%x dcbp= 0x%x devp= 0x%x\n", dpbp->dpb_hba_xfer,
			dpbp->dpb_hba_dcb, dpbp->dpb_hba_dev);
	xsbp= dpb_hba_cmdxsbp(dpbp);
	printf("CMDXSB hbadata_p= 0x%x dpb_p= 0x%x\n", xsbp->hbadata_p,
		HBA_dpb_p(xsbp));
	printf("sc_cmdpt=0x%x sc_datapt= 0x%x sc_cmdsz= 0x%x sc_datasz= 0x%x\n",
		xsbp->SCB.sc_cmdpt, xsbp->SCB.sc_datapt, xsbp->SCB.sc_cmdsz,
		xsbp->SCB.sc_datasz);
	printf("sc_comp_code=0x%x sc_cb= 0x%x sa_lun= 0x%x target= 0x%x\n",
		xsbp->SCB.sc_comp_code, xsbp->SCB.sc_cb, 
		XSB_lun(xsbp), XSB_targ(xsbp));
	xsbp= dpb_hba_fltxsbp(dpbp);
	printf("FLTXSB hbadata_p= 0x%x dpb_p= 0x%x\n", xsbp->hbadata_p,
		HBA_dpb_p(xsbp));
	printf("sc_cmdpt=0x%x sc_datapt= 0x%x sc_cmdsz= 0x%x sc_datasz= 0x%x\n",
		xsbp->SCB.sc_cmdpt, xsbp->SCB.sc_datapt, xsbp->SCB.sc_cmdsz,
		xsbp->SCB.sc_datasz);
	printf("sc_comp_code=0x%x sc_cb= 0x%x sa_lun= 0x%x target= 0x%x\n",
		xsbp->SCB.sc_comp_code, xsbp->SCB.sc_cb, 
		XSB_lun(xsbp), XSB_targ(xsbp));
}


struct	mcis_scb *mcis_scbp;
mcisp_scb(addr)
int	addr;
{
	struct mcis_scb *bp;
	struct mcis_tsb *tsbp;

	printf("mcis_scb (size= 0x%x)\n", sizeof(struct mcis_scb));
	printf("mcis_tsb (size= 0x%x)\n", sizeof(struct mcis_tsb));

	if ((addr>WATERM_L) && (addr < WATERM_H)) {
		if (mcis_scbp == NULL)
			return;
		addr = (int)mcis_scbp + (addr * sizeof(struct mcis_scb));
	}
	mcis_scbp = bp = (struct  mcis_scb *)addr;
	tsbp = (struct mcis_tsb *)&(bp->s_tsb);

	printf("SCB cmdop= 0x%x ns= %d nd=%d op= 0x%x baddr= 0x%x\n",
		bp->s_cmdop, bp->s_ns, bp->s_nd, bp->MSCB_op, bp->s_baddr);
	printf("hostaddr= 0x%x hostcnt= 0x%x bcnt= 0x%x blen= %d\n",
		bp->s_hostaddr, bp->s_hostcnt, 
		*((ushort *)&bp->s_cdb[0]), *((ushort *)&bp->s_cdb[2]));
	printf("ADDR tsb= 0x%x dmaseg= 0x%x cdb= 0x%x <%d:%d> blksz=%d\n",
		bp->s_tsbp, &bp->s_dmaseg[0], bp->s_cdb, bp->s_targ, bp->s_lun,
		bp->s_blksz);
	printf("ownerp= 0x%x status= 0x%x retry= %d s_dmp= 0x%x sp= 0x%x\n",
		bp->s_ownerp, bp->s_status, bp->s_retry, bp->s_dmp, bp->s_sp);

	printf("\nTSB status=0x%x resid=0x%x targlen= 0x%x targstat= 0x%x\n",
		tsbp->t_status, tsbp->t_resid,tsbp->t_targlen,tsbp->t_targstat);
	printf("hastat= 0x%x targerr= 0x%x haerr= 0x%x\n",
		tsbp->t_hastat, tsbp->t_targerr, tsbp->t_haerr);
	printf("crate= 0x%x cstat= 0x%x\n", tsbp->t_crate, tsbp->t_cstat);

}
struct	scs *mcis_scsp;
mcisp_scs(addr)
int	addr;
{
	struct	scs *scsp;

	printf("scs (size= 0x%x)\n", sizeof(struct scs));
	mcis_scsp = scsp = (struct  scs *)addr;
	printf("SCS op= 0x%x addr1= 0x%x addr= 0x%x lun=0x%x len= 0x%x cont= 0x%x\n",
		scsp->ss_op, scsp->ss_addr1, gsc_fromsc_short(scsp->ss_addr), 
		scsp->ss_lun, scsp->ss_len, scsp->ss_cont);
		
}

struct	scm *mcis_scmp;
mcisp_scm(addr)
int	addr;
{
	struct	scm *scmp;

	printf("scm (size= 0x%x)\n", sizeof(struct scm));
	mcis_scmp = scmp = (struct  scm *)addr;
	printf("SCS op= 0x%x addr= 0x%x lun=0x%x len= 0x%x cont= 0x%x\n",
		scmp->sm_op, gsc_fromsc_int(scmp->sm_addr), scmp->sm_lun, 
		gsc_fromsc_short(scmp->sm_len), scmp->sm_cont);
		
}


struct	mcis_scb *mcis_scbp;
mcisp_ccb(addr)
int	addr;
{
	struct	mcis_scb *scbp;

	printf("mcis_scb (size= 0x%x)\n", sizeof(struct mcis_scb));
	if ((addr>WATERM_L) && (addr < WATERM_H)) {
		if (mcis_scbp == NULL)
			return;
		addr = (int)mcis_scbp + (addr * sizeof(struct mcis_scb));
	}

	mcis_scbp = scbp = (struct mcis_scb *)addr;
	printf("MCIS SCB=0x%x  cmdop=0x%x cmdsup=0x%x op= 0x%x \n",
		mcis_scbp, scbp->s_cmdop, scbp->s_cmdsup, scbp->MSCB_op);
}

#endif
