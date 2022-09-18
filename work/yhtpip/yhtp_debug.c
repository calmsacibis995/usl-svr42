
/*
 * SVR 4.2 STREAMS YHTP - Release 1.0 
 *
 * Copyright 1994 NET612 Computer Department of NUDT 
 * All Rights Reserved. 
 */


#define YHTPSTATES
#define	YHTPTIMERS
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
#include <net/yhtpip/protosw.h>
#include <svc/errno.h>
#include <net/yhtpip/yhroute.h>
#include <net/yhtpip/yhif.h>
#include <net/yhtpip/yhin.h>
#include <net/yhtpip/yhin_pcb.h>
#include <net/yhtpip/yhin_systm.h>
#include <net/yhtpip/yhip.h>
#include <net/yhtpip/yhip_var.h>
#include <net/yhtpip/yhtp.h>
#include <net/yhtpip/yhtp_fsm.h>
#include <net/yhtpip/yhtp_seq.h>
#include <net/yhtpip/yhtp_timer.h>
#include <net/yhtpip/yhtp_var.h>
#include <net/yhtpip/yhtpip.h>
#include <net/yhtpip/yhtp_debug.h>
#include <io/strlog.h>

int	yhtpconsdebug = 0;	/* send debug printfs to console if set */
int	yhtpalldebug = 0;	/* trace all connections if set		*/
int	yhtp_debx;

/*
 * Tcp debug routines
 */
/*ARGSUSED*/
yhtp_trace(act, ostate, tp, ti, req)
	short act, ostate;
	struct yhtpcb *tp;
	struct yhtpiphdr *ti;
	int req;
{
	yhtp_seq seq, ack;
	int len, flags;
	int	blen;
	char	outbuf[512];

	if (yhtp_ndebug > 0 ) {
		struct yhtp_debug *td;

		if (yhtp_debx == yhtp_ndebug)
			yhtp_debx = 0;
		td = &yhtp_debug[yhtp_debx++];
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
	if (yhtpconsdebug == 0)
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
	blen += yhdbgstrcpy(outbuf, "\nYHTP:	");
	if (tp) {
		sprintf(&outbuf[blen], "%x %s:", tp, yhtpstates[ostate]);
		blen = strlen(outbuf);
	} else
		blen += yhdbgstrcpy(&outbuf[blen], "???????? :");

	sprintf(&outbuf[blen], "%s ", yhtanames[act]);
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
			len -= sizeof (struct yhtphdr);
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
#define pf(f)	{ if (ti->ti_flags & f) { outbuf[blen++] = cp; blen += yhdbgstrcpy(&outbuf[blen], "f"); cp = ',' ; } }
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
		sprintf(&outbuf[blen], "%s", yhtli_primitives[req & 0xff]);
		break;
	case TA_TIMER:
		sprintf(&outbuf[blen], "<%s>", yhtptimers[req]);
		break;
	default:
		break;
	}

	if (tp == 0) {
		strlog(YHTPM_ID, 1, 1, SL_TRACE, outbuf);
		return;
	}
	blen = strlen(outbuf);
	if (tp->t_state > YHTP_NSTATES || tp->t_state < 0 )
		sprintf(&outbuf[blen], " -> Bad State (%d)",
			tp->t_state);
	else
		sprintf(&outbuf[blen], " -> %s",
			yhtpstates[tp->t_state]);
	blen = strlen(outbuf);

	sprintf(&outbuf[blen], "\n\trcv_(nxt,wnd,up) (%x,%x,%x)\n\tsnd_(una,nxt,max) (%x,%x,%x)\n", tp->rcv_nxt, tp->rcv_wnd, tp->rcv_up, tp->snd_una, tp->snd_nxt, tp->snd_max);
	blen = strlen(outbuf);
	sprintf(&outbuf[blen], "\tsnd_(wl1,wl2,wnd) (%x,%x,%x)\n\tsnd_cwnd %x", tp->snd_wl1, tp->snd_wl2, tp->snd_wnd, tp->snd_cwnd);
	strlog(YHTPM_ID, 1, 1, SL_TRACE, outbuf);

#endif /* DEBUG */

	return;
}

int
yhdbgstrcpy(src,dst)
register char	*src,*dst;
{
	register int i = 0;
	while ( *src++ = *dst++ )
		++i;
	return(i);
}
