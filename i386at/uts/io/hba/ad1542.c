/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/hba/ad1542.c	1.38"
#ident	"$Header: roy/Adaptec 1/28/92 $"

/*	Copyright (c) 1988, 1989  Intel Corporation	*/
/*	All Rights Reserved	*/

/*      INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied under the terms of a license agreement   */
/*	or nondisclosure agreement with Intel Corporation and may not be   */
/*	copied or disclosed except in accordance with the terms of that    */
/*	agreement.							   */

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
 * Comments in general by Roy A. Neese (RAN - Adaptec Engineer)
 * The way I write code is to take the approach of keeping it simple.
 * In my style, you could see many ways of improving the code, make it more
 * efficient, but the reason I do write the code the way I do is I know
 * someone else is going to have to put thier hands on this code someday and
 * it would be nice if it was easy to understand.  So I give up a little in
 * code efficiency to make life easier for the next guy.
 *
 * The comments here are for world wide changes in the driver that do not
 * facilitate common comments in the source.  You will see them notated
 * by "RAN Cx".  Those are comments telling you to look here.
 *
 * C0 - Changed the way q_count was used.  It is now a counter to keep track of
 *	how many jobs are running on any given target.  Want to allow about 2
 *	commands per target.  Remember, it is the job of the driver to try to
 *	keep the adapter as busy as possible.
 * C1 - Added a true I/O queue depth counter (q_depth) to track how many I/O's
 *	are in the the queue.  Need this to know whether I should try to do a
 *	s/g command or not.
 * C2 - adsc_freeblk() is now the macro FREEBLK as all is has to do is now
 *	reset the c_active flag in the ccb.  This happens as now the ccbs are
 *	a circular linked list.  Made it simpler to get a free one and easier
 *	to free it once you are done with it.
 * C3 - added some externs to allow the user to alter bus on/off times, dma
 *	transfer rate, and s/g enable/disable.  In space.c is quite a bit of
 *	comments on the variables and how they should(!) be used.
 */

/*
**	SCSI Host Adapter Driver for Adaptec AHA-1542A
*/

#include <svc/errno.h>
#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <util/cmn_err.h>
#include <fs/buf.h>
#include <svc/systm.h>
#include <acc/priv/privilege.h>
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
#include <io/hba/ad1542.h>
#include <util/mod/moddefs.h>
#include <io/ddi.h>
#include <io/ddi_i386at.h>

extern	struct	hba_idata	adscidata[];
extern	int	adsc_cntls;
extern	int	adsc_num_ccbs;

#define		DRVNAME	"adsc - Adaptec SCSI HBA driver"

STATIC	int	adsc_load(), adsc_unload();
int	adscinit();
void	adscstart();

MOD_HDRV_WRAPPER(adsc, adsc_load, adsc_unload, NULL, DRVNAME);

static	int	mod_dynamic = 0;

STATIC int
adsc_load(c)
int c;
{
	mod_dynamic = 1;

	if( adscinit()) {
		return( ENODEV );
	}
	mod_drvattach( &adsc_attach_info );
	adscstart();
	return(0);
}

STATIC int
adsc_unload()
{
	return(EBUSY);
}

HBA_INFO(adsc, 0, 0x10000);

int adscdevflag = D_NEW | D_DMA;		/* SVR4 style driver	*/

/* Allocated in space.c */
extern int		adsc_ctlr_id;	    /* hba scsi id		*/
extern int		sc_lowat;	    /* LU Q low water mark	*/
extern struct ver_no	sdi_ver;	    /* SDI version structure	*/
extern long		sdi_started;	    /* SDI initialization flag	*/
/*
 * I added these to be used in space.c, so a user could better tailor the
 * system to meet the needs RAN C3 03/31/92
 */
extern int		adsc_sg_enable;		/* s/g control byte */
extern int		adsc_buson;		/* bus on time */
extern int		adsc_busoff;		/* bus off time */
extern int		adsc_dma_rate;		/* DMA transfer rate */

/* The sm_poolhead struct is used for the dynamic struct allocation routines */
/* as pool to extarct from and return structs to. THis pool provides 28 byte */
/* structs (the size of sblk_t structs roughly).			     */
extern struct head	sm_poolhead;

struct scsi_ha		*sc_ha;		    /* SCSI HA structures	*/
#ifdef AHA_DEBUG
dma_t		*dmalist;	    /* pointer to DMA list pool		*/
dma_t		*dfreelist;	    /* List of free DMA lists	*/
#else
static dma_t		*dmalist;	    /* pointer to DMA list pool	*/
static dma_t		*dfreelist;	    /* List of free DMA lists	*/
#endif
static char		sc_cmd[MAX_CMDSZ];  /* SCSI commands		*/
static char		adscinit_time;	    /* Init time (poll) flag	*/
static struct ident	inq_data;	    /* Inquiry data 		*/
static int		adsc_gtol[MAX_HAS]; /* xlate global hba# to loc */
static int		adsc_ltog[MAX_HAS]; /* local hba# to global     */
static int		adsc_hacnt;	    /* # of ad1542 HBAs found   */

struct adsc_dmac {
    unsigned long amode;
    unsigned char dmode;
    unsigned long amask;
    unsigned char dmask;
    unsigned long mca_amask;
    unsigned char mca_dmask;
   
};

SGARRAY		*scatter;
SGARRAY		*sg_next;		/* RAN 04/02/92 */

unsigned char	ad_ccb_offset[4];

/*
 * Added this to correct a potential problem.  If the sc_lowat is set
 * to allow more than 1 job per target, the driver could run out of ccb
 * resources.  Ideally, the number of ccb's should be sc_lowat * NMBX
 * for each adapter.
 * RAN 04/03/92
 */
int	adsc_num_ccbs;

/*
 * DMA channels 1, 2, 3, and 4 are not used by the 1540.  If an attempt to
 * set the software to any of these DMA channel is done, it would be best to
 * panic than to program them.
 * RAN 12/06/91
 */

struct adsc_dmac adsc_dmac[] = {
    {	/* DMA channel 0 */
	0x0b,			/* DMA mode address */
	0xc0,			/* mode data */
	0x0a,			/* DMA mask address*/
	0x00,			/* mask data */
	0x0a,			/* MCA DMA mask address*/
	0x04,			/* MCA mask data */
    },
/*
 * These DMA channels are not used by the 1540.  If an attempt to set the
 * software to any of these DMA channel is done, it would be best to panic
 * than to program them.
 * RAN 12/06/91
 */
    {   /* DMA channel 1 */
        0x0b,                   /* DMA mode address */
        0xc1,                   /* mode data */
        0x0a,                   /* DMA mask address*/
        0x01,                   /* mask data */
        0x0a,                   /* MCA DMA mask address*/
        0x05,                   /* MCA mask data */
    },
    {   /* DMA channel 2 */
        0x0b,                   /* DMA mode address */
        0xc2,                   /* mode data */
        0x0a,                   /* DMA mask address*/
        0x02,                   /* mask data */
        0x0a,                   /* MCA DMA mask address*/
        0x06,                   /* MCA mask data */
    },
    {   /* DMA channel 3 */
        0x0b,                   /* DMA mode address */
        0xc3,                   /* mode data */
        0x0a,                   /* DMA mask address*/
        0x03,                   /* mask data */
        0x0a,                   /* MCA DMA mask address*/
        0x07,                   /* MCA mask data */
    },
    {   /* DMA channel 4 */
        0xd6,                   /* DMA mode address */
        0xc0,                   /* mode data */
        0xd4,                   /* DMA mask address*/
        0x00,                   /* mask data */
        0xd4,                   /* MCA DMA mask address*/
        0x04,                   /* MCA mask data */
    },
    {	/* DMA channel 5 */
	0xd6,			/* DMA mode address */
	0xc1,			/* mode data */
	0xd4,			/* DMA mask address*/
	0x01,			/* mask data */
	0xd4,			/* MCA DMA mask address*/
	0x05,			/* MCA mask data */
    },
    {	/* DMA channel 6 */
	0xd6,			/* DMA mode address */
	0xc2,			/* mode data */
	0xd4,			/* DMA mask address*/
	0x02,			/* mask data */
	0xd4,			/* MCA DMA mask address*/
	0x06,			/* MCA mask data */
    },
    {	/* DMA channel 7 */
	0xd6,			/* DMA mode address */
	0xc3,			/* mode data */
	0xd4,			/* DMA mask address*/
	0x03,			/* mask data */
	0xd4,			/* MCA DMA mask address*/
	0x07,			/* MCA mask data */
    }
};

int		adscopen(dev_t *, int, int, cred_t *);
void		adscstart();
int		adscinit();

struct ccb *	adsc_getblk(int);
void		adsc_dmalist(sblk_t *, struct proc *);
void		adsc_flushq(struct scsi_lu *, int, int);
void		adsc_init_ccbs();
void		adsc_pass_thru();
void		adsc_int();
void		adsc_putq(struct scsi_lu *, sblk_t *);
void		adsc_next(struct scsi_lu *);
void		adsc_func(sblk_t *);
void		adsc_send(int, int, struct ccb *);
void		adsc_cmd(SGARRAY *, sblk_t *);
void		adsc_done(int, int, struct ccb *, int, struct sb *);
int		ha_init();
int		adsc_hainit(int);
void		ha_done(int, struct ccb *, int);
void		adsc_sgdone(int, struct ccb *, int);
long		adsc_tol();
SGARRAY		*adsc_get_sglist();
void		adsc_sctgth(struct scsi_lu *);

#ifdef AHA_DEBUG
int adscdebug = 0;
#endif


/*
** Function name: adscstart()
** Description:
**	Called by kernel to perform driver initialization
**	after the kernel data area has been initialized.
*/

void
adscstart()
{
	/*
	 * Clear init time flag to stop the HA driver
	 * from polling for interrupts and begin taking
	 * interrupts normally.
	 */
	adscinit_time = FALSE;
}


/*
** Function name: adscopen()
** Description:
** 	Driver open() entry point. It checks permissions, and in the
**	case of a pass-thru open, suspends the particular LU queue.
*/

int
adscopen(devp, flags, otype, cred_p)
dev_t	*devp;
cred_t	*cred_p;
int	flags;
int	otype;
{
	dev_t	dev = *devp;
	register int	c = adsc_gtol[SC_HAN(dev)];
	register int	t = SC_TCN(dev);
	register int	l = SC_LUN(dev);
	register struct scsi_lu *q;
	int  oip;

	if (drv_priv(cred_p)) {
		return(EPERM);
	}
	if (t == sc_ha[c].ha_id)
		return(0);

	/* This is the pass-thru section */

	q = &LU_Q(c, t, l);

	oip = spl5();
 	if ((q->q_count > 0)  || 		/* RAN C0 */
	    (q->q_flag & (QBUSY | QSUSP | QPTHRU)))
	{
		splx(oip);
		return(EBUSY);
	}

	q->q_flag |= QPTHRU;
	splx(oip);
	return(0);
}


/*
** Function name: adscclose()
** Description:
** 	Driver close() entry point.  In the case of a pass-thru close
**	it resumes the queue and calls the target driver event handler
**	if one is present.
*/

int
adscclose(dev, flags, otype, cred_p)
cred_t	*cred_p;
dev_t	dev;
int	flags;
int	otype;
{
	register int	c = adsc_gtol[SC_HAN(dev)];
	register int	t = SC_TCN(dev);
	register int	l = SC_LUN(dev);
	register struct scsi_lu *q;
	int  oip;

	if (adsc_illegal(SC_HAN(dev), t, l, 0)) {
		return(ENXIO);
	}

	if (t == sc_ha[c].ha_id)
		return(0);

	q = &LU_Q(c, t, l);

	oip = spl5();
	q->q_flag &= ~QPTHRU;

#ifdef AHA_DEBUG
	sdi_aen(SDI_FLT_PTHRU, c, t, l);
#else
	if (q->q_func != NULL)
		(*q->q_func) (q->q_param, SDI_FLT_PTHRU);
#endif

	adsc_next(q);
	splx(oip);
	return(0);
}



/*
** Function name: adscioctl()
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
adscioctl(dev, cmd, arg, mode, cred_p, rval_p)
cred_t	*cred_p;
int	*rval_p;
dev_t	dev;
int	cmd;
caddr_t	arg;
int	mode;
{
	register int	c = adsc_gtol[SC_HAN(dev)];
	register int	t = SC_TCN(dev);
	register int	l = SC_LUN(dev);
	register struct sb *sp;
	int  oip;
	int  uerror = 0;

	if (adsc_illegal(SC_HAN(dev), t, l, 0)) {
		return(ENXIO);
	}

	switch(cmd) {
	case SDI_SEND: {
		register buf_t *bp;
		struct sb  karg;
		int  rw;
		char *save_priv;

		if (t == sc_ha[c].ha_id) { 	/* illegal ID */
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
		sp->SCB.sc_cmdpt = sc_cmd;

		if (copyin(karg.SCB.sc_cmdpt, sp->SCB.sc_cmdpt,
		    sp->SCB.sc_cmdsz)) {
			uerror = EFAULT;
			goto done;
		}

		rw = (sp->SCB.sc_mode & SCB_READ) ? B_READ : B_WRITE;
		bp->b_resid = (long)sp;

		/*
		 * If the job involves a data transfer then the
		 * request is done thru physiock() so that the user
		 * data area is locked in memory. If the job doesn't
		 * involve any data transfer then adsc_pass_thru()
		 * is called directly.
		 */
		if (sp->SCB.sc_datasz > 0) { 
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

			if (uerror = physiock(adsc_pass_thru, bp, dev, rw, 
			    4194303, &ha_uio)) {
				goto done;
			}
		} else {
			bp->b_un.b_addr = sp->SCB.sc_datapt;
			bp->b_bcount = sp->SCB.sc_datasz;
			bp->b_blkno = NULL;
			bp->b_dev = dev;
			bp->b_flags |= rw;

			adsc_pass_thru(bp);  /* fake physio call */
			biowait(bp);
		}

		/* update user SCB fields */

		karg.SCB.sc_comp_code = sp->SCB.sc_comp_code;
		karg.SCB.sc_status = sp->SCB.sc_status;
		karg.SCB.sc_time = sp->SCB.sc_time;

		if (copyout((caddr_t)&karg, arg, sizeof(struct sb)))
			uerror = EFAULT;

	   done:
		freerbuf(bp);
		sdi_freeblk(sp);
		splx(oip);
		break;
		}

	case B_GETTYPE:
		if(copyout("scsi", ((struct bus_type *)arg)->bus_name, 5)) {
			return(EFAULT);
		}
		if(copyout("scsi", ((struct bus_type *)arg)->drv_name, 5)) {
			return(EFAULT);
		}
		break;

	case	HA_VER:
		if (copyout((caddr_t)&sdi_ver,arg, sizeof(struct ver_no)))
			return(EFAULT);
		break;
	case SDI_BRESET: {
		register struct scsi_ha *ha = &sc_ha[c];

		oip = spl5();
		if (ha->ha_npend > 0)     /* jobs are outstanding */
			return(EBUSY);
		else {
			cmn_err(CE_WARN,
				"!%s: HA %d - Bus is being reset\n",
				adscidata[c].name, adsc_ltog[c]);
			outb(ha->ha_base + HA_CNTL, HA_SBR);
		}
		splx(oip);
		break;
		}

	default:
		return(EINVAL);
	}
	return(uerror);
}


/*
** Function name: adscintr()
** Description:
**	Driver interrupt handler entry point.  Called by kernel when
**	a host adapter interrupt occurs.
*/

void
adscintr(vect)
unsigned int  vect;	/* HA interrupt vector	*/
{
	register struct scsi_ha	 *ha;
	register struct mbx	 *mp;
	register struct ccb	 *cp;
	register int		 c,n;
	unsigned long		 addr;
/*
 * Had to add this also.  Yes, let's make it bigger and harder to read than
 * it already is.  I wish I didn't have to do this, but I need to be able to
 * control the queue depth for each device as well as the adapter's queue
 * depth, so I can safely multi-thread.  Anybody got any better ideas?  Speak
 * now!
 * RAN 11/21/91
 */
	struct scsi_lu		*q;
	char			t,l;
	char			*err_str;


#ifdef AHA_DEBUG
	if(adscdebug > 0)
		printf("adscintr(%d)\n", vect);
#endif
	/*
	 * Determine which host adapter interrupted from
	 * the interrupt vector.
	 */
	for (c = 0; c < adsc_cntls; c++) {
		ha = &sc_ha[c];
		if (ha->ha_vect == vect)
			break;
	}

	if( !adscidata[c].active) {
#ifdef AHA_DEBUG
                cmn_err (CE_WARN,"%s: Inactive HBA received an interrupt.\n",
adscidata[c].name);
#endif
                return;
        }
 
	/*
	 * Search for an incoming mailbox that is not empty.
	 * A round-robin search algorithm is used.
	 */
	n  = NMBX;
	mp = ha->ha_take;
	do {
		if (mp->m_stat != EMPTY)
			break;
		if (++mp == &ha->ha_mbi[NMBX])
			mp = ha->ha_mbi;
	} while (--n);

	/*
	 * Repeat until no outstanding incoming messages
	 * are left before exiting the interrupt routine.
	 */
	while (mp->m_stat != EMPTY) {
		switch (mp->m_stat) {
		case FAILURE:
		case ABORTED:
		case SUCCESS:
			addr = sdi_swap24((int)mp->m_addr);
			cp = (struct ccb *) xphystokv(addr);

/*
 * The adapter status has been virtually ignored all along.  If an adapter
 * (c_hstat) error occurs, the user needs to know about it, as there will
 * be no sense key/error code to rely upon for the actual error that may
 * have occured.
 * RAN 11/22/91
 */
			if(cp->c_hstat > HS_ST) {
				switch(cp->c_hstat) {
					case HS_DORUR:
						err_str =
						   "Data Over/Under Run\n";
						break;
					case HS_UBF:
						err_str =
						   "Unexpected Bus Free\n";
						break;
					case HS_TBPSF:
						err_str =
						   "Target Bus Phase Sequence Failure\n";
						break;
					case HS_ICOC:
						err_str =
						   "Invalid CCB Operation Code\n";
						break;
					case HS_LCDNHTSL:
						err_str =
						   "Linked CCB does not have the same LUN\n";
						break;
					case HS_ITDRFH:
						err_str =
						   "Invalid Target Direction Received from Host\n";
						break;
					case HS_DCRITM:
						err_str =
						   "Duplicate CCB Received in Target Mode\n";
						break;
					case HS_ICOSLP:
						err_str =
						   "Invalid CCB or Segment List Pointer\n";
						break;
					default:
						err_str =
						   "Unknown Adapter Status\n";
						break;
				}
				cmn_err(CE_WARN, "%s: %d: %s",
				   adscidata[c].name, adsc_ltog[c], err_str);
				if(cp->c_hstat > HS_ICOSLP)
					cmn_err(CE_CONT, " 0x%x", cp->c_hstat);
			}
			ha->ha_npend--;
/*
 * Added this code to get the q_count variable out and decrement it
 * RAN 11/21/91
 */
			t = (cp->c_dev >> 5);			/* RAN C0 */
			l = (cp->c_dev & 0x07);			/*    ^   */
			q = &LU_Q(c, t, l);			/*    ^   */
			q->q_count--;				/*    ^   */
			if(q->q_count >= sc_lowat)		/*    ^   */
				q->q_flag |= QBUSY;		/*    ^   */
			else
				q->q_flag &= ~QBUSY;		/*    ^   */


			if ((cp->c_opcode == SCSI_CMD) || 
			    (cp->c_opcode == SCSI_DMA_CMD)) {
				adsc_sgdone(c, cp, mp->m_stat);
			} else 
				ha_done(c, cp, mp->m_stat);

			break;

		case NOT_FND:	/* CB presented before timeout abort */
			break;

		default:

#ifdef AHA_DEBUG
			cmn_err(CE_WARN,
				"%s: HA %d - Illegal mailbox status 0x%x\n",
				adscidata[c].name, adsc_ltog[c], mp->m_stat);
#endif
			break;
		}

		/*
		 * Mark mail box as empty and advance 
		 * to the next incoming mail box.
		 */
		mp->m_stat = EMPTY;
		if (++mp == &ha->ha_mbi[NMBX])
			mp = ha->ha_mbi;
	}
/*
 * Acknowledge the interrupt, once you have finished all the current MBI's.
 * RAN 11/19/91
 */
	if((inb(ha->ha_base + HA_ISR)) & HA_INTR)
		outb((ha->ha_base + HA_CNTL), HA_IACK); /* acknowledge */

	ha->ha_take = mp;   /* save pointer value */
}


/*
** Function name: adsc_done()
** Description:
**	This is the interrupt handler routine for SCSI jobs which have
**	a controller CB and a SCB structure defining the job.
*/

void
adsc_done(flag, c, cp, status, sp)
int			c;	/* HA controller number */
int			status;	/* HA mbx status */
struct ccb		*cp;	/* command block */
int			flag;	/* used to control the start of the next job.*/
register struct sb	*sp;	/* the guy used for normal commands */
{
	register struct scsi_lu	*q;
	register int		i;
	long			x;
	char			t,l;

#ifdef AHA_DEBUG
	if(adscdebug > 0)
		printf("adsc_done(%x, %x, %x)\n", c, cp, status);
#endif

	t = (cp->c_dev >> 5);
	l = (cp->c_dev & 0x07);
	q = &LU_Q(c, t, l);

	ASSERT(sp);

	q->q_flag &= ~QSENSE;	/* Old sense data now invalid */


	/* Determine completion status of the job */
	switch(status) {
	case SUCCESS:
#ifdef AHA_DEBUGSGK
		if(cp->c_opcode == SCSI_DMA_CMD)
			printf("adsc_done SUCCESS\n");
#endif
		sp->SCB.sc_comp_code = SDI_ASW;
		break;

	case FAILURE:
#ifdef AHA_DEBUGSG
		printf("adsc_done FAILURE %x\n", cp->c_hstat);
#endif
		if(cp->c_hstat == NO_ERROR) {	/* good HA status */
#ifdef AHA_DEBUG
			if(adscdebug > 3)
				printf("adsc_done FAILURE - NO_ERROR\n");
#endif
			if (cp->c_tstat == S_GOOD) {
#ifdef AHA_DEBUG
			if(adscdebug > 3)
				printf("adsc_done FAILURE - NO_ERROR - GOOD\n");
#endif
				sp->SCB.sc_comp_code = SDI_ASW;
			} else {
				sp->SCB.sc_comp_code = (unsigned long)SDI_CKSTAT;
				sp->SCB.sc_status = cp->c_tstat;
				/* Cache request sense info (note padding) */
				bcopy((caddr_t)(cp->c_cdb+cp->c_cmdsz),
					(caddr_t)(&q->q_sense)+1,
					sizeof(q->q_sense)-1);
#ifdef AHA_DEBUGSG
				printf("<Set QSENSE>\n");
#endif
				q->q_flag |= QSENSE;

			}
		}
		else if (cp->c_hstat == NO_SELECT)
			sp->SCB.sc_comp_code = (unsigned long)SDI_NOSELE;
		else if (cp->c_hstat == TC_PROTO)
			sp->SCB.sc_comp_code = (unsigned long)SDI_TCERR; 
		else
			sp->SCB.sc_comp_code = (unsigned long)SDI_HAERR;
		break;

	case ABORTED:
#ifdef AHA_DEBUGSG
		printf("adsc_done ABORTED\n");
#endif
		sp->SCB.sc_comp_code = (unsigned long)SDI_ABORT;

		break;

	default:
		ASSERT(0);
#ifdef AHA_DEBUGSG
		cmn_err(CE_WARN, "%s: Illegal status 0x%x!\n",adscidata[c].name, status);
#endif
		sp->SCB.sc_comp_code = (unsigned long)SDI_ERROR;
		break;
	}

	/***
	** Due to a problem with the HBAs initial access
	** to media (SS_TEST), we will not suspend
	** the queue if the current failure is a result of these
	** commands.
	***/
	if( ((struct scs *)sp->SCB.sc_cmdpt)->ss_op != SS_TEST &&
	    ((struct scs *)sp->SCB.sc_cmdpt)->ss_op != SS_INQUIR ) {

		if ((sp->SCB.sc_comp_code & SDI_SUSPEND) && 
	    	(sp->SCB.sc_int != adsc_int)) {
			/***
			** The SDI Suspend bit is set, and we are
			** not "pass-thru", so suspend the queue.
			***/
			q->q_flag |= QSUSP;
		}
	}

	/* call target driver interrupt handler */
	sdi_callback(sp);

	FREEBLK(cp);			/* RAN C2 */

	if(flag == SG_START)
		adsc_next(q);
} 


/*
** Function name: ha_done()
** Description:
**	This is the interrupt handler routine for SCSI jobs which have
**	a controller CB and a SFB structure defining the job.
*/

void
ha_done(c, cp, status)
int		c;	  /* HA controller number */
int		status;	  /* HA mbx status */
struct ccb      *cp;	  /* command block */
{
	register struct scsi_lu  *q;
	register struct sb *sp;
	char	t,l;

#ifdef AHA_DEBUG
	if(adscdebug > 0)
		printf("ha_done(%x, %x, %x)\n", c, cp, status);
#endif

	t = (cp->c_dev >> 5);
	l = (cp->c_dev & 0x07);
	q = &LU_Q(c, t, l);

	sp = cp->c_bind;

	ASSERT(sp);

	/* Determine completion status of the job */

	switch (status) {
	case SUCCESS:
#ifdef AHA_DEBUG
	if(adscdebug > 3)
		printf("ha_done SUCCESS\n");
#endif
		sp->SFB.sf_comp_code = SDI_ASW;
		adsc_flushq(q, SDI_CRESET, 0);
		break;
	case FAILURE:
#ifdef AHA_DEBUG
	if(adscdebug > 3)
		printf("ha_done FAILURE\n");
#endif
		sp->SFB.sf_comp_code = (unsigned long)SDI_HAERR;
		q->q_flag |= QSUSP;
		break;
	default:
		ASSERT(0);
#ifdef AHA_DEBUG
		cmn_err(CE_WARN, "%s: Illegal status 0x%x!\n",adscidata[c].name, status);
#endif
		sp->SCB.sc_comp_code = (unsigned long)SDI_ERROR;
		break;
	}

	/* call target driver interrupt handler */
	sdi_callback(sp);

	FREEBLK(cp);			/* RAN C2 */

	adsc_next(q);
} 



/*===========================================================================*/
/* SCSI Driver Interface (SDI-386) Functions
/*===========================================================================*/


/*
** Function name: adscinit()
** Description:
**	This is the initialization routine for the SCSI HA driver.
**	All data structures are initialized, the boards are initialized
**	and the EDT is built for every HA configured in the system.
**	Changed ha_init to just verify the chance the adapter is in the
**	system.  If it is, then go ahead and malloc the stuff that will
**	be used during normal driver use.
*/

int
adscinit()
{
	register struct scsi_ha *ha;
	register struct scsi_lu *q;
	register dma_t  *dp;
	int  c, i, j, ha_structsize, mb_structsize, ccb_structsize;
	int luq_structsize, dma_listsize, sg_listsize;
	static adscfirst_time = 1;
	int sleepflag, cntl_num;

	if(!adscfirst_time) {
                cmn_err(CE_WARN,"Adaptec: Already initialized.");
                return(-1);
        }
#ifdef AHA_DEBUG
	if(adscdebug > 0)
		printf("adscinit sdi_started = %d\n", sdi_started);
#endif

	adscinit_time = TRUE;

	for(i = 0; i < MAX_HAS; i++)
		adsc_gtol[i] = adsc_ltog[i] = -1;

	sdi_started = TRUE;

	sleepflag = mod_dynamic ? KM_SLEEP : KM_NOSLEEP;
	sdi_ver.sv_release = 1;
	sdi_ver.sv_machine = SDI_386_AT;
	sdi_ver.sv_modes   = SDI_BASIC1;

	/* need to allocate space for sc_ha, must be contiguous */
	ha_structsize = adsc_cntls*(sizeof (struct scsi_ha));
	for (i = 2; i < ha_structsize; i *= 2);
	ha_structsize = i;

	sc_ha = (struct scsi_ha *)kmem_zalloc(ha_structsize, sleepflag);
	if (sc_ha == NULL) {
		cmn_err(CE_WARN, "%s: %s command blocks", adscidata[0].name, AD_IERR_ALLOC);
		return(-1);
	}

/*
 * need to allocate space for mailboxes, ccb's and lu queues
 * they must all be contiguous
 */
	mb_structsize = 2*NMBX*(sizeof (struct mbx));
	adsc_num_ccbs = sc_lowat * NMBX;	/* RAN 04/03/92 */
	ccb_structsize = adsc_num_ccbs*(sizeof (struct ccb));
	luq_structsize = MAX_EQ*(sizeof (struct scsi_lu));
	dma_listsize = NDMA * (sizeof (dma_t));
	sg_listsize = NMBX * (sizeof (SGARRAY));

	for (i = 2; i < mb_structsize; i *= 2);
	mb_structsize = i;

	for (i = 2; i < ccb_structsize; i *= 2);
	ccb_structsize = i;

	for (i = 2; i < luq_structsize; i *= 2);
	luq_structsize = i;

	for (i = 2; i < dma_listsize; i *= 2);
	dma_listsize = i;

	for (i = 2; i < sg_listsize; i *= 2);
	sg_listsize = i;

#ifdef	AHA_DEBUG
	if(mb_structsize > PAGESIZE)
		cmn_err(CE_WARN, "%s: mb_structs exceeds pagesize (may cross physical page boundary)\n", adscidata[0].name);
	if(ccb_structsize > PAGESIZE)
		cmn_err(CE_WARN, "%s: CCBs exceed pagesize (may cross physical page boundary)\n", adscidata[0].name);
	if(dma_listsize > PAGESIZE)
		cmn_err(CE_WARN, "%s: dmalist exceeds pagesize (may cross physical page boundary)\n", adscidata[0].name);
	if(sg_listsize > PAGESIZE)
		cmn_err(CE_WARN, "%s: scatter array exceeds pagesize (may cross physical page boundary)\n", adscidata[0].name);
#endif /* AHA_DEBUG */

	if((dmalist = (dma_t *)kmem_zalloc(dma_listsize, sleepflag)) == NULL) {
		cmn_err(CE_WARN, "%s: %s dmalist\n", adscidata[0].name, AD_IERR_ALLOC);
		kmem_free(sc_ha, ha_structsize );
		return(-1);
	}

	/* Build list of free DMA lists */
	dfreelist = NULL;
	for (i = 0; i < NDMA; i++) {
		dp = &dmalist[i];
		dp->d_next = dfreelist;
		dfreelist = dp;
	}

	if((scatter = (SGARRAY *)kmem_zalloc(sg_listsize, sleepflag)) == NULL) {
		cmn_err(CE_WARN, "%s: %s scatter array\n", adscidata[0].name, AD_IERR_ALLOC);
		kmem_free(dmalist, dma_listsize );
		kmem_free(sc_ha, ha_structsize );
		return(-1);
	}
	sg_next = scatter;

	for (c = 0; c < adsc_cntls; c++) {
		if(!ha_init(c)) 	/* see if an adapter is there */
			continue;

		ha = &sc_ha[c];

		ha->ha_mbo = (struct mbx *)kmem_zalloc(mb_structsize, sleepflag);
		if (ha->ha_mbo == NULL) {
			cmn_err(CE_WARN, "%s: %s mailboxes\n", adscidata[c].name, AD_IERR_ALLOC);
			continue;
		}
		ha->ha_mbi = &ha->ha_mbo[NMBX];

		ha->ha_ccb = (struct ccb *)kmem_zalloc(ccb_structsize, sleepflag);
		if (ha->ha_ccb == NULL) {
			cmn_err(CE_WARN, "%s: %s command blocks\n", adscidata[c].name, AD_IERR_ALLOC);
			continue;
		}

		ha->ha_dev = (struct scsi_lu *)kmem_zalloc(luq_structsize, sleepflag);
		if (ha->ha_dev == NULL) {
			cmn_err(CE_WARN, "%s: %s lu queues \n", adscidata[c].name, AD_IERR_ALLOC);
			continue;
		}

		ha->ha_state  = 0;
		ha->ha_id     = adsc_ctlr_id;
		ha->ha_npend  = 0;
		ha->ha_give   = &ha->ha_mbo[0];
		ha->ha_take   = &ha->ha_mbi[0];

		for (j = 0; j < MAX_EQ; j++)
		{
			q = &ha->ha_dev[j];
			q->q_first = NULL;
			q->q_last  = NULL;
			q->q_count = 0;		/* RAN C0 */
			q->q_flag  = 0;
			q->q_depth = 0;		/* RAN C1 */
			q->q_func  = NULL;
		}

		sc_ha[c].ha_base = adscidata[c].ioaddr1;
		sc_ha[c].ha_vect = adscidata[c].iov;
		adsc_dma_init(c);

#ifdef AHA_DEBUG
		if(adscdebug > 3)
			printf("adscinit calling adsc_init_ccbs\n");
#endif

		adsc_init_ccbs(c);

		if(!adsc_hainit(c)) {	/* initialize HA communication */
			continue;
		}

		/* Get an HBA number from SDI and Register HBA with SDI */
		if( (cntl_num = sdi_gethbano( adscidata[c].cntlr )) <= -1) {
	     		cmn_err (CE_WARN,"%s: No HBA number available.\n", adscidata[c].name);
			continue;
		}

		adscidata[c].cntlr = cntl_num;
		adsc_gtol[cntl_num] = c;
		adsc_ltog[c] = cntl_num;

		if( (cntl_num = sdi_register(&adschba_info, &adscidata[c])) < 0) {
	     		cmn_err (CE_WARN,"%s: HA %d, SDI registry failure %d.\n",
				adscidata[c].name, c, cntl_num);
			continue;
		}

		adscidata[c].active = 1;
		adsc_hacnt++;
	}

	for (c = 0; i < adsc_cntls; c++) {
		if( !adscidata[c].active) {
			ha = &sc_ha[c];
			if( ha->ha_mbo != NULL) 
				kmem_free((void *)ha->ha_mbo, mb_structsize);
			if( ha->ha_ccb != NULL) 
				kmem_free((void *)ha->ha_ccb, ccb_structsize);
			if( ha->ha_dev != NULL) 
				kmem_free((void *)ha->ha_dev, luq_structsize);
			ha->ha_mbo = NULL;
			ha->ha_ccb = NULL;
			ha->ha_dev = NULL;
		}
	}
	if (adsc_hacnt == 0) {
		cmn_err(CE_NOTE,"!%s: No HAs found.\n",adscidata[0].name);
		kmem_free((void *)sc_ha, ha_structsize);
		kmem_free(dmalist, dma_listsize );
		dmalist = NULL;
		sc_ha = NULL;
		return(-1);
	}

	adscfirst_time = 0;

	return(0);
}


/*
** Function name: adscsend()
** Description:
** 	Send a SCSI command to a controller.  Commands sent via this
**	function are executed in the order they are received.
*/

long
adscsend(hbap)
register struct hbadata *hbap;
{
	register struct scsi_ad *sa;
	register struct scsi_lu *q;
	register sblk_t *sp = (sblk_t *) hbap;
	register int	c, t, l;
	int   oip;

#ifdef AHA_DEBUG
	if(adscdebug > 0)
		printf("adscsend(%x)\n", hbap);
#endif

	sa = &sp->sbp->sb.SCB.sc_dev;
	c = adsc_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);
	l = SDI_LUN(sa);

	if (sp->sbp->sb.sb_type != SCB_TYPE) {
		return (SDI_RET_ERR);
	}

	if (adsc_illegal(SDI_HAN(sa), t, l, sa->sa_major) ) {
		sp->sbp->sb.SCB.sc_comp_code = (unsigned long)SDI_SCBERR;
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

	adsc_putq(q, sp);

	adsc_next(q);

	splx(oip);
	return (SDI_RET_OK);
}


/*
** Function name: adscicmd()
** Description:
**	Send an immediate command.  If the logical unit is busy, the job
**	will be queued until the unit is free.  SFB operations will take
**	priority over SCB operations.
*/

long
adscicmd(hbap)
register struct  hbadata *hbap;
{
	register struct scsi_ad *sa;
	register struct scsi_lu *q;
	register sblk_t *sp = (sblk_t *) hbap;
	register int	c, t, l;
	int   oip;
	struct ident	     *inq_data;
	struct scs	     *inq_cdb;
	struct sdi_edt	*edtp;

#ifdef AHA_DEBUG
	if(adscdebug > 0)
		printf("adscicmd(%x)\n", hbap);
#endif

	oip = spl5();

	switch (sp->sbp->sb.sb_type) {
	case SFB_TYPE:
		sa = &sp->sbp->sb.SFB.sf_dev;
		c = adsc_gtol[SDI_HAN(sa)];
		t = SDI_TCN(sa);
		l = SDI_LUN(sa);
		q = &LU_Q(c, t, sa->sa_lun);
#ifdef AHA_DEBUG
	if(adscdebug > 3)
		printf("sdi_icmd: SFB c=%d t=%d l=%d \n", c, t, sa->sa_lun);
#endif

		if (adsc_illegal(SDI_HAN(sa), t, l, sa->sa_major)) {
			sp->sbp->sb.SFB.sf_comp_code =
			    (unsigned long)SDI_SFBERR;
			sdi_callback(sp->sbp);
			splx(oip);
			return (SDI_RET_OK);
		}

		sp->sbp->sb.SFB.sf_comp_code = SDI_ASW;

		switch (sp->sbp->sb.SFB.sf_func) {
		case SFB_RESUME:
			q->q_flag &= ~QSUSP;
			adsc_next(q);
			break;
		case SFB_SUSPEND:
			q->q_flag |= QSUSP;
			break;
		case SFB_RESETM:
			if((edtp = sdi_redt(c,t,l)) && edtp->pdtype == ID_TAPE) {
				/*  this is a NOP for tape devices */
				sp->sbp->sb.SFB.sf_comp_code = SDI_ASW;
				break;
			}
			/* else fall thru */
		case SFB_ABORTM:
			sp->sbp->sb.SFB.sf_comp_code = SDI_PROGRES;
			adsc_putq(q, sp);
			adsc_next(q);
			splx(oip);
			return (SDI_RET_OK);
		case SFB_FLUSHR:
			adsc_flushq(q, SDI_QFLUSH, 0);
			break;
		case SFB_NOPF:
			break;
		default:
			sp->sbp->sb.SFB.sf_comp_code =
			    (unsigned long)SDI_SFBERR;
		}

		sdi_callback(sp->sbp);
		splx(oip);
		return (SDI_RET_OK);

	case ISCB_TYPE:
		sa = &sp->sbp->sb.SCB.sc_dev;
		c = adsc_gtol[SDI_HAN(sa)];
		t = SDI_TCN(sa);
		l = SDI_LUN(sa);
		q = &LU_Q(c, t, sa->sa_lun);
#ifdef AHA_DEBUG
	if(adscdebug > 3)
		printf("sdi_icmd: SCB c=%d t=%d l=%d \n", c, t, sa->sa_lun);
#endif
		inq_cdb = (struct scs *)sp->sbp->sb.SCB.sc_cmdpt;
		if ((t == sc_ha[c].ha_id) && (l == 0) && (inq_cdb->ss_op == SS_INQUIR)) {
			inq_data = (struct ident *)sp->sbp->sb.SCB.sc_datapt;
			inq_data->id_type = ID_PROCESOR;
			(void)strncpy(inq_data->id_vendor, adscidata[c].name, 24);
			inq_data->id_vendor[23] = NULL;
			sp->sbp->sb.SCB.sc_comp_code = SDI_ASW;
			splx (oip);
			return (SDI_RET_OK);
		}

		if(adsc_illegal(SDI_HAN(sa), t, l, sa->sa_major) &&
		    !adscinit_time) {
			sp->sbp->sb.SCB.sc_comp_code =
			    (unsigned long)SDI_SCBERR;
			sdi_callback(sp->sbp);
			splx(oip);
			return (SDI_RET_OK);
		}

		sp->sbp->sb.SCB.sc_comp_code = SDI_PROGRES;
		sp->sbp->sb.SCB.sc_status = 0;

		adsc_putq(q, sp);
		adsc_next(q);
		splx(oip);
		return (SDI_RET_OK);

	default:
		splx(oip);
		return (SDI_RET_ERR);
	}
}



/*
** Function name: adscxlat()
** Description:
**	Perform the virtual to physical translation on the SCB
**	data pointer. 
*/

void
adscxlat(hbap, flag, procp)
register struct  hbadata *hbap;
int flag;
struct proc *procp;
{
	extern void adsc_dmalist();
	extern void	dma_freelist();
	register sblk_t *sp = (sblk_t *) hbap;

#ifdef AHA_DEBUG
	if(adscdebug > 0)
		printf("adscxlat(%x, %x, %x)\n", hbap, flag, procp);
#endif

	if(sp->s_dmap) {
#ifdef AHA_DEBUGSGK
		printf("s_dmap = %x, sc_link = %x\n",
		    sp->s_dmap,sp->sbp->sb.SCB.sc_link);
#endif
#ifdef AHA_DEBUG
		if(adscdebug > 0)
			printf("adscxlat: calling dma_freelist(%x)\n",
			        sp->s_dmap);
#endif
		dma_freelist(sp->s_dmap);
		sp->s_dmap = NULL;
	}

	if(sp->sbp->sb.SCB.sc_link) {
		cmn_err(CE_WARN,
		    "Adaptec: Linked commands NOT available\n");
		sp->sbp->sb.SCB.sc_link = NULL;
	}

	if(sp->sbp->sb.SCB.sc_datapt) {
		/*
		 * Do scatter/gather if data spans multiple pages
		 */
		sp->s_addr = vtop(sp->sbp->sb.SCB.sc_datapt, procp);
		if (sp->sbp->sb.SCB.sc_datasz > pgbnd(sp->s_addr))
			adsc_dmalist(sp, procp);
	} else
		sp->sbp->sb.SCB.sc_datasz = 0;

#ifdef AHA_DEBUG
	if(adscdebug > 3)
		printf("adscxlat: returning\n");
#endif
}


/*
** Function name: adscgetblk()
** Description:
**	Allocate a SB structure for the caller.  The function will
**	sleep if there are no SCSI blocks available.
*/

struct hbadata *
adscgetblk()
{
	register sblk_t	*sp;

	sp = (sblk_t *)sdi_get(&sm_poolhead, KM_NOSLEEP);
	return((struct hbadata *)sp);
}


/*
** Function name: adscfreeblk()
** Description:
**	Release previously allocated SB structure. If a scatter/gather
**	list is associated with the SB, it is freed via dma_freelist().
**	A nonzero return indicates an error in pointer or type field.
*/

long
adscfreeblk(hbap)
register struct hbadata *hbap;
{
	extern void	dma_freelist();
	register sblk_t *sp = (sblk_t *) hbap;

	if(sp->s_dmap) {
		dma_freelist(sp->s_dmap);
		sp->s_dmap = NULL;
	}
	sdi_free(&sm_poolhead, (jpool_t *)sp);
	return (SDI_RET_OK);
}

/*
** Function name: adscgetinfo()
** Description:
**	Return the name, etc. of the given device.  The name is copied into
**	a string pointed to by the first field of the getinfo structure.
*/

void
adscgetinfo(sa, getinfo)
struct scsi_ad *sa;
struct hbagetinfo *getinfo;
{
	register char  *s1, *s2;
	static char temp[] = "HA X TC X";
#ifdef AHA_DEBUG
	if(adscdebug > 0)
		printf("adscgetinfo(%x, %s)\n", sa, getinfo->name);
#endif

	s1 = temp;
	s2 = getinfo->name;
	temp[3] = SDI_HAN(sa) + '0';
	temp[8] = SDI_TCN(sa) + '0';

	while ((*s2++ = *s1++) != '\0')
		;

	getinfo->iotype = F_DMA | F_SCGTH;
}


/*===========================================================================*/
/* SCSI Host Adapter Driver Utilities
/*===========================================================================*/


/*
** Function name: adsc_pass_thru()
** Description:
**	Send a pass-thru job to the HA board.
*/

void
adsc_pass_thru(bp)
struct buf  *bp;
{
	int	c = adsc_gtol[SC_HAN(bp->b_dev)];
	int	t = SC_TCN(bp->b_dev);
	int	l = SC_LUN(bp->b_dev);
	register struct scsi_lu	*q;
	register struct sb *sp;
	struct proc *procp;
	int  oip;

	sp = (struct sb *) bp->b_resid;
#ifdef AHA_DEBUG
	if (sp->SCB.sc_wd != (long) bp)
		cmn_err(CE_PANIC,
		    "%s: Corrupted address from physio", adscidata[c].name);
#endif

	sp->SCB.sc_dev.sa_lun = l;
	sp->SCB.sc_dev.sa_fill = (adsc_ltog[c] << 3) | t;
	sp->SCB.sc_datapt = (caddr_t) paddr(bp);
	sp->SCB.sc_int = adsc_int;

	drv_getparm(UPROCP, (ulong *) &procp);
	sdi_translate(sp, bp->b_flags, procp);

	q = &LU_Q(c, t, l);

	oip = spl5();

	adsc_putq(q, (sblk_t *)((struct xsb *)sp)->hbadata_p);

	adsc_next(q);

	splx(oip);
}


/*
** Function name: adsc_int()
** Description:
**	This is the interrupt handler for pass-thru jobs.  It just
**	wakes up the sleeping process.
*/

void
adsc_int(sp)
struct sb *sp;
{
	struct buf  *bp;

#ifdef AHA_DEBUG
	if(adscdebug > 0)
		printf("adsc_int: sp=%x \n",sp);
#endif
	bp = (struct buf *) sp->SCB.sc_wd;
	biodone(bp);
}



/*
** Function name: adsc_flushq()
** Description:
**	Empty a logical unit queue.  If flag is set, remove all jobs.
**	Otherwise, remove only non-control jobs.
*/

void
adsc_flushq(q, cc, flag)
register struct scsi_lu *q;
int  cc, flag;
{
	register sblk_t  *sp, *nsp;

	ASSERT(q);

	sp = q->q_first;
	q->q_first = q->q_last = NULL;
	q->q_count = 0;		/* RAN C0 */
	q->q_depth = 0;		/* RAN C1 */

	while (sp) {
		nsp = sp->s_next;
		if (!flag && (queclass(sp) > QNORM))
			adsc_putq(q, sp);
		else {
			sp->sbp->sb.SCB.sc_comp_code = (ulong)cc;
			sdi_callback(sp->sbp);
		}
		sp = nsp;
	}
}


/*
** Function name: adsc_putq()
** Description:
**	Put a job on a logical unit queue.  Jobs are enqueued
**	on a priority basis.
*/

void
adsc_putq(q, sp)
register struct scsi_lu	*q;
register sblk_t *sp;
{
	register cls = queclass(sp);

	ASSERT(q);

#ifdef AHA_DEBUG
	if(adscdebug > 2)
		printf("adsc_putq(%x, %x\n", q, sp);
#endif

	/* 
	 * If queue is empty or queue class of job is less than
	 * that of the last one on the queue, tack on to the end.
	 */
	if(!q->q_first || (cls <= queclass(q->q_last))) {
		if(q->q_first) {
			q->q_last->s_next = sp;
			sp->s_prev = q->q_last;
		} else {
			q->q_first = sp;
			sp->s_prev = NULL;
		}
		sp->s_next = NULL;
		q->q_last = sp;

	} else {
		register sblk_t *nsp = q->q_first;

		while(queclass(nsp) >= cls)
			nsp = nsp->s_next;
		sp->s_next = nsp;
		sp->s_prev = nsp->s_prev;
		if(nsp->s_prev)
			nsp->s_prev->s_next = sp;
		else
			q->q_first = sp;
		nsp->s_prev = sp;
	}
	q->q_depth++;		/* RAN C1 */
}


/*
** Function name: adsc_next()
** Description:
**	Attempt to send the next job on the logical unit queue.
**	All jobs are not sent if the Q is busy.
*/

void
adsc_next(q)
register struct scsi_lu *q;
{
	register sblk_t	*sp;
	caddr_t		p;

	ASSERT(q);
#ifdef AHA_DEBUG
	if(adscdebug > 2)
		printf("adsc_next(%x)\n", q);
#endif
	if (q->q_flag & QBUSY)
		return;

	if((sp = q->q_first) == NULL) {	 /*  queue empty  */
#ifdef AHA_DEBUG
	if(adscdebug > 2)
		printf("adsc_next: queue is empty\n");
#endif
		q->q_depth = 0;		/* RAN C1 */
		return;			/*       or	  */
	}
/*
 * QBUSY is now a misnomer.  It really indicates if the queue of commands
 * has reached the lowat mark for building I/O's for a given target.  As a
 * matter of fact the lowat variable (which is set in space.c) is really the
 * high water mark for starting I/O's on a given target.  If the number of
 * commands falls below the lowat variable, then more I/O's are allowed to
 * start, until the number of I/O's running on a given target is above the
 * lowat variable.  Once the number of I/O's is above the lowat variable,
 * the commands are shoved in the queue for later work.  This is where s/g
 * (local) becomes possible, as long as the commands have been sorted in an
 * ascending order.
 * RAN 11/23/91
 */

	if (q->q_flag & QSUSP && sp->sbp->sb.sb_type == SCB_TYPE) {
#ifdef AHA_DEBUG
	if(adscdebug > 2)
		printf("adsc_next: device is suspended\n");
#endif
		return;
	}

/*
 * The test conditions:
 * 1) If the command type is a function, instead of a normal SCSI command
 *    can't do s/g.  So do [1a].
 * 2) There is more than 1 command in the queue.
 * 3) If the current command is already a s/g command from the page I/O stuff
 *    just do it in a normal manner.
 * 4) All the test conditions were right, so we have a chance to do a s/g
 *    command.
 * 5) Shucks!  One of the test conditions (2, or 3) failed and we have to
 *    do the command normally.
 * RAN 11/23/91
 */
	if(sp->sbp->sb.sb_type == SFB_TYPE) {		/* [1]  */
		if ( !(q->q_first = sp->s_next))
			q->q_last = NULL;
		q->q_depth--;				/* RAN C1 */
		adsc_func(sp);				/* [1a] */
	} else {
#ifdef AHA_DEBUGSG1
		if(q->q_depth > 1 && sp->s_dmap == NULL)
			printf("<QD%d>", q->q_depth);
#endif
#ifdef AHA_DEBUGSGK
		if(sp->s_dmap != NULL)
			printf("<s_dmap not NULL>");
#endif
		if(q->q_depth > 1 &&			/* [2]  */
		  sp->s_dmap == NULL && adsc_sg_enable == 1) {
#ifdef AHA_DEBUGSG1
			printf("<Enter S/G %d> ", q->q_depth);
#endif
			adsc_sctgth(q);			/* [4] */
		} else {
			if(!(q->q_first = sp->s_next))
				q->q_last = NULL;
			q->q_depth--;				/* RAN C1 */
			adsc_cmd(SGNULL , sp);	/* [5]  */
		}
	}

	if(adscinit_time == TRUE) {		/* need to poll */
		if(adsc_wait(adsc_gtol[SC_HAN(ad2dev_t(sp->sbp->sb.SCB.sc_dev))], 500) == FAILURE) 
			sp->sbp->sb.SCB.sc_comp_code = (unsigned long)SDI_TIME;
		sdi_callback(sp->sbp);
	}
}


/*
** Function name: adsc_func()
** Description:
**	Create and send an SFB associated command. 
*/

void
adsc_func(sp)
register sblk_t *sp;
{
	register struct scsi_ad *sa;
	register struct ccb *cp;
	int  c, t;

	sa = &sp->sbp->sb.SFB.sf_dev;
	c = adsc_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);

	cp = adsc_getblk(c);
	cp->c_bind = &sp->sbp->sb;

	/* fill in the controller command block */

	cp->c_opcode = SCSI_TRESET;
	cp->c_dev = (char)((t << 5) | sa->sa_lun);
	cp->c_hstat = 0;
	cp->c_tstat = 0;

	cmn_err(CE_WARN,
	    "!%s: HA %d TC %d is being reset\n", adscidata[c].name, c, t);
	adsc_send(c, START, cp);   /* send cmd */
}



/*
** Function name: adsc_send()
** Description:
**	Send a command to the host adapter board.
*/

void
adsc_send(c, cmd, cp)
int		c;	/* HA controller number */
int		cmd;	/* HA mbx command */
struct ccb	*cp;	/* command block */
{
	register struct scsi_ha *ha = &sc_ha[c];
	register struct mbx	*mp;
	register int 		n;
	int			oi;
/*
 * Had to add this also.  Yes, let's make it bigger and harder to read than
 * it already is.  I wish I didn't have to do this, but I need to be able to
 * control the queue depth for each device as well as the adapter's queue
 * depth, so I can safely multi-thread.  Anybody got any better ideas?  Speak
 * now!
 * RAN 11/21/91
 */
	struct scsi_lu		*q;
	char			t,l;

#ifdef AHA_DEBUG
	if(adscdebug > 0)
		printf("adsc_send( %x, %x, %x)\n", c, cmd , cp);
#endif
	if (cmd == START) {
		ha->ha_npend++;
	}

	/*
	 * Search for an empty outgoing mail box.
	 * A round-robin search algorithm is used.
	 */
/*
 * This routine was open for being interrupted as no spl was done.  Making
 * it possible for an interrupt to fall into the space after getting the
 * MBO and marking it.  A narrow window, but a window nevertheless.
 * RAN 11/19/91
 */

	oi = spl5();	/* RAN */
	n = NMBX;
	mp = ha->ha_give;
	do {
		if (mp->m_stat == EMPTY)
			break;
		if (++mp == &ha->ha_mbo[NMBX])
			mp = ha->ha_mbo;
	} while (--n);

	if (mp->m_stat != EMPTY)
		cmn_err(CE_PANIC, "%s: Mailbox overflow", adscidata[c].name);

	/*
	 * Fill in the mailbox and inform host adapter
	 * of the new request.
         */
	mp->m_addr = sdi_swap24((int)cp->c_addr);
	mp->m_stat = cmd;
	outb((ha->ha_base + HA_CMD), HA_CKMAIL);	
/*
 * Added this code to get the q_count variable out and decrement it
 * RAN 11/21/91
 */
	t = (cp->c_dev >> 5);			/* RAN C0 */
	l = (cp->c_dev & 0x07);			/*    ^   */
	q = &LU_Q(c, t, l);			/*    ^   */
	q->q_count++;				/*    ^   */
	if(q->q_count >= sc_lowat)		/*    ^   */
		q->q_flag |= QBUSY;		/*    ^   */
	else
		q->q_flag &= ~QBUSY;		/*    ^   */

	/*
	 * Advance to the next outgoing mail box and
	 * save pointer value.
	 */
	if (++mp == &ha->ha_mbo[NMBX])
		mp = ha->ha_mbo;
	ha->ha_give = mp;
	splx(oi);
}


/*
** Function name: adsc_getblk()
** Description:
**	Allocate a controller command block structure.
**	I have rewritten this entire function RAN 12/06/91
*/

struct ccb *
adsc_getblk(c)
int	c;
{
	register struct scsi_ha	*ha = &sc_ha[c];
	register struct ccb	*cp;
	register int		cnt;
	register int		x;

	x = ad_ccb_offset[c];
	cp = &ha->ha_ccb[x];
	cnt = 0;

	/*
	 * [1]	If the current ccb is not being used then (1a), advance the
	 *	counter, check to see if it needs to wrap (probably could use
	 *	x %= adsc_num_ccbs but what the heck) and break out of the loop.
	 * [2]	The value of x shoud be at least one more than it was before.
	 *	Mark the ccb active(TRUE) and return the pointer.
	 * [3]	The current ccb is active, so let's increment a counter.  If
	 *	the counter is greater than than the number of mailboxes
	 *	(adsc_num_ccbs), then we have searched through the entire list
	 *	of ccb's and didn't find one we could use (too bad).  Now we
	 *	have to PANIC the system.  This should really never occur unless
	 *	something has gone awry somewhere else.  But if we still have
	 *	some ccb's to look at, we advance to the next ccb (cp->c_next).
	 * [3a]	We would get here if we found a busy(TRUE) ccb, so we need to
	 *	move the x counter up one.
	 */
	for(;;) {
		if(cp->c_active == TRUE) {		/* [1] */
			cnt++;				/* [3] */
			if(cnt >= adsc_num_ccbs) {		/*  ^  */
				cmn_err(CE_PANIC,	/*  ^  */
				 "%s: Out of command blocks",
				 adscidata[c].name);
			}
			cp = cp->c_next;		/*  ^  */
			x++;				/* [3a] */
			if(x >= adsc_num_ccbs)		/*  ^   */
				x = 0;			/*  ^   */
		} else {				/* [1a] */
			x++;				/*  ^   */
			if(x >= adsc_num_ccbs)		/*  ^   */
				x = 0;			/*  ^   */
			break;				/*  ^   */
		}
	}
	ad_ccb_offset[c] = x;				/* [2] */
	cp->c_active = TRUE;				/*  ^  */
	return(cp);					/*  ^  */
}


/*
** Function name: adsc_dmalist()
** Description:
**	Build the physical address(es) for DMA to/from the data buffer.
**	If the data buffer is contiguous in physical memory, sp->s_addr
**	is already set for a regular SB.  If not, a scatter/gather list
**	is built, and the SB will point to that list instead.
*/

void
adsc_dmalist(sp, procp)
sblk_t *sp;
struct proc *procp;
{
	struct dma_vect	tmp_list[SG_SEGMENTS];
	register struct dma_vect  *pp;
	register dma_t  *dmap;
	register long   count, fraglen, thispage;
	caddr_t		vaddr;
	paddr_t		addr, base;
	int		i, oip;

	vaddr = sp->sbp->sb.SCB.sc_datapt;
	count = sp->sbp->sb.SCB.sc_datasz;
	pp = &tmp_list[0];

	/* First build a scatter/gather list of physical addresses and sizes */
#ifdef AHA_DEBUGSGK
	printf("<Kernel - build s/g command>\n");
#endif

	for (i = 0; (i < SG_SEGMENTS) && count; ++i, ++pp) {
		base = vtop(vaddr, procp);	/* Phys address of segment */
		fraglen = 0;			/* Zero bytes so far */
		do {
			thispage = min(count, pgbnd(vaddr));
			fraglen += thispage;	/* This many more contiguous */
			vaddr += thispage;	/* Bump virtual address */
			count -= thispage;	/* Recompute amount left */
			if (!count)
				break;		/* End of request */
			addr = vtop(vaddr, procp); /* Get next page's address */
		} while (base + fraglen == addr);

		/* Now set up dma list element */
		pp->d_addr[0] = msbyte(base);
		pp->d_addr[1] = mdbyte(base);
		pp->d_addr[2] = lsbyte(base);
		pp->d_cnt[0] = msbyte(fraglen);
		pp->d_cnt[1] = mdbyte(fraglen);
		pp->d_cnt[2] = lsbyte(fraglen);
	}
	if (count != 0)
		cmn_err(CE_PANIC, "Adaptec: Job too big for DMA list");

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
		oip = spl5();
		while ( !(dmap = dfreelist))
			sleep((caddr_t)&dfreelist, PRIBIO);
		dfreelist = dmap->d_next;
		splx(oip);

		sp->s_dmap = dmap;
		sp->s_addr = vtop((caddr_t) dmap->d_list, procp);
		dmap->d_size = i * sizeof(struct dma_vect);
#ifdef AHA_DEBUGSGK
		printf("<S/G len %d DL%ld>\n", i, datal);
#endif
		bcopy((caddr_t) &tmp_list[0],
			(caddr_t) dmap->d_list, dmap->d_size);
	}
	return;
}


/*
** Function name: dma_freelist()
** Description:
**	Release a previously allocated scatter/gather DMA list.
*/

static void
dma_freelist(dmap)
dma_t *dmap;
{
	register int  oip;
	
	ASSERT(dmap);
#ifdef AHA_DEBUG
	if(adscdebug > 0)
		printf("dma_freelist(%x)\n", dmap);
#endif

	oip = spl5();
	dmap->d_next = dfreelist;
	if (dfreelist == NULL)
		wakeup((caddr_t)&dfreelist);
	dfreelist = dmap;
	splx(oip);
}

/*
** Function Name: adsc_wait()
** Description:
**	Poll for a completion from the host adapter.  If an interrupt
**	is seen, the HA's interrupt service routine is manually called. 
**  NOTE:	
**	This routine allows for no concurrency and as such, should 
**	be used selectively.
*/

adsc_wait(c, time)
int c, time;
{
	register struct scsi_ha  *ha = &sc_ha[c];
	unsigned char	status;
	int act;

	while (time > 0) {
		status = inb(ha->ha_base + HA_ISR);
		if (status & HA_INTR ) {
			act = adscidata[c].active;
                        adscidata[c].active = 1;
			adscintr(ha->ha_vect);
                        adscidata[c].active = act;
			return (SUCCESS);
		}
		drv_usecwait(1000);  /* wait 1 msec */
		time--;
	}
	return (FAILURE);	
}

/*
** Function name: adsc_init_ccbs()
** Description:
**	Initialize the controller CB free list.
*/

void
adsc_init_ccbs(c)
int	c;
{
	register struct scsi_ha		*ha = &sc_ha[c];
	register struct ccb		*cp;
	register struct sg_array	*sg_p;
	register int			i;

#ifdef AHA_DEBUG
	if(adscdebug > 0)
		printf("adsc_init_ccbs( %x)\n", c);
#endif

/*
 * I made the ccb list a circular linked list to make it easier to get one
 * and free one as well.
 * RAN 12/04/91
 */
	ad_ccb_offset[c] = 0;
	for(i = 0; i < (adsc_num_ccbs-1); i++) {	/* RAN 04/03/92 */
		cp = &ha->ha_ccb[i];
		cp->c_addr = kvtophys((caddr_t)cp);
		cp->c_active = FALSE;
		cp->c_bind = NULL;
/*
 * Moved this down here to save some time in the actual building of the
 * commands.  If they are to be supported in the future, you will want to
 * put them back in the loop again.
 * RAN 11/22/91
 */
		cp->c_linkpt[0] = NULL;		/* RAN */
		cp->c_linkpt[1] = NULL;		/*  ^  */
		cp->c_linkpt[2] = NULL;		/*  ^  */
		cp->c_next = &ha->ha_ccb[i+1];
		cp->c_sg_p = SGNULL;
	}
	cp = &ha->ha_ccb[i];
	cp->c_addr = kvtophys((caddr_t)cp);
	cp->c_active = FALSE;
	cp->c_bind = NULL;
	cp->c_next = &ha->ha_ccb[0];
	cp->c_sg_p = SGNULL;
/*
 * Now add the initialization for the scatter/gather lists and the pointers
 * for the sblk_t sp arrays to keep track of.  I have opted for circular
 * queue (linked lists) of each of these lists.  In the routine that returns a
 * free list I will keep a forward moving global that will keep pointing to
 * the next array.  This will almost always insure the free pointer will be
 * found in the first check, keeping overhead to a minimum.
 * RAN 11/23/91
 */
	for(i = 0; i < (NMBX-1); i++) {	/* RAN 04/03/92 */
		sg_p = &scatter[i];
		sg_p->sg_flags = SG_FREE;
		sg_p->sg_next = &scatter[i+1];
	}
	sg_p = &scatter[i];
	sg_p->sg_flags = SG_FREE;
	sg_p->sg_next = &scatter[0];
}

/*
** Function name: ha_init()
** Description:
**	Reset the HA board and return good status if it looks like a 1540
**	else return bad status.
*/

int
ha_init(c)
int	c;	/* HA controller number */
{
	register ulong port = (adscidata[c].ioaddr1);
	register int	i;

/*
 * If the control port is a 0xff, the adapter is not there.  As a matter of
 * fact, there is a good chance that nothing is there.
 * but, if it isn't a 0xff then,............
 * [1]	Reset the adapter and look at the status ports to determine if it
 *	looks like a 1542 type of adapter.  If it does, then return good
 *	and keep on truckin.
 *	Regardless of the state of the 1540, there is no way the adapter
 *	status port will contain a 0xff, unless the adapter is really brain
 *	dead, in which case, it ain't gonna' work anyway.
 * RAN 12/06/91
 */
	i = inb(port + HA_CNTL);
	if(i == 0xff)
		return(0);
	else {					/* [1] */
		outb(port + HA_CNTL, HA_RST);
		spinwait(2*1000);

		i = inb(port + HA_STAT);

		if(!(i & HA_DIAGF) && (i & HA_IREQD))
			return(1);
		else
			return(0);
	}
}


/*
** adsc_hainit()
**	This is the guy that does all the hardware initialization of the
**	adapter after it has been determined to be in the system.  Must be
**	called immediately after ha_init to insure the status of the adapter
**	ports are not changed.
** RAN 12/06/91
*/

int
adsc_hainit(c)
int	c;	/* HA controller number */
{
	register struct scsi_ha  *ha = &sc_ha[c];
	register int	i;
	register char 	*ch;
	unsigned char	status;
	unsigned long	addr;

#ifdef AHA_DEBUG
	if(adscdebug > 0)
		printf("adsc_hainit(%x)\n", c);
#endif

	status = inb(ha->ha_base + HA_STAT);
	if(!(status & HA_DIAGF) && (status & HA_IREQD)) {
		/* initialize the communication area */

		for (i = 0; i < NMBX; i++) {
			ha->ha_mbo[i].m_stat = EMPTY;
			ha->ha_mbi[i].m_stat = EMPTY;
		}

		addr = kvtophys((caddr_t)ha->ha_mbo);
		ch = (char *)&addr;

		outb((ha->ha_base + HA_CMD), HA_INIT);
		spinwait(1);
		outb((ha->ha_base + HA_CMD), NMBX);
		spinwait(1);
		outb((ha->ha_base + HA_CMD), ch[2]);
		spinwait(1);
		outb((ha->ha_base + HA_CMD), ch[1]);
		spinwait(1);
		outb((ha->ha_base + HA_CMD), ch[0]);
		spinwait(1);

		outb((ha->ha_base + HA_CNTL), HA_IACK);
/*
 * The bus on time should be set to 7 micro-seconds.  This is the safest
 * value to use without starving floppy DMA.
 * RAN 11/19/91
 */
                /* Set Bus-On Time to 7 microseconds */
		outb((ha->ha_base + HA_CMD), HA_BONT);
		spinwait(1);
		outb((ha->ha_base + HA_CMD), adsc_buson);
		spinwait(1);
                outb((ha->ha_base + HA_CNTL), HA_IACK);

/*
 * By default the adapter is usually (keyword here) set to 4 micro-seconds
 * bus-off time. but there have been various versions of the adapter that
 * have used different times.  This is just to insure the adapter is set
 * correctly (I know, I know,...paranoia).
 * RAN 11/20/19
 */
		/* Set Bus-Off Time to 4 micro-seconds  RAN 11/20/91 */
		outb((ha->ha_base + HA_CMD), HA_BOFFT);
		spinwait(1);
		outb((ha->ha_base + HA_CMD), adsc_busoff);
		spinwait(1);
                outb((ha->ha_base + HA_CNTL), HA_IACK);
/*
 * The default for the DMA rate is set via the jumpers on the 1542, but can
 * be overriden by software.  By default the DMA rate is set to 5.0MB/sec
 * from the factory, but can be jumpered only up to 8.0MB/sec.  Software
 * can set the speed higher and quite a few speeds inbetween.  See space.c
 * for further explanations.
 * RAN 03/31/92
 */
		outb((ha->ha_base + HA_CMD), HA_XFERS);
		spinwait(1);
		outb((ha->ha_base + HA_CMD), adsc_dma_rate);
		spinwait(1);
                outb((ha->ha_base + HA_CNTL), HA_IACK);

		status = inb(ha->ha_base + HA_STAT);
		if(!(status & HA_IREQD) && (status & HA_READY)) {
#ifdef AHA_DEBUG
	if(adscdebug > 3) { 
		printf("ha_init: this HA is operational\n");
	}
#endif
			cmn_err(CE_NOTE,
			    "%s found at address 0x%X\n",
			     adscidata[c].name, ha->ha_base);
			/* mark this HA operational */
			ha->ha_state |= C_SANITY;
			return(1);
		}
	}
	cmn_err(CE_CONT, "!%s %d NOT found at address 0x%X\n",
	    adscidata[c].name, adsc_ltog[c],ha->ha_base);

	return(0);
}

/*
** Function name: adsc_dma_init()
** Description:
**	Init the mother board dma channel for first party DMA.
**	c is the location controller number
*/
adsc_dma_init(c)
int c;
{
	uint bus_p;

	/* get the dma channel number from the configuration info */
	register ulong chan = (adscidata[c].dmachan1 & 0x7);

	/* if running in a micro-channel machine, do different initialization */
	if (!drv_gethardware(IOBUS_TYPE, &bus_p) && (bus_p == BUS_MCA)) {

	/* set mask on of the needed channel */
		outb(adsc_dmac[chan].mca_amask,adsc_dmac[chan].mca_dmask);

	}
	else
	{
/*
 * If the DMA chan gets set to something other that 0, 5, 6, or 7, something
 * will go wrong.  I put in this as insurance, but you may want to deal with
 * it in another manner.  FIXIT?
 * RAN 12/06/91
 */

	if(chan > 0 && chan < 5) {
		cmn_err(CE_PANIC,
		    "%s: DMA channel %d is not valid for the 154x",
		    adscidata[c].name, chan);
	}

	/* set mode of the needed channel */
	outb(adsc_dmac[chan].amode,adsc_dmac[chan].dmode);

	/* set mask off of the needed channel */
	outb(adsc_dmac[chan].amask,adsc_dmac[chan].dmask);
	}
}

adsc_illegal(hba, scsi_id, lun, m) 
short hba;
unsigned char scsi_id, lun;
int m;
{
	if(sdi_redt(hba, scsi_id, lun)) {
		return(0);
	} else {
#ifdef AHA_DEBUG
	if(adscdebug > 0)
		printf("adsc_illegal(%d,%d,%d)\n",hba, scsi_id, lun);
#endif
		return(1);
	}
}

/*
** adsc_sctgth()
**	Here's a brand new routine.  This one is responsible for building
**	local s/g commands.  It depends on the driver being able to queue
**	commands for any given target, as well as the adapter being able to
**	support s/g type commands.
**	There probably ought to be a variable to control whether or not s/g
**	is enabled in space.c FIXIT?
**	RAN 12/06/91
*/

void
adsc_sctgth(q)
register struct scsi_lu *q;
{
	register SGARRAY	*sg_p;
	register SG		*sg;
	register sblk_t		*sp, *start_sp;
	char			current_command, command;
	int			segments;
	int			length;
	long			start_block, current_block;
	long			total_count;
	caddr_t			p;
	paddr_t			addr;

/*
 * This comment section is rather long.  I hope you don't mind how I like to
 * comment code.  If you do, tough.  RAN 12/06/91
 *
 * [1]	Right off the bat, see if we can get a s/g list.  If not, do the
 *	command in a normal manner.
 * [2]	Get the command to do.
 * [3]	Not real sure why this is done, but that is the way it was done in the
 *	original code.
 * [4]	Is the command a NULL?  If not, then doit, else return.
 * [5]	What type of command is it?
 * [6]	Just your normal SCSI command.
 * [7]	Must have gotten a s/g list.
 * [8]	Set the segment counter to 0.  segment is used to count the number of
 *	s/g segments we have done, as well as getting the pointer to the next
 *	s/g segment in the list.
 * [9]	While the command is not a NULL, try to build a s/g command.
 * [10]	Darn, the command is a function type of command.  Can't get a s/g
 *	list for that.
 * [11]	Get a pointer to the current segment in the s/g list of segments.
 * [12]	If segments == 0, then this is the first segment of the s/g list,
 *	so I need to setup the initial variables.  You may wonder why I don't
 *	do this outside of the while() loop.  The next step in the evolution
 *	of the code is to make it recursively look through the queued command
 *	list for more s/g stuff.
 * [13]	Have to get the SCSI command for a later compare.
 * [14]	Now we have to get the physical block address of the command.  I hope
 *	that number will be stored by sd01 in some element in the future
 *	FIXIT?
 * [15]	Got to get the total count also.  13, 14, and 15 become clear in a
 *	second.
 * [16]	Got to save the pointer to this command in two places.  One in
 *	start_sp, in case we don't get a hit and in spcomms, in case we do get
 *	a hit.
 * [17]	Shove the count and data address for this command into the first s/g
 *	segment.
 * [18]	Increment the segment variable so we can get the next s/g segment.
 * [19]	Set the length to the length of a s/g segment.
 * [20]	Point to the next command in the queue.
 * [21]	Start it again, at ([9]).
 * [22]	Okay, we have setup the first s/g segment, now the fun begins.  Get
 *	the block number and command for this command.
 * [23]	Now let's see if we have a potential s/g command.  If the command
 *	is the same as the first command, the block number + the total
 *	count = the first command block number, and the number of segments
 *	used does not exceed the s/g list >>>>we have a s/g hit!
 * [24]	Increment the total count of data by what is set in the current
 *	command.
 * [25]	Increment the length of the s/g list.
 *	NOTE: The stuff between 25 and 26 should be self explanatory.
 * [26] Darn, we did not have a s/g hit, or have hit the end of the s/g list.
 * [27]	We only get here if we have s/g hits and still have segments to use.
 *	So we need to make sure we are pointing to the next command.
 *
 * There are three conditions that will get us to this point (28).  1) We
 * have a s/g command to do.  2) There are no more commands in the queue to
 * look at.  3) We do not have a sequential access, so no s/g to do.
 *
 * [28]	Darn, no s/g hit, we'll have to do the command as a normal I/O.
 * [29]	Is it a real command?  Or am I just being paranoid?
 * [30] Oh BOY!! A s/g command to do.
 * [31]	Cleanup time.  There will always be at least one more command to do
 *	at this point unless, we are leaving due to running out of commands
 *	in the queue, so this check is not a paranoid check.
 * [32]	Make sure we are pointing to the next command before leaving, or you
 *	will be sorry.
 */


	if((sg_p = adsc_get_sglist()) == SGNULL) {		/* [1] */
		sp = q->q_first;				/* [2] */
		if(!(q->q_first = sp->s_next))			/* [3] */
			q->q_last = NULL;			/*  ^  */
		if(sp != NULL) {				/* [4] */
			if (sp->sbp->sb.sb_type == SFB_TYPE) {	/* [5] */
#ifdef AHA_DEBUGSG
				printf("<FUNCTION CALLED>\n");
#endif
				q->q_depth--;			/* RAN C1 */
				adsc_func(sp);
			} else {				/* [6] */
#ifdef AHA_DEBUGSG
				printf("<NO S/G lists>");
#endif
				q->q_depth--;
				adsc_cmd(SGNULL, sp);
			}
		}
		return;
	}							/* [7] */
	segments = 0;						/* [8] */
	start_sp = NULL;
	while((sp = q->q_first) != NULL) {			/* [9] */
		if ( !(q->q_first = sp->s_next))		/* [20]*/
			q->q_last = NULL;
		if(sp->sbp->sb.sb_type == SFB_TYPE) {		/* [10]*/
#ifdef AHA_DEBUGSG
			printf("<SFB_TYPE>\n");
#endif
			q->q_depth--;	/* RAN C1 */
			adsc_func(sp);
			if(segments == 0) {
				sg_p->sg_flags = SG_FREE;
				return;
			}
			continue;
		}
 		p = sp->sbp->sb.SCB.sc_cmdpt;			/* [13]*/
		if((sp->sbp->sb.SCB.sc_datasz > 65535 || sp->s_dmap != NULL) ||
		   (p[0] != 0x8 && p[0] != 0x28 &&
		   p[0] != 0xa && p[0] != 0x2a)) {
#ifdef AHA_DEBUGSG
			if(segments > 1)
				printf("Breaking out of s/g loop\n");
#endif
			break;
		}
		sg = &sg_p->sg_seg[segments];			/* [11]*/
		if(segments == 0) {				/* [12]*/
			command = p[0];
/*
 * This is how we get the disk block number from the stuff above me.
 * RAN 11/22/91
 */
			if(p[0] == 0x28 || p[0] == 0x2a) {
				start_block = adsc_tol(&p[2]);
				/* [15] */
				total_count = sp->sbp->sb.SCB.sc_datasz;
			} else {
				start_block = (p[1] & 0x1f);
				start_block <<= 8;
				start_block |= p[2];
				start_block <<= 8;
				start_block |= p[3];
				/* [15] */
				total_count = sp->sbp->sb.SCB.sc_datasz;
			}
			start_sp = sp;				/* [16]*/
			sg_p->spcomms[segments] = sp;
			adsc_mkadr3(&sg->sg_size_msb, &total_count); /* [17]*/
			adsc_mkadr3(&sg->sg_addr_msb, &sp->s_addr);
			segments++;				/* [18] */
			length = SG_LENGTH;			/* [19] */
#ifdef AHA_DEBUGSG1
			printf("<SB%ld TC%d, SC%d L%d>\n",
			    start_block, (total_count >> 9),
			    command, length);
#endif
			continue;				/* [21]*/
		}
		current_command = p[0];				/*  ^  */
		if(p[0] == 0x28 || p[0] == 0x2a) {
			current_block = adsc_tol(&p[2]);
		} else {
			current_block = (p[1] & 0x1f);
			current_block <<= 8;
			current_block |= p[2];
			current_block <<= 8;
			current_block |= p[3];
		}
								/* [23] */
		if(((start_block + (total_count >> 9)) == current_block) &&
		  command == current_command &&
		  segments < SG_SEGMENTS) {
			total_count += sp->sbp->sb.SCB.sc_datasz;   /* [24]*/
			length += 6;				/* [25] */
			sg_p->spcomms[segments] = sp;
			adsc_mkadr3(&sg->sg_size_msb,
			   &sp->sbp->sb.SCB.sc_datasz);
			adsc_mkadr3(&sg->sg_addr_msb, &sp->s_addr);
			segments++;
#ifdef AHA_DEBUGSG
			printf("<SB%ld CB%ld TC%d, SC%d CC%d L%d ADDR%lx>\n",
			    start_block, current_block, (total_count >> 9),
			    command, current_command, length, sp->s_addr);
#endif
		} else {
#ifdef AHA_DEBUGSG1
			printf("<Stop Search> ");
#endif
			break;					/* [26] */
		}
#ifdef AHA_DEBUGSG1
		printf("<Continue Search>\n");
#endif
	}
	if(segments <= 1) {					/* [28] */
		sg_p->sg_flags = SG_FREE;
		if(start_sp != NULL)  {				/* [29] */
#ifdef AHA_DEBUGSG1
			printf("<Do a single I/O>\n");
#endif
			q->q_depth--;
			adsc_cmd(SGNULL, start_sp);
		}
	} else {						/* [30] */
		sg_p->sg_len = length;		/* RAN 04/02/92 */
		sg_p->dlen = total_count;	/* RAN 04/02/92 */
		sg_p->addr = kvtophys((caddr_t)&sg_p->sg_seg[0].sg_size_msb);
#ifdef AHA_DEBUGSG
		printf("<Snd S/G SG%d TC%ld QD%d ADDR%lx>\n", segments,
			total_count, q->q_depth, sg_p->addr);
#endif
		q->q_depth -= segments;				/* RAN C1 */
		adsc_cmd(sg_p, ((sblk_t *)0));
	}
	if(sp != NULL) {					/* [31] */
#ifdef AHA_DEBUGSG1
		printf("<NORMAL Cleanup>\n");
#endif
		q->q_depth--;
		adsc_cmd(SGNULL, sp);
	}
#ifdef AHA_DEBUGSG1
	else
		printf("<NO Command Cleanup>\n");
#endif
}

/*
** adsc_get_sglist
**	Here is the routine that returns a pointer to the next free
**	s/g list.  Each list is 16 segments long.  I have only allocated
**	NMBX number of these structures, instead of doing it on a per
**	adapter basis.  Doesn't hurt if it returns a NULL as it just means
**	the calling routine will just have to do it the old-fashion way.
**	This routine runs just like adsc_getblk, which is commented very
**	well.  So see those comments if you need to.
**	RAN 12/06/91
*/

SGARRAY *
adsc_get_sglist()
{
	register SGARRAY	*sg;
	register int		cnt;

	sg = sg_next;
	cnt = 0;

	for(;;) {
		if(sg->sg_flags == SG_BUSY) {
			cnt++;
			if(cnt >= NMBX) {
#ifdef AHA_DEBUGSG
				printf("<NO S/G Segments>\n");
#endif
				return(SGNULL);
			}
			sg = sg->sg_next;
		} else {
			break;
		}
	}
	sg->sg_flags = SG_BUSY;
	sg_next = sg->sg_next;
	return(sg);
}

/*
** Function name: adsc_cmd()
** Description:
**	Create and send an SCB associated command, using a scatter gather
**	list created by adsc_sctgth(). 
**	I have re-written it to take sufficient arguments to do a local
**	s/g command.  The code favors this type of command as there is more
**	overhead to build a s/g command anyway, so it should balance out.
**	RAN 12/06/91
*/

void
adsc_cmd(sg_p, sp)
SGARRAY		*sg_p;
sblk_t		*sp;
{
	register struct scsi_ad *sa;
	register struct ccb	*cp;
	register struct scsi_lu	*q;
	register int		i;
	char			*p;
	long			bcnt, cnt, sg_paddr;
	int			c, t;
	paddr_t			addr;

#ifdef AHA_DEBUG
	if(adscdebug > 2)
		printf("adsc_cmd(%x)\n", sp);
#endif

	if(sg_p != SGNULL)
		sp = sg_p->spcomms[0];

	sa = &sp->sbp->sb.SCB.sc_dev;
	c = adsc_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);

	cp = adsc_getblk(c);
	cp->c_bind = &sp->sbp->sb;

	/* fill in the controller command block */

	cp->c_dev = (char)((t << 5) | sa->sa_lun);
	cp->c_hstat = 0;
	cp->c_tstat = 0;
	cp->c_cmdsz = sp->sbp->sb.SCB.sc_cmdsz;
	cp->c_sensz = sizeof(cp->c_sense);

	/*
	 * If the sg_p pointer is not NULL or the sp->s_dmap contains a valid
	 * address, we have a s/g command to do.
	 * [1] Set the ccb opcode to a s/g type.
	 * [2] If s_dmap is valid, then set the length and addr.  If it is
	 * 	not valid, the length and addr will have been passed in the
	 *	call to adsc_cmd, to indicate the driver built a local s/g
	 *	command. Setting the c_sg_p to a NULL will allow the interrupt
	 *	to recognize this command as a normal I/O or a page I/O s/g
	 *	command.
	 * [3] Must be a normal command. (shucks)
	 * [4] Set up the ccb for a normal command, set the length and addr.
	 */
	if(sp->s_dmap || sg_p != SGNULL) {
#ifdef AHA_DEBUGSGK
		printf("<S/G command ");
#endif
		cp->c_opcode = SCSI_DMA_CMD;		/* [1] */
		if(sp->s_dmap) {			/* [2] */
#ifdef AHA_DEBUGSGK
			printf("KERNEL BUILT>\n");
#endif
			cnt = sp->s_dmap->d_size;	/*  ^  */
			addr = sp->s_addr;		/*  ^  */
			cp->c_sg_p = SGNULL;		/*  ^  */
		} else {
			cnt = sg_p->sg_len;		/* RAN 04/02/92 */
			addr = sg_p->addr;		/* RAN 04/02/92 */
#ifdef AHA_DEBUGSGK
			printf("HOMEMADE>\n");
#endif
		}
	} else {					/* [3] */
		cp->c_opcode = SCSI_CMD;		/* [4] */
		cnt = sp->sbp->sb.SCB.sc_datasz;	/*  ^  */
		addr = sp->s_addr;			/*  ^  */
		cp->c_sg_p = SGNULL;
	}

	adsc_mkadr3(&cp->c_datasz[0], &cnt);
	adsc_mkadr3(&cp->c_datapt[0], &addr);

	/* copy SCB cdb to controller CB cdb */
 	p = sp->sbp->sb.SCB.sc_cmdpt;
	for (i = 0; i < sp->sbp->sb.SCB.sc_cmdsz; i++)
		cp->c_cdb[i] = *p++;

	/*
	 * [1]  Have to save the pointer to the s/g list, if it was built
	 *	locally so it can be released at interrupt time.
	 * [2]  Go ahead and divide by 512 to get the actual block count for the
	 *	SCSI command.  This *ASSUMES* the device we are sending the
	 *	command to is a 512 byte block device.  Should be changed to
	 *	get the block size of the device, as OPTICAL drives may not be
	 *	512 byte block sizes.  FIXIT?
	 * [3]  If the command was a locally built s/g command, we have to munge
	 *	the actual number of blocks requested in the SCSI command.
	 * [3a] I don't know if I should PANIC here or not.  If this code can be
	 *	reached and *not* be a READ or WRITE, then in adsc_sctgth we
	 *	should explicitly look for READ or WRITE as part of the
	 *	determining factor whether a s/g command can be done or not.
	 */
	if(sg_p != SGNULL) {
		cp->c_sg_p = sg_p;				/* [1] */
		bcnt = sg_p->dlen >> 9;
		switch(cp->c_cdb[0]) {				/* [3] */
			case SS_READ:
				cp->c_cdb[4] = (unsigned char)bcnt;
				cp->c_dev |= HA_RD_DIR;
				break;
			case SM_READ:
				cp->c_cdb[7] =
				    (unsigned char)((bcnt >> 8) & 0xff);
				cp->c_cdb[8] =
				    (unsigned char)(bcnt & 0xff);
				cp->c_dev |= HA_RD_DIR;
				break;
			case SS_WRITE:
				cp->c_cdb[4] = (unsigned char)bcnt;
				cp->c_dev |= HA_WRT_DIR;
				break;
			case SM_WRITE:
				cp->c_cdb[7] =
				    (unsigned char)((bcnt >> 8) & 0xff);
				cp->c_cdb[8] =
				    (unsigned char)(bcnt & 0xff);
				cp->c_dev |= HA_WRT_DIR;
				break;
			default:				/* [3a] */
				cmn_err(CE_PANIC,
				    "%s: Unknown SCSI command for s/g\n",
				    adscidata[c].name);
				break;
		}
	}

	q = &LU_Q(c, t, sa->sa_lun);
	if ((q->q_flag & QSENSE) && (cp->c_cdb[0] == SS_REQSEN)) {
#ifdef AHA_DEBUGSG
		printf("<REQUEST SENSE COMMAND>\n");
#endif
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
 		if (cp->c_opcode != SCSI_DMA_CMD) {
#ifdef AHA_DEBUGSG
			printf("<Non-S/G type>\n");
#endif
 			bcopy((caddr_t)(&q->q_sense)+1,
 				(caddr_t)xphystokv(sp->s_addr),
 				cnt);
#ifdef AHA_DEBUGSG
			printf("<BCOPY DONE>\n");
#endif
 		} else {
			struct dma_vect	*VectPtr;
			int	VectCnt;
			caddr_t	Src;
			caddr_t	Dest;
			int	Count;
		 
#ifdef AHA_DEBUGSG
			printf("<S/G type>\n");
#endif
 			VectPtr = (struct dma_vect *) xphystokv(sp->s_addr);
 			VectCnt = cnt / sizeof(struct dma_vect);
 			Src = (caddr_t)(&q->q_sense) + 1;
 			for (i=0; i < VectCnt; i++) {
 				Dest = (caddr_t)(VectPtr->d_addr[0] << 16 |
 					VectPtr->d_addr[1] << 8 |
 					VectPtr->d_addr[2]);
 				Dest = (caddr_t)xphystokv((paddr_t)Dest);
 
 				Count = (int)(VectPtr->d_cnt[0] << 16 |
 					 VectPtr->d_cnt[1] << 8 |
 					 VectPtr->d_cnt[2]);
 
 				bcopy (Src, Dest, Count);
 				Src += Count;
 				VectPtr++;
 			}
 		}
#ifdef AHA_DEBUGSG
		printf("<Calling adsc_done>\n");
#endif
		adsc_done(SG_START, c, cp, SUCCESS, cp->c_bind);
		return;
	}
	adsc_send(c, START, cp);   /* send cmd */
}

/*
** adsc_tol()
**	This routine flips a 4 byte Intel ordered long to a 4 byte SCSI/
**	Adaptec style long.
*/

long
adsc_tol(adr)		/* scsi 4 byte value to 4 byte		*/
register char  adr[];
{
	union {
		char  tmp[4];
		long lval;
	}  x;

	x.tmp[0] = adr[3];
	x.tmp[1] = adr[2];
	x.tmp[2] = adr[1];
	x.tmp[3] = adr[0];
	return(x.lval);
}

/*
** adsc_mkadr3()
**	This is a handy dandy routine to convert the lower three bytes of
**	a long to the backwards 3 byte structure the 1542 uses.  Yes I know it's
**	dumb that it should have to be done anyway.  But it would either have to
**	be done on the adapter or by the host CPU, and the host CPU has quite a
**	bit more power than our poor little 8085.
**	RAN 11/23/91
*/

adsc_mkadr3(str, adr)	/* convert 4 byte address to 3 byte address */
register unsigned char	str[];	/* 3 byte field				    */
register unsigned char	adr[];	/* 4 byte field  MSB not used...	    */
{
	str[0] = adr[2];
	str[1] = adr[1];
	str[2] = adr[0];
}

/*
** adsc_sgdone()
**	This routine is used as a front end to calling adsc_done.
**	This is the cleanest way I found to cleanup a local s/g command.
**	One should note, however, with this scheme, if an error occurs
**	during the local s/g command, the system will think an error will
**	have occured on all the commands.  Not exactly the way I want it to
**	work.  FIXIT!
**	RAN 12/06/91
*/

void
adsc_sgdone(c, cp, status)
int		c;	  /* HA controller number */
struct ccb      *cp;	  /* command block */
int		status;	  /* HA mbx status */
{
	register sblk_t		*sb;
	register struct sb	*sp;
	register int		i;
	int			r;
	long			x;

	if(cp->c_opcode == SCSI_DMA_CMD && cp->c_sg_p != SGNULL) {
		x = cp->c_datasz[0];
		x <<= 8;
		x |= cp->c_datasz[1];
		x <<= 8;
		x |= cp->c_datasz[2];
		x /= 6;
#ifdef AHA_DEBUGSG
		printf("<T%d -", x);
#endif
		r = SG_NOSTART;
		for(i = 0; i < x; i++) {
			sb = cp->c_sg_p->spcomms[i];
#ifdef AHA_DEBUGSG
			ASSERT(sb);
#endif
			adsc_done(r, c, cp, status, &sb->sbp->sb);
#ifdef AHA_DEBUGSG
			cp->c_sg_p->spcomms[i] = ((sblk_t *)0);
			printf(" %d", i);
#endif
			if((i + 1) == x)
				r = SG_START;
		}
#ifdef AHA_DEBUGSG
		printf(">\n");
#endif
		cp->c_sg_p->sg_flags = SG_FREE;
		cp->c_sg_p = SGNULL;
	} else { /* call target driver interrupt handler */
#ifdef AHA_DEBUGSGK
		if(cp->c_opcode == SCSI_DMA_CMD)
			printf("Kernel S/G\n");
#endif
		sp = cp->c_bind;
		adsc_done(SG_START, c, cp, status, sp);
	}
}
