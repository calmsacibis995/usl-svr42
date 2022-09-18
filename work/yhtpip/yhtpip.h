

/*****************************************************
 *
 * SVR 4.2 STREAMS YHTP - Release 1.0 
 *
 * Copyright 1994 NET612 Computer Department of NUDT 
 * All Rights Reserved. 
 *
 ****************************************************/


#ifndef _NET_YHTPIP_YHTPIP_H	/* wrapper symbol for kernel use */
#define _NET_YHTPIP_YHTPIP_H	/* subject to change without notice */

/*
 */

#ifdef _KERNEL_HEADERS

#ifndef _NET_YHTPIP_YHTP_H
#include <net/yhtpip/yhtp.h>	/* REQUIRED */
#endif

#ifndef _NET_YHTPIP_YHIP_VAR_H
#include <net/yhtpip/yhip_var.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <netinet/yhtp.h>	/* REQUIRED */
#include <netinet/ip_var.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Tcp+ip header, after ip options removed.
 */
struct yhtpiphdr {
	struct 	ipovly ti_i;		/* overlaid ip structure */
	struct	yhtphdr ti_t;		/* yhtp header */
};
#define	ti_next		ti_i.ih_next
#ifdef STRNET
#define ti_mblk		ti_i.ih_mblk
#else
#define	ti_prev		ti_i.ih_prev
#endif STRNET
#define	ti_x1		ti_i.ih_x1
#define	ti_pr		ti_i.ih_pr
#define	ti_len		ti_i.ih_len
#define	ti_src		ti_i.ih_src
#define	ti_dst		ti_i.ih_dst
#define	ti_sport	ti_t.th_sport
#define	ti_dport	ti_t.th_dport
#define	ti_seq		ti_t.th_seq
#define	ti_ack		ti_t.th_ack
#define	ti_x2		ti_t.th_x2
#define	ti_off		ti_t.th_off
#define	ti_flags	ti_t.th_flags
#define	ti_win		ti_t.th_win
#define	ti_sum		ti_t.th_sum
#define	ti_urp		ti_t.th_urp
#endif	/* _NET_YHTPIP_YHTPIP_H */
