/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_TCPIP_TCP_DEBUG_H	/* wrapper symbol for kernel use */
#define _NET_TCPIP_TCP_DEBUG_H	/* subject to change without notice */

#ident	"@(#)uts-comm:net/tcpip/tcp_debug.h	1.2.3.2"
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

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif

#ifndef _NET_TCPIP_TCP_VAR_H
#include <net/tcpip/tcp_var.h>	/* REQUIRED */
#endif

#ifndef _NET_TCPIP_TCPIP_H
#include <net/tcpip/tcpip.h>	/* REQUIRED */
#endif

#ifndef _NET_TCPIP_IN_SYSTM_H
#include <net/tcpip/in_systm.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */
#include <netinet/tcp_var.h>	/* REQUIRED */
#include <netinet/tcpip.h>	/* REQUIRED */
#include <netinet/in_systm.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

struct	tcp_debug {
	n_time	td_time;
	short	td_act;
	short	td_ostate;
	caddr_t	td_tcb;
	struct	tcpiphdr td_ti;
	short	td_req;
	struct	tcpcb td_cb;
};

#define	TA_INPUT 	0
#define	TA_OUTPUT	1
#define	TA_USER		2
#define	TA_RESPOND	3
#define	TA_DROP		4
#define TA_TIMER	5

#ifdef TANAMES
char	*tanames[] =
    { "input", "output", "user", "respond", "drop", "timer" };
#endif

#ifdef _KERNEL
extern int		tcp_ndebug, tcp_debx;
extern struct tcp_debug	tcp_debug[];
#endif /* _KERNEL */

#endif	/* _NET_TCPIP_TCP_DEBUG_H */
