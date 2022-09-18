/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/hba/mcis.c	1.34"
#ident	"$Header: $"

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

#include <util/param.h>
#include <util/types.h>
#include <util/debug.h>
#include <proc/cred.h>
#include <util/sysmacros.h>
#include <mem/immu.h>
#include <mem/kmem.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <fs/buf.h>
#include <proc/signal.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <svc/bootinfo.h>
#include <io/conf.h>
#include <io/dma.h>
#include <io/uio.h>
#include <io/target/scsi.h>
#include <io/target/sdi_edt.h>
#include <io/target/sdi.h>
#include <io/target/dynstructs.h>
#include <io/hba/mcis.h>
#include <io/ddi.h>
#include <io/ddi_i386at.h>
#include <util/mod/moddefs.h>

#define		DRVNAME	"mcis - MCA SCSI HBA driver"

extern	struct	hba_idata	mcis_idata[];

STATIC	int	mcis_load(), mcis_unload();

int	mcis_init();
void	mcis_start();

MOD_HDRV_WRAPPER(mcis_, mcis_load, mcis_unload, NULL, DRVNAME);

HBA_INFO(mcis_, 0, 0x0);

static	int	mod_dynamic = 0;

STATIC int
mcis_load()
{
	mod_dynamic = 1;
	if( mcis_init() ) {
		return(ENODEV);
	}
	mod_drvattach(&mcis__attach_info);
	mcis_start();
	return(0);
}

STATIC int
mcis_unload()
{
	return(EBUSY);
}


#ifdef DEBUG

#define MCIS_DENTRY	0x0001
#define MCIS_DINTR	0x0002
#define MCIS_DB_ENT_EXT	0x0004
#define MCIS_DB_DETAIL	0x0008
#define MCIS_DB_MISC	0x0010

ulong	mcis_debug = 0;

#endif /* DEBUG */

#define pgbnd(a) (NBPP - ((NBPP - 1) & (int )(a)))

extern struct mcis_blk	*mcis_cfginit();
extern int		mcis_hbainit();
extern long		mcis_send();
extern void		mcis_putq();
extern void		mcis_dmalist();
extern void		mcis_next();
extern void		mcis_sfbcmd();
extern void		mcis_getedt();
extern void		mcis_initld();
extern void		mcis_inqcmd();
extern void		mcis_poscmd();
extern void		mcis_invld();
extern void		mcis_waitedt();
extern void		mcis_pass_thru();
extern void		mcis_int();
extern void		mcis_scbcmd();
extern void		mcis_flushq();
extern void		mcis_freeld();

/*
** Global Variables from space.c
*/

extern int	mcis_cache;	/* = 0                        */
extern int	mcis__cntls;	/* = MCIS__CNTLS from idtools */
extern int	mcis_global_tout;	/* mcis board timeout */

/* 
** Global variables from sdi.c 
** sm_poolhead is needed for the dynamic struct allocation routines to alloc
** the small 28 byte structs for mcis_srb structs
*/
extern struct head	sm_poolhead;

/*
** Global Variables
*/

int		mcis__devflag = D_NEW | D_DMA;

struct ver_no	mcis_ver;

struct mcis_blk	*mcis_ha[ MAX_HAS ];		/* one per board */
int		mcis_lds_assigned = FALSE;
#define MAX_CMDSZ 12			/* currently in 3 headers */
char		mcis_cmd[ MAX_CMDSZ ];
int		mcis_gtol[ MAX_HAS ];
int		mcis_ltog[ MAX_HAS ];
int		mcis_hacnt; 		/* number of active HA's */
int		mcis_polltime = TRUE;
/*
 *	The array declared below, mcis_tcn_xlat, is used to translate all
 *	incoming and outgoing references to target controller number.
 *
 *	It is used to force target 6 on the SCSI bus to be treated as target 0
 *	by UNIX.  This is to make sure we can always boot from this target,
 *	even if there is a second disk on the HBA.  This means that the boot
 *	disk MUST be target 6 for this to work all the time.
 *
 *	This hack is neccessary because UNIX always treats the first DISK
 *	device ( in target number order ) as the boot device but the Model 90
 *	MUST have the boot disk at target 6.  If you add a second disk, it
 *	will therefore have to be at a lower target number and the boot
 *	will fail.
 *
 *	DO NOT CHANGE this array unless all you do is swap pairs of numbers.
 *	Doing so will break this because the same array is used to translate
 *	into and out-of the driver and this will only work if values are
 *	swapped in pairs.  If values are rotated in some manner, there must
 *	be a different array for gtol and ltog translations.
 */
int		mcis_tcn_xlat[] = { 6, 1, 2, 3, 4, 5, 0, 7 };

void
mcis_start()
{
 	/*
        ** Clear init time flag to stop the HA driver
	** from polling for interrupts and begin taking
	** interrupts normally.
	*/

	mcis_polltime = FALSE;

}


/*
** Function name: mcis_open()
**
** Description:
**	Open host adapter driver.  May be pass-thru open to a specific
**	LU if the specified device id is not equal to the board's id.
*/

int
mcis_open( devp, flags, otype, cred_p )
dev_t	*devp;
int	flags;
int	otype;
cred_t	*cred_p;
{
	struct mcis_luq	*q;
	int		c;
	int		t;
	int		opri;
	int		error;
	int		minor = getminor( *devp );

	if ( drv_priv(cred_p) )
		return EPERM;

	c = mcis_gtol[ MCIS_HAN( minor )];
	t = mcis_tcn_xlat[ MCIS_TCN( minor ) ];

	if ( t == mcis_ha[ c ]->b_targetid )	/* Regular open */
		return 0;

	/* pass-thru open to particular lu */

	q = MCIS_LU_Q( c, t, MCIS_LUN( minor ));

	error = EBUSY;
	opri = spl5();

	if ( q->q_count == 0
		&& ! ( q->q_flag & ( MCIS_QLUBUSY | MCIS_QSUSP | MCIS_QPTHRU )))
	{
		q->q_flag |= MCIS_QPTHRU;
		error = 0;
	}

	splx( opri );
	return error;
}

/*
** Function name: mcis_close()
**
** Description:
**	Close connection to board or LU if pass-thru close.
*/

int
mcis_close( dev, flags, otype, cred_p )
dev_t	dev;
int	flags;
int	otype;
cred_t	*cred_p;
{
	int		minor = getminor( dev );
	int		c = mcis_gtol[ MCIS_HAN( minor )];
	int		t = mcis_tcn_xlat[ MCIS_TCN( minor ) ];
	int		opri;
	struct mcis_luq	*q;

	if ( t == mcis_ha[ c ]->b_targetid )
		return 0;

	/* pass-thru close */

	q = MCIS_LU_Q( c, t, MCIS_LUN( minor ));

	opri = spl5();

	q->q_flag &= ~MCIS_QPTHRU;

	/*
	** This is needed in case requests came in via
	** mcis_icmd() after pass-thru open. 
	*/

	mcis_next( q );
	splx( opri );
	return 0;
}

/*
** Function name: mcis_ioctl()
**
** Description:
*/

int
mcis_ioctl( dev, cmd, arg, mode, cred_p, rval_p )
dev_t	dev;
int	cmd;
caddr_t	arg;
int	mode;
cred_t	*cred_p;
int	*rval_p;
{
	int			minor = getminor( dev );
	register int		c = mcis_gtol[ MCIS_HAN( minor )];
	int			error = 0;
	int			opri;

	/* uid = 0 is required: it's enforced in open */

	switch( cmd )
	{
		case SDI_SEND:
		{
			register int		t = mcis_tcn_xlat[ MCIS_TCN( minor ) ];
			register struct sb	*sp;
			register buf_t		*bp;
			char			*save_priv;
			struct sb		karg;
			int			rw;

			/* Only allowed for pass-thru */

			if ( t == mcis_ha[ c ]->b_targetid )
			{
				error = ENXIO;
				break;
			}

			if ( copyin( arg, (caddr_t)&karg, sizeof( struct sb )))
			{
				error = EFAULT;
				break;
			}

			/* ISCB Only - specified in SDI Manual */

			if ( karg.sb_type != ISCB_TYPE
				|| karg.SCB.sc_cmdsz <= 0
				|| karg.SCB.sc_cmdsz > MAX_CMDSZ )
			{ 
				error = EINVAL;
				break;
			}

			sp = sdi_getblk();
			save_priv = sp->SCB.sc_priv;
			bcopy( (caddr_t)&karg, (caddr_t)sp, sizeof( struct sb ));
			bp = getrbuf(KM_SLEEP);
			opri = spl5();
			bp->b_iodone = NULL;
			sp->SCB.sc_priv = save_priv;
			sp->SCB.sc_wd = (long)bp;
			sp->SCB.sc_cmdpt = mcis_cmd;
	
			if ( copyin( karg.SCB.sc_cmdpt, sp->SCB.sc_cmdpt,
							sp->SCB.sc_cmdsz ))
			{
				error = EFAULT;
				goto done;
			}
	
			rw = ( sp->SCB.sc_mode & SCB_READ ) ? B_READ : B_WRITE;
			bp->b_resid = (long)sp;
	
			/*
			 * If the job involves a data transfer then the
			 * request is done thru physio() so that the user
			 * data area is locked in memory. If the job doesn't
			 * involve any data transfer then adsc_pass_thru()
			 * is called directly.
			 */

			if ( sp->SCB.sc_datasz > 0 )
			{ 
				struct iovec	ha_iov;
				struct uio	ha_uio;
	
				ha_iov.iov_base = sp->SCB.sc_datapt;	
				ha_iov.iov_len = sp->SCB.sc_datasz;	
				ha_uio.uio_iov = &ha_iov;
				ha_uio.uio_iovcnt = 1;
				ha_uio.uio_offset = 0;
				ha_uio.uio_segflg = UIO_USERSPACE;
				ha_uio.uio_fmode = 0;
				ha_uio.uio_resid = sp->SCB.sc_datasz;
				error = physiock( mcis_pass_thru, bp,
						dev, rw, 4194303, &ha_uio);
				if (error)
					goto done;
			}
			else
			{
				bp->b_un.b_addr = sp->SCB.sc_datapt;
				bp->b_bcount = sp->SCB.sc_datasz;
				bp->b_blkno = NULL;
				bp->b_dev = dev;
				bp->b_flags |= rw;
	
				mcis_pass_thru( bp );  /* fake physio call */
				biowait( bp );
			}
	
			/* update user SCB fields */
	
			karg.SCB.sc_comp_code = sp->SCB.sc_comp_code;
			karg.SCB.sc_status = sp->SCB.sc_status;
			karg.SCB.sc_time = sp->SCB.sc_time;
	
			if ( copyout( (caddr_t)&karg, arg, sizeof( struct sb )))
				error = EFAULT;

done:			splx( opri );
			freerbuf( bp );
			sdi_freeblk( sp );
			break;
		}
	
		case B_GETTYPE:
		{
			if ( copyout( "scsi",
				((struct bus_type *)arg)->bus_name, 5 ))
			{
				error = EFAULT;
				break;
			}

			if ( copyout( "mcis_",
				((struct bus_type *)arg)->drv_name, 6 ))
			{
				error = EFAULT;
			}

			break;
		}

		case	HA_VER:

			if ( copyout( (caddr_t)&mcis_ver, arg,
						sizeof( struct ver_no )))
				error = EFAULT;

			break;

		case SDI_BRESET:
		{
			struct mcis_blk	*mcis_blkp = mcis_ha[ c ];
			int		ctrl_addr = mcis_blkp->b_ioaddr
								+ MCIS_CTRL;
			int		i = 6;

			opri = spl5();

			if ( mcis_blkp->b_npend > 0 )	/* outstanding jobs */
				error = EBUSY;
			else
			{
				cmn_err( CE_WARN,
					"!IBM MCA Host Adapter: HA %d - Bus is being reset\n",
					mcis_ltog[ c ]);

				outb( ctrl_addr, inb( ctrl_addr ) & ISCTRL_RESERVE
							| ISCTRL_RESET );

				/*
				** According to ref man: must wait at least
				** 50ms and then turn off controller reset
				*/

				while ( i-- )
					tenmicrosec();

				/* re-enable both dma and intr */

				outb( ctrl_addr, inb( ctrl_addr ) & ISCTRL_RESERVE
						| ISCTRL_EDMA | ISCTRL_EINTR );

			}

			splx( opri );
			break;
		}

		case SDI_TRESET:
		default:

			error = EINVAL;
	}

	return( error );
}

/*
** Function name: mcis_getinfo()
**
** Description:
**	Return the name and iotype of the given device.  The name is copied
**	into a string pointed to by the first field of the getinfo structure.
*/

void
mcis_getinfo( sa, getinfo )
struct scsi_ad	*sa;
struct hbagetinfo	*getinfo;
{
 	register char	*s1, *s2;
	static char	temp[] = "HA X TC X";

#ifdef MCIS_DEBUG
	 if( mcis_debug > 0 )
		printf( "mcis_getinfo( 0x%x, 0x%x )\n", sa, getinfo );
#endif

	s1 = temp;
	s2 = getinfo->name;
	temp[ 3 ] = SDI_HAN( sa ) + '0';
	temp[ 8 ] = SDI_TCN( sa ) + '0';

	while (( *s2++ = *s1++ ) != '\0' )
		;
	getinfo->iotype = F_DMA | F_SCGTH;
}

/*
** Function name: mcis_getblk()
**
** Description:
**	Allocate a SB structure for the caller.	 The function calls
**	sdi_get to return a struct will be cast as mcis_srb.
*/

struct hbadata *
mcis_getblk()
{
	struct mcis_srb	*srbp;

	srbp = (struct mcis_srb *)sdi_get(&sm_poolhead, KM_NOSLEEP);
	return (struct hbadata *)srbp;
}

/*
** Function name: mcis_freeblk()
**
** Description:
**	Frees up hbadata/mcis_srb by calling sdi_free which returns
**	struct back to the pool.
*/

long
mcis_freeblk( hbap )
register struct hbadata	*hbap;
{
	register struct mcis_srb	*srbp = (struct mcis_srb *)hbap;

	sdi_free(&sm_poolhead, (jpool_t *)srbp);
	return SDI_RET_OK;
}


/*
** Function name: mcis_xlat()
**
** Description:
**	Perform the virtual to physical translation on the SCB data pointer,
**	possibly creating scatter/gather list if buffer spans physical pages
**	and is non-contiguous.
*/

void
mcis_xlat( hbap, flag, procp )
register struct hbadata	*hbap;
int			flag;
struct proc		*procp;
{
	struct mcis_srb	*srbp = (struct mcis_srb *)hbap;
	struct xsb	*sbp = srbp->s_sbp;

#ifdef MCIS_DEBUG
	if ( mcis_debug > 0 )
		printf( "mcis_xlat( 0x%x, 0x%x, 0x%x )\n", hbap, flag, procp );
#endif /* MCIS_DEBUG */

	/*
	** The IBM MCA SCSI board supports linked commands, but the
	** SVR4 SDI manual says they are not supported.
	*/

	if ( sbp->sb.SCB.sc_link )
	{
		cmn_err( CE_WARN,
		   "!IBM MCA Host Adapter: Linked commands NOT available\n");

		sbp->sb.SCB.sc_link = NULL;
	}

	if ( sbp->sb.SCB.sc_datapt )
	{
		srbp->s_addr = vtop( sbp->sb.SCB.sc_datapt, procp );
		srbp->s_size = sbp->sb.SCB.sc_datasz;
		srbp->s_proc = procp;
	}
	else
	{
		srbp->s_addr = 0;
		srbp->s_size = 0;
	}

#ifdef MCIS_DEBUG
	if ( mcis_debug > 3 )
		printf( "mcis_xlat: returning\n" );
#endif
}

/*
** Function name: mcis_init()
**
** Description:
*/

int
mcis_init( )
{
	static int	mcisfirst_time = TRUE;
	int		size;
	int		cntl_num, c, i;
	uint	bus_p;
	struct	mcis_blk	*blkp;
	int	ctrl_addr;

#ifdef MCIS_MLDEBUG
	printf( "mcis_bdinit entry\n" );
#endif

	if (!mcisfirst_time) {
#ifdef MCIS_MLDEBUG
		cmn_err(CE_WARN,"MCIS init called more than once");
#endif
		return(-1);
	}

	/* if not running in a micro-channel machine, skip initialization */
	if (!drv_gethardware(IOBUS_TYPE, &bus_p) && (bus_p != BUS_MCA))
		return(-1);

	for ( i = 0; i < MAX_HAS; i++ )
		mcis_gtol[ i ] = mcis_ltog[ i ] = -1;

	for (c = 0; c < mcis__cntls; c++) {
		if (!mcis_hbainit(c))	{
			continue;
		}
		else {
			if ((cntl_num = sdi_gethbano(mcis_idata[c].cntlr)) <= -1) {
				cmn_err(CE_WARN,"%s: No HBA number.",mcis_idata[c].name);
				continue;
			}
			mcis_idata[c].cntlr = cntl_num;
			mcis_gtol[cntl_num] = c;
			mcis_ltog[c] = cntl_num;

			if ((cntl_num=sdi_register(&mcis_hba_info,&mcis_idata[c])) < 0) {
				cmn_err(CE_WARN,"%s: HA %d SDI register slot %d failed",
                                	mcis_idata[c].name, c, cntl_num);
                                	continue;
			}
			mcis_idata[c].active = 1;

			mcis_hacnt++;

			/* enable dma and intr */
			blkp = mcis_ha[c];
			ctrl_addr = blkp->b_ioaddr + MCIS_CTRL;
			outb( ctrl_addr, inb( ctrl_addr ) & ISCTRL_RESERVE | 
				ISCTRL_EDMA | ISCTRL_EINTR );
		}
	}
	if (mcis_hacnt == 0)	{
		cmn_err(CE_NOTE,"!%s: No HA's found.", mcis_idata[0].name);
		return(-1);
	}

	mcisfirst_time = FALSE;
	mcis_ver.sv_release = 1;
	mcis_ver.sv_machine = SDI_386_MCA;
	mcis_ver.sv_modes = SDI_BASIC1;

#ifdef MCIS_MLDEBUG
	printf("mcis_bdinit leave\n");
#endif
	return 0;
}


/*
** hba scsi command send routine
**
** This routine only accepts SCB_TYPE commands
*/

long
mcis_send( hbap )
struct hbadata *hbap;
{
	struct mcis_srb	*srbp = (struct mcis_srb *)hbap;
	struct xsb	*sbp = srbp->s_sbp;
	struct mcis_luq	*q;
	struct scsi_ad	*sa;
	long		error = SDI_RET_OK;
	int		opri;

#ifdef DEBUG
 {
	if ( mcis_debug & MCIS_DB_ENT_EXT )
		printf( "mcis_send( 0x%x )\n", hbap );

	if ( sbp->sb.sb_type != SCB_TYPE )
	{
		printf( "mcis_send: bad sb_type\n" );
		return SDI_RET_ERR;
	}
 }
#endif /* DEBUG */

	sa = &sbp->sb.SCB.sc_dev;

	q = MCIS_LU_Q( mcis_gtol[ SDI_HAN( sa )], mcis_tcn_xlat[ SDI_TCN( sa ) ], SDI_LUN( sa ));

	opri = spl5();

	if ( q->q_flag & MCIS_QPTHRU )
	{
		error = SDI_RET_RETRY;
	}
	else
	{
		mcis_putq( q, srbp );
		mcis_next( q );
	}

	splx( opri );

#ifdef DEBUG
	if ( mcis_debug & MCIS_DB_ENT_EXT )
		printf( "mcis_icmd: exiting\n" );
#endif

	return error;
}


/*
** Function name: mcis_icmd()
**
** Description:
**	Send an immediate command.  If the logical unit is busy, the job
**	will be queued until the unit is free.  SFB operations will take
**	priority over SCB operations.
*/

long
mcis_icmd( hbap )
struct hbadata *hbap;
{
	struct mcis_srb	*srbp = (struct mcis_srb *)hbap;
	struct xsb	*sbp = srbp->s_sbp;
	struct scsi_ad	*sa;
	struct mcis_luq	*q;
	register int	c;
	register int	t;
	register int	l;
	int		opri;
	int		i;
	int		sb_type = sbp->sb.sb_type;
	struct ident	*inq_data;
	struct scs	*inq_cdb;

#ifdef DEBUG
	if ( mcis_debug & MCIS_DB_ENT_EXT )
		printf( "mcis_icmd( 0x%x )\n", hbap );
#endif

	if ( sb_type == SFB_TYPE )
		sa = &sbp->sb.SFB.sf_dev;
	else
		sa = &sbp->sb.SCB.sc_dev;

	c = mcis_gtol[ SDI_HAN( sa )];
	t = mcis_tcn_xlat[ SDI_TCN( sa ) ];
	q = MCIS_LU_Q( c, t, SDI_LUN( sa ));

#ifdef DEBUG
	if ( mcis_debug & MCIS_DB_DETAIL )
		printf( "mcis_icmd: c = %d t = %d l = %d\n", c, t, sa->sa_lun );
#endif

	opri = spl5();

	switch ( sb_type )
	{
		case SFB_TYPE:

			sbp->sb.SFB.sf_comp_code = SDI_ASW;
	
			switch ( sbp->sb.SFB.sf_func ) 
			{
				case SFB_RESUME:

					q->q_flag &= ~MCIS_QSUSP;
					mcis_next( q );
					break;

				case SFB_SUSPEND:

					q->q_flag |= MCIS_QSUSP;
					break;

				case SFB_ABORTM:
				case SFB_RESETM:

					mcis_putq( q, srbp );
					mcis_next( q );
					splx( opri );
					return SDI_RET_OK;

				case SFB_FLUSHR:

					mcis_flushq( q, SDI_QFLUSH, 0 );
					break;

				case SFB_NOPF:

					break;

				default:

					sbp->sb.SFB.sf_comp_code = SDI_SFBERR;
					break;
			}
	
			sdi_callback( &sbp->sb );
			splx( opri );
			return SDI_RET_OK;
	
		case ISCB_TYPE:

			inq_cdb = (struct scs *)sbp->sb.SCB.sc_cmdpt;
			l = SDI_LUN( sa );

			if ( inq_cdb->ss_op == SS_INQUIR )
			{
				inq_data = (struct ident *)sbp->sb.SCB.sc_datapt;

				/* Is inquiry data request for the host adapter ? */

				if ( t == mcis_ha[ c ]->b_targetid )
				{
					if ( l )	/* fail if lu != 0 */
					{
						sbp->sb.SCB.sc_comp_code = SDI_SCBERR;
						splx( opri );
						return SDI_RET_OK;
					}
	
					inq_data->id_type = ID_PROCESOR;
	
					(void)strncpy( inq_data->id_vendor,
						mcis_idata[c].name, 24 );
	
					inq_data->id_vendor[23] = NULL;
				}
				else
				{
					struct ident	*ident;

					ident = &mcis_ha[ c ]->b_scb_array[(t << 3) | l]->s_ident;
					bcopy( ident, inq_data,
							sizeof( struct ident ));

				}

				sbp->sb.SCB.sc_comp_code = SDI_ASW;
				splx( opri );
				return SDI_RET_OK;
			}
	
			mcis_putq( q, srbp );
			mcis_next( q );
			splx( opri );
			return SDI_RET_OK;
	
		default:

			splx( opri );
			return SDI_RET_ERR;
	}
}

/*
 *	interrupt handler
 */

void
mcis_intr( vect )
int	vect;
{
	register struct	mcis_blk	*blkp;
	struct xsb			*sbp;
	register struct mcis_srb	*srbp;
	register struct	mcis_scb	*scbp;
	struct mcis_tsb			*tsbp;
	unsigned long			*comp_code;
	char				status;
	char				*sp = &status;
	ushort				ioaddr;
	int				scb_type = 1;
	int				c;
#ifdef MCIS_MLDEBUG3
	static	mcis_intrcnt = 0;

	mcis_intrcnt++;
#endif

	/* Determine which host adapter interrupted */

	for ( c = 0; c < mcis_hacnt; c++ )
	{
		blkp = mcis_ha[ c ];

		if ( blkp->b_intr == vect )
			break;
	}

	if ( c == mcis_hacnt || ! ( inb( blkp->b_ioaddr + MCIS_STAT)
							& ISSTAT_INTRHERE ))
	{
		return;
	}

	ioaddr = blkp->b_ioaddr;

	status = inb( ioaddr + MCIS_INTR );
	blkp->b_intr_code = MCIS_rintrp( sp )->ri_code;
	blkp->b_intr_dev  = MCIS_rintrp( sp )->ri_ldevid;

	if ( MCIS_BUSYWAIT( ioaddr ))
		return;

	MCIS_SENDEOI( ioaddr, blkp->b_intr_dev );
	blkp->b_npend--;
	scbp = blkp->b_ldm[ blkp->b_intr_dev ].ld_scbp;

	if( scbp == 0 ) {
#ifdef MCIS_MLDEBUG3
		printf("mcis_intr: ioaddr 0x%x, vect 0x%x, mcis_intrcnt 0x%x\n",
			ioaddr, vect, mcis_intrcnt);
		printf("mcis_intr:  b_intr_dev 0x%x, b_intr_code 0x%x, blkp->b_npend 0x%x\n",
			blkp->b_intr_dev, blkp->b_intr_code, blkp->b_npend);
#endif
		return;
	}
	srbp = scbp->s_srbp;
	sbp = srbp->s_sbp;

	if ( sbp->sb.sb_type == SFB_TYPE )
	{
		scb_type = FALSE;
		comp_code = &sbp->sb.SFB.sf_comp_code;
	}
	else
	{
		comp_code = &sbp->sb.SCB.sc_comp_code;
	}

	switch ( blkp->b_intr_code )
	{
		case ISINTR_SCB_OK:
		case ISINTR_SCB_OK2:

			*comp_code = SDI_ASW;
			break;

		case ISINTR_ICMD_OK:

			/* check for assign command */

			if ( scbp->s_status == MCIS_WAITLD )
			{
				blkp->b_ldm[ MCIS_HBALD ].ld_scbp = NULL;
				scbp->s_status = MCIS_GOTLD;

				if ( scb_type )
					mcis_scbcmd( srbp, scbp );
				else
					mcis_sfbcmd( srbp, scbp );

			/*
			** If there was an error sending the command,
			** mcis_s[cf]bcmd() set comp_code to SDI_ABORT and
			** did sdi_callback().  We cannot just return and
			** catch the error here.  mcis_s[cf]bcmd() MUST
			** deal with the error since they are normally
			** called outside of interrupt routine.
			*/

				return;
			}

			*comp_code = SDI_ASW;
			break;

		case ISINTR_CMD_FAIL:

			if ( ! scb_type )
			{
				*comp_code = SDI_ABORT;
				break;
			}

			tsbp = &scbp->s_tsb;
#ifdef MCIS_DEBUG
			if ( mcis_debug & MCIS_DINTR )
				printf( "mcis_intr: FAIL tstat= 0x%x terr= 0x%x herr= 0x%x\n",
					tsbp->t_targstat, tsbp->t_targerr, 
					tsbp->t_haerr);
#endif
			/* record target device status */

			/* This is correct */

			sbp->sb.SCB.sc_status = tsbp->t_targstat << 1;

			if ( ! tsbp->t_haerr && ! tsbp->t_targerr
					&& ! tsbp->t_targstat)
				*comp_code= SDI_ASW;

			else if ( tsbp->t_haerr )
				*comp_code = SDI_HAERR;

			else
				*comp_code = SDI_CKSTAT;

			break;

		default:

			*comp_code = SDI_ABORT;
			break;
	}

	if ( ! mcis_lds_assigned )
		mcis_freeld( blkp, scbp->s_ldmap );

	scbp->s_luq.q_flag &= ~MCIS_QLUBUSY;

	if ( *comp_code == SDI_ASW )
	{
		mcis_next( &scbp->s_luq );
		sdi_callback( &sbp->sb );
	}
	else
	{
		sdi_callback( &sbp->sb );
	
		/*
		** If comp_code == SDI_CKSTAT, a lot can happen before we ever
		** get to the mcis_next below().  sdi_callback will genereate
		** an SS_REQSEN command to mcis_icmd which will call mcis_putq()
		** and mcis_next().  If the request sense command is placed first
		** on the Q, all is fine. The command will get sent to the board,
		** and we will return here to finally call the following
		** mcis_next().  At which point the Q will be busy and mcis_next
		** will return.  Finally this interrupt routine will be called
		** and the sense info will be returned to the target driver.
		**
		** Let's consider the case where the request sense will not be
		** put on the front of the Q.  The only commands ahead of it will
		** be of type SFB_TYPE or ISCB_TYPE.  If the head of the Q is an
		** SFB_TYPE, it has to be an SFB_ABORTM in which case we don't
		** care that the request sense data we want may be invalidated by
		** the completion of the abort.  The target driver was aborting
		** the same command that generated the SDI_CKSTAT.
		**
		** If the command on the head of the Q was an ISCB_TYPE, there is
		** no guarantee what the command will be.  Whatever it is, it will
		** may (or is that will ?) invalidate the sense info for the
		** command we are processing right here.  This could be a problem,
		** however ISCB_TYPE are special purpose commands and for the time
		** being, we accept any consequences.
		**
		** If wierd things ever start to happen, this comment is meant to
		** indicate that there may be a potential hole here.
		**
		** I considered requesting the sense information right here,
		** saving it, and then returning it when the subsequent
		** request described above came in.  Unfortunately there are
		** several problems with that approach.  First I'd have to
		** poll for the resulting interrupt otherwise the SS_REQSEN
		** from the higher level driver may get here before the
		** sense information is returned.  The trouble with polling
		** is there may be many LU's attached with interrupts pending
		** ahead of the SS_REQSEN I'm waiting for.  Then I'd have to
		** deal with them also and they may require the same SS_REQSEN
		** processing, and well,  I think you see what I mean.  It
		** could get ugly.  I'm not saying it's impossible, I just
		** don't think the added complexity will buy us anything in
		** performance or correctness.
		*/
	
		mcis_next( &scbp->s_luq );
	}
}

/*
** Function name: mcis_scbcmd()
**
** Description:
**	Create and send an SCB associated command. 
*/

void
mcis_scbcmd( srbp, scbp )
struct mcis_srb	*srbp;
struct mcis_scb	*scbp;
{
	struct mcis_blk	*blkp = scbp->s_blkp;
	struct xsb	*sbp = srbp->s_sbp;
	char		*cmdpt;
	unchar		*scb_cdb;
	int		lcv;

	if ( ! mcis_lds_assigned )
	{
		if ( scbp->s_status == MCIS_GOTLD )
		{
			scbp->s_status = 0;
		}
		else
		{
			/* check for assign error */

			if ( mcis_getld( blkp, scbp, 1 ) == MCIS_HBALD )
			{
				goto abort;
			}
			else if ( scbp->s_status == MCIS_WAITLD )
				return;
		}
	}

	/* Determine if it's a group 0 or group 1 scsi command */

	cmdpt = sbp->sb.SCB.sc_cmdpt;
	scb_cdb = scbp->s_cdb;

	lcv = SCS_SZ;

	if ( cmdpt[ 0 ] & 0x20 )	/* Group 1 Command */
		lcv = SCM_SZ;

	while ( lcv-- )
		*scb_cdb++ = *cmdpt++;

	scbp->s_cmdop = ISCMD_SNDSCSI;
	scbp->s_cmdsup = ISCMD_LSCB;
	scbp->s_ch = 0;		/* zero this out in case anyone has set it */

	/*
	** I could do this during initialization, but as long as I
	** would have to set turn off MCIS_PT bit anyhow, this is
	** just as easy.
	*/

	scbp->s_opfld = MCIS_BB | MCIS_RE | MCIS_ES;

	/*
	** If it's a REQUEST SENSE scsi command, the SS bit needs to be
	** set because the device may return sense or extended sense
	** information.  The SS bit tells the adapter not to generate
	** an error if the number of bytes returned is less that the
	** number requested.
	*/

	if ( scbp->s_cdb[ 0 ] == SS_REQSEN )
		scbp->s_opfld |= MCIS_SS;

	if ( sbp->sb.SCB.sc_mode & SCB_READ )
		scbp->s_opfld |= MCIS_RD;

	if ( scbp->s_cdb[ 0 ] == SS_MSENSE )
		scbp->s_opfld |= (MCIS_SS | MCIS_RD);

	scbp->s_baddr = sbp->sb.SCB.sc_cmdsz;

	if ( srbp->s_addr && sbp->sb.SCB.sc_datasz > pgbnd( srbp->s_addr ))
		mcis_dmalist( scbp );

	scbp->s_hostaddr = srbp->s_addr;
	scbp->s_hostcnt = srbp->s_size;

	if ( mcis_docmd( blkp, scbp, scbp->s_ldmap->LD_CB.a_ld, ISATTN_LSCB ))
	{
abort:
		sbp->sb.SCB.sc_comp_code = SDI_ABORT;
		scbp->s_luq.q_flag &= ~MCIS_QLUBUSY;
		sdi_callback( &sbp->sb );
		mcis_next(  &scbp->s_luq );
	}
}


/*
** Function name: mcis_dmalist()
**
** Description:
**	Build the physical address(es) for DMA to/from the data buffer.
**	If the data buffer is contiguous in physical memory, sp->s_addr
**	is already set for a regular SB.  If not, a scatter/gather list
**	is built, and the SB will point to that list instead.
*/

void
mcis_dmalist( scbp )
struct mcis_scb	*scbp;
{
	register struct mcis_dma	*dmap = scbp->s_dmaseg;
	register struct mcis_srb	*srbp = scbp->s_srbp;
	struct proc			*procp = srbp->s_proc;
	long				count;
	long				fraglen;
	long				thispage;
	caddr_t				vaddr;
	paddr_t				base;
	paddr_t				addr;
	int				i;

	vaddr = srbp->s_sbp->sb.SCB.sc_datapt;
	count = srbp->s_sbp->sb.SCB.sc_datasz;

	/* First build a scatter/gather list of physical addresses and sizes */

	for ( i = 0; i < MAXDSEG && count; i++, dmap++ )
	{
		base = vtop( vaddr, procp );	/* Physical address of segment */
		fraglen = 0;			/* Zero bytes so far */

		do {
			thispage = min( count, pgbnd( vaddr ));
			fraglen += thispage;	/* This many more are contiguous */
			vaddr += thispage;	/* Bump virtual address */
			count -= thispage;	/* Recompute amount left */

			if ( ! count )
				break;		/* End of request */

			addr = vtop( vaddr, procp ); /* Get next page's address */
		} while ( base + fraglen == addr );

		/* Now set up dma list element */

		dmap->d_addr = base;
		dmap->d_cnt = fraglen;
	}

	if ( count != 0 )
		cmn_err( CE_PANIC,
			"IMB MCA Host Adapter: Job too big for DMA list");

	/*
	 * If the data buffer was contiguous in physical memory,
	 * there was no need for a scatter/gather list; We're done.
	 */

	if ( i > 1 )	/* We need a scatter/gather list  */
	{

		srbp->s_addr = kvtophys( (caddr_t)scbp->s_dmaseg );
		srbp->s_size = i * sizeof( struct mcis_dma );
		scbp->s_opfld |= MCIS_PT;
	}
}

/*
** Function name: mcis_putq()
**
** Description:
**	Put a job on a logical unit queue.  Jobs are enqueued
**	on a priority basis.
*/

void
mcis_putq( q, srbp )
register struct mcis_luq	*q;
register struct mcis_srb	*srbp;
{
	register int	cls = MCIS_QCLASS( srbp );

	ASSERT( q );

#ifdef DEBUG
	if ( mcis_debug & MCIS_DB_ENT_EXT )
		printf( "mcis_putq( 0x%x, 0x%x )\n", q, srbp );
#endif

	/* 
	 * If queue is empty or queue class of job is less than
	 * that of the last one on the queue, tack on to the end.
	 */

	if ( ! q->q_first || cls <= MCIS_QCLASS( q->q_last ))
	{
		if ( q->q_first )
		{
			q->q_last->s_next = srbp;
			srbp->s_prev = q->q_last;
		}
		else
		{
			q->q_first = srbp;
			srbp->s_prev = NULL;
		}

		srbp->s_next = NULL;
		q->q_last = srbp;

	}
	else
	{
		register struct mcis_srb *next = q->q_first;

		while ( MCIS_QCLASS( next ) >= cls )
			next = next->s_next;

		srbp->s_next = next;
		srbp->s_prev = next->s_prev;

		if ( next->s_prev )
			next->s_prev->s_next = srbp;
		else
			q->q_first = srbp;

		next->s_prev = srbp;
	}

	q->q_count++;
}


/*
** Function name: mcis_next()
**
** Description:
**	Attempt to send the next job on the logical unit queue.
*/

void
mcis_next( q )
register struct mcis_luq	*q;
{
	register struct mcis_srb	*srbp;
	struct xsb			*sbp;
	unsigned long			scb_type;

	ASSERT( q );

#ifdef DEBUG
	if ( mcis_debug & MCIS_DB_ENT_EXT )
		printf( "mcis_next( 0x%x )\n", q );
#endif

	if (( srbp = q->q_first ) == NULL || q->q_flag & MCIS_QLUBUSY )
	{
#ifdef DEBUG
	if ( mcis_debug & MCIS_DB_MISC )
		printf( "mcis_next: q_flags = 0x%x sb_type = 0x%x\n",
					q->q_flag, srbp->s_sbp->sb.sb_type );
#endif
		return;
	}

	sbp = srbp->s_sbp;

	if ( scb_type = ( sbp->sb.sb_type != SFB_TYPE ))
		sbp->sb.SCB.sc_status = 0;

	q->q_flag |= MCIS_QLUBUSY;

	if ( ! ( q->q_first = srbp->s_next ))
		q->q_last = NULL;

	q->q_count--;
	q->q_scbp->s_srbp = srbp;
	
	if ( scb_type )
		mcis_scbcmd( srbp, q->q_scbp );
	else
		mcis_sfbcmd( srbp, q->q_scbp );

	if ( mcis_polltime )		/* need to poll */
	{
		register ulong	*comp_code;

		if ( scb_type )
			comp_code = &sbp->sb.SCB.sc_comp_code;
		else
			comp_code = &sbp->sb.SFB.sf_comp_code;

		*comp_code = SDI_ASW;

		if ( mcis_pollret( q->q_scbp->s_blkp ))
			*comp_code = SDI_TIME;

		q->q_flag &= ~MCIS_QLUBUSY;
		sdi_callback( &sbp->sb );
	}
}


/*
** Function name: mcis_sfbcmd()
** Description:
**	Create and send an SFB associated command. 
*/

void
mcis_sfbcmd( srbp, scbp )
register struct mcis_srb *srbp;
register struct mcis_scb *scbp;
{
	struct mcis_blk	*blkp = scbp->s_blkp;
	struct xsb	*sbp = srbp->s_sbp;

	if ( ! mcis_lds_assigned )
	{
		if ( scbp->s_status == MCIS_GOTLD )
		{
			scbp->s_status = 0;
		}
		else
		{
			/* check for assign error */

			if ( mcis_getld( blkp, scbp, 1 ) == MCIS_HBALD )
			{
				goto abort;
			}
			else if ( scbp->s_status == MCIS_WAITLD )
				return;
		}
	}

	scbp->s_cmdop = ISCMD_ABORT;
	scbp->s_cmdsup = ISCMD_ICMD;
 
	if ( mcis_docmd( blkp, scbp, scbp->s_ldmap->LD_CB.a_ld, ISATTN_ICMD ))
	{
abort:
		scbp->s_luq.q_flag &= ~MCIS_QLUBUSY;
		sbp->sb.SFB.sf_comp_code = SDI_ABORT;
		mcis_next( &scbp->s_luq );
		sdi_callback( &sbp->sb );
	}
}


/*
** Function name: mcis_flushq()
**
** Description:
**	Empty a logical unit queue.  If flag is set, remove all jobs.
**	Otherwise, remove only non-control jobs.
*/

void
mcis_flushq( q, cc, flag )
register struct mcis_luq	*q;
int			cc;
int			flag;
{
	register struct mcis_srb  *srbp;
	register struct mcis_srb  *nsrbp;

	ASSERT( q );

	srbp = q->q_first;
	q->q_first = q->q_last = NULL;
	q->q_count = 0;

	while ( srbp )
	{
		nsrbp = srbp->s_next;

		if ( ! flag && MCIS_QCLASS( srbp ) != SCB_TYPE )
			mcis_putq( q, srbp );
		else
		{
			srbp->s_sbp->sb.SCB.sc_comp_code = (ulong)cc;
			sdi_callback( &srbp->s_sbp->sb );
		}

		srbp = nsrbp;
	}
}

/*
 *	hba initialization routine
 */

int
mcis_hbainit(cntlnum)
int	cntlnum;
{
	register struct	mcis_blk	*blkp;
	struct	mcis_scb		*scbp;
	struct	mcis_scb		*nodev_scbp;
	int				first_time = TRUE;
	int				size;
	int				t;
	int				l;
	int				i;
	int				dma_listsize;
	struct mcis_dma			*dmalist;

	/* detect the hba location */

	if ( mcis_findhba( mcis_idata[cntlnum].ioaddr1 ))
		return 0;

	/* hba local configuration initialization */

	if (( blkp = mcis_cfginit(mcis_idata[cntlnum].ioaddr1, 
					mcis_idata[cntlnum].dmachan1 )) == NULL )
		return 0;

	/* get equipment description table - edt */

	mcis_getedt( blkp );

	if ( ! blkp->b_numdev )
	{
		kmem_free( (_VOID *)blkp->b_scbp, blkp->b_scballoc );
		blkp->b_scbp = NULL;
		kmem_free( (_VOID *)blkp, sizeof( struct mcis_blk ));
		blkp = NULL;
		return 0;
	} 

	/*
	** Now there are 128 mcis_scb's alloced, with every other one full
	** of inquiry data.  In general, there will usually be only a few
	** actual scsi devices attached to the bus.  To save space, I'm
	** going to allocate one per actual device and one extra to be
	** the nodev case.  The nodev case is only really needed for its
	** inquiry data.  I could probably do without it, but it would
	** further complicate the code.  My objective is to greatly reduce
	** the run time space with the least impact on the code.
	*/

	size = ( blkp->b_numdev + 1 ) * sizeof( struct mcis_scb );

	if (( scbp = (struct mcis_scb *)kmem_zalloc( size, (mod_dynamic ? KM_SLEEP : KM_NOSLEEP ))) == NULL ) {
		cmn_err( CE_WARN, "%s: Initialization error - cannot allocate scb's", mcis_idata[cntlnum].name );
		return 0;
	}
		
	nodev_scbp = scbp++;
	mcis_lds_assigned = ( blkp->b_ldcnt >= blkp->b_numdev );

	/*
	 * Allocate the dma scatter gather lists.  These must fall within
	 * a page, since none of the MAXDSEG chunks of lists can cross a
	 * physical page boundary.
	 */
	dma_listsize = blkp->b_numdev * MAXDSEG * sizeof(struct mcis_dma);
	for (i = 2; i < dma_listsize; i *= 2);
	dma_listsize = i;

#ifdef	DEBUG
	if(dma_listsize > PAGESIZE)
		cmn_err(CE_WARN, "%s: dmalist exceeds pagesize (may cross physical page boundary)\n", mcis_idata[0].name);
#endif /* DEBUG */

	if((dmalist = (struct mcis_dma *)kmem_zalloc(dma_listsize, mod_dynamic ? KM_SLEEP:KM_NOSLEEP)) == NULL) {
		cmn_err(CE_WARN, "%s: Initialization error allocating dmalist\n", mcis_idata[cntlnum].name);
		kmem_free(scbp, size );
		return 0;
	}

	for ( t = 0; t < MAX_TCS; t++ )
	{
		register int	offset;

		for ( l = 0; l < MAX_LUS; l++ )
		{
			offset = ( t << 3 ) | l;

			/* This is assigned a value in mcis_waitedt() */

			if ( blkp->b_scb_array[ offset ])
			{
				blkp->b_scb_array[ offset ] = scbp;
				bcopy( &blkp->b_scbp[ offset << 1 ], scbp,
						sizeof( struct mcis_scb ));

				bcopy( &blkp->b_scbp[( offset << 1 ) + 1 ],
					&scbp->s_ident, sizeof( struct ident ));

				/*
				** These need to be re-initialized since
				** the location of the structs is changing
				*/

				scbp->s_luq.q_scbp = scbp;
				scbp->s_tsbp = kvtophys( (caddr_t)( &scbp->s_tsb ));

				if ( mcis_lds_assigned
					&& mcis_getld( blkp, scbp, 0 ) != MCIS_HBALD )
				{
					scbp->s_status = MCIS_OWNLD;
				}

				scbp->s_dmaseg = dmalist;
				dmalist += MAXDSEG;

				scbp++;
			}
			else
			{
				if ( first_time )
				{
					bcopy( &blkp->b_scbp[ offset << 1 ],
						nodev_scbp,
						sizeof( struct mcis_scb ));

					bcopy( &blkp->b_scbp[( offset << 1 )+ 1 ],
						&nodev_scbp->s_ident,
						sizeof( struct ident ));

					/* 
					** id_type may be ID_NODEV
					*/

					nodev_scbp->s_ident.id_type = ID_NODEV;
					first_time = FALSE;
				}

				blkp->b_scb_array[ offset ] = nodev_scbp;
			}
		}

	}

	kmem_free( (_VOID *)blkp->b_scbp, blkp->b_scballoc );
	blkp->b_scbp = NULL;
	blkp->b_scballoc = 0;

	mcis_ha[cntlnum] = blkp;
	return 1;
}


/*
 *	Adapter detection routine
 */

int
mcis_findhba( ioaddr )
register uint	ioaddr;
{
	register int	i;
	char		 status;
	char	 	*sp = &status;
	int		ctrl_addr = ioaddr + MCIS_CTRL;

	/* hardware reset */

	outb( ctrl_addr, inb( ctrl_addr ) & ISCTRL_RESERVE | ISCTRL_RESET );

	/* According to ref man:
	   must wait for at least 50ms and then turn off controller reset */

	for ( i = 6; i; i-- )
		tenmicrosec();

	outb( ctrl_addr, inb( ctrl_addr ) & ISCTRL_RESERVE );

	/* check busy in status reg  */

	if ( MCIS_BUSYWAIT( ioaddr ))
		return 1;

	/* wait for interrupt */

	if ( MCIS_INTRWAIT( ioaddr ))
		return 1;
	
	status = inb( ioaddr + MCIS_INTR );
	MCIS_SENDEOI( ioaddr, MCIS_HBALD );

	if ( MCIS_rintrp(sp)->ri_code ) 
		return 1;

	/* enable dma channel */

	outb( ctrl_addr, inb( ctrl_addr ) & ISCTRL_RESERVE | ISCTRL_EDMA );

	status = inb( ctrl_addr );

	cmn_err(CE_NOTE, "MCA SCSI Host Adapter found at address 0x%X\n", ioaddr);

#ifdef MCIS_MLDEBUG2
	printf( "mcis_findhba: returning\n" );
#endif
	return 0;
}


/*
 *	adapter local structures configuration routine
 */

struct  mcis_blk *
mcis_cfginit( ioaddr, dmachan )
uint 	ioaddr;
uint	dmachan;
{
	struct mcis_blk	*blkp;
	struct mcis_pos	*posp;
	int		i;
	void mcis_fccmd();

#ifdef MCIS_MLDEBUG2
	printf( "mcis_cfginit: entry\n" );
#endif

	/* allocate memory for initialization */

	blkp = (struct mcis_blk *)kmem_zalloc( sizeof( struct mcis_blk ),
				(mod_dynamic ? KM_SLEEP : KM_NOSLEEP) );

	if ( blkp == NULL )
		goto clean1;

	blkp->b_ioaddr = ioaddr;
	blkp->b_dmachan = dmachan;
	blkp->b_scb_cnt = 128;
	blkp->b_scballoc = 128 * sizeof( struct mcis_scb );
	blkp->b_scbp = (struct mcis_scb *)kmem_zalloc( blkp->b_scballoc,
				 (mod_dynamic ? KM_SLEEP : KM_NOSLEEP) );

	if ( blkp->b_scbp == NULL )
		goto clean2;

	/* scsi bus reset */

	if ( mcis_docmd( blkp, (( ISCMD_ICMD << 8 ) | ISCMD_RESET ), 
						MCIS_HBALD, ISATTN_ICMD)
		|| mcis_pollret( blkp )
		|| blkp->b_intr_code != ISINTR_ICMD_OK )
	{
#ifdef MCIS_MLDEBUG
		printf( "mcis_cfginit: fail on scsi reset retcod = 0x%x\n",
			blkp->b_intr_code );
#endif
		goto clean3;
	}

	/* get POS information */

	posp = (struct mcis_pos *)( blkp->b_scbp + 1 );
	mcis_poscmd( blkp->b_scbp, posp );

	if ( mcis_docmd( blkp, blkp->b_scbp, MCIS_HBALD, ISATTN_SCB )
		|| mcis_pollret( blkp )
		|| ( blkp->b_intr_code != ISINTR_SCB_OK  &&
		     blkp->b_intr_code != ISINTR_SCB_OK2 ))
	{
#ifdef MCIS_MLDEBUG
		printf( "mcis_cfginit: fail on getpos retcod = 0x%x\n",
						blkp->b_intr_code );
#endif
		goto clean3;
	}

	/* Issue a feature control command to set HBA global timeout */

	mcis_fccmd( (struct fcscb *)blkp->b_scbp );

	if ( mcis_docmd( blkp, *((int *)(blkp->b_scbp)),
						MCIS_HBALD, ISATTN_ICMD)
		|| mcis_pollret( blkp )
		|| blkp->b_intr_code != ISINTR_ICMD_OK )
	{
		cmn_err( CE_WARN,
		   "!IBM MCA Host Adapter: Feature Control command failed\n");
#ifdef MCIS_MLDEBUG
		printf( "mcis_cfginit: fail on feature control cmd, retcod = 0x%x\n",
						blkp->b_intr_code );
#endif
	}

	/* someday, someone might have to set intr vect at runtime */

	blkp->b_intr = posp->p_intr;
	blkp->b_targetid = posp->p3_targid;

	/* calculate available logical device */

	if ( posp->p_ldcnt < MCIS_MAXLD )
		blkp->b_ldcnt = posp->p_ldcnt - 1;
	else
		blkp->b_ldcnt = MCIS_MAXLD - 1;

#ifdef MCIS_MLDEBUG2
	printf( "mcis_cfginit: mcis_blkp = 0x%x scbp = 0x%x\n", 
						blkp, blkp->b_scbp );

	printf( "mcis_cfginit: hbaid = 0x%x targetid = %d\n", 
					posp->p_hbaid, blkp->b_targetid );

	printf( "dma = 0x%x fair = %d ioaddr = 0x%x romseg = 0x%x\n",
		posp->p3_dma, posp->p3_fair, posp->p2_ioaddr, posp->p2_romseg );

	printf( "intr = 0x%x pos4 = %d slotsz = 0x%x ldcnt = 0x%x\n",
		posp->p_intr, posp->p_pos4, posp->p_slotsz, posp->p_ldcnt );

	printf( "pacing = 0x%x tmeoi = 0x%x tmreset = 0x%x cache = 0x%x\n",
		posp->p_pacing, posp->p_tmeoi, posp->p_tmreset, posp->p_cache );
#endif

	/* initialize logical device manager */

	mcis_initld( blkp );

	return blkp;

clean3: kmem_free( (_VOID *)blkp->b_scbp, blkp->b_scballoc );	
		blkp->b_scbp = NULL;
clean2: kmem_free( (_VOID *)blkp, sizeof( struct mcis_blk ));
		blkp = NULL;
clean1: printf( "mcis_cfginit: unable to allocate memory\n" );
	return NULL;
}

/*
 *	Adapter get equipment description table routine
 *	inquiry commands are single-threaded through all possible scsi devices
 */

void
mcis_getedt( blkp )
register struct	mcis_blk *blkp;
{
	register struct mcis_scb	*scbp;
	struct ident			*ident;
	register int			targ;
	int				lun;
	int				ld;
	int				ctrl_addr = blkp->b_ioaddr + MCIS_CTRL;

	/* check the complete scsi bus */

	for ( lun = 0; lun < MAX_LUS; lun++ )
		for ( targ = 0; targ < MAX_TCS; targ++ )
		{
#ifdef MCIS_MLDEBUG2
			printf( "mcis_getedt: < %d, %d > \n", targ, lun );
#endif
			scbp = (struct mcis_scb *)(&blkp->b_scbp[
						( targ << 3 | lun ) << 1 ]);

			scbp->s_luq.q_scbp = scbp;
			scbp->s_blkp = blkp;
			scbp->s_targ = targ;
			scbp->s_lun  = lun;
			scbp->s_tsbp = kvtophys( (caddr_t)( &scbp->s_tsb ));

			if ( targ == blkp->b_targetid )
				continue;

			ident = (struct ident *)(scbp + 1);
			mcis_inqcmd( scbp, ident );
			ident->id_type = ID_NODEV;

			/* get logical device with no wait */

			ld = mcis_getld( blkp, scbp, 1 );

			/* check for assign error */

			if ( ld == MCIS_HBALD )
				continue;

			/* check for available logical device */

			if ( ld != -1 )
			{
				if ( mcis_docmd( blkp, scbp, ld, ISATTN_SCB))
				{
					mcis_freeld( blkp, scbp->s_ldmap );
					continue;
				}
			}

			/* counter for outstanding inquiry command */

			blkp->b_inqdout++;

			/* wait for outstanding inquiry command to arrive */

			while ( blkp->b_inqdout )
				mcis_waitedt( blkp );
		}

	if ( ! blkp->b_numdev ) 
		return;

	/* invlidate logical devices assignment */

	mcis_invld( blkp );

	/* enable dma */

	outb( ctrl_addr, inb( ctrl_addr ) & ISCTRL_RESERVE
						| ISCTRL_EDMA );
}


/*
 *	wait for edt information
 */

void
mcis_waitedt( blkp )
register struct	mcis_blk *blkp;
{
	register struct	mcis_scb	*scbp;
	register struct	ident		*ident;
	struct mcis_ldevmap		*ldp;

	if ( mcis_pollret( blkp )) 
		return;

	scbp = blkp->b_ldm[ blkp->b_intr_dev ].ld_scbp;
	ident = (struct ident *)(scbp + 1);

	/* check for outstanding logical device assignment completion */

	if ( blkp->b_intr_dev == MCIS_HBALD )
	{
#ifdef MCIS_MLDEBUG2
		printf( "mcis_waitedt: ASGN retcod = 0x%x\n",
						blkp->b_intr_code );
#endif
		blkp->b_ldm[ MCIS_HBALD ].ld_scbp = NULL;
		scbp->s_status = 0;

		if ( blkp->b_intr_code == ISINTR_ICMD_OK
			&& ! mcis_docmd( blkp, scbp,
				scbp->s_ldmap->LD_CB.a_ld, ISATTN_SCB ))
			return;

		/* check for inquiry command return status */

	}
	else if ( blkp->b_intr_code != ISINTR_SCB_OK
			&&  blkp->b_intr_code != ISINTR_SCB_OK2 )
	{
		ident->id_type = ID_NODEV;

#ifdef MCIS_MLDEBUG2
		ldp = &blkp->b_ldm[ blkp->b_intr_dev ];

		printf( "mcis_waitedt: INQCMD retcod = 0x%x on ld = %d < %d, %d >\n",
			blkp->b_intr_code, blkp->b_intr_dev, 
			ldp->LD_CB.a_targ, ldp->LD_CB.a_lun );
#endif
	/* increment device count for any new devices responded */
	}
	else if ( ident->id_type != ID_NODEV )
	{
		blkp->b_numdev++;

		/*
		** This is added to support the change to reduce the space
		** required during run time.  A non-NULL value in b_scb_array
		** indicates <t:l> is a real device.  This value is checked
		** in mcis_hbainit().
		*/

		ldp = &blkp->b_ldm[ blkp->b_intr_dev ];
		blkp->b_scb_array[( ldp->LD_CB.a_targ << 3 )
			| ldp->LD_CB.a_lun ] = (struct mcis_scb *)1;

#ifdef MCIS_MLDEBUG2
		printf( "mcis_waitedt: <targ,lun> = < %d, %d > dev = 0x%x\n",
			ldp->LD_CB.a_targ, ldp->LD_CB.a_lun, ident->id_type );
#endif
	}

	blkp->b_inqdout--;
	mcis_freeld( blkp, scbp->s_ldmap );
}

/*
 *	invalidate all logical device mapping	
 */

void
mcis_invld( blkp )
register struct mcis_blk *blkp;
{
	register struct	mcis_ldevmap	*ldp;
	register struct	mcis_scb	*scbp;
	struct ident			*ident;

#ifdef MCIS_MLDEBUG2
	printf( "mcis_invld: entry\n" );
#endif

	/* search through the logical device pool */

	ldp = LDMHD_FW( blkp );

	for (; ldp != &(blkp->b_ldmhd); ldp = ldp->ld_avfw )
	{
		scbp = ldp->ld_scbp;
		ident = (struct ident *)(scbp + 1);

		if ( ident->id_type == ID_NODEV )
			continue;

		/* remove the previous assignment */

		ldp->LD_CB.a_rm = 1;		

		if ( mcis_docmd( blkp, ldp->LD_CMD, MCIS_HBALD, ISATTN_ICMD)
			|| mcis_pollret( blkp )
			|| blkp->b_intr_code != ISINTR_ICMD_OK)
		{
#ifdef MCIS_MLDEBUG
			printf( "mcis_invld: fail = 0x%x retcod = 0x%x\n",
					ldp->LD_CMD, blkp->b_intr_code );
#endif
		}

		ldp->LD_CB.a_rm = 0;		
		scbp->s_ldmap = NULL;
	}
}


/*
 *	initialize the logical device map
 */

void
mcis_initld( blkp )
struct mcis_blk *blkp;
{
	register struct	mcis_ldevmap	*dp;
	register struct	mcis_ldevmap	*lp;
	register struct	mcis_scb	*scbp;
	int				i;
	int				maxld;
	int				targ;

	dp = &(blkp->b_ldmhd);
	dp->ld_avfw = dp->ld_avbk = dp;
	maxld = blkp->b_ldcnt;

	lp = blkp->b_ldm;

	for ( targ = 0, i = 0; i < maxld; i++, lp++ )
	{
		lp->ld_avbk = dp;
		lp->ld_avfw = dp->ld_avfw;
		dp->ld_avfw->ld_avbk = lp;
		dp->ld_avfw = lp;
		lp->LD_CMD  = 0;
		lp->LD_CB.a_cmdop = (ushort)((ISCMD_ICMD << 8) | ISCMD_ASSIGN);
		lp->LD_CB.a_ld = i;

		/* targets 0 to 6 has been assigned by the firmware */

		if ( targ >= 8 )
			continue;

		if ( targ != blkp->b_targetid )
		{
			scbp = &blkp->b_scbp[ targ << 4 ];
			lp->LD_CB.a_targ = targ;
			lp->LD_CB.a_lun  = 0;
			lp->ld_scbp = scbp;
			scbp->s_ldmap = lp;
		}

		targ++;
	}
}

/*
 *	setup an inquiry command
 */

void
mcis_inqcmd( scbp, hostaddr )
struct mcis_scb	*scbp;
int		hostaddr;
{

	scbp->s_cmdop = ISCMD_DEVINQ;
	scbp->s_cmdsup = ISCMD_SCB;
	scbp->s_opfld = MCIS_BB | MCIS_SS | MCIS_RE | MCIS_ES | MCIS_RD;
	scbp->s_hostaddr = kvtophys( (caddr_t)hostaddr );
	scbp->s_hostcnt = sizeof( struct ident );
	scbp->s_tsbp = kvtophys( (caddr_t)( &scbp->s_tsb ));
}

/*
 *	setup a pos command
 */

void
mcis_poscmd( scbp, hostaddr )
struct mcis_scb	*scbp;
int		hostaddr;
{
	scbp->s_cmdop = ISCMD_GETPOS;
	scbp->s_cmdsup = ISCMD_SCB;
	scbp->s_opfld = MCIS_BB | MCIS_RE | MCIS_ES | MCIS_RD;
	scbp->s_hostaddr = kvtophys( (caddr_t)hostaddr );
	scbp->s_hostcnt = sizeof( struct mcis_pos );
	scbp->s_tsbp = kvtophys( (caddr_t)( &scbp->s_tsb ));
}

/*
 *	setup a feature control command
 */

void
mcis_fccmd( fcscbp )
struct mcis_fcscb	*fcscbp;
{
	fcscbp->fc_cmdop = ISCMD_HBACTRL;
	fcscbp->fc_cmdsup = ISCMD_ICMD;
	fcscbp->fc_gtv = mcis_global_tout;
	fcscbp->fc_drate = 0;
}

/*
 *	adapter command interface routine
 */

int
mcis_docmd( blkp, cmd, dev, opcode )
struct mcis_blk	*blkp;
int		cmd;
unchar		dev;
unchar		opcode;
{
	register ushort		ioaddr;
	register int		i;
	register paddr_t	outcmd;
	int			oldspl;

	ioaddr = blkp->b_ioaddr;
	outcmd = cmd;

	if ( opcode != ISATTN_ICMD )
		outcmd = kvtophys( outcmd );

/*	check busy in status reg 					*/

	if ( MCIS_CMDOUTWAIT( ioaddr ))
		return 1;

	oldspl = spl7();

	for ( i=0; i < 4 ; i++ )
	{
		outb( ioaddr + MCIS_CMD + i, ( outcmd & 0xff ));
		outcmd >>= 8;
	}

	outb( ioaddr + MCIS_ATTN, opcode | dev );
	blkp->b_npend++;
	splx( oldspl );

#ifdef MCIS_MLDEBUG2
	printf( "mcis_docmd: cmd= 0x%x attn= 0x%x\n", cmd, opcode | dev );
#endif

	return 0;
}

/*
 *	wait for interrupt return by polling
 */

int
mcis_pollret( mcis_blkp )
register struct	mcis_blk *mcis_blkp;
{
	register ushort	ioaddr;
	char		status;
	char		*sp = &status;

	ioaddr = mcis_blkp->b_ioaddr;

/*	wait for interrupt						*/

	if ( MCIS_INTRWAIT( ioaddr ))
		return 1;
	
	status = inb( ioaddr + MCIS_INTR );
	mcis_blkp->b_intr_code = MCIS_rintrp( sp )->ri_code;
	mcis_blkp->b_intr_dev  = MCIS_rintrp( sp )->ri_ldevid;

	if ( MCIS_BUSYWAIT( ioaddr ))
		return 1;

	MCIS_SENDEOI( ioaddr, mcis_blkp->b_intr_dev );
	mcis_blkp->b_npend--;

#ifdef MCIS_MLDEBUG2
	printf( "mcis_pollret: icode= 0x%x idev= 0x%x\n",
			mcis_blkp->b_intr_code, mcis_blkp->b_intr_dev );
#endif

	return 0;
}

/*
 *	generic adapter ready wait routine
 */

int
mcis_wait( ioaddr, mask, onbits, offbits )
ushort	ioaddr;
ushort	mask;
ushort	onbits;
ushort	offbits;
{
	register ushort	port;
	register int	i;
	register ushort	maskval; 

	port = ioaddr + MCIS_STAT;

	for ( i = 3000000; i; i-- )
	{
		maskval = inb( port ) & mask;

		if (( maskval & onbits ) == onbits
				&& ( maskval & offbits ) == 0 )
			return 0;

		tenmicrosec();
	}

#ifdef MCIS_MLDEBUG
	printf( "mcis_wait: ERROR on = 0x%x off = 0x%x\n", onbits, offbits );
#endif

	return 1;
}

/*
 *	Get logical device mapping routine
 */

int
mcis_getld( mcis_blkp, mcis_scbp, nowait )
register struct	mcis_blk	*mcis_blkp;
register struct	mcis_scb	*mcis_scbp;
int				nowait;
{
	register struct	mcis_ldevmap	*ldp;
	int				op;
	int				avld = -1;

	op = spl5();
	ldp = mcis_scbp->s_ldmap;

	/* check for valid assignement */

	if ( ldp && ( ldp->LD_CB.a_targ == mcis_scbp->s_targ &&
		ldp->LD_CB.a_lun == mcis_scbp->s_lun ))
	{
		avld = ldp->LD_CB.a_ld;
	}
	else
	{
		/* allocate a new logical device number */

		ldp = LDMHD_FW( mcis_blkp );

		/* check for empty list */

		if ( ldp == &(mcis_blkp->b_ldmhd) )
		{
#ifdef MCIS_MLDEBUG
			printf( "mcis_getld: empty ld list\n" );
#endif
			splx( op );
			return MCIS_HBALD;
		}
	}

	ldp->ld_avbk->ld_avfw = ldp->ld_avfw;
	ldp->ld_avfw->ld_avbk = ldp->ld_avbk;
	splx( op );
	ldp->ld_avbk = NULL;
	ldp->ld_avfw = NULL;
	ldp->ld_scbp = mcis_scbp;

	if ( avld != -1 )
		return avld;

	ldp->LD_CB.a_targ = mcis_scbp->s_targ;
	ldp->LD_CB.a_lun = mcis_scbp->s_lun;

	if ( mcis_docmd( mcis_blkp, ldp->LD_CMD, MCIS_HBALD, ISATTN_ICMD ))
	{
		mcis_freeld( mcis_blkp, ldp );
		return MCIS_HBALD;
	}

	/* set the ld ptr to me */

	mcis_scbp->s_ldmap = ldp;

	if ( nowait )
	{
		mcis_scbp->s_status = MCIS_WAITLD;
		mcis_blkp->b_ldm[ MCIS_HBALD ].ld_scbp = mcis_scbp;
		return avld;
	}

	/* wait for return status if specified - in device initialization */

	if ( mcis_pollret( mcis_blkp )
		|| mcis_blkp->b_intr_code != ISINTR_ICMD_OK )
	{
#ifdef MCIS_MLDEBUG
		printf( "mcis_getld: fail = 0x%x retcod = 0x%x\n",
					ldp->LD_CMD, mcis_blkp->b_intr_code );
#endif
		mcis_freeld( mcis_blkp, mcis_scbp->s_ldmap );
		mcis_scbp->s_ldmap = NULL;
		return MCIS_HBALD;
	}
	
	return ldp->LD_CB.a_ld;
}


/*
 *	free logical device mapping
 */

void
mcis_freeld( mcis_blkp, ldp )
register struct	mcis_blk	*mcis_blkp;
register struct	mcis_ldevmap	*ldp;
{
	register struct	mcis_ldevmap	**ldmhdp;
	int				op;

#ifdef MCIS_MLDEBUG2
	printf( "mcis_freeld: entry\n" );
#endif
	/* relink back to the logical device map pool */

	op = spl5();
	ldmhdp = &(mcis_blkp->b_ldmhd.ld_avbk);
	(*ldmhdp)->ld_avfw = ldp;
	ldp->ld_avbk = *ldmhdp;
	*ldmhdp = ldp;
	ldp->ld_avfw = &(mcis_blkp->b_ldmhd);
	splx( op );
}

/*
** Function name: mcis_pass_thru()
**
** Description:
**	Send a pass-thru job to the HA board.
*/

void
mcis_pass_thru( bp )
struct buf *bp;
{
	register struct mcis_luq	*q;
	register struct sb		*sp;
	struct proc			*procp;
	int				minor = getminor( bp->b_dev );
	int				c = mcis_gtol[ MCIS_HAN( minor )];
	int				t = mcis_tcn_xlat[ MCIS_TCN( minor ) ];
	int				l = MCIS_LUN( minor );
	int				opri;

	sp = (struct sb *)bp->b_resid;

#ifdef MCIS_DEBUG
	if ( sp->SCB.sc_wd != (long)bp )
		cmn_err( CE_PANIC, "IBM MCA Host Adapter: Corrupted address from physio");
#endif

	sp->SCB.sc_dev.sa_lun = l;
	sp->SCB.sc_dev.sa_fill = ( mcis_ltog[ c ] << 3 ) | mcis_tcn_xlat[ t ];
	sp->SCB.sc_datapt = (caddr_t)paddr( bp );
	sp->SCB.sc_int = mcis_int;

	drv_getparm( UPROCP, (ulong*)&procp );
	sdi_translate( sp, bp->b_flags, procp );

	q = MCIS_LU_Q( c, t, l );

	opri = spl5();
	mcis_putq( q, (struct mcis_srb *)((struct xsb *)sp)->hbadata_p );
	mcis_next( q );
	splx( opri );
}

/*
** Function name: mcis_int()
**
** Description:
**	This is the interrupt handler for pass-thru jobs.  It just
**	wakes up the sleeping process.
*/

void
mcis_int( sp )
struct sb *sp;
{
	struct buf *bp;

#ifdef MCIS_DEBUG
	if ( mcis_debug > 0 )
		printf( "adsc_int: sp = %x\n",sp );
#endif

	bp = (struct buf *)sp->SCB.sc_wd;
	biodone( bp );
}
