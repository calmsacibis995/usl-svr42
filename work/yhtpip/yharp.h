

/*****************************************************
 *
 * SVR 4.2 STREAMS YHTP - Release 1.0 
 *
 * Copyright 1994 NET612 Computer Department of NUDT 
 * All Rights Reserved. 
 *
 ****************************************************/





#ifndef _NET_YHTPIP_YHARP_H	/* wrapper symbol for kernel use */
#define _NET_YHTPIP_YHARP_H	/* subject to change without notice */

/*
 * pcb's shared by yhapp module and yharp driver
 */

#define	N_ARP	10
struct yhapp_pcb {
	queue_t        *yhapp_q;	/* read pointer for our queue */
	struct yharp_pcb *yharp_pcb;/* cross pointer to our yharp module */
	char            yhapp_uname[IFNAMSIZ];	/* enet unit name */
	struct yharpcom   yhapp_ac;	/* common structure for this unit */
};

struct yharp_pcb {
	queue_t        *yharp_qtop;	/* upstream read queue */
	queue_t        *yharp_qbot;	/* downstream write queue */
	int             yharp_index;	/* mux index for link */
	struct yhapp_pcb *yhapp_pcb;/* cross pointer to yhapp module */
	char            yharp_uname[IFNAMSIZ];	/* enet unit name */
	mblk_t         *yharp_saved;	/* saved input request */
	int		yharp_flags;	/* flags */
};

#endif	/* _NET_YHTPIP_YHARP_H */
