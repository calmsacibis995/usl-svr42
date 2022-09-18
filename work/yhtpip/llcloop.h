


/*****************************************************
 *
 * SVR 4.2 STREAMS YHTP - Release 1.0 
 *
 * Copyright 1994 NET612 Computer Department of NUDT 
 * All Rights Reserved. 
 *
 ****************************************************/




#ifndef _NET_YHTPIP_LLCLOOP_H	/* wrapper symbol for kernel use */
#define _NET_YHTPIP_LLCLOOP_H	/* subject to change without notice */


#ifdef _KERNEL_HEADERS

#ifndef _IO_STREAM_H
#include <io/stream.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/stream.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

struct loop_pcb {
	queue_t        *loop_qtop;
	int             loop_state;	/* Bound? */
};

#endif	/* _NET_YHTPIP_LLCLOOP_H */
