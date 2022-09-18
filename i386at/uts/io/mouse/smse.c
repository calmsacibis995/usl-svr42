/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/mouse/smse.c	1.13"
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
#include <proc/cred.h>
#include <proc/proc.h>
#include <util/cmn_err.h>
#include <io/ws/chan.h>
#include <io/mouse.h>
#include <io/mouse/mse.h>
#include <io/asy/iasy.h>
#include <io/asy/asy.h>
#include <io/ddi.h>

#include <util/mod/moddefs.h>

STATIC int smse_load(), smse_unload();

MOD_STR_WRAPPER(smse, smse_load, smse_unload, "smse - serial mouse driver");

STATIC int
smse_load(void)
{
	/* Module specific load processing... */
	cmn_err(CE_NOTE, "!MOD: in smse_load()");
	return(0);
}


STATIC int
smse_unload(void)
{
	/* Module specific unload processing... */
	cmn_err(CE_NOTE, "!MOD: in smse_unload()");
	return(0);
}

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

/*
** States while parsing M+ format from Microsoft compatible serial mice
*/

#define WAIT_FOR_START_BYTE	0
#define WAIT_FOR_X_DELTA	1
#define WAIT_FOR_Y_DELTA	2
#define WAIT_FOR_MIDDLE_BUTTON	3	/* for Logitech 3-button */

/*
** States while parsing configuration information
*/

#define START_CONFIG		4
#define MM_PROTOCOL		5
#define M_PLUS_PROTOCOL		6

extern void	wakeup();
extern void	mse_iocnack(), mse_iocack();
extern void	mse_copyout(), mse_copyin();
extern void	mseproc();

extern void	smse_MM_parse();
extern void	smse_M_plus_parse();
extern void	smse_config_parse();
extern void	smse_config_port();

extern int	smseopen(), smseclose(), smse_rput(), smse_wput(), smse_srvp();

int		smsedevflag = 0;

struct module_info smse_info =
		   {
			24, "smse", 0, INFPSZ, 256, 128
		   };

static struct qinit smse_rinit =
		    {
			smse_rput, smse_srvp, smseopen,
			smseclose, NULL, &smse_info, NULL
		    };

static struct qinit smse_winit =
		    {
			smse_wput, NULL, NULL, NULL,
			NULL, &smse_info, NULL
		    };

struct streamtab smseinfo =
		 {
			&smse_rinit, &smse_winit, NULL, NULL
		 };

/*
 * Configure the mouse
 */

void
smse_config_mouse( msp )
struct strmseinfo	*msp;
{
	mblk_t	*bp;

#ifdef MOUSEDEBUG1
	printf( "smse_config_mouse:entered\n" );
#endif

	if (( bp = allocb( sizeof( int ), 0 )) == NULL )
		return;

	bp->b_datap->db_type = M_DATA;

	*bp->b_wptr++ = 'S';	/* select MM series format */
	*bp->b_wptr++ = 'D';	/* select prompt mode */
	*bp->b_wptr++ = '*';	/* change baud rate */
	*bp->b_wptr++ = 'q';	/* select 9600 baud */

	putnext( msp->wqp, bp );	/* send to serial port driver */

	smse_config_port( msp );

	/* re-configure mouse */

	if (( bp = allocb( sizeof( int ), 0 )) == NULL )
		return;

	bp->b_datap->db_type = M_DATA;

	*bp->b_wptr++ = 'S';	/* select MM series format */
	*bp->b_wptr++ = 'K';	/* select 20 reports/sec rate */
	*bp->b_wptr++ = 0;	
	*bp->b_wptr++ = 0;

	putnext( msp->wqp, bp );	/* send to serial port driver */

#ifdef MOUSEDEBUG1
	printf( "smse_config_mouse:exited\n" );
#endif
}


void
smse_config_port( msp )
struct strmseinfo	*msp;
{
	mblk_t		*bp;
	struct iocblk	*iocbp;
	struct termio	*cb;

#ifdef MOUSEDEBUG1
	printf( "smse_config_port:entered\n" );
#endif

	if (( bp = allocb( sizeof( struct iocblk ), 0 )) == NULL )
		return;

	bp->b_datap->db_type = M_IOCTL;
	iocbp = (struct iocblk *)bp->b_rptr;
	bp->b_wptr += sizeof( struct iocblk );
	iocbp->ioc_cmd = TCSETAF;
	iocbp->ioc_count = sizeof( struct termio );

	if (( bp->b_cont = allocb( sizeof( struct termio ), 0 )) == NULL )
	{
		freemsg( bp );
		return;
	}

	cb = (struct termio *)bp->b_cont->b_rptr;
	bp->b_cont->b_wptr += sizeof( struct termio );
	cb->c_iflag = IGNBRK | IGNPAR;
	cb->c_oflag = 0;

	if ( msp->smseparse == smse_MM_parse )
	{
		cb->c_cflag = B9600 | CS8 | CREAD | CLOCAL | PARENB | PARODD;
	}
	else
	{
		cb->c_cflag = B1200 | CS7 | CREAD | CLOCAL;
	}

	cb->c_lflag = 0;
	cb->c_line = 0;
	cb->c_cc[ VMIN ] = 1;
	cb->c_cc[ VTIME ] = 0;
	putnext( msp->wqp, bp );

	if (( bp = allocb( sizeof( struct iocblk ), 0 )) == NULL )
		return;

	bp->b_datap->db_type = M_IOCTL;
	iocbp = (struct iocblk *)bp->b_rptr;
	bp->b_wptr += sizeof( struct iocblk );
	iocbp->ioc_cmd = SETRTRLVL;
	iocbp->ioc_count = sizeof( int );

	if (( bp->b_cont = allocb( sizeof( int ), 0 )) == NULL )
	{
		freemsg( bp );
		return;
	}

	*(int *)bp->b_cont->b_wptr = T_TRLVL1;
	bp->b_cont->b_wptr += sizeof( int );
	putnext( msp->wqp, bp );

#ifdef MOUSEDEBUG1
	printf( "smse_config_port:exited\n" );
#endif
}


void
smse_default_parse( msp )
struct strmseinfo	*msp;
{
	msp->smseparse = smse_M_plus_parse;
	msp->report_not_sent = TRUE;
	msp->middle_button_down = FALSE;
	smse_config_port( msp );
}


int
smseopen( q, devp, flag, sflag, cred_p )
queue_t		*q;
dev_t		*devp;
register int	flag;
register int	sflag;
struct cred	*cred_p;
{
	mblk_t			*bp;
	struct strmseinfo	*msp;

	if ( q->q_ptr != NULL )
		return EBUSY;

#ifdef MOUSEDEBUG1
	printf( "smseopen:entered\n" );
#endif

	delay( HZ / 2 );

	/* allocate and initialize state structure */

	if (( q->q_ptr = (caddr_t) kmem_zalloc( sizeof( struct strmseinfo ),
						KM_SLEEP )) == NULL )
	{
		cmn_err( CE_WARN, "SMSE: open fails, can't allocate state structure\n" );
		return ( ENOMEM );
	}

	msp = (struct strmseinfo *)q->q_ptr;
	msp->rqp = q;
	msp->wqp = WR( q );
	msp->wqp->q_ptr = q->q_ptr;
	msp->old_buttons = 0x07;	/* Initialize to all buttone up */

	while (( bp = allocb( sizeof( int ), BPRI_MED )) == NULL )
	{
		(void)bufcall( sizeof( int ), BPRI_MED, wakeup,
				(caddr_t)&q->q_ptr );
		sleep( &q->q_ptr, STIPRI | PCATCH );
	}

	bp->b_datap->db_type = M_DATA;

	/*
	** The following bytes are being sent to whatever mouse
	** happens to be attached to the serial port.  If it is
	** a Microsoft-compatible Logitech NON-Programmable mouse,
	** it will echo "*?SDT" back.  If it is a Logitech type C
	** mouse, it will ignore the "*?" and respond to the "SDt".
	** If it is a Microsoft-compatible programmable, it will 
	** respond with 4 bytes of configuration information.
	*/

	*bp->b_wptr++ = '*';	/* query general configuration */
	*bp->b_wptr++ = '?';
	*bp->b_wptr++ = 'S';	/* select MM series format */
	*bp->b_wptr++ = 'D';	/* select prompt mode */
	*bp->b_wptr++ = 't';	/* query format and prompt mode */

	msp->state = START_CONFIG;
	msp->smseparse = smse_config_parse;

	putnext( msp->wqp, bp );	/* send to serial port driver */

	/*
	** The routine to parse messages from the asy module is
	** initially set to smse_config_parse().  The timeout()
	** is used to default to Microsoft compatible mouse in
	** case a programmable one attached.  smse_parse_config
	** is only looking for a response from the Logitech Type C
	** and the NON-programmable Microsoft-compatible since
	** they both respond deterministically.
	*/

	msp->msetimeid = timeout( smse_default_parse,
					msp, drv_usectohz( 500000 ));

#ifdef MOUSEDEBUG1
	printf( "smseopen:leaving\n" );
#endif

	return( 0 );
}


int
smseclose( q, flag, cred_p )
queue_t		*q;
register int	flag;
struct cred	*cred_p;
{
	register int		oldpri;
	struct strmseinfo	*msp;
	mblk_t			*bp;

#ifdef MOUSEDEBUG1
	printf( "smseclose:entered\n" );
#endif

	msp = (struct strmseinfo *) q->q_ptr;

 	untimeout( msp->msetimeid );

	if (( bp = allocb( sizeof( long ), BPRI_MED)) == NULL )
		cmn_err( CE_WARN, "smseclose: reset of serial mouse baud rate failed" );
	else	/* reset baud rate to 1200 bps */
	{
		if ( msp->smseparse == smse_MM_parse )
		{
			*bp->b_wptr++ = 'S';	/* select MM series format */
			*bp->b_wptr++ = 'D';	/* select prompt mode */
		}

		*bp->b_wptr++ = '*';	/* change baud rate */
		*bp->b_wptr++ = 'n';	/* select 1200 baud */

		putnext( msp->wqp, bp );
	}
	
	oldpri = splstr();
	kmem_free( (caddr_t) msp, sizeof( struct strmseinfo ));
	q->q_ptr = (caddr_t) NULL;
	WR( q )->q_ptr = (caddr_t) NULL;
	splx( oldpri );

#ifdef MOUSEDEBUG1
	printf( "smseclose:leaving\n" );
#endif

	return 0;
}


int
smse_rput( q, mp )
queue_t	*q;
mblk_t	*mp;
{

#ifdef MOUSEDEBUG1
	printf( "smse_rput:entered\n" );
#endif

	switch ( mp->b_datap->db_type )
	{
		case M_DATA:

			/* Queue has been attached but not opened yet */
			if (q->q_ptr == NULL)
				putnext(q, mp);
			else
				putq( q, mp );
			break;

		case M_IOCACK:
		{
			struct iocblk *iocp;

			iocp = (struct iocblk *)mp->b_rptr;

			if ( iocp->ioc_cmd == TCSETAF || iocp->ioc_cmd == SETRTRLVL )
				freemsg( mp );
			else
				putnext( q, mp );

			break;
		}
		case M_FLUSH:

			if ( *mp->b_rptr & FLUSHR )
				flushq( q, FLUSHDATA );

			putnext( q, mp );
			break;

		default:

			putnext( q, mp );
			break;
	}

#ifdef MOUSEDEBUG1
	printf( "smse_rput:leaving\n" );
#endif
}


int
smse_wput( q, mp )
queue_t	*q;
mblk_t	*mp;
{
	struct iocblk			*iocbp;
	register struct strmseinfo	*mseptr;
	register mblk_t			*bp;
	register struct copyreq		*cqp;
	register struct copyresp	*csp;
	int				oldpri;

#ifdef MOUSEDEBUG1
	printf( "smse_wput:entered\n" );
#endif

	mseptr = (struct strmseinfo *)q->q_ptr;
	iocbp = (struct iocblk *) mp->b_rptr;

	switch ( mp->b_datap->db_type )
	{
		case M_FLUSH:

#ifdef MOUSEDEBUG
			printf( "smse_wput:M_FLUSH\n" );
#endif

			if ( *mp->b_rptr & FLUSHW )
				flushq( q, FLUSHDATA );

			putnext( q, mp );
			break;

		case M_IOCTL:

#ifdef MOUSEDEBUG1
			printf( "smse_wput:M_IOCTL\n" );
#endif

			switch( iocbp->ioc_cmd )
			{
				case MOUSEIOCREAD:

					if (( bp = allocb( sizeof( struct mouseinfo ), BPRI_MED )) == NULL )
					{ 
						mse_iocnack( q, mp, iocbp, EAGAIN, 0 );
						break;
					}

					oldpri = spltty();
					bcopy( &mseptr->mseinfo, bp->b_rptr, sizeof( struct mouseinfo ));
					mseptr->mseinfo.xmotion = mseptr->mseinfo.ymotion = 0;
					mseptr->mseinfo.status &= BUTSTATMASK;
					bp->b_wptr += sizeof( struct mouseinfo );
					splx( oldpri );

					if ( iocbp->ioc_count == TRANSPARENT )
						mse_copyout( q, mp, bp, sizeof( struct mouseinfo ), 0 );
					else
					{
						mp->b_datap->db_type = M_IOCACK;
						iocbp->ioc_count = sizeof( struct mouseinfo );
						qreply( q, mp );
					}

					break;

				default:

					mse_iocnack( q, mp, iocbp, EINVAL, 0 );
					break;
			}

			break;

		case M_IOCDATA:

#ifdef MOUSEDEBUG1
			printf("smse_wput:M_IOCDATA\n");
#endif

			csp = (struct copyresp *)mp->b_rptr;

			if ( csp->cp_cmd != MOUSEIOCREAD )
			{
				putnext( q, mp );
				break;
			}

			if ( csp->cp_rval )
			{
				freemsg( mp );
				return;
			}

			mse_iocack( q, mp, iocbp, 0 );
			break;

		default:

			putnext( q, mp );
			break;
	}

#ifdef MOUSEDEBUG1
	printf("smse_wput:leaving\n");
#endif
}


void
smse_M_plus_parse( q, mp )
queue_t	*q;
mblk_t	*mp;
{

	register mblk_t			*bp;
	register struct strmseinfo	*mseptr;
	register unchar			c;
	register int			lcv;

	/*
	** Parse the next byte of serial input.
	** This assumes the input is in M+ format.
	*/

	mseptr = (struct strmseinfo *)q->q_ptr;

	for ( bp = mp; bp != (mblk_t *)NULL; bp = bp->b_cont )
	{
		lcv = bp->b_wptr - bp->b_rptr;

		while ( lcv-- )
		{
			c = *bp->b_rptr++;

			if ( c & 0x40 )		/* start byte */
			{
				if ( mseptr->state == WAIT_FOR_MIDDLE_BUTTON 
						&& mseptr->report_not_sent )
					mseproc( mseptr );

				mseptr->x = ( c & 0x03 ) << 6;	/* MS X bits */
				mseptr->y = ( c & 0x0C ) << 4;	/* MS Y bits */
				mseptr->state = WAIT_FOR_X_DELTA;

				c = ~c;
				mseptr->button = ( c >> 4 ) & 0x01
						 | ( c >> 3 ) & 0x04
						 | 0x2;

				continue;
			}

			switch ( mseptr->state )
			{
				case WAIT_FOR_X_DELTA:

					mseptr->x |= c & 0x3F;
					mseptr->state = WAIT_FOR_Y_DELTA;
					break;

				case WAIT_FOR_Y_DELTA:

					mseptr->y |= c & 0x3F;
					mseptr->state = WAIT_FOR_MIDDLE_BUTTON;
					mseptr->report_not_sent = TRUE;
					break;

				case WAIT_FOR_MIDDLE_BUTTON:

					c = ( c >> 4 ) & 0x02;
					mseptr->button ^= c;
					mseptr->middle_button_down = c;
					mseptr->state = WAIT_FOR_START_BYTE;
					mseproc( mseptr );
					break;
			}
   		}
	}

	freemsg( mp );

	if ( mseptr->state == WAIT_FOR_MIDDLE_BUTTON &&
					! mseptr->middle_button_down )
	{
		mseproc( mseptr );
		mseptr->x = mseptr->y = 0;
		mseptr->report_not_sent = FALSE;
	}
}


void
smse_config_parse( q, mp )
queue_t	*q;
mblk_t	*mp;
{

	register mblk_t			*bp;
	register struct strmseinfo	*mseptr;
	register unchar			c;
	register int			lcv;

	/*
	** Need to determine what type of serial mouse is attached:
	**
	** If it is a Logitech Type C, it will respond to to the 't'
	** I sent in smseopen() with 2 bytes, 'S' followed by 'D'.
	** If it is a NON-programmable Microsoft-compatible, it will
	** echo the "*?DSt" I sent in smseopen() right back.  The
	** programmable Microsoft-compatible responds non-
	** deterministically, so we will default to that if no
	** recognizable response is forth coming.
	*/

	mseptr = (struct strmseinfo *)q->q_ptr;

	for ( bp = mp; bp != (mblk_t *)NULL; bp = bp->b_cont )
	{
		lcv = bp->b_wptr - bp->b_rptr;

		while ( lcv-- )
		{
			c = *bp->b_rptr++;

			switch ( mseptr->state )
			{
				case START_CONFIG:

					if ( c == 'S' )
						mseptr->state = MM_PROTOCOL;
					else if ( c == '*' )
						mseptr->state = M_PLUS_PROTOCOL;

					break;

				case MM_PROTOCOL:

					if ( c == 'D' )
					{
						untimeout( mseptr->msetimeid );
						mseptr->smseparse = smse_MM_parse;
						mseptr->msetimeid = timeout(smse_config_mouse, mseptr,
									 drv_usectohz(200000));
						goto free;
					}

					mseptr->state = START_CONFIG;
					break;

				case M_PLUS_PROTOCOL:

					if ( c == '?' )
					{
						untimeout( mseptr->msetimeid );
						mseptr->smseparse = smse_M_plus_parse;
						mseptr->report_not_sent = TRUE;
						mseptr->middle_button_down = FALSE;
						smse_config_port( mseptr );
						goto free;
					}

					mseptr->state = START_CONFIG;
					break;
			}

   		}
	}
free:
	freemsg( mp );
}


void
smse_MM_parse( q, mp )
queue_t	*q;
mblk_t	*mp;
{

	register mblk_t			*bp;
	register struct strmseinfo	*mseptr;
	register unchar			c;

	/* Parse the next byte of serial input.
	 * This assumes the input is in MM Series format.
	 */
	mseptr = (struct strmseinfo *)q->q_ptr;

	for ( bp = mp; bp != (mblk_t *)NULL; bp = bp->b_cont )
	{
		while ( bp->b_wptr - bp->b_rptr )
		{
			c = *bp->b_rptr++;

			if ( c & 0x80 )
			{
				/* Bit seven set always means the first byte */

				mseptr->button = (~c & 0x07);		/* Buttons */
				mseptr->x = ( c & 0x10 ) ? 1 : -1;	/* X sign bit */
				mseptr->y = ( c & 0x08 ) ? -1 : 1;	/* Y sign bit */
				mseptr->state = 1;
				continue;
			}

			switch ( mseptr->state )
			{
				case 1:	/* Second byte is X movement */

					mseptr->x *= c;
					mseptr->state = 2;
					break;

				case 2:	/* Third byte is Y movement */

					mseptr->y *= c;
					mseptr->state = 0;
					mseproc( mseptr );
					break;
			}
   		}
	}

	freemsg( mp );
}


int
smse_srvp( q )
queue_t	*q;
{
	register mblk_t *mp;

	while (( mp = getq( q )) != NULL )
	{
		switch ( mp->b_datap->db_type )
		{
			case M_DATA:

				(*((struct strmseinfo *)q->q_ptr)->smseparse)( q, mp );
				break;

			default:

				putnext( q, mp );
				break;
		}
	}
}
