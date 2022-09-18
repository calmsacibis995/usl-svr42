/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_ACC_PRIV_PRIVILEGE_H	/* wrapper symbol for kernel use */
#define	_ACC_PRIV_PRIVILEGE_H	/* subject to change without notice */

#ident	"@(#)uts-comm:acc/priv/privilege.h	1.11.3.8"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef	_UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif	/* _UTIL_TYPES_H */

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */

#endif	/* _KERNEL_HEADERS */

/**********************************************************
 *
 * The following is the typedef for the user-level privilege
 * definition.  It is here because kernel routines also need
 * to know about this particular type.
 *
 **********************************************************/

typedef	unsigned	long	priv_t;

/**********************************************************
 *
 * The following are the known privilege sets.
 *
 *	PS_FIX		for fixed privilege sets
 *	PS_INH		for inheritable privilege sets
 *	PS_MAX		for maximum privilege sets
 *	PS_WKG		for working privilege sets
 *
 **********************************************************/

#define	PS_FIX		0x66000000
#define	PS_INH		0x69000000
#define	PS_MAX		0x6d000000
#define	PS_WKG		0x77000000
#define	PS_TYPE		0xff000000

/**********************************************************
 *
 * The following are the supported object types for
 * privilege mechanisms.
 *
 **********************************************************/

#define	PS_FILE_OTYPE	0x0
#define	PS_PROC_OTYPE	0x1

/**********************************************************
 *
 * The following is the set of all known privileges.
 * The define NPRIVS is the number of privileges
 * currently in use.  It should be modified whenever a
 * privilege is added or deleted. Further description
 * of each privilege can be found in intro(2).
 *
 **********************************************************/

#define	NPRIVS		26

#define	P_OWNER		0x00000000
#define	P_AUDIT		0x00000001
#define	P_COMPAT	0x00000002
#define	P_DACREAD	0x00000003
#define	P_DACWRITE	0x00000004
#define	P_DEV		0x00000005
#define	P_FILESYS	0x00000006
#define	P_MACREAD	0x00000007
#define	P_MACWRITE	0x00000008
#define	P_MOUNT		0x00000009
#define	P_MULTIDIR	0x0000000a
#define	P_SETPLEVEL	0x0000000b
#define	P_SETSPRIV	0x0000000c
#define	P_SETUID	0x0000000d
#define	P_SYSOPS	0x0000000e
#define	P_SETUPRIV	0x0000000f
#define	P_DRIVER	0x00000010
#define	P_RTIME		0x00000011
#define	P_MACUPGRADE	0x00000012
#define	P_FSYSRANGE	0x00000013
#define	P_SETFLEVEL	0x00000014
#define	P_AUDITWR	0x00000015
#define	P_TSHAR		0x00000016
#define	P_PLOCK		0x00000017
#define	P_CORE		0x00000018
#define	P_LOADMOD	0x00000019
#define	P_ALLPRIVS	0x00ffffff


/**********************************************************
 *
 * The following  defines  are recognized by the privilege
 * mechanisms.  They are returned in the argument value of
 * the secsys() system call in the form of flags when  the
 * command is ES_PRVINFO.
 *
 **********************************************************/

#define	PM_UIDBASE	0x00000001
#define	PM_ULVLINIT	0x00000002
#define	PM_PRVMODE	0x00000004

/**********************************************************
 *
 * The following are the CMDS recognized by the procpriv()
 * and filepriv() system calls.
 *
 **********************************************************/

#define	SETPRV		0x0
#define	CLRPRV		0x1
#define	PUTPRV		0x2
#define GETPRV		0x3
#define CNTPRV		0x4

/**********************************************************
 *
 * Structure definition for the privilege sets supported
 * by individual privilege servers.  Also some defines
 * that are used at user-level related to the privilege
 * mechanisms.
 *
 **********************************************************/

#define	PRVNAMSIZ	 32
#define	PRVMAXSETS	256

typedef	struct	pm_setdef {
	priv_t	sd_mask;		/* masked type for this privilege set */
	uint	sd_setcnt;		/* number of privileges in this set   */
	char	sd_name[PRVNAMSIZ];	/* name of this privilege set         */
	ulong_t	sd_objtype;		/* object type of this privilege set  */ 
} setdef_t;

#if defined(_KERNEL) || defined(_KMEMUSER)

/**********************************************************
 *
 * The following macros are used by the different privilege
 * servers to manipulate privilege bits.
 *
 **********************************************************/

/* Turn on significant bits within kernel privilege vector. */
#define	pm_allon		((1 << NPRIVS) - 1)

/* Mask off type field within privilege descriptor and returns the privilege */
#define	pm_pos(p)		(pvec_t)((p) & ~PS_TYPE)

/* Mask off the privilege field and return the privilege type */
#define	pm_type(p)		(pvec_t)((p) & PS_TYPE)

/* Convert privilege type to ASCII character */
#define	pm_pridc(p)		(pvec_t)((p) >> 24)

/* Set the pvec_t bit corresponding to the privilege passed */
#define	pm_privbit(p)		(pvec_t)(1 << (p))

/* Convert an ASCII character to a privilege type */
#define	pm_pridt(p)		(pvec_t)((p) << 24)

/* Validate the privilege descripter passed */
#define	pm_invalid(p)		(((pm_pos((p)) < 0 || pm_pos((p)) > NPRIVS) && pm_pos((p)) != P_ALLPRIVS) ? 1 : 0)

/* Turn on privilege passed within vector passed */
#define	pm_setbits(p, v)	(v |= (((p) == P_ALLPRIVS) ? pm_allon : (1<<pm_pos(p))))

/* 
 * Check the credential(a) passed,
 * to determine if privilege(b) is on within the working privilege set
 */
#define pm_privon(a, b)		((a)->cr_wkgpriv & (b))

/* 
 * Check both credentials(a,b) passed, 
 * to determine if the maximum privilege set of (b) is a subset of (a)
 */
#define	pm_subset(a, b)		(((a)->cr_maxpriv & (b)->cr_maxpriv) == (b)->cr_maxpriv)

/*
 * If the maximum privileges in the credentials passed are non-zero
 * then the process is privileged.
 */
#define	pm_privileged(a)	((a)->cr_maxpriv)

/***********************************************************
 *
 * Structure definitions for the kernel privilege table
 * data types.  Used by any privilege mechanism that stores
 * the information in the kernel.
 *
 ***********************************************************/

/* least privilege file table */
typedef struct	lpftab {
	struct	lpftab	*lpf_next;	/* ptr to next file in list    */
	ino_t	lpf_nodeid;		/* node id                     */
	pvec_t	lpf_fixpriv;		/* fixed privileges            */
	pvec_t	lpf_inhpriv;		/* inheritable privileges      */
	time_t	lpf_validity;		/* validity info for integrity */
} lpftab_t;

/* least privilege file system id table */
typedef struct	lpdtab {
	struct	lpdtab	*lpd_next;	/* ptr to next file system in list    */
	lpftab_t	*lpd_list;	/* ptr to a privileged file on        */
					/* this particular file system        */
	dev_t	lpd_fsid;		/* the id number for this file system */
} lpdtab_t;

/* least privilege device per file system table */
typedef struct	lpktab {
	struct	lpktab	*lpk_next;	/* ptr to next device in list    */
	lpdtab_t	*lpk_list;	/* ptr to a file system on       */
					/* this particular device        */
	dev_t	lpk_dev;		/* the id number for this device */
} lpktab_t;

#endif	/* _KERNEL || _KMEMUSER */

#endif	/* _ACC_PRIV_PRIVILEGE_H */
