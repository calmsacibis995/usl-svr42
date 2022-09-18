/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:net/tcpip/udp_state.c	1.5.3.2"
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

#define STRNET

#ifdef INET
#include <net/tcpip/symredef.h>
#endif INET

#include <util/types.h>
#include <util/param.h>
#include <util/spl.h>

#include <io/stream.h>
#include <io/log/log.h>
#include <io/strlog.h>

#ifdef SYSV
#include <net/transport/tihdr.h>
#include <net/transport/tiuser.h>
#else
#include <net/transport/tihdr.h>
#include <net/transport/tiuser.h>
#endif SYSV

#include <net/tcpip/nihdr.h>
#include <net/tcpip/protosw.h>
#include <net/transport/socket.h>
#include <net/transport/socketvar.h>
#include <svc/errno.h>
#include <net/tcpip/if.h>
#include <net/tcpip/in.h>
#include <net/tcpip/in_var.h>
#include <net/tcpip/route.h>
#include <net/tcpip/in_pcb.h>
#include <net/tcpip/in_systm.h>
#include <net/tcpip/ip.h>
#include <net/tcpip/ip_var.h>
#include <net/tcpip/ip_icmp.h>
#include <net/tcpip/udp.h>
#include <net/tcpip/udp_var.h>
#include <net/tcpip/ip_str.h>


mblk_t         *reallocb();

#define CHECKSIZE(bp,size) if (((bp) = reallocb((bp), (size),0)) == NULL) {\
			return;\
			}

/*
 * this is the subfunction of the upper put routine which handles data and
 * protocol packets for us.
 */

udp_state(q, bp)
	queue_t        *q;
	register mblk_t *bp;
{
	register union T_primitives *t_prim;
	register struct inpcb *inp = qtoinp(q);
	int             error = 0;
	mblk_t         *head;
	struct sockaddr_in *sin;
	struct in_addr  laddr;
	struct in_addr	taddr;
	register int	s;
	struct inpcb	finp;

	/*
	 * check for pending error, or a broken state machine
	 */

	STRLOG(UDPM_ID, 0, 9, SL_TRACE, "got to udp_state");
	if (inp->inp_error != 0) {
		T_errorack(q, bp, TSYSERR, inp->inp_error);
		return;
	}
	/* just send pure data, if we're ready */
	if (bp->b_datap->db_type == M_DATA) {
		if ((inp->inp_state & SS_ISCONNECTED) != 0) {
			udp_output(inp, bp, (struct in_addr *)0);
		} else {
			CHECKSIZE(bp, sizeof(struct T_error_ack));
			bp->b_datap->db_type = M_PCPROTO;
			t_prim = (union T_primitives *) bp->b_rptr;
			bp->b_wptr = bp->b_rptr + sizeof(struct T_error_ack);
			t_prim->type = T_ERROR_ACK;
			t_prim->error_ack.ERROR_prim = T_DATA_REQ;
			t_prim->error_ack.TLI_error = TOUTSTATE;
			qreply(q, bp);
		}
		return;
	}
	/* if it's not data, it's proto or pcproto */

	t_prim = (union T_primitives *) bp->b_rptr;
	STRLOG(UDPM_ID, 0, 9, SL_TRACE, "Proto msg, type is %d", t_prim->type);

	switch (t_prim->type) {
	case T_INFO_REQ:
		/* our state doesn't matter here */
		CHECKSIZE(bp, sizeof(struct T_info_ack));
		bp->b_rptr = bp->b_datap->db_base;
		bp->b_wptr = bp->b_rptr + sizeof(struct T_info_ack);
		t_prim = (union T_primitives *) bp->b_rptr;
		t_prim->type = T_INFO_ACK;
		t_prim->info_ack.TSDU_size = 16 * 1024;
		t_prim->info_ack.ETSDU_size = -2;
		t_prim->info_ack.CDATA_size = -2;	/* ==> not supported */
		t_prim->info_ack.DDATA_size = -2;
		t_prim->info_ack.ADDR_size = sizeof(struct sockaddr_in);
		t_prim->info_ack.OPT_size = -1;
		t_prim->info_ack.TIDU_size = 16 * 1024;
		t_prim->info_ack.SERV_type = T_CLTS;
		t_prim->info_ack.CURRENT_state = inp->inp_tstate;
		t_prim->info_ack.PROVIDER_flag = TP_SNDZERO;
		bp->b_datap->db_type = M_PCPROTO;	/* make sure */
		qreply(q, bp);
		break;

	case T_BIND_REQ:
		if (inp->inp_tstate != TS_UNBND) {
			T_errorack(q, bp, TOUTSTATE, 0);
			break;
		}
		if (t_prim->bind_req.ADDR_length == 0) {
			error = in_pcbbind(inp, (mblk_t *) NULL);
		} else {
			bp->b_rptr += t_prim->bind_req.ADDR_offset;
			error = in_pcbbind(inp, bp);
			bp->b_rptr -= t_prim->bind_req.ADDR_offset;
		}
		if (error == EACCES) {
			T_errorack(q, bp, TACCES, 0);
			error = 0;
			break;
		}
		if (error == EINVAL) {
			T_errorack(q, bp, TBADADDR, 0);
			error = 0;
			break;
		}
		if (error)
			break;
		inp->inp_tstate = TS_IDLE;
		if ((bp = reallocb(bp, sizeof(struct T_bind_ack)
				   + inp->inp_addrlen, 1))
		    == NULL) {
			return;
		}
		t_prim = (union T_primitives *) bp->b_rptr;
		t_prim->bind_ack.PRIM_type = T_BIND_ACK;
		t_prim->bind_ack.ADDR_length = inp->inp_addrlen;
		t_prim->bind_ack.ADDR_offset = sizeof(struct T_bind_req);
		sin = (struct sockaddr_in *)
			(bp->b_rptr + sizeof(struct T_bind_ack));
		bp->b_wptr = (unsigned char *)
			(((caddr_t) sin) + inp->inp_addrlen);
		bzero((caddr_t) sin, inp->inp_addrlen);
		sin->sin_family = inp->inp_family;
		sin->sin_addr = inp->inp_laddr;
		sin->sin_port = inp->inp_lport;
		bp->b_datap->db_type = M_PCPROTO;
		qreply(q, bp);
		break;

	case T_UNBIND_REQ:
		if (inp->inp_tstate != TS_IDLE) {
			T_errorack(q, bp, TOUTSTATE, 0);
			break;
		}
		in_pcbdisconnect(inp);
		inp->inp_laddr.s_addr = INADDR_ANY;
		inp->inp_lport = 0;
		inp->inp_tstate = TS_UNBND;
		T_okack(q, bp);
		break;

		/*
		 * Initiate connection to peer. For udp this is simply faked
		 * by asigning a pseudo-connection, and sending up a
		 * conection confirmation.
		 */
	case T_CONN_REQ:{
			if (inp->inp_tstate != TS_IDLE) {
				T_errorack(q, bp, TOUTSTATE, 0);
				break;
			}
			bp->b_rptr += t_prim->conn_req.DEST_offset;
			error = in_pcbconnect(inp, bp);
			bp->b_rptr -= t_prim->conn_req.DEST_offset;
			if (error == EINVAL) {
				T_errorack(q, bp, TBADADDR, 0);
				error = 0;
				break;
			}
			if (error)
				break;
			T_okack(q, bp);
			T_conn_con(inp);
			break;
		}

	case T_DISCON_REQ:
		if (inp->inp_faddr.s_addr == INADDR_ANY) {
			error = ENOTCONN;
			break;
		}
		in_pcbdisconnect(inp);
		inp->inp_state &= ~SS_ISCONNECTED;	/* XXX */
		T_okack(q, bp);
		break;

	case T_OPTMGMT_REQ:
		udp_ctloutput(q, bp);
		break;

	case T_DATA_REQ:
		if ((inp->inp_state & SS_ISCONNECTED) == 0) {
			freemsg(bp);	/* TLI doesn't want errors here */
			break;
		}
		head = bp;
		bp = bp->b_cont;
		head->b_cont = NULL;
		freeb(head);
		if (bp == NULL) {
			break;
		}
		udp_output(inp, bp, (struct in_addr *)0);
		break;

	case T_UNITDATA_REQ:
		laddr = inp->inp_laddr;
		if (inp->inp_faddr.s_addr != INADDR_ANY) {
			error = EISCONN;
			break;
		}
		if (bp->b_cont == NULL) {
			freeb(bp);
			break;
		}
		bp->b_rptr += t_prim->unitdata_req.DEST_offset;
		s = splstr();
		error = in_pcbconnect(inp, bp);
		finp.inp_laddr = inp->inp_laddr;
		finp.inp_lport = inp->inp_lport;
		finp.inp_faddr = inp->inp_faddr;
		finp.inp_fport = inp->inp_fport;
		in_pcbdisconnect(inp);
		inp->inp_laddr = laddr;
		(void) splx(s);
		bp->b_rptr -= t_prim->unitdata_req.DEST_offset;
		if (error == EINVAL) {
			T_errorack(q, bp, TBADADDR, 0);
			error = 0;
			break;
		}
		if (error)
			break;
		head = bp;
		bp = bp->b_cont;
		head->b_cont = NULL;
		freeb(head);
		udp_output(inp, bp, &finp);
		break;

	case T_CONN_RES:
	case T_ORDREL_REQ:
	case T_EXDATA_REQ:
	default:
		T_errorack(q, bp, TNOTSUPPORT, 0);
		return;
	}
	if (error)
		T_errorack(q, bp, TSYSERR, error);
}

udp_ctloutput(q, bp)
	queue_t        *q;
	mblk_t         *bp;
{
	int             in_pcboptmgmt(), ip_options();
	static struct opproc funclist[] = {
		SOL_SOCKET, in_pcboptmgmt,
		IPPROTO_IP, ip_options,
		0, 0
	};

	dooptions(q, bp, funclist);
}
