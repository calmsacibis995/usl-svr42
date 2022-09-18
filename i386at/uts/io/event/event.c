/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/event/event.c	1.3"
#ident	"$Header: $"

#include <util/types.h>
#include <util/param.h>
#include <mem/immu.h>
#include <util/sysmacros.h>
#include <svc/errno.h>
#include <proc/signal.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <io/conf.h>
#include <mem/kmem.h>
#include <io/tty.h>
#include <io/stream.h>
#include <proc/cred.h>
#include <io/uio.h>
#include <fs/vnode.h>
#include <io/gvid/genvid.h>
#include <util/cmn_err.h>
#include <io/ddi.h>
#include <io/termios.h>
#include <io/xque/xque.h>
#include <io/event/event.h>
#include <proc/session.h>
#include <util/debug.h>


extern int gviddevflag;
extern gvid_t Gvid;
extern int gvidflg; 

/*
#define DEBUG
*/

#ifdef DEBUG
#define EVENT_DEBUG(a)	 if (event_debug) cmn_err(CE_NOTE,  a) 
#endif 

int event_debug = 0;


/*
 *   Event clone driver
 */

void event_iocack();
void event_iocnack();
void event_copyout();
void event_copyin();
void event_do_iocdata();

STATIC int eventopen(), eventclose();

static int eventwput();

static struct module_info event_info = {0xdfdf, "event", 0, 512, 512, 128};

static struct qinit eventrint = {NULL, NULL, eventopen, eventclose,NULL,&event_info, NULL};
static struct qinit eventwint = {eventwput, NULL, NULL, NULL, NULL,&event_info, NULL};
struct streamtab eventinfo = {&eventrint, &eventwint, NULL, NULL};

/*
 * evchan_dev[] and evchan_cnt are defined in master.d file 
 */
extern struct evchan evchan_dev[];	
extern int evchan_cnt;
 
int eventdevflag = D_OLD;

/* ARGSUSED */
STATIC int 
eventopen(rqp, dev, flag, sflag)
queue_t *rqp;
dev_t dev;
int flag;
int sflag;
{
        register struct evchan *evchanp = NULL;
        dev_t ttyd;
	int	oldpri;
	int	majnum, minnum, ndev, error;

	if (error = ws_getctty(&ttyd))
		return error;

 	majnum = getmajor(ttyd);
 
	oldpri = splhi();
 	while (gvidflg & GVID_ACCESS) /* sleep */
 		if (sleep((caddr_t) &gvidflg, STOPRI|PCATCH)) {
			splx(oldpri);
 			u.u_error = EINTR;
 			/* even if ioctl was not ours, we've
 			 * effectively handled it */
 			return (OPENFAIL);
 		}

	gvidflg |= GVID_ACCESS; 
	splx(oldpri);
	
 	/* return if controlling tty is gvid */
 	if (majnum != Gvid.gvid_maj) {
		gvidflg &= ~GVID_ACCESS; 
		wakeup((caddr_t) &gvidflg);
                u.u_error =  ENXIO;
                return(OPENFAIL);
	}

 	minnum = getminor(ttyd);
#ifdef DEBUG
	cmn_err(CE_NOTE, "major %d minor %d", majnum, minnum);
#endif

 	if (minnum >= Gvid.gvid_num) {
 		u.u_error = ENXIO;
		gvidflg &= ~GVID_ACCESS; 
		wakeup((caddr_t) &gvidflg);
                u.u_error =  ENXIO;
                return(OPENFAIL);
 	}
 
	/* done with lookup in Gvid. Release access to it */
	gvidflg &= ~GVID_ACCESS; 
	wakeup((caddr_t) &gvidflg);

#ifdef DEBUG
	EVENT_DEBUG( "entering eventopen\n");
#endif

	if (sflag != CLONEOPEN) {
#ifdef DEBUG
		EVENT_DEBUG( "invalid sflag\n");
#endif

		u.u_error = EINVAL;
		return(OPENFAIL);
	}

	for(ndev= 0; ndev < evchan_cnt; ndev++)
		if(evchan_dev[ndev].eq_state == EVCH_CLOSE ||
			evchan_dev[ndev].eq_ttyd ==  ttyd)
			break;
        if (ndev >= evchan_cnt ) {
#ifdef DEBUG
		EVENT_DEBUG( "no more devices left to allocate\n");
#endif

		u.u_error = ENODEV;
                return(OPENFAIL);
	}

#ifdef DEBUG
cmn_err(CE_NOTE, "event ttyd is %x", ttyd);
#endif

        if (evchan_dev[ndev].eq_ttyd ==  ttyd) {
#ifdef DEBUG
		EVENT_DEBUG( "EXLUSIVE OPEN once per channel\n");
#endif

		u.u_error = EINVAL;
                return(OPENFAIL);
	}
	evchanp = &evchan_dev[ndev];
	if(allocate_scoq(evchanp) == NULL)
		return(OPENFAIL);
        evchan_dev[ndev].eq_ttyd =  ttyd;
        evchan_dev[ndev].eq_rdev =  0;
        evchan_dev[ndev].eq_chp  =  NULL;
        evchan_dev[ndev].eq_rqp  =  (caddr_t) rqp;
        evchan_dev[ndev].eq_block_msg  =  NULL;
        evchanp = &evchan_dev[ndev];
	evchanp->eq_state = EVCH_OPEN;
	evchanp->eq_xqinfo.xq_private = (caddr_t) evchanp;
	evchanp->eq_xqinfo.xq_buttons = 07; /* start with all buttons up */
	evchanp->eq_emask = 0 ; /* T_STRING|T_BUTTON|T_REL_LOCATOR; */
	rqp->q_ptr = evchanp;
	WR(rqp)->q_ptr = evchanp;
	
#ifdef DEBUG
	EVENT_DEBUG( "returning from eventopen()\n");
#endif

	return(ndev);
}

int
event_check_que(qp, rdev, chp, cmd)
xqInfo *qp;
dev_t	rdev;
unsigned *chp;
int	cmd;
{
	register ndev;
	int	type;

#ifdef DEBUG
cmn_err(CE_NOTE, "checking for tty %x", rdev);
#endif

        for(ndev= 0; ndev < evchan_cnt; ndev++)
                if(evchan_dev[ndev].eq_state != EVCH_CLOSE &&
                        evchan_dev[ndev].eq_rdev ==  rdev)
                        break;
	if(ndev < evchan_cnt)
	{
		if(cmd == LDEV_MSEATTACHQ) {
			evchan_dev[ndev].eq_xqinfo.xq_devices   |= QUE_MOUSE;
			type = T_REL_LOCATOR|T_BUTTON;
		} else {
			evchan_dev[ndev].eq_xqinfo.xq_devices   |= QUE_KEYBOARD;
        		type = T_STRING;
		}
		evchan_dev[ndev].eq_emask |= type;
#ifdef DEBUG
		cmn_err(CE_NOTE, "event_check_que: emask %o",
				evchan_dev[ndev].eq_emask);
#endif
		*qp = evchan_dev[ndev].eq_xqinfo;
		evchan_dev[ndev].eq_chp = chp;
		return(0);
	}
#ifdef DEBUG
cmn_err(CE_NOTE, "checking for tty %x NOT FOUND", rdev);
#endif


	return(ENODEV);
}

STATIC int 
eventclose(rqp)
queue_t *rqp;
{
        register struct evchan *evchanp;
        
#ifdef DEBUG
	EVENT_DEBUG( "entering eventclose");
#endif
        evchanp = (struct evchan *) rqp->q_ptr;

	if(evchanp->eq_block_msg) 
                     freemsg((mblk_t *) evchanp->eq_block_msg);
	evchanp->eq_rqp = NULL;
	evchanp->eq_block_msg = NULL;

	scoq_close(&evchanp->eq_xqinfo);
	evchanp->eq_state = EVCH_CLOSE;
	evchanp->eq_ttyd   = 0;
	evchanp->eq_rdev   = 0;

	if(evchanp->eq_chp)
		ws_queuemode(evchanp->eq_chp, LDEV_ATTACHQ, 0);
	evchanp->eq_chp = NULL;
#ifdef DEBUG
	EVENT_DEBUG( "returning from eventclose\n");
#endif

	return( 0);
}

void
event_set_dev(evchanp, mp)
struct evchan *evchanp;
mblk_t *mp;
{
	struct	event_getq_info *xp;

	xp  = ((struct  event_getq_info *) mp->b_rptr);
	evchanp->eq_rdev = xp->einfo_rdev;
#ifdef DEBUG
	cmn_err(CE_NOTE, "event_set_dev rdev is %d", evchanp->eq_rdev);
#endif

}

void
event_wakeup(evchanp)
register struct evchan *evchanp;
{

        struct iocblk *iocp;
        mblk_t *mp;

	if((mp = (mblk_t *) evchanp->eq_block_msg)) {
#ifdef DEBUG
		cmn_err(CE_NOTE, "wake up ioctl is sent");
#endif
		iocp = (struct iocblk *)mp->b_rptr;
		event_iocack(evchanp, mp, iocp, 0);
		evchanp->eq_block_msg = NULL;
	}
}

static int 
eventwput(qp, mp)
queue_t *qp;
mblk_t *mp;
{
        register struct evchan *evchanp;
	struct iocblk *iocp;
	mblk_t *bp;
	int transparent;
	
	
#ifdef DEBUG
	EVENT_DEBUG( "entering eventwput\n");
#endif


        evchanp = (struct evchan *) qp->q_ptr;
		
	switch(mp->b_datap->db_type) {
	case M_FLUSH:
#ifdef DEBUG
		EVENT_DEBUG( "event got flush request\n");
#endif

		flushq(qp, FLUSHDATA);
		flushq(WR(qp), FLUSHDATA);
		freemsg(mp);
	break;
	case M_IOCDATA:
                event_do_iocdata(qp, mp, evchanp);
                break;

	case M_IOCTL:
			iocp = (struct iocblk *)mp->b_rptr;
        		transparent = (iocp->ioc_count == TRANSPARENT);
			if(!transparent) {
				event_iocnack(evchanp, mp, iocp, EINVAL, -1);
#ifdef DEBUG
				EVENT_DEBUG( "NOT transparent ioctl ");
#endif

				return;
			}
			
			switch(iocp->ioc_cmd) {
			case EQIO_GETQP:
#ifdef DEBUG
			cmn_err(CE_NOTE, "GOT EQIO_GETQP ioctl evchanp %x",
					 evchanp);
#endif
			    if(!mp->b_cont) {
				event_iocnack(evchanp, mp, iocp, EINVAL, -1);
#ifdef DEBUG
			EVENT_DEBUG( "NO b_cont ioctl evchanp ");
#endif
                                break;
			    }
			    bp = allocb(sizeof(caddr_t), BPRI_MED);
			    if(bp == NULL){
				event_iocnack(evchanp, mp, iocp, EAGAIN, 0);
				break;
			    }
			    event_set_dev(evchanp, mp->b_cont);
                            evchanp->eq_state = EVCH_ACTIVE;

			    *((caddr_t *) bp->b_rptr) = 
				((caddr_t ) evchanp->eq_xqinfo.xq_uaddr);
			    bp->b_wptr =  bp->b_rptr + sizeof(caddr_t);
			    event_copyout(evchanp, mp, bp, sizeof(caddr_t));
#ifdef DEBUG
cmn_err(CE_NOTE, "event address is %x", evchanp->eq_xqinfo.xq_uaddr);
#endif

                           break;
        		case EQIO_BLOCK:   
#ifdef DEBUG
			EVENT_DEBUG( "GOT EQIO_BLOCK ioctl");
#endif

			{
				register QUEUE *qp;
			
				if ( evchanp->eq_state != EVCH_ACTIVE ) {
					event_iocnack(evchanp, mp, iocp, ENODEV, -1);
					return;
				}

				qp = (QUEUE *) evchanp->eq_xqinfo.xq_qaddr;
				if (qp->head != qp->tail) {
					event_iocack(evchanp, mp, iocp, 0);
					break;
					}
				evchanp->eq_block_msg = (caddr_t) mp;
#ifdef DEBUG
	cmn_err(CE_NOTE, "event_block: evanchp %x, qp %x, mp %x",
			evchanp, qp, mp);
#endif
			}
                           break;
        		case EQIO_SETEMASK:
#ifdef DEBUG
			EVENT_DEBUG( "GOT EQIO_SETEMASK ioctl");
#endif
			    if(!mp->b_cont) {
				event_iocnack(evchanp, mp, iocp, EINVAL, -1);
				break;
			    }
			    evchanp->eq_emask = 
				*((emask_t *) mp->b_cont->b_rptr);
			    event_iocack(evchanp, mp, iocp, 0);
                           break;
                       case EQIO_GETEMASK:
#ifdef DEBUG
                        EVENT_DEBUG( "GOT EQIO_GETEMASK ioctl");
#endif
                            bp = allocb(sizeof(emask_t), BPRI_MED);
                            if(bp == NULL){
                                event_iocnack(evchanp, mp, iocp, EAGAIN, 0);
                                break;
                            }
                            *((emask_t *) bp->b_rptr) = evchanp->eq_emask;
                            bp->b_wptr =  bp->b_rptr + sizeof(emask_t);
                            event_copyout(evchanp, mp, bp, sizeof(emask_t));
                           break;

        		case EQIO_SUSPEND:
#ifdef DEBUG
			EVENT_DEBUG( "GOT EQIO_SUSPEND ioctl");
#endif
				if ( evchanp->eq_state != EVCH_ACTIVE ) {
					event_iocnack(evchanp, mp, iocp, EINVAL, -1);
				}
				else {
					evchanp->eq_state = EVCH_SUSPENDED;
					event_iocack(evchanp, mp, iocp, 0);
				}
                           break;
        		case EQIO_RESUME:
#ifdef DEBUG
			EVENT_DEBUG( "GOT EQIO_RESUME ioctl");
#endif
				if ( evchanp->eq_state != EVCH_SUSPENDED)
					event_iocnack(evchanp, mp, iocp, EINVAL, -1);
				else {
					event_iocack(evchanp, mp, iocp, 0);
					evchanp->eq_state = EVCH_ACTIVE;
				}
                           break;
        		default:
#ifdef DEBUG
			EVENT_DEBUG( "GOT unknown ioctl");
#endif

				event_iocnack(evchanp, mp, iocp, EINVAL, -1);
        		}
		break;
	default:
		event_iocnack(evchanp, mp, iocp, EINVAL, -1);
	}
#ifdef DEBUG
	EVENT_DEBUG( "return from eventwput()\n");
#endif

	return( 0);
}

void
event_iocack(evchanp, mp, iocp, rval)
struct  evchan *evchanp;
mblk_t *mp;
struct iocblk *iocp;
int rval;
{
	mblk_t	*tmp;
        queue_t *qp = (queue_t *) evchanp->eq_rqp;
#ifdef DEBUG
	EVENT_DEBUG( "ACKNOWLDGEMENT");
#endif

	mp->b_datap->db_type = M_IOCACK;
	iocp->ioc_rval = rval;
	iocp->ioc_count = iocp->ioc_error = 0;
	if ((tmp = unlinkb(mp)) != (mblk_t *)NULL)
		freemsg(tmp);
	putnext(qp,mp);
}

void
event_iocnack(evchanp, mp, iocp, error, rval)
struct  evchan *evchanp;
mblk_t *mp;
struct iocblk *iocp;
int error;
int rval;
{
        queue_t *qp = (queue_t *) evchanp->eq_rqp;
#ifdef DEBUG
	EVENT_DEBUG( "NACKNOWLDGEMENT");
#endif

	mp->b_datap->db_type = M_IOCNAK;
	iocp->ioc_rval = rval;
	iocp->ioc_error = error;
	putnext(qp,mp);
}


void
event_copyout(evchanp, mp, nmp, size, state)
struct	evchan *evchanp;
register mblk_t *mp, *nmp;
uint size;
unsigned long state;
{
	register struct copyreq *cqp;
	queue_t *qp = (queue_t *) evchanp->eq_rqp;

#ifdef DEBUG
	EVENT_DEBUG( " BEGIN OF event_copyout");
#endif


	cqp = (struct copyreq *) mp->b_rptr;
	cqp->cq_size = size;
	cqp->cq_addr = (caddr_t) * (long *) mp->b_cont->b_rptr;
#ifdef DEBUG
	cmn_err(CE_NOTE, "Copying into user address %x",
			cqp->cq_addr);
#endif

	cqp->cq_flag = 0;
	cqp->cq_private = (mblk_t *) NULL;

	mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);
	mp->b_datap->db_type = M_COPYOUT;

	if (mp->b_cont) freemsg(mp->b_cont);
	mp->b_cont = nmp;

	putnext(qp, mp);
#ifdef DEBUG
	EVENT_DEBUG( " END OF event_copyout");
#endif

}


void
event_copyin(evchanp, mp, size)
struct  evchan *evchanp;
register mblk_t *mp;
int size;
{
        queue_t *qp = (queue_t *) evchanp->eq_rqp;

	register struct copyreq *cqp;

#ifdef DEBUG
	EVENT_DEBUG( " BEGIN OF event_copyin");
#endif

	cqp = (struct copyreq *) mp->b_rptr;
	cqp->cq_size = size;
	cqp->cq_addr = (caddr_t) * (long *) mp->b_cont->b_rptr;
	cqp->cq_flag = 0;
	cqp->cq_private = (mblk_t *) NULL;

	if (mp->b_cont) freemsg(mp->b_cont);
	mp->b_cont = NULL;

	mp->b_datap->db_type = M_COPYIN;
	mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);

	putnext(qp, mp);
#ifdef DEBUG
	EVENT_DEBUG( " END OF event_copyin");
#endif

}



void
event_do_iocdata(qp, mp, evchanp)
	queue_t *qp;
	register mblk_t *mp;
	register struct evchan *evchanp;
{
	register struct iocblk *iocp;
	struct copyresp *csp;
	int    		tmp;

#ifdef DEBUG
	EVENT_DEBUG( " BEGIN OF event_do_iocdata");
#endif

	iocp = (struct iocblk *)mp->b_rptr;

	csp = (struct copyresp *) mp->b_rptr;


	switch (iocp->ioc_cmd) {
		case EQIO_GETEMASK:
		case EQIO_GETQP:
#ifdef DEBUG
	EVENT_DEBUG( "IOCDATA of type EQIO_GETEMASK/EQIO_GETQP");
#endif

			if (csp->cp_rval) {
				freemsg(mp);
#ifdef DEBUG
	EVENT_DEBUG( " END OF event_do_iocdata");
#endif

				return;
			}

			event_iocack(evchanp,mp,iocp,0);
			break;
		default:
#ifdef DEBUG
	cmn_err(CE_NOTE, "IOCDATA of type %d", iocp->ioc_cmd);
#endif

			break;
	}
#ifdef DEBUG
	EVENT_DEBUG(" END OF event_do_iocdata");
#endif

}

