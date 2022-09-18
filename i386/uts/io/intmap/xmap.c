/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:io/intmap/xmap.c	1.3"
#ident	"$Header: $"

/*
 *	Copyright (C) The Santa Cruz Operation, 1988-1989.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential.
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
							/* BEGIN SCO_INTL */
/*
 *	routine to allocate struct xmap.  This structure is used as an
 *	extension to the tty structure by the emap and nmap routines.
 *	The space is obtained by permanently snaffling inboard buffers.
 *
 *	Also includes routine to clear all of tty struct without losing
 *	the xmap pointer (the xmap is cleared as well).  This is called
 *	from sxtopen which previously wiped the whole struct tty.
 *
 *	MODIFICATION HISTORY
 *	created	18 Feb 88	scol!craig
 *	S001	31 Dec 88	buckm
 *	- Changes for MP.
 */

#include <util/types.h>
#include <svc/errno.h>
#include <util/param.h>
#include <fs/buf.h>
#include <io/tty.h>
#include <io/intmap/emap.h>
#include <io/intmap/xmap.h>
#include <svc/systm.h>
#include <io/stream.h>
#include <mem/kmem.h>
#include <io/ddi.h>
#include <util/cmn_err.h>

#ifdef	SCO_DEBUG
#define I18N_DEBUG(a)	 cmn_err(CE_NOTE,  a)
#endif

#define	XMAPSPERBUF	(E_TABSZ / sizeof(struct xmap))

static int xmapsleft = 0;
struct xmap *nextxmap;

xmapalloc(tp)
struct tty *tp;
{	struct buf *bp;

#ifdef SCO_DEBUG
	I18N_DEBUG("xmapalloc: \n");
#endif
	if (!xmapsleft) {
		bp = ngeteblk(E_TABSZ);	/* Get a buffer */
		bzero((caddr_t)paddr(bp), E_TABSZ);	/* Zero it */
		xmapsleft = XMAPSPERBUF;
		nextxmap = (struct xmap *)paddr(bp);
	}

	tp->t_xmp = nextxmap;
	nextxmap++;
	xmapsleft--;
}

xmttyclr(tp)
struct tty *tp;
{	struct xmap *savexmp;

#ifdef SCO_DEBUG
	I18N_DEBUG("xmttyclr: \n");
#endif
	savexmp = tp->t_xmp;
	bzero((caddr_t)tp, sizeof(struct tty));
	if (savexmp) {
		bzero((caddr_t)savexmp, sizeof(struct xmap));
		tp->t_xmp = savexmp;
	}
}

str_xmapalloc(emp_tp)
struct emp_tty *emp_tp;
{	struct buf *bp;

#ifdef SCO_DEBUG
	I18N_DEBUG("str_xmapalloc: \n");
#endif
	if (!xmapsleft) {
		bp = getrbuf(KM_NOSLEEP); 		/* Get a buffer */
		if (bp == (struct buf *) NULL)
			return (ENOMEM);
		bp->b_un.b_addr = kmem_alloc(E_TABSZ,KM_NOSLEEP);
		if (paddr(bp) == (paddr_t) NULL) {
			freerbuf(bp);
			return (ENOMEM);
		}

		bzero((caddr_t)paddr(bp), E_TABSZ);	/* Zero it */
		xmapsleft = XMAPSPERBUF;
		nextxmap = (struct xmap *)paddr(bp);
	}

	emp_tp->t_xmp = nextxmap;
	nextxmap++;
	xmapsleft--;
}
