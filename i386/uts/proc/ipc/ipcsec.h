/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_IPC_IPCSEC_H	/* wrapper symbol for kernel use */
#define	_PROC_IPC_IPCSEC_H	/* subject to change without notice */

#ident	"@(#)uts-x86:proc/ipc/ipcsec.h	1.4"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>
#endif

#ifndef _ACC_DAC_ACL_H
#include <acc/dac/acl.h>
#endif

#elif	defined(_KERNEL)

#include <sys/types.h>
#include <sys/acl.h>

#endif /* _KERNEL_HEADERS */

/* Common IPC DAC structure */
struct ipc_dac {
	int 		aclcnt;		/* number of ACL entries */
	struct 	acl	acls[1];	/* ACL entries in memory */
};

/* Common IPC Access Control Structure */
struct ipc_sec {
	struct ipc_dac	*dacp;		/* DAC ptr */
	lid_t		ipc_lid;	/* MAC level identifier */
	lid_t		ipc_cmwlid;	/* MAC level identifier  CMW */
};

/*
 * For portability, the size of the DAC structure is computed by the
 * following macro.  This is to avoid potential problems caused by padding.
 */
#define	DACSIZE(aclcnt) \
		(sizeof(struct ipc_dac) + (((aclcnt) - 1) * sizeof(struct acl)))

/*
 * Common IPC routine to free DAC structure.
 * Note that ipc_secp must be an unevaluated security entry.
 * Further note that the if-else statement allows this macro to be
 * called from anywhere without generating ambiguous if-else code
 * or syntax errors.
 */
#define	FRIPCACL(ipc_secp) \
	if ((ipc_secp)->dacp == (struct ipc_dac *)NULL) \
		; \
	else { \
		kmem_free((void *)((ipc_secp)->dacp), \
			  (size_t)DACSIZE((ipc_secp)->dacp->aclcnt)); \
		(ipc_secp)->dacp = (struct ipc_dac *)NULL; \
	}

#endif	/* _PROC_IPC_IPCSEC_H */
