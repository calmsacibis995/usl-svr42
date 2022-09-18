/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/hba/dpt.c	1.40"
#ident	"$Header: miked 1/29/92 $"

/*      Copyright (c) 1988, 1989, 1990, 1991  Intel Corporation     */
/*	All Rights Reserved	*/

/*      INTEL CORPORATION PROPRIETARY INFORMATION                          */
/*                                                                         */
/*	This software is supplied under the terms of a license agreement   */
/*	or nondisclosure agreement with Intel Corporation and may not be   */
/*	copied or disclosed except in accordance with the terms of that    */
/*	agreement.							   */
/*                                                                         */
/*      Copyright (c) 1990 Distributed Processing Technology               */
/*      All Rights Reserved                                                */

/****************************************************************************
**      ISA/EISA SCSI Host Adapter Driver for DPT PM2011/PM2012 A/B        **
*****************************************************************************/

#include <svc/errno.h>
#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <util/cmn_err.h>
#include <fs/buf.h>
#include <mem/immu.h>
#include <svc/systm.h>
#include <io/mkdev.h>
#include <io/conf.h>
#include <proc/cred.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <svc/bootinfo.h>
#include <util/debug.h>
#include <io/target/scsi.h>
#include <io/target/sdi_edt.h>
#include <io/target/sdi.h>
#include <io/target/dynstructs.h>
#include <io/hba/dpt.h>
#include <util/mod/moddefs.h>
#include <io/ddi.h>
#include <io/ddi_i386at.h>

/*
#define DPT_DEBUG
*/

extern  struct	hba_idata	dptidata[];
extern	int	dpt_cntls;

#define		DRVNAME	"dpt - DPT SCSI HBA driver"

STATIC	int	dpt_load(), dpt_unload();
int	dptinit();
void	dptstart();
void	dpthalt();

MOD_HDRV_WRAPPER(dpt, dpt_load, dpt_unload, dpthalt, DRVNAME);

static	int	mod_dynamic = 0;

STATIC int
dpt_load()
{
	mod_dynamic = 1;
	if( dptinit() ) {
		return(ENODEV);
	}
	mod_drvattach(&dpt_attach_info);
	dptstart();
	return(0);
}

STATIC int
dpt_unload()
{
	return(EBUSY);
}

HBA_INFO(dpt, 0, 0x1f000);

int dptdevflag = D_NEW | D_DMA;
/* pool for dynamic struct allocation for dpt_sblk_t */
extern struct head	sm_poolhead;

/* Allocated in space.c */
extern unsigned int     dpt_sc_hacnt;	/* Total # of controllers declared*/
extern int              dpt_ctlr_id;	/* hba scsi id			*/
extern scsi_stat_t      dpt_sc_hastat[];/* SCSI HA Status Packets	*/
extern struct ver_no    dpt_sdi_ver;	/* SDI version structure	*/
extern long             sdi_started;	/* SDI initialization flag	*/

struct dpt_scsi_ha   	*dpt_sc_ha;	/* SCSI HA structures		*/
int dpt_init_flag = 0;	/* UPDATE */

dpt_dma_t        *dpt_dmalistpool;	/* pointer to DMA list pool     */
dpt_dma_t        *dpt_dfreelist;	/* List of free DMA lists       */
char             dpt_sc_cmd[MAX_CMDSZ]; /* local SCSI command buffer    */
char             dptinit_time;		/* Init time (poll) flag        */
int		 dpt_gtol[MAX_HAS];	/* xlate global hba# to loc */
int		 dpt_ltog[MAX_HAS];	/* local hba# to global     */
int		 dpt_hacnt;		/* # hba's found */
struct RdConfig  eata_cfg;		/* EATA Read Config Data struc  */

struct  dpt_ccb *dpt_getblk();		/* Alloc controller cmd block struct */
void    dpt_freeblk();			/* Release SCSI command block.       */
void    dpt_init_cps();			/* Init controller CP free list.     */
void    dpt_pass_thru();		/* Send pass-thru job to the HA.     */
void    dpt_int();			/* Int Handler for pass-thru jobs.   */
void    dpt_putq();			/* Put job on Logical Unit Queue.    */
void    dpt_next();			/* Send next Logical Unit Queue Job. */
void    dpt_cmd();			/* Create/Send an SCB associated cmd */
void    dpt_func();			/* Create/Send an SFB associated cmd */
void    dpt_send();			/* Send command to the HA board.     */
void    dpt_done();			/* Intr done for CP and SCB.         */
void    dpt_timer();			/* Handles Controller command timing */
void	dpt_flushq();
int     dpt_ha_init();			/* Initialize HA communication       */
void    dpt_ha_done();			/* Intr done for CP and SFB.         */

extern int	dpt_lu_max;	/* For each LU, the max number */
				/* of jobs on the dpt card concurrently */

extern int	dpt_hba_max;	/* Maximum number of jobs that */
				/* allowed on the HBA at one time */
int	dpt_active_jobs = 0;	/* total # of jobs on the board */

/***
** Halt Routine Changes
***/
int	dpt_flush_in_progress = FALSE;
int	dpt_flush_intr = FALSE;


/*
** Function name: dptstart()
** Description:
**	Called by kernel to perform driver initialization
**	after the kernel data area has been initialized.
*/

void
dptstart()
{
	/* UPDATE:
		if (! dpt_init_flag) {
			return;
		}
	*/
	/*
	 * Clear init time flag to stop the HA driver
	 * from polling for interrupts and begin taking
	 * interrupts normally.
	 */
	dptinit_time = FALSE;
}

void
dpthalt()
{
	register int	c, lun;
	register struct dpt_ccb		*cp;
	register struct dpt_scsi_ha	*ha;
	static BYTE	dpt_halt_done = 0;
	extern dpt_flush_time_out;

	if (dpt_halt_done) {
		return;
	}

	/********************************************************************
	** For each DPT HBA in the system Flush and invalidate cache for   **
	** all SCSI ID/LUNs by issuing a SCSI ALLOW MEDIA REMOVAL command. **
	** This will cause the HBA to flush all requests to the device and **
	** then invalidate its cache for that device.  This code operates  **
	** as a single thread, only one device is flushed at a time.  It   **
	** probably wont buy anything to send all of them at once, this is **
	** only run at Shutdown time.					   **
	********************************************************************/

	dpt_lu_max = 1;
	dpt_hba_max = 1;

	for (c = 0; c < dpt_sc_hacnt; c++) {
		cp = dpt_getblk(c);
		cp->c_time = 0;

		/***
		** If the controller is not active, no need
		** to try flushing data 8-).
		***/
		if (!dptidata[c].active) {
			continue;
		}

		/***
		** Build the EATA Command Packet structure
 		** for a SCSI Prevent/Allow Cmd
		***/
		cp->CPop.byte		= 0;
		cp->CPcdb[0]		= SS_LOCK; 
		*(ulong *)&cp->CPcdb[1]	= 0;
		cp->CPcdb[5]		= 0;
		cp->CPdataDMA 		= 0L;

		ha = &dpt_sc_ha[c];

		cmn_err(CE_NOTE,"%s: Flushing cache, if present.",dptidata[c].name);
		for (cp->CPID=0; cp->CPID<=7; cp->CPID++) {
			for ( lun=0; lun<= 7; lun++) {

			     if( sdi_redt(c, cp->CPID, lun) == (struct sdi_edt *)0 ) {
				     continue;
			     }

			     cp->CPmsg0 = (HA_IDENTIFY_MSG | HA_DISCO_RECO)+lun;
			     dpt_flush_in_progress = TRUE;
			     dpt_flush_intr = FALSE;
			     cp->c_active = TRUE;
			     scsi_send_cmd(ha->ha_base,cp->c_addr,CP_DMA_CMD);
			     if( dpt_wait( c, dpt_flush_time_out ) == FAILURE ) {
				cmn_err(CE_NOTE,"%s: Incomplete Flush - Target %d, LUN %d.",dptidata[c].name, cp->CPID,lun);
			     }

			     dpt_flush_in_progress = FALSE;
			     dpt_flush_intr = FALSE;
			}
		}

		cmn_err(CE_NOTE,"%s: Flushing complete.",dptidata[c].name);

		dpt_freeblk(c, cp);	/* Release SCSI command block. */
	}

	dpt_halt_done = 1;

}

/*
** Function name: dptopen()
** Description:
** 	Driver open() entry point. It checks permissions, and in the
**	case of a pass-thru open, suspends the particular LU queue.
*/

int
dptopen(devp, flags, otype, cred_p)
dev_t	*devp;
cred_t	*cred_p;
int	flags;
int	otype;
{
	dev_t	dev = *devp;
	register int	c = dpt_gtol[SC_HAN(dev)];
	register int	t = SC_TCN(dev);
	register int	l = SC_LUN(dev);
	register struct dpt_scsi_lu *q;
	int  oip;

	/* UPDATE
		if (! dpt_init_flag) {
			dpt_init_flag = 1;
			printf("dptopen: dptinit(c) return value = \n", dptinit(c));
			dptstart();
		}
	*/

	if (cred_p->cr_uid != 0) {
		return(EPERM);
	}
	if (t == dpt_sc_ha[c].ha_id)
		return(0);

	/* This is the pass-thru section */

	q = &LU_Q(c, t, l);

	oip = spl5();
	if ((q->q_count > 0) ||
		(q->q_active == dpt_lu_max) ||
		(q->q_flag & (QSUSP | QPTHRU)))
	{
		splx(oip);
		return(EBUSY);
	}

	q->q_flag |= QPTHRU;
	splx(oip);
	return(0);
}


/*
** Function name: dptclose()
** Description:
** 	Driver close() entry point.  In the case of a pass-thru close
**	it resumes the queue and calls the target driver event handler
**	if one is present.
*/

int
dptclose(dev, flags, otype, cred_p)
cred_t	*cred_p;
dev_t	dev;
int	flags;
int	otype;
{
	register int	c = dpt_gtol[SC_HAN(dev)];
	register int	t = SC_TCN(dev);
	register int	l = SC_LUN(dev);
	register struct dpt_scsi_lu *q;
	int  oip;

	if (dpt_illegal(SC_HAN(dev), t, l, 0)) {
		return(ENXIO);
	}

	if (t == dpt_sc_ha[c].ha_id)
		return(0);

	q = &LU_Q(c, t, l);

	oip = spl5();
	q->q_flag &= ~QPTHRU;

	if (q->q_func != NULL)
		(*q->q_func) (q->q_param, SDI_FLT_PTHRU);

	dpt_next(q);
	splx(oip);
	return(0);
}



/*
** Function name: dptioctl()
** Description:
**	Driver ioctl() entry point.  Used to implement the following
**	special functions:
**
**	SDI_SEND     -	Send a user supplied SCSI Control Block to
**			the specified device.
**	B_GETTYPE    -  Get bus type and driver name
**	B_HA_CNT     -	Get number of HA boards configured
**	REDT	     -	Read the extended EDT from RAM memory
**	SDI_BRESET   -	Reset the specified SCSI bus
*/

int
dptioctl(dev, cmd, arg, mode, cred_p, rval_p)
cred_t	*cred_p;
int	*rval_p;
dev_t	dev;
int	cmd;
caddr_t	arg;
int	mode;
{
	register int	c = dpt_gtol[SC_HAN(dev)];
	register int	t = SC_TCN(dev);
	register int	l = SC_LUN(dev);
	register struct sb *sp;
	int  oip;

	/* change #1 */
	if (dpt_illegal(SC_HAN(dev), t, l, 0)) {
		return(ENXIO);
	}

	switch(cmd)
	{
	case SDI_SEND:
		{
			register buf_t *bp;
			struct sb  karg;
			int  errnum, rw;
			char *save_priv;

			if (t == dpt_sc_ha[c].ha_id) { 	/* illegal ID */
				return(ENXIO);
			}
			if (copyin(arg, (caddr_t)&karg, sizeof(struct sb))) {
				return(EFAULT);
			}
			if ((karg.sb_type != ISCB_TYPE) ||
			    (karg.SCB.sc_cmdsz <= 0 )   ||
			    (karg.SCB.sc_cmdsz > MAX_CMDSZ )) {
				return(EINVAL);
			}

			sp = sdi_getblk();
			save_priv = sp->SCB.sc_priv;
			bcopy((caddr_t)&karg, (caddr_t)sp, sizeof(struct sb));

			bp = getrbuf(KM_SLEEP);
			oip = spl5();
			bp->b_iodone = NULL;
			sp->SCB.sc_priv = save_priv;
			sp->SCB.sc_wd = (long)bp;
			sp->SCB.sc_cmdpt = dpt_sc_cmd;

			if (copyin(karg.SCB.sc_cmdpt, sp->SCB.sc_cmdpt,
			    sp->SCB.sc_cmdsz)) {
				freerbuf(bp);
				return(EFAULT);
			}

			rw = (sp->SCB.sc_mode & SCB_READ) ? B_READ : B_WRITE;
			bp->b_resid = (long)sp;

			/*
			 * If the job involves a data transfer then the
			 * request is done thru physio() so that the user
			 * data area is locked in memory. If the job doesn't
			 * involve any data transfer then dpt_pass_thru()
			 * is called directly.
			 */
			if (sp->SCB.sc_datasz > 0)
			{
				struct iovec  ha_iov;
				struct uio    ha_uio;

				ha_iov.iov_base = sp->SCB.sc_datapt;
				ha_iov.iov_len = sp->SCB.sc_datasz;
				ha_uio.uio_iov = &ha_iov;
				ha_uio.uio_iovcnt = 1;
				ha_uio.uio_offset = 0;
				ha_uio.uio_segflg = UIO_USERSPACE;
				ha_uio.uio_fmode = 0;
				ha_uio.uio_resid = sp->SCB.sc_datasz;

				if (errnum = physiock(dpt_pass_thru, bp, dev, rw,
				    4194303, &ha_uio)) {
					freerbuf(bp);
					return(errnum);
				}
			} else {
				bp->b_un.b_addr = sp->SCB.sc_datapt;
				bp->b_bcount = sp->SCB.sc_datasz;
				bp->b_blkno = NULL;
				bp->b_dev = dev;
				bp->b_flags |= rw;

				dpt_pass_thru(bp);  /* fake physio call */
				iowait(bp);
			}

			/* update user SCB fields */

			karg.SCB.sc_comp_code = sp->SCB.sc_comp_code;
			karg.SCB.sc_status = sp->SCB.sc_status;
			karg.SCB.sc_time = sp->SCB.sc_time;

			if (copyout((caddr_t)&karg, arg, sizeof(struct sb))) {
				freerbuf(bp);
				return(EFAULT);
			}

done:
			freerbuf(bp);
			sdi_freeblk(sp);
			splx(oip);
			break;
		}

	case B_GETTYPE:
		if (copyout("scsi", ((struct bus_type *)arg)->bus_name, 5)) {
			return(EFAULT);
		}
		if (copyout("scsi", ((struct bus_type *)arg)->drv_name, 5)) {
			return(EFAULT);
		}
		break;

	case	HA_VER:
		if (copyout((caddr_t)&dpt_sdi_ver,arg, sizeof(struct ver_no)))
			return(EFAULT);
		break;
	case SDI_BRESET:
		{
			register struct dpt_scsi_ha *ha = &dpt_sc_ha[c];

			oip = spl5();
			if (ha->ha_npend > 0)     /* jobs are outstanding */
				return(EBUSY);
			else {
				cmn_err(CE_WARN,
					"!DPT Host Adapter: HA %d - Bus is being reset\n", dpt_ltog[c]);
				outb((dpt_sc_ha[c].ha_base + HA_COMMAND), CP_EATA_RESET);
				drv_usecwait(1000);

			}
			splx(oip);
			break;
		}

	default:
		return(EINVAL);
	}

	return(0);
}


/*
** Function name: dptintr()
** Description:
**      Driver interrupt handler entry point.
**      Called by kernel when a host adapter interrupt occurs.
*/

void
dptintr(vect)
unsigned int  vect;	/* HA interrupt vector	*/
{
	register struct dpt_scsi_ha	 *ha;
	register struct dpt_ccb      *cp;
	register scsi_stat_t     *SP;
	register int             c,stat;
	register char *		 message = "";

	/********************************************************************
	** Determine which host adapter interrupted from the interrupt     **
	** vector, Auxiliary Status register AUX_INTR flag.                **
	*********************************************************************/
	for (c = 0; c < dpt_sc_hacnt; c++) {
		ha = &dpt_sc_ha[c];
		if (ha->ha_vect == vect) {
			if (!dptidata[c].active)
				continue;
			if ( inb(ha->ha_base + HA_AUX_STATUS) & HA_AUX_INTR ) {
				ha->ha_npend--;
				break;
			}
		}
	}
	if ( c == dpt_sc_hacnt ) {
#ifdef DPT_DEBUG
		cmn_err(CE_NOTE,"DPT inactive HBA received an interrupt");
#endif
		return;
	}

	SP = &dpt_sc_hastat[c];		/* Set Status Packet pointer.  */

	if( (SP->SP_Controller_Status & 0x80) != 0x80 ) {
		/** Spurious? **/
		cmn_err(CE_NOTE,"!%s: Spurious Interrupt; EOC not set.",dptidata[c].name);

		return;
	}

	if( SP->CPaddr.vp == 0 ) {
		/** Spurious? **/
		cmn_err(CE_NOTE,"!%s: Spurious Interrupt; CCB pointer not set.",dptidata[c].name);
		return;
	}

	cp = SP->CPaddr.vp;		/* Get Command Packet Vpointer.*/
	cp->c_active = FALSE;		/* Mark it not active.         */

	cp->CP_Controller_Status  = SP->SP_Controller_Status & HA_STATUS_MASK;
	cp->CP_SCSI_Status        = SP->SP_SCSI_Status;
	SP->SP_Controller_Status = 0;

	stat = inb(ha->ha_base + HA_STATUS); /* Read Controller Status Reg. */

	if( ( dpt_flush_in_progress == TRUE ) && ( cp->CPcdb[0] == SS_LOCK ) ) {
		dpt_flush_intr = TRUE;
		return;
	}

	if (cp->c_bind == NULL) {
		cmn_err(CE_NOTE,"!%s: Spurious Interrupt; c_bind not set.",dptidata[c].name);
		return;
	}

	if (cp->c_bind->sb_type == SFB_TYPE) {
		dpt_ha_done(c, cp, stat);
	}
	else {
		dpt_done(c, cp, stat);
	}

	return;
}


/*
** Function name: dpt_done()
** Description:
**	This is the interrupt handler routine for SCSI jobs which have
**	a controller CB and a SCB structure defining the job.
*/

void
dpt_done(c, cp, status)
int		c;	  	/* HA controller number */
register int	status;		/* HA status */
struct dpt_ccb	*cp;		/* command block */
{
	register struct dpt_scsi_lu  *q;
	register struct sb *sp;
	ulong curtime;

	/* CPID holds t and CPmsg0 holds l  */
	q = &LU_Q(c, (char)cp->CPID, (char)(cp->CPmsg0 & 0x07));

	sp = cp->c_bind;

	ASSERT(sp);

	q->q_flag &= ~QSENSE;                /* Old sense data now invalid  */

	/** Determine completion status of the job ***/

	if ( !(status & HA_ST_ERROR)  && cp->CP_Controller_Status == 0 ) {

		sp->SCB.sc_comp_code = SDI_ASW;

	} else {			    /* Error bit WAS set in status */

		if (cp->CP_Controller_Status) {
			sp->SCB.sc_status = cp->CP_SCSI_Status;

			switch (cp->CP_Controller_Status) {
			case HA_ERR_SELTO:
				sp->SCB.sc_comp_code = SDI_RETRY;
				break;
			case HA_ERR_RESET:
				sp->SCB.sc_comp_code = SDI_RESET;
				break;
			case HA_ERR_INITPWR:
				sp->SCB.sc_comp_code = SDI_ASW;
				break;
			case HA_ERR_SHUNG:
				sp->SCB.sc_comp_code = SDI_SBUSER;
				break;

			default:
				if (cp->CP_SCSI_Status == SC_NOSENSE)
					sp->SCB.sc_comp_code = SD_NOSENSE;
				else
					sp->SCB.sc_comp_code = SDI_CKSTAT;
			}

		} else if (cp->CP_SCSI_Status) {
			sp->SCB.sc_status = cp->CP_SCSI_Status;
			if (cp->CP_SCSI_Status == SC_SELFAIL) {
				sp->SCB.sc_comp_code = SDI_NOSELE;
			} else if (cp->CP_SCSI_Status == SC_LUCOMM) {
				sp->SCB.sc_comp_code = SDI_TCERR;
			} else {
				sp->SCB.sc_comp_code = SDI_CKSTAT;
			}

			/* Cache request sense info into QueSense  */
			bcopy((caddr_t)(cp->sense),
			    (caddr_t)(&q->q_sense)+1, sizeof(q->q_sense)-1);
			q->q_flag |= QSENSE;

		} else {
			sp->SCB.sc_comp_code = SD_NOSENSE;
		}
	}
	drv_getparm(LBOLT, (ulong *)&curtime);

	/***
	** Due to a problem with the DPT 2012/B HBA when doing the
	** initial access to media (SS_TEST), we will not suspend
	** the queue if the current failure is a result of these
	** commands.
	***/
	if( ((struct scs *)sp->SCB.sc_cmdpt)->ss_op != SS_TEST &&
	    ((struct scs *)sp->SCB.sc_cmdpt)->ss_op != SS_INQUIR ) {

		if ((sp->SCB.sc_comp_code & SDI_SUSPEND) &&
		   (sp->SCB.sc_int != dpt_int)) {
			/***
			** The SDI Suspend bit is set, and we are
			** not "pass-thru", so suspend the queue.
			***/
			q->q_flag |= QSUSP;
		}
	}

	sdi_callback(sp);	/* call target driver interrupt handler */

	dpt_freeblk(c, cp);	/* Release SCSI command block. */

	dpt_active_jobs--;	/* One less job on the board */
	q->q_active--;		/* for this queue in particular */
	dpt_next(q);		/* Send Next Job on the Queue. */
}


/*
** Function name: dpt_ha_done()
** Description:
**	This is the interrupt handler routine for SCSI jobs which have
**      a controller CP and a SFB structure defining the job.
*/

void
dpt_ha_done(c, cp, status)
int		c;		/* HA controller number */
int		status;		/* HA mbx status */
struct dpt_ccb	*cp;		/* command block */
{
	register struct dpt_scsi_lu  *q;
	register struct sb *sp;

	/* CPID holds t and CPmsg0 holds l  */
	q = &LU_Q(c, (char)cp->CPID, (char)(cp->CPmsg0 & 0x07));

	sp = cp->c_bind;

	ASSERT(sp);

	/* Determine completion status of the job */

	if ( (status & HA_ST_ERROR) && cp->CP_Controller_Status != S_GOOD ) {
		sp->SFB.sf_comp_code = SDI_HAERR;
		q->q_flag |= QSUSP;
	} else {
		sp->SFB.sf_comp_code = SDI_ASW;
		/* UPDATE: ad1542.c calls adsc_flushq(q, SDI_CRESET, 0); here */
	}

	sdi_callback(sp);	/* call target driver interrupt handler */

	dpt_freeblk(c, cp);	/* Release SCSI command block. */

	dpt_active_jobs--;	/* One less job on the board */
	q->q_active--;		/* for this queue in particular */

	dpt_next(q);		/* Send Next Job on the Queue. */
}



/*
** Function name: dpt_timer()
** Description:
**	Scheduled at minute intervals to perform timing.  Callout
**	routines (like this one) run at hi priority.  No spl necessary.
*/

void
dpt_timer(c)
{
	register struct dpt_ccb *cp;
	register int n;
	ulong curtime;

	cp = &dpt_sc_ha[c].ha_ccb[0];	/* Get controller dpt_ccb pointer. */
	for (n = 0; n < NCPS; n++, cp++)
	{
		if (!cp->c_active)	/* If no command pending       */
			continue;	/*      or                     */
		if (cp->c_time == 0)	/* command not being timed     */
			continue;	/* then just continue.         */

		drv_getparm(LBOLT, (ulong *)&curtime);
		if ((cp->c_time + cp->c_start) > curtime)
			continue;	/* Not timed out yet.          */

		/******************************************
		** Job timed out - terminate the command **
		*******************************************/
		cp->c_time = 0;		/* Set as not being timed.     */
		dpt_send(c, ABORT, cp);	/* Send ABORT Command.         */
	}

	(void) timeout(dpt_timer, (caddr_t)c, 60*HZ);
}


/*===========================================================================*/
/* SCSI Driver Interface (SDI-386) Functions
/*===========================================================================*/


/*
** Function name: dptinit()
** Description:
**	This is the initialization routine for the DPT HA driver.
**	Called by the main init loop if static or dpt_load if loadable.
*/

int
dptinit()
{
	register struct dpt_scsi_ha *ha;
	register struct dpt_scsi_lu *q;
	register dpt_dma_t  *dp;
	int  i, j, c;
	unsigned  ha_structsize, tab_structsize, ccb_structsize;
	unsigned  luq_structsize, dma_listsize;
	uint	bus_p;
	static dptfirst_time = 1;
	int sleepflag, cntl_num;

	dptinit_time = TRUE;

	/* if running in a micro-channel machine, skip initialization */
	if (!drv_gethardware(IOBUS_TYPE, &bus_p) && (bus_p == BUS_MCA)) {
		dpt_sc_ha = NULL;
		return(-1);
	}

	if (!dptfirst_time) {
		cmn_err(CE_WARN,"!DPT: Already initialized.");
		return(-1);
	}

	for (i=0; i < MAX_HAS; i++) {
		dpt_gtol[i] = dpt_ltog[i] = -1;
	}
	sdi_started = TRUE;

	sleepflag = mod_dynamic ? KM_SLEEP : KM_NOSLEEP;
	dpt_sdi_ver.sv_release = 1;
	dpt_sdi_ver.sv_machine = SDI_386_AT;
	dpt_sdi_ver.sv_modes   = SDI_BASIC1;

	/* need to allocate space for sc_ha, must be contiguous */
	ha_structsize = dpt_sc_hacnt*(sizeof (struct dpt_scsi_ha));

	for (i = 2; i < ha_structsize ; i *= 2) ;
	ha_structsize = i;

	dpt_sc_ha = (struct dpt_scsi_ha *)kmem_zalloc(ha_structsize, sleepflag);
	if (dpt_sc_ha == NULL) {
		cmn_err(CE_WARN,"DPT Host Adapter: Initialization error - cannot allocate host adapter structures");
		return(-1);
	}
		
	/* allocate space for ccb's & LU queues, must be contiguous */
	ccb_structsize = NCPS*(sizeof (struct dpt_ccb));
	for (i = 2; i < ccb_structsize ; i *= 2) ;
	ccb_structsize = i;

	luq_structsize = MAX_EQ*(sizeof (struct dpt_scsi_lu));
	for (i = 2; i < luq_structsize ; i *= 2) ;
	luq_structsize = i;

	dma_listsize = NDMA * sizeof(dpt_dma_t);
	for (i = 2; i < dma_listsize; i *= 2);
	dma_listsize = i;

#ifdef	DPT_DEBUG
	if(ccb_structsize > PAGESIZE)
		cmn_err(CE_WARN, "%s: CCBs exceed pagesize (may cross physical page boundary)\n", dptidata[0].name);
	if(dma_listsize > PAGESIZE)
		cmn_err(CE_WARN, "%s: dmalist exceeds pagesize (may cross physical page boundary)\n", dptidata[0].name);
#endif /* DPT_DEBUG */

	if((dpt_dmalistpool = (dpt_dma_t *)kmem_zalloc(dma_listsize, sleepflag)) == NULL) {
		cmn_err(CE_WARN, "%s: Initalization error allocating dpt_dmalistpool\n", dptidata[0].name);
		kmem_free(dpt_sc_ha, ha_structsize );
		return(-1);
	}

	/* Build list of free DMA lists */
	dpt_dfreelist = NULL;
	for (i = 0; i < NDMA; i++) {
		dp = &dpt_dmalistpool[i];
		dp->d_next = dpt_dfreelist;
		dpt_dfreelist = dp;
	}

	for (c = 0; c < dpt_cntls; c++) {
		ha = &dpt_sc_ha[c];
		ha->ha_state  = 0;
		ha->ha_id     = dpt_ctlr_id;
		ha->ha_npend  = 0;
		ha->ha_cblist = NULL;
		ha->ha_base = dptidata[c].ioaddr1;
		ha->ha_vect = dptidata[c].iov;

		if (dpt_ha_init(c) != 0)        /* initialize HA */
			continue;

		/* controller was found now alloc ccb's, LU Q's */
		ha->ha_ccb = (struct dpt_ccb *)kmem_zalloc(ccb_structsize, sleepflag);
		if (ha->ha_ccb == NULL) {
			cmn_err(CE_WARN,"DPT Host Adapter: Initialization error - cannot allocate command blocks");
			continue;
		}

		ha->ha_dev = (struct dpt_scsi_lu *)kmem_zalloc(luq_structsize, sleepflag);
		if (ha->ha_dev == NULL) {
			cmn_err(CE_WARN,"DPT Host Adapter: Initialization error - cannot allocate logical unit queues");
			continue;
		}
		
		for (j = 0; j < MAX_EQ; j++) {
			q = &ha->ha_dev[j];
			q->q_first = NULL;
			q->q_last  = NULL;
			q->q_count = 0;
			q->q_flag  = 0;
			q->q_func  = NULL;
		}
		dpt_init_cps(c);

		if ((cntl_num = sdi_gethbano(dptidata[c].cntlr)) <= -1) {
			cmn_err(CE_WARN,"%s: No HBA number available.",dptidata[c].name);
			continue;
		}
		dptidata[c].cntlr = cntl_num;
		dpt_gtol[cntl_num] = c;
		dpt_ltog[dpt_gtol[cntl_num]] = cntl_num;

		if ((cntl_num=sdi_register(&dpthba_info,&dptidata[c])) < 0) {
			cmn_err(CE_WARN,"%s: HA %d SDI register slot %d failed",
				dptidata[c].name, c, cntl_num);
				continue;
		}
		dptidata[c].active = 1;
		dpt_hacnt++;

		dpt_timer(c);   /* start timer */
	}
	/* Now cleanup any unnecessary structs */
	for (i = 0; i < dpt_cntls; i++) {
		if( !dptidata[i].active) {
			ha = &dpt_sc_ha[i];
			if( ha->ha_ccb != NULL) {
                                kmem_free((void *)ha->ha_ccb, ccb_structsize);
                                ha->ha_ccb = NULL;
			}
			if( ha->ha_dev != NULL) {
                                kmem_free((void *)ha->ha_dev, luq_structsize);
                                ha->ha_dev = NULL;
			}
		}
	}

	if (dpt_hacnt == 0) {
		cmn_err(CE_NOTE,"!%s: No HAs found.", dptidata[0].name);
		kmem_free((void *)dpt_sc_ha, ha_structsize);
		dpt_sc_ha = NULL;
		return(-1);
	}

	dptfirst_time = 0;
	return(0);
}


/*
** Function name: dptsend()
** Description:
** 	Send a SCSI command to a controller.  Commands sent via this
**	function are executed in the order they are received.
*/

long
dptsend(hbap)
register struct hbadata *hbap;
{
	register struct scsi_ad *sa;
	register struct dpt_scsi_lu *q;
	register dpt_sblk_t *sp = (dpt_sblk_t *) hbap;
	register int	c, t, l;
	int   oip;

	sa = &sp->sbp->sb.SCB.sc_dev;
	c = dpt_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);
	l = SDI_LUN(sa);

	if (sp->sbp->sb.sb_type != SCB_TYPE)
	{
		return (SDI_RET_ERR);
	}

	/* change #1 */
	if (dpt_illegal(SDI_HAN(sa), t, l, sa->sa_major) )
	{
		sp->sbp->sb.SCB.sc_comp_code = SDI_SCBERR;
		sdi_callback(sp->sbp);
		return (SDI_RET_OK);
	}

	q = &LU_Q(c, t, sa->sa_lun);

	oip = spl5();
	if (q->q_flag & QPTHRU) {
		splx(oip);
		return (SDI_RET_RETRY);
	}

	sp->sbp->sb.SCB.sc_comp_code = SDI_PROGRES;
	sp->sbp->sb.SCB.sc_status = 0;

	dpt_putq(q, sp);

	if ( !(q->q_flag & QSUSP ) ) {
		dpt_next(q);
	}

	splx(oip);
	return (SDI_RET_OK);
}


/*
** Function name: dpticmd()
** Description:
**	Send an immediate command.  If the logical unit is busy, the job
**	will be queued until the unit is free.  SFB operations will take
**	priority over SCB operations.
*/

long
dpticmd(hbap)
register struct  hbadata *hbap;
{
	register struct scsi_ad *sa;
	register struct dpt_scsi_lu *q;
	register dpt_sblk_t *sp = (dpt_sblk_t *) hbap;
	register int	c, t, l;
	struct ident	 *inq_data;
	struct scs	     *inq_cdb;
	int   oip;

	oip = spl5();

	switch (sp->sbp->sb.sb_type)
	{
	case SFB_TYPE:
		sa = &sp->sbp->sb.SFB.sf_dev;
		c = dpt_gtol[SDI_HAN(sa)];
		t = SDI_TCN(sa);
		l = SDI_LUN(sa);
		q = &LU_Q(c, t, sa->sa_lun);

		/* change #1 */
		if (dpt_illegal(SDI_HAN(sa), t, l, sa->sa_major))
		{
			sp->sbp->sb.SFB.sf_comp_code = SDI_SFBERR;
			sdi_callback(sp->sbp);
			splx(oip);
			return (SDI_RET_OK);
		}

		sp->sbp->sb.SFB.sf_comp_code = SDI_ASW;

		switch (sp->sbp->sb.SFB.sf_func)
		{
		case SFB_RESUME:
			q->q_flag &= ~QSUSP;
			dpt_next(q);
			break;
		case SFB_SUSPEND:
			q->q_flag |= QSUSP;
			break;
		case SFB_ABORTM:
		case SFB_RESETM:
			sp->sbp->sb.SFB.sf_comp_code = SDI_PROGRES;
			dpt_putq(q, sp);
			dpt_next(q);
			splx(oip);
			return (SDI_RET_OK);
		case SFB_FLUSHR:
			dpt_flushq(q, SDI_QFLUSH, 0);
			break;
		case SFB_NOPF:
			break;
		default:
			sp->sbp->sb.SFB.sf_comp_code = SDI_SFBERR;
		}

		sdi_callback(sp->sbp);
		splx(oip);
		return (SDI_RET_OK);

	case ISCB_TYPE:
		sa = &sp->sbp->sb.SCB.sc_dev;
		c = dpt_gtol[SDI_HAN(sa)];
		t = SDI_TCN(sa);
		l = SDI_LUN(sa);
		q = &LU_Q(c, t, sa->sa_lun);

		inq_cdb = (struct scs *)sp->sbp->sb.SCB.sc_cmdpt;
		if ((t == dpt_sc_ha[c].ha_id) && (l == 0) && (inq_cdb->ss_op == SS_INQUIR)) {
			inq_data = (struct ident *)sp->sbp->sb.SCB.sc_datapt;
			inq_data->id_type = ID_PROCESOR;
			(void)strncpy(inq_data->id_vendor, dptidata[c].name, 24);
			inq_data->id_vendor[23] = NULL;
			sp->sbp->sb.SCB.sc_comp_code = SDI_ASW;
			splx (oip);
			return (SDI_RET_OK);
		}

		/* change #1 */
		if (dpt_illegal(SDI_HAN(sa), t, l, sa->sa_major) && ! dptinit_time)
		{
			sp->sbp->sb.SCB.sc_comp_code = SDI_SCBERR;
			sdi_callback(sp->sbp);
			splx(oip);
			return (SDI_RET_OK);
		}

		sp->sbp->sb.SCB.sc_comp_code = SDI_PROGRES;
		sp->sbp->sb.SCB.sc_status = 0;

		dpt_putq(q, sp);
		dpt_next(q);
		splx(oip);
		return (SDI_RET_OK);

	default:
		splx(oip);
		return (SDI_RET_ERR);
	}
}


/*
** Function name: dptxlat()
** Description:
**	Perform the virtual to physical translation on the SCB
**	data pointer.
*/

void
dptxlat(hbap, flag, procp)
register struct  hbadata *hbap;
int flag;
struct proc *procp;
{
	extern void	dpt_dmalist();
	extern void	dma_freelist();
	register dpt_sblk_t *sp = (dpt_sblk_t *) hbap;

	if (sp->s_dmap)
	{
		dma_freelist(sp->s_dmap);
		sp->s_dmap = NULL;
	}

	if (sp->sbp->sb.SCB.sc_link)
	{
		cmn_err(CE_WARN, "!DPT Host Adapter: Linked commands NOT available\n");
		sp->sbp->sb.SCB.sc_link = NULL;
	}

	if (sp->sbp->sb.SCB.sc_datapt)
		dpt_dmalist(sp, procp);
	else {
		sp->s_addr = 0;
		sp->sbp->sb.SCB.sc_datasz = 0;
	}
}


/*
** Function name: dptgetblk()
** Description:
**	Allocate a SB structure for the caller.  The function will
**	sleep if there are no SCSI blocks available.
*/

struct hbadata *
dptgetblk()
{
	dpt_sblk_t  *sp;

	sp = (dpt_sblk_t *)sdi_get(&sm_poolhead, KM_NOSLEEP);
	return ((struct hbadata *)sp);
}


/*
** Function name: dptfreeblk()
** Description:
**	Release previously allocated SB structure. If a scatter/gather
**	list is associated with the SB, it is freed via dma_freelist().
**	A nonzero return indicates an error in pointer or type field.
*/

long
dptfreeblk(hbap)
register struct hbadata *hbap;
{
	register int	i, oip;
	extern void	dma_freelist();
	register dpt_sblk_t *sp = (dpt_sblk_t *) hbap;


	if (sp->s_dmap) {
		dma_freelist(sp->s_dmap);
		sp->s_dmap = NULL;
	}
	sdi_free(&sm_poolhead, (jpool_t *)sp);
	return ((long) SDI_RET_OK);
}

/*
** Function name: dptgetinfo()
** Description:
**	Return the name and iotype of the given device.  The name is copied
**	into a string pointed to by the first field of the getinfo structure.
*/

void
dptgetinfo(sa, getinfo)
struct scsi_ad *sa;
struct hbagetinfo *getinfo;
{
	register char  *s1, *s2;
	static char temp[] = "HA X TC X";
	struct dpt_scsi_ha *ha;
	struct RdConfig *rcfg = &eata_cfg;

	s1 = temp;
	s2 = getinfo->name;
	temp[3] = SDI_HAN(sa) + '0';
	temp[8] = SDI_TCN(sa) + '0';

	while ((*s2++ = *s1++) != '\0')
		;

	ha = &dpt_sc_ha[ dpt_gtol[SDI_HAN(sa)] ];
	if (EATA_ReadConfig(ha->ha_base, rcfg)) {
		if (rcfg->DMAChannelValid) {
			/***
			** A 2011, Restrict DMA to 16Mb
			***/
			getinfo->iotype = F_DMA | F_SCGTH;
		}
		else {
			/***
			** A 2012, UnRestricted DMA
			***/
			getinfo->iotype = F_PIO;
		}
	}
}


/*===========================================================================*/
/* SCSI Host Adapter Driver Utilities
/*===========================================================================*/


/*
** Function name: dpt_pass_thru()
** Description:
**	Send a pass-thru job to the HA board.
*/

void
dpt_pass_thru(bp)
struct buf  *bp;
{
	int	c = dpt_gtol[SC_HAN(bp->b_dev)];
	int	t = SC_TCN(bp->b_dev);
	int	l = SC_LUN(bp->b_dev);
	register struct dpt_scsi_lu	*q;
	register struct sb *sp;
	struct proc *procp;
	int  oip;

	sp = (struct sb *) bp->b_resid;
#ifdef DPT_DEBUG
	if (sp->SCB.sc_wd != (long) bp)
		cmn_err(CE_PANIC, "DPT Host Adapter: Corrupted address from physio");
#endif

	sp->SCB.sc_dev.sa_lun = l;
	sp->SCB.sc_dev.sa_fill = (dpt_ltog[c] << 3) | t;
	sp->SCB.sc_datapt = (caddr_t) paddr(bp);
	sp->SCB.sc_int = dpt_int;

	drv_getparm(UPROCP, (ulong*)&procp);
	sdi_translate(sp, bp->b_flags, procp);

	if (bp->b_flags & B_READ)
		sp->SCB.sc_mode = SCB_READ;
	else
		sp->SCB.sc_mode = SCB_WRITE;

	q = &LU_Q(c, t, l);

	oip = spl5();

	dpt_putq(q, (dpt_sblk_t *)((struct xsb *)sp)->hbadata_p);
	dpt_next(q);

	splx(oip);
}


/*
** Function name: dpt_int()
** Description:
**	This is the interrupt handler for pass-thru jobs.  It just
**	wakes up the sleeping process.
*/

void
dpt_int(sp)
struct sb *sp;
{
	struct buf  *bp;

	bp = (struct buf *) sp->SCB.sc_wd;
	biodone(bp);
}



/*
** Function name: dpt_flushq()
** Description:
**	Empty a logical unit queue.  If flag is set, remove all jobs.
**	Otherwise, remove only non-control jobs.
*/

void
dpt_flushq(q, cc, flag)
register struct dpt_scsi_lu *q;
int  cc, flag;
{
	register dpt_sblk_t  *sp, *nsp;

	ASSERT(q);

	sp = q->q_first;
	q->q_first = q->q_last = NULL;
	q->q_count = 0;

	while (sp) {
		nsp = sp->s_next;
		if (!flag && (queclass(sp) > QNORM))
			dpt_putq(q, sp);
		else {
			sp->sbp->sb.SCB.sc_comp_code = (ulong)cc;
			sdi_callback(sp->sbp);
		}
		sp = nsp;
	}
}


/*
** Function name: dpt_putq()
** Description:
**	Put a job on a logical unit queue.  Jobs are enqueued
**	on a priority basis.
*/

void
dpt_putq(q, sp)
register struct dpt_scsi_lu	*q;
register dpt_sblk_t *sp;
{
	register cls = queclass(sp);

	ASSERT(q);


	/*
	 * If queue is empty or queue class of job is less than
	 * that of the last one on the queue, tack on to the end.
	 */
	if ( !q->q_first || (cls <= queclass(q->q_last)) ){
		if (q->q_first) {
			q->q_last->s_next = sp;
			sp->s_prev = q->q_last;
		} else {
			q->q_first = sp;
			sp->s_prev = NULL;
		}
		sp->s_next = NULL;
		q->q_last = sp;

	} else {
		register dpt_sblk_t *nsp = q->q_first;

		while (queclass(nsp) >= cls)
			nsp = nsp->s_next;
		sp->s_next = nsp;
		sp->s_prev = nsp->s_prev;
		if (nsp->s_prev)
			nsp->s_prev->s_next = sp;
		else
			q->q_first = sp;
		nsp->s_prev = sp;
	}

	q->q_count++;
}


/*
** Function name: dpt_next()
** Description:
**	Attempt to send the next job on the logical unit queue.
**	All jobs are not sent if the Q is busy.
*/

void
dpt_next(q)
register struct dpt_scsi_lu *q;
{
	register dpt_sblk_t  *sp;

	ASSERT(q);

	if ((sp = q->q_first) == NULL)		/*  queue empty  */
	{
		return;				/*       or	  */
	}

	if ((q->q_active >= dpt_lu_max) ||	/*  individual LU busy  */
	    (dpt_active_jobs >= dpt_hba_max)) {	/* hba busy */
		return;
	}

	if (q->q_flag & QSUSP  &&		/*  Q suspended  */
	sp->sbp->sb.sb_type == SCB_TYPE)
	{
		return;
	}

	if ( !(q->q_first = sp->s_next))
		q->q_last = NULL;

	q->q_count--;
	q->q_active++;
	dpt_active_jobs++;

	if (sp->sbp->sb.sb_type == SFB_TYPE)
		dpt_func(sp);
	else
		dpt_cmd(sp);

	if(dptinit_time)		/* need to poll */
	{
		/***
		** Wait up to 5 seconds for the command to complete.
		** I know this is a __LONG__ time, but we've seen
		** 2011 with Rev 3C firmware take over 1 second to
		** show completion of SS_INQUIR on some devices.
		***/
		if(dpt_wait(dpt_gtol[SC_HAN(ad2dev_t(sp->sbp->sb.SCB.sc_dev))], 5000) == FAILURE) {
			sp->sbp->sb.SCB.sc_comp_code = SDI_TIME;
			sdi_callback(sp->sbp);
		}
	}
}


/*
** Function name: dpt_cmd()
** Description:
**	Create and send an SCB associated command.
*/

void
dpt_cmd(sp)
register dpt_sblk_t *sp;
{
	register struct scsi_ad *sa;
	register struct dpt_ccb *cp;
	register struct dpt_scsi_lu	*q;
	register int  i;
	register char *p;
	unsigned long cnt;
	int  c, t;

	sa = &sp->sbp->sb.SCB.sc_dev;
	c = dpt_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);

	cp = dpt_getblk(c);
	cp->c_bind = &sp->sbp->sb;
	cp->c_time = (sp->sbp->sb.SCB.sc_time*HZ) / 1000;

	/* Build the EATA Command Packet structure */
	cp->CP_OpCode        = CP_DMA_CMD;
	cp->CPop.byte        = HA_AUTO_REQ_SEN;
	cp->CPID             = (BYTE)(t);
	cp->CPmsg0           = (HA_IDENTIFY_MSG | HA_DISCO_RECO) + sa->sa_lun;

	if (sp->s_CPopCtrl) {
		cp->CPop.byte |= sp->s_CPopCtrl;
		sp->s_CPopCtrl = 0;
	}
	if (sp->s_dmap)  {              /* scatter/gather is used      */
#ifdef DPT_DEBUG
	printf("dpt_cmd: scatter/gather is being used.\n");
#endif
		cp->CPop.bit.Scatter = 1;
		cnt = sp->s_dmap->SG_size;
	} else {                             /* block mode                  */
#ifdef DPT_DEBUG
	printf("dpt_cmd: block mode is being used.\n");
#endif
		cnt = sp->sbp->sb.SCB.sc_datasz;
	}
	if (cnt) {
		if (sp->sbp->sb.SCB.sc_mode & SCB_READ) {
#ifdef DPT_DEBUG
	printf("dpt_cmd: doing a READ.\n");
#endif
			cp->CPop.bit.DataIn  = 1;
		} else {
#ifdef DPT_DEBUG
	printf("dpt_cmd: doing a WRITE.\n");
#endif
			cp->CPop.bit.DataOut = 1;
		}
		*(unsigned long *)cp->CPdataLen = sdi_swap32(cnt);
	} else
		*(unsigned long *)cp->CPdataLen = 0;

	cp->CPdataDMA = sdi_swap32(sp->s_addr);

	q = &LU_Q(c, t, sa->sa_lun);         /* Get SCSI Dev/Lun Pointer.   */

	/*********************************************************************
	** If a Request Sense command and ReqSen Data cached then copy to   **
	**   data buffer and return.                                        **
	**********************************************************************/
	p = sp->sbp->sb.SCB.sc_cmdpt;        /* Get Command CDB Pointer.    */

	if ( (q->q_flag & QSENSE) && (*p == SS_REQSEN)) {
		q->q_flag &= ~QSENSE;
		/*
		 * If the REQ SENSE data is going to a buffer within kernel
		 * space, then the data is just copied.  Otherwise, the
		 * data is going to a user-space buffer (Pass-thru) so the
		 * data must be copied to the individual segments of
		 * contiguous physical memory that make up the user's buffer.
		 * Note - The buffer address sent by the user
		 * (sp->sbp->sb.SCB.sc_datapt) can not be used directly
		 * since this routine may be executing on the interrupt thread.
		 */
		if (! cp->CPop.bit.Scatter) {
#ifdef DPT_DEBUG
	printf("dpt_cmd: one big chunk to kernel memory.\n");
#endif
			bcopy((caddr_t)(&q->q_sense)+1,
			    (caddr_t)xphystokv(sp->s_addr),
			    cnt);
		} else {
		/* copy req sns data to places defined by scat/gath list */
			SG_vect	*VectPtr;
			int	VectCnt;
			caddr_t	Src;
			caddr_t	Dest;
			int	Count;

#ifdef DPT_DEBUG
	printf("dpt_cmd: scatter/gather results being returned.\n");
#endif
			VectPtr = (SG_vect *) xphystokv(sp->s_addr);
			VectCnt = cnt / sizeof(*VectPtr);
			Src = (caddr_t)(&q->q_sense) + 1;
			for (i=0; i < VectCnt; i++) {
				Dest = (caddr_t) sdi_swap32(VectPtr->Phy);
				Dest = (caddr_t)xphystokv((paddr_t)Dest);

				Count = (int) sdi_swap32(VectPtr->Len);

				bcopy (Src, Dest, Count);
				Src += Count;
				VectPtr++;
			}
		}
		i = HA_ST_SEEK_COMP | HA_ST_READY;
		cp->CP_Controller_Status = S_GOOD;
		dpt_done(c, cp, i);
		return;
	}
#ifdef DPT_DEBUG
	printf("dpt_cmd: not request sense, send command to HBA.\n");
#endif

	for (i=0; i < sp->sbp->sb.SCB.sc_cmdsz; i++)
		cp->CPcdb[i] = *p++;        /* Copy SCB cdb to CP cdb.     */
	dpt_send(c, START, cp);             /* Send command to the HA.     */
}


/*
** Function name: dpt_func()
** Description:
**	Create and send an SFB associated command.
*/

void
dpt_func(sp)
register dpt_sblk_t *sp;
{
	register struct scsi_ad *sa;
	register struct dpt_ccb *cp;
	int  c, t;
	struct sdi_edt *edtp;

	/* Only SFB_ABORTM and SFB_RESETM messages get here.                */

#ifdef DPT_DEBUG
	if(sp->sbp->sb.SFB.sf_func!=SFB_ABORTM && sp->sbp->sb.SFB.sf_func!=SFB_RESETM)
	{
		cmn_err(CE_WARN, "DPT Host Adapter: Unsupported SFB command: %X\n",sp->sbp->sb.SFB.sf_func);
		return;
	}
#endif

	sa = &sp->sbp->sb.SFB.sf_dev;
	c = dpt_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);

	cp = dpt_getblk(c);
	cp->c_bind = &sp->sbp->sb;
	cp->CPID   = (BYTE)t;
	cp->CPmsg0 = sa->sa_lun;
	cp->c_time = 0;

	if( (edtp = sdi_redt(c, cp->CPID, cp->CPmsg0)) != (struct sdi_edt *)0 &&
	    edtp->pdtype != ID_TAPE ) {

		cmn_err(CE_WARN, "!DPT Host Adapter: HA %d - Bus is being reset\n", dpt_ltog[c]);
		outb((dpt_sc_ha[c].ha_base + HA_COMMAND), CP_EATA_RESET);
		drv_usecwait(1000);
	}

	cp->CP_Controller_Status = 0;
	cp->CP_SCSI_Status       = 0;
	dpt_ha_done(c, cp, 0);
}



/*
** Function name: dpt_send()
** Description:
**      Send a command to the host adapter board.
*/

void
dpt_send(c, cmd, cp)
int		c;	/* HA controller number  */
int		cmd;	/* HA control command    */
struct dpt_ccb	*cp;	/* command block pointer */
{
	register char *CPaddr;
	register struct dpt_scsi_ha *ha = &dpt_sc_ha[c];

	if (cmd == START) {
		cp->c_active = TRUE;	/* Set Command Active flag.         */
		drv_getparm(LBOLT, (ulong *)&cp->c_start); /* Set start time */
		ha->ha_npend++;		/* Increment number pending on ctlr */
	} else if (cmd == ABORT) {
		outb((ha->ha_base + HA_COMMAND), CP_EATA_RESET);
		drv_usecwait(1000);
		return;
	}

	/* UPDATE: this call resolved in dpt_a.s -- remove from here*/
	scsi_send_cmd(ha->ha_base, cp->c_addr, CP_DMA_CMD);

	/********** This does the same thing as the above call **************
		CPaddr = (char *)&cp->c_addr;
		while (inb(ha->ha_base + HA_AUX_STATUS) & HA_AUX_BUSY )  ;
	
		outb((ha->ha_base + HA_DMA_BASE + 3), *CPaddr++);
		outb((ha->ha_base + HA_DMA_BASE + 2), *CPaddr++);
		outb((ha->ha_base + HA_DMA_BASE + 1), *CPaddr++);
		outb((ha->ha_base + HA_DMA_BASE + 0), *CPaddr);
		outb((ha->ha_base + HA_COMMAND), CP_DMA_CMD);
	*********************************************************************/
}


/*
** Function name: dpt_getblk()
** Description:
**      Allocate a controller command block structure.
*/

struct dpt_ccb *
dpt_getblk(c)
int	c;
{
	register struct dpt_scsi_ha	*ha = &dpt_sc_ha[c];
	register struct dpt_ccb	*cp;
	int oip;

	if (ha->ha_cblist) {
		cp = ha->ha_cblist;
		ha->ha_cblist = cp->c_next;
		return (cp);
	}
	cmn_err(CE_PANIC, "DPT Host Adapter: Out of command blocks");
}


/*
** Function name: dpt_freeblk()
** Description:
**      Release a previously allocated command block.
*/

void
dpt_freeblk(c, cp)
int  c;
struct dpt_ccb  *cp;
{
	register struct dpt_scsi_ha	*ha = &dpt_sc_ha[c];
	int oip;

	oip = spl6();
	cp = &ha->ha_ccb[cp->c_index];
	cp->c_bind = NULL;
	cp->c_next = ha->ha_cblist;
	ha->ha_cblist = cp;
	splx(oip);
}


/*
** Function name: dpt_dmalist()
** Description:
**	Build the physical address(es) for DMA to/from the data buffer.
**	If the data buffer is contiguous in physical memory, only 1 base
**	address is provided for a regular SB.  If not, a scatter/gather
**	list is built, and the SB will point to that list instead.
*/
void
dpt_dmalist(sp, procp)
register dpt_sblk_t *sp;
struct proc *procp;
{
	SG_vect	tmp_list[MAX_DMASZ];
	register SG_vect  *pp;
	register dpt_dma_t  *dmap;
	register long   count, fraglen, thispage;
	caddr_t		vaddr;
	paddr_t		addr, base;
	int		i, oip;
#ifdef DPT_DEBUG
	printf("dpt_dmalist: on entry\n");
#endif
	vaddr = sp->sbp->sb.SCB.sc_datapt;
	count = sp->sbp->sb.SCB.sc_datasz;
	pp = &tmp_list[0];

	/* First build a scatter/gather list of physical addresses and sizes */

	for (i = 0; (i < MAX_DMASZ) && count; ++i, ++pp) {
		base = vtop(vaddr, procp);	/* Physical addr of segment */
		fraglen = 0;			/* Zero bytes so far */
		do {
			thispage = min(count, pgbnd(vaddr));
			fraglen += thispage;	/* This many more are contig */
			vaddr += thispage;	/* Bump virtual address */
			count -= thispage;	/* Recompute amount left */
			if (!count)
				break;		/* End of request */
			addr = vtop(vaddr, procp); /* Get next page's address */
		} while (base + fraglen == addr);

		/* Now set up dma list element */
		pp->Phy.l = sdi_swap32(base);
		pp->Len.l = sdi_swap32(fraglen);
	}
	if (count != 0)
		cmn_err(CE_PANIC, "DPT Host Adapter: Job too big for DMA list");

	if (i == 1)
		/*
		 * The data buffer was contiguous in physical memory.
		 * There is no need for a scatter/gather list.
		 */
		sp->s_addr = (paddr_t) (sdi_swap32(tmp_list[0].Phy.l));
	else {
		/*
		 * We need a scatter/gather list.
		 * Allocate one and copy the list we built into it.
		 */
#ifdef DPT_DEBUG
	printf("dpt_dmalist: building a scatter/gather list.\n");
#endif
		oip = spl6();
		while ( !(dmap = dpt_dfreelist))
			sleep((caddr_t)&dpt_dfreelist, PRIBIO);
		dpt_dfreelist = dmap->d_next;
		splx(oip);

		sp->s_dmap = dmap;
		sp->s_addr = vtop((caddr_t) dmap->d_list, procp);
		dmap->SG_size = i * sizeof(SG_vect);
		bcopy((caddr_t) &tmp_list[0],
		    (caddr_t) dmap->d_list, dmap->SG_size);
	}
#ifdef DPT_DEBUG
	printf("dpt_dmalist: on exit\n");
#endif
	return;
}


/*
** Function name: dma_freelist()
** Description:
**	Release a previously allocated scatter/gather DMA list.
*/
static void
dma_freelist(dmap)
dpt_dma_t *dmap;
{
	register int  oip;

	ASSERT(dmap);

	oip = spl6();
	dmap->d_next = dpt_dfreelist;
	if (dpt_dfreelist == NULL)
		wakeup((caddr_t)&dpt_dfreelist);
	dpt_dfreelist = dmap;
	splx(oip);
}

/*
** Function Name: dpt_wait()
** Description:
**	Poll for a completion from the host adapter.  If an interrupt
**	is seen, the HA's interrupt service routine is manually called.
**  NOTE:
**	This routine allows for no concurrency and as such, should
**	be used selectively.
*/
dpt_wait(c, time)
int c, time;
{
	register struct dpt_scsi_ha  *ha = &dpt_sc_ha[c];
	int act;

	while (time > 0) {

		if ( inb(ha->ha_base + HA_AUX_STATUS) & HA_AUX_INTR ) {
			if( ( dpt_flush_in_progress == TRUE ) &&
			    ( dpt_flush_intr == TRUE ) ) {
				/***
				** Controller has generated an interrupt to
				** acknowledge completion of command, and
				** dptintr() has servicved the interrupt.
				***/
				return( SUCCESS );
			}
			act = dptidata[c].active;
			dptidata[c].active = 1;
			dptintr(ha->ha_vect);
			dptidata[c].active = act;
			return (SUCCESS);
		}
		else {
			if( ( dpt_flush_in_progress == TRUE ) &&
			    ( dpt_flush_intr == TRUE ) ) {
				/***
				** Controller has generated an interrupt to
				** acknowledge completion of command, and
				** dptintr() has servicved the interrupt.
				***/
				return( SUCCESS );
			}
		}

		drv_usecwait(1000); /** Wait 1 msec **/
		time--;
	}

	/***
	** Command we are waiting for has not completed.
	***/
	cmn_err(CE_WARN, "!%s: Command completion not indicated, dpt_wait()\n",dptidata[c].name);
	return (FAILURE);
}


/*
** Function name: dpt_init_cps()
** Description:
**      Initialize the controller CP free list.
*/
void
dpt_init_cps(c)
int	c;
{
	register struct dpt_scsi_ha  *ha = &dpt_sc_ha[c];
	register struct dpt_ccb  *cp;
	register int    i;

	ha->ha_cblist = NULL;

	for (i = 0; i < NCPS; i++)
	{
		cp = &ha->ha_ccb[i];
		cp->c_active  = 0;
		cp->c_index   = i;
		cp->c_bind    = NULL;

		/** Save data addresses into CP in  68000 format ***/
		cp->c_addr    = kvtophys((caddr_t)cp);
		cp->c_addr    = sdi_swap32(cp->c_addr);
		cp->CPstatDMA = kvtophys((caddr_t)&dpt_sc_hastat[c]);
		cp->CPstatDMA = sdi_swap32(cp->CPstatDMA);
		cp->CP_ReqDMA = kvtophys((caddr_t)cp->sense);
		cp->CP_ReqDMA = sdi_swap32(cp->CP_ReqDMA);

		/** Save Command Packet virtual address pointer ***/
		cp->CPaddr.vp = cp;
		cp->ReqLen    = sizeof(struct sense);
		cp->c_next    = ha->ha_cblist;
		ha->ha_cblist = cp;
	}
}


/*
** Function name: dpt_ha_init()
** Description:
**      Reset the HA board and initialize the communications.
*/

int
dpt_ha_init(c)  /* HA controller number */
int  c;
{
	register struct dpt_scsi_ha  *ha = &dpt_sc_ha[c];
	register struct RdConfig *rcfg = &eata_cfg;
	int i, slot_id_addr;
	char idbyte1;
	char idbyte2;


	if (inb(ha->ha_base + HA_STATUS) != (HA_ST_SEEK_COMP | HA_ST_READY)) {
		outb((ha->ha_base + HA_COMMAND), CP_EATA_RESET);
		drv_usecwait(4000000);  /* 4 full second wait */
	}

	/* Issue an EATA Read Config command to the controller, if necessary*/
	/*  make any systems configs ... like allocate a DMA Channel.	    */

	if (EATA_ReadConfig(ha->ha_base, rcfg)) {

#ifdef DPT_DEBUG
	printf("dpt_ha_init: EATA_ReadConfig succeeded.\n");
#endif

		if (rcfg->DMAChannelValid) {
			/* the card is a 2011 */
#ifdef DPT_DEBUG
	cmn_err(CE_NOTE, "DPT 2011 Host Adapter identified.\n");
#endif
			dptH_DMA_Setup(rcfg->DMA_Channel);
		}
		else if (rcfg->DMAsupported && c == 0) {
			/* the card is a 2012 and it is the first one */
			/* query each slot for a dpt card. */
#ifdef DPT_DEBUG
	printf("dpt_ha_init: We got a 2012.\n");
#endif
			for (i = 1; i <= 15; i++) {
				slot_id_addr = SLOT_ID_ADDR(i); /* iC80 */
				/* 1214= 0 00100 10000 10100
				 *           D     P     T
				 */
				idbyte1 = (char)inb(slot_id_addr);
				idbyte2 = (char)inb(slot_id_addr + 1);

				if ((idbyte1 == (char)0x12) &&
				    (idbyte2 == (char)0x14)) {
#ifdef DPT_DEBUG
	printf("dpt_ha_init: DPT EISA ID, We are in slot %d\n",i);
#endif
					break;
				}
				else if ((idbyte1 == (char)0x06) &&
					 (idbyte2 == (char)0x94)) {
#ifdef DPT_DEBUG
	printf("dpt_ha_init: AT&T EISA ID, We are in slot %d\n",i);
#endif
					break;
				}
				else if ((idbyte1 == (char)0x38) &&
					 (idbyte2 == (char)0xA3)) {
#ifdef DPT_DEBUG
	printf("dpt_ha_init: NEC EISA ID, We are in slot %d\n",i);
#endif
					break;
				}
			}
			ha->ha_base = SLOT_BASE_IO_ADDR(i); /* iC88 */
		}

		if( i > 15 ) {
			return( 1 );
		}

		if (inb(ha->ha_base + HA_STATUS) == (HA_ST_SEEK_COMP | HA_ST_READY) &&
		    ha->ha_vect == rcfg->IRQ_Number) {
#ifdef DPT_DEBUG
	printf("dpt_ha_init: Marking Host Adapter as operational\n");
#endif
	cmn_err(CE_NOTE, "DPT Host Adapter found at address 0x%X\n", ha->ha_base);
			ha->ha_state |= C_SANITY; /* Mark HA operational */
			return( 0 );
		}
		else {
#ifdef DPT_DEBUG
	printf("DPT Host Adapter NOT found at address 0x%X\n", ha->ha_base);
	cmn_err(CE_CONT, "!DPT Host Adapter NOT found at address 0x%X\n", ha->ha_base);
#endif
			return( 1 );
		}
	}
	else {
#ifdef DPT_DEBUG
	printf("dpt_ha_init: EATA_ReadConfig failed.\n");
#endif
		return( 1 );
	}
}


/*******************************************************************************
** EATA_ReadConfig - Issue an EATA Read Config Command, Process PIO.	      **
*******************************************************************************/
EATA_ReadConfig(port, rcfg)
int port;
register struct RdConfig *rcfg;
{
	register int status;
	ulong	 loop = 50000L;

	/* Wait for controller not busy */
	status = inb(port + HA_STATUS) & HA_ST_BUSY;
	while ( status == HA_ST_BUSY && loop--) {
		drv_usecwait(1);
		status = inb(port + HA_STATUS) & HA_ST_BUSY;
	}
#ifdef DPT_DEBUG
	printf("EATA_ReadConfig: controller ready or timedout, status = 0x%X.\n",status);
#endif

	if ( status == HA_ST_BUSY ) {
#ifdef DPT_DEBUG
	printf("EATA_ReadConfig: controller status = HA_ST_BUSY, returning ...\n");
#endif
		return(0);
	}

	/* Send the Read Config EATA PIO Command */
/*
	outb(port + HA_STATUS, CP_READ_CFG_PIO);
*/
	outb(port + HA_COMMAND, CP_READ_CFG_PIO);

	/* Wait for DRQ Interrupt       */
	loop   = 50000L;
/*
	while ( !(inb(port + HA_STATUS) & HA_ST_DRQ) && loop--)
*/
	while (((status = inb(port + HA_STATUS)) != ( HA_ST_DRQ | HA_ST_SEEK_COMP | HA_ST_READY)) && loop --)
		drv_usecwait(1);

	if ( !(inb(port + HA_AUX_STATUS) & HA_ST_DRQ) ) {
#ifdef DPT_DEBUG
	printf("EATA_ReadConfig: controller aux status not = HA_ST_DRQ, returning ...\n");
#endif
		return(0);
	}

	/* Take the Config Data         */
	repinsw(port+HA_DATA, (char *)rcfg, 512 / 2 );

	if ( (status = inb(port + HA_STATUS) ) & HA_ST_ERROR ) {
#ifdef DPT_DEBUG
	printf("EATA_ReadConfig: controller HA_ST_ERROR, status = 0x%X.\n",status);
#endif
		return(0);
	}
#ifdef DPT_DEBUG
	printf("EATA_ReadConfig: signature = %x%x%x%x.\n",rcfg->EATAsignature[0],rcfg->EATAsignature[1],rcfg->EATAsignature[2],rcfg->EATAsignature[3]);
#endif

	/* Verify that it is an EATA Controller		*/
	if (rcfg->EATAsignature[0] != 'E' ||
	    rcfg->EATAsignature[1] != 'A' ||
	    rcfg->EATAsignature[2] != 'T' ||
	    rcfg->EATAsignature[3] != 'A' ) {
#ifdef DPT_DEBUG
	printf("EATA_ReadConfig: signature wrong.\n");
#endif
		return(0);
	}
#ifdef DPT_DEBUG
	printf("EATA_ReadConfig: signature right.\n");
#endif

	return(1);
}


dptH_DMA_Setup(chan)
int chan;
{
	register int Channel;

	Channel = (8 - chan) & 7;  /* DMA channel 5,6,7,0 maps from 3,2,1,0 */

	if (Channel < 4) {
		iooutb(DMA0_3MD, Channel | CASCADE_DMA);
		iooutb(DMA0_3MK, Channel);
	} else {
		iooutb(DMA4_7MD, (Channel & 3) | CASCADE_DMA);
		iooutb(DMA4_7MK,  Channel & 3);
	}

	return(Channel);
}

dpt_illegal(hba, scsi_id, lun, m)
short hba;
unsigned char scsi_id, lun;
int m;
{
	if (sdi_redt(hba, scsi_id, lun)) {
		return 0;
	} else {
		return 1;
	}
}
