/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_KMEM_H	/* wrapper symbol for kernel use */
#define _MEM_KMEM_H	/* subject to change without notice */

#ident	"@(#)uts-comm:mem/kmem.h	1.1.2.4"
#ident	"$Header: $"

/*
 * Public definitions for kernel memory allocator.
 */

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Flag argument to routines in the kmem_alloc() family.  The value
 * of KM_NOSLEEP *MUST* agree with NOSLEEP as defined in immu.h!
 */
#define KM_SLEEP	0	/* can sleep to get memory */
#define KM_NOSLEEP	1	/* cannot sleep to get memory */

/*
 * By default, the seg_kmem segment driver will return DMA-able pages.
 * The KM_NO_DMA flag may be OR-ed with KM_SLEEP or KM_NOSLEEP to
 * specify that DMA-able pages are not required.  This allows us to
 * avoid penalizing the kernel memory allocator and other code that
 * relies on the seg_kmem driver.  KM_NO_DMA is ignored if not RESTRICTED_DMA.
 */
#define KM_NO_DMA	2	/* DMA-able pages not required */

#if defined(__STDC__)

extern void   kmem_init(void);
extern _VOID *kmem_alloc(size_t, int);
extern _VOID *kmem_zalloc(size_t, int);
extern _VOID *kmem_fast_alloc(caddr_t *, size_t, int, int);
extern _VOID *kmem_fast_zalloc(caddr_t *, size_t, int, int);
extern void   kmem_free(_VOID *, size_t);
extern void   kmem_fast_free(caddr_t *, caddr_t);

#else

extern void   kmem_init();
extern _VOID *kmem_alloc();
extern _VOID *kmem_zalloc();
extern _VOID *kmem_fast_alloc();
extern _VOID *kmem_fast_zalloc();
extern void   kmem_free();
extern void   kmem_fast_free();

#endif	/* __STDC__ */

#endif	/* _MEM_KMEM_H */
