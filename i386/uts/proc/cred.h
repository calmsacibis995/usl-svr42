/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ifndef _PROC_CRED_H	/* wrapper symbol for kernel use */
#define _PROC_CRED_H	/* subject to change without notice */

#ident	"@(#)uts-x86:proc/cred.h	1.5"
#ident	"$Header: $"
/*
 * User credentials.  The size of the cr_groups[] array is configurable
 * but is the same (ngroups_max) for all cred structures; cr_ngroups
 * records the number of elements currently in use, not the array size.
 */

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

typedef struct cred {
	ushort	cr_ref;			/* reference count */
	ushort	cr_ngroups;		/* number of groups in cr_groups */
	uid_t	cr_uid;			/* effective user id */
	gid_t	cr_gid;			/* effective group id */
	uid_t	cr_ruid;		/* real user id */
	gid_t	cr_rgid;		/* real group id */
	uid_t	cr_suid;		/* "saved" user id (from exec) */
	gid_t	cr_sgid;		/* "saved" group id (from exec) */
        pvec_t  cr_savpriv;             /* saved privilege vector */
        pvec_t  cr_wkgpriv;             /* working privilege vector */
        pvec_t  cr_maxpriv;             /* maximum privilege vector */
	lid_t	cr_lid;			/* Level IDentifier (MAC) */
	lid_t	cr_cmwlid;		/* Level IDentifier (MAC) CMW */
	ulong	cr_flags;		/* Add'l cred flags (see below) */
	gid_t	cr_groups[1];		/* supplementary group list */
} cred_t;

/*
   Definitions for flags in cr_flags
*/

#define CR_MLDREAL 0x00000001 /* indicates proc is in "real" MLD mode */

#ifdef _KERNEL

/*
 * the following macro simply increments the
 * reference count of the named credential structure.
 */

#define	crhold(cr)	(cr)->cr_ref++

/*
 * the maximum number of supplemental groups
 */

extern int ngroups_max;	

/*
 * pointer to credential structure used by system processes
 */

extern	struct	cred	*sys_cred;

#if defined(__STDC__)

extern void cred_init(void);
extern void crfree(cred_t *);
extern cred_t *crget(void);
extern cred_t *crcopy(cred_t *);
extern cred_t *crdup(cred_t *);
extern cred_t *crgetcred(void);
extern size_t crgetsize(void);
extern int pm_denied(cred_t *, int);
extern int groupmember(gid_t, cred_t *);
extern int hasprocperm(cred_t *, cred_t *);

#else

extern void cred_init();
extern void crfree();
extern cred_t *crget();
extern cred_t *crcopy();
extern cred_t *crdup();
extern cred_t *crgetcred();
extern size_t crgetsize();
extern int pm_denied();
extern int groupmember();
extern int hasprocperm();


#endif	/* __STDC */


#endif

#endif	/* _PROC_CRED_H */
