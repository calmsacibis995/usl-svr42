/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/mpt/mpt.c	1.7"
#ident	"$Header: $"

/* Enhanced Application Compatibility Support */
/*
 *	Copyright (C) The Santa Cruz Operation, 1988, 1989.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated 
 *	as Confidential.
 */

/*
 * The code marked with symbols from the list below, is owned
 * by The Santa Cruz Operation Inc., and represents SCO value
 * added portions of source code requiring special arrangements
 * with SCO for inclusion in any product.
 *  Symbol:		 Market Module:
 * SCO_BASE 		Platform Binding Code 
 * SCO_ENH 		Enhanced Device Driver
 * SCO_ADM 		System Administration & Miscellaneous Tools 
 * SCO_C2TCB 		SCO Trusted Computing Base-TCB C2 Level 
 * SCO_DEVSYS 		SCO Development System Extension 
 * SCO_INTL 		SCO Internationalization Extension
 * SCO_BTCB 		SCO Trusted Computing Base TCB B Level Extension 
 * SCO_REALTIME 	SCO Realtime Extension 
 * SCO_HIGHPERF 	SCO High Performance Tape and Disk Device Drivers	
 * SCO_VID 		SCO Video and Graphics Device Drivers (2.3.x)		
 * SCO_TOOLS 		SCO System Administration Tools				
 * SCO_FS 		Alternate File Systems 
 * SCO_GAMES 		SCO Games
 */
							/* BEGIN SCO_BASE */
/*
 * pseudo tty driver
 */

/*
 *	MODIFICATION HISTORY
 *	M000	07/08/85	lees
 *	- Removed #include "../h/ioctl.h", since tty.h includes it via
 *	  termio.h.
 *	S000	05/05/87	timons
 *	- Added ioctl to allow blocking/non-blocking mode switching 
 *	  for mptread.
 *	????	05/21/87	timons
 *	- 386'ed
 *	S001	30 June 87	sams
 *	- Fix mptioctl() : use copyin and cvttoaddr()
 *	S002	24 Sept 87	timons
 *	- fixed sign extention bug in mptioctl.  MPT_BLK ioctls from 286
 *	  binaries were getting sign extended and therefore were not working.
 *	L000	22 Apr 88	scol!peters
 *	- Select system call (Mod Hist added by scol!craig).
 *	L001	25 Apr 88	scol!craig
 *	- Changes to L000 to move new fields out of tty struct into xmap
 *	  struct, thus preserving binary compatibility for drivers.
 *	L003	06 June 88	graemet
 *	- added VP/IX support.  Much of this comes from the sio driver,
 *	  and is conditional on VPIX.
 *	S004	28 June	88	timon
 *	- modifications to graeme's code to merge into xnet source tree.
 *	  Added dynamic allocation of the vpix struct ptr array and changed 
 * 	  some things to conform to standards.
 *	S005	6 Oct 88	sco!jeffj
 *	- Have mptselect call selfailure() when select conditions
 *	  are not met.
 *	S006	17 Jan 89	timon
 *	- mods for 3.2 unix
 *	S007	7 Feb 89	sco!buckm
 *	Pick up latest mods from Xenix:
 *	- Have mptread() check FNDELAY for non-blocking read.
 *	- use ttyflush() to flush in mptclose() and mptioctl().
 *	S008	10 Feb 89	sco!jamescb
 *	- cleanup of S006. Use minor(dev) where dev is passed in
 *	  to a function. Not #ifdef'd on M_S_UNIX 'cuz it's generic.
 *	S009	31 Jul 89	sco!curtisg
 *	- Fixed minor bug caused by output delays in OPOST mode.
 *	  A QESC character in the input gets translated to two
 *	  QESC's, but should be converted back into one on output.
 *	  Really, we should call the line discipline output code
 *	  here, I think.
 *	S010	8 Dec 89	sco!asharpe
 *	- POSIX conformance: fully support the FNONBLOCK flag,
 *	  setting the proper u.u_error.
 *	S011	14 Feb 90	sco!loranb
 *	- Have mptwrite() check FNDELAY for non-blocking write.
 */

/*
 * Since this driver is part of an add-on package, we always define VPIX (for
 * the 386).  This is because we don't know which kind of kernel we'll be
 * running on: one with or without VPIX support, so it is safer to assume
 * VPIX support.  All of the VPIX support in this code should be harmless
 * when running with a non-VPIX kernel.
 */

#include <util/param.h>
#include <util/types.h>
#include <util/sysmacros.h>
#include <proc/signal.h>
#include <svc/errno.h>
#include <io/conf.h>
#include <mem/page.h>
#include <proc/seg.h>
#include <mem/immu.h>
#include <proc/user.h>
#include <svc/systm.h>		/* for Hz */
#include <io/tty.h>
#include <io/spt/spt.h>
#include <io/ttold.h>						/*begin	*L000*/
#include <fs/file.h>
#include <io/poll.h>
#include <proc/proc.h>						/*end	*L000*/
#include <mem/kmem.h>
#include <io/termio.h>
#include <io/asy/asy.h>
#include <proc/tss.h>
#include <fs/fcntl.h>
#include <util/mod/moddefs.h>

#define DRVNAME "mpt - Loadable pseudo tty driver."

MOD_DRV_WRAPPER(mpt, NULL, NULL, NULL, DRVNAME);

#ifdef VPIX					/* v L003 v , S004 */
#include <vpix/v86.h>
#endif 						/* ^ L003 ^ */
/*
 *     A pseudo tty is a special device which is not unlike a pipe.
 * It is used to communicate between two processes.  However, it allows
 * one to simulate a teletype, including mode setting, interrupt, and
 * multiple end of files (all not possible on a pipe).
 *     There are really two drivers here.  One is the device which looks
 * like a tty and can be thought of as the slave device, and hence its
 * routines are prefixed with 'spt' (slave pseudo tty).  The other driver
 * can be thought of as the master device, and its routines are prefixed
 * by 'mpt' (master pseudo tty).
 *     To type on the simulated keyboard of the pseudo tty, one does a 'write'
 * to the master device.  To get the simulated printout from the pseudo tty,
 * one does a 'read' on the master device.
 *     Normally, the master device is called 'ptyn' and the slave device is
 * called 'ttypn' (to make programs like 'who' happy).
 */

extern struct tty spt_tty[];
extern char mptflags[];
extern int nspttys;
extern struct pollhead *spt_php[];
extern struct pollhead *mpt_php[];


#ifdef VPIX
extern caddr_t spt_v86p[];     /* save proc.p_v86 here for pseudo ints, S004 */
#endif

int mptdevflag = D_OLD;

/*
 *	The master pseudo tty routines.
 */

mptopen(dev, flag)
dev_t dev;
int flag;
{
	register struct tty *tp;
	extern	int sptproc();

	dev=minor(dev);
	if (dev >= nspttys) {
		u.u_error = ENXIO;
		return;
	}
	tp = &spt_tty[dev];
	if (mptflags[dev] & MPT_OPEN ) {
		u.u_error = EIO;
		return;
	}

	/*
	 * S000 blocking kludge
	 * start all devices in the default blocking mode
	 */
	mptflags[dev] |= ( MPT_OPEN | MPT_BLK_FLAG );
	tp->t_proc = sptproc;		/* start routine */
	tp->t_state |= CARR_ON;
	if (tp->t_state & WOPEN) {
		wakeup((caddr_t) &tp->t_canq);
	}

	if (mpt_php[dev] == NULL)
		mpt_php[dev] =
			(struct pollhead *)kmem_zalloc(sizeof(struct pollhead),
						       KM_SLEEP);
}


mptclose(dev)
dev_t dev;
{
	register struct tty *tp;

	dev=minor(dev);
	tp = &spt_tty[dev];

	if (tp->t_state & ISOPEN) {
		signal(tp->t_pgrp, SIGHUP);
	}
	tp->t_state &= ~CARR_ON;	/* virtual carrier is gone */
	ttyflush(tp, (FREAD|FWRITE));	/* clean things up	S007 */

	mptflags[dev] = 0;		/* mark as closed */

	kmem_free((_VOID *)mpt_php[dev], sizeof(struct pollhead));
	mpt_php[dev] = NULL;
}


mptread(dev)
dev_t dev;
{
	register struct tty  	*tp;
	register int c;				/* S009 */

	dev=minor(dev);
	tp = &spt_tty[dev];
	if ((tp->t_state & (CARR_ON|ISOPEN)) == 0)
		return;

	if (mptflags[dev] & MPT_BLK_FLAG) {
		while (tp->t_outq.c_cc == 0 || (tp->t_state & TTSTOP)) {
								/* S010 vvv */
			if (u.u_fmode & FNONBLOCK) {
				u.u_error = EAGAIN;
				return;
			}
			if (u.u_fmode&FNDELAY)
				return;
								/* S010 ^^^ */
		    	/* woken by sptproc */
			sleep((caddr_t) &tp->t_outq.c_cf, TTIPRI);
		}
	}
	else {	/* S000 - non-blocking mode */
		if ((tp->t_outq.c_cc == 0) || (tp->t_state & TTSTOP)) {
			goto skip_data;
		}
	}
/* S009 v */
	if (!(tp->t_oflag & OPOST))
	    while ( tp->t_outq.c_cc && passc(getc(&tp->t_outq) & 0377) >= 0 )
		;
	else
	    while ( tp->t_outq.c_cc) {
		c = getc(&tp->t_outq);
		if (c == QESC)
			(void) getc(&tp->t_outq);
		if (passc(c & 0377) < 0)
			break;
	    }
/* S009 ^ */

skip_data:

	if ((tp->t_state & TTIOW) && tp->t_outq.c_cc == 0) {
		wakeup((caddr_t) &tp->t_oflag);
		tp->t_state &= ~TTIOW;
	}
	if (tp->t_outq.c_cc <= ttlowat[tp->t_cflag & CBAUD]) {	/*begin	*L000*/
		if (tp->t_state & OASLP) {
			tp->t_state &= ~OASLP;
			wakeup((caddr_t) &tp->t_outq);
		}
		pollwakeup(spt_php[dev], POLLWRNORM);
	}
}


mptwrite(dev)
dev_t dev;
{
	register struct tty *tp;
	register unsigned n;
	register char *cp;
	char buf[100];

	dev=minor(dev);
	tp = &spt_tty[dev];
	if ((tp->t_state & (CARR_ON|ISOPEN)) == 0) {
		return;
	}
	if ( tp->t_rbuf.c_ptr == NULL )
		return;
	while (n = u.u_count) {
		if (n > sizeof(buf)) {
			n = sizeof(buf);
		}
		if (copyin(u.u_base, (caddr_t) buf, n) == -1) {
			u.u_error = EFAULT;
			break;
		}
		u.u_offset += n;
		u.u_base += n;
		u.u_count -= n;
		cp = buf;
		while (n--) {
			while (tp->t_delct && tp->t_rawq.c_cc >= TTYHOG - 2) {
			    /* too full; wait for something to be read */
				wakeup((caddr_t) &tp->t_rawq);
				if (u.u_fmode & FNDELAY) {   /* S011 start */
					u.u_offset -= n+1;
					u.u_base -= n+1;
					u.u_count += n+1;
					return;		     
				}			     /* S011 end */
				sleep((caddr_t) &tp->t_rawq.c_cf, TTOPRI);
			}
			if (tp->t_iflag&IXON) {
				register int ctmp;

				ctmp = *cp;
				if( tp->t_state & TTSTOP ) {	
					if (ctmp == CSTART || tp->t_iflag&IXANY)
						(*tp->t_proc)(tp, T_RESUME);
				} else {
					if (ctmp == CSTOP)
						(*tp->t_proc)(tp, T_SUSPEND);
				}
				if (ctmp == CSTART || ctmp == CSTOP)
					return;
			}
#ifdef VPIX							/* v L003 v */
			if ((tp->t_iflag & DOSMODE) && !tp->t_rawq.c_cc 
		     	     && spt_v86p[dev]) {
				v86setint(spt_v86p[dev], V86VI_KBD);
			}
#endif /* VPIX */						/* ^ L003 ^ */
			*tp->t_rbuf.c_ptr = *cp++;
			tp->t_rbuf.c_count--;
			(*linesw[tp->t_line].l_input)(tp, L_BUF);
		}
	}
}


mptioctl(dev, cmd, arg, mode)
int dev;
int cmd;
faddr_t arg;
int mode;
{
	register struct tty *tp = &spt_tty[minor(dev)];		/*begin	*L000*/
	short argval;						/* S002 */

	dev=minor(dev);

 	if (cmd == TIOCSETP) {
		/* master must flush to prevent hanging */
		ttyflush(tp, (FREAD|FWRITE));			/* S007 */
	}
	else if ((cmd & 0xffff) == MPT_BLK ) {			/* S002 */
			/* vvv S001 */
		copyin(arg, (caddr_t)&argval, sizeof(short));	/* S002 */

		if (argval)
			/* ^^^ S001 */
		    mptflags[dev] |= MPT_BLK_FLAG;
		else
		    mptflags[dev] &= (~MPT_BLK_FLAG);
		return;
	}
	ttiocom(tp, cmd, arg, dev);
	if ((tp->t_state & TTIOW) && tp->t_outq.c_cc == 0) {
		wakeup((caddr_t) &tp->t_oflag);
		tp->t_state &= ~TTIOW;
	}
}

								/*begin	*L000*/
mptchpoll(dev, events, anyyet, reventsp, phpp)
	dev_t dev;
	short events;
	int anyyet;
	short *reventsp;
	struct pollhead **phpp;
{
	struct tty *tp;
	struct proc *p;
	int s;

	dev = minor(dev);
	*reventsp = 0;
	tp = &spt_tty[dev];

	if ((tp->t_state&CARR_ON) == 0)
		*reventsp |= POLLHUP;
	else {
		if (events & POLLRDNORM) {
			/*
			 * Need to block timeouts (ttrstart).
			 */
			s = spl5();
			if ((tp->t_state&ISOPEN) &&
			    tp->t_outq.c_cc && (tp->t_state&TTSTOP) == 0)
				*reventsp |= POLLRDNORM;
			splx(s);
		}

		if ((events & POLLWRNORM) && (tp->t_state&ISOPEN) &&
		    (tp->t_rawq.c_cc + tp->t_canq.c_cc < TTYHOG-2 ||
		     tp->t_canq.c_cc == 0 && !(tp->t_lflag & ICANON)))
			    *reventsp |= POLLWRNORM;

	}

	if (*reventsp == 0 && !anyyet)
		*phpp = mpt_php[dev];

	return 0;
}
							/* END SCO_BASE */

/* End Enhanced Application Compatibility Support */
