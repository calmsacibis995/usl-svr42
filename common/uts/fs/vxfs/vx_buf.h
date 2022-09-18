/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* @(#)usr/src/common/uts/fs/vxfs/vx_buf.h	1.3 18 Apr 1992 23:31:23 -  */
#ident	"@(#)uts-comm:fs/vxfs/vx_buf.h	1.4"

/*
 * Copyright (c) 1991, 1992 VERITAS Software Corporation.  ALL RIGHTS RESERVED.
 * UNPUBLISHED -- RIGHTS RESERVED UNDER THE COPYRIGHT
 * LAWS OF THE UNITED STATES.  USE OF A COPYRIGHT NOTICE
 * IS PRECAUTIONARY ONLY AND DOES NOT IMPLY PUBLICATION
 * OR DISCLOSURE.
 * 
 * THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 * TRADE SECRETS OF VERITAS SOFTWARE.  USE, DISCLOSURE,
 * OR REPRODUCTION IS PROHIBITED WITHOUT THE PRIOR
 * EXPRESS WRITTEN PERMISSION OF VERITAS SOFTWARE.
 * 
 *               RESTRICTED RIGHTS LEGEND
 * USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT IS
 * SUBJECT TO RESTRICTIONS AS SET FORTH IN SUBPARAGRAPH
 * (C) (1) (ii) OF THE RIGHTS IN TECHNICAL DATA AND
 * COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013.
 *               VERITAS SOFTWARE
 * 4800 GREAT AMERICA PARKWAY, SUITE 420, SANTA CLARA, CA 95054
 */

/*
 * Portions Copyright 1992, 1991 UNIX System Laboratories, Inc.
 * Portions Copyright 1990 - 1984 AT&T
 * All Rights Reserved
 */

#ifndef	_FS_VXFS_VX_BUF_H
#define	_FS_VXFS_VX_BUF_H


/*
 *  vx_buf.h--buffer management for vxfs file system.
 */

#define	VX_BUFWAIT	0	/* pseudo-flag; wait for buf if locked */
#define	VX_NOBUFWAIT	1	/* don't wait for locked buf */
#define	VX_INCORE	2	/* only get buffer if incore */
#define	VX_NONBLOCK	4	/* non blocking read for pageout */

#define	VX_BUFHOLD(size)	{ \
	vx_bufspace += (size) >> 10; \
	TED_ASSERT("f:bufhold:1a", \
		vx_bufspace >= 0 && vx_bufspace <= v.v_bufhwm && /*TED_*/ \
		size >= DEV_BSIZE && (size & DEV_BSIZE - 1) == 0); /*TED_*/ \
}

#define	VX_BUFRELE(size)	{ \
	vx_bufspace -= (size) >> 10; \
	if (vx_bufsleep && vx_bufspace <= vx_maxbufspace) { \
		vx_bufsleep = 0; \
		wakeup((caddr_t)&vx_bufspace); \
	} else {	/*TED_*/ \
	TED_ASSERT("f:bufrele:1a", vx_bufspace >= 0 && \
		vx_bufspace <= v.v_bufhwm && /*TED_*/ \
		size >= DEV_BSIZE && (size & DEV_BSIZE - 1) == 0); /*TED_*/ \
	} \
}

/*
 * Unlink a buffer from the available list and mark it busy.
 */

#define vx_notavail(bp) { \
	(bp)->av_back->av_forw = (bp)->av_forw; \
	(bp)->av_forw->av_back = (bp)->av_back; \
	(bp)->b_flags |= B_BUSY; \
	bfreelist.b_bcount--; \
}

#define	VX_PAGEPROC	(u.u_procp == proc_pageout || u.u_procp == proc_sched)

#endif	/* _FS_VXFS_VX_BUF_H */
