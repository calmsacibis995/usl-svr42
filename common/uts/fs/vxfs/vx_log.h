/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* @(#)usr/src/common/uts/fs/vxfs/vx_log.h	1.3 12 Apr 1992 16:23:54 -  */
#ident	"@(#)uts-comm:fs/vxfs/vx_log.h	1.3"

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

#ifndef _FS_VXFS_VX_LOG_H
#define _FS_VXFS_VX_LOG_H

/*
 * Generic log record - variable length
 * Log record consists of 1 or more functions constituting a
 * transaction. Each function in the transaction has the same
 * log_id value. The log_ser value is incremented for each function.
 * The log record for a function is always a multiple of 16 bytes
 * and does not cross a VX_ATOMIC_IOSZ boundary. The log_len is always the
 * exact number of data bytes, and does not include padding. If the
 * dependent data will cause a VX_ATOMIC_IOSZ boundary to be crossed, a
 * continuation record is used. The continuation records have the
 * same log_id, log_func, log_ser fields, but have an incrementing log_fser
 * field. Since many log records are short, one log block may contain
 * multiple transactions.
 *
 * log_id is the transaction id number, which is an incrementing serial
 * number held from fs_logid. If the id wraps to zero, it is incremented
 * to one, so an id of zero indicates nothing there. There should never
 * be a missing number in the log. A change in log_id indicates that a
 * new transaction has begun.
 *
 * log_func is the function type
 *
 * log_ser is the function number within the transaction (0-127)
 * log_lser is the last function number within the transaction
 *
 * log_fser is the continuation record number within the function (0-127)
 * log_lfser is the last continuation record number within the function
 * a continuation record is used whenever a VX_ATOMIC_IOSZ boundary is crossed
 *
 * log_len is the number of data bytes in the record. If a function has
 * continuation records, the number of data bytes in the function is the
 * sum of the log_len fields. Note that records are always a multiple of
 * 16 bytes, but the log_len field is always exact.
 */

#define	VX_LOGFILL	4			/* round to 16 bytes */

struct vx_log {
	long		log_id;			/* transaction id number */
	short		log_func;		/* function */
	char		log_ser;		/* serial # within trans */
	char		log_lser;		/* last serial # within trans */
	char		log_fser;		/* serial # within func */
	char		log_lfser;		/* last serial # within func */
	short		log_len;		/* data length */
	char		log_depend[VX_LOGFILL];	/* variable */
};

#define VX_LOGMASK	(sizeof (struct vx_log) - 1)
#define VX_LOGOVER	(sizeof(struct vx_log) - VX_LOGFILL)
#define VX_LOGMAX	(VX_ATOMIC_IOSZ - VX_LOGOVER)

#define VX_LOGROUNDUP(len) \
	((len + VX_LOGOVER + VX_LOGMASK)  & (~VX_LOGMASK))

/*
 * This is the size of a done or undo subfunction.  To make sure we
 * don't cross any sector boundaries when putting done records into the
 * log, we round the buffer offset to a VX_DONELOGSIZE boundary.  The
 * code assumes that VX_ATOMIC_IOSZ is a multiple of VX_DONELOGSIZE.
 * The done/undo subfunction contains 8 bytes of data.
 */

#define VX_DONELOGSIZE	0x20

/*
 * When the logid reaches 1 billion, we reset the logid at the next
 * 60 second sync interval.  By doing this, we can avoid dealing
 * with logid's that wrap.  If the reset hasn't happened by the
 * time 500 million more transactions occur, then the file system
 * will be disabled.
 */

#define	VX_MAXLOGID	(1 << 30)
#define	VX_DISLOGID	(VX_MAXLOGID + (1 << 29))

#endif /* _FS_VXFS_VX_LOG_H */
