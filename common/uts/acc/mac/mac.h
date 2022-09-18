/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _ACC_MAC_MAC_H	/* wrapper symbol for kernel use */
#define _ACC_MAC_MAC_H	/* subject to change without notice */

#ident	"@(#)uts-comm:acc/mac/mac.h	1.15.3.4"
#ident	"$Header: $"

#ifdef	_KERNEL_HEADERS

#ifndef	_UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#ifndef	_SVC_TIME_H
#include <svc/time.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */
#include <sys/time.h>	/* REQUIRED */

#endif	/* _KERNEL_HEADERS */

#define	MAC_GET 	1	/* get a level ((f)lvlfile & lvlproc) */
#define	MAC_SET 	2	/* set a level ((f)lvlfile & lvlproc) */

/*
 * The following two definitions are for specifying the argument
 * to the mldmode(2) system call.
 */

#define MLD_REAL	0x1	/* put process in real MLD mode */
#define MLD_VIRT	0x0	/* put process in virtual MLD mode */
#define MLD_QUERY	0x2	/* query MLD mode */

#define	MACEQUAL	1	/* MAC equality request */
#define	MACDOM		2	/* MAC domination request */

/*
 * The following are the maximum number of classifications, categories,
 * and LIDs that the current implementation of LIDs can support.
 * Note that MAXCATS can be modified to accommodate for more categories.
 * MAXLIDS can also be increased if needed to a max value of
 * (0xffffffff/sizeof(struct mac_level)). 64k should be more than enough.
 * MAXCLASSES can also be increased to max of 0xffff.
 * 
 */

#define	MAXLIDS		65536
#define	MAXCLASSES	256
#define	MAXCATS		1024	/* max # of categories allowed */

/*
 * The following are used for manipulation of the internal level structure.
 * Note that CAT_SHIFT is hard coded so that 2^CAT_SHIFT = CAT_SIZE.
 * If MAXCATS is modified, it should be a power of 2, and CAT_SHIFT must
 * be modified accordingly.
 */

#define CAT_SIZE	(((MAXCATS-1)/(sizeof(ulong)*NBBY))+1)
#define	CAT_SHIFT	5

/*
 * The following is the internal format of a full level to which a lid maps.
 * It is also the format of a "record" in the lid.internal file.
 */

struct mac_level {
	ulong_t		lvl_cat[CAT_SIZE];	/* category bits */
	ushort_t	lvl_class;		/* classification */
	uchar_t		lvl_valid; 		/* validity flag */
	ushort_t	lvl_catsig[CAT_SIZE+1]; /* signif of corresp. lvl_cat entry */
};

/*
 * Following are the definitions for the LID states.
 */

#define	LVL_INVALID	((uchar_t)'\0')
#define	LVL_ACTIVE	((uchar_t)'A')
#define	LVL_INACTIVE	((uchar_t)'D')


/*
 * Applications that read /dev/mem must be built like the kernel.
 * A new symbol "_KMEMUSER" is defined for this purpose.
 */
#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * The following is the definition of the format of an entry in the LID cache.
 */

struct mac_cachent {
	lid_t		 ca_lid;	/* LID */
	timestruc_t	 ca_lastref;	/* last reference time (for LRU */
					/* replacement algorithm)       */
	ulong_t		 ca_count;	/* reference count 		*/
	struct mac_level ca_level;	/* level corresponding to LID   */
};

#define	MAC_WANT	0x1		/* bit for cacheflag, process is */
					/* sleeping on cacheflag address */

/*
 * The following definition is for the size of the array
 * to be allocated for the effective directory name.
 * The name is the character representation of the hexadecimal
 * value of the LID for the effective directory.
 * It takes at most 8 characters to represent a LID in hex.
 * An additional character is added for the terminating '\0'.
 */

#define MLD_SZ	9

#endif


/*
 * Security Attributes data structure for block and character special file 
 */

struct devstat {
	ushort_t dev_relflag;	/* device release flags */
	ushort_t dev_mode;	/* DEV_STATIC or DEV_DYNAMIC */
	level_t	dev_hilevel;	/* maximum level of a device */
	level_t	dev_lolevel;	/* minimum level of a device */
	ushort_t dev_state;	/* device state: public or private */
	ushort_t dev_usecount;	/* flag set to 1 if device "in use", 0 otherwise */
};

/* device security attributes flags */

/* release flags on a block or character special file */
#define DEV_PERSISTENT	1	/* dev sec attributes set via a devstat call */
#define DEV_LASTCLOSE	2	/* dev sec attributes set until last close on dev */
#define DEV_SYSTEM	3	/* dev sec attributes set to  "system setting" */

/* mode of a device */
#define	DEV_STATIC	1	/* restricts level change when dev is "tranquil" */
				/* (not open and not mapped) or private */
#define	DEV_DYNAMIC	2	/* level change ok even if dev is open or mapped */


/* valid states of a device */
#define DEV_PRIVATE	1	/* only privileged access allowed */
#define DEV_PUBLIC	2	/* unprivileged and privileged access allowed */


/* valid cmd for the devstat system call */
#define	DEV_GET		1	/* get dev security attributes */
#define	DEV_SET		2	/* set dev security attributes */

#ifdef	_KERNEL
extern int	mac_installed;		/* flag to tell if MAC is installed */

/*
 * Generic MAC will advertise the use of the following two macros for
 * access checking only.
 * It is up to the individual features to build specific MAC functions
 * for their needs based on these two macros.
 * Note that these macros don't guarantee the validation of a LID.
 * This responsibility is left to the caller.
 */

/*
 * This macro performs the MACEQUAL and MACDOM operations
 * given two LIDs.  No privilege checks are performed.  It only
 * calls internal routine mac_liddom() if it definitely is about
 * to perform a domination check.
 * A sanity check on the operation argument is not of particular
 * interest since this routine is only called from within the kernel.
 * Note that if MAC is not installed checks are bypassed.
 */
#define	MAC_ACCESS(op, lid1, lid2) \
( \
	mac_installed == 0 \
		? 0 \
		: lid1 == lid2 \
			? 0 \
			: op == MACDOM \
				? mac_liddom(lid1, lid2) \
				: EACCES \
)

/*
 * This macro performs a MAC access check on the given vnode based on
 * the mode of access.
 * Internal routine mac_vaccess() is only called if a privilege
 * check is to be made or if an actual domination check is to be made.
 * Note that in mac_vaccess() the privilege check on VWRITE could
 * have been performed before doing a domination check.  However,
 * by always performing the domination check first, the mac_vaccess()
 * routine can be called without calling MAC_VACCESS() and the
 * functionality of mac_vaccess() would still be intact.  If it turns
 * out that we need to speed up performance for the privileged user
 * we can change mac_vaccess() accordingly.
 */
#define	MAC_VACCESS(vp, mode, credp) \
( \
	MAC_ACCESS(MACEQUAL, (credp)->cr_lid, (vp)->v_lid) \
		? mac_vaccess(vp, mode, credp) \
		: 0 \
)

#if defined (__STDC__)

/* incomplete structure definitions to avoid including header files */
struct cred;
struct vnode;
struct mac_cachent;

extern int	mac_liddom(lid_t, lid_t);
extern int	mac_vaccess(struct vnode *, int, struct cred *);
extern int	mac_getlevel(lid_t, struct mac_cachent **);
extern int	mac_valid(lid_t);

#else

extern int	mac_liddom();
extern int	mac_vaccess();
extern int	mac_getlevel();
extern int	mac_valid();

#endif	/* __STDC__ */

#endif	/* _KERNEL */

#endif	/* _ACC_MAC_MAC_H */
