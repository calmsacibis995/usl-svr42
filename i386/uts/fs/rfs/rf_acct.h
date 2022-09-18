/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_RFS_RF_ACCT_H	/* wrapper symbol for kernel use */
#define _FS_RFS_RF_ACCT_H	/* subject to change without notice */

#ident	"@(#)uts-x86:fs/rfs/rf_acct.h	1.5"
#ident	"$Header: $"


#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif

#ifndef _UTIL_SYSINFO_H
#include <util/sysinfo.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */
#include <sys/sysinfo.h>	/* REQUIRED */

#else

#include <sys/types.h>		/* SVR4.0COMPAT */
#include <sys/sysinfo.h>	/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

/*
 * Accounting structure for RFS client caching.
 */
typedef struct rfc_info {
	ulong rfci_pmread;	/* read cache miss pages */
	ulong rfci_pmwrite;	/* write cache miss pages */
	ulong rfci_ptread;	/* read pages sought in cache */
	ulong rfci_ptwrite;	/* write pages sought in cache */
	ulong rfci_pabort;	/* pages aborted from cache */
	ulong rfci_snd_dis;	/* cache disable messages sent */
	ulong rfci_rcv_dis;	/* cache disable messages received */
	ulong rfci_snd_msg;	/* total messages sent */
	ulong rfci_rcv_msg;	/* total messages received */
	ulong rfci_vc_hit;	/* vattr cache hits */
	ulong rfci_vc_miss;	/* vattr cache misses */
	ulong rfci_ac_hit;	/* access cache hits */
	ulong rfci_ac_miss;	/* access cache misses */
	ulong rfci_dis_data;	/* on server, messages incurred for files
				 * with caching temporarily disabled
				 * in resources mounted with caching */
} rfc_info_t;

typedef struct rf_srv_info {
			/* ELEMENT FOR sar -Du */
	time_t	rfsi_serve;	/* ticks in rfs server since boot */
			/* ELEMENTS FOR sar -S */
	ulong	rfsi_nservers;	/* sum of all servers since boot */
	ulong	rfsi_srv_que;	/* sum of server queue length since boot */
	ulong	rfsi_srv_occ;	/* seconds server queue found occupied */
	ulong	rfsi_rcv_que;	/* sum of server work list length since boot */
	ulong	rfsi_rcv_occ;	/* seconds server work list found occupied */
} rf_srv_info_t;

#if defined(_KERNEL)

extern rfc_info_t	rfc_info;
extern fsinfo_t		rfcl_fsinfo;
extern fsinfo_t		rfsr_fsinfo;
extern rf_srv_info_t	rf_srv_info;

extern time_t		*rfsi_servep;	/* SVID compliance hack for sar(1) */

extern int		minserve;	/* tunable: server low water mark */
extern int		maxserve;	/* tunable: server high water mark */

extern void rf_clock();

#endif

#endif /* _FS_RFS_RF_ACCT_H */
