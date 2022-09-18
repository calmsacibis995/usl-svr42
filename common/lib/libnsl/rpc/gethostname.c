/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)libnsl:common/lib/libnsl/rpc/gethostname.c	1.2.8.1"
#ident  "$Header: gethostname.c 1.2 91/06/26 $"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice 
*
* Notice of copyright on this source code product does not indicate 
*  publication.
*
*	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991  UNIX System Laboratories, Inc.
*          All rights reserved.
*/ 

#include <sys/systeminfo.h>
#include <sys/errno.h>

#ifdef _NSL_RPC_ABI
/* For internal use only when building the libnsl RPC routines */
#define	sysinfo	_abi_sysinfo
#endif
/* 
 * gethostname bsd compatibility
 */

extern	int	errno;

gethostname(hname, hlen)
char *hname;
int hlen;
{
	int	error;

	error = sysinfo(SI_HOSTNAME, hname, hlen);
	/*
	 * error > 0 ==> number of bytes to hold name
	 * and is discarded since gethostname only
	 * cares if it succeeded or failed
	 */
	return (error == -1 ? -1 : 0);
}
