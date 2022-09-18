

/*****************************************************
 *
 * SVR 4.2 STREAMS YHTP - Release 1.0 
 *
 * Copyright 1994 NET612 Computer Department of NUDT 
 * All Rights Reserved. 
 *
 ****************************************************/



#ifndef _NET_YHTPIP_INSREM_H	/* wrapper symbol for kernel use */
#define _NET_YHTPIP_INSREM_H	/* subject to change without notice */



#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/* simulate vax insque and remque instructions. */

typedef struct vq {
	caddr_t         fwd, back;
} vq_t;

#define dequenxt(e)     (e)->fwd = ((vq_t *)(e)->fwd)->fwd

#define enque(e, p)     (e)->fwd = (p)->fwd; (p)->fwd = (caddr_t)(e)

#define	insque(e, p)	(e)->back = (caddr_t)(p); \
			(e)->fwd = (caddr_t)((vq_t *)(p)->fwd); \
			((vq_t *)(p)->fwd)->back = (caddr_t)(e); \
			(p)->fwd = (caddr_t)(e);

#define	remque(e)	((vq_t *)(e)->back)->fwd =  \
					(caddr_t)(e)->fwd; \
			((vq_t *)(e)->fwd)->back = \
					(caddr_t)(e)->back; \
			(e)->fwd = (caddr_t) 0; \
			(e)->back = (caddr_t)0;

#endif	/* _NET_YHTPIP_INSREM_H */
