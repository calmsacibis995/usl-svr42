


/*****************************************************
 *
 * SVR 4.2 STREAMS YHTP - Release 1.0 
 *
 * Copyright 1994 NET612 Computer Department of NUDT 
 * All Rights Reserved. 
 *
 ****************************************************/




#ifndef _NET_YHTPIP_YHTP_DEBUG_H	/* wrapper symbol for kernel use */
#define _NET_YHTPIP_YHTP_DEBUG_H	/* subject to change without notice */

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif

#ifndef _NET_YHTPIP_YHTP_VAR_H
#include <net/yhtpip/yhtp_var.h>	/* REQUIRED */
#endif

#ifndef _NET_YHTPIP_YHTPIP_H
#include <net/yhtpip/yhtpip.h>	/* REQUIRED */
#endif

#ifndef _NET_YHTPIP_YHIN_SYSTM_H
#include <net/yhtpip/yhin_systm.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */
#include <netinet/yhtp_var.h>	/* REQUIRED */
#include <netinet/yhtpip.h>	/* REQUIRED */
#include <netinet/in_systm.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

struct	yhtp_debug {
	n_time	td_time;
	short	td_act;
	short	td_ostate;
	caddr_t	td_tcb;
	struct	yhtpiphdr td_ti;
	short	td_req;
	struct	yhtpcb td_cb;
};

#define	TA_INPUT 	0
#define	TA_OUTPUT	1
#define	TA_USER		2
#define	TA_RESPOND	3
#define	TA_DROP		4
#define TA_TIMER	5

#ifdef TANAMES
char	*yhtanames[] =
    { "input", "output", "user", "respond", "drop", "timer" };
#endif

#ifdef _KERNEL
extern int		yhtp_ndebug, yhtp_debx;
extern struct yhtp_debug	yhtp_debug[];
#endif /* _KERNEL */

#endif	/* _NET_YHTPIP_YHTP_DEBUG_H */
