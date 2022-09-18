/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_RDMA_H	/* wrapper symbol for kernel use */
#define _MEM_RDMA_H	/* subject to change without notice */

#ident	"@(#)uts-comm:mem/rdma.h	1.2"
#ident	"$Header: $"

/*
 * Support for the RESTRICTED_DMA property, for systems which use DMA
 * but cannot access all of physical memory via DMA.
 */

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

extern boolean_t rdma_enabled;	/* Non-zero => restricted DMA support enabled:
				 * the tune.t_dmalimit tunable is set and
				 * there is memory which exceeds DMA capacity.
				 */

#define	DMA_PP(pp)	((pp)->p_dma)
#define	DMA_PFN(pfn)	((pfn) < tune.t_dmalimit && (pfn) >= tune.t_dmabase)
#define	DMA_BYTE(b)	DMA_PFN(btop(b))

#ifdef __STDC__
struct buf;
extern void rdma_fix_swtbls(void);
extern void rdma_fix_bswtbl(int);
extern int rdma_physio(int (*)(), struct buf *, int);
#else
extern void rdma_fix_swtbls();
extern void rdma_fix_bswtbl();
extern int rdma_physio();
#endif

#endif	/* _KERNEL */

#endif	/* _MEM_RDMA_H */
