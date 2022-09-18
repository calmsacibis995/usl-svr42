/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:net/tcpip/in_transp.c	1.3.3.1"
#ident	"$Header: $"

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991 UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */


/*
 * This module provides generic handler routines for various transparent
 * ioctls.
 */

#define STRNET

#ifdef INET
#include <net/tcpip/symredef.h>
#endif INET

#include <util/types.h>
#include <util/param.h>
#include <svc/time.h>
#include <util/sysmacros.h>
#include <svc/errno.h>
#include <io/stropts.h>
#include <io/stream.h>
#include <io/strlog.h>
#include <io/log/log.h>
#include <net/tcpip/nihdr.h>
#include <net/dlpi.h>
#include <net/transport/socket.h>
#include <net/transport/sockio.h>
#include <net/tcpip/if.h>
#include <net/tcpip/strioc.h>
#include <net/tcpip/in.h>
#include <net/tcpip/in_var.h>
#include <net/tcpip/route.h>
#include <net/tcpip/ip_str.h>
#include <net/tcpip/ip_var.h>
#include <net/tcpip/in_pcb.h>

#ifdef SYSV
#include <util/cmn_err.h>
#endif SYSV
#include <mem/kmem.h>
#include <net/transport/timod.h>

inet_doname(q, bp)
	queue_t		*q;
	mblk_t		*bp;
{
	struct inpcb *inp = qtoinp(q);
	struct sockaddr_in localaddr;
	struct sockaddr_in remoteaddr;

	if (inp == (struct inpcb *) 0) {
		/* strlog this */
		return;
	}

	bzero((caddr_t) &localaddr, sizeof(localaddr));
	bzero((caddr_t) &remoteaddr, sizeof(remoteaddr));
	localaddr.sin_family = AF_INET;
	localaddr.sin_addr = inp->inp_laddr;
	localaddr.sin_port = inp->inp_lport;
	remoteaddr.sin_family = AF_INET;
	remoteaddr.sin_addr = inp->inp_faddr;
	remoteaddr.sin_port = inp->inp_fport;

	switch (bp->b_datap->db_type) {
	case M_IOCTL:
		inp->inp_iocstate = INP_IOCS_DONAME;
		if (ti_doname(q, bp, (caddr_t) &localaddr, sizeof (localaddr),
			      (caddr_t) &remoteaddr, sizeof (remoteaddr))
		    != DONAME_CONT)
			inp->inp_iocstate = 0;
		break;
			

	case M_IOCDATA:
		if (ti_doname(q, bp, (caddr_t) &localaddr, sizeof (localaddr),
			      (caddr_t) &remoteaddr, sizeof (remoteaddr))
		    != DONAME_CONT)
			inp->inp_iocstate = 0;
		break;
	}
}


