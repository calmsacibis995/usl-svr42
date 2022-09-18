
/*
 * SVR 4.2 STREAMS YHTP - Release 1.0 
 *
 * Copyright 1994 NET612 Computer Department of NUDT 
 * All Rights Reserved. 
 */

/*
 * This module provides generic handler routines for various transparent
 * ioctls.
 */

#define STRNET
#include <util/types.h>
#include <util/param.h>
#include <svc/time.h>
#include <util/sysmacros.h>
#include <svc/errno.h>
#include <io/stropts.h>
#include <io/stream.h>
#include <io/strlog.h>
#include <io/log/log.h>
#include <net/yhtpip/nihdr.h>
#include <net/dlpi.h>
#include <net/transport/socket.h>
#include <net/transport/sockio.h>
#include <net/yhtpip/yhif.h>
#include <net/yhtpip/strioc.h>
#include <net/yhtpip/yhin.h>
#include <net/yhtpip/yhin_var.h>
#include <net/yhtpip/yhroute.h>
#include <net/yhtpip/yhip_str.h>
#include <net/yhtpip/yhip_var.h>
#include <net/yhtpip/yhin_pcb.h>

#ifdef SYSV
#include <util/cmn_err.h>
#endif SYSV
#include <mem/kmem.h>
#include <net/transport/timod.h>

yhinet_doname(q, bp)
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
	localaddr.sin_family = AF_OSI;
	localaddr.sin_addr = inp->inp_laddr;
	localaddr.sin_port = inp->inp_lport;
	remoteaddr.sin_family = AF_OSI;
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


