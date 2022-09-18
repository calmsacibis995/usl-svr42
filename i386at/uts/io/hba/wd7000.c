/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/hba/wd7000.c	1.39"
#ident	"$Header: $"

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

/*
 *	The following is a SVR4 386 WD7000 Driver. This driver was written 
 *	to talk to an AT&T version of the  Western Digital 7000-ASC Host 
 *	adapter(E037) and has been proven to do so. Some changes were made 
 *	to the Host Adapter to enhance it's functionality and performance 
 *	that did not exist in the "generic" WD Host Adapter at the time this 
 *	driver was developed. These changes may or may not exist in the 
 *	"generic" WD Host Adapter at present or in the future. If this source 
 *	code is modified in an attempt to work with a "generic" WD Host Adapter, 
 *	minimally, the following changes should be made.
 *	
 *		- Disable synchronous negotiations. This can be disabled 
 *		  in this driver by removing the call to wd_sync_init() in 
 *		  the wd_init() routine. This will insure all SCSI transfers 
 * 		  are done in SCSI asynchronous mode.
 *		
 *		- do not use the scatter/gather functionality. This function 
 *		  does not exist in the generic WD firmware. Each scatter/gather 
 *		  job must be broken down into seperate SCSI requests. This 
 *		  will significantly reduce the performance of large raw jobs.
 *		
 *		- remove check for AT&T revision of the firmware in wd_init().
 *		  This check is installed to insure all features the driver 
 *		  requires are supplied in the HA firmware.
 */		  
	


	
#include	<acc/priv/privilege.h>
#include	<svc/errno.h>
#include	<util/types.h>
#include	<util/param.h>
#include	<io/mkdev.h>
#include	<io/conf.h>
#include	<proc/signal.h>
#include	<proc/user.h>
#include	<util/cmn_err.h>
#include	<util/debug.h>
#include	<fs/buf.h>
#include	<svc/systm.h>
#include	<proc/cred.h>
#include	<io/i8237A.h>
#include 	<io/uio.h>
#include	<mem/kmem.h>
#include	<svc/bootinfo.h> 
#include	<io/target/sdi_edt.h>
#include	<io/target/scsi.h>
#include	<io/target/sdi.h>
#include	<io/hba/had.h>
#include	<io/hba/wd7000.h>

#include	<util/mod/moddefs.h>
#include	<io/ddi.h>
#include	<io/ddi_i386at.h>

extern	struct	hba_idata	wd_idata[];
extern	int	wd__cntls;

#define		DRVNAME	"wd7000 - SCSI HBA driver"

STATIC	int	wd__load(), wd__unload();
int	wd_init();
void	wd_start();

MOD_HDRV_WRAPPER(wd_, wd__load, wd__unload, NULL, DRVNAME);

static	int	mod_dynamic = 0;

STATIC int
wd__load()
{
	int ret;

	mod_dynamic = 1;
	if( ret = wd_init() ) {
		return( ENODEV );
	}
	mod_drvattach( &wd__attach_info );
	wd_start();
	return(0);
}

STATIC int
wd__unload()
{
	return( EBUSY );
}

HBA_INFO(wd_, 0, 0x0);

extern long sdi_started;

void		wd_dmainit ();
void		wd_dmalist ();
void		wd_dmafreelist ();
void		wd_sync_init ();
static void	wd_callback();

DMA_LIST	*dma_pool;
DMA_LIST	*dma_free_list;

int wd_devflag = D_NEW | D_DMA;	/* New style driver DMA driver */
struct wd_ctrl	*wd_ctrl;	/* one per HA card = #C 	*/

/* The following are allocated in the space file */

extern struct wd_addr	wd_ad[MAX_TCS];/* SCSI I/O addresses 		*/
extern union cdb_item	wd_cdb_pool[];/* HA driver CDB pool.		*/

long		wd_hacnt;	/* Total # of controllers    = #C	*/
extern int	wd_cdbsz;	/* HA driver CDB pool size		*/

/* The following are driver allocated variables */

int		wd_ctoi[MAX_HAS];
int		wd_itoc[MAX_HAS];
char		wd_sync_buf[25];
union sb_item	wd_scb;	/* SCSI SCB used at init time		*/
union sb_item	*wd_sblist;	/* SCSI SCB free list pointer		*/
long		wd_psz;	/* SCSI SCB data size in bytes		*/
char 		*wd_sbstart;	/* SCSI SCB data area start address	*/
char 		*wd_sbend;	/* SCSI SCB data area end address	*/
char 		*wd_tmp_buff;
long		wd_tot_mem;	/* total size of system memory.		*/

union cdb_item	*wd_cdb_list;	/* HA driver CDB free list pointer	*/
struct ver_no	wd_ver;	/* HA drv. SDI version structure	*/
long		wd_started;	/* HA drv. SDI initialization flag	*/
buf_t 		*sc_bp;		/* Local buffer for sleep at init time	*/
static	long	wd_timer_flag;/* SCSI delay variable			*/
struct ident	inq_data;	/* inquiry data structure		*/
HA_RB		rd_ha_vers;	/* read host adapter version structure */
HA_RB		rexp;		/* read parameters data structure */


static int	send_job();
static int	wd_done();
static int	wd_cmd();
static int	wd_cklu();
static int	wd_getq();
static int	ha_edt();
static int	ha_done();
static void	wd_cint();
static void	wd_default();
static void	wd_reset();
static void	wd_restime();
static void	wd_resetck();
static void	free_cdb();
static void	enqueue();
static void	add_job();
static void	add_ijob();
static void	sub_job();
static void	next_job();
static void	start_jobs();
int		wd_pass_thru();
static struct	wd_cdb_buf	*get_cdb();
void		wd_int();
void		wd_timer();
void		wd_sanity();
void		wd_init_mailboxes();
void		wd_init_srbs();


/*===========================================================================*/
/* Debugging mechanism that is to be compiled only if -DDEBUG is included
/* on the compile line for the driver				       
/* DPR provides levels 0 - 9 of debug information.                  
/*	0: No debugging
/*	1: entry and exit points of major routines
/*	2: entry and exit of major loops within major routines
/*	3: Variable values within major routines
/*	4 - 6: Same as above for ll functions
/*	7 - 9: Same as above for all driver functions
/*============================================================================*/



extern char 	wd_Debug[];
extern char 	wd_Board[];
extern int 	wd_dbgsize;
int		wd_tid;
int		wd_tflag;

#ifdef DEBUG

#define DPR(b,l)	if (((b == 8) || (wd_Board[b])) && wd_Debug[l]) printf

#endif


#define	SCSI_DELAY(x)	wd_timer_flag = 1; \
			timeout (tdelay, NULL, x); \
			while (wd_timer_flag)



/*============================================================================
** Function name: wd_start()
** Description:
**	Called by kernel to perform driver initialization
**	After the kernel data area has been initialized.
*/

void
wd_start ()
{

	int c;

/*
 *	Here we will simply return if the wd_ctrl structure array has
 *	either not been allocated or been freed.  We determine this by
 *	testing for NULL.  If the driver init fails, it set wd_ctrl
 *	to NULL explictly after freeing the kmem.
 */
	if ( wd_ctrl == NULL )
		return;

	/*
	 * this loop turns off init time flag which
	 * stops the HA driver from polling for interrupts 
	 * and begins taking interrupts normally.
	 */
	for (c = 0; c < MAX_HAS; c++) {
		if (wd_ctoi[c] >= 0) {
			SC(wd_ctoi[c]).state &= ~C_INIT_TIME; 
		}
	}

	return;
}


/*============================================================================
** Function name: wd_open()
** Description:
** 	Driver Open Entry Point. It checks permissions. And in the case of
**	a pass-thru open it suspends the particular TC LU queue.
*/

int
wd_open(devp, flags, otype, cred_p)
dev_t	*devp;
cred_t	*cred_p;
int	flags;
int	otype;
{
	dev_t	dev = *devp;
	int		oip;
	register int	c = SC_CONTROL(getminor(dev));
	register int	t = SC_TARGET(getminor (dev));
	register int	l = SC_LUN(getminor (dev));

#ifdef DEBUG
	DPR(8,1)(" open dev=%x c=%d t=%d l=%d ",dev,c,t,l);
#endif

	if (SC_ILLEGAL(c, t))
		return(ENXIO);

	if (drv_priv(cred_p))
	{
		return(EPERM);
	}
	
	if (t == SC(c).ha_id)
		return(0);

	/* This is the pass-thru section */

	oip = spl5 ();

	if (LU(c, t, l).jobs.all)
	{
		splx(oip);
		return(EBUSY);
	}
	else if (t != SC(c).ha_id)
		LU(c, t, l).jobs.t.suspended |= PT_SUSPEND;

	splx (oip);
	return(0);
}



/*============================================================================
** Function name: wd_close()
** Description:
** 	Driver Close Entry Point. In the case of a pass-thru close it 
**	resumes the queue and calls the target driver event handler if 
**	one is present.
*/

int
wd_close(dev, flags, otype, cred_p)
cred_t	*cred_p;
dev_t	dev;	/* External Device # */
int	flags;
int	otype;
{
	int		oip;
	register int	c = SC_CONTROL(getminor (dev));
	register int	t = SC_TARGET(getminor (dev));
	register int	l = SC_LUN(getminor (dev));
	register struct lu_ctrl	*lp = &LU(c, t, l);

#ifdef DEBUG
	DPR(8,1)(" close dev=%x c=%d t=%d l=%d ",dev,c,t,l);
#endif /* DEBUG */

	
	if (SC_ILLEGAL(c, t))
		return(ENXIO);

	lp = &LU(c, t, l);
	oip = spl5 ();
	if (lp->jobs.t.function) 
	{
		cmn_err (CE_WARN,"%s: (HA %u tc %u lu %u), close failed, device busy.\n", wd_idata[c].name, wd_itoc[c], t, l);

	}
	else if (t != SC(c).ha_id) 	/* Resume it */
	{	
		lp->jobs.t.suspended &= ~PT_SUSPEND;
#ifdef DEBUG
		sdi_aen(SDI_FLT_PTHRU, c, t, l);
#else
		if (lp->dv_func != NULL)
			lp->dv_func (lp->dv_param,SDI_FLT_PTHRU);
#endif

		send_job (c, t, l);
	}
	splx (oip);
	return(0);
}


/*============================================================================
** Function name: wd_ioctl()
** Description:
**	Driver ioctl () Entry Point. Used to implement the following 
**	special functions:
**
**
**    SDI_SEND		- Send a user supplied SCSI Control Block to the
**			  specified scsi function.
**    GETVER		- Get the host adapter HW version
**    HA_VER		- Get sdi version structure 
**    REDT		- Read the extended EDT from RAM memory
**    SDI_BRESET	- Reset the specified bus 
**    RD_EXP		- Read the parameters setting the HA is using.
**    WR_EXP		- Write parameters for the HA to use.
**
*/

int
wd_ioctl(dev, cmd, argp, mode, cred_p, rval_p)
cred_t	*cred_p;
int	*rval_p;
dev_t	dev;
int	cmd;
caddr_t	argp;
int	mode;
{
	struct bus_type	b;
	int		oip;
	int		c = SC_CONTROL(getminor (dev));
	int		t = SC_TARGET(getminor (dev));
	int		uerror = 0;
#ifdef DEBUG
	paddr_t		p;
	register short	dbg;
#endif /* DEBUG */
	
#ifdef DEBUG
	DPR (c,1)("\nwd_ioctl: dev=%x c=%d t=%d func=%x ", dev,c,t,cmd);
#endif /* DEBUG */

	if (SC_ILLEGAL(c, t))
	{
		return(ENXIO);
	}
	/* If reset in progress wait for it to complete */
	while (SC(c).state & C_RS_BLK)
	{
		sleep ((caddr_t)&SC(c).state, PZERO);
	}
	switch (cmd)
	{
	case	SDI_SEND:	
		uerror = ll_send_cmd (dev, argp);
		break;
	case	SDI_BRESET:
		oip = spl5 ();
		wdbd_init(c);
		wd_init_srbs(c);
		wd_reset(c);
		wd_resetck(c);
		splx (oip);
		break;
	case	B_GETTYPE:
		b.bus_name[0] = 's';
		b.bus_name[1] = 'c';
		b.bus_name[2] = 's';
		b.bus_name[3] = 'i';
		b.bus_name[4] = NULL;
		b.drv_name[0] = NULL;
		if (copyout ((caddr_t)&b, argp, sizeof (struct bus_type)) != SUCCESS)
			uerror = EFAULT;
		break;
	case	HA_VER:
		if (copyout((caddr_t)&wd_ver,argp, sizeof(struct ver_no)))
			uerror = EFAULT;
		break;
	case	B_HA_CNT:
		if (copyout((caddr_t)&wd_hacnt,argp, sizeof(wd_hacnt)))
			uerror = EFAULT;
		break;

#ifdef DEBUG
	case	RD_EXP:
		oip = spl5 ();
		rw_exp (c, RD_EXP, argp);
		splx (oip);
		break;
	case	WR_EXP:
		oip = spl5 ();
		rw_exp (c, WR_EXP, argp);
		splx (oip);
		break;
	case	TIMEOUTFLG:
		if (wd_tflag = ~wd_tflag)
		{
			wd_tid = timeout (wd_timer, NULL, ONE_MINUTE);
			printf ("\nScsi timeouts are turned on.\n");
		}
		else 
		{
			untimeout (wd_tid);
			printf ("\nScsi timeouts are turned off.\n");
		}
		break;
	case	DEBUGFLG:
		if (copyin (argp, wd_Debug, wd_dbgsize) != SUCCESS)
		{
			return(EFAULT);
		}
		wd_Board[c] = 1;
		printf ("\nNew debug values :");
		{
		int dbg;
		for (dbg = 0; dbg < wd_dbgsize; dbg++)
			printf (" %d", wd_Debug[dbg]);
		}
		if (wd_Debug[0] == 0)
			wd_Board[c] = 0;
		printf ("\n");
		break;
#endif /* DEBUG */
	default:		/* Invalid Request */
		uerror = EINVAL;
		break;
	} 
	return(uerror);
}


/*============================================================================
** Function name: wd_intr()
** Description:
**	Driver Interrupt Handler Entry Point. This code is entered upon 
**	the occurrence of an ha controller interrupt.
*/

void
wd_intr (vect)
register int  vect;	/* HA interrupt vector	*/
{
	SCSI_RB				*srb=NULL;
	unsigned char			status;
	unsigned char			cq_status;
	int				c, cqn;
	int				padr=NULL;

	/* calculate which HA board interrupted */

	for (c = 0; c < wd__cntls; c++) {
		if (wd_ad[c].ha_vect == vect)
			break;
	}

	if (c >= wd__cntls)
	{
#ifdef DEBUG
		cmn_err (CE_WARN,"WD7000: Received an unknown interrupt.\n");
#endif 
		return;
	}
	if ( !wd_idata[c].active) {
#ifdef DEBUG
		cmn_err (CE_WARN,"%s: Inactive HBA received an interrupt.\n", wd_idata[c].name);
#endif 
		return;
	}
	status = inb(INT_STATUS(c));	/* read interrupt status REG */

#ifdef DEBUG
	DPR(8,1)("\nWD INT(%d) S=0x%x ", c, status);
#endif
	if (status & CMD_COMPLETE)
	{
		cqn = (status & 0x3F);			/* get completion Q */
		cq_status = SC(c).cq[cqn].status;	/* number and status*/
		
		switch (cq_status)
		{
		case TC_RESET:
		case AB_RESET:
		case SPC_ERROR:
			cmn_err (CE_WARN,"!%s: HA %d Failure, resetting bus", wd_idata[c].name,wd_itoc[c]);
			wdbd_init(c);
			wd_init_srbs(c);
			wd_reset(c);
			wd_resetck(c);
			next_job(c);
			return;
			break;
		case NO_ERROR:		/* these completion cases have	*/
		case CK_SSB:		/* a pointer in the completion  */
		case HA_ERROR:		/* Q address field. 		*/

			/* get SRB address and process the completion	*/

			padr = sdi_swap24(SC(c).cq[cqn].pdp);
			srb = (SCSI_RB *) xphystokv(padr);
#ifdef DEBUG
			DPR(8,9)("padr=0x%x srb=0x%x\n",padr,srb);
#endif
			outb(INT_ACK(c), 0);	/* acknowledge the interrupt */

			if ((srb->opcode == SCSI_CMD) || 
			   (srb->opcode == SCSI_DMA_CMD))
				wd_done (c, srb, cq_status);
			else 
				ha_done(c, srb,cq_status);
			break;
		default:
			cmn_err (CE_NOTE,"!%s: HA %u Illegal completion status=%x\n", wd_idata[c].name, wd_itoc[c], cq_status);
			break;
		}
	}
	else
	{
		cmn_err (CE_NOTE,"!%s: Unexpected interrupt from HA %d status = 0x%x", wd_idata[c].name, wd_itoc[c],status);
	}
	if (srb == NULL)		/* Int. not yet acknowledged */
		outb(INT_ACK(c), 0);

	next_job (c);
	return;
} 

/*============================================================================
** Function name: wd_done()
** Description:
**	This is the interrupt handler routine for SCSI jobs which have
**	a SCSI request block and SCB structure defining the job.
*/

static int
wd_done (c, srb, cq_status)
register int  	c;		/* HA Controller number	    */
SCSI_RB		*srb;		/* SRB pointer for this job */
int		cq_status;	/* completion Q status byte */
{
	register struct wd_ctrl	*scp = &SC(c);
	register struct lu_ctrl		*lp;
	register struct sb2		*psb;
	char				t,l;
	unsigned char			ssb,hasb;


	/* get job TC, LU and status bytes from the srb */

	t = (srb->dev >> 5);
	l = (srb->dev & 0x07);
	ssb = srb->ssb;
	hasb= srb->hasb;

	/* release the srb to the free list	*/

	((union srb_buf *)srb)->next = NULL;
	((union srb_buf *)srb)->next = scp->srb_list;
	scp->srb_list = (union srb_buf *) srb;

#ifdef DEBUG
	DPR(8,1)("c=%d t=%d l=%d cqs=0x%x ssb=0x%x hasb=0x%x\n", 
			c, t, l, cq_status, ssb, hasb);
#endif
	lp = &LU(c, t, l);
	psb = (struct sb2 *) lp->jp;

	/* check if there are no active jobs or the completion */
	/* is for a TC LU combination which has no active jobs */

	if ((!scp->active_jobs) || ((psb == NULL) || (!lp->jp->sp_sent)))
	{
#ifdef DEBUG
		if (!scp->active_jobs)
			DPR (8,0)("WD7000: active_jobs not set\n");
		cmn_err (CE_WARN,"%s: Unexpected completion reported", wd_idata[c].name);
		cmn_err (CE_CONT,"for HA %u tc %u lu %u.\n", wd_itoc[c], t, l);
#endif
		return;
	}
	/* At this point we received a valid 	*/
	/* completion update job control flags	*/

	scp->active_jobs--;
	scp->jobs--;

	if (psb->sbp->sb_type == SCB_TYPE)
		lp->jobs.t.normal--;
	else
	{
		lp->jobs.t.immediate--;
		if (psb->sbp->SCB.sc_int == wd_int)
			lp->jobs.t.suspended &= ~PT_ACTIVE;
	}
	/* determine completion status of the job */

	switch (cq_status)
	{
	case NO_ERROR:
		psb->sbp->SCB.sc_comp_code = SDI_ASW;
		break;
	case CK_SSB:
		if (ssb == S_GOOD)	/* good SCSI status */
			psb->sbp->SCB.sc_comp_code = SDI_ASW;
		else
		{
			psb->sbp->SCB.sc_comp_code = SDI_CKSTAT;
			psb->sbp->SCB.sc_status = ssb;
		}
		break;
	case HA_ERROR:
		if (hasb == NO_SELECT)
			psb->sbp->SCB.sc_comp_code = SDI_NOSELE;

		/* HA was required to do padding(0x41). This flag may not
		 * manifest itself in this manner with other (non-AT&T) 
		 * versions of firmware.
		 */ 
		else if (hasb == 0x41) 
			psb->sbp->SCB.sc_comp_code = SDI_ASW;
		else
			psb->sbp->SCB.sc_comp_code = (SDI_HAERR | SDI_SUSPEND);
		break;
	case B_RESET:
		psb->sbp->SCB.sc_comp_code = SDI_RESET;
		break;
	default:	/* other cases already handled in wdint() */
		break;
	}
	if ((psb->sbp->SCB.sc_comp_code & SDI_SUSPEND) && 
		(psb->sbp->SCB.sc_int != wd_int) && (psb->sbp->SCB.sc_int != wd_cint))
	{
		lp->jobs.t.suspended |= SUSPEND;
	}
	sub_job (psb, c, t, l);
	send_job (c, t, l);

	wd_callback(psb);

	return;
} 

/*============================================================================
** Function name: ha_done()
** Description:
**	This is the interrupt handler routine for HA jobs which have
**	a request block structure defining the job.
*/

static int
ha_done (c, ha_rb, cq_status)
register int  	c;		/* HA Controller number	    */
HA_RB		*ha_rb;		/* HA request pointer for this job */
int		cq_status;	/* completion Q status byte */
{

#ifdef DEBUG
	DPR(8,1)("ha_done: NON SCSI COMPLETION opcode=%x\n",ha_rb->opcode);
#endif

	SC(c).state &= ~C_REDT_REQ;	
	wakeup ((caddr_t)&SC(c).state);

}

/*============================================================================
** Function name: wd_reset()
** Description:
**	This is called upon the occurence of a SCSI bus reset. It starts
**	the 5 second delay during which no jobs are sent to the HA board.
**	It notifies the target drivers which have an event handler
**	assigned and also schedules wd_restime() to be called after 5 
**	seconds.
*/

static void
wd_reset (c)
register int	c;	/* Controller number */
{
	register struct wd_ctrl	*scp = &SC(c);
	register struct lu_ctrl		*lp;
	int				t, l;

	if (scp->state & C_RS_BLK)
	{
		scp->state &= ~C_RS_BLK;
		scp->state |= C_RS_START;
		untimeout(scp->resid);
		scp->resid = timeout(wd_restime,(caddr_t) c, 5*ONE_SECOND);
		return;
	}
	scp->state |= C_RS_START;
	scp->resid = timeout (wd_restime,(caddr_t) c, 5*ONE_SECOND);

	for (t = 0; t < MAX_TCS; t++)	/* Notify target drivers */
	{
		for (l = 0; l < MAX_LUS; l++)
		{
			lp = &LU(c, t, l);
#ifdef DEBUG
			sdi_aen(SDI_FLT_RESET, c, t, -1);
#else
			if (lp->dv_func != NULL)
				lp->dv_func (lp->dv_param,SDI_FLT_RESET);
#endif
		}
	}
	return;
}

/*============================================================================
** Function name: wd_restime()
** Description:
**	Called after 5 seconds to start the 1 second dalay timing during
**	SCSI bus resets during which only immediate jobs can be sent.
**	This function schedules itself to run in 1 second to stop the
**	timing sequence.
*/

static void
wd_restime (c)
register int	c;	/* Controller number */
{
	register struct wd_ctrl	*scp = &SC(c);

	if (scp->state & C_RS_START)
	{
		scp->state &= ~C_RS_START;
		scp->state |= C_RS_IMM;
		scp->resid = timeout (wd_restime, (caddr_t)c, 1*ONE_SECOND);
#ifdef DEBUG
		DPR (c,10)("[");
#endif
	}
	else
	{
		scp->state &= ~C_RS_IMM;
		wakeup ((caddr_t)&scp->state); 
#ifdef DEBUG
		DPR (c,10)("]\n");
#endif
	}
	start_jobs(c);
}

/*============================================================================
** Function name: wd_resetck()
** Description:
**	This routine checks for any jobs
**	not returned and deletes them from the LU Qs and returns them 
**	to the target drivers.
*/

static void
wd_resetck (c)
register int	c;	/* Controller number */
{
	register struct wd_ctrl	*scp = &SC(c);
	register struct lu_ctrl		*lp;
	register struct sb2		*sp, *next, *prev;
	int				t, l;

	if (!(scp->state & C_RS_START))
		return;
	for (t = 0; t < MAX_TCS; t++)
	{
	    for (l = 0; l < MAX_LUS; l++)
	    {
		lp = &LU(c, t, l);
		prev = NULL;
		next = lp->jp;
		while (prev != next)
		{
			sp = next;	/* sp is current job to check */
			prev = next;
			next = next->sp_forw;
			if (next == lp->jp)	/* first job stop */
				next = prev;
			if (sp->sp_sent)
			{

#ifdef DEBUG
  DPR (c,10)("sp=0x%x sp_sent=%d type=%d forw=0x%x back=0x%x\n",sp,sp->sp_sent, sp->sbp->sb_type,sp->sp_forw,sp->sp_back);
#endif
				switch (sp->sbp->sb_type) 
				{
				case SCB_TYPE:
				case ISCB_TYPE:
					if (scp->active_jobs > 0) 
					{
						scp->active_jobs--;
						scp->jobs--;
					}
					if (sp->sbp->sb_type == SCB_TYPE) 
					{
						if (lp->jobs.t.normal > 0 )
							lp->jobs.t.normal-- ;
					}
					else
					{
						if (lp->jobs.t.immediate > 0 )
							lp->jobs.t.immediate-- ;
					}
					sub_job (sp, c, t, l);
					if (sp->sbp->SCB.sc_comp_code != SDI_TIME)
						sp->sbp->SCB.sc_comp_code = SDI_RESET;
					wd_callback(sp);
					break;
				default:
#ifdef DEBUG
					cmn_err (CE_WARN,"%s: Illegal type on SCB job list for lu %u, tc %u, HA %u.\n",wd_idata[c].name,l,t,scp->maj);
#endif 
					break;
				}
			}
		}
		if ((lp->fp != NULL) && (lp->fp->sp_sent))
		{
#ifdef DEBUG
  DPR (c,10)("Recovering from outstanding SFB job oustanding\n");
			cmn_err (CE_WARN, "%s: Recovering from SFB job mismatch on lu %u, tc %u, HA %u.\n",wd_idata[c].name,l,t,scp->maj);
#endif
			if (lp->fp->sbp->sb_type == SFB_TYPE) 
			{
				if (scp->active_jobs > 0) 
				{
					scp->active_jobs--;
					scp->jobs--;
				}
				if (lp->jobs.t.function > 0 )
					lp->jobs.t.function-- ;
				sp = lp->fp;
				lp->fp = NULL;
				sp->sp_sent = FALSE;
				sp->sbp->SFB.sf_comp_code = SDI_RESET;

				wd_callback(sp);
			}
		}
	    }
	}
#ifdef DEBUG
  	DPR (c,10)(" out st =0x%x \n",scp->state);
#endif
}

/*============================================================================
** Function name: wd_timer()
** Description:
**	This function is scheduled every minute to perform timing on SCB
**	associated jobs. Timing is not performed if the LU is open for
**	pass-tru although the pass-tru job itself is timed. If a job times
**	out a SCSI Bus reset is sent to the HA card and the SANITY flag is
**	set. The falg is cleared as soon as a completion comes back from
**	the HA card. If no completions return from the HA board within
**	20 seconds the HA is marked out-of-service. If a reset can not be
**	sent because the Q is busy jobs are stopped and 10 seconds later
**	the reset is sent by wd_timout() regardless.
*/

void
wd_timer ()
{
	register int		c;
	register int		t;
	register int		l;
	int			i;
	register struct sb2	*p;
	register struct lu_ctrl	*lp;

	for (i = 0; i < MAX_HAS; i++) { 	/* HA Card           */
		if ((c = wd_ctoi[i]) < 0 || SC(c).jobs == NULL)
			continue;
	for (t = 0; t < MAX_TCS; t++)  {	/* Target Controller */
	   for (l = 0; l < MAX_LUS; l++) {	/* Logical Unit      */
		lp = &LU(c, t, l);

		/* if queue is "pass thru suspended" and no pass thru 
		   jobs are active or  the queue is pump suspended */
		if (((lp->jobs.t.suspended&PT_SUSPEND) &&
			(!(lp->jobs.t.suspended&PT_ACTIVE))) ||
			(lp->jobs.t.suspended&PUMP_SUSPEND))
		{
			continue;
		}
		
		/* if there are no jobs for that queue */
		if ((p = lp->jp) == NULL) {
			if ((p = lp->fp) == NULL)
				continue;
		}
		if ((lp->jobs.t.suspended&SUSPEND) && (p == lp->jp) &&
			(p->sbp->sb_type == SCB_TYPE))
		{
			continue;
		}
		p->sp_time -= ONE_MINUTE;

		if (p->sp_time > 0) {
			continue;
		}
		cmn_err (CE_NOTE,"!%s: Job timeout for HA %u tc %u lu %u\n",
					wd_idata[c].name,wd_itoc[c],t,l);
		p->sbp->SCB.sc_comp_code = SDI_TIME;
		/* has job been sent? */
		if (!p->sp_sent)
		{
#ifdef DEBUG
	DPR(8,1)("WD7000: TIMEOUT JOB NOT SENT \n");
#endif
			SC(c).jobs--;
			switch (p->sbp->sb_type)
			{
			case SCB_TYPE:
				if (lp->jobs.t.normal > 0)
					lp->jobs.t.normal--;
				break;
			case ISCB_TYPE:
				if (lp->jobs.t.immediate > 0)
					lp->jobs.t.immediate--;
				break;
			case SFB_TYPE:
				if (lp->jobs.t.function > 0)
					lp->jobs.t.function--;
				break;
			default:
				break;
			}
			if ((p->sbp->sb_type != SFB_TYPE) &&
				(TC(c, t).waiting_jobs))
			{
				SC(c).waiting_jobs--;
				TC(c, t).waiting_jobs--;
			}
			p->sp_sent = TRUE;	/* fake out sub_job */
			sub_job(p, c, t, l);
			wd_callback(p);
			continue;
		}
		else	/* timed out job is on ha */
		{
			/* reset the card and proceed */
			wdbd_init(c);
			wd_init_srbs(c);
			wd_reset(c);
			wd_resetck(c);
		}
	   } /* for l */
	} /* for t */
	} /* for c */
#ifdef DEBUG
	if (wd_tflag) {
		wd_tid = timeout (wd_timer, NULL, ONE_MINUTE);
	}
#else
	(void) timeout (wd_timer, NULL, ONE_MINUTE);
#endif
	return;
}

/*============================================================================
** Function name: wd_sanity()
** Description:
**	This function is called 20 seconds after a job has timed out. It
**	checks the sanity flag. If its not set it means the HA is working.
**	If the flag is still set the HA card is no longer responding so
**	all queues are cleaned up and the jobs returned to the target
**	drivers.
*/

void
wd_sanity (c)
register int	c;
{
	register int		t;
	register int		l;
	register struct sb2	*p;

	if ((SC(c).state & C_SANITY) == NULL) 
	{
#ifdef DEBUG
		DPR (8,0)("Firmware in HA %d is sane.\n", wd_itoc[c]);
#endif
		return;
	}
	SC(c).state &= ~C_OPERATIONAL;

	for (t = 0; t < MAX_TCS; t++) 
	{
		for (l = 0; l < MAX_LUS; l++) 
		{
			while (((p = LU(c, t, l).jp) != NULL) ||
				((p = LU(c, t, l).fp) != NULL)) 
			{
#ifdef DEBUG
				DPR (8,0)("\nKilling job : psb = 0x%x ", p);
				DPR (8,0)("type = %d ", p->sbp->sb_type);
				DPR (8,0)("c = %d t = %d l = %d\n", c, t, l);
#endif
				p->sp_sent = TRUE;
				switch (p->sbp->sb_type) 
				{
				case SCB_TYPE:
					sub_job (p, c, t, l);
					p->sbp->SCB.sc_comp_code = SDI_TIME;

					wd_callback(p);
					break;
				case ISCB_TYPE:
					sub_job (p, c, t, l);
					p->sbp->SCB.sc_comp_code = SDI_TIME;

					wd_callback(p);
					break;
				case SFB_TYPE:
					p->sbp->SFB.sf_comp_code = SDI_TIME;

					wd_callback(p);
					LU(c, t, l).fp = NULL;
					break;
				default:
					break;
				}
			}
			LU(c, t, l).jobs.all = NULL;
		}
		TC(c, t).waiting_jobs = NULL;
	}
	SC(c).jobs = NULL;
	SC(c).active_jobs = NULL;
	SC(c).waiting_jobs = NULL;
/*
	if (SC(c).free_rq == 0) 	FIX 
	{
		SC(c).rq->fr_busy = FALSE; 
#ifdef DEBUG
   		DPR (8,0)("wd_sanity: RESET BUSY bit\n");
#endif
	}
*/

}

/*============================================================================
** Function name: wd_init()
** Description:
**	This is the initialization routine for the HA driver. All data
**	structures are initiliazed. The board is initialized and the EDT
**	is build for every HA configured in the system.
*/

int
wd_init ()
{
	register int	c, t, l;
	int		wd_lus, structsize, dma_listsize, cntl_num;
	union sb_item	*p;
	uint	bus_p;
	int	sleepflag;
	int	wd_active[MAX_HAS];

	if (wd_started) {
		cmn_err(CE_WARN,"WD7000: Already initialized.");
		return(-1);
	}

#ifdef DEBUG
	wd_tflag = TRUE;
#endif

	/* if running in a micro-channel machine, skip initialization */
	if (!drv_gethardware(IOBUS_TYPE, &bus_p) && (bus_p == BUS_MCA)) {
		wd_ctrl = NULL;	/* this lets wd_start know we failed */
		return(-1);
	}

	sleepflag = mod_dynamic ? KM_SLEEP : KM_NOSLEEP;
	wd_ver.sv_release = 1;
	wd_ver.sv_machine = SDI_386_AT;
	wd_ver.sv_modes = SDI_BASIC1;

#ifdef DEBUG
	DPR (8,0)("\tWD7000-386 DEBUG DRIVER INSTALLED\n");
#endif

	structsize = wd__cntls * (sizeof (struct wd_ctrl));
	for (l = 128; l < structsize ; l *= 2) /* NULL BODY */ ;
	structsize = l;

	dma_listsize = NUM_DMA_LISTS * (sizeof (DMA_LIST));
	for (l = 2; l < dma_listsize; l *= 2);
	dma_listsize = l;

#ifdef DEBUG
	if(structsize > PAGESIZE)
		cmn_err(CE_WARN, "%s: wd_ctrl structs exceed pagesize (may cross physical page boundary)\n", wd_idata[0].name);
	if(dma_listsize > PAGESIZE)
		cmn_err(CE_WARN, "%s: dmalist exceeds pagesize (may cross physical page boundary)\n", wd_idata[0].name);
#endif /* DEBUG */

	wd_ctrl = (struct wd_ctrl *)kmem_zalloc(structsize, sleepflag);
	if (wd_ctrl == NULL) {
		cmn_err(CE_WARN,"%s: Initialization error - cannot allocate wd_ctrl", wd_idata[0].name);
		return -1;
	}

	if((dma_pool = (DMA_LIST *)kmem_zalloc(dma_listsize, sleepflag)) == NULL) {
		cmn_err(CE_WARN, "%s: Initialization error - cannot allocate dmalist\n", wd_idata[0].name);
		kmem_free(wd_ctrl, structsize );
		return(-1);
	}

	/* initialize init time SCB */
	wd_scb.sb2.sbp = sdi_getblk();
	wd_scb.sb2.sbp->SCB.sc_comp_code = SDI_NOALLOC;

	/* Initialize the pool of cdb's */
	for (l = 0; l < wd_cdbsz - 1; l++) {
		wd_cdb_pool[l].next = &wd_cdb_pool[l+1];
	}
	wd_cdb_pool[l].next = NULL;
	wd_cdb_list = &wd_cdb_pool[0];

	for (l = 0; l < MAX_HAS; l++) {
		wd_ctoi[l] = -1;
		wd_itoc[l] = -1;
	}

	wd_lus = 0;

	for( c=0; c<wd__cntls; c++) {
		wd_ad[c].ha_status = wd_idata[c].ioaddr1;
		wd_ad[c].cmd_reg = wd_idata[c].ioaddr1;
		wd_ad[c].int_status = wd_idata[c].ioaddr1 + 1;
		wd_ad[c].int_ack = wd_idata[c].ioaddr1 + 1;
		wd_ad[c].control_reg = wd_idata[c].ioaddr1 + 2;
		wd_ad[c].ha_vect = wd_idata[c].iov;

		SC(c).state    = NULL;
		SC(c).max_jobs = NULL;
		SC(c).jobs     = NULL;
		SC(c).tc_cnt   = NULL;
		SC(c).ha_id    = HA_ID;
		SC(c).resid    = NULL;
		SC(c).waiting_jobs = NULL;
		SC(c).active_jobs = NULL;
		SC(c).next_tc = NULL;
		SC(c).next_lu = NULL;
		SC(c).bp      = NULL;
		SC(c).srb_list = NULL;

		for (t = 0; t < MAX_TCS; t++) {
			TC(c, t).lu_cnt = NULL;
			TC(c, t).waiting_jobs = NULL;

			for (l = 0; l < MAX_LUS; l++) {
				LU(c, t, l).jobs.all = NULL;
#ifdef DEBUG
#else
				LU(c, t, l).dv_func = NULL;
				LU(c, t, l).dv_param = NULL;
#endif
				LU(c, t, l).fp       = NULL;
				LU(c, t, l).jp       = NULL;
			}
		}

		wd_init_mailboxes(c);
		wd_init_srbs(c);

		/* 
	 	* Set init_time flag which the driver uses to
	 	* decide whether to poll the HA or take interrupts.
	 	* (Sleeping is not allowed at init time.)
	 	*/ 
		SC(c).state |= C_INIT_TIME; 

		if(wdbd_init(c) != PASS) {
			continue;
		}
		/* initialize HA communication */
		ha_edt (c, sleepflag);	/* build edt  */
		wd_sync_init (c); 

		for (t=0; t < MAX_TCS; t++) {
			wd_lus += TC(c, t).lu_cnt;
		}
		wd_idata[c].active = 1;
	}
	if( wd_lus == 0 ) {
		cmn_err(CE_NOTE,"!%s:  No HA's found.\n", wd_idata[0].name);
		kmem_free(wd_ctrl,structsize);
		wd_ctrl = NULL;
		return -1;
	}

	/* Calculate total size of memory in the system. Needed for 
	 * a WD 7000-ASC HA bug that is described in wd_dmalist()
	 */
	wd_tot_mem = (ptob(physmem) + 0xfffff) & 0xff00000;

#ifdef DEBUG
	DPR (8,0)("WD7000: TOTAL MEMORY = %D.\n",wd_tot_mem);
#endif

	if ((wd_tmp_buff=(char*)kmem_zalloc(NBPP,sleepflag)) == NULL) {
		cmn_err (CE_WARN,"%s: Initialization failure - cannot allocate buffer.\n", wd_idata[0].name);
		kmem_free(wd_ctrl,structsize);
		kmem_free(dma_pool,dma_listsize);
		wd_ctrl = NULL;
		dma_pool = NULL;
		return -1;
	}

	/* minimally allocate SCBs for at least one LU	*/
	if (wd_lus <= 0) {
		wd_lus = 1;
	}

	/* get byte size of SCB data space */
	wd_psz = wd_lus * N_SCBS * sizeof(union sb_item);

	if ((wd_sbstart = kmem_zalloc(wd_psz, sleepflag)) == NULL) {
		cmn_err(CE_WARN,"%s: Initialization failure - cannot allocate sb_item.\n", wd_idata[0].name);
		kmem_free(wd_ctrl,structsize);
		kmem_free(wd_tmp_buff,NBPP);
		kmem_free(dma_pool,dma_listsize);
		wd_ctrl = NULL;
		dma_pool = NULL;
		wd_tmp_buff = NULL;
		return -1;
	}

#ifdef DEBUG
	DPR(8,4)("wd_init_scbs: sizeof(sb_item) = %u bytes, total space = %u.\n", sizeof(union sb_item), wd_psz);
#endif
	
	/* link up all SCBs */
	wd_sbend = wd_sbstart + wd_psz;
	for (p = (union sb_item *) wd_sbstart; 
			(p < (union sb_item *)wd_sbend); p++) {
		p->sb2.sbp = NULL;
		p->sb2.sp_vip = NULL;
		p->sb2.sp_isz = NULL;
		p->sb2.sp_sent = FALSE;
		p->next = p + 1;
	}
	--p;
	p->next = NULL;

	wd_sblist = (union sb_item*)wd_sbstart; /* init head of list */

	wd_dmainit ();

	/* Get an HBA number from SDI and Register HBA with SDI */
	for( c=0; c<wd__cntls; c++) {
		if( !wd_idata[c].active ) {
	     		cmn_err (CE_NOTE,"%s: HA %d, not active.\n", wd_idata[c].name, c);
			continue;
		}
		if( (cntl_num = sdi_gethbano( wd_idata[c].cntlr )) <= -1) {
	     		cmn_err (CE_NOTE,"%s: No HBA number available.\n", wd_idata[c].name);
			continue;
		}

		wd_idata[c].cntlr = cntl_num;
		wd_ctoi[cntl_num] = c;
		wd_itoc[wd_ctoi[cntl_num]] = cntl_num;

		if( (cntl_num = sdi_register(&wd_hba_info, &wd_idata[c])) < 0) {
	     		cmn_err (CE_NOTE,"%s: HA %d, SDI registry failure %d.\n",
				wd_idata[c].name, c, cntl_num);
			continue;
		}
		wd_hacnt++;

#ifdef DEBUG
		DPR (8,1)("WD hba %d is SDI hba %d\n", c, cntl_num);
#endif
	}

	if (wd_hacnt == 0) {
		cmn_err(CE_NOTE,"!%s: No HAs found.\n", wd_idata[0].name);
		kmem_free(wd_ctrl,structsize);
		kmem_free(wd_tmp_buff,NBPP);
		kmem_free(wd_sbstart,wd_psz);
		kmem_free(dma_pool,dma_listsize);
		wd_ctrl = NULL;
		wd_tmp_buff = NULL;
		wd_sbstart = NULL;
		dma_pool = NULL;
		return -1;
	}
	wd_timer ();
	wd_started = TRUE;
	return(0);
}



void
wd_init_mailboxes(c)
int	c;
{
int	i;
	
	for (i = 0; i < NUM_OGMB; i++)
	{
		SC(c).rq[i].status = NULL;
		SC(c).rq[i].pdp = NULL;
	}
	for (i = 0; i < NUM_ICMB; i++)
	{
		SC(c).rq[i].status = NULL;
		SC(c).rq[i].pdp = NULL;
	}
}


void
wd_init_srbs(c)
int	c;
{
int	i;

	SC(c).srb_list = NULL;

	/* Initialize the SRB linked list */
	for (i = 0; i < (NUM_OGMB - 1) ; i++)
		SC(c).srb[i].next = &SC(c).srb[i+1];

	SC(c).srb[i].next = NULL;
	SC(c).srb_list = &SC(c).srb[0];
}

void
wd_sync_init(c)
int	c;
{
	int 	rqn,i;
	int 	padr=NULL;
	unsigned char cmd;
	
	for (i=0;i<25;i++)
		wd_sync_buf[i] = NULL;	/* clear it out */
		
	/* negotiate for 4 Meg with an offset of 12 */
	for (i=0;i<16;i=i+2)
		wd_sync_buf[i] = FOUR_MEG | OFFSET_12;

	rexp.opcode = SET_EXECUT_PARAMS;	/* write exp */

	rexp.bytes[0] = NULL;	/*reserved byte */
	rexp.bytes[1] = NULL;
	rexp.bytes[2] = NULL;
	rexp.bytes[3] = 16;	/* xfer size */

	padr = vtop(wd_sync_buf, NULL);

	rexp.bytes[4] = msbyte(padr);	/* cmd data area read from */
	rexp.bytes[5] = mdbyte(padr);
	rexp.bytes[6] = lsbyte(padr);
	rexp.bytes[7] = NULL;
	rexp.bytes[8] = NULL;
	rexp.bytes[9] = NULL;
	rexp.bytes[10] = NULL;
	rexp.bytes[11] = NULL;
	rexp.bytes[12] = NULL;
	rexp.bytes[13] = NULL;
	rexp.status = NULL;

	/* set up the request Q entry */

	rqn = wd_getq (c);
	SC(c).rq[rqn].pdp = sdi_swap24(vtop((caddr_t)&rexp, NULL));

	SC(c).rq[rqn].status = RQ_BUSY;
	cmd = (RD_MB | (rqn & 0x3F));	/* send cmd to HA board */
	ha_cmd(c, cmd);

	if (wd_wait_for_comp(c,1000000) == FAIL) /* 1 sec */
	{
		cmn_err (CE_WARN,"!%s: Write Parameters command timed out.\n",
			wd_idata[c].name);
	}

}


/*============================================================================
** Function name: wd_send()
** Description:
**	Send the command descriptor block to the controller.
**	Commands sent via this function are executed in the order they
**	are received.
**
**	Return values:
**		 0   : Request has not completed, so the target driver
**		       interrupt handler will be called.
**		-1   : The type field is invalid.
**		 1   : The job has not completed, and the target driver
**		       should retry the job later.
*/

long
wd_send (sp)
register struct	sb2	*sp;
{
	register struct lu_ctrl		*lp;
	register struct wd_ctrl	*cp;
	register int	c = SDI_CONTROL(sp->sbp->SCB.sc_dev.sa_fill);
	register int	t = SDI_TARGET(sp->sbp->SCB.sc_dev.sa_fill);
	register int	l = sp->sbp->SCB.sc_dev.sa_lun;
	int		oip;

	c = wd_ctoi[c];
#ifdef DEBUG
	DPR (8,2)("wd_send c%d t%d l%d ",c,t,l);
#endif
	if ((sp == NULL) || (sp->sbp->sb_type != SCB_TYPE))
	{
#ifdef DEBUG
		cmn_err (CE_WARN,"%s: wd_send called with an illegal pointer.\n", wd_idata[c].name);
#endif
		return (SDI_RET_ERR);
	}
	if ((sp < (struct sb2 *) wd_sbstart) || 
			(sp > (struct sb2 *) wd_sbend))
	{
#ifdef DEBUG
		cmn_err (CE_WARN,"%s: wd_send SCB not allocated by WD7000\n", wd_idata[c].name);
#endif
		return (SDI_RET_ERR);
	}
	if (sp->sbp->SCB.sc_datasz > 131072) /* WD HA does not support > 128K jobs */ 
	{
		sp->sbp->SCB.sc_comp_code = SDI_SCBERR;
		return (SDI_RET_ERR);
	}
	lp = &LU(c, t, l);
	cp = &SC(c);
	oip = spl5 ();
	if (lp->jobs.t.normal >= cp->max_jobs)
	{
		splx (oip);
		return (SDI_RET_RETRY);
	}
	if (((cp->state & C_OPERATIONAL) == NULL) && !lp->jobs.t.suspended)
	{
		splx (oip);
		sp->sbp->SCB.sc_comp_code = SDI_OOS;
		timeout (wd_callback, (caddr_t)sp, ONE_MSEC);
		return (SDI_RET_OK);
	}
	if (sp->sbp->SCB.sc_time < 0) 
	{
		sp->sbp->SCB.sc_comp_code = SDI_SCBERR;
		timeout (wd_callback, (caddr_t) sp, ONE_MSEC);
		splx (oip);
		return (SDI_RET_OK);
	}

	/* convert milliseconds to ticks; but only if > 0 */
	if (sp->sbp->SCB.sc_time)
	{
         	if (sp->sbp->SCB.sc_time < 1000 * 1000)
                        sp->sbp->SCB.sc_time = drv_usectohz(sp->sbp->SCB.sc_time * 1000);
     		else
                    	sp->sbp->SCB.sc_time = drv_usectohz(sp->sbp->SCB.sc_time) *
1000;
	}
		
	sp->sbp->SCB.sc_comp_code = SDI_PROGRES;
	lp->jobs.t.normal++;
	cp->jobs++;
	add_job (sp, c, t, l);
	send_job (c, t, l);
	splx (oip);

	return (SDI_RET_OK);
}

/*============================================================================
** Function name: wd_icmd()
** Description:
**	Send an immediate command. Only one immediate command
**	may be sent per logical unit at a time. If the logical unit
**	is busy, the job will be queued until the unit is free.
**	SFB operations are executed in the order they are submitted,
**	and will take priority over the SCB operations.
**
**	Return values:
**		 0   : Request has not completed, so the target driver
**		       interrupt handler will be called.
**		-1   : The type field is invalid.
**		 1   : The job has not completed, and the target driver
**		       should retry the job later.
*/

static	struct	ident	nodev_inq_data;

long
wd_icmd (psb)
register struct sb2 *psb;
{
	register struct lu_ctrl	*lp;
	register int	c;
	register int	t;
	register int	l;
	int		oip;
	int		opcode;
	int		i;
	struct scs	*inq_cdb;

	if (psb == NULL)
	{
#ifdef DEBUG
		cmn_err (CE_WARN,"WD7000: wd_icmd called with an illegal pointer.\n");
#endif
		return (SDI_RET_ERR);
	}
	if ((psb < (struct sb2 *) wd_sbstart) || 
			(psb > (struct sb2 *) wd_sbend))
	{
#ifdef DEBUG
		cmn_err (CE_WARN,"WD7000: wd_icmd SCB not allocated by WD7000");
#endif
		return (SDI_RET_ERR);
	}
	oip = spl5 ();

	switch (psb->sbp->sb_type)
	{
	case	SFB_TYPE:
		c = SDI_CONTROL(psb->sbp->SFB.sf_dev.sa_fill);
		t = SDI_TARGET(psb->sbp->SFB.sf_dev.sa_fill);
		l = psb->sbp->SFB.sf_dev.sa_lun;
		c = wd_ctoi[c];
		lp = &LU(c, t, l);
#ifdef DEBUG
		DPR (8,2)(" wd_icmd SFB c%d t%d l%d ",c,t,l);
#endif

		psb->sbp->SFB.sf_comp_code = SDI_ASW;
		opcode = DOS;
		switch (psb->sbp->SFB.sf_func) 
		{
		case	SFB_NOPF:	/* Do nothing */
			break;
		case	SFB_RESETM:	/* Reset message */
			opcode = TCRST;
			break;
		case	SFB_ABORTM:	/* Abort message */
			if (SC(c).state & C_RAM)
				 opcode = ABORT;
			break;
		case	SFB_SUSPEND:
			lp->jobs.t.suspended |= SUSPEND;
			break;
		case	SFB_RESUME:
			lp->jobs.t.suspended &= ~SUSPEND;
			send_job (c, t, l);
			break;
		case	SFB_FLUSHR:	
			wd_flushq(c, t, l);
			break;
		default:
#ifdef DEBUG
			cmn_err (CE_WARN,"%s: wd_icmd called with an illegal opcode of %u.\n", wd_idata[c].name, psb->sbp->SFB.sf_func);
#endif
			psb->sbp->SFB.sf_comp_code = SDI_SFBERR;
			break;
		}
		if (((SC(c).state & C_OPERATIONAL) == NULL) &&
				(!lp->jobs.t.suspended))
		{
			psb->sbp->SFB.sf_comp_code = SDI_OOS;
			timeout (wd_callback, (caddr_t)psb, ONE_MSEC);
			splx (oip);
			return (SDI_RET_OK);
		}
		if (lp->jobs.t.function)
		{
			psb->sbp->SFB.sf_comp_code = SDI_ONEIC;
			timeout (wd_callback, (caddr_t) psb, ONE_MSEC);
			splx (oip);
			return (SDI_RET_OK);
		}
		if (opcode == DOS) 
		{
			timeout (wd_callback, (caddr_t)psb, ONE_MSEC);
			splx (oip);
			return (SDI_RET_OK);
		}
		lp->jobs.t.function++;
		SC(c).jobs++;
		lp->fp = psb;
		psb->sp_time = ONE_MINUTE + 1;
		psb->sp_sent = FALSE;
		send_job (c, t, l);
		splx (oip);
		return (SDI_RET_OK);
		break;

	case	ISCB_TYPE:
		c = SDI_CONTROL(psb->sbp->SCB.sc_dev.sa_fill);
		t = SDI_TARGET(psb->sbp->SCB.sc_dev.sa_fill);
		l = psb->sbp->SCB.sc_dev.sa_lun;
		c = wd_ctoi[c];
		lp = &LU(c, t, l);
#ifdef DEBUG
		DPR (8,2)(" wd_icmd ISCB c%d t%d l%d ",c,t,l);
#endif
		/* respond to SS_INQUIR */
		if((inq_cdb = (struct scs *)psb->sbp->SCB.sc_cmdpt)->ss_op == SS_INQUIR)	{
			struct	ident	*idp, *inq_dp;

			inq_dp = (struct ident *)psb->sbp->SCB.sc_datapt;

			if(t == SC(c).ha_id) {
				if(l != 0)	{
					psb->sbp->SCB.sc_comp_code = SDI_SCBERR;
					splx(oip);
					return(SDI_RET_OK);
				}

				inq_dp->id_type = ID_PROCESOR;
				(void)strncpy(inq_dp->id_vendor, wd_idata[c].name, 24);
				inq_dp->id_vendor[23] = NULL;
				psb->sbp->SCB.sc_comp_code = SDI_ASW;
				splx (oip);
				return (SDI_RET_OK);
			}

			if(!(idp = lp->idp))	{
				idp = &nodev_inq_data;
			}

			bcopy((caddr_t)idp, (caddr_t)inq_dp, sizeof(struct ident));
			psb->sbp->SCB.sc_comp_code = SDI_ASW;
			splx(oip);
			return(SDI_RET_OK);
		}

		/* if device not found during ha_edt don't send command */
		if (TC(c,t).lu_cnt == 0) {
			psb->sbp->SCB.sc_comp_code = SDI_NOTEQ;
			splx (oip);
			return (SDI_RET_ERR);
		}

		/* WD HA does not support > 128K jobs */ 
		if (psb->sbp->SCB.sc_datasz > 131072) 
		{
			psb->sbp->SCB.sc_comp_code = SDI_SCBERR;
			splx (oip);
			return (SDI_RET_ERR);
		}
		if (((SC(c).state & C_OPERATIONAL) == NULL) && 
					(!lp->jobs.t.suspended))
		{
			psb->sbp->SCB.sc_comp_code = SDI_OOS;
			timeout (wd_callback, (caddr_t)psb, ONE_MSEC);
			splx (oip);
			return (SDI_RET_OK);
		}
		if ((lp->jobs.t.immediate) && 
			(!(lp->jobs.t.suspended & PT_ACTIVE))) 
		{
			psb->sbp->SCB.sc_comp_code = SDI_ONEIC;
			timeout (wd_callback, (caddr_t)psb, ONE_MSEC);
			splx (oip);
			return (SDI_RET_OK);
		}
		if (psb->sbp->SCB.sc_time < 0)
		{
			psb->sbp->SCB.sc_comp_code = SDI_SCBERR;
			timeout (wd_callback, (caddr_t)psb, ONE_MSEC);
			splx (oip);
			return (SDI_RET_OK);
		}

		/* convert milliseconds to microseconds; but only if > 0 */
		if (psb->sbp->SCB.sc_time)
		{
                 	if (psb->sbp->SCB.sc_time < 1000 * 1000)
                                psb->sbp->SCB.sc_time = drv_usectohz(psb->sbp->SCB.sc_time * 1000);
               		else
                            	psb->sbp->SCB.sc_time = drv_usectohz(psb->sbp->SCB.sc_time) * 1000;
		}

		psb->sbp->SCB.sc_comp_code = SDI_PROGRES;
		lp->jobs.t.immediate++;
		SC(c).jobs++;
		add_ijob (psb, c, t, l, FALSE);
		send_job (c, t, l);
		splx (oip);
		if ( SC(c).state & C_INIT_TIME )
		{
			if ((wd_wait_for_comp(c,THREE_MILL_U_SEC)) == FAIL)
			{
#ifdef DEBUG
				cmn_err(CE_WARN,"%s: HA %d, TC %d, LU %d job timeout.", wd_idata[c].name,wd_itoc[c],t,l);
#endif
				SC(c).jobs--;
				SC(c).tc[t].lu[l].jobs.t.normal--;
				psb->sbp->SCB.sc_comp_code = SDI_TIME;

				sub_job(psb, c, t, l);
			}
		}
		return (SDI_RET_OK);
		break;
	default:
#ifdef DEBUG
		cmn_err (CE_WARN,"%s: wd_icmd called with an illegal type of %u.\n", wd_idata[c].name, psb->sbp->sb_type);
#endif
		splx (oip);
		return (SDI_RET_ERR);
		break;
	}
}

/*============================================================================
** Function name: wd_xlat()
** Description:
**	This function performs the virtual to physical translation on the
**	SCB data pointer. 
*/

void
wd_xlat (sp, flag, procp)
register struct sb2	*sp;
register int		flag;
struct   proc		*procp;
{

#ifdef DEBUG
	DPR (8,2)("wd_xlat ");
#endif

	if (sp->sp_vip) 
	{
		wd_dmafreelist ((DMA_LIST *)sp->sp_vip);
		sp->sp_vip = NULL;
		sp->sp_isz = NULL;
	}
	if (sp->sbp->SCB.sc_link != NULL) 
	{
	     cmn_err (CE_NOTE,"!WD7000: Linked commands NOT available.\n");
	     sp->sbp->SCB.sc_link = NULL;
	}

	sp->sp_read = (flag & B_READ) ? 1 : 0;	/* set job direction */

	if (sp->sbp->SCB.sc_datapt != NULL)
	{
		/*
		 * Do scatter/gather if data spans multiple pages
		 */
		sp->sp_pdb = vtop (sp->sbp->SCB.sc_datapt, procp);
		if (sp->sbp->SCB.sc_datasz > Bytes_Til_Page_Boundary (sp->sp_pdb))
			wd_dmalist (sp, procp);
	}
	else
		sp->sbp->SCB.sc_datasz = NULL;
	return;
}

/*============================================================================
** Function name: wd_getblk()
** Description:
**	This function returns a SCB to the caller. The function will 
**	sleep if there are no SCSI blocks available.
**	
*/

struct hbadata *
wd_getblk ()
{
	register int		oip;
	register union sb_item	*psb;

	oip = spl5 ();
	while (wd_sblist == NULL)
		sleep ((caddr_t)&wd_sblist, PZERO);

	psb = wd_sblist;
	wd_sblist = wd_sblist->next;
	psb->next = NULL;
	splx (oip);

	return ((struct hbadata *)psb);
}


/*============================================================================
** Function name: wd_freeblk()
** Description:
**	This function frees up a previously allocated SCB structure.
**	The SCB is returned to the pool and if there is a scatter/gather
**	list associated with the SCB it is freed via wd_dmafreelist().
**	A nonzero return indicates an error in the pointer or type field.
*/

long
wd_freeblk (psb)
register union sb_item	*psb;
{
	int	oip;

	if (psb == NULL)
	{
#ifdef DEBUG
		cmn_err (CE_WARN,"WD7000: wd_freeblk called with an illegal pointer.\n");
#endif
		return (SDI_RET_ERR);
	}
	if ((psb < (union sb_item *) wd_sbstart) || 
			(psb > (union sb_item *) wd_sbend))
	{
#ifdef DEBUG
		cmn_err (CE_WARN,"WD7000: wd_freeblk SCB not allocated by WD7000\n");
#endif
		return (SDI_RET_ERR);
	}
	if (psb->sb2.sp_vip)
	{
		wd_dmafreelist ((DMA_LIST *)psb->sb2.sp_vip);
		psb->sb2.sp_vip = NULL;
		psb->sb2.sp_isz = NULL;
	}
	oip = spl5 ();
	psb->next = wd_sblist;
	wd_sblist = psb;

	if (psb->next == NULL)
		wakeup ((caddr_t)&wd_sblist);

	splx (oip);
	return (SDI_RET_OK);
}

/*============================================================================
** Function name: wd_getinfo()
** Description:
**	This function returns the name and iotype of the given device.
**	It decodes the major number and produces a string indicating the
**	name of the addressed controller. The name is copied into a string
**	pointed to by the first field of the getinfo structure.
**	The returned string may be as long as 48 bytes.
**	This function is only for use by the target driver when it wants 
**	to print a message or get the iotype info.
*/

void
wd_getinfo (addr, getinfo)
struct scsi_ad	*addr;
struct hbagetinfo *getinfo;
{
	register int	c = SDI_CONTROL(addr->sa_fill);
	register int	t = SDI_TARGET(addr->sa_fill);
	char	*name;
	register int	i = 0;

	c = wd_ctoi[c];
#ifdef DEBUG
	DPR (8,2)(" wd_getinfo c%d t%d maj%d ",c,t,addr->sa_major);
#endif
	name = getinfo->name;
	*name++ = 'H';
	*name++ = 'A';
	*name++ = ' ';

	if (c > 9) {
		*name++ = '1';
		*name++ = '0' + (c - 10);
	}
	else {
		*name++ = '0' + c;
	}
	*name++ = ' ';
	*name++ = 'T';
	*name++ = 'C';
	*name++ = ' ';
	*name++ = '0' + t;
	*name = NULL;

	getinfo->iotype = F_DMA | F_SCGTH;

	return;
}

/*============================================================================
** Function name: wd_fltinit()
** Description:
**	This function is called by the target drivers to enable their 
**	event handler for a given TC LU conbination. The event handler 
**	is called when a SCSI Bus reset occurs or when the LU is closed 
**	after a pass-thru operation.
*/

#ifdef DEBUG
#else
void
wd_fltinit(sa, func, param)
struct scsi_ad *sa;
void (*func)();
long param;
{
	int	c = SDI_CONTROL(sa->sa_fill);
	int	t = SDI_TARGET(sa->sa_fill);
	int	l = sa->sa_lun;
	register struct	lu_ctrl	*lup;

	c = wd_ctoi[c];
	lup = &LU(c, t, l);
        lup->dv_func = func;
        lup->dv_param = param;

	return;
}
#endif

/*============================================================================
** Function name: wd_alloc()
** Description:
**	This function allows the host adapter driver to allocate
**	memory which may be more efficient than main store.
**	This memory may NOT be in the address space of the CPU and
**	should only be accessed by routines provided by the host
**	adapter driver. A NULL return indicates that storage could NOT
**	be allocated.
**	This function is not supported by this implementation of SDI.
*/

char *
wd_alloc (size, addr)
int		size;
struct scsi_ad	*addr;
{
	return (NULL);
}

/*============================================================================
** Function name: wd_free()
** Description:
**	Free up the previously allocated "special" memory.
**	This function should free up space allocated by wd_alloc
**	only, and returns nonzero on error.
*/

int
wd_free (pt)
char	*pt;
{
	return (TRUE);
}

/*============================================================================
** Function name: wd_copy()
** Description:
**	Copies data from the CPU's memory to the "special"
**	memory allocated by the wd_alloc routine.
**	If the IMMED bit is set, the function will not return until
**	the data is copied. If the IMMED bit is not set, the function
**	may return before the copy is completed; however, the host
**	adapter driver must gaurantee that the copy will be complete
**	before the data is used by a SCSI command.
**	If the copy fails a value of -1 is returned.
**	If a delayed copy fails, any SCSI job accessing that area will
**	be returned with the MEMERR completion status.
**	This function is not supported by this implementation of SDI.
*/

long
wd_copy (cmd, pt1, pt2, size)
long	cmd;
char	*pt1;
char	*pt2;
long	size;
{
	return (-1);
}

/*============================================================================
** Function name: ll_send_cmd()
** Description:
**	This function starts a pass-thru job. The user SCB and CDB is read
**	into a local SCB and CDB. Then ll_send_scb() is called to do the
**	work. Upon return the local SCB is copied back to the user SCB.
*/

int
ll_send_cmd (dev, pa)
dev_t		dev;
struct sb	*pa;
{
	struct sb		*psb;
	struct wd_cdb_buf	*p;
	caddr_t			user_cdb;
	int			c = SC_CONTROL(getminor (dev));
	int			t = SC_TARGET(getminor (dev));
	char			*save_priv;
	int 			uerror = 0;

	if (t == SC(c).ha_id) 
	{
		cmn_err (CE_WARN,"%s: Pass-thru attempted with an invalid id %u, HA %u.\n", wd_idata[c].name, SC(c).ha_id, wd_itoc[c]);
		return (ENXIO);
	}
	psb = sdi_getblk();
	save_priv = psb->SCB.sc_priv;
	if (copyin ((caddr_t)pa, (caddr_t)psb, sizeof (struct sb)) != SUCCESS)
	{
		psb->SCB.sc_priv = save_priv;
		sdi_freeblk (psb);
		return(EFAULT);
	}
	psb->SCB.sc_priv = save_priv;
	if ((psb->sb_type == ISCB_TYPE) && (psb->SCB.sc_cmdpt != NULL) &&
		(psb->SCB.sc_cmdsz > NULL) && 
			(psb->SCB.sc_cmdsz <= MAX_CMDSZ))
	{
		user_cdb = psb->SCB.sc_cmdpt;
		p = get_cdb ();
		if (copyin (user_cdb, p->cdb, 
				psb->SCB.sc_cmdsz) == SUCCESS)
		{
			psb->SCB.sc_cmdpt = (caddr_t) p;
			uerror = ll_send_scb (dev, psb);

			psb->SCB.sc_cmdpt = user_cdb;
			if (copyout ((caddr_t)psb, (caddr_t)pa, sizeof(struct sb)) != SUCCESS)
				uerror = EFAULT;
		}
		else
			uerror = EFAULT;

		free_cdb (p);
	}
	else 
		uerror = EINVAL;

	sdi_freeblk (psb);
	return(uerror);
}

/*============================================================================
** Function name: ll_send_scb()
** Description:
**	This function given the SCB starts the I/O request for the 
**	pass-thru job. If the job involves a data transfer then the 
**	request if done thru physio() so that the user data area is 
**	locked in memory. If the job doesn't involve any data transfer 
**	then wd_pass_thru() is called directly.
*/

int
ll_send_scb (dev, psb)
dev_t		dev;
struct sb	*psb;
{
	long			user_wd;
	caddr_t			user_datapt;
	struct wd_cdb_buf	*ha_cdb;
	buf_t			*bp;
	int			uerror = 0;
	struct uio		hauio;
	struct iovec		havec;
	
	
	ha_cdb = (struct wd_cdb_buf *) psb->SCB.sc_cmdpt;
	bp = &(ha_cdb->bp);
	psb->SCB.sc_cmdpt = (caddr_t) ha_cdb->cdb;

	if (psb->SCB.sc_mode & SCB_READ)
		bp->b_flags |= B_READ;
	else
		bp->b_flags |= B_WRITE;

	user_wd = psb->SCB.sc_wd;		/* save user word     */
	user_datapt = psb->SCB.sc_datapt;	/* save user data ptr */
	psb->SCB.sc_wd = (long)bp;
	psb->SCB.sc_int = wd_int;

	bp->b_resid = (long)psb;

	havec.iov_base = (psb->SCB.sc_datapt);	
	havec.iov_len = (psb->SCB.sc_datasz);	
	hauio.uio_iov = &havec;
	hauio.uio_iovcnt = 1;
	hauio.uio_offset = 0;
	hauio.uio_segflg = UIO_USERSPACE;
	hauio.uio_fmode = 0;
	hauio.uio_resid = (psb->SCB.sc_datasz);

	if (psb->SCB.sc_datasz == NULL)
	{ 
		bp->b_flags |= B_BUSY;
		bp->b_un.b_addr = psb->SCB.sc_datapt;
		bp->b_bcount = psb->SCB.sc_datasz;
		bp->b_blkno = NULL;
		bp->b_edev = dev;
				
		wd_pass_thru (bp);	/* Fake physio call */
		biowait (bp);
	}
	else 
	{
		uerror = physiock((void(*)())wd_pass_thru, bp, dev, bp->b_flags, 
							4194303, &hauio);
	}
	/* restore user SCB modified fields */

	psb->SCB.sc_wd = user_wd;
	psb->SCB.sc_datapt = user_datapt;
	psb->SCB.sc_int = NULL;

	return(uerror);
}

/*============================================================================
** Function name: wd_pass_thru()
** Description:
**	This function checks if a pass-tru job can be performed. If so
**	The job is sent to the Ha board.
*/
int
wd_pass_thru (bp)
buf_t	*bp;
{
	struct sb	*psb;
	int		oip;
	int		c = SC_CONTROL(getminor (bp->b_edev));
	int		t = SC_TARGET(getminor (bp->b_edev));
	int		l = SC_LUN(getminor (bp->b_edev));
	struct proc	*procp;

	psb = (struct sb *)bp->b_resid;
	psb->SCB.sc_dev.sa_fill = ((wd_itoc[c] << 3) | t);
	if (psb->SCB.sc_wd != (long)bp)
	{
	     cmn_err (CE_WARN,"!%s: Corrupted address from physio.\n", wd_idata[c].name);
	     biodone (bp);
	     return(EIO);
	}
	psb->SCB.sc_datapt = (caddr_t) paddr(bp);
	drv_getparm (UPROCP, (ulong*)&procp);
	sdi_translate (psb, bp->b_flags, procp);
	oip = spl5 ();
	if ((SC(c).state & C_OPERATIONAL) == NULL) 
	{
		splx (oip);
		psb->SCB.sc_comp_code = SDI_OOS;
		biodone (bp);
		return(EIO);
	}
	/* allow only one pass-tru job at a time */
	if ((LU(c, t, l).jobs.t.immediate) && 
		(LU(c, t, l).jobs.t.suspended & PT_ACTIVE))
	{
		splx (oip);
		psb->SCB.sc_comp_code = SDI_ONEIC;
		biodone (bp);
		return(EBUSY);
	}
	LU(c, t, l).jobs.t.immediate++;
	SC(c).jobs++;
	add_ijob ((struct sb2 *)((struct xsb *)psb)->hbadata_p, c, t, l, TRUE);
	send_job (c, t, l);
	splx (oip);

	return(0);
}
/*============================================================================
** Function name: wd_int()
** Description:
**	This is the interrupt handler for pass-thru jobs. It basically
**	just wakes up the ll_send_scb() function.
*/

void
wd_int (psb)
struct sb	*psb;
{
	buf_t	*bp = (buf_t *)psb->SCB.sc_wd;

	if (bp->b_resid != (long)psb)
	{
#ifdef DEBUG
		cmn_err (CE_WARN,"WD7000: Corrupted address returned during pass through operation.\n");
#endif
		return;
	}
	if (psb->SCB.sc_comp_code == SDI_PROGRES)
	{
#ifdef DEBUG
		cmn_err (CE_WARN,"WD7000: Bad completion code returned during pass through operation.\n");
#endif
		psb->SCB.sc_comp_code = SDI_ASW;
	}
	bp->b_resid = NULL;
	biodone (bp);
}

/*============================================================================
** Function name: get_cbd()
** Description:
**	This routine returns the next available cdb from the free cdb
**	list. If none are available, the process is put to sleep
**	until one becomes available. The free list is simply a
**	NULL terminated singly linked list of cdb's.
**
**			+----+		+----+		+----+
**    wd_cdb_list ----->|next|--------->|next|--------->|next|-|-|-
**			+----+		+----+		+----+
**
*/

static struct wd_cdb_buf *
get_cdb ()
{
	register union cdb_item	*cdb;
	register int	oip;
	
	oip = spl5 ();
	while (wd_cdb_list == NULL)
		sleep ((caddr_t)&wd_cdb_list, PZERO);

	cdb = wd_cdb_list;
	wd_cdb_list = cdb->next;
	cdb->next = NULL;

	splx (oip);
	return ((struct wd_cdb_buf *)cdb);
}

/*============================================================================
** Function name: free_cdb()
** Description:
**	This routine frees up the specified cdb and places it back on
**	the free list of cdb's. If the free list was empty, a wakeup
**	call is made to allow any processes that were waiting for a
**	cdb to fight for the one that was just freed.
*/

static void
free_cdb (cdb)
register union cdb_item	*cdb;
{
	register int	oip;
	
	oip = spl5 ();
	cdb->next = wd_cdb_list;
	wd_cdb_list = cdb;

	if (cdb->next == NULL)
		wakeup ((caddr_t)&wd_cdb_list);
	splx (oip);

	return;
}

/*============================================================================
** Function name: send_job()
** Description:
**	This function will attempt to send the first job on the queue
**	for the given TC LU. All jobs are not sent if the Q is busy.
**	Normal jobs are sent if:
**		1. No reset timing is in progress
**		2. The LU is not pump_suspended OR pass-tru suspended
**		   OR suspended because of an error.
**	Immediate jobs are sent if:
**		1. We not doing the 5 second reset delay.
**		2. We are not pump suspended.
**		3. If open for pass-tru then only pass-thru jobs can be 
**		   sent. They are indicated by the PT_ACTIVE flag being set.
*/

static int
send_job (c, t, l)
int	c;	/* HA Controller 	*/
int	t;	/* target controller	*/
int	l;	/* logical unit		*/
{
	register struct lu_ctrl	*lup;	/* LU pointer	*/
	register struct sb2	*sp;	/* sb pointer 	*/
	int			cmd;	/* SFB type cmd */
	int			retcode;/* return code	*/

#ifdef DEBUG
	DPR (8,4)("send_job c%d t%d l%d ",c,t,l);
#endif
	ha_ready(c);
	retcode = FALSE;

	if (wd_getq (c) >= NUM_OGMB)
	{
#ifdef DEBUG
		DPR (8,0)("send_job: NO RQs ");
#endif
		return (retcode);
	}
	lup = &LU(c, t, l);

	/* check if there are free SRBs and there are jobs to be sent */

	if ((SC(c).srb_list != NULL) && (((TC(c, t).waiting_jobs) && 
			(lup->jp) && (!lup->jp->sp_sent)) || (lup->fp)))
	{
		if (lup->fp)
			sp = lup->fp;
		else
			sp = lup->jp;
#ifdef DEBUG
		if (sp->sp_sent)
		{
  	  		DPR (8,0)("TRIED to send a job twice!!!! sent=0x%x\n",
				sp->sp_sent);
	  		return (retcode);
		}
#endif
		if (sp->sbp->sb_type == SCB_TYPE)
		{
			if ((!(SC(c).state & C_RS_BLK)) &&
					!lup->jobs.t.suspended)
			{
				SC(c).waiting_jobs--;
				TC(c, t).waiting_jobs--;
				retcode = TRUE;
				enqueue (c, SCSI_CTL, SUBDEV(t, l), sp->sp_pdb, 
						sp->sbp->SCB.sc_datasz, sp);
			}
		}
		else	/* its an immediate job	*/
		{
			if ((!(SC(c).state & C_RS_START)) &&
				(!(lup->jobs.t.suspended & PUMP_SUSPEND)))
			{
				if ((!(lup->jobs.t.suspended & PT_SUSPEND)) ||
				    ((lup->jobs.t.suspended & PT_SUSPEND) &&
				     (lup->jobs.t.suspended & PT_ACTIVE)))
				{
					if (sp->sbp->sb_type == ISCB_TYPE)
					{
				  
						SC(c).waiting_jobs--;
						TC(c, t).waiting_jobs--;
					   	enqueue (c, SCSI_CTL, 
							SUBDEV(t, l),
						    	sp->sp_pdb, 
						    	sp->sbp->SCB.sc_datasz, 
							sp);
					}
					else
					{
						if (sp->sbp->SFB.sf_func == TCRST)
							cmd = TCRST;
						else
							cmd = ABORT;
						enqueue (c, cmd, SUBDEV(t, l),
						    	NULL, NULL, NULL);
					}
					retcode = TRUE;
				}
			}
		}
	}
	return (retcode);
}

/*============================================================================
** Function name: enqueue()
** Description:
**	This function fills in the request queue entry and interrupts the
**	HA board. Non SCB jobs generate express ints. while SCB jobs
**	generate normal interrupts.
*/

static void
enqueue (c, cmd, dev, addr, length, sp)
int			c;	/* Controller */
char			cmd;	/* Command    */
char			dev;	/* Subdevice  */
long			addr;	/* Address    */
int			length;	/* Length     */
register struct sb2	*sp;	/* sb pointer */
{

	SCSI_RB		*srb;	/* SCSI cmd request block ptr 	*/
	int		rqn;	/* index of request Q to use	*/
	caddr_t		p;	/* pointer to CDB in the SCB	*/
	int		i;

#ifdef DEBUG
	DPR (8,4)("enqueue c%d dev=%x addr=%x len=%x sp=%x\n", c, dev,
			addr, length, sp);
#endif

	SC(c).active_jobs++;
	sp->sp_sent = TRUE;

	/* get a free SRB from list */

	srb = (SCSI_RB *) SC(c).srb_list;
	SC(c).srb_list = SC(c).srb_list->next;
	((union srb_buf *)srb)->next = NULL;

	/* fill in the SCSI request block */

	srb->dev = dev;
	srb->ssb = NULL;
	srb->hasb = NULL;
	if (sp->sp_isz)	/* raw mode, i.e, scatter/gather */
	{
		srb->opcode = SCSI_DMA_CMD;
		srb->xfer_size[0] = msbyte(sp->sp_isz);
		srb->xfer_size[1] = mdbyte(sp->sp_isz);
		srb->xfer_size[2] = lsbyte(sp->sp_isz);
	}
	else		/* block mode, i.e, NO scatter/gather */
	{
		srb->opcode = SCSI_CMD;
		srb->xfer_size[0] = msbyte(length);
		srb->xfer_size[1] = mdbyte(length);
		srb->xfer_size[2] = lsbyte(length);
	}
	srb->pdp[0] = msbyte(addr);
	srb->pdp[1] = mdbyte(addr);
	srb->pdp[2] = lsbyte(addr);
	srb->link[0] = NULL;
	srb->link[1] = NULL;
	srb->link[2] = NULL;
	srb->rw = (sp->sp_read) ? 1 : 0;	/* set job direction 	*/

	/* copy SCB cdb to SCSI request block */

	p = sp->sbp->SCB.sc_cmdpt;
	for (i = 0; i < sp->sbp->SCB.sc_cmdsz; i++)
		srb->cdb[i] = *p++;

	/* set up the request Q entry */

	rqn = wd_getq (c);
	SC(c).rq[rqn].pdp = sdi_swap24(vtop((caddr_t)srb, NULL));

	SC(c).rq[rqn].status = RQ_BUSY;

	cmd = (RD_MB | (rqn & 0x3F));	/* send cmd to HA board */
	ha_cmd(c, cmd);
	return;
}

/*============================================================================
** Function name: add_job()
** Description:
**	This function adds normal jobs to the LU queue. The job is
**	always added to the end of the list.
**
**  +-------------------+  forw  +------------------+
**  | LU(c, t, l).jp    | -----> | next job to send |
**  | first job to send | <----- |                  |
**  +-------------------+  back  +------------------+
*/

static void
add_job (sp, c, t, l)
register struct sb2	*sp;
int			c;
int			t;
int			l;
{
	register struct lu_ctrl	*lp = &LU(c, t, l);

#ifdef DEBUG
	DPR (8,4)("add_job c%d t%d l%d ",c,t,l);
#endif
	if (lp->jp == NULL)
	{
		lp->jp = sp;
		sp->sp_forw = sp;
		sp->sp_back = sp;
		SC(c).waiting_jobs++;
		TC(c, t).waiting_jobs++;
	}
	else
	{
		sp->sp_forw = lp->jp;
		sp->sp_back = lp->jp->sp_back;
		lp->jp->sp_back->sp_forw = sp;
		lp->jp->sp_back = sp;
	}
	if (sp->sbp->SCB.sc_time)
		sp->sp_time = ONE_MINUTE + sp->sbp->SCB.sc_time;
	else
		sp->sp_time = ONE_WEEK;
	return;
}


/*============================================================================
** Function name: add_ijob()
** Description:
** 	This function adds an immediate job to the LU queue. Immediate jobs
**	are always added to the head of the list except if a jobs has already
**	been sent or there is a pass-tru job on the Q in which case the job
**	is added behind that first job.
*/

static void
add_ijob (sp, c, t, l, ptru)
register struct sb2	*sp;
int			c;
int			t;
int			l;
int			ptru; 	/* TRUE if pass-thru job */
{
	register struct lu_ctrl	*lp = &LU(c, t, l);

	if (lp->jp == NULL)
	{	
		lp->jp = sp;
		sp->sp_forw = sp;
		sp->sp_back = sp;
		SC(c).waiting_jobs++;
		TC(c, t).waiting_jobs++;
	}
	else if ((lp->jp->sp_sent) || (lp->jobs.t.suspended & PT_ACTIVE))
	{
		sp->sp_forw = lp->jp->sp_forw;
		lp->jp->sp_forw = sp;
		sp->sp_back = lp->jp;
		sp->sp_forw->sp_back = sp;
	}
	else 
	{
		sp->sp_forw = lp->jp;
		sp->sp_back = lp->jp->sp_back;
		lp->jp->sp_back->sp_forw = sp;
		lp->jp->sp_back = sp;
		lp->jp = sp;
	}
	if (sp->sbp->SCB.sc_time)
		sp->sp_time = ONE_MINUTE + sp->sbp->SCB.sc_time;
	else
		sp->sp_time = ONE_WEEK;
	if (ptru)
		lp->jobs.t.suspended |= PT_ACTIVE;

	return;
}


/*============================================================================
** Function name: sub_job()
** Description:
**	This function removes the first job from the LU Q. The first
**	job on the Q is passed into the function.
*/

static void
sub_job (sp, c, t, l)
register struct sb2	*sp;
int			c;
int			t;
int			l;
{
	register struct lu_ctrl	*lp = &LU(c, t, l);

#ifdef DEBUG
	if (!sp->sp_sent)
  		DPR (8,0)("Subtracting a job that has NOT been sent.\n");
#endif
	if (sp->sp_forw == sp)
		lp->jp = NULL;
	else 
	{
		lp->jp = sp->sp_forw;
		sp->sp_back->sp_forw = sp->sp_forw;
		sp->sp_forw->sp_back = sp->sp_back;
		SC(c).waiting_jobs++;
		TC(c, t).waiting_jobs++;
	}
	sp->sp_forw = NULL;
	sp->sp_back = NULL;
	sp->sp_time = NULL;
	sp->sp_sent = FALSE;

	return;
}

/*============================================================================
** Function name: next_job()
** Description:
**	This function sends the next available job to the ha board.
**	The next available job is the first job found in the Q which
**	can be sent. The routine stops when a job is sent or when
**	all the queues for this board have been searched. The search
**	allways starts it the Q pointed by next_tc next_lu vars.
*/

static void
next_job (c)
int	c;
{
	register struct wd_ctrl *cp = &SC(c);
	register struct tc_ctrl *tp;
	register int		t;
	register int		l;
	int			last_tc, done, tc_cnt;

	if ((cp->srb_list == NULL) || (cp->state & C_RS_START) || 
					(cp->waiting_jobs == NULL))
	{
		return;
	}
	last_tc = MAX_TCS;
	tc_cnt = 0; 
	done = FALSE;
	while (done == FALSE)
	{
		for (t = cp->next_tc, tp = &cp->tc[t]; 
			((t < last_tc) && (done == FALSE)); t++, tp++, tc_cnt++)
		{
			if (tp->waiting_jobs > 0)
			{
				for (l = cp->next_lu;
					((l < MAX_LUS) && (done == FALSE));l++) 
				{
					if (send_job (c, t, l))
					{
					    done = TRUE;
					    break;
					}
				}
				if (done == TRUE)
				{
					cp->next_lu = (l + 1) % MAX_LUS;
					if (cp->next_lu == 0)
						cp->next_tc = (t + 1) % MAX_TCS;
					else
						cp->next_tc = t;
				}
				else
					cp->next_lu = NULL;
			}
			else
				cp->next_lu = NULL;
		}
		if (done == FALSE)
		{
			if (tc_cnt >= MAX_TCS)
				done = TRUE;
			else
			{
				last_tc = (cp->next_tc + 1) % (unsigned)MAX_TCS;
				cp->next_tc = 0;
			}
		}
	}
	return;
}

/*============================================================================
** Function name: stop_jobs()
** Description:
**	Used to stop all jobs in order to allow the board to be pumped.
**	This function is also used in other cases to stop jobs.
*/

static void
stop_jobs (c)
register int	c;
{
	register int	t, l, oip;

	oip = spl5 ();

	for (t = 0; t < MAX_TCS; t++) 
	{
		for (l = 0; l < MAX_LUS; l++)
			LU(c, t, l).jobs.t.suspended |= PUMP_SUSPEND;
	}
	splx (oip);
	return;
}

/*============================================================================
** Function name: start_jobs()
** Description:
**	Used to restart jobs after they have been stopped.
*/

static void
start_jobs (c)
register int	c;
{
	register int	t;
	register int	l;
	register int	oip;
	register struct lu_ctrl	*lp;

	oip = spl5 ();

	for (t = 0; t < MAX_TCS; t++)
	{
		for (l = 0; l < MAX_LUS; l++)
		{
			lp = &LU(c, t, l);
			lp->jobs.t.suspended &= ~PUMP_SUSPEND;
			send_job (c, t, l);
		}
	}
	splx (oip);
}

/*---------------------------------------------------------------------------*/
/* WD7000 Host Adapter Driver Utilities
/*---------------------------------------------------------------------------*/

/*============================================================================
** Function name: wdbd_init()
** Description:
**	This function resets the HA board sets up the dma channel and
**	initializes the commucation queues.
*/

wdbd_init(c)
register int	c;	/* HA controller number */
{
	union init_blk	iblk;	/* init cmd block */
	int		i;
	unsigned int	padr;
	unsigned char 	status;
	int		retcode = FAIL;	/* assume fail */
	char	dmac = wd_idata[c].dmachan1 - 4;
	
	outb(CONTROL_REG(c), SCSI_PORT_RESET ); /* reset scsi bus */
	for (i = 0; i < 50; i++);
	outb(CONTROL_REG(c), 0x00);

	outb(CONTROL_REG(c),  ASC_RESET); /* reset ha board */
	outb(CONTROL_REG(c), 0x00);
	{
		int	time = THREE_MILL_U_SEC; /* 3 second to wait max */
		while (time > 0)
		{
			if (!(RD_HA_STATUS(c) & INT_FLAG))
			{
				drv_usecwait(1000);	/* wait 1 ms */
				time -= 1000;
			}
			else 
			{
			outb(INT_ACK(c),0); /* ack the int from reset */
			break;
			}
		}
	}
	drv_usecwait(THREE_MILL_U_SEC);	/* wait 3 seconds */

	if(inb(HA_STATUS(c)) == 0xFF) {
		cmn_err(CE_CONT,"!%s %d NOT found at address 0x%X.\n", wd_idata[c].name,c,wd_ad[c].ha_status);
		return(retcode);
	}

	if (!(RD_HA_STATUS(c) == CMD_READY))
	{
		cmn_err(CE_NOTE,"!%s %d not ready to accept commands\n", wd_idata[c].name,c);
		return(retcode);
	}
	cmn_err(CE_NOTE,"%s found at address 0x%X.\n", wd_idata[c].name,wd_ad[c].ha_status);

#ifdef DEBUG
	DPR(8,0)("WD7000: HA %d RESET OK \n",wd_itoc[c]);
#endif

	/* setup the initialization command block */

	iblk.init.opcode = INIT;
	iblk.init.ha_id = HA_ID;
	iblk.init.on_time = 24;
	iblk.init.off_time = 24;
	iblk.init.resv = 0;
	padr = vtop((caddr_t)SC(c).rq,NULL);

	iblk.init.mbox_pdp[0] = msbyte(padr);
	iblk.init.mbox_pdp[1] = mdbyte(padr);
	iblk.init.mbox_pdp[2] = lsbyte(padr);
	iblk.init.num_ic_mb = NUM_ICMB;
	iblk.init.num_og_mb = NUM_OGMB;

	/* sent init cmd one byte at a time */

	for (i = 0; i < ICMD_SIZE; i++)
		ha_cmd(c, iblk.data[i]);
	
	ha_ready(c);
	drv_usecwait(ONE_MILL_U_SEC);	/* 1 second */
	status = inb(HA_STATUS(c));

	if (RD_HA_STATUS(c) & INIT_FLAG)	/* init flag must be set */
	{
#ifdef DEBUG
		DPR(8,0)("WD7000: HA %d INIT OK st=0x%x\n",wd_itoc[c],status);
#endif
		/* mark this HA operational */

		SC(c).state |= (C_OPERATIONAL | C_RAM);
		SC(c).max_jobs = MAX_RAM_JOBS;

		/* Set enable the ASC DMA Image */
		outb(CONTROL_REG(c), (INT_ENABLE | DMA_REQ_ENABLE)); 

		/* Disable the DMA Channel in the AT DMA chip */
		outb(DMA2WMR, (dmac | CASCADE));	

		/* Enable the 386 DMA by clearing DMA Channel Mask */
		outb(DMA2WSMR, dmac);	

		retcode = PASS;
	}
	else
	{
	     cmn_err(CE_WARN,"%s: HA %d Initialization failed status=0x%x.\n"
					, wd_idata[c].name,wd_itoc[c],status);
	}
	return (retcode);
}

/*============================================================================
** Function name: ha_edt()
** Description:
**	This function builds the equipped device table for the given
**	SCSI Bus. It sends inquiries to every TC. Then, for all TCs
**	which answered the inquiry the number of LUs is calculated by 
**	calling wd_cklu().
*/

static int
ha_edt (c, sleepflag)
register int	c;	/* HA controller number */
int	sleepflag;	/* can we sleep		*/
{
	struct scs	inq_cdb;
	int		t, lu, i, comp_code, config;
	char		*p;

#ifdef DEBUG
	DPR(8,3)("ha_edt: c=%d ", c);
#endif

	nodev_inq_data.id_type = ID_NODEV;

	config = FALSE;		/* set to true if bus has at least 1 tc */
	sc_bp = getrbuf(KM_NOSLEEP);
	sc_bp->b_flags &= ~B_BUSY;
	sc_bp->b_iodone = NULL;
	inq_cdb.ss_op = SS_INQUIR;	/* inquiry cdb		*/
	inq_cdb.ss_lun = NULL;		/* it first try lu 0	*/
	inq_cdb.ss_addr = NULL;
	inq_cdb.ss_addr1 = NULL;
	inq_cdb.ss_len = IDENT_SZ;
	inq_cdb.ss_cont = NULL;

	comp_code = wd_cmd (c, SC(c).ha_id, 0, &inq_cdb, SCS_SZ, 
			&inq_data, IDENT_SZ, B_READ);

	if ((comp_code == SDI_ASW) || (comp_code == SDI_CKSTAT))
	{
#ifdef DEBUG
		cmn_err(CE_WARN,"%s: Target set at Host Adapter SCSI ID. ", wd_idata[c].name);
		cmn_err(CE_CONT,"HA %d, Target match at ID %d \n",wd_itoc[c],SC(c).ha_id);
		cmn_err(CE_CONT,"WD7000 subsystem being turned off!!");
#endif
		freerbuf(sc_bp);
		return;
	}
	else if (comp_code == SDI_TIME)
	{
		cmn_err(CE_WARN,"%s: HA %d no devices found, turning off HA.", wd_idata[c].name,wd_itoc[c]);
		freerbuf(sc_bp);
		return;
	}	

	for (t = 0; t < MAX_TCS; t++)	/* determine LU equippage */
	{
		TC(c, t).lu_cnt = NULL;
		if (t != SC(c).ha_id)
		{
			for (lu = 0; lu < MAX_LUS; lu++)
				wd_cklu (c, t, lu, sleepflag);

			if (TC(c, t).lu_cnt) {
				SC(c).tc_cnt++;
			}
		}
	}

	freerbuf(sc_bp);
	return ;
}

/*============================================================================
** Function name: wd_cklu()
** Description:
**	This function determines if the given LU is equipped. First An Inquiry
**	cammand is send if it passes the LU is marked equipped. For disks
**	a test unit ready is also send since some vendors don't return the
**	correct inquiry data to indicate that the addressed LU is not
**	present.
*/

static int
wd_cklu (c, t, l, sleepflag)
int	c;		/* HA Controller 	*/
int	t;		/* target controller	*/
int	l;		/* logical unit		*/
int	sleepflag;	/* can we sleep		*/
{
	struct scs	inq_cdb;	/* inquiry cdb		*/
	struct scs	tur_cdb;	/* test unit ready cdb	*/
	struct ident	*idp;
	int		comp_code;	/* SDI completion code 	*/

#ifdef DEBUG
	DPR(8,3)("wd_cklu: c=%d t=%d l=%d\n", c, t, l);
#endif
	inq_cdb.ss_op = SS_INQUIR;	/* inquiry cdb		*/
	inq_cdb.ss_lun = l;
	inq_cdb.ss_addr = NULL;
	inq_cdb.ss_addr1 = NULL;
	inq_cdb.ss_len = IDENT_SZ;
	inq_cdb.ss_cont = NULL;

	comp_code = wd_cmd (c, t, l, &inq_cdb, SCS_SZ,
					&inq_data, IDENT_SZ, B_READ);
	if (comp_code == SDI_ASW)
	{
		if ((inq_data.id_type == ID_RANDOM) && (!(inq_data.id_rmb)))
		{
			tur_cdb.ss_op = SS_TEST; /* test unit ready cdb	*/
			tur_cdb.ss_lun = l;
			tur_cdb.ss_addr = NULL;
			tur_cdb.ss_addr1 = NULL;
			tur_cdb.ss_len = NULL;
			tur_cdb.ss_cont = NULL;

			comp_code = wd_cmd (c, t, l, &tur_cdb, SCS_SZ, 
						NULL, NULL, B_READ);

			/* send it again first one clears unit attention */

			comp_code = wd_cmd (c, t, l, &tur_cdb, SCS_SZ, 
						NULL, NULL, B_READ);
						
			if (comp_code == SDI_ASW) 
			{
				TC(c, t).lu_cnt++;
			}
			else	{
				LU(c, t, l).idp = &nodev_inq_data;
				return;
			}
		}
		else if (inq_data.id_type != ID_NODEV)
		{
			TC(c, t).lu_cnt++;
		}
		else	{
			LU(c, t, l).idp = &nodev_inq_data;
			return;
		}
	}
	else	{
		LU(c, t, l).idp = &nodev_inq_data;
		return;
	}

	if((idp = kmem_zalloc(sizeof(struct ident), sleepflag)) == NULL)	{
		cmn_err(CE_WARN,
			"!wd_cklu(%d, %d, %d): could not allocate memory for ident structure\n",
			c, t, l);

		LU(c, t, l).idp = &nodev_inq_data;
		return;
	}

	bcopy((caddr_t)&inq_data, (caddr_t)idp, sizeof(struct ident));
	LU(c, t, l).idp = idp;
}


/*============================================================================
** Function name: wd_cmd()
** Description:
**	This function builds and sends an SCB associated scsi command. 
*/

static int
wd_cmd (c, t, l, cdb_p, cdbsz, data_p, datasz, rw_flag)
int	c;		/* HA Controller 	*/
int	t;		/* target controller	*/
int	l;		/* logical unit		*/
caddr_t	cdb_p;		/* pointer to cdb 	*/
long	cdbsz;		/* size of cdb		*/
caddr_t	data_p;		/* command data area 	*/
long	datasz;		/* size of buffer	*/
int	rw_flag;	/* read write flag	*/
{
	struct sb2	*psb;
	buf_t		*bp;
	struct proc	*procp;
	int		retcode;

#ifdef DEBUG
	DPR(8,3)("wd_cmd: c=%d t=%d l=%d\n", c, t, l);
#endif

	bp = sc_bp;
	bp->b_flags |= B_BUSY;

	psb = (struct sb2 *) &wd_scb;
	psb->sbp->sb_type = SCB_TYPE;
	psb->sbp->SCB.sc_int = wd_cint;
	psb->sbp->SCB.sc_cmdpt = cdb_p;
	psb->sbp->SCB.sc_cmdsz = cdbsz;
	psb->sbp->SCB.sc_datapt = data_p;
	psb->sbp->SCB.sc_datasz = datasz;
	psb->sbp->SCB.sc_wd = (long) bp;
	psb->sbp->SCB.sc_time = (30 * ONE_SECOND);
	psb->sbp->SCB.sc_dev.sa_lun = l;
	psb->sbp->SCB.sc_dev.sa_fill = ((wd_itoc[c] << 3) | t);

	drv_getparm (UPROCP, (ulong*)&procp);
	wd_xlat (psb, rw_flag, procp);

	psb->sbp->SCB.sc_comp_code = SDI_PROGRES;
	SC(c).jobs++;
	SC(c).tc[t].lu[l].jobs.t.normal++;
	add_job (psb, c, t, l);
	send_job (c, t, l);

	if ( SC(c).state & C_INIT_TIME )
	{
		if ((wd_wait_for_comp(c,THREE_MILL_U_SEC)) == FAIL)
		{
#ifdef DEBUG
			cmn_err(CE_NOTE,"%s: HA %d, TC %d, LU %d job timeout.", wd_idata[c].name,wd_itoc[c],t,l);
#endif
			SC(c).jobs--;
			SC(c).tc[t].lu[l].jobs.t.normal--;
			psb->sbp->SCB.sc_comp_code = SDI_TIME;

			sub_job(psb, c, t, l);
		}
	}
	else
	{
		while (bp->b_flags & B_BUSY)
			sleep ((caddr_t)bp, PZERO);
	}
	retcode = psb->sbp->SCB.sc_comp_code;
	wd_scb.sb2.sbp->SCB.sc_comp_code = SDI_NOALLOC;

	return (retcode);
}

/*============================================================================
** Function Name: wd_wait_for_comp()
** Description:
**	This routine is used to wait for a completion from the host 
**	adapter. This routine poll the stataus register on the HA every
**	milli-second. After the interrupt is seen, the HA's interrupt 
**	service routine is manually called. This routine is used primarily
**	at init time before the sleeping is legal.
** NOTE:	
	This routine allow for no  concurrency and as such, should 
**	be used selectivly.
*/
wd_wait_for_comp(c,time)
int	c;		/* HA controller */
int	time;		/* microseconds to wait for completion */
{
	int	act;

	while (time > 0)
	{
		if ((RD_HA_STATUS(c) & INT_FLAG))
			{
				act = wd_idata[c].active;
				wd_idata[c].active = 1;
				wd_intr(wd_ad[c].ha_vect);
				wd_idata[c].active = act;
				return (PASS);
			}
		drv_usecwait(1000);	/* wait 1 milli-seconds */
		time -= 1000;
	}
	return(FAIL);	
}

/*============================================================================
** Function name: wd_cint()
** Description:
**	This is the interrupt handler for the wd_cmd() function.
*/

static void
wd_cint (psb)
struct sb	*psb;
{
	buf_t	*bp;

#ifdef DEBUG
	DPR(8,3)("wd_cint ");
	DPR(8,3)("wd_cint psb=%x ",psb);
#endif
	bp =  (buf_t *) psb->SCB.sc_wd;
	bp->b_flags &= ~B_BUSY;
	wakeup ((caddr_t)bp);

}


/*============================================================================
** Function name: ha_ready()
** Description:
**	This function checks to see if the CMD port of the HA board
**	is ready to take in another command.
*/

ha_ready(c)
int	c;	/* HA Controller 	*/
{
	int		i;
	unsigned char ha_status;

	for (i = 0; i < 3000; i++)
	{
		ha_status = inb(HA_STATUS(c));
		if ((ha_status & 0xF0) & CMD_READY)
			return (PASS);
	}
#ifdef DEBUG
	cmn_err(CE_WARN,"%s: HA %d port hung st = 0x%x ", wd_idata[c].name,wd_itoc[c],ha_status);
#endif
	return (FAIL);
}

/*============================================================================
** Function name: ha_cmd()
** Description:
**	This function writes a command to the HA command register.
*/

ha_cmd(c, cmd)
int		c;	/* HA Controller 	*/
unsigned char	cmd;
{
	int	i;

	for (i=0; i<3000;i++)
	{
#ifdef DEBUG
		DPR(8,5)("writing %x to command register for %xth time.\n", cmd, i);
#endif
		outb(CMD_REG(c), cmd);
		if (ha_ready(c) == FAIL)
			return;

		if (!(RD_HA_STATUS(c) & CMD_REJECT))
			return;
	}
#ifdef DEBUG
	DPR(8,0)("WD7000: HA %d CMD REJECTED ",wd_itoc[c]);
#endif
}

/*============================================================================
** Function name: wd_getq()
** Description:
**	This function returns the next available request queue entry.
*/

static int
wd_getq(c)
int	c;
{
	int i;

	for (i=0;i<NUM_OGMB;i++)
		if (SC(c).rq[i].status != RQ_BUSY)
			break;
	return (i);
}


char rd_buf[25];	/* returned data buffer */
int
rw_exp (c, opcode, pa)
int	c;
int	opcode;
char	*pa;	/* user data pointer */
{
	int 	rqn,i;
	int 	padr=NULL;
	unsigned char cmd;
	
	for (i=0;i<25;i++)
		rd_buf[i] = NULL;

	if (opcode == WR_EXP)
	{
		if (copyin (pa, rd_buf, 25) != SUCCESS)
		{
			return(EFAULT);
		}
	}
	rexp.opcode = (opcode == RD_EXP) ? RD_EXECUT_PARAMS : SET_EXECUT_PARAMS;

	rexp.bytes[0] = NULL;	/*reserved byte */
	rexp.bytes[1] = NULL;
	rexp.bytes[2] = NULL;
	rexp.bytes[3] = 25;	/* xfer size */

	padr = vtop(rd_buf, NULL);

	rexp.bytes[4] = msbyte(padr);	/* cmd data area to write to */
	rexp.bytes[5] = mdbyte(padr);
	rexp.bytes[6] = lsbyte(padr);
	rexp.bytes[7] = NULL;
	rexp.bytes[8] = NULL;
	rexp.bytes[9] = NULL;
	rexp.bytes[10] = NULL;
	rexp.bytes[11] = NULL;
	rexp.bytes[12] = NULL;
	rexp.bytes[13] = NULL;
	rexp.status = NULL;

	/* set up the request Q entry */

	rqn = wd_getq (c);
	SC(c).rq[rqn].pdp = sdi_swap24(vtop((caddr_t)&rexp, NULL));
	SC(c).rq[rqn].status = RQ_BUSY;
	cmd = (RD_MB | (rqn & 0x3F));	/* send cmd to HA board */
	ha_cmd(c, cmd);

	if (wd_wait_for_comp(c,TEN_MILL_U_SEC) == PASS)
	{
		if (copyout(rd_buf, pa, 25) != SUCCESS)
		{
			return(EFAULT);
		}
	}	
	else	/* timeout */
	{
		cmn_err (CE_NOTE,"!%s: Read Parameters command timed out.\n", wd_idata[c].name);
		return (EIO);
	}

	return(0) ;
}

static void
wd_dmainit ()
{
	register int i, oip;
	
#ifdef DEBUG
	DPR(8,1)("wd_dmainit:\n");
#endif
	oip = spl5 ();
	
	dma_free_list = dma_pool;
	for (i = 0; i < (NUM_DMA_LISTS - 1); ++i)
	{
		dma_pool[i].next = &dma_pool[i + 1];
	}
	dma_pool[i].next = NULL;	/* last element */

	splx (oip);
	
} /* wd_dmainit () */

/*============================================================================
** Function name: wd_dmalist()
** Description:
**	Build the physical address(es) for DMA to/from the data buffer.
**	If the data buffer is contiguous in physical memory, sp->s_addr
**	is already set for a regular SB.  If not, a scatter/gather list
**	is built, and the SB will point to that list instead.
*/

static void
wd_dmalist (sp, procp)
struct sb2	*sp;
struct proc	*procp;
{
	int		i, oip;
	DMA_LIST	*lp, tmp_list;
	register DMA_PAIR *pp;
	register long   count, fraglen, thispage;
	caddr_t		vaddr;
	paddr_t		addr, base;

#ifdef DEBUG
	DPR(8,5)("wd_dmalist:\n");
#endif
	vaddr = sp->sbp->SCB.sc_datapt;
	count = sp->sbp->SCB.sc_datasz;
	pp = &tmp_list.pair[0];

	/* First build a scatter/gather list of physical addresses and sizes */

	for (i = 0; (i < NUM_DMA_PAIR_PER_LIST) && count; ++i, ++pp)
	{
		base = vtop(vaddr, procp);	/* Physical address of segment */
		fraglen = 0;			/* Zero bytes so far */
		do {
			thispage = min(count, Bytes_Til_Page_Boundary (vaddr));
			fraglen += thispage;	/* This many more are contiguous */
			vaddr += thispage;	/* Bump virtual address */
			count -= thispage;	/* Recompute amount left */
			if (!count)
				break;		/* End of request */
			addr = vtop(vaddr, procp); /* Get next page's address */
		} while (base + fraglen == addr &&
			addr < wd_tot_mem - NBPP); /* (For the kludge below) */

		/* This is a kludge to handle a WD7000-ASC HA bug. They cannot DMA
		 * out of the very last block of system memory without accessing 
		 * past the end of memory(which would cause the system to crash!).
		 * If the kernel ever asks us to tell the HA to read out of the 
		 * last page of memory, we must move the data to a safer spot.
		 */
		if ((sp->sp_read == FALSE) && (base >= wd_tot_mem - NBPP))
		{
			bcopy((caddr_t) (vaddr-fraglen), (caddr_t) wd_tmp_buff, fraglen);
			base = (long) vtop(wd_tmp_buff, NULL);
		} 
		/* Now set up dma list element */
	
		pp->count[0] = msbyte(fraglen);
		pp->count[1] = mdbyte(fraglen);
		pp->count[2] = lsbyte(fraglen);
		pp->physaddr[0] = msbyte(base);
		pp->physaddr[1] = mdbyte(base);
		pp->physaddr[2] = lsbyte(base);
#ifdef	DEBUG
		DPR(8,5)("%X %X;", fraglen, base);
#endif
	}
	/*
	 * If the data buffer was contiguous in physical memory,
	 * there was no need for a scatter/gather list; We're done.
	 */
	if (i > 1)
	{
		/*
		 * We need a scatter/gather list.
		 * Allocate one and copy the list we built into it.
		 */
		oip = spl5 ();
		while (dma_free_list == NULL)
			sleep ((caddr_t) &dma_free_list, PZERO);
		lp = dma_free_list;
		dma_free_list = lp->next;
		lp->next = NULL;
		splx (oip);

		sp->sp_vip = (long) lp;
		sp->sp_pdb = kvtophys ((caddr_t) lp);
		sp->sp_isz = i * sizeof (DMA_PAIR);
		bcopy((caddr_t) &tmp_list.pair[0], (caddr_t) lp, sp->sp_isz);
	}
	return;
} /* wd_dmalist () */


static void
wd_dmafreelist (lp)
DMA_LIST *lp;
{
	register int oip;
	
#ifdef DEBUG
	DPR(8,5)("wd_dmafreelist:\n");
#endif
	oip = spl5 ();
	
	lp->next = dma_free_list;
	dma_free_list = lp;
	
	if (lp->next == NULL)
	{
		wakeup ((caddr_t) &dma_free_list);
	}
	splx (oip);
	
} /* wd_dmafreelist () */


static void
wd_callback(psb)
struct sb2 *psb;
{
	sdi_callback(psb->sbp);
}


wd_flushq (c, t, l)
int			c;
int			t;
int			l;
{
	struct sb2	*sp;
	struct lu_ctrl	*lp = &LU(c, t, l);
	struct sb2	*prev;
	struct sb2	*next;

	prev = NULL;
	next = lp->jp;
	for (;prev != next;) {
		sp = next;
		prev = next;
		next = sp->sp_forw;

		if (sp->sp_sent == FALSE) {
			SC(c).jobs--;
			switch (sp->sbp->sb_type)
			{
			case SCB_TYPE:
				if (lp->jobs.t.normal > 0)
					lp->jobs.t.normal--;
				if (TC(c, t).waiting_jobs) {
					SC(c).waiting_jobs--;
					TC(c, t).waiting_jobs--;
				}
				sp->sp_sent = TRUE;	/* fake out sub_job */
				sub_job(sp, c, t, l);
				sp->sbp->SCB.sc_comp_code = SDI_QFLUSH;
				wd_callback(sp);
				break;
			default:
				break;
			}
		}
	}

}
