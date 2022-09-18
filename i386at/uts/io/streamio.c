/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/streamio.c	1.29"
#ident	"$Header: $"

#include <acc/audit/audit.h>
#include <acc/dac/acl.h>
#include <acc/mac/cca.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/file.h>
#include <fs/filio.h>
#include <fs/ioccom.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <io/poll.h>
#include <io/sad/sad.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strsubr.h>
#include <io/open.h>
#include <io/termio.h>
#include <io/termios.h>
#include <io/ttold.h>
#include <io/uio.h>
#include <io/xt/jioctl.h>
#include <io/gvid/genvid.h>
#include <io/kd/kd.h>
#include <io/ws/vt.h>
#include <io/mouse.h>
#include <io/asy/asy.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/session.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/secsys.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/spl.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/mod/mod_k.h>

/* Enhanced Application Compatibility Support */
#include <svc/isc.h>
#include <svc/sco.h>
#include <net/transport/timod.h>
#include <io/xque/xque.h>
#include <io/event/event.h>
/* End Enhanced Application Compatibility Support */

/*
 * id value used to distinguish between different ioctl messages
 */
STATIC long ioc_id;

/*
 *  Qinit structure and Module_info structures
 *        for stream head read and write queues
 */
static int strrput();
int	strwsrv();
STATIC int strsink();

STATIC struct module_info strm_info = { 0, "strrhead", 0, INFPSZ, STRHIGH, STRLOW };
STATIC struct module_info stwm_info = { 0, "strwhead", 0, 0, 0, 0 };
struct	qinit strdata = { strrput, NULL, NULL, NULL, NULL, &strm_info, NULL };
struct	qinit stwdata = { NULL, strwsrv, NULL, NULL, NULL, &stwm_info, NULL };
STATIC struct qinit deadrend = {
	strsink, NULL, NULL, NULL, NULL, &strm_info, NULL
};
STATIC struct qinit deadwend = {
	NULL, NULL, NULL, NULL, NULL, &stwm_info, NULL
};

extern struct streamtab fifoinfo;

/*
 * Tunable parameters set in kernel.cf and used only in this file.
 */
extern int	strmsgsz;	/* maximum stream message size */
extern int	strctlsz;	/* maximum size of ctl part of message */
extern long	strthresh;	/* Strcount threshold above which */
				/* some Streams operations will be stopped */

/*
 * switch for small message consolidation feature.
 * off as the feature is broken
 */
STATIC int strhold = 0;	/* switch for small message consolidation feature */

/*
 * Open a stream device.
 */
int
stropen(vp, devp, flag, crp)
	struct vnode *vp;
	dev_t *devp;
	int flag;
	cred_t *crp;
{
	register struct stdata *stp;
	register queue_t *qp;
	register int s;
	dev_t dummydev;
	struct autopush *ap;
	int error = 0;
	int freed = 0;
	extern vnode_t *specfind();

	/*
	 * If the stream already exists, wait for any open in progress
	 * to complete, then call the open function of each module and
	 * driver in the stream.  Otherwise create the stream.
	 */
retry:
	if (stp = vp->v_stream) {

		/*
		 * Waiting for stream to be created to device
		 * due to another open.
		 */
		if (stp->sd_flag & (STWOPEN|STRCLOSE)) {
			if (flag & (FNDELAY|FNONBLOCK)) {
				error = EAGAIN;
				goto ckreturn;
			}
			if (sleep((caddr_t)stp, STOPRI|PCATCH)) {
				error = EINTR;
				goto ckreturn;
			}
			goto retry;  /* could be clone! */
		}

		if (stp->sd_flag & (STRDERR|STWRERR)) {
			error = EIO;
			goto ckreturn;
		}

		s = splstr();
		stp->sd_flag |= STWOPEN;

		/*
		 * Open all modules and devices down stream to notify
		 * that another user is streaming.  For modules, set the
		 * last argument to MODOPEN and do not pass any open flags.
		 * Ignore dummydev since this is not the first open.
		 * Note: still at plstr as specified by the DDI.
		 */
		qp = stp->sd_wrq;
		while (SAMESTR(qp)) {
			qp = qp->q_next;
			if (qp->q_flag & QREADR)
				break;
			if (qp->q_flag & QOLD) {	/* old interface */
				dev_t oldev;
				extern void gen_setup_idinfo();

				gen_setup_idinfo(crp);

			/* check if dev is too large for old interface */
				if ((oldev = cmpdev(*devp)) == NODEV){
					error = ENXIO;
					break;
				}
				if ((*RD(qp)->q_qinfo->qi_qopen)(RD(qp), oldev,
				    flag, (qp->q_next ? MODOPEN : 0)) ==
				    OPENFAIL) {
					if ((error = u.u_error) == 0)	/* XXX */
						error = ENXIO;
					break;
				}
			} else {			/* new interface */
				dummydev = *devp;
				if (error=((*RD(qp)->q_qinfo->qi_qopen)(RD(qp),
				    &dummydev, flag, (qp->q_next ? MODOPEN : 0),
				    crp)))
					break;
			}
		}

		stp->sd_flag &= ~(STRHUP|STWOPEN);
		stp->sd_rerror = 0;
		stp->sd_werror = 0;
		splx(s);
ckreturn:
		/*
		 * If there was an error and a stream is still
		 * associated with the vnode and this is the last
		 * reference to the stream, close it down.
		 */
		if (error) {
			if (vp->v_stream && vp->v_count == 1) {
				struct vnode *rvp;

				rvp = specfind(vp->v_rdev, vp->v_type);
				if (rvp) {
					VN_RELE(rvp);
					if (rvp->v_count == 1)
						strclose(vp, flag, crp);
				}
			}
		}
		wakeprocs((caddr_t)stp, PRMPT);
		return (error);
	} 

	/* 
	 * This vnode isn't streaming.  SPECFS already
	 * checked for multiple vnodes pointing to the
	 * same stream, so create a stream to the driver.
	 *
	 * firewall - don't use too much memory
	 */

	if (strthresh && (Strcount > strthresh) && pm_denied(u.u_cred, P_SYSOPS))
		return (ENOSR);

	if (!(qp = allocq())) {
		cmn_err(CE_CONT, "stropen: out of queues\n");
		return (ENOSR);
	}

	if ((stp = shalloc(qp)) == NULL) {
		freeq(qp);
		return (ENOSR);
	}

	/*
	 * We could have slept in the above allocations.
	 * Check vp->v_stream again to make sure nobody
	 * came in and opened the stream before us.
	 */
	if (vp->v_stream) {
		shfree(stp);
		freeq(qp);
		goto retry;
	}
	
        /*
         * We don't know that the vnode is really at the level of
         * the process, but it should be at the same level as the
         * stream being created, and this MAC_ASSUME() ensures
         * that the CCA tool figures this out.
         */
        MAC_ASSUME (vp, MAC_SAME);

	/* 
	 * Initialize stream head.
	 */
	s = splstr();
	stp->sd_flag = STWOPEN;
	stp->sd_siglist = NULL;
	stp->sd_pollist.ph_events = 0;
	stp->sd_pollist.ph_list = NULL;
	stp->sd_sigflags = 0;
	stp->sd_mark = NULL;
	stp->sd_closetime = STRTIMOUT * HZ;
	splx(s);
	stp->sd_sidp = NULL;
	stp->sd_pgidp = NULL;
	stp->sd_vnode = vp;
	stp->sd_rerror = 0;
	stp->sd_werror = 0;
	stp->sd_wroff = 0;
	stp->sd_iocwait = 0;
	stp->sd_iocblk = NULL;
	stp->sd_pushcnt = 0;
	setq(qp, &strdata, &stwdata);
	qp->q_ptr = WR(qp)->q_ptr = (caddr_t)stp;
	vp->v_stream = stp;

	if (vp->v_type == VFIFO) {
		stp->sd_strtab = &fifoinfo;
		goto opendone;
	}

	/*
	 * Open driver and create stream to it (via qattach).  Device 
	 * opens may sleep, but must set PCATCH if they do so that
	 * signals will not cause a longjmp.  Failure to do this may
	 * result in the queues and stream head not being freed.
	 */
	dummydev = *devp;
	if (error = qattach(qp, devp, flag, CDEVSW, getmajor(*devp), crp))
		goto opendone;

	/*
	 * The following line has been moved after the qattach() call.
	 * In the event that qattach() causes the STREAMS driver to be
	 * loaded, we'll pick up the correct value here.
	 */
	stp->sd_strtab = cdevsw[getmajor(*devp)].d_str;

	/*
	 * check for autopush
	 */
	ap = strphash(getemajor(*devp));
	while (ap) {
		if (ap->ap_major == getemajor(*devp)) {
			if (ap->ap_type == SAP_ALL)
				break;
			else if ((ap->ap_type == SAP_ONE) &&
				 (ap->ap_minor == geteminor(*devp)))
					break;
			else if ((ap->ap_type == SAP_RANGE) &&
				 (geteminor(*devp) >= ap->ap_minor) &&
				 (geteminor(*devp) <= ap->ap_lastminor))
					break;
		}
		ap = ap->ap_nextp;
	}
	if (ap == NULL)
		goto opendone;

	for (s = 0; s < ap->ap_npush; s++) {
		queue_t *nqp;

		if (stp->sd_flag & (STRHUP|STRDERR|STWRERR)) {
			error = (stp->sd_flag & STRHUP) ? ENXIO : EIO;
			strclose(vp, flag, crp);
			freed++;
			break;
		}
		if (stp->sd_pushcnt >= nstrpush) {
			strclose(vp, flag, crp);
			freed++;
			error = EINVAL;
			break;
		}

		/*
 		 * push new module and call its open routine via qattach
 		 */
		if (error = qattach(qp, &dummydev, 0, FMODSW, ap->ap_list[s], crp)) {
			strclose(vp, flag, crp);
			freed++;
			break;
		} else
			stp->sd_pushcnt++;
	} /* for */

opendone:

	/*
	 * Wake up others that are waiting for stream to be created.
	 */
	s = splstr();
	stp->sd_flag &= ~STWOPEN;
	splx(s);
	wakeprocs((caddr_t)stp, PRMPT);
	if (error) {
		if (!freed) {
			vp->v_stream = NULL;
			stp->sd_vnode = NULL;
			shfree(stp);
			freeq(qp);
		}
		return (error);
	}

	return (0);
}

/*
 * Close a stream.  
 * This is called from closef() on the last close of an open stream.
 * Strclean() will already have removed the siglist and pollist
 * information, so all that remains is to remove all multiplexor links
 * for the stream, pop all the modules (and the driver), and free the
 * stream structure.
 */
int
strclose(vp, flag, crp)
	struct vnode *vp;
	int flag;
	cred_t *crp;
{
	register s;
	register struct stdata *stp;
	register queue_t *qp;
	mblk_t *mp;
	extern void strtime();
	int id;
	int rval;

	ASSERT(vp->v_stream);

	stp = vp->v_stream;
	qp = stp->sd_wrq;

	ASSERT(stp->sd_pollist.ph_list == NULL);

        /* freectty should have been called by now */
	ASSERT(stp->sd_sidp == NULL);
	ASSERT(stp->sd_pgidp == NULL);

	s = splstr();
	stp->sd_flag |= STRCLOSE;

	if (mp = qp->q_first) /* message "held" by strwrite */
		qp->q_first = NULL;

 	/*
 	 * if this queue appears in the linked list of write queues
 	 * with held messages, remove it
 	 */
 	if (scanqhead)  {
 		register struct queue *q, *prev = NULL;
 
 		for (q = scanqhead; q; q = q->q_link)  {
 			if (q == qp)  {
 				if (prev)
 					prev->q_link = q->q_link;
 				else
 					scanqhead = q->q_link;
 				if (q == scanqtail)
 					scanqtail = prev;
				break;
			}
 			prev = q;
 		}
 	}
	splx(s);

	if (mp)  {
		if (stp->sd_flag & (STRHUP|STWRERR))
			freemsg(mp);
		else
			putnext(qp, mp);
	}

	s = splstr();
	stp->sd_flag &= ~(STRDERR|STWRERR);	/* help unlink succeed */
	stp->sd_rerror = 0;
	stp->sd_werror = 0;
	splx(s);
	(void) munlinkall(stp, LINKCLOSE|LINKNORMAL, crp, &rval);

	while (SAMESTR(qp)) {
		if (!(flag & (FNDELAY|FNONBLOCK)) && (stp->sd_closetime > 0)) {
			s = splstr();
			stp->sd_flag |= (STRTIME | WSLEEP);
			id = timeout(strtime, (caddr_t)stp, stp->sd_closetime);
			/*
			 * sleep until awakened by strwsrv() or strtime() 
			 */
			while ((stp->sd_flag & STRTIME) && qp->q_next->q_first) {
				stp->sd_flag |= WSLEEP;
				/* ensure strwsrv gets enabled */
				qp->q_next->q_flag |= QWANTW;
				if (sleep((caddr_t)qp, STIPRI|PCATCH))
					break;
			}
			untimeout(id);
			stp->sd_flag &= ~(STRTIME | WSLEEP);
			splx(s);
		}
		qdetach(RD(qp->q_next), 1, flag, crp);
	}

	s = splstr();
	flushq(qp, FLUSHALL);
	for (mp = RD(qp)->q_first; mp; mp = mp->b_next) {
		if (mp->b_datap->db_type == M_PASSFP) 
			closef(((struct strrecvfd *)mp->b_rptr)->f.fp);
	}
	flushq(RD(qp), FLUSHALL);
	if (qp->q_flag & QENAB) {
 		register struct queue *q, *prev = NULL;

		for (q = qhead; q; q = q->q_link)  {
			if (q == qp) {
				if (prev)
					prev->q_link = q->q_link;
				else
					qhead = q->q_link;
				if (q == qtail)
					qtail = prev;
				break;
			}
			prev = q;
		}
	}

	/*
	 * If the write queue of the stream head is pointing to a
	 * read queue, we have a twisted stream.  If the read queue
	 * is alive, convert the stream head queues into a dead end.
	 * If the read queue is dead, free the dead pair.
	 */
	if (qp->q_next && !SAMESTR(qp)) {
		if (qp->q_next->q_qinfo == &deadrend) {	/* half-closed pipe */
			freeq(qp->q_next);
			freeq(RD(qp));
		} else if (qp->q_next == RD(qp)) {	/* fifo */
			freeq(RD(qp));
		} else {				/* pipe */
			qp->q_qinfo = &deadwend;
			RD(qp)->q_qinfo = &deadrend;
		}
	} else {
		freeq(RD(qp)); /* free stream head queue pair */
	}

	if (stp->sd_iocblk) {
		freemsg(stp->sd_iocblk);
		stp->sd_iocblk = NULL;
	}
	stp->sd_vnode = NULL;
	vp->v_stream = NULL;
	stp->sd_flag &= ~STRCLOSE;
	splx(s);
	wakeprocs((caddr_t)stp, PRMPT);
	shfree(stp);
	return (0);
}

/*
 * Clean up after a process when it closes a stream.  This is called
 * from closef for all closes, whereas strclose is called only for the 
 * last close on a stream.  The pollist, siglist, and eventlist are
 * scanned for entries for the current process, and these are removed.
 */
strclean(vp)
	struct vnode *vp;
{
	register struct strevent *sep, *psep, *tsep;
	register int s;
	struct stdata *stp;
	int update = 0;

	stp = vp->v_stream;
	psep = NULL;
	s = splstr();
	sep = stp->sd_siglist;
	while (sep) {
		if (sep->se_procp == u.u_procp) {
			tsep = sep->se_next;
			if (psep)
				psep->se_next = tsep;
			else
				stp->sd_siglist = tsep;
			sefree(sep);
			update = 1;
			sep = tsep;
		} else {
			psep = sep;
			sep = sep->se_next;
		}
	}
	if (update) {
		stp->sd_sigflags = 0;
		for (sep = stp->sd_siglist; sep; sep = sep->se_next)
			stp->sd_sigflags |= sep->se_events;
		update = 0;
	}
	psep = NULL;
	splx(s);
}

/*
 * Read a stream according to the mode flags in sd_flag:
 *
 * (default mode)              - Byte stream, msg boundries are ignored
 * RMSGDIS (msg discard)       - Read on msg boundries and throw away 
 *                               any data remaining in msg
 * RMSGNODIS (msg non-discard) - Read on msg boundries and put back
 *		                 any remaining data on head of read queue
 *
 * Consume readable messages on the front of the queue until u.u_count
 * is satisfied, the readable messages are exhausted, or a message
 * boundary is reached in a message mode.  If no data was read and
 * the stream was not opened with the NDELAY flag, block until data arrives.
 * Otherwise return the data read and update the count.
 *
 * In default mode a 0 length message signifies end-of-file and terminates
 * a read in progress.  The 0 length message is removed from the queue
 * only if it is the only message read (no data is read).
 *
 * An attempt to read an M_PROTO or M_PCPROTO message results in an 
 * EBADMSG error return, unless either RDPROTDAT or RDPROTDIS are set.
 * If RDPROTDAT is set, M_PROTO and M_PCPROTO messages are read as data.
 * If RDPROTDIS is set, the M_PROTO and M_PCPROTO parts of the message
 * are unlinked from and M_DATA blocks in the message, the protos are
 * thrown away, and the data is read.
 */
/* ARGSUSED */
int
strread(vp, uiop, crp)
	struct vnode *vp;
	struct uio *uiop;
	cred_t *crp;
{
	register s;
	register struct stdata *stp;
	register mblk_t *bp, *nbp;
	int n;
	int done = 0;
	int error = 0;
	char rflg;
	short mark;
	short delim;
	unsigned char pri;
	
	ASSERT(vp->v_stream);
	stp = vp->v_stream;

	if (error = straccess(stp, JCREAD))
		return (error);
	if (stp->sd_flag & (STRDERR|STPLEX))
		return ((stp->sd_flag & STPLEX) ? EINVAL : stp->sd_rerror);

	/*
	 * Loop terminates when uiop->uio_resid == 0.
	 */
	rflg = 0;
	for (;;) {
		s = splstr();
		mark = 0;
		delim = 0;
		while (!(bp = getq(RD(stp->sd_wrq)))) {
			if (stp->sd_flag & STRHUP) {
				splx(s);
				return (error);
			}
			if (rflg && !(stp->sd_flag & STRDELIM)) {
				splx(s);
				return (error);
			}

			/*
			 * if FIFO/pipe, don't sleep here. Sleep in the
			 * fifo read routine.
			 */
			if (vp->v_type == VFIFO) {
				splx(s);
				return (ESTRPIPE);
			}

			if ((error = strwaitq(stp, READWAIT, uiop->uio_resid,
			  uiop->uio_fmode, &done)) || done) {
				if ((uiop->uio_fmode & FNDELAY) &&
				    (stp->sd_flag & OLDNDELAY) &&
				    (error == EAGAIN))
					error = 0;
				splx(s);
				return (error);
			}
		}
		if (stp->sd_mark == bp) {
			if (rflg) {
				putbq(RD(stp->sd_wrq), bp);
				splx(s);
				return (error);
			}
			mark = 1;
			stp->sd_mark = NULL;
		}
		if ((stp->sd_flag & STRDELIM) && (bp->b_flag & MSGDELIM))
			delim = 1;
		splx(s);

		pri = bp->b_band;
		if (qready())
			runqueues();

		switch (bp->b_datap->db_type) {

		case M_DATA:
ismdata:
			if (msgdsize(bp) == 0) {
				if (mark || delim) {
					freemsg(bp);
				} else if (rflg) {

					/*
					 * If already read data put zero
					 * length message back on queue else
					 * free msg and return 0.
					 */
					bp->b_band = pri;
					putbq(RD(stp->sd_wrq), bp);
				} else {
					freemsg(bp);
				}
				return (0);
			}

			rflg = 1;
			while (bp && uiop->uio_resid) {
				if (n = MIN(uiop->uio_resid, bp->b_wptr - bp->b_rptr))
					error = uiomove((char *)bp->b_rptr, n, UIO_READ, uiop);
			
				if (error) {
					freemsg(bp);
					return (error);
				}

				bp->b_rptr += n;
				while (bp && (bp->b_rptr >= bp->b_wptr)) {
					nbp = bp;
					bp = bp->b_cont;
					freeb(nbp);
				}
			}
	
			/*
			 * The data may have been the leftover of a PCPROTO, so
			 * if none is left reset the STRPRI flag just in case.
			 */
			if (bp) {
				/* 
				 * Have remaining data in message.
				 * Free msg if in discard mode.
				 */
				if (stp->sd_flag & RMSGDIS) {
					freemsg(bp);
					s = splstr();
					stp->sd_flag &= ~STRPRI;
					splx(s);
				} else {
					s = splstr();
					bp->b_band = pri;
					if (mark && !stp->sd_mark) {
						stp->sd_mark = bp;
						bp->b_flag |= MSGMARK;
					}
					if (delim)
						bp->b_flag |= MSGDELIM;
					putbq(RD(stp->sd_wrq),bp);
					splx(s);
				}
			} else {
				s = splstr();
				stp->sd_flag &= ~STRPRI;
				splx(s);
			}
	
			/*
			 * Check for signal messages at the front of the read
			 * queue and generate the signal(s) if appropriate.
			 * The only signal that can be on queue is M_SIG at
			 * this point.
			 */
			while (((bp = RD(stp->sd_wrq)->q_first)) &&
				(bp->b_datap->db_type == M_SIG)) {
				bp = getq(RD(stp->sd_wrq));
				strsignal(stp, *bp->b_rptr, (long)bp->b_band);
				freemsg(bp);
				if (qready())
					runqueues();
			}
	
			if ((uiop->uio_resid == 0) || mark || delim ||
			    (stp->sd_flag & (RMSGDIS|RMSGNODIS)))
				return (error);
			continue;
		
		case M_PROTO:
		case M_PCPROTO:
			/*
			 * Only data messages are readable.  
			 * Any others generate an error, unless
			 * RDPROTDIS or RDPROTDAT is set.
			 */
			if (stp->sd_flag & RDPROTDAT) {
				for (nbp = bp; nbp; nbp = bp->b_next)
					nbp->b_datap->db_type = M_DATA;
				s = splstr();
				stp->sd_flag &= ~STRPRI;
				splx(s);
				goto ismdata;
			} else if (stp->sd_flag & RDPROTDIS) {
				while (bp &&
				    ((bp->b_datap->db_type == M_PROTO) ||
				    (bp->b_datap->db_type == M_PCPROTO))) {
					nbp = unlinkb(bp);
					freeb(bp);
					bp = nbp;
				}
				s = splstr();
				stp->sd_flag &= ~STRPRI;
				splx(s);
				if (bp) {
					bp->b_band = pri;
					goto ismdata;
				} else {
					break;
				}
			}
			/* fall thru */

		case M_PASSFP:
			if ((bp->b_datap->db_type == M_PASSFP) &&
			    (stp->sd_flag & RDPROTDIS)) {
				closef(((struct strrecvfd *)bp->b_rptr)->f.fp);
				freemsg(bp);
				break;
			}
			putbq(RD(stp->sd_wrq), bp);
			return (EBADMSG);

		default:
			/*
			 * Garbage on stream head read queue.
			 */
			ASSERT(0);
			freemsg(bp);
			break;
		}
	}

	/* NOTREACHED */
}

/*
 * Stream read put procedure.  Called from downstream driver/module
 * with messages for the stream head.  Data, protocol, and in-stream
 * signal messages are placed on the queue, others are handled directly.
 */

static
strrput(q, bp)
	register queue_t *q;
	register mblk_t *bp;
{
	register struct stdata *stp;
	register struct iocblk *iocbp;
	register int s;
	struct stroptions *sop;
	struct copyreq *reqp;
	struct copyresp *resp;
	struct striopst *sp;
	unsigned char bpri;
	qband_t *qbp;

	stp = (struct stdata *)q->q_ptr;

	ASSERT(!(stp->sd_flag & STPLEX));

	switch (bp->b_datap->db_type) {

	case M_DATA:
	case M_PROTO:
	case M_PCPROTO:
	case M_PASSFP:
		s = splstr();
		if (bp->b_datap->db_type == M_PCPROTO) {
			/*
			 * Only one priority protocol message is allowed at the
			 * stream head at a time.
			 */
			if (stp->sd_flag & STRPRI) {
				freemsg(bp);
				splx(s);
				return;
			}
		}

		/*
		 * Marking doesn't work well when messages
		 * are marked in more than one band.  We only
		 * remember the last message received, even if
		 * it is placed on the queue ahead of other
		 * marked messages.
		 */
		if (bp->b_flag & MSGMARK)
			stp->sd_mark = bp;

		/* 
		 * Wake sleeping read/getmsg
		 */
		if (stp->sd_flag & RSLEEP) {
			stp->sd_flag &= ~RSLEEP;
			if (stp->sd_vnode->v_type == VFIFO)
				wakeprocs((caddr_t)q, NOPRMPT); /*don't thrash*/
			else
				wakeprocs((caddr_t)q, PRMPT);
		}

		putq(q, bp);

		if (bp->b_datap->db_type == M_PCPROTO) {
			stp->sd_flag |= STRPRI;
			if (stp->sd_sigflags & S_HIPRI)
				strsendsig(stp->sd_siglist, S_HIPRI, 0L);
			if (stp->sd_pollist.ph_events & POLLPRI) 
				pollwakeup(&stp->sd_pollist, POLLPRI);
		} else if (q->q_first == bp) {
			if (stp->sd_sigflags & S_INPUT)
				strsendsig(stp->sd_siglist, S_INPUT,
				    (long)bp->b_band);
			if (stp->sd_pollist.ph_events & POLLIN) 
				pollwakeup(&stp->sd_pollist, POLLIN);
			if (bp->b_band == 0) {
			    if (stp->sd_sigflags & S_RDNORM)
				    strsendsig(stp->sd_siglist, S_RDNORM, 0L);
			    if (stp->sd_pollist.ph_events & POLLRDNORM) 
				    pollwakeup(&stp->sd_pollist, POLLRDNORM);
			} else {
			    if (stp->sd_sigflags & S_RDBAND)
				    strsendsig(stp->sd_siglist, S_RDBAND,
					(long)bp->b_band);
			    if (stp->sd_pollist.ph_events & POLLRDBAND) 
				    pollwakeup(&stp->sd_pollist, POLLRDBAND);
			}
		}

		splx(s);
		return;


	case M_ERROR:
		/* 
		 * An error has occured downstream, the errno is in the first
		 * byte of the message.
		 */
		if ((bp->b_wptr - bp->b_rptr) == 2) {	/* New flavor */
			unsigned char rw = 0;

			s = splstr();
			if (*bp->b_rptr != NOERROR) {	/* read error */
				if (*bp->b_rptr != 0) {
					stp->sd_flag |= STRDERR;
					rw |= FLUSHR;
				} else {
					stp->sd_flag &= ~STRDERR;
				}
				stp->sd_rerror = *bp->b_rptr;
			}
			bp->b_rptr++;
			if (*bp->b_rptr != NOERROR) {	/* write error */
				if (*bp->b_rptr != 0) {
					stp->sd_flag |= STWRERR;
					rw |= FLUSHW;
				} else {
					stp->sd_flag &= ~STWRERR;
				}
				stp->sd_werror = *bp->b_rptr;
			}
			splx(s);
			if (rw) {
				wakeprocs((caddr_t)q, PRMPT); /* readers */
				wakeprocs((caddr_t)WR(q), PRMPT); /* writers */
				wakeprocs((caddr_t)stp, PRMPT);	/* ioctllers */

				s = splstr();
				if (stp->sd_sigflags & S_ERROR) 
					strsendsig(stp->sd_siglist, S_ERROR,
					    ((rw & FLUSHR) ?
					    (long)stp->sd_rerror :
					    (long)stp->sd_werror));
				pollwakeup(&stp->sd_pollist, POLLERR);
				splx(s);
				freemsg(bp);
				(void) putctl1(WR(q)->q_next, M_FLUSH, rw);
				return;
			}
		} else if (*bp->b_rptr != 0) {		/* Old flavor */
			s = splstr();
			stp->sd_flag |= (STRDERR|STWRERR);
			stp->sd_rerror = *bp->b_rptr;
			stp->sd_werror = *bp->b_rptr;
			splx(s);
			wakeprocs((caddr_t)q, PRMPT); /* the readers */
			wakeprocs((caddr_t)WR(q), PRMPT); /* the writers */
			wakeprocs((caddr_t)stp, PRMPT); /* the ioctllers */

			s = splstr();
			if (stp->sd_sigflags & S_ERROR) 
				strsendsig(stp->sd_siglist, S_ERROR,
				    (stp->sd_werror ? (long)stp->sd_werror :
				    (long)stp->sd_rerror));
			pollwakeup(&stp->sd_pollist, POLLERR);
			splx(s);
			freemsg(bp);
			(void) putctl1(WR(q)->q_next, M_FLUSH, FLUSHRW);
			return;
		}
		freemsg(bp);
		return;

	case M_HANGUP:
	
		freemsg(bp);
		s = splstr();
		stp->sd_werror = ENXIO;
		stp->sd_flag |= STRHUP;
		splx(s);

		/*
		 * send signal if controlling tty
		 */
                if (stp->sd_sidp) {
                        prsignal(stp->sd_sidp, SIGHUP);
                        if (stp->sd_sidp != stp->sd_pgidp)
                                pgsignal(stp->sd_pgidp, SIGTSTP);
		}

		wakeprocs((caddr_t)q, PRMPT);	/* the readers */
		wakeprocs((caddr_t)WR(q), PRMPT);	/* the writers */
		wakeprocs((caddr_t)stp, PRMPT);	/* the ioctllers */

		/*
		 * wake up read, write, and exception pollers and
		 * reset wakeup mechanism.
		 */

		strhup(stp);
		return;


	case M_SIG:
		/*
		 * Someone downstream wants to post a signal.  The
		 * signal to post is contained in the first byte of the
		 * message.  If the message would go on the front of
		 * the queue, send a signal to the process group
		 * (if not SIGPOLL) or to the siglist processes
		 * (SIGPOLL).  If something is already on the queue,
		 * just enqueue the message.
		 */
		if (q->q_first) {
			putq(q, bp);
			return;
		}
		/* flow through */

	case M_PCSIG:
		/*
		 * Don't enqueue, just post the signal.
		 */
		strsignal(stp, *bp->b_rptr, 0L);
		freemsg(bp);
		return;

	case M_FLUSH:
		/*
		 * Flush queues.  The indication of which queues to flush
		 * is in the first byte of the message.  If the read queue 
		 * is specified, then flush it.  If FLUSHBAND is set, just
		 * flush the band specified by the second byte of the message.
		 */
	    {
		register mblk_t *mp;
		register mblk_t *nmp;
		mblk_t *last;
		int backenable = 0;
		int band;
		queue_t *nq;
		unsigned char pri, i;


		s = splstr();
		if (*bp->b_rptr & FLUSHR) {
		    if (*bp->b_rptr & FLUSHBAND) {
			ASSERT((bp->b_wptr - bp->b_rptr) >= 2);
			pri = *(bp->b_rptr + 1);
			if (pri > q->q_nband)
			    goto wrapflush;
			if (pri == 0) {
			    mp = q->q_first;
			    q->q_first = NULL;
			    q->q_last = NULL;
			    q->q_count = 0;
			    for (qbp = q->q_bandp; qbp; qbp = qbp->qb_next) {
				qbp->qb_first = NULL;
				qbp->qb_last = NULL;
				qbp->qb_count = 0;
				qbp->qb_flag &= ~QB_FULL;
			    }
			    q->q_blocked = 0;
			    q->q_flag &= ~QFULL;
			    while (mp) {
				nmp = mp->b_next;
				if (mp->b_band == 0) {
				    if (mp->b_datap->db_type == M_PASSFP) 
					closef(((struct strrecvfd *)
					  mp->b_rptr)->f.fp);
				    freemsg(mp);
				} else {
					putq(q, mp);
				}
				mp = nmp;
			    }
			    if ((q->q_flag & QWANTW) &&
			      (q->q_count <= q->q_lowat)) {
				/* find nearest back queue with service proc */
				q->q_flag &= ~QWANTW;
				for (nq = backq(q); nq && !nq->q_qinfo->qi_srvp;
				  nq = backq(nq))
				    ;
				if (nq) {
				    qenable(nq);
				    setqback(nq, 0);
				}
			    }
			} else {	/* pri != 0 */
			    band = pri;
			    qbp = q->q_bandp;
			    while (--band > 0)
				qbp = qbp->qb_next;
			    mp = qbp->qb_first;
			    if (mp == NULL)
				goto wrapflush;
			    last = qbp->qb_last;
			    if (mp == last)	/* only message in band */
				last = mp->b_next;
			    while (mp != last) {
				nmp = mp->b_next;
				if (mp->b_band == pri) {
				    if (mp->b_datap->db_type == M_PASSFP) 
					closef(((struct strrecvfd *)
					  mp->b_rptr)->f.fp);
				    rmvq(q, mp);
				    freemsg(mp);
				}
				mp = nmp;
			    }
			    if (mp && mp->b_band == pri) {
				if (mp->b_datap->db_type == M_PASSFP) 
				    closef(((struct strrecvfd *)
				      mp->b_rptr)->f.fp);
				rmvq(q, mp);
				freemsg(mp);
			    }
			}
		    } else {	/* flush entire queue */
			mp = q->q_first;
			q->q_first = NULL;
			q->q_last = NULL;
			q->q_count = 0;
			stp->sd_flag &= ~STRPRI;
			for (qbp = q->q_bandp; qbp; qbp = qbp->qb_next) {
			    qbp->qb_first = NULL;
			    qbp->qb_last = NULL;
			    qbp->qb_count = 0;
			    qbp->qb_flag &= ~QB_FULL;
			}
			q->q_blocked = 0;
			q->q_flag &= ~QFULL;
			while (mp) {
			    nmp = mp->b_next;
			    if (mp->b_datap->db_type == M_PASSFP) 
				closef(((struct strrecvfd *)mp->b_rptr)->f.fp);
			    freemsg(mp);
			    mp = nmp;
			}
			bzero((caddr_t)qbf, NBAND);
			bpri = 1;
			for (qbp = q->q_bandp; qbp; qbp = qbp->qb_next) {
			    if ((qbp->qb_flag & QB_WANTW) &&
			      (qbp->qb_count <= qbp->qb_lowat)) {
				qbp->qb_flag &= ~QB_WANTW;
				backenable = 1;
				qbf[bpri] = 1;
			    }
			    bpri++;
			}
			if ((q->q_flag & QWANTW) && (q->q_count <= q->q_lowat)) {
			    q->q_flag &= ~QWANTW;
			    backenable = 1;
			    qbf[0] = 1;
			}

			/*
			 * If any band can now be written to, and there is a
			 * writer for that band, then backenable the closest
			 * service procedure.
			 */
			if (backenable) {
			    /* find nearest back queue with service proc */
			    for (nq = backq(q); nq && !nq->q_qinfo->qi_srvp;
			      nq = backq(nq))
				;
			    if (nq) {
				qenable(nq);
				for (i = 0; i < bpri; i++)
				    if (qbf[i])
					setqback(nq, i);
			    }
			}
		    }
		}
wrapflush:
		splx(s);
		if ((*bp->b_rptr & FLUSHW) && !(bp->b_flag & MSGNOLOOP)) {
			*bp->b_rptr &= ~FLUSHR;
			bp->b_flag |= MSGNOLOOP;
			qreply(q, bp);
			return;
		}
		freemsg(bp);
		return;
	    }

	case M_IOCACK:
	case M_IOCNAK:
		iocbp = (struct iocblk *)bp->b_rptr;
		/*
		 * If not waiting for ACK or NAK then just free msg.
		 * If already have ACK or NAK for user then just free msg.
		 * If incorrect id sequence number then just free msg.
		 */
		s = splstr();
		if ((stp->sd_flag & IOCWAIT) == 0 || stp->sd_iocblk ||
		    (stp->sd_iocid != iocbp->ioc_id)) {
			/* toss associated post-processing request */
			sp = (struct striopst *)findioc(iocbp->ioc_id);
			if (sp)
				kmem_free(sp, sizeof(struct striopst));
			freemsg(bp);
			splx(s);
			return;
		}

		/*
		 * Assign ACK or NAK to user and wake up.
		 */
		stp->sd_iocblk = bp;
		splx(s);
		wakeprocs((caddr_t)stp, PRMPT);
		return;

	case M_COPYIN:
	case M_COPYOUT:
		reqp = (struct copyreq *)bp->b_rptr;

		/*
		 * If not waiting for ACK or NAK then just fail request.
		 * If already have ACK, NAK, or copy request, then just
		 * fail request.
		 * If incorrect id sequence number then just fail request.
		 */
		s = splstr();
		if ((stp->sd_flag & IOCWAIT) == 0 || stp->sd_iocblk ||
		    (stp->sd_iocid != reqp->cq_id)) {
			if (bp->b_cont) {
				freemsg(bp->b_cont);
				bp->b_cont = NULL;
			}
			bp->b_datap->db_type = M_IOCDATA;
			resp = (struct copyresp *)bp->b_rptr;
			resp->cp_rval = (caddr_t)1;	/* failure */
			/* toss associated post-processing request */
			sp = (struct striopst *)findioc(reqp->cq_id);
			if (sp)
				kmem_free(sp, sizeof(struct striopst));
			splx(s);
			(*stp->sd_wrq->q_next->q_qinfo->qi_putp)(stp->sd_wrq->q_next, bp);
			if (qready())
				runqueues();
			return;
		}

		/*
		 * Assign copy request to user and wake up.
		 */
		stp->sd_iocblk = bp;
		splx(s);
		wakeprocs((caddr_t)stp, PRMPT);
		return;

	case M_SETOPTS:
		/*
		 * Set stream head options (read option, write offset,
		 * min/max packet size, and/or high/low water marks for 
		 * the read side only).
		 */

		bpri = 0;
		ASSERT((bp->b_wptr - bp->b_rptr) == sizeof(struct stroptions));
		sop = (struct stroptions *)bp->b_rptr;
		s = splstr();
		if (sop->so_flags & SO_READOPT) {
			switch (sop->so_readopt & RMODEMASK) {
			case RNORM: 
				stp->sd_flag &= ~(RMSGDIS | RMSGNODIS);
				break;

			case RMSGD:
				stp->sd_flag = ((stp->sd_flag & ~RMSGNODIS) |
				    RMSGDIS);
				break;

			case RMSGN:
				stp->sd_flag = ((stp->sd_flag & ~RMSGDIS) |
				    RMSGNODIS);
				break;
			}
			switch(sop->so_readopt & RPROTMASK) {
			case RPROTNORM:
				stp->sd_flag &= ~(RDPROTDAT | RDPROTDIS);
				break;

			case RPROTDAT:
				stp->sd_flag = ((stp->sd_flag & ~RDPROTDIS) |
				    RDPROTDAT);
				break;

			case RPROTDIS:
				stp->sd_flag = ((stp->sd_flag & ~RDPROTDAT) |
				    RDPROTDIS);
				break;
			}
		}
				
		if (sop->so_flags & SO_WROFF)
			stp->sd_wroff = sop->so_wroff;
		if (sop->so_flags & SO_MINPSZ)
			q->q_minpsz = sop->so_minpsz;
		if (sop->so_flags & SO_MAXPSZ)
			q->q_maxpsz = sop->so_maxpsz;
		if (sop->so_flags & SO_HIWAT) {
		    if (sop->so_flags & SO_BAND) {
			if (strqset(q, QHIWAT, sop->so_band, sop->so_hiwat))
				cmn_err(CE_WARN,
				    "strrput: could not allocate qband\n");
			else
				bpri = sop->so_band;
		    } else {
			q->q_hiwat = sop->so_hiwat;
		    }
		}
		if (sop->so_flags & SO_LOWAT) {
		    if (sop->so_flags & SO_BAND) {
			if (strqset(q, QLOWAT, sop->so_band, sop->so_lowat))
				cmn_err(CE_WARN,
				    "strrput: could not allocate qband\n");
			else
				bpri = sop->so_band;
		    } else {
			q->q_lowat = sop->so_lowat;
		    }
		}
		if (sop->so_flags & SO_MREADON)
			stp->sd_flag |= SNDMREAD;
		if (sop->so_flags & SO_MREADOFF)
			stp->sd_flag &= ~SNDMREAD;
		if (sop->so_flags & SO_NDELON)
			stp->sd_flag |= OLDNDELAY;
		if (sop->so_flags & SO_NDELOFF)
			stp->sd_flag &= ~OLDNDELAY;
		if (sop->so_flags & SO_ISTTY)
			stp->sd_flag |= STRISTTY;
		if (sop->so_flags & SO_ISNTTY)
			stp->sd_flag &= ~STRISTTY;
		if (sop->so_flags & SO_TOSTOP) 
			stp->sd_flag |= STRTOSTOP;
		if (sop->so_flags & SO_TONSTOP) 
			stp->sd_flag &= ~STRTOSTOP;
		if (sop->so_flags & SO_DELIM)
			stp->sd_flag |= STRDELIM;
#ifndef _STYPES
		if (sop->so_flags & SO_NODELIM)
			stp->sd_flag &= ~STRDELIM;
		if (sop->so_flags & SO_STRHOLD)
			stp->sd_flag |= STRHOLD;
#endif /* _STYPES */

		freemsg(bp);

		if (bpri == 0) {
			if ((q->q_count <= q->q_lowat) &&
			    (q->q_flag & QWANTW)) {
				q->q_flag &= ~QWANTW;
				for (q = backq(q); q && !q->q_qinfo->qi_srvp;
				    q = backq(q))
					;
				if (q) {
					qenable(q);
					setqback(q, bpri);
				}
			}
		} else {
			unsigned char i;

			qbp = q->q_bandp;
			for (i = 1; i < bpri; i++)
				qbp = qbp->qb_next;
			if ((qbp->qb_count <= qbp->qb_lowat) &&
			    (qbp->qb_flag & QB_WANTW)) {
				qbp->qb_flag &= ~QB_WANTW;
				for (q = backq(q); q && !q->q_qinfo->qi_srvp;
				    q = backq(q))
					;
				if (q) {
					qenable(q);
					setqback(q, bpri);
				}
			}
		}
		splx(s);

		return;

	/*
	 * The following set of cases deal with situations where two stream
	 * heads are connected to each other (twisted streams).  These messages
	 * have no meaning at the stream head.
	 */
	case M_BREAK:
	case M_CTL:
	case M_DELAY:
	case M_START:
	case M_STOP:
	case M_IOCDATA:
	case M_STARTI:
	case M_STOPI:
		freemsg(bp);
		return;

	case M_IOCTL:
		/*
		 * Always NAK this condition
		 * (makes no sense)
		 */
		bp->b_datap->db_type = M_IOCNAK;
		qreply(q, bp);
		return;

	default:
		ASSERT(0);
		freemsg(bp);
		return;
	}
}

/*
 * Write attempts to break the read request into messages conforming
 * with the minimum and maximum packet sizes set downstream.  
 *
 * Write will always attempt to get the largest buffer it can to satisfy the
 * message size. If it can not, then it will try up to 2 classes down to try
 * to satisfy the write. Write will not block if downstream queue is full and
 * O_NDELAY is set, otherwise it will block waiting for the queue to get room.
 * 
 * A write of zero bytes gets packaged into a zero length message and sent
 * downstream like any other message.
 *
 * If buffers of the requested sizes are not available, the write will
 * sleep until the buffers become available.
 *
 * Write (if specified) will supply a write offset in a message if it
 * makes sense. This can be specified by downstream modules as part of
 * a M_SETOPTS message.  Write will not supply the write offset if it
 * cannot supply any data in a buffer.  In other words, write will never
 * send down an empty packet due to a write offset.
 */
int
strwrite(vp, uiop, crp)
	struct vnode *vp;
	struct uio *uiop;
	cred_t *crp;
{
	register struct stdata *stp;
	register struct queue *wqp;
	register mblk_t *mp;
	register int s;
	long rmin, rmax;
	long iosize;
	char waitflag;
	int tempmode;
	int done = 0;
	int error = 0;
	int strmakemsg();

	ASSERT(vp->v_stream);
	stp = vp->v_stream;

	if (error = straccess(stp, JCWRITE))
		return (error);
	if (stp->sd_flag & STPLEX)
		return (EINVAL);
	if (stp->sd_flag & (STWRERR|STRHUP)) {
		if (stp->sd_flag & STRSIGPIPE)
			psignal(u.u_procp, SIGPIPE);

		/*
		 * this is for POSIX compatibility
		 */
		return ((stp->sd_flag & STRHUP) ? EIO : stp->sd_werror);
	}

	/*
	 * firewall - don't use too much memory
	 */
	if (strthresh && (Strcount > strthresh) && pm_denied(u.u_cred, P_SYSOPS))
		return (ENOSR);

	/*
	 * Check the min/max packet size constraints.  If min packet size
	 * is non-zero, the write cannot be split into multiple messages
	 * and still guarantee the size constraints. 
	 */
	wqp = stp->sd_wrq;
	rmin = wqp->q_next->q_minpsz;
	rmax = wqp->q_next->q_maxpsz;
	ASSERT((rmax >= 0) || (rmax == INFPSZ));
	if (rmax == 0)
		return (0);
	if (strmsgsz != 0) {
		if (rmax == INFPSZ)
			rmax = strmsgsz;
		else  {
			if (vp->v_type == VFIFO)
				rmax = MIN(PIPE_BUF, rmax);
			else	rmax = MIN(strmsgsz, rmax);
		}
	}
	if (rmin > 0) {
		if (uiop->uio_resid < rmin)
			return (ERANGE);
	    	if ((rmax != INFPSZ) && (uiop->uio_resid > rmax))
			return (ERANGE);
	}

	/*
	 * Do until count satisfied or error.
	 */
	waitflag = WRITEWAIT;
	if (stp->sd_flag & OLDNDELAY)
		tempmode = uiop->uio_fmode & ~FNDELAY;
	else
		tempmode = uiop->uio_fmode;

	do {
		register int size = uiop->uio_resid;
		mblk_t *amp;	/* auto */

		s = splstr();
		while (!bcanput(wqp->q_next, 0)) {

			/*
			 * if FIFO/pipe, don't sleep here. Sleep in the
			 * fifo write routine.
			 */
			if (vp->v_type == VFIFO) {
				splx(s);
				return (ESTRPIPE);
			}
			if ((error = strwaitq(stp, waitflag, (off_t)0,
			    tempmode, &done)) || done) {
				splx(s);
				return (error);
			}
		}

		/* still at splstr */
		if (mp = wqp->q_first)  {
			int spaceleft;
			/*
			 * Grab the previously held message "mp" and see
			 * if there's room in it to hold this write's data.
			 */
			spaceleft = mp->b_datap->db_lim - mp->b_wptr;
			if (size <= spaceleft)  {
				if (error = uiomove(mp->b_wptr, size, UIO_WRITE, uiop))  {
					splx(s);
					return error;
				}
				mp->b_wptr += size;
				/*
				 * If the buffer would hold yet another write
				 * of the same size, return.  (wait a while)
				 */
				if ((size<<1) <= spaceleft)  {
					splx(s);
					return 0;
				}
			}

			wqp->q_first = NULL;
			splx(s);
		}
		else  {
			splx(s);

			/*
			 * Determine the size of the next message to be
			 * packaged.  May have to break write into several
			 * messages based on max packet size.
			 */
			if (rmax == INFPSZ)
				iosize = uiop->uio_resid;
			else	iosize = MIN(uiop->uio_resid, rmax);

			if ((error = strmakemsg((struct strbuf *)NULL,
			    iosize, uiop, stp, (long)0, &amp)) || !amp)
				return (error);
			mp = amp;

			/*
			 * When explicitly enabled (typically for TTYs), check to
			 * see if data might be coalesced for better performance.
			 *
			 * Policy:  Use the size of this write as predictor of the
			 * size of the next write.  If this msg buffer has space for
			 * another write of the same size, hold onto it for a while.
			 */
			if (strhold && (stp->sd_flag & STRHOLD) &&
			   size <= (mp->b_datap->db_lim - mp->b_wptr))  {

				extern struct queue *scanqhead, *scanqtail;
				extern strscanflag;

				splstr();
				wqp->q_first = mp;
				stp->sd_rtime = lbolt + STRSCANP;

				/*
				 * Add queue to held-message list
				 */
				if (!(wqp->q_flag & QHLIST))  {
					wqp->q_flag |= QHLIST;

					if (!scanqhead)
						scanqhead = wqp;
					else
						scanqtail->q_link = wqp;

					scanqtail = wqp;
					wqp->q_link = NULL;

					if (!strscanflag)  {
						strscanflag++;
						timeout(strscan, 0, STRSCANP);
					}
				}
				splx(s);
				return 0;
			}
		}

		/*
		 * Put block downstream.
		 */
		wqp->q_first = NULL;
		if ((uiop->uio_resid == 0) && (stp->sd_flag & STRDELIM))
			mp->b_flag |= MSGDELIM;
		(*wqp->q_next->q_qinfo->qi_putp)
			(wqp->q_next, mp);
		waitflag |= NOINTR;
		if (qready())
			runqueues();

	} while (uiop->uio_resid);

	return 0;
}

/*
 * Stream head write service routine.
 * Its job is to wake up any sleeping writers when a queue
 * downstream needs data (part of the flow control in putq and getq).
 * It also must wake anyone sleeping on a poll().
 * For stream head right below mux module, it must also invoke put procedure
 * of next downstream module.
 */

strwsrv(q)
	register queue_t *q;
{
	register struct stdata *stp;
	register int s;
	register queue_t *tq;
	register qband_t *qbp;
	register int i;
	qband_t *myqbp;
	int isevent;

	stp = (struct stdata *)q->q_ptr;
	ASSERT(!(stp->sd_flag & STPLEX));
	s = splstr();

	if (stp->sd_flag & WSLEEP) {
		stp->sd_flag &= ~WSLEEP;
		if (stp->sd_vnode->v_type == VFIFO)
			wakeprocs((caddr_t)q, NOPRMPT); /*don't thrash*/
		else
			wakeprocs((caddr_t)q, PRMPT);
	}

	if ((tq = q->q_next) == NULL) {

		/*
		 * The other end of a stream pipe went away.
		 */
		splx(s);
		return;
	}
	while (tq->q_next && !tq->q_qinfo->qi_srvp)
		tq = tq->q_next;

	if (q->q_flag & QBACK) {
		if (tq->q_flag & QFULL) {
			tq->q_flag |= QWANTW;
		} else {
			if (stp->sd_sigflags & S_WRNORM)
				strsendsig(stp->sd_siglist, S_WRNORM, 0L);
			if (stp->sd_pollist.ph_events & POLLWRNORM)
				pollwakeup(&stp->sd_pollist, POLLWRNORM);
		}
	}

	isevent = 0;
	i = 1;
	bzero((caddr_t)qbf, NBAND);
	myqbp = q->q_bandp;
	for (qbp = tq->q_bandp; qbp; qbp = qbp->qb_next) {
		if (!myqbp)
			break;
		if (myqbp->qb_flag & QB_BACK) {
			if (qbp->qb_flag & QB_FULL) {
				qbp->qb_flag |= QB_WANTW;
			} else {
				isevent = 1;
				qbf[i] = 1;
			}
		}
		myqbp = myqbp->qb_next;
		i++;
	}
	while (myqbp) {
		if (myqbp->qb_flag & QB_BACK) {
			isevent = 1;
			qbf[i] = 1;
		}
		myqbp = myqbp->qb_next;
		i++;
	}

	if (isevent) {
	    for (i--; i; i--) {
		if (qbf[i]) {
			if (stp->sd_sigflags & S_WRBAND)
				strsendsig(stp->sd_siglist, S_WRBAND, (long)i);
			if (stp->sd_pollist.ph_events & POLLWRBAND)
				pollwakeup(&stp->sd_pollist, POLLWRBAND);
		}
	    }
	}

	splx(s);
}

/*
 * ioctl for streams
 */
int
strioctl(vp, cmd, arg, flag, copyflag, crp, rvalp)
	struct vnode *vp;
	int cmd;
	int arg;
	int flag;
	int copyflag;
	cred_t *crp;
	int *rvalp;
{
	register struct stdata *stp;
	register int s;
	struct strioctl strioc;
	struct uio uio;
	struct iovec iov;
	enum jcaccess access;
	mblk_t *mp;
	int error = 0;
	int done = 0;
	char *fmt = NULL;
#ifdef VPIX
	struct v86blk *p_v86blk;
#endif
 	mblk_t *bp1 = NULL;
	int	keymap_type = -1;
	caddr_t event_qaddr = NULL;
	int	sco_xioc_cmd = 0;
	extern void adt_recvfd();	/* audit recording function */
	extern int audit_on;		/* true if auditing is enabled */

	ASSERT(vp->v_stream);
	ASSERT(copyflag == U_TO_K || copyflag == K_TO_K);
	stp = vp->v_stream;

	/* Enhanced Application Compatibility Support */

	/* It if is an ISC POSIX exec then translate ISC POSIX values
	 * to that of SVR4.
	 * If is a SCO exec AND it is a SCO Old XIOC termios
	 * translate it to the BSC version 
	 */

#define	LONGIOCTYPE	0xffffff00

	if (ISC_USES_POSIX) {
		if ((cmd & LONGIOCTYPE) == ISC_TIOC ) {
			if (cmd == ISC_TCGETPGRP) {
				/* This check is made in SVR4.0 Library */
				if (stp->sd_sidp != 
				    u.u_procp->p_sessp->s_sidp)
					return ENOTTY;
				cmd = TIOCGPGRP;
			} else if (cmd == ISC_TCSETPGRP)
				cmd = TIOCSPGRP;
		}
	} else if (IS_SCOEXEC)  {
		switch( cmd & LONGIOCTYPE) {
		case LDEV_MOUSE: 
		case EVLD_IOC: 
			switch(cmd){
			case LDEV_ATTACHQ:
			case LDEV_MSEATTACHQ:
                        	{
                               	 file_t *fp;

                               	 if (getf(arg, &fp))
                                        arg = -1;
					else arg = fp->f_vnode->v_rdev;
                        	}
                        break;
                        case LDEV_GETEV:
                        case LDEV_SETRATIO:
                        case LDEV_SETTYPE:
                        break;
                }
		break;
                case    EQIOC:
                	switch(cmd){
                        	case    EQIO_GETQP:
					event_qaddr = (caddr_t) arg;
                                	arg = vp->v_rdev;
                	}
		break;
		case SCO_OXIOC:
			/* stri386ioctl needs the original cmd */

			sco_xioc_cmd = (cmd & ~IOCTYPE) | SCO_XIOC;
			break;
		case SCO_TIOC:
			if (cmd == SCO_TIOCGPGRP) {
				/* This check is made in SVR4.0 Library */
				if (stp->sd_sidp != 
				    u.u_procp->p_sessp->s_sidp)
					return ENOTTY;
				cmd = TIOCGPGRP;
			} else if (cmd == SCO_TIOCSPGRP)
				cmd = TIOCSPGRP;
			break;
		case SCO_OLD_C_IOC:
			cmd = (cmd & ~IOCTYPE) | SCO_C_IOC;
			break;
		case MIOC:
		case WSIOC:
			switch(cmd) {
			case GIO_KEYMAP:
			case KDDFLTKEYMAP:
			case PIO_KEYMAP:
				keymap_type = SCO_FORMAT;
				break;
			}
		}
	}
	else {
		switch( cmd & LONGIOCTYPE) {
		case MIOC:
		case WSIOC:
			switch(cmd) {
			case GIO_KEYMAP:
			case KDDFLTKEYMAP:
			case PIO_KEYMAP:
				keymap_type = USL_FORMAT;
				break;
			}
			break;
		}
	}
	/* End Enhanced Application Compatibility Support */

 	if(stri386ioctl(vp, &cmd, arg, rvalp, &error))
		return (error);

/* Enhanced Application Compatibility Support */

	/* stri386ioctl needs the original cmd */

	if(sco_xioc_cmd ){
		if((cmd & MODESWITCH) != MODESWITCH)
			cmd = sco_xioc_cmd;
	}
/* End Enhanced Application Compatibility Support */

	/*
	 *  if a message is being "held" awaiting possible consolidation,
	 *  send it downstream before processing ioctl.
	 */
	{
		register queue_t *q = stp->sd_wrq;

		s = splstr();
		if (mp = q->q_first)  {
			q->q_first = NULL;

			splx(s);
			putnext(q, mp);
		}
		else	splx(s);
	}

	switch (cmd) {
	case I_RECVFD:
	case I_E_RECVFD:
	case I_S_RECVFD:
		access = JCREAD;
		break;

	case I_FDINSERT:
	case I_SENDFD:
	case TIOCSTI:
		access = JCWRITE;
		break;

	/* Enhanced Application Compatibility Support */
	case SCO_XCGETA:
	/* End Enhanced Application Compatibility Support */
	case TCGETA:
	case TCGETS:
	case TIOCGETP:
	case TIOCGPGRP:
	case JWINSIZE:
	case TIOCGSID:
	case TIOCMGET:
	case LDGETT:
	case TIOCGETC:
	case TIOCLGET:
	case TIOCGLTC:
	case TIOCGETD:
	case TIOCGWINSZ:
	case LDGMAP:
	case I_CANPUT:
	case I_NREAD:
	case FIONREAD:
	case FIORDCHK:
	case I_FIND:
	case I_LOOK:
	case I_GRDOPT:
	case I_GETSIG:
	case I_PEEK:
	case I_GWROPT:
	case I_LIST:
	case I_CKBAND:
	case I_GETBAND:
	case I_GETCLTIME:
		access = JCGETP;
		break;

#ifdef VPIX
/*
** The following ioctl's have been added to the 'JCGETP' class to allow 
** VPIX to run in the background under job control.  This works since
** VPIX will open a new VT to run in anyway.
*/
	case KDGKBTYPE:
	case KIOCINFO:
	case KDGETLED:
	case VT_OPENQRY:
	case MOUSEIOCREAD:
		access = JCGETP;
		break;

	case KDSETLED:
		if (u.u_procp->p_v86)
			access = JCGETP;
		else
			access = JCSETP;
		break;
#endif /* VPIX */

	default:
		access = JCSETP;
		break;
	}


	if (error = straccess(stp, access))
		return (error);
	if (stp->sd_flag & (STRDERR|STWRERR|STPLEX))
		return ((stp->sd_flag & STPLEX) ? EINVAL :
		    (stp->sd_werror ? stp->sd_werror : stp->sd_rerror));

	switch (cmd) {

	default:
		if (((cmd & IOCTYPE) == LDIOC) ||
		    ((cmd & IOCTYPE) == tIOC) ||
		    ((cmd & IOCTYPE) == TIOC)) {

			/*
			 * The ioctl is a tty ioctl - set up strioc buffer 
			 * and call strdoioctl() to do the work.
			 */
			if (stp->sd_flag & STRHUP)
				return (ENXIO);
			strioc.ic_cmd = cmd;
			strioc.ic_timout = INFTIM;

			switch (cmd) {
			case TCXONC:
			case TCSBRK:
			case TCFLSH:
			case TCDSET:
				strioc.ic_len = sizeof(int);
				strioc.ic_dp = (char *)&arg;
				return (strdoioctl(stp, &strioc, NULL, K_TO_K,
				    STRINT, crp, rvalp));

			case TCSETA:
			case TCSETAW:
			case TCSETAF:
				strioc.ic_len = sizeof(struct termio);
				strioc.ic_dp = (char *)arg;
				return (strdoioctl(stp, &strioc, NULL,
				    copyflag, STRTERMIO, crp, rvalp));

			case TCSETS:
			case TCSETSW:
			case TCSETSF:
				strioc.ic_len = sizeof(struct termios);
				strioc.ic_dp = (char *)arg;
				return (strdoioctl(stp, &strioc, NULL,
				    copyflag, STRTERMIOS, crp, rvalp));

			case LDSETT:
				strioc.ic_len = sizeof(struct termcb);
				strioc.ic_dp = (char *)arg;
				return (strdoioctl(stp, &strioc, NULL,
				    copyflag, STRTERMCB, crp, rvalp));

			case TIOCSETP:
				strioc.ic_len = sizeof(struct sgttyb);
				strioc.ic_dp = (char *)arg;
				return (strdoioctl(stp, &strioc, NULL,
				    copyflag, STRSGTTYB, crp, rvalp));

			case TIOCSTI:
				if (pm_denied(crp, P_DACREAD)) {
					if ((flag & FREAD) == 0) 
						return (EPERM);
					if (stp->sd_sidp != u.u_procp->p_sessp->s_sidp)
						return (EACCES);
				}
				strioc.ic_len = sizeof(char);
				strioc.ic_dp = (char *)arg;
				return (strdoioctl(stp, &strioc, NULL,
				    copyflag, "c", crp, rvalp));

			case TCGETA:
				fmt = STRTERMIO;

			case TCGETS:
				if (fmt == NULL)
					fmt = STRTERMIOS;

			case LDGETT:
				if (fmt == NULL)
					fmt = STRTERMCB;

			case TIOCGETP:
				if (fmt == NULL)
					fmt = STRSGTTYB;
				strioc.ic_len = 0;
				strioc.ic_dp = (char *)arg;
				return (strdoioctl(stp, &strioc, NULL,
				    copyflag, fmt, crp, rvalp));
			}
		}

#ifdef VPIX
		/*
 		 *  Support for VP/IX ioctls
 		 */
 		switch (cmd) {
 		case KDSKBMODE:
 		case AIOCDOSMODE:
 		case AIOCINTTYPE:
 	        case AIOCNONDOSMODE: 
 		case AIOCINFO:
#ifdef DEBUG
			cmn_err(CE_NOTE,"VP/IX ioctl %x",cmd);
#endif
 			while (!(bp1 = allocb(max(sizeof(struct v86blk), sizeof(struct copyreq)), BPRI_HI))) {
#ifdef DEBUG
				cmn_err(CE_NOTE,"Cannot allocb");
#endif

 				if (error = strwaitbuf(sizeof(struct v86blk), BPRI_HI)) {
 					return (error);
 				}
 			}
#ifdef DEBUG
			cmn_err(CE_NOTE,"bp1 %x b_wptr %x",bp1,bp1->b_wptr);
#endif
 			p_v86blk = (struct v86blk *)bp1->b_wptr;
 			p_v86blk->v86_u_procp = u.u_procp;
 			p_v86blk->v86_u_renv = u.u_renv;
 			p_v86blk->v86_p_pid = u.u_procp->p_pidp->pid_id;
 			p_v86blk->v86_p_ppid = u.u_procp->p_ppid;
 			p_v86blk->v86_p_cred = u.u_procp->p_cred;
 			p_v86blk->v86_p_v86 = u.u_procp->p_v86;
 			bp1->b_wptr += sizeof(struct v86blk);
#ifdef DEBUG
			cmn_err(CE_NOTE,
				"procp %x renv %x p_pid %x p_ppid %x cred %x v86 %x",
				u.u_procp, u.u_renv, u.u_procp->p_pidp->pid_id,
				u.u_procp->p_ppid, u.u_procp->p_cred,
				u.u_procp->p_v86);
#endif
		}
#endif /* VPIX */

	/* Enhanced Application Compatibility Support */

		if(keymap_type != -1)
		{
			struct keymap_flags *kp;

                        while (!(bp1 = allocb(sizeof(struct keymap_flags), 
					BPRI_HI)))
			{
                                if (error = strwaitbuf(sizeof(struct keymap_flags), BPRI_HI))
                                        return (error);
                        }
			bp1->b_wptr += sizeof(struct keymap_flags);
			kp = (struct keymap_flags *) bp1->b_rptr;
			kp->km_type = keymap_type;
			kp->km_magic = KEYMAP_MAGIC;
		}
		else	if(event_qaddr != NULL)
		{
			struct	event_getq_info *xp;
                        
                        while (!(bp1 = allocb(sizeof(struct event_getq_info), 
					BPRI_HI))) 
			{
                                if (error = strwaitbuf(sizeof(struct event_getq_info), BPRI_HI))
                                        return (error);
                        }
			bp1->b_wptr += sizeof(struct event_getq_info);
			xp = (struct event_getq_info *) bp1->b_rptr;
			xp->einfo_addr = event_qaddr;
			xp->einfo_rdev  = (dev_t) arg;
		}

	/* End Enhanced Application Compatibility Support */
		/*
		 * Unknown cmd - send down request to support 
		 * transparent ioctls.
		 */

		strioc.ic_cmd = cmd;
		strioc.ic_timout = INFTIM;
		strioc.ic_len = TRANSPARENT;
		strioc.ic_dp = (char *)&arg;
#ifdef VPIX
		return (strdoioctl(stp, &strioc, bp1 , copyflag, (char *)NULL,
		    crp, rvalp));
#else
		return (strdoioctl(stp, &strioc, NULL, copyflag, (char *)NULL,
		    crp, rvalp));
#endif

	case I_STR:
		/*
		 * Stream ioctl.  Read in an strioctl buffer from the user
		 * along with any data specified and send it downstream.
		 * Strdoioctl will wait allow only one ioctl message at
		 * a time, and waits for the acknowledgement.
		 */

		if (stp->sd_flag & STRHUP)
			return (ENXIO);
		error = strcopyin((caddr_t)arg, (caddr_t)&strioc,
		    sizeof(struct strioctl), STRIOCTL, copyflag);
		if (error)
			return (error);
		if ((strioc.ic_len < 0) || (strioc.ic_timout < -1))
			return (EINVAL);
/* Enhanced Application Compatibility Support */
		if (IS_SCOEXEC && !SCO_USES_SHNSL &&
		    (strioc.ic_cmd & ~0xff) == ('T'<<8)) {
			unsigned low_byte = (strioc.ic_cmd & 0xff);

			if (low_byte >= 100 && low_byte <= 103 &&
			    modispushed(stp, "timod"))
				strioc.ic_cmd += 40;
	        }
/* End Enhanced Application Compatibility Support */
		error = strdoioctl(stp, &strioc, NULL, copyflag, (char *)NULL,
		    crp, rvalp);
		if (error == 0)
			error = strcopyout((caddr_t)&strioc, arg,
			    sizeof(struct strioctl), STRIOCTL, copyflag);
		return (error);

	case I_NREAD:
	case FIONREAD:
		/*
		 * Return number of bytes of data in first message
		 * in queue in "arg" and return the number of messages
		 * in queue in return value.
		 */
	    {
		int size = 0;
		int count = 0;

		s = splstr();
		mp = RD(stp->sd_wrq)->q_first;
		while (mp) {
			if (!(mp->b_flag & MSGNOGET))
				break;
			mp = mp->b_next;
		}
		if (mp) 
			size = msgdsize(mp);
		error = strcopyout(&size, (int *)arg, sizeof(size), STRINT,
		    copyflag);
		if (error || cmd == FIONREAD) {
			splx(s);
			return (error);
		}
		for (; mp; mp = mp->b_next)
			if (!(mp->b_flag & MSGNOGET))
				count++;
		*rvalp = count;
		splx(s);
		return (error);
	    }
	case FIORDCHK:
		/*
		 * FIORDCHK does not use arg value (like FIONREAD),
	         * instead a count is returned. I_NREAD value may
		 * not be accurate but safe. The real thing to do is
		 * to add the msgdsizes of all data  messages until
		 * a non-data message.
		 */
	    {
		int size = 0;

		s = splstr();
		mp = RD(stp->sd_wrq)->q_first;
		while (mp) {
			if (!(mp->b_flag & MSGNOGET))
				break;
			mp = mp->b_next;
		}
		if (mp)
			size = msgdsize(mp);
		*rvalp = size;
		splx(s);
		return (0);
	    }

	case I_FIND:
		/*
		 * Get module name.
		 */
	    {
		char mname[FMNAMESZ+1];
		queue_t *q;
		int i;

		error = strcopyin((caddr_t)arg, mname, FMNAMESZ+1, STRNAME,
		    copyflag);
		if (error)
			return (error);

		/*
		 * Find module in fmodsw.
		 */
		if ((i = findmod(mname)) < 0)
			return (EINVAL);

		*rvalp = 0;

		if(fmodsw[i].f_str == NULL)	{
			return(error);
		}

		/* Look downstream to see if module is there. */
		for (q = stp->sd_wrq->q_next; q &&
		    (fmodsw[i].f_str->st_wrinit != q->q_qinfo); q = q->q_next)
			;

		*rvalp =  (q ? 1 : 0);
		return (error);
	    }

	case I_PUSH:
		/*
		 * Push a module.
		 */

	    {
		register int i;
		register queue_t *q;
		register queue_t *tq;
		register qband_t *qbp;
		int idx;
		dev_t dummydev;
		o_pid_t *oldttyp;
		uchar_t *rqbf, *wqbf;
		int renab, wenab;
		int nrband, nwband;
		char mname[FMNAMESZ+1];

		if (stp->sd_flag & STRHUP)
			return (ENXIO);
		if (stp->sd_pushcnt >= nstrpush)
			return (EINVAL);

		/*
		 * firewall - don't use too much memory
		 */

		if (strthresh && (Strcount > strthresh) && pm_denied(u.u_cred, P_SYSOPS))
			return (ENOSR);
		
		/*
		 * Get module name and look up in fmodsw.
		 */
		error = strcopyin((caddr_t)arg, mname, FMNAMESZ+1, STRNAME,
		    copyflag);
		if (error)
			return (error);
		if ((idx = findmod(mname)) < 0)
			return (EINVAL);

		rqbf = kmem_zalloc(NBAND, KM_SLEEP);
		wqbf = kmem_zalloc(NBAND, KM_SLEEP);
		while (stp->sd_flag & STWOPEN) {
			if (flag & (FNDELAY|FNONBLOCK)) {
				kmem_free(rqbf, NBAND);
				kmem_free(wqbf, NBAND);
				return (EAGAIN);
			}
			if (sleep((caddr_t)stp, STOPRI|PCATCH)) {
				kmem_free(rqbf, NBAND);
				kmem_free(wqbf, NBAND);
				return (EINTR);
			}
			if (stp->sd_flag & (STRDERR|STWRERR|STRHUP|STPLEX)) {
				kmem_free(rqbf, NBAND);
				kmem_free(wqbf, NBAND);
				return ((stp->sd_flag & STPLEX) ? EINVAL :
				    (stp->sd_werror ? stp->sd_werror :
				    stp->sd_rerror));
			}
		}
		s = splstr();
		stp->sd_flag |= STWOPEN;
		oldttyp = u.u_ttyp;
		q = RD(stp->sd_wrq);
		renab = 0;
		if (q->q_flag & QWANTW) {
			renab = 1;
			rqbf[0] = 1;
		}
		nrband = (int)q->q_nband;
		for (i = 1, qbp = q->q_bandp; i <= nrband; i++) {
			if (qbp->qb_flag & QB_WANTW) {
				renab = 1;
				rqbf[i] = 1;
			}
			qbp = qbp->qb_next;
		}
		for (q = stp->sd_wrq->q_next; q && !q->q_qinfo->qi_srvp;
		    q = q->q_next)
			;
		wenab = 0;
		nwband = 0;
		if (q) {
			if (q->q_flag & QWANTW) {
				wenab = 1;
				wqbf[0] = 1;
			}
			nwband = (int)q->q_nband;
			for (i = 1, qbp = q->q_bandp; i <= nwband; i++) {
				if (qbp->qb_flag & QB_WANTW) {
					wenab = 1;
					wqbf[i] = 1;
				}
				qbp = qbp->qb_next;
			}
		}
		splx(s);

		/*
		 * Push new module and call its open routine
		 * via qattach().  Modules don't change device
		 * numbers, so just ignore dummydev here.
		 */
		dummydev = vp->v_rdev;
		if ((error = qattach(RD(stp->sd_wrq), &dummydev, 0, FMODSW, idx, crp)) == 0) {

			stp->sd_pushcnt++;
			if (vp->v_type == VCHR) { /* sorry, no pipes allowed */
				if ((oldttyp == NULL) && (u.u_ttyp != NULL)) {

					/* 
					 * pre SVR4 driver has allocated the
					 * stream as a controlling terminal -
					 * check against SVR4 criteria and
					 * deallocate it if it fails
					 */
					if (!strctty(u.u_procp, stp)) {
						*u.u_ttyp = 0;
						u.u_ttyp = NULL;
					}
				} else if (stp->sd_flag & STRISTTY) {

					/*
					 * this is a post SVR4 tty driver -
					 * try to allocate it as a
					 * controlling terminal
					 */	
					(void) strctty(u.u_procp, stp);
				}
			}
		} else {
			s = splstr();
			tq = RD(stp->sd_wrq);
			for (q = backq(tq); q && !q->q_qinfo->qi_srvp;
			    q = backq(q))
				;
			if (q) {
				done = 0;
				if (rqbf[0] && !(tq->q_flag & QWANTW))
					done = 1;
				else
					rqbf[0] = 0;
				qbp = tq->q_bandp;
				for (i = 1; i <= nrband; i++) {
					if (rqbf[i] && !(qbp->qb_flag&QB_WANTW))
						done = 1;
					else
						rqbf[i] = 0;
					qbp = qbp->qb_next;
				}
				if (done) {
					qenable(q);
					for (i = 0; i <= nrband; i++) {
						if (rqbf[i])
							setqback(q, i);
					}
				}
			}
			for (q = stp->sd_wrq->q_next; q && !q->q_qinfo->qi_srvp;
			    q = q->q_next)
				;
			if (q) {
				done = 0;
				if (wqbf[0] && !(q->q_flag & QWANTW))
					done = 1;
				else
					wqbf[0] = 0;
				qbp = q->q_bandp;
				for (i = 1; i <= nwband; i++) {
					if (wqbf[i] && !(qbp->qb_flag&QB_WANTW))
						done = 1;
					else
						wqbf[i] = 0;
					qbp = qbp->qb_next;
				}
				if (done) {
					qenable(stp->sd_wrq);
					for (i = 0; i <= nwband; i++) {
						if (wqbf[i])
						    setqback(stp->sd_wrq, i);
					}
				}
			}
			goto pushdone;
		}

		/*
		 * If flow control is on, don't break it - enable
		 * first back queue with svc procedure.
		 */
		s = splstr();
		q = RD(stp->sd_wrq->q_next);
		if (q->q_qinfo->qi_srvp) {
			for (q = backq(q); q && !q->q_qinfo->qi_srvp;
			    q = backq(q))
				;
			if (q && renab) {
				qenable(q);
				for (i = 0; i <= nrband; i++) {
					if (rqbf[i])
						setqback(q, i);
				}
			}
		}
		q = stp->sd_wrq->q_next;
		if (q->q_qinfo->qi_srvp) {
			if (wenab) {
				qenable(stp->sd_wrq);
				for (i = 0; i <= nwband; i++) {
					if (wqbf[i])
					    setqback(stp->sd_wrq, i);
				}
			}
		}
pushdone:
		stp->sd_flag &= ~STWOPEN;
		splx(s);
		kmem_free((caddr_t)rqbf, NBAND);
		kmem_free((caddr_t)wqbf, NBAND);
		wakeprocs((caddr_t)stp, PRMPT);
		return (error);
	    }

	case I_POP:
		/*
		 * Pop module (if module exists).
		 */
		if (stp->sd_flag&STRHUP)
			return (ENXIO);
		if (!stp->sd_wrq->q_next)	/* for broken pipes */
			return (EINVAL);
		if (stp->sd_wrq->q_next->q_next &&
		    !(stp->sd_wrq->q_next->q_flag & QREADR)) {
			qdetach(RD(stp->sd_wrq->q_next), 1, flag, crp);
			stp->sd_pushcnt--;
			return (0);
		}
		return (EINVAL);

	case I_LOOK:
		/*
		 * Get name of first module downstream.
		 * If no module, return an error.
		 */
	    {
		int i;

		for (i = 0; i < fmodcnt; i++)
			if (fmodsw[i].f_str->st_wrinit == stp->sd_wrq->q_next->q_qinfo) {
				error = strcopyout(fmodsw[i].f_name,
				    (char *)arg, FMNAMESZ+1, STRNAME, copyflag);
				return (error);
			}
		return (EINVAL);
	    }

	case I_LINK:
	case I_PLINK:
		/* 
		 * Link a multiplexor.
		 */
	    {
		struct file *fpdown;
		struct linkinfo *linkp, *alloclink();
		struct stdata *stpdown;
		queue_t *rq;

		/*
		 * Test for invalid upper stream
		 */
		if (stp->sd_flag & STRHUP)
			return (ENXIO);
		if (vp->v_type == VFIFO)
			return (EINVAL);
		if (!stp->sd_strtab->st_muxwinit)
			return (EINVAL);
		if (error = getf(arg, &fpdown))
			return (error);

		/*
		 * Test for invalid lower stream.
		 */
		if (((stpdown = fpdown->f_vnode->v_stream) == NULL) ||
		    (stpdown == stp) || (stpdown->sd_flag &
		    (STPLEX|STRHUP|STRDERR|STWRERR|IOCWAIT)) ||
		    linkcycle(stp, stpdown))
			return (EINVAL);
		if (cmd == I_PLINK)
			rq = NULL;
		else
			rq = getendq(stp->sd_wrq);
		if (!(linkp = alloclink(rq, stpdown->sd_wrq, fpdown)))
			return (EAGAIN);
		strioc.ic_cmd = cmd;
		strioc.ic_timout = 0;
		strioc.ic_len = sizeof(struct linkblk);
		strioc.ic_dp = (char *)&linkp->li_lblk;
	
		/* Set up queues for link */
		rq = RD(stpdown->sd_wrq);
		setq(rq, stp->sd_strtab->st_muxrinit, stp->sd_strtab->st_muxwinit);
		rq->q_ptr = WR(rq)->q_ptr = NULL;
		rq->q_flag |= QWANTR;	
		WR(rq)->q_flag |= QWANTR;

		if (error = strdoioctl(stp, &strioc, NULL, K_TO_K, STRLINK, crp, rvalp)) {
			lbfree(linkp);
			setq(rq, &strdata, &stwdata);
			rq->q_ptr = WR(rq)->q_ptr = (caddr_t)stpdown;
			return (error);
		}
		s = splstr();
		stpdown->sd_flag |= STPLEX;
		fpdown->f_count++;
		if (error = mux_addedge(stp, stpdown, linkp->li_lblk.l_index)) {
			int type;
			if (cmd == I_LINK) {
				type = LINKIOCTL|LINKNORMAL;
				strioc.ic_cmd = I_UNLINK;
			} else {	/* I_PLINK */
				type = LINKIOCTL|LINKPERSIST;
				strioc.ic_cmd = I_PUNLINK;
			}
			splx(s);
			(void) munlink(stp, linkp, type, crp, rvalp);
			return (error);
		}
		splx(s);
		/*
		 * Wake up any other processes that may have been
		 * waiting on the lower stream.  These will all
		 * error out.
		 */
		wakeprocs((caddr_t)rq, PRMPT);
		wakeprocs((caddr_t)WR(rq), PRMPT);
		wakeprocs((caddr_t)stpdown, PRMPT);
		*rvalp = linkp->li_lblk.l_index;
		return (0);
	    }
	
	case I_UNLINK:
	case I_PUNLINK:
		/*
		 * Unlink a multiplexor.
		 * If arg is -1, unlink all links for which this is the
		 * controlling stream.  Otherwise, arg is a index number
		 * for a link to be removed.
		 */
	    {
		struct linkinfo *linkp;
		int type;

		if (vp->v_type == VFIFO)
			return (EINVAL);
		if (cmd == I_UNLINK)
			type = LINKIOCTL|LINKNORMAL;
		else	/* I_PUNLINK */
			type = LINKIOCTL|LINKPERSIST;
		if (arg == 0)
			return (EINVAL);
		if (arg == MUXID_ALL)
			error = munlinkall(stp, type, crp, rvalp);
		else {
			if (!(linkp = findlinks(stp, arg, type))) {
				/* invalid user supplied index number */
				return (EINVAL);
			}
			error = munlink(stp, linkp, type, crp, rvalp);
		}
		return (error);
	    }

	case I_FLUSH:
		/*
		 * send a flush message downstream
		 * flush message can indicate 
		 * FLUSHR - flush read queue
		 * FLUSHW - flush write queue
		 * FLUSHRW - flush read/write queue
		 */
		if (stp->sd_flag & STRHUP)
			return (ENXIO);
		if (arg & ~FLUSHRW)
			return (EINVAL);
		while (!putctl1(stp->sd_wrq->q_next, M_FLUSH, arg))
			if (error = strwaitbuf(1, BPRI_HI))
				return (error);

		if (qready())
			runqueues();
		return (0);

	case I_FLUSHBAND:
	    {
		struct bandinfo binfo;

		error = strcopyin((caddr_t)arg, (caddr_t)&binfo, sizeof(struct
		    bandinfo), STRBANDINFO, copyflag);
		if (error)
			return (error);
		if (stp->sd_flag & STRHUP)
			return (ENXIO);
		if (binfo.bi_flag & ~FLUSHRW)
			return (EINVAL);
		while (!(mp = allocb(2, BPRI_HI))) {
			if (error = strwaitbuf(2, BPRI_HI))
				return (error);
		}
		mp->b_datap->db_type = M_FLUSH;
		*mp->b_wptr++ = binfo.bi_flag | FLUSHBAND;
		*mp->b_wptr++ = binfo.bi_pri;
		putnext(stp->sd_wrq, mp);
		if (qready())
			runqueues();
		return (0);
	    }

	case I_SRDOPT:
		/*
		 * Set read options
		 *
		 * RNORM - default stream mode
		 * RMSGN - message no discard
		 * RMSGD - message discard
		 * RPROTNORM - fail read with EBADMSG for M_[PC]PROTOs
		 * RPROTDAT - convert M_[PC]PROTOs to M_DATAs
		 * RPROTDIS - discard M_[PC]PROTOs and retain M_DATAs
		 */
	    {
		long oldflag;

		if (arg & ~(RMODEMASK | RPROTMASK))
			return (EINVAL);

		s = splstr();
		oldflag = stp->sd_flag;
		switch (arg & RMODEMASK) {
		case RNORM: 
			stp->sd_flag &= ~(RMSGDIS | RMSGNODIS);
			break;
		case RMSGD:
			stp->sd_flag = (stp->sd_flag & ~RMSGNODIS) | RMSGDIS;
			break;
		case RMSGN:
			stp->sd_flag = (stp->sd_flag & ~RMSGDIS) | RMSGNODIS;
			break;
		default:	/* more than 1 bit set, note: RNORM is 0 */
			splx(s);
			return (EINVAL);
		}

		switch(arg & RPROTMASK) {
		case RPROTNORM:
			stp->sd_flag &= ~(RDPROTDAT | RDPROTDIS);
			break;

		case RPROTDAT:
			stp->sd_flag = ((stp->sd_flag & ~RDPROTDIS) |
			    RDPROTDAT);
			break;

		case RPROTDIS:
			stp->sd_flag = ((stp->sd_flag & ~RDPROTDAT) |
			    RDPROTDIS);
			break;
		case 0:	/* setting none is ok */
			break;
		default:	/* more than 1 bit set */
			stp->sd_flag = oldflag;	/* undo above */
			splx(s);
			return (EINVAL);
		}
		splx(s);
		return (0);
	    }

	case I_GRDOPT:
		/*
		 * Get read option and return the value
		 * to spot pointed to by arg
		 */
	    {
		int rdopt;

		rdopt = ((stp->sd_flag & RMSGDIS) ? RMSGD :
			  ((stp->sd_flag & RMSGNODIS) ? RMSGN : RNORM));
		rdopt |= ((stp->sd_flag & RDPROTDAT) ? RPROTDAT :
			  ((stp->sd_flag & RDPROTDIS) ? RPROTDIS : RPROTNORM));

		error = strcopyout(&rdopt, (int *)arg, sizeof(rdopt), STRINT,
		    copyflag);
		return (error);
	    }

	case I_SETSIG:
		/*
		 * Register the calling proc to receive the SIGPOLL
		 * signal based on the events given in arg.  If
		 * arg is zero, remove the proc from register list.
		 */
	    {
		struct strevent *sep, *psep;

		psep = NULL;
		s = splstr();
		for (sep = stp->sd_siglist; sep && (sep->se_procp !=
		     u.u_procp); psep = sep, sep = sep->se_next)
			;

		if (arg) {
			if (arg & ~(S_INPUT|S_HIPRI|S_MSG|S_HANGUP|S_ERROR|
			    S_RDNORM|S_WRNORM|S_RDBAND|S_WRBAND|S_BANDURG)) {
				splx(s);
				return (EINVAL);
			}
			if ((arg & S_BANDURG) && !(arg & S_RDBAND)) {
				splx(s);
				return (EINVAL);
			}

			/*
			 * If proc not already registered, add it
			 * to list.
			 */
			if (!sep) {
				if (!(sep = sealloc(SE_SLEEP))) {
					splx(s);
					return (EAGAIN);
				}
				if (psep)
					psep->se_next = sep;
				else
					stp->sd_siglist = sep;
				sep->se_procp = u.u_procp;
			}

			/*
			 * Set events.
			 */
			sep->se_events = arg;
		} else {

			/*
			 * Remove proc from register list.
			 */
			if (sep) {
				if (psep)
					psep->se_next = sep->se_next;
				else
					stp->sd_siglist = sep->se_next;
				sefree(sep);
			} else {
				splx(s);
				return (EINVAL);
			}
		}

		/*
		 * Recalculate OR of sig events.
		 */
		stp->sd_sigflags = 0;
		for (sep = stp->sd_siglist; sep; sep = sep->se_next)
			stp->sd_sigflags |= sep->se_events;
		splx(s);
		return (0);
	    }

	case I_GETSIG:
		/*
		 * Return (in arg) the current registration of events
		 * for which the calling proc is to be signalled.
		 */
	    {
		struct strevent *sep;

		s = splstr();
		for (sep = stp->sd_siglist; sep; sep = sep->se_next)
			if (sep->se_procp == u.u_procp) {
				error = strcopyout((caddr_t)&sep->se_events,
				    (int *)arg, sizeof(int), STRINT, copyflag);
				splx(s);
				return (error);
			}
		splx(s);
		return (EINVAL);
	    }

	case I_PEEK:
	    {
		struct strpeek strpeek;
		int n;

		error = strcopyin((caddr_t)arg, (caddr_t)&strpeek,
		  sizeof(strpeek), STRPEEK, copyflag);
		if (error)
			return (error);

		s = splstr();
		mp = RD(stp->sd_wrq)->q_first;
		while (mp) {
			if (!(mp->b_flag & MSGNOGET))
				break;
			mp = mp->b_next;
		}
		splx(s);
		if (!mp || ((strpeek.flags & RS_HIPRI) &&
		    queclass(mp) == QNORM)) {
			*rvalp = 0;
			return (0);
		}

		if (mp->b_datap->db_type == M_PASSFP)
			return (EBADMSG);

		if (mp->b_datap->db_type == M_PCPROTO) 
			strpeek.flags = RS_HIPRI;
		else
			strpeek.flags = 0;

		/*
		 * First process PROTO blocks, if any.
		 */
		iov.iov_base = strpeek.ctlbuf.buf;
		iov.iov_len = strpeek.ctlbuf.maxlen;
		uio.uio_iov = &iov;
		uio.uio_iovcnt = 1;
		uio.uio_offset = 0;
		uio.uio_segflg = (copyflag == U_TO_K) ? UIO_USERSPACE :
		    UIO_SYSSPACE;
		uio.uio_fmode = 0;
		uio.uio_resid = iov.iov_len;
		while (mp && mp->b_datap->db_type != M_DATA && uio.uio_resid >= 0) {
			if ((n = MIN(uio.uio_resid, mp->b_wptr - mp->b_rptr)) &&
			    (error = uiomove((caddr_t)mp->b_rptr, n, UIO_READ, &uio)))
				return (error);
			mp = mp->b_cont;
		}
		strpeek.ctlbuf.len = strpeek.ctlbuf.maxlen - uio.uio_resid;
	
		/*
		 * Now process DATA blocks, if any.
		 */
		while (mp && mp->b_datap->db_type != M_DATA)
			mp = mp->b_cont;
		iov.iov_base = strpeek.databuf.buf;
		iov.iov_len = strpeek.databuf.maxlen;
		uio.uio_iovcnt = 1;
		uio.uio_resid = iov.iov_len;
		while (mp && uio.uio_resid >= 0) {
			if ((n = MIN(uio.uio_resid, mp->b_wptr - mp->b_rptr)) &&
			    (error = uiomove((char *)mp->b_rptr, n, UIO_READ, &uio)))
				return (error);
			mp = mp->b_cont;
		}

		strpeek.databuf.len = strpeek.databuf.maxlen - uio.uio_resid;
		error = strcopyout((caddr_t)&strpeek, (caddr_t)arg,
		    sizeof(strpeek), STRPEEK, copyflag);
		if (error)
			return (error);
		*rvalp = 1;
		return 0;
	    }

	case I_FDINSERT:
	    {
		struct strfdinsert strfdinsert;
		struct file *resftp;
		struct stdata *resstp;
		queue_t *q;
		register long msgsize;
		long rmin, rmax;
		int strmakemsg();

		if (stp->sd_flag & STRHUP)
			return (ENXIO);
		if (stp->sd_flag & (STRDERR|STWRERR|STPLEX))
			return ((stp->sd_flag & STPLEX) ? EINVAL :
			    (stp->sd_werror ? stp->sd_werror : stp->sd_rerror));
		error = strcopyin((caddr_t)arg, (caddr_t)&strfdinsert, 
		    sizeof(strfdinsert), STRFDINSERT, copyflag);
		if (error)
			return (error);
		if (strfdinsert.offset < 0 ||
		   (strfdinsert.offset % sizeof(queue_t *)) != 0)
			return (EINVAL);
		if (getf(strfdinsert.fildes, &resftp) ||
		   ((resstp = resftp->f_vnode->v_stream) == NULL))
			return (EINVAL);

		if (resstp->sd_flag & (STRDERR|STWRERR|STRHUP|STPLEX))
			return ((resstp->sd_flag & STPLEX) ? EINVAL :
			    (resstp->sd_werror ? resstp->sd_werror :
			    resstp->sd_rerror));

		/* get read queue of stream terminus */
		for (q = resstp->sd_wrq->q_next; q->q_next; q = q->q_next)
			;
		q = RD(q);

		if (strfdinsert.ctlbuf.len < strfdinsert.offset + sizeof(queue_t *))
			return (EINVAL);

		/*
		 * Check for legal flag value.
		 */
		if (strfdinsert.flags & ~RS_HIPRI)
			return (EINVAL);

		/*
		 * Make sure ctl and data sizes together fall within 
		 * the limits of the max and min receive packet sizes 
		 * and do not exceed system limit.  A negative data
		 * length means that no data part is to be sent.
		 */
		rmin = stp->sd_wrq->q_next->q_minpsz;
		rmax = stp->sd_wrq->q_next->q_maxpsz;
		ASSERT((rmax >= 0) || (rmax == INFPSZ));
		if (rmax == 0)
			return (ERANGE);
		if (strmsgsz != 0) {
			if (rmax == INFPSZ)
				rmax = strmsgsz;
			else
				rmax = MIN(strmsgsz, rmax);
		}
		if ((msgsize = strfdinsert.databuf.len) < 0)
			msgsize = 0;
		if ((msgsize < rmin) ||
		    ((msgsize > rmax) && (rmax != INFPSZ)) ||
		    (strfdinsert.ctlbuf.len > strctlsz))
			return (ERANGE);

		s = splstr();
		while (!(strfdinsert.flags & RS_HIPRI) &&
		    !bcanput(stp->sd_wrq->q_next, 0)) {
			if ((error = strwaitq(stp, WRITEWAIT, (off_t)0,
			    flag, &done)) || done) {
				splx(s);
				return (error);
			}
		}
		splx(s);

		iov.iov_base = strfdinsert.databuf.buf;
		iov.iov_len = strfdinsert.databuf.len;
		uio.uio_iov = &iov;
		uio.uio_iovcnt = 1;
		uio.uio_offset = 0;
		uio.uio_segflg = (copyflag == U_TO_K) ? UIO_USERSPACE :
		    UIO_SYSSPACE;
		uio.uio_fmode = 0;
		uio.uio_resid = iov.iov_len;
		if ((error = strmakemsg(&strfdinsert.ctlbuf,
		    strfdinsert.databuf.len, &uio, stp,
		    strfdinsert.flags, &mp)) || !mp)
			return (error);

		/*
		 * Place pointer to queue 'offset' bytes from the
		 * start of the control portion of the message.
		 */

		*((queue_t **)(mp->b_rptr + strfdinsert.offset)) = q;

		/*
		 * Put message downstream.
		 */
		(*stp->sd_wrq->q_next->q_qinfo->qi_putp)(stp->sd_wrq->q_next, mp);
		if (qready())
			runqueues();
		return (error);
	    }

	case I_SENDFD:
	    {
		register queue_t *qp;
		register struct adt_strrecvfd *adtstr;
		register struct strrecvfd *srf;
		register struct adtrecvfd *arf;
		struct file *fp;

		if (stp->sd_flag & STRHUP)
			return (ENXIO);

		for (qp = stp->sd_wrq; qp->q_next; qp = qp->q_next)
			;
		if (qp->q_qinfo != &strdata)
			return (EINVAL);
	 	if (error = getf(arg, &fp))
			return (error);
		if ((qp->q_flag & QFULL) ||
		    !(mp = allocb(sizeof(struct adt_strrecvfd), BPRI_MED)))
			return (EAGAIN);
		adtstr = (struct adt_strrecvfd *)mp->b_rptr;
		srf = (struct strrecvfd *)&adtstr->adt_strrecvfd;
		arf = (struct adtrecvfd *)&adtstr->adt_adtrecvfd;
		mp->b_wptr += sizeof(struct adt_strrecvfd);
		mp->b_datap->db_type = M_PASSFP;
		srf->f.fp = fp;
		srf->uid = u.u_cred->cr_uid;
		srf->gid = u.u_cred->cr_gid;
		arf->adtrfd_sendpid = u.u_procp->p_pidp->pid_id;
		arf->adtrfd_sendfd = arg;
		fp->f_count++;
		strrput(qp, mp);
		return (0);
	    }

	case I_RECVFD:
	case I_E_RECVFD:
	case I_S_RECVFD:
	    {
		register struct adt_strrecvfd *adtstr;
		register struct strrecvfd *srf;
		register struct adtrecvfd *arf;
		int vmode;
		int i, fd;
		struct file *fp;
		union {
			struct e_strrecvfd estrfd; /* EFT data structure */
			struct o_strrecvfd ostrfd; /* non-EFT data structure -
						    * SVR3 compatibility mode.
						    */
		} str;

		s = splstr();
		while (!(mp = getq(RD(stp->sd_wrq)))) {
			if (stp->sd_flag & STRHUP) {
				splx(s);
				return (ENXIO);
			}
			if ((error = strwaitq(stp, GETWAIT, (off_t) 0,
			    flag, &done)) || done) {
				splx(s);
				return (error);
			}
		}
		if (mp->b_datap->db_type != M_PASSFP) {
			putbq(RD(stp->sd_wrq), mp);
			splx(s);
			return (EBADMSG);
		}
		splx(s);
		adtstr = (struct adt_strrecvfd *)mp->b_rptr;
		srf = (struct strrecvfd *)&adtstr->adt_strrecvfd;
		arf = (struct adtrecvfd *)&adtstr->adt_adtrecvfd;

 		/*
 		 * make sure the process receiving this file
 		 * descriptor has the appropriate domination and/or
 		 * privileges to use this file descriptor.
 		 */
		fp = srf->f.fp;
		/*
                 * if type is VFIFO then check for MAC_WRITE regardless
                 * of flag value.
                 */
                if (fp->f_vnode->v_type == VFIFO)
                        vmode = VWRITE;
                else
                        vmode = (fp->f_flag & (FWRITE|FTRUNC)) ? VWRITE : VREAD;

 		if (error = MAC_VACCESS(fp->f_vnode, vmode, crp)) {
 			putbq(RD(stp->sd_wrq), mp);
 			return (error);
 		}

		/*
		 * If sender of the file was not its owner, the receiver must
		 * pass DAC on the file to be received.  For compatability,
		 * the check is only performed if MAC is installed.
		 */
		if (mac_installed) {
			vattr_t	va;

			/*
			 * Using files open time credentials instead of the
			 * receivers credentials to get the file attributes
			 * because the VOP_GETATTR is on be-half of the kernel
			 * and not the user attempting to receive the file.
			 */
			if (error = VOP_GETATTR(fp->f_vnode, &va, 0,
			    fp->f_cred)) {
	 			putbq(RD(stp->sd_wrq), mp);
 				return (error);
 			}
			if (srf->uid != va.va_uid) {
				if (error = VOP_ACCESS(fp->f_vnode,
				    vmode, DAC_ACC, u.u_cred)) {
					putbq(RD(stp->sd_wrq), mp);
					return (error);
				}
 			}
		}
		if (error = ufalloc(0, &fd)) {
			putbq(RD(stp->sd_wrq), mp);
			return (error);
		}
		setf(fd, fp);
		srf->f.fd = fd;

		if (cmd == I_RECVFD) {
			/* check to see if uid/gid values are too large. */

			if (srf->uid > USHRT_MAX || srf->gid > USHRT_MAX) {
				srf->f.fp = fp;
				putbq(RD(stp->sd_wrq), mp);
				setf(fd, NULLFP);
				return (EOVERFLOW);
			}
			str.ostrfd.f.fd = srf->f.fd;
			str.ostrfd.uid = (o_uid_t) srf->uid;
			str.ostrfd.gid = (o_gid_t) srf->gid;
			for(i = 0; i < 8; i++)
				str.ostrfd.fill[i] = 0;		/* zero pad */

			if (error = strcopyout((caddr_t)&str.ostrfd,
			    (caddr_t)arg, sizeof(struct o_strrecvfd),
			    O_STRRECVFD, copyflag)) {
				srf->f.fp = fp;
				putbq(RD(stp->sd_wrq), mp);
				setf(fd, NULLFP);
				return (error);
			}
		} else if (cmd == I_S_RECVFD) {
			struct s_strrecvfd *sstrfd = (struct s_strrecvfd *)NULL;
			static size_t sstrfdsize = 0;

			if (sstrfdsize == 0)
				sstrfdsize = sizeof(struct s_strrecvfd) +
				    crgetsize() - sizeof(struct sub_attr);
			if ((sstrfd = (struct s_strrecvfd *)
			    kmem_alloc(sstrfdsize, KM_SLEEP)) == NULL) {
				srf->f.fp = fp;
				putbq(RD(stp->sd_wrq), mp);
				setf(fd, NULLFP);
				return (ENOMEM);
			}

			sstrfd->fd = srf->f.fd;
			sstrfd->uid = (uid_t)srf->uid;
			sstrfd->gid = (gid_t)srf->gid;
			bcopy((caddr_t)fp->f_cred, (caddr_t)&sstrfd->s_attrs,
			    crgetsize());

			if (error = strcopyout((caddr_t)sstrfd, (caddr_t)arg,
			    sstrfdsize, S_STRRECVFD, copyflag)) {
				srf->f.fp = fp;
				putbq(RD(stp->sd_wrq), mp);
				setf(fd, NULLFP);
                                kmem_free(sstrfd, sstrfdsize);
				return (error);
			}
                        kmem_free(sstrfd, sstrfdsize);
		} else {		/* I_E_RECVFD */
			str.estrfd.f.fd = srf->f.fd;
			str.estrfd.uid = (uid_t)srf->uid;
			str.estrfd.gid = (gid_t)srf->gid;
			for(i = 0; i < 8; i++)
				str.estrfd.fill[i] = 0;		/* zero pad */

			if (error = strcopyout((caddr_t)&str.estrfd,
			    (caddr_t)arg, sizeof(struct e_strrecvfd),
			    STRRECVFD, copyflag)) {
				srf->f.fp = fp;
				putbq(RD(stp->sd_wrq), mp);
				setf(fd, NULLFP);
				return (error);
			}
		}
		/* Get audit information needed from receiver and dump and
		 * audit record
		 */
		if (audit_on) {
			arf->adtrfd_recvpid = u.u_procp->p_pidp->pid_id;
			arf->adtrfd_recvfd = fd;
			adt_recvfd(arf);
		}

		freemsg(mp);
		return (0);
	    }

	case I_SWROPT: 
		/*
		 * Set/clear the write options. arg is a bit
		 * mask with any of the following bits set...
		 * 	SNDZERO - send zero length message
		 *	SNDPIPE - send sigpipe to process if
		 *		sd_werror is set and process is
		 *		doing a write or putmsg.
		 * The new stream head write options should reflect
		 * what is in arg.
		 */
		if (arg & ~(SNDZERO|SNDPIPE))
			return (EINVAL);

		s = splstr();
		stp->sd_flag &= ~(STRSNDZERO|STRSIGPIPE);
		if (arg & SNDZERO)
			stp->sd_flag |= STRSNDZERO;
		if (arg & SNDPIPE)
			stp->sd_flag |= STRSIGPIPE;
		splx(s);
		return (0);

	case I_GWROPT:
	    {
		int wropt = 0;

		if (stp->sd_flag & STRSNDZERO)
			wropt |= SNDZERO;
		if (stp->sd_flag & STRSIGPIPE)
			wropt |= SNDPIPE;
		error = strcopyout(&wropt, (int *)arg, sizeof(wropt), STRINT,
		    copyflag);
		return (error);
	    }

	case I_LIST:
		/* 
		 * Returns all the modules found on this stream,
		 * upto the driver. If argument is NULL, return the
		 * number of modules (including driver). If argument
		 * is not NULL, copy the names into the structure
		 * provided.
		 */

	    {
		queue_t *q;
		int num_modules, space_allocated;
		struct str_list strlist;

		if (arg == NULL) { /* Return number of modules plus driver */
			q = stp->sd_wrq;
			*rvalp = stp->sd_pushcnt + 1;
		} else {
			error = strcopyin((caddr_t)arg, (caddr_t)&strlist,
			    sizeof(struct str_list), STRLIST, copyflag);
			if (error)
				return (error);
			if ((space_allocated = strlist.sl_nmods) <= 0) 
				return (EINVAL);
			q = stp->sd_wrq;
			num_modules = 0;
			while (SAMESTR(q) && (space_allocated != 0)) {
				error = strcopyout(
				    q->q_next->q_qinfo->qi_minfo->mi_idname,
				    (struct str_list *)strlist.sl_modlist,
				    (FMNAMESZ + 1), STRNAME, copyflag);
				if (error)
					return (error);
				q = q->q_next;
				space_allocated--;
				num_modules++;
				strlist.sl_modlist++;
			}
			if (SAMESTR(q))  /* Space ran out */
				return (ENOSPC);
			error = strcopyout((caddr_t) &num_modules, (int *)arg,
			    sizeof(int), STRINT, copyflag);
		}
		return (error);
	    }

	case I_CKBAND:
	    {
		queue_t *q = RD(stp->sd_wrq);
		qband_t *qbp;

		/*
		 * Ignores MSGNOGET.
		 */
		if ((arg < 0) || (arg >= NBAND))
			return (EINVAL);
		if (arg > (int)q->q_nband) {
			*rvalp = 0;
		} else {
			if (arg == 0) {
				mp = q->q_last;
				if (mp) {
					if ((mp->b_band == 0) && (queclass(mp) == QNORM))
						*rvalp = 1;
					else
						*rvalp = 0;
				}
				else
					*rvalp = 0;
			} else {
				qbp = q->q_bandp;
				while (--arg > 0)
					qbp = qbp->qb_next;
				if (qbp->qb_first)
					*rvalp = 1;
				else
					*rvalp = 0;
			}
		}
		return (0);
	    }

	case I_GETBAND:
	    {
		int intpri;

		/*
		 * Ignores MSGNOGET.
		 */
		mp = RD(stp->sd_wrq)->q_first;
		if (!mp)
			return (ENODATA);
		intpri = (int)mp->b_band;
		error = strcopyout((caddr_t)&intpri, (caddr_t)arg, sizeof(int),
		    STRINT, copyflag);
		return (error);
	    }

	case I_ATMARK:
	    {
		queue_t *q = RD(stp->sd_wrq);

		if (!arg || (arg & ~(ANYMARK|LASTMARK)))
			return (EINVAL);
		s = splstr();
		mp = q->q_first;

		/*
		 * Hack for sockets compatibility.  We need to
		 * ignore any messages at the stream head that
		 * are marked MSGNOGET and are not marked MSGMARK.
		 */
		while (mp && ((mp->b_flag & (MSGNOGET|MSGMARK)) == MSGNOGET))
			mp = mp->b_next;

		if (!mp)
			*rvalp = 0;
		else if ((arg & ANYMARK) && (mp->b_flag & MSGMARK))
			*rvalp = 1;
		else if ((arg & LASTMARK) && (mp == stp->sd_mark))
			*rvalp = 1;
		else
			*rvalp = 0;
		splx(s);
		return (0);
	    }

	case I_CANPUT:
	    {
		char band;

		if ((arg < 0) || (arg >= NBAND))
			return (EINVAL);
		band = (char)arg;
		*rvalp = bcanput(stp->sd_wrq->q_next, band);
		return (0);
	    }

	case I_SETCLTIME:
	    {
		long closetime;

		error = strcopyin((caddr_t)arg, (caddr_t)&closetime,
		    sizeof(long), STRLONG, copyflag);
		if (error)
			return (error);
		if (closetime < 0)
			return (EINVAL);

		/*
		 *  Convert between milliseconds and clock ticks.
		 */
		stp->sd_closetime = ((closetime / 1000) * HZ) +
		    ((((closetime % 1000) * HZ) + 999) / 1000);
		return (0);
	    }

	case I_GETCLTIME:
	    {
		long closetime;

		/*
		 * Convert between clock ticks and milliseconds.
		 */
		closetime = ((stp->sd_closetime / HZ) * 1000) +
		    (((stp->sd_closetime % HZ) * 1000) / HZ);
		error = strcopyout((caddr_t)&closetime, (caddr_t)arg,
		    sizeof(long), STRLONG, copyflag);
		return (error);
	    }

        case TIOCSSID:
	{
         	pid_t sid;

                if (stp->sd_sidp != u.u_procp->p_sessp->s_sidp)
                        return ENOTTY;
		if (error = strcopyin(arg, (caddr_t)&sid, sizeof(pid_t),
                    STRPIDT, copyflag))
                        return error;
		return realloctty(u.u_procp, sid);
	}

	case TIOCGSID:
		if (stp->sd_sidp == NULL)
			return ENOTTY;
		return strcopyout((caddr_t)&stp->sd_sidp->pid_id,
                  arg, sizeof(pid_t), STRPIDT, copyflag);

	case TIOCSPGRP:
	{
		pid_t pgrp;
		proc_t *q, *pgfind();


		if (stp->sd_sidp != u.u_procp->p_sessp->s_sidp)
			return ENOTTY;

/* Enhanced Application Compatibility Support */
		if (ISC_USES_POSIX)
			pgrp = arg;
/* End Enhanced Application Compatibility Support */
		else if (error = strcopyin(arg, (caddr_t)&pgrp, sizeof(pid_t),
                    STRPIDT, copyflag))
			return (error);
		if (pgrp == stp->sd_pgidp->pid_id)
			return 0;
		if (pgrp <= 0 || pgrp >= MAXPID)
			return EINVAL;
		if ((q = pgfind(pgrp)) == NULL
                  || q->p_sessp != u.u_procp->p_sessp)
			return EPERM;
		PID_RELE(stp->sd_pgidp);
		stp->sd_pgidp = q->p_pgidp;
		PID_HOLD(stp->sd_pgidp);
		return 0;
	}

	case TIOCGPGRP:
		if (stp->sd_sidp == NULL)
			return ENOTTY;

/* Enhanced Application Compatibility Support */
		if (ISC_USES_POSIX)
			*rvalp = stp->sd_pgidp->pid_id;
		else 
/* End Enhanced Application Compatibility Support */
			return strcopyout((caddr_t)&stp->sd_pgidp->pid_id,
					 arg, sizeof(pid_t), STRPIDT, copyflag);

	case FIONBIO:
	case FIOASYNC:
		return (0);	/* handled by the upper layer */
	}
}

/*
 * Send an ioctl message downstream and wait for acknowledgement.
 */
int
strdoioctl(stp, strioc, ebp, copyflag, fmtp, crp, rvalp)
	struct stdata *stp;
	struct strioctl *strioc;
	mblk_t *ebp;
	int copyflag;
	char *fmtp;
	cred_t *crp;
	int *rvalp;
{
	register mblk_t *bp;
	register s;
	struct iocblk *iocbp;
	struct copyreq *reqp;
	struct copyresp *resp;
	struct striopst *sp;
	mblk_t *fmtbp;
	int id;
	long saveiocid;
	int transparent = 0;
	int error = 0;
	int len = 0;
	caddr_t taddr;
	extern void str2time(), str3time();

	if (strioc->ic_len == TRANSPARENT) {	/* send arg in M_DATA block */
		transparent = 1;
		strioc->ic_len = sizeof(int);
	}
	
	if ((strioc->ic_len < 0) ||
	    ((strmsgsz > 0) && (strioc->ic_len > strmsgsz))) {
		if (ebp)
			freeb(ebp);
		return (EINVAL);
	}

	while (!(bp = allocb(max(sizeof(struct iocblk), sizeof(struct copyreq)), BPRI_HI))) 
		if (error = strwaitbuf(sizeof(struct iocblk), BPRI_HI)) {
			if (ebp)
				freeb(ebp);
			return (error);
		}

	iocbp = (struct iocblk *)bp->b_wptr;
	iocbp->ioc_count = strioc->ic_len;
	iocbp->ioc_cmd = strioc->ic_cmd;
	crhold(crp);
#ifndef _STYPES
	iocbp->ioc_cr = crp;
#else
	iocbp->ioc_uid = crp->cr_uid;
	iocbp->ioc_gid = crp->cr_gid;
#endif
	iocbp->ioc_error = 0;
	iocbp->ioc_rval = 0;
	bp->b_datap->db_type = M_IOCTL;
	bp->b_wptr += sizeof(struct iocblk);

	/*
	 * If there is data to copy into ioctl block, do so.
	 */
	if (iocbp->ioc_count) {
		if (transparent)
			id = K_TO_K;
		else
			id = copyflag;
		if (error = putiocd(bp, ebp, strioc->ic_dp, id, SE_NOSLP, fmtp)) {
			freemsg(bp);
			if (ebp)
				freeb(ebp);
			crfree(crp);
			return (error);
		}

		/*
		 * We could have slept copying in user pages.
		 * Recheck the stream head state (the other end
		 * of a pipe could have gone away).
		 */
		if (stp->sd_flag & (STRHUP|STRDERR|STWRERR|STPLEX)) {
			error = ((stp->sd_flag & STPLEX) ? EINVAL :
			    (stp->sd_werror ? stp->sd_werror : stp->sd_rerror));
			freemsg(bp);
			crfree(crp);
			return (error);
		}
	} else {
		bp->b_cont = ebp;
	}
	s = splstr();
	if (transparent)
		iocbp->ioc_count = TRANSPARENT;

	/*
	 * Block for up to STRTIMOUT sec if there is a outstanding
	 * ioctl for this stream already pending.  All processes
	 * sleeping here will be awakened as a result of an ACK
	 * or NAK being received for the outstanding ioctl, or
	 * as a result of the timer expiring on the outstanding
	 * ioctl (a failure), or as a result of any waiting
	 * process's timer expiring (also a failure).
	 */
	stp->sd_flag |= STR2TIME;
	id = timeout(str2time, (caddr_t)stp, STRTIMOUT*HZ);

	while (stp->sd_flag & IOCWAIT) {
		stp->sd_iocwait++;
		if (sleep((caddr_t)&stp->sd_iocwait,STIPRI|PCATCH) ||
		   !(stp->sd_flag & STR2TIME)) {
			stp->sd_iocwait--;
			error = ((stp->sd_flag & STR2TIME) ? EINTR : ETIME);
			if (!stp->sd_iocwait)
				stp->sd_flag &= ~STR2TIME;
			splx(s);
			untimeout(id);
			freemsg(bp);
			crfree(crp);
			return (error);
		}
		stp->sd_iocwait--;
		if (stp->sd_flag & (STRHUP|STRDERR|STWRERR|STPLEX)) {
			error = ((stp->sd_flag & STPLEX) ? EINVAL :
			    (stp->sd_werror ? stp->sd_werror : stp->sd_rerror));
			if (!stp->sd_iocwait)
				stp->sd_flag &= ~STR2TIME;
			splx(s);
			untimeout(id);
			freemsg(bp);
			crfree(crp);
			return (error);
		}
	}
	untimeout(id);
	if (!stp->sd_iocwait)
		stp->sd_flag &= ~STR2TIME;

	/*
	 * Have control of ioctl mechanism.
	 * Send down ioctl packet and wait for response.
	 */
	if (stp->sd_iocblk) {
		freemsg(stp->sd_iocblk);
		stp->sd_iocblk = NULL;
	}
	stp->sd_flag |= IOCWAIT;

	/* 
	 * Assign sequence number.
	 */
	iocbp->ioc_id = (stp->sd_iocid = ++ioc_id);
	saveiocid = stp->sd_iocid;

	splx(s);
	(*stp->sd_wrq->q_next->q_qinfo->qi_putp)(stp->sd_wrq->q_next, bp);
	if (qready())
		runqueues();

	/*
	 * Timed wait for acknowledgment.  The wait time is limited by the
	 * timeout value, which must be a positive integer (number of seconds
	 * to wait, or 0 (use default value of STRTIMOUT seconds), or -1
	 * (wait forever).  This will be awakened either by an ACK/NAK
	 * message arriving, the timer expiring, or the timer expiring 
	 * on another ioctl waiting for control of the mechanism.
	 */
waitioc:
	s = splstr();
	if (!(stp->sd_flag & STR3TIME) && strioc->ic_timout >= 0)
		id = timeout(str3time, (caddr_t) stp,
		    (strioc->ic_timout ? strioc->ic_timout: STRTIMOUT) * HZ);

	stp->sd_flag |= STR3TIME;
	/*
	 * If the reply has already arrived, don't sleep.  If awakened from
	 * the sleep, fail only if the reply has not arrived by then.
	 * Otherwise, process the reply.
	 */
	while (!stp->sd_iocblk) {
		if (stp->sd_flag & (STRDERR|STWRERR|STPLEX|STRHUP)) {
			error = ((stp->sd_flag & STPLEX) ? EINVAL :
			    (stp->sd_werror ? stp->sd_werror : stp->sd_rerror));
			stp->sd_flag &= ~(STR3TIME|IOCWAIT);
			if (strioc->ic_timout >= 0)
				untimeout(id);
			/* purge any outstanding post processing request */
			sp = (struct striopst *)findioc(saveiocid);
			if (sp)
				kmem_free(sp, sizeof(struct striopst));
			splx(s);
			wakeprocs((caddr_t)&(stp->sd_iocwait), PRMPT);
			crfree(crp);
			return (error);
		}

		if (sleep((caddr_t)stp,STIPRI|PCATCH) ||
		    !(stp->sd_flag & STR3TIME))  {
			error = ((stp->sd_flag & STR3TIME) ? EINTR : ETIME);
			stp->sd_flag &= ~(STR3TIME|IOCWAIT);
			bp = NULL;
			/*
			 * A message could have come in after we were scheduled
			 * but before we were actually run.
			 */
			if (stp->sd_iocblk) {
				bp = stp->sd_iocblk;
				stp->sd_iocblk = NULL;
			}
			if (strioc->ic_timout >= 0)
				untimeout(id);
			/* purge any outstanding post processing request */
			sp = (struct striopst *)findioc(saveiocid);
			if (sp)
				kmem_free(sp, sizeof(struct striopst));
			splx(s);
			if (bp) {
				if ((bp->b_datap->db_type == M_COPYIN) ||
				    (bp->b_datap->db_type == M_COPYOUT)) {
					if (bp->b_cont) {
						freemsg(bp->b_cont);
						bp->b_cont = NULL;
					}
					bp->b_datap->db_type = M_IOCDATA;
					resp = (struct copyresp *)bp->b_rptr;
					resp->cp_rval = (caddr_t)1;	/* failure */
					(*stp->sd_wrq->q_next->q_qinfo->qi_putp)(stp->sd_wrq->q_next, bp);
					if (qready())
						runqueues();
				} else
					freemsg(bp);
			}
			wakeprocs((caddr_t)&(stp->sd_iocwait), PRMPT);
			crfree(crp);
			return (error);
		}
	}
	ASSERT(stp->sd_iocblk);
	bp = stp->sd_iocblk;
	stp->sd_iocblk = NULL;
	if ((bp->b_datap->db_type == M_IOCACK) || (bp->b_datap->db_type == M_IOCNAK)) {
		stp->sd_flag &= ~(STR3TIME|IOCWAIT);
		if (strioc->ic_timout >= 0)
			untimeout(id);
		splx(s);
		wakeprocs((caddr_t)&(stp->sd_iocwait), PRMPT);
	} else {
		splx(s);
	}


	/*
	 * Have received acknowlegment.
	 */

	switch (bp->b_datap->db_type) {
	case M_IOCACK:
		/*
		 * Positive ack.
		 */
		iocbp = (struct iocblk *)bp->b_rptr;

		/*
		 * Set error if indicated.
		 */
		if (iocbp->ioc_error) {
			error = iocbp->ioc_error;
			break;
		}

		/*
		 * Set return value.
		 */
		*rvalp = iocbp->ioc_rval;

		/*
		 * Data may have been returned in ACK message (ioc_count > 0).
		 * If so, copy it out to the user's buffer.
		 */
		if (iocbp->ioc_count && !transparent) {
			if (strioc->ic_cmd == TCGETA ||
			    strioc->ic_cmd == TCGETS ||
			    strioc->ic_cmd == TIOCGETP ||
			    strioc->ic_cmd == LDGETT) {
				if (error = getiocd(bp, strioc->ic_dp,
				    copyflag, fmtp))
					break;
			} else {
/* Enhanced Application Compatibility Support */
				if (IS_SCOEXEC && !SCO_USES_SHNSL &&
				    strioc->ic_cmd == TI_GETINFO &&
				    modispushed(stp, "timod"))
					iocbp->ioc_count -= sizeof(long);
/* End Enhanced Application Compatibility Support */

				if (error = getiocd(bp, strioc->ic_dp,
			    			copyflag, (char *)NULL)) 
					break;
			}
		}
		if (!transparent) {
			if (len)	/* an M_COPYOUT was used with I_STR */
				strioc->ic_len = len;
			else
				strioc->ic_len = iocbp->ioc_count;
		}
/*
 * If there was a post processing request, now is the time to do it
 */
		sp = (struct striopst *)findioc(saveiocid);
		if (sp) {
			/* there was one */
			error = (*sp->sio_func)(sp->sio_arg, sp->sio_iocid, sp->sio_q, rvalp);
			kmem_free(sp, sizeof(struct striopst));
		}
		break;

	case M_IOCNAK:
		/*
		 * Negative ack.
		 *
		 * The only thing to do is set error as specified
		 * in neg ack packet.
		 */
		iocbp = (struct iocblk *)bp->b_rptr;

		error = (iocbp->ioc_error ? iocbp->ioc_error : EINVAL);
		break;

	case M_COPYIN:
		/*
		 * Driver or module has requested user ioctl data.
		 */
		reqp = (struct copyreq *)bp->b_rptr;
		fmtbp = bp->b_cont;
		bp->b_cont = NULL;
		if (reqp->cq_flag & RECOPY) {
			/* redo I_STR copyin with canonical processing */
			ASSERT(fmtbp);
			reqp->cq_size = strioc->ic_len;
			error = putiocd(bp, NULL, strioc->ic_dp, copyflag, SE_SLEEP,
			    (fmtbp ? (char *)fmtbp->b_rptr : (char *)NULL));
		} else if (reqp->cq_flag & STRCANON) {
			/* copyin with canonical processing */
			ASSERT(fmtbp);
			error = putiocd(bp, NULL, reqp->cq_addr, copyflag, SE_SLEEP,
			    (fmtbp ? (char *)fmtbp->b_rptr : (char *) NULL));
		} else {
			/* copyin raw data (i.e. no canonical processing) */
			error = putiocd(bp, NULL, reqp->cq_addr, copyflag,
			    SE_SLEEP, (char *)NULL);
		}
		if (fmtbp)
			freeb(fmtbp);
		if (error && bp->b_cont) {
			freemsg(bp->b_cont);
			bp->b_cont = NULL;
		}

		bp->b_wptr = bp->b_rptr + sizeof(struct copyresp);
		bp->b_datap->db_type = M_IOCDATA;
		resp = (struct copyresp *)bp->b_rptr;
		resp->cp_rval = (caddr_t)error;

		(*stp->sd_wrq->q_next->q_qinfo->qi_putp)(stp->sd_wrq->q_next, bp);
		if (qready())
			runqueues();

		if (error) {
			if (strioc->ic_timout >= 0)
				untimeout(id);
			crfree(crp);
			/* purge any outstanding post processing request */
			s = splstr();
			sp = (struct striopst *)findioc(saveiocid);
			splx(s);
			if (sp)
				kmem_free(sp, sizeof(struct striopst));
			return (error);
		}

		goto waitioc;

	case M_COPYOUT:
		/*
		 * Driver or module has ioctl data for a user.
		 */
		reqp = (struct copyreq *)bp->b_rptr;
		ASSERT(bp->b_cont);
		if (transparent) {
			taddr = reqp->cq_addr;
		} else {
			taddr = strioc->ic_dp;
			len = reqp->cq_size;
		}
		if (reqp->cq_flag & STRCANON) {
			/* copyout with canonical processing */
			if (fmtbp = bp->b_cont) {
				bp->b_cont = fmtbp->b_cont;
				fmtbp->b_cont = NULL;
			}
			error = getiocd(bp, taddr, copyflag,
			    (fmtbp ? (char *)fmtbp->b_rptr : (char *)NULL));
			if (fmtbp)
				freeb(fmtbp);
		} else {
			/* copyout raw data (i.e. no canonical processing) */
			error = getiocd(bp, taddr, copyflag, (char *)NULL);
		}
		freemsg(bp->b_cont);
		bp->b_cont = NULL;

		bp->b_wptr = bp->b_rptr + sizeof(struct copyresp);
		bp->b_datap->db_type = M_IOCDATA;
		resp = (struct copyresp *)bp->b_rptr;
		resp->cp_rval = (caddr_t)error;

		(*stp->sd_wrq->q_next->q_qinfo->qi_putp)(stp->sd_wrq->q_next, bp);
		if (qready())
			runqueues();

		if (error) {
			if (strioc->ic_timout >= 0)
				untimeout(id);
			crfree(crp);
			/* purge any outstanding post processing request */
			s = splstr();
			sp = (struct striopst *)findioc(saveiocid);
			splx(s);
			if (sp)
				kmem_free(sp, sizeof(struct striopst));
			return (error);
		}
		goto waitioc;

	default:
		ASSERT(0);
		break;
	}

	freemsg(bp);
	crfree(crp);
	/*
	 * purge any outstanding post processing request (this could happen
	 * on any errors above
	 */
	s = splstr();
	sp = (struct striopst *)findioc(saveiocid);
	splx(s);
	if (sp)
		kmem_free(sp, sizeof(struct striopst));
	return (error);
}

/*
 * Get the next message from the read queue.  If the message is 
 * priority, STRPRI will have been set by strrput().  This flag
 * should be reset only when the entire message at the front of the
 * queue as been consumed.
 */
int
strgetmsg(vp, mctl, mdata, prip, flagsp, fmode, rvp)
	register struct vnode *vp;
	register struct strbuf *mctl;
	register struct strbuf *mdata;
	unsigned char *prip;
	int *flagsp;
	int fmode;
	rval_t *rvp;
{
	register s;
	register struct stdata *stp;
	register mblk_t *bp, *nbp;
	mblk_t *savemp = NULL;
	mblk_t *savemptail = NULL;
	int n, bcnt;
	int done = 0;
	int flg = 0;
	int more = 0;
	int error = 0;
	char *ubuf;
	int mark;
	unsigned char pri;
	queue_t *q;

	ASSERT(vp->v_stream);
	stp = vp->v_stream;
	q = RD(stp->sd_wrq);
	rvp->r_val1 = 0;

	if (error = straccess(stp, JCREAD))
		return (error);
	if (stp->sd_flag & (STRDERR|STPLEX))
		return ((stp->sd_flag & STPLEX) ? EINVAL : stp->sd_rerror);

	switch (*flagsp) {
	case MSG_HIPRI:
		if (*prip != 0)
			return (EINVAL);
		break;

	case MSG_ANY:
	case MSG_BAND:
		break;

	default:
		return (EINVAL);
	}

	s = splstr();
	mark = 0;
	while (((*flagsp & MSG_HIPRI) && !(stp->sd_flag & STRPRI)) ||
	    ((*flagsp & MSG_BAND) && (!q->q_first ||
	    ((q->q_first->b_band < *prip) && !(stp->sd_flag & STRPRI)))) ||
	    !(bp = getq(q))) {
		/*
		 * If STRHUP, return 0 length control and data.
		 */
		if (stp->sd_flag & STRHUP) {
			mctl->len = mdata->len = 0;
			*flagsp = flg;
			splx(s);
			return (error);
		} 
		if ((error = strwaitq(stp, GETWAIT, (off_t)0, fmode, &done)) || done) {
			splx(s);
			return (error);
		}
	}
	if (bp == stp->sd_mark) {
		mark = 1;
		stp->sd_mark = NULL;
	}
	splx(s);
	
	if (bp->b_datap->db_type == M_PASSFP) {
		s = splstr();
		if (mark && !stp->sd_mark)
			stp->sd_mark = bp;
		putbq(q, bp);
		splx(s);
		return (EBADMSG);
	}

	pri = bp->b_band;
	if (qready())
		runqueues();

	/*
	 * Set HIPRI flag if message is priority.
	 */
	if (stp->sd_flag & STRPRI)
		flg = MSG_HIPRI;
	else
		flg = MSG_BAND;

	/*
	 * First process PROTO or PCPROTO blocks, if any.
	 */
	if (mctl->maxlen >= 0 && bp && bp->b_datap->db_type != M_DATA) {
		bcnt = mctl->maxlen;
		ubuf = mctl->buf;
		while (bp && bp->b_datap->db_type != M_DATA && bcnt >= 0) {
			if ((n = MIN(bcnt, bp->b_wptr - bp->b_rptr)) &&
			    copyout((caddr_t)bp->b_rptr, ubuf, n)) {
				error = EFAULT;
				s = splstr();
				stp->sd_flag &= ~STRPRI;
				splx(s);
				more = 0;
				freemsg(bp);
				goto getmout;
			}
			ubuf += n;
			bp->b_rptr += n;
			if (bp->b_rptr >= bp->b_wptr) {
				nbp = bp;
				bp = bp->b_cont;
				freeb(nbp);
			}
			if ((bcnt -= n) <= 0)
				break;
		}
		mctl->len = mctl->maxlen - bcnt;
	} else
		mctl->len = -1;
	
		
	if (bp && bp->b_datap->db_type != M_DATA) {	
		/*
		 * More PROTO blocks in msg.
		 */
		more |= MORECTL;
		savemp = bp;
		while (bp && bp->b_datap->db_type != M_DATA) {
			savemptail = bp;
			bp = bp->b_cont;
		}
		savemptail->b_cont = NULL;
	}

	/*
	 * Now process DATA blocks, if any.
	 */
	if (mdata->maxlen >= 0 && bp) {
		bcnt = mdata->maxlen;
		ubuf = mdata->buf;
		while (bp && bcnt >= 0) {
			if ((n = MIN(bcnt, bp->b_wptr - bp->b_rptr)) &&
			    copyout((caddr_t)bp->b_rptr, ubuf, n)) {
				error = EFAULT;
				s = splstr();
				stp->sd_flag &= ~STRPRI;
				splx(s);
				more = 0;
				freemsg(bp);
				goto getmout;
			}
			ubuf += n;
			bp->b_rptr += n;
			if (bp->b_rptr >= bp->b_wptr) {
				nbp = bp;
				bp = bp->b_cont;
				freeb(nbp);
			}
			if ((bcnt -= n) <= 0)
				break;
		}
		mdata->len = mdata->maxlen - bcnt;
	} else
		mdata->len = -1;

	if (bp) {			/* more data blocks in msg */
		more |= MOREDATA;
		if (savemp)
			savemptail->b_cont = bp;
		else
			savemp = bp;
	} 

	s = splstr();
	if (savemp) {
		savemp->b_band = pri;
		if (mark && !stp->sd_mark) {
			savemp->b_flag |= MSGMARK;
			stp->sd_mark = savemp;
		}
		putbq(q, savemp);
	} else {
		stp->sd_flag &= ~STRPRI;
	}
	splx(s);

	*flagsp = flg;
	*prip = pri;

	/*
	 * Getmsg cleanup processing - if the state of the queue has changed
	 * some signals may need to be sent and/or poll awakened.
	 */
getmout:
	while ((bp = q->q_first) && (bp->b_datap->db_type == M_SIG)) {
		bp = getq(q);
		strsignal(stp, *bp->b_rptr, (long)bp->b_band);
		freemsg(bp);
		if (qready())
			runqueues();
	}

	/*
	 * If we have just received a high priority message and a
	 * regular message is now at the front of the queue, send
	 * signals in S_INPUT processes and wake up processes polling
	 * on POLLIN.
	 */
	if ((bp = q->q_first) && !(stp->sd_flag & STRPRI)) {
 	    if (flg & MSG_HIPRI) {
		s = splstr();
		if (stp->sd_sigflags & S_INPUT) 
			strsendsig(stp->sd_siglist, S_INPUT, (long)bp->b_band);
		if (stp->sd_pollist.ph_events & POLLIN)
			pollwakeup(&stp->sd_pollist, POLLIN);

		if (bp->b_band == 0) {
		    if (stp->sd_sigflags & S_RDNORM)
			    strsendsig(stp->sd_siglist, S_RDNORM, 0L);
		    if (stp->sd_pollist.ph_events & POLLRDNORM) 
			    pollwakeup(&stp->sd_pollist, POLLRDNORM);
		} else {
		    if (stp->sd_sigflags & S_RDBAND)
			    strsendsig(stp->sd_siglist, S_RDBAND,
				(long)bp->b_band);
		    if (stp->sd_pollist.ph_events & POLLRDBAND) 
			    pollwakeup(&stp->sd_pollist, POLLRDBAND);
		}
		splx(s);
	    } else {
		if (pri != bp->b_band) {
		    s = splstr();
		    if (bp->b_band == 0) {
			if (stp->sd_sigflags & S_RDNORM)
				strsendsig(stp->sd_siglist, S_RDNORM, 0L);
			if (stp->sd_pollist.ph_events & POLLRDNORM) 
				pollwakeup(&stp->sd_pollist, POLLRDNORM);
		    } else {
			if (stp->sd_sigflags & S_RDBAND)
				strsendsig(stp->sd_siglist, S_RDBAND,
				    (long)bp->b_band);
			if (stp->sd_pollist.ph_events & POLLRDBAND) 
				pollwakeup(&stp->sd_pollist, POLLRDBAND);
		    }
		    splx(s);
		}
	    }
	}
	rvp->r_val1 = more;
	return (error);
}

/*
 * Put a message downstream.
 */
int
strputmsg(vp, mctl, mdata, pri, flag, fmode)
	register struct vnode *vp;
	register struct strbuf *mctl;
	register struct strbuf *mdata;
	unsigned char pri;
	register flag;
	int fmode;
{
	register struct stdata *stp;
	mblk_t *mp;
	register s;
	register long msgsize;
	long rmin, rmax;
	int error, done;
	struct uio uio;
	struct iovec iov;
	int strmakemsg();

	ASSERT(vp->v_stream);
	stp = vp->v_stream;

	if (error = straccess(stp, JCWRITE))
		return (error);
	if (stp->sd_flag & STPLEX)
		return (EINVAL);
	if (stp->sd_flag & (STRHUP|STWRERR)) {
		if (stp->sd_flag & STRSIGPIPE)
			psignal(u.u_procp, SIGPIPE);
		return(stp->sd_werror);
	}

	/*
	 * firewall - don't use too much memory
	 */

	if (strthresh && (Strcount > strthresh) && pm_denied(u.u_cred, P_SYSOPS))
		return (ENOSR);

	/*
	 * Check for legal flag value.
	 */
	switch (flag) {
	case MSG_HIPRI:
		if ((mctl->len < 0) || (pri != 0))
			return (EINVAL);
		break;
	case MSG_BAND:
		break;

	default:
		return (EINVAL);
	}

	/*
	 * Make sure ctl and data sizes together fall within the
	 * limits of the max and min receive packet sizes and do
	 * not exceed system limit.
	 */
	rmin = stp->sd_wrq->q_next->q_minpsz;
	rmax = stp->sd_wrq->q_next->q_maxpsz;
	ASSERT((rmax >= 0) || (rmax == INFPSZ));
	if (rmax == 0)
		return (ERANGE);
	if (strmsgsz != 0) {
		if (rmax == INFPSZ)
			rmax = strmsgsz;
		else
			rmax = MIN(strmsgsz, rmax);
	}
	if ((msgsize = mdata->len) < 0) {
		msgsize = 0;
		rmin = 0;	/* no range check for NULL data part */
	}
	if ((msgsize < rmin) ||
	    ((msgsize > rmax) && (rmax != INFPSZ)) ||
	    (mctl->len > strctlsz))
		return (ERANGE);

	s = splstr();
	while (!(flag & MSG_HIPRI) && !bcanput(stp->sd_wrq->q_next, pri)) {
		if ((error = strwaitq(stp, WRITEWAIT, (off_t) 0, fmode, &done)) || done) {
			splx(s);
			return (error);
		}
	}
	splx(s);

	iov.iov_base = mdata->buf;
	iov.iov_len = mdata->len;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_offset = 0;
	uio.uio_segflg = UIO_USERSPACE;
	uio.uio_fmode = 0;
	uio.uio_resid = iov.iov_len;
	if ((error = strmakemsg(mctl, mdata->len, &uio, stp, (long)flag, &mp)) || !mp)
		return (error);
	mp->b_band = pri;

	/*
	 * Put message downstream.
	 */
	(*stp->sd_wrq->q_next->q_qinfo->qi_putp)(stp->sd_wrq->q_next, mp);
	if (qready())
		runqueues();
	return (0);
}

/*
 * Determines whether the necessary conditions are set on a stream
 * for it to be readable, writeable, or have exceptions.
 */
int
strpoll(stp, events, anyyet, reventsp, phpp)
	register struct stdata *stp;
	short events;
	int anyyet;
	short *reventsp;
	struct pollhead **phpp;
{
	register short retevents = 0;
	register s;
	register mblk_t *mp;
	qband_t *qbp;

	if (stp->sd_flag & STPLEX) {
		*reventsp = POLLNVAL;
		return (0);
	}
	s = splstr();

	if (stp->sd_flag & STRDERR) {
		if ((events == 0) ||
		    (events & (POLLIN|POLLPRI|POLLRDNORM|POLLRDBAND))) {
			splx(s);
			*reventsp = POLLERR;
			return(0);
		}
	}
	if (stp->sd_flag & STWRERR) {
		if ((events == 0) || (events & (POLLOUT|POLLWRBAND))) {
			splx(s);
			*reventsp = POLLERR;
			return (0);
		}
	}

	if (stp->sd_flag & STRHUP) {
		retevents |= POLLHUP;
	} else {
		register queue_t *tq;

		tq = stp->sd_wrq->q_next;
		while (tq->q_next && !tq->q_qinfo->qi_srvp)
	    		tq = tq->q_next;
		if (events & POLLWRNORM) {
			if (tq->q_flag & QFULL)
				/* ensure backq svc procedure runs */
				tq->q_flag |= QWANTW;
			else
				retevents |= POLLOUT;
		}
		if (events & POLLWRBAND) {
			qbp = tq->q_bandp;
			if (qbp) {
				while (qbp) {
					if (qbp->qb_flag & QB_FULL)
						qbp->qb_flag |= QB_WANTW;
					else
						retevents |= POLLWRBAND;
					qbp = qbp->qb_next;
				}
			} else {
				retevents |= POLLWRBAND;
			}
		}
	}

	mp = RD(stp->sd_wrq)->q_first;
	if (!(stp->sd_flag & STRPRI)) {
		while (mp) {
			if ((events & POLLRDNORM) && (mp->b_band == 0))
				retevents |= POLLRDNORM;
			if ((events & POLLRDBAND) && (mp->b_band > 0))
				retevents |= POLLRDBAND;
			if (events & POLLIN)
				retevents |= POLLIN;

			/*
			 * MSGNOGET is really only to make poll return
			 * the intended events when the module is really
			 * holding onto the data.  Yeah, it's a hack and
			 * we need a better solution.
			 */
			if (mp->b_flag & MSGNOGET)
				mp = mp->b_next;
			else
				break;
		}
	} else {
		ASSERT(mp != NULL);
		if (events & POLLPRI)
			retevents |= POLLPRI;
	}
	*reventsp = retevents;
	if (retevents) {
		splx(s);
		return (0);
	}

	/*
	 * If poll() has not found any events yet, set up event cell
	 * to wake up the poll if a requested event occurs on this
	 * stream.  Check for collisions with outstanding poll requests.
	 */
	if (!anyyet) {
		*phpp = &stp->sd_pollist;
	}
	splx(s);
	return (0);
}

STATIC int
strsink(q, bp)
	register queue_t *q;
	register mblk_t *bp;
{
	struct copyresp *resp;

	switch (bp->b_datap->db_type) {
	case M_FLUSH:
		if ((*bp->b_rptr & FLUSHW) && !(bp->b_flag & MSGNOLOOP)) {
			*bp->b_rptr &= ~FLUSHR;
			bp->b_flag |= MSGNOLOOP;
			qreply(q, bp);
		} else {
			freemsg(bp);
		}
		break;

	case M_COPYIN:
	case M_COPYOUT:
		if (bp->b_cont) {
			freemsg(bp->b_cont);
			bp->b_cont = NULL;
		}
		bp->b_datap->db_type = M_IOCDATA;
		resp = (struct copyresp *)bp->b_rptr;
		resp->cp_rval = (caddr_t)1;	/* failure */
		qreply(q, bp);
		break;

	case M_IOCTL:
		if (bp->b_cont) {
			freemsg(bp->b_cont);
			bp->b_cont = NULL;
		}
		bp->b_datap->db_type = M_IOCNAK;
		qreply(q, bp);
		break;

	default:
		freemsg(bp);
		break;
	}
}

/* 
 * 386-specific ioctl handler for redirection of ioctls from /dev/vt*
 * to associated video memory device (/dev/kdvm00 for /dev/vt00, etc.)
 * This routine must be defined for anyone using the STREAMS-based
 * keyboard/display driver.
 * Return 0 if we did not service the ioctl, 1 if we did.
 */
int 
stri386ioctl(vp, cmdp, arg, rvalp, errorp)
        struct vnode *vp;
 	int *cmdp;
 	int arg;
 	int *rvalp, *errorp;
{
        dev_t dev = 0;
	int	cmd = *cmdp;
	major_t majnum;
	minor_t minnum;
	int oldpri;

 	extern gvid_t Gvid;
 	extern int gvidflg;
	extern void ws_setcompatflgs(), ws_clrcompatflgs();
	extern int ws_compatset();
	/* Enhanced Application Compatibility Support */
	int	remap_streams_ioctls(),
		remap_posix_ioctls();
	/* End Enhanced Application Compatibility Support */
 
	if(vp != NULL) { /* we are called FROM strioctl and NOT from the KD driver */
        	dev = vp->v_rdev;
		*errorp = 0;
	 	if (! (gvidflg & GVID_SET)) 
 			return (0); /* cannot deal with ioctl for the moment.
 			     */
 		dev = vp->v_rdev;
 		majnum = getmajor(dev);
 
		oldpri = splhi();
 		while (gvidflg & GVID_ACCESS) /* sleep */
 			if (sleep((caddr_t) &gvidflg, STOPRI|PCATCH)) {
				splx(oldpri);
 				*errorp = EINTR;
 				/* even if ioctl was not ours, we've
 			 	* effectively handled it */
 				return (1);
 			}

		gvidflg |= GVID_ACCESS; 
		splx(oldpri);
	
 		/* return if ioctl meant for someone else */
 		if (majnum != Gvid.gvid_maj) {
			gvidflg &= ~GVID_ACCESS; 
			wakeup((caddr_t) &gvidflg);
			return (0);
		}

 		minnum = getminor(dev);
	 	if (minnum >= Gvid.gvid_num) {
 			*errorp = ENXIO;
			gvidflg &= ~GVID_ACCESS; 
			wakeup((caddr_t) &gvidflg);
 			return (1); /* this technically shouldn't happen. */
 		}
 
		/* done with lookup in Gvid. Release access to it */
		gvidflg &= ~GVID_ACCESS; 
		wakeup((caddr_t) &gvidflg);
	
	}
	/* screen out ioctls that we know do not get redirected */
	switch (cmd & 0xffffff00) {
/* Enhanced Application Compatibility Support */
	case S_IOC:	/* 'X' << 8 */
		if(vp == NULL) 
			return(1);
		break;
	case O_MODESWITCH:	/* 'S'<<8 */
		*cmdp = remap_streams_ioctls(dev, cmd);
		break;
	case USL_OLD_MODESWITCH:  /* 'x'<<8 */
		*cmdp = remap_posix_ioctls(dev, cmd);
		break;
	case LDEV_MOUSE:
	case EVLD_IOC:
	case SCO_C_IOC:
/* End Enhanced Application Compatibility Support */
	case KIOC:
	case VTIOC:
	case MODESWITCH:
	case CGAIOC:
	case EGAIOC:
	case MCAIOC:
	case PGAIOC:
	case CONSIOC: /* also handles GIO_COLOR */
	case MAPADAPTER:
	case MIOC:
	case WSIOC:
	case GIO_ATTR:
		break;

	default:/* not redirected */
		if(vp == NULL)
			return (1); 
		return (0);
	}

	if(vp == NULL)
		return(0); 

 	switch (*cmdp) {

	case WS_GETSYSVCOMPAT:
		*rvalp = ws_sysv_compatset(dev, arg);
		return (1);

	case WS_CLRSYSVCOMPAT:
		ws_sysv_clrcompatflgs(dev, arg);
		return (1);

	case WS_SETSYSVCOMPAT:
		ws_sysv_setcompatflgs(dev, arg);
		return (1);

	case WS_SETXXCOMPAT:
		ws_setcompatflgs(dev);
		return (1);
 
	case WS_CLRXXCOMPAT:
		ws_clrcompatflgs(dev);
		return (1);
 
	case WS_GETXXCOMPAT:
		*rvalp = ws_compatset(dev);
		return (1);
 
	case XENIX_SPECIAL_IOPRIVL: 
		/* resolve conflict with new TERMIOS TCGETX, STSET ioctls */
 		if (isXOUT || ws_compatset(dev)) {
			*cmdp = SPECIAL_IOPRIVL;
			cmn_err(CE_NOTE, "redirecting XENIX_SPECIAL_IOPRIVL");
		}
		break;
 	} /* switch */
 
	return (0);
}

/* Enhanced Application Compatibility Support */
int
modeswitch3_2(cmd)
int	cmd;
{
static struct {
	int	svr4_val;
	} modeswitch_tbl[] = {
/*SVR4 */	/*USL SVR3.2 */ 
DM_B40x25,	/* 0 */		/* 40x25 black & white text */
DM_C40x25,	/* 1 */		/* 40x25 color text */
DM_B80x25,	/* 2 */		/* 80x25 black & white text */
DM_C80x25,	/* 3 */		/* 80x25 color text */
DM_BG320,	/* 4 */		/* 320x200 black & white graphics */
DM_CG320,	/* 5 */		/* 320x200 color graphics */
DM_BG640,	/* 6 */		/* 640x200 black & white graphics */
DM_EGAMONO80x25,/* 7 */		/* EGA mode 7 */
LOAD_COLOR,	/* 8 */		/* mode for loading color characters */
LOAD_MONO,	/* 9 */		/* mode for loading mono characters */
DM_ENH_B80x43,	/* 10 */	/* 80x43 black & white text */
DM_ENH_C80x43,	/* 11 */	/* 80x43 color text */
DM_ENH_C80x43,	/* 12 */	/* Not in SVR3.2. 80x43 color text */
DM_CG320_D,	/* 13 */	/* EGA mode D */
DM_CG640_E,	/* 14 */	/* EGA mode E */
DM_EGAMONOAPA,	/* 15 */	/* EGA mode F */
DM_CG640x350,	/* 16 */	/* EGA mode 10 */
DM_ENHMONOAPA2,	/* 17 */	/* EGA mode F with extended memory */
DM_ENH_CG640,	/* 18 */	/* EGA mode 10* */
DM_ENH_B40x25,	/* 19 */	/* enhanced 40x25 black & white text */
DM_ENH_C40x25,	/* 20 */	/* enhanced 40x25 color text */
DM_ENH_B80x25,	/* 21 */	/* enhanced 80x25 black & white text */
DM_ENH_C80x25,	/* 22 */	/* enhanced 80x25 color text */
DM_ENH_CGA,	/* 23 */	/* AT&T 640x400 CGA hw emulation mode */
DM_ATT_640,	/* 24 */	/* AT&T 640x400 16 color graphics */
DM_VGA_C40x25,	/* 25 */	/* VGA 40x25 color text */
DM_VGA_C80x25,	/* 26 */	/* VGA 80x25 color text */
DM_VGAMONO80x25,/* 27 */	/* VGA mode 7 */
DM_VGA640x480C,	/* 28 */	/* VGA 640x480 2 color graphics */
DM_VGA640x480E,	/* 29 */	/* VGA 640x480 16 color graphics */
DM_VGA320x200,	/* 30 */	/* VGA 320x200 256 color graphics */
DM_VDC800x600E,	/* 31 */	/* VDC-600 800x600 16 color graphics */
DM_VDC640x400V	/* 32 */	/* VDC-600 640x400 256 color graphics */
};
	int	newcmd;

	if((cmd & 0xff) > 32)
		return(cmd);
	newcmd = MODESWITCH | modeswitch_tbl[(cmd & 0xff)].svr4_val;
	return(newcmd);
}

int
remap_streams_ioctls(dev, cmd)
dev_t	dev;
int	cmd;
{
	int	newcmd;

	if(VIRTUAL_XOUT) {
		if(ws_sysv_compatset(dev, 4))
			newcmd = STR | (cmd & 0xff);
		else	newcmd = MODESWITCH | (cmd & 0xff);

	} else if( isCOFF) {
		if(ws_sysv_compatset(dev, 3) || ws_sysv_compatset(dev, 4))
			newcmd = STR | (cmd & 0xff);
		else	newcmd = MODESWITCH | (cmd & 0xff);
	} else { /* SVR4 */
		if(ws_sysv_compatset(dev, 3))
			newcmd = modeswitch3_2(cmd);
		else	if(ws_sysv_compatset(dev, 4)||ws_compatset(dev))
			newcmd = MODESWITCH | (cmd & 0xff);
		else	newcmd = STR | (cmd & 0xff);
	}
	return(newcmd);
}


int
remap_posix_ioctls(dev, cmd)
dev_t	dev;
int	cmd;
{
	int	newcmd;

	if(VIRTUAL_XOUT) {
		newcmd = (cmd & ~IOCTYPE) | SCO_XIOC;
	} else if( isCOFF) {
		if(ws_vdcset(dev) || ws_sysv_compatset(dev, 3))
			newcmd = modeswitch3_2(cmd);
 		else	if(ws_compatset(dev))
			newcmd = (cmd & ~IOCTYPE) | SCO_XIOC;
		else	newcmd = MODESWITCH | (cmd & 0xff);
	} else { /* SVR4 */
		if(ws_sysv_compatset(dev, 3))
			newcmd = modeswitch3_2(cmd);
		else	if(ws_compatset(dev))
				newcmd = (cmd & ~IOCTYPE) | SCO_XIOC;
		else	newcmd = MODESWITCH | (cmd & 0xff);
	}
	return(newcmd);
}
/* Enhanced Application Compatibility Support */


