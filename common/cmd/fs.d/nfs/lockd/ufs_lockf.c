/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nfs.cmds:nfs/lockd/ufs_lockf.c	1.4.5.3"
#ident	"$Header: $"

#include <sys/types.h>
#include "prot_lock.h"

int				LockID = 0; /* Monotonically increasing id */


/*
 * return a string representation of the lock.
 */
void
print_lock(l)
	struct data_lock	*l;
{

	printf("[ID=%d, pid=%d, base=%d, len=%d, type=%s rsys=%x]",
		l->LockID, l->lld.l_pid, l->lld.l_start, l->lld.l_len,
		(l->lld.l_type == F_WRLCK) ? "EXCL" : "SHRD",
		l->lld.l_sysid);
	return;
}
