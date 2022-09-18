/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:net/tcpip/tcp_debug.c	1.6.3.1"
#ident	"$Header: $"

/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991 UNIX System Laboratories, Inc.
 *  	          All rights reserved.
 */

/*
 * System V STREAMS TCP - Release 3.0 
 *
 * Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI) 
 * All Rights Reserved. 
 *
 * The copyright above and this notice must be preserved in all copies of this
 * source code.  The copyright above does not evidence any actual or intended
 * publication of this source code. 
 *
 * This is unpublished proprietary trade secret source code of Lachman
 * Associates.  This source code may not be copied, disclosed, distributed,
 * demonstrated or licensed except as expressly authorized by Lachman
 * Associates. 
 *
 * System V STREAMS TCP was jointly developed by Lachman Associates and
 * Convergent Technologies. 
 */


#define TCPSTATES
#define	TCPTIMERS
#define	TANAMES
#define PRUREQUESTS
#define TLI_PRIMS

#define STRNET

#include <util/types.h>
#include <util/param.h>
#include <svc/systm.h>
#include <io/stream.h>
#include <net/transport/socket.h>
#include <net/transport/socketvar.h>
#include <net/tcpip/protosw.h>
#include <svc/errno.h>
#include <net/tcpip/route.h>
#include <net/tcpip/if.h>
#include <net/tcpip/in.h>
#include <net/tcpip/in_pcb.h>
#include <net/tcpip/in_systm.h>
#include <net/tcpip/ip.h>
#include <net/tcpip/ip_var.h>
#include <net/tcpip/tcp.h>
#include <net/tcpip/tcp_fsm.h>
#include <net/tcpip/tcp_seq.h>
#include <net/tcpip/tcp_timer.h>
#include <net/tcpip/tcp_var.h>
#include <net/tcpip/tcpip.h>
#include <net/tcpip/tcp_debug.h>
#include <io/strlog.h>

int	tcpconsdebug = 0;	/* send debug printfs to console if set */
int	tcpalldebug = 0;	/* trace all connections if set		*/
int	tcp_debx;

/*
 * Tcp debug routines
 */
/*ARGSUSED*/
tcp_trace(act, ostate, tp, ti, req)
	short act, ostate;
	struct tcpcb *tp;
	struct tcpiphdr *ti;
	int req;
{
	tcp_seq seq, ack;
	int len, flags;
	int	blen;
	char	outbuf[512];

	if (tcp_ndebug > 0 ) {
		struct tcp_debug *td;

		if (tcp_debx == tcp_ndebug)
			tcp_debx = 0;
		td = &tcp_debug[tcp_debx++];
		td->td_time = iptime();
		td->td_act = act;
		td->td_ostate = ostate;
		td->td_tcb = (caddr_t)tp;
		if (tp)
			td->td_cb = *tp;
		else
			bzero((caddr_t)&td->td_cb, sizeof (*tp));
		if (ti)
			td->td_ti = *ti;
		else
			bzero((caddr_t)&td->td_ti, sizeof (*ti));
		td->td_req = req;
	}
	if (tcpconsdebug == 0)
		return;
#ifdef DEBUG

	/*
	 * "consdebug" is now a misnomer since debugging info goes
	 * to strlog (the volume of output was swamping the console).
	 * can't pass strings from kernel space up to user with strlog,
	 * so must do 
	 *	sprintf(buf, "%s", state[i]);
	 *	strlog(..., buf);
	 * rather than
	 *	strlog(..., "%s", state[i]);
	 * also, strlog appends a newline to each format arg it's given,
	 * so must construct entire outbuf before calling strlog.
	 */
	blen = 0;
	bzero(outbuf, sizeof(outbuf));
	blen += dbgstrcpy(outbuf, "\nTCP:	");
	if (tp) {
		sprintf(&outbuf[blen], "%x %s:", tp, tcpstates[ostate]);
		blen = strlen(outbuf);
	} else
		blen += dbgstrcpy(&outbuf[blen], "???????? :");

	sprintf(&outbuf[blen], "%s ", tanames[act]);
	blen = strlen(outbuf);

	switch (act) {
	case TA_INPUT:
	case TA_OUTPUT:
	case TA_DROP:
		if (ti == 0)
			break;
		seq = ti->ti_seq;
		ack = ti->ti_ack;
		len = ti->ti_len;
		if (act == TA_OUTPUT) {
			seq = ntohl(seq);
			ack = ntohl(ack);
			len = ntohs((u_short)len);
		}
		if (act == TA_OUTPUT)
			len -= sizeof (struct tcphdr);
		if (len)
			sprintf(&outbuf[blen], "[%x..%x]", seq, seq + len);
		else
			sprintf(&outbuf[blen], "%x", seq);
		blen = strlen(outbuf);
		sprintf(&outbuf[blen], "@%x, urp=%x ", ack, ti->ti_urp);
		blen = strlen(outbuf);
		flags = ti->ti_flags;
		if (flags) {
#ifndef lint
			char cp = '<';
#define pf(f)	{ if (ti->ti_flags & f) { outbuf[blen++] = cp; blen += dbgstrcpy(&outbuf[blen], "f"); cp = ',' ; } }
			pf(TH_SYN);
			pf(TH_ACK);
			pf(TH_FIN);
			pf(TH_RST);
			pf(TH_PUSH);
			pf(TH_URG);
#endif /* lint */
			outbuf[blen++] = '>';
		}
		break;

	case TA_USER:
		sprintf(&outbuf[blen], "%s", tli_primitives[req & 0xff]);
		break;
	case TA_TIMER:
		sprintf(&outbuf[blen], "<%s>", tcptimers[req]);
		break;
	default:
		break;
	}

	if (tp == 0) {
		strlog(TCPM_ID, 1, 1, SL_TRACE, outbuf);
		return;
	}
	blen = strlen(outbuf);
	if (tp->t_state > TCP_NSTATES || tp->t_state < 0 )
		sprintf(&outbuf[blen], " -> Bad State (%d)",
			tp->t_state);
	else
		sprintf(&outbuf[blen], " -> %s",
			tcpstates[tp->t_state]);
	blen = strlen(outbuf);

	sprintf(&outbuf[blen], "\n\trcv_(nxt,wnd,up) (%x,%x,%x)\n\tsnd_(una,nxt,max) (%x,%x,%x)\n", tp->rcv_nxt, tp->rcv_wnd, tp->rcv_up, tp->snd_una, tp->snd_nxt, tp->snd_max);
	blen = strlen(outbuf);
	sprintf(&outbuf[blen], "\tsnd_(wl1,wl2,wnd) (%x,%x,%x)\n\tsnd_cwnd %x", tp->snd_wl1, tp->snd_wl2, tp->snd_wnd, tp->snd_cwnd);
	strlog(TCPM_ID, 1, 1, SL_TRACE, outbuf);

#endif /* DEBUG */

	return;
}

int
dbgstrcpy(src,dst)
register char	*src,*dst;
{
	register int i = 0;
	while ( *src++ = *dst++ )
		++i;
	return(i);
}
