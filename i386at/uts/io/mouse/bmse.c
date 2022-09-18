/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/mouse/bmse.c	1.6"
#ident	"$Header: $"

/*
 * Serial Mouse Module - Streams
 */

#include <util/param.h>
#include <util/types.h>
#include <mem/kmem.h>
#include <proc/signal.h>
#include <svc/errno.h>
#include <fs/file.h>
#include <io/termio.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strtty.h>
#include <util/debug.h>
#include <io/ddi.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <util/cmn_err.h>
#include <io/ws/chan.h>
#include <io/mouse.h>
#include <io/mouse/mse.h>

#include	<util/mod/moddefs.h>

#define	DRVNAME	"bmse - Loadable bus mouse driver"

STATIC	int	bmse_load(), bmse_unload();

MOD_DRV_WRAPPER(bmse, bmse_load, bmse_unload, NULL, DRVNAME);

/*
 * Wrapper functions.
 */

void	bmseinit();

STATIC	int
bmse_load(void)
{
	cmn_err(CE_NOTE, "!MOD: in bmse_load()");

	bmseinit();
	mod_drvattach(&bmse_attach_info);
	return(0);
}

STATIC	int
bmse_unload(void)
{
	cmn_err(CE_NOTE, "!MOD: in bmse_unload()");

	mod_drvdetach(&bmse_attach_info);
	return(0);
}

int		bmsedevflag = 0;
char		bmseclosing = 0;
char		bmseInPort = 0;
void		(*bmsegetdata)();
extern void	bmseInPortData();
extern void	bmseLogitechData();
static unsigned	BASE_IOA;	/* Set to base I/O addr of bus mouse */

static struct strmseinfo	*bmseptr = 0;

extern	void	mse_copyin(), mse_copyout();
extern	void	mse_iocnack(), mse_iocack();
extern	void	mseproc();

extern int	mse_nbus;

extern struct mouseconfig	mse_config;

int	bmseopen(), bmseclose(), bmse_rsrv(), bmse_wput();
void	bmseintr();

struct module_info	bmse_info = { 23, "bmse", 0, INFPSZ, 256, 128 };

static struct qinit	bmse_rinit = {
	NULL, NULL, bmseopen, bmseclose, NULL, &bmse_info, NULL };

static struct qinit	bmse_winit = {
	bmse_wput, NULL, NULL, NULL, NULL, &bmse_info, NULL };

struct streamtab 	bmseinfo = { &bmse_rinit, &bmse_winit, NULL, NULL };

void
bmseinit()
{
	register int	i;

	mse_config.present = 0;

	if ( mse_nbus )
	{
		BASE_IOA = mse_config.io_addr;

		/*
		** Determine what type of mouse board is installed.
		*/

		i = inb( IDENTREG );

		if ( i == SIGN )	/* Microsoft InPort Board */
		{
			outb( ADDRREG, RESET );	/* reset chip */
			bmseInPort = 1;
			bmsegetdata = bmseInPortData;
			mse_config.present = 1;
		}
		else	/* Logitech Board */
		{
			/* Check if the mouse board exists */
			outb( CONFIGURATOR_PORT, 0x91 );
			tenmicrosec();
			outb( SIGNATURE_PORT, 0xC) ;
			tenmicrosec();
			i = inb( SIGNATURE_PORT );
			tenmicrosec();
			outb( SIGNATURE_PORT, 0x50 );
			tenmicrosec();
			if ( i == 0xC && inb (SIGNATURE_PORT) == 0x50 )
			{
				mse_config.present = 1;
				bmsegetdata = bmseLogitechData;
				control_port( INTR_DISABLE );
			}
		}
	}
}

int
bmseopen( q, devp, flag, sflag, cred_p )
queue_t		*q;
dev_t		*devp;
register int	flag;
register int	sflag;
struct cred	*cred_p;
{
	register int	oldpri;

#ifdef MOUSEDEBUG1
	printf("bmseopen:entered\n");
#endif

	if ( ! mse_config.present )
		return EIO;

	if ( q->q_ptr != NULL )
	{
#ifdef MOUSEDEBUG1
		printf("bmseopen:already open\n");
#endif
		return 0;		/* already attached */
	}

	oldpri = splstr();

	while ( bmseclosing )
		sleep( &bmse_info, PZERO + 1 );

	splx( oldpri );

	/* allocate and initialize state structure */

	if (( bmseptr = (struct strmseinfo *) kmem_zalloc(
			 sizeof( struct strmseinfo ), KM_SLEEP )) == NULL )
	{
		cmn_err( CE_WARN,
			 "BMSE: open fails, can't allocate state structure");
		return ENOMEM;
	}
	q->q_ptr = (caddr_t) bmseptr;
	WR( q )->q_ptr = (caddr_t) bmseptr;
	bmseptr->rqp = q;
	bmseptr->wqp = WR( q );

	if ( bmseInPort )
	{
		outb( ADDRREG, MODE );	/* select mode register */
		outb( DATAREG, QUADMODE | DATAINT | HZ30 );
	}
	else
	{
		control_port( INTR_ENABLE );
	}

#ifdef MOUSEDEBUG1
	printf("bmseopen:leaving\n");
#endif
	return 0;
}


int
bmseclose( q, flag, cred_p )
queue_t		*q;
register int	flag;
struct cred	*cred_p;
{
	register int	oldpri;

#ifdef MOUSEDEBUG
	printf("bmseclose:entered\n");
#endif
	if ( bmseInPort )
	{
		outb( ADDRREG, MODE );	/* select mode register */
		outb( DATAREG, 0 );
	}
	else
	{
		control_port( INTR_DISABLE );	/* Disable interrupts */
	}

	oldpri = splstr();
	bmseclosing = 1;
	q->q_ptr = (caddr_t) NULL;
	WR(q)->q_ptr = (caddr_t) NULL;
	kmem_free( bmseptr, sizeof( struct strmseinfo ));
	bmseptr = (struct strmseinfo *) NULL;
	bmseclosing = 0;
	wakeup( &bmse_info );
	splx( oldpri );

#ifdef MOUSEDEBUG
	printf("bmseclose:leaving\n");
#endif
	return;
}

int
bmse_wput( q, mp )
queue_t	*q;
mblk_t	*mp;
{
	register struct iocblk	*iocbp;
	register mblk_t		*bp;
	register struct copyreq	*cqp;
	register struct copyresp *csp;
	register int		oldpri;

#ifdef MOUSEDEBUG
	printf("bmse_wput:entered\n");
#endif
	if ( bmseptr == 0 )
	{
		freemsg( mp );
#ifdef MOUSEDEBUG
	printf("bmse_wput:bmseptr == NULL\n");
#endif
		return;
	}

	iocbp = (struct iocblk *) mp->b_rptr;

	switch ( mp->b_datap->db_type )
	{
		case M_FLUSH:
#ifdef MOUSEDEBUG
			printf("bmse_wput:M_FLUSH\n");
#endif
			if ( *mp->b_rptr & FLUSHW )
				flushq( q, FLUSHDATA );

			qreply( q, mp );
			break;

		case M_IOCTL:
#ifdef MOUSEDEBUG
			printf("bmse_wput:M_IOCTL\n");
#endif
			switch( iocbp->ioc_cmd )
			{
				case MOUSEIOCREAD:
#ifdef MOUSEDEBUG
					printf("bmse_wput:M_IOCTL-MOUSEIOCREAD\n");
#endif

					if (( bp = allocb( sizeof( struct mouseinfo ), BPRI_MED )) == NULL )
					{ 
						mse_iocnack( q, mp, iocbp, EAGAIN, 0 );
						break;
					}

					oldpri = splstr();
					bcopy( &bmseptr->mseinfo, bp->b_rptr, sizeof( struct mouseinfo ));
					bmseptr->mseinfo.xmotion = bmseptr->mseinfo.ymotion = 0;
					bmseptr->mseinfo.status &= BUTSTATMASK;
					splx( oldpri );
					bp->b_wptr += sizeof( struct mouseinfo );
					if ( iocbp->ioc_count == TRANSPARENT) 
						mse_copyout( q, mp, bp, sizeof( struct mouseinfo ), 0 );
					else
					{
#ifdef MOUSEDEBUG
						printf("bmse_wput:M_IOCTL- not transparent\n");
#endif
						mp->b_datap->db_type = M_IOCACK;
						iocbp->ioc_count = sizeof( struct mouseinfo );
						if ( mp->b_cont )
							freemsg( mp->b_cont );

						mp->b_cont = bp;
						qreply( q, mp );
					}

					break;

				default:
#ifdef MOUSEDEBUG
					printf("bmse_wput:M_IOCTL-DEFAULT\n");
#endif
					mse_iocnack( q, mp, iocbp, EINVAL, 0 );
					break;
			}

			break;

		case M_IOCDATA:
#ifdef MOUSEDEBUG
			printf("bmse_wput:M_IOCDATA\n");
#endif
			csp = (struct copyresp *)mp->b_rptr;
			if ( csp->cp_cmd != MOUSEIOCREAD )
			{
#ifdef MOUSEDEBUG
				printf("bmse_wput:M_IOCDATA - NACKing\n");
#endif
				mse_iocnack( q, mp, iocbp, EINVAL, 0 );
				break;
			}

			if ( csp->cp_rval )
			{
#ifdef MOUSEDEBUG
				printf("bmse_wput:M_IOCDATA - freemsging\n");
#endif
				freemsg( mp );
				break;
			}
#ifdef MOUSEDEBUG
			printf("bmse_wput:M_IOCDATA - ACKing\n");
#endif
			mse_iocack( q, mp, iocbp, 0 );
			break;

		default:

			freemsg( mp );
			break;
	}

#ifdef MOUSEDEBUG
	printf("bmse_wput:leaving\n");
#endif
}

void
bmseintr( vect )
int	vect;
{
	if ( mse_config.ivect != vect )
	{
		cmn_err( CE_WARN,
			"Mouse interrupt on un-configured vector: %d", vect );
		return;
	}

	if ( ! bmseptr )
	{
#ifdef MOUSEDEBUG
		cmn_err(CE_NOTE,"received interrupt before opened");
#endif
		if ( bmseInPort )
		{
			outb( ADDRREG, MODE );	/* select mode register */
			outb( DATAREG, QUADMODE | DATAINT | HZ30 );
		}
		else
		{
			control_port( INTR_ENABLE );
		}

		return;
	}

	(*bmsegetdata)();
}

void
bmseInPortData()
{
	register unsigned	BASE_IOA = mse_config.io_addr;
	register unchar		d;


	outb( ADDRREG, MODE );
	outb( DATAREG, QUADMODE | HZ30 | HOLD );

	outb( ADDRREG, MSTATUS );
	d = inb( DATAREG );	/* read mouse status byte */
	bmseptr->button = ~d & BUTMASK;

	if ( d & MSEMOTION )
	{
		outb( ADDRREG, XMOTION );
		bmseptr->x = inb( DATAREG );
		outb( ADDRREG, YMOTION );
		bmseptr->y = inb( DATAREG );
	}
	else
	{
		bmseptr->x = bmseptr->y = 0;
	}

	mseproc( bmseptr );

	/* Re-enable interrupts on the mouse and return */

	outb( ADDRREG, MODE );	/* select mode register */
	outb( DATAREG, QUADMODE | DATAINT | HZ30 );
}

void
bmseLogitechData()
{
	register unsigned	BASE_IOA = mse_config.io_addr;
	register unchar		d;


	/*
	**  Get the mouse's status and put it into the
	** appropriate virtual structure
	*/

	control_port( INTR_DISABLE | HC | HIGH_NIBBLE | X_COUNTER );
	bmseptr->x = ( data_port & 0x0f ) << 4;
	control_port( INTR_DISABLE | HC | LOW_NIBBLE | X_COUNTER );
	bmseptr->x |= ( data_port & 0x0f );
	control_port( INTR_DISABLE | HC | HIGH_NIBBLE | Y_COUNTER );
	bmseptr->y = ( data_port & 0x0f ) << 4;
	control_port( INTR_DISABLE | HC | LOW_NIBBLE | Y_COUNTER );
	bmseptr->y |= (( d = data_port ) & 0x0f );
	bmseptr->button = ( d >> 5 ) & 0x07;

	mseproc( bmseptr );

	/* Re-enable interrupts on the mouse and return */

	control_port( INTR_ENABLE );
}
