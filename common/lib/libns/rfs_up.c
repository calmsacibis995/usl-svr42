/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libns:common/lib/libns/rfs_up.c	1.1.8.2"
#ident  "$Header: rfs_up.c 1.2 91/06/26 $"
#include <sys/types.h>
#include <sys/nserve.h>
#include <sys/list.h>
#include <sys/rf_sys.h>
#include <sys/vnode.h>
#include <sys/rf_messg.h>
#include <sys/rf_comm.h>
#include <errno.h>
#include <nserve.h>
#include <stdio.h>
#include "nslog.h"

/*
 * Return 0 if RFS is running, -1 otherwise.
 */
rfs_up()
{
	int rfstate;

	LOG2(L_TRACE, "(%5d) enter: ns_up\n", Logstamp);

	if ((rfstate = rfsys(RF_RUNSTATE)) != RF_UP) {
		errno = ENONET;
		LOG2(L_TRACE, "(%5d) leave: ns_up\n", Logstamp);
		return -1;
	} else {
		LOG2(L_TRACE, "(%5d) leave: ns_up\n", Logstamp);
		return 0;
	}
}
