/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _ACC_MAC_COVERT_H	/* wrapper symbol for kernel use */
#define _ACC_MAC_COVERT_H	/* subject to change without notice */

#ident	"@(#)uts-x86:acc/mac/covert.h	1.5"
#ident	"$Header: $"

#ifdef	_KERNEL

/*
 * This header file is for Covert Channel treatment.
 */

#define CC_DEBUG	0

/*
 * Covert channel event structure.
 * An instance of this structure is defined wherever there exists
 * a covert channel to be treated by the limiter.  The address of
 * that structure is passed to the limiter as an argument.
 * The caller is responsible for setting cc_type to indicate
 * the type of the event (chosen from the list defined below)
 * and cc_bits to indicate the maximum number of bits that
 * could potentially be transmitted by this particular event.
 * Other fields are for use by the limiter.
 */
typedef struct cc_event {
	long	cc_type;	/* event type, see list below */
	short	cc_bits;	/* max possible bits transmitted */
	short	cc_flags;	/* flags for use in limiter */
	long	cc_start;	/* start time in ticks */
	long	cc_count;	/* event counter (bits in cycle) */
	char	cc_filler[16];	/* for future compatibility */
#ifdef CC_DEBUG
	long	cc_naudit;	/* number of audit records cut */
	long	cc_ndelay;	/* number of delays */
	long	cc_delayticks;	/* number of ticks delayed */
	long	cc_maxbps;	/* max bps within a cycle */
	long	cc_recstart;	/* start time of recording */
	long	cc_recstop;	/* stop time of recording */
	long	cc_nunpriv;	/* number of bits by unprivileged procs */
	long	cc_npriv;	/* number of bits by privileged procs */
#endif
} ccevent_t;

/*
 * Following are the covert channel events requiring auditing.
 */
#define CC_ALLOC_INODE		1	/* Allocation of (SFS) inodes */
#define CC_ALLOC_IPC		2	/* Allocation of IPC */
#define CC_CACHE_MACLVL		3	/* Cache MAC LIDs through lvldom */
#define CC_CACHE_MACSEC		4	/* Cache MAC LIDs through secadvise */
#define CC_CACHE_PAGE		5	/* Cache pages */
#define CC_RE_DB		6 	/* Resource Exhaustion data blocks */
#define CC_RE_FLOCK		7	/* Resource Exhaustion file locking */
#define CC_RE_INODE		8	/* Resource Exhaustion inodes */
#define CC_RE_LOG		9	/* Resource Exhaustion log driver */
#define CC_RE_NAMEFS		10	/* Resource Exhaustion namefs */
#define CC_RE_PIPE		11	/* Resource Exhaustion pipes */
#define CC_RE_PROC		12	/* Resource Exhaustion processes */
#define CC_RE_SAD		13	/* Resource Exhaustion sad driver */
#define CC_RE_SCSI		14	/* Resource Exhaustion SCSI */
#define CC_SPEC_DIROFF		15	/* i_diroff incore inode field */
#define CC_SPEC_DIRRM		16	/* non-empty directory removal */
#define CC_SPEC_SYNC		17	/* sync call */
#define CC_SPEC_UNLINK		18	/* unlink open file with 0 link count */
#define CC_SPEC_WAKEUP		19	/* wakeup from locks */
#define CC_RE_MSG   		20	/* Resource Exhaustion msgfp, msgmap */
#define CC_RE_SEM   		21	/* Resource Exhaustion semfup, semmap */
#define CC_RE_TIMERS		22	/* Resource Exhaustion hrtimers */
#define CC_SPEC_TIMERS		23	/* multiple BSD timer commands */
#define CC_CACHE_DNLC		24	/* Cache DNLC */

/*
 * The default number of bits per event for each covert channel event.
 * If the value is calculated dynamically, we define it as 0 here.
 */
#define CCBITS_ALLOC_INODE	0	/* Allocation of (SFS) inodes */
#define CCBITS_ALLOC_IPC	1	/* Allocation of IPC */
#define CCBITS_CACHE_MACLVL	1	/* Cache MAC LIDs through lvldom */
#define CCBITS_CACHE_MACSEC	1	/* Cache MAC LIDs through secadvise */
#define CCBITS_CACHE_PAGE	1	/* Cache pages */
#define CCBITS_RE_DB		1	/* Resource Exhaustion data blocks */
#define CCBITS_RE_FLOCK		1	/* Resource Exhaustion file locking */
#define CCBITS_RE_INODE		1	/* Resource Exhaustion inodes */
#define CCBITS_RE_LOG		1	/* Resource Exhaustion log driver */
#define CCBITS_RE_NAMEFS	1	/* Resource Exhaustion namefs */
#define CCBITS_RE_PIPE		1	/* Resource Exhaustion pipes */
#define CCBITS_RE_PROC		1	/* Resource Exhaustion processes */
#define CCBITS_RE_SAD		1	/* Resource Exhaustion sad driver */
#define CCBITS_RE_SCSI		1	/* Resource Exhaustion SCSI */
#define CCBITS_SPEC_DIROFF	1	/* i_diroff incore inode field */
#define CCBITS_SPEC_DIRRM	1	/* non-empty directory removal */
#define CCBITS_SPEC_SYNC	1	/* sync call */
#define CCBITS_SPEC_UNLINK	1	/* unlink open file with 0 link count */
#define CCBITS_SPEC_WAKEUP	1	/* wakeup from locks */
#define CCBITS_RE_MSG   	1	/* Resource Exhaustion msgfp, msgmap */
#define CCBITS_RE_SEM   	1	/* Resource Exhaustion semfup, semmap */
#define CCBITS_RE_TIMERS	1	/* Resource Exhaustion hrtimers */
#define CCBITS_SPEC_TIMERS	1	/* multiple BSD timer commands */
#define CCBITS_CACHE_DNLC	1	/* Cache DNLC */

/*
 * Following is the definition for the minimum number of free ids
 * to sample from, in the case that randomization is used to
 * treat a channel.
 */
#define RANDMINFREE	1024

/*
 * Following are the tunable parameters.
 */
struct cc_tune {
	struct {
		long	ct_delay;	/* delay threshold in ticks */
		long	ct_audit;	/* audit threshold in ticks */
		long	ct_cycle;	/* cycling period in ticks */
	} cc_thresh;		/* threshold tunable structure */
	long	cc_psearchmin;	/* minimal process search */
};

#define	cc_threshold	cc_tune.cc_thresh

/*
 * Following are the definitions of the information to be retrieved
 * from cc_getinfo().
 */
#define	CC_PSEARCHMIN	1

#if defined (__STDC__)

/* incomplete structure definitions to avoid including header files */
struct cred;

extern int cc_getinfo(int);				/* CC get info */
extern void cc_limiter(ccevent_t *, struct cred *);	/* CC generic limiter */

#else

extern int cc_getinfo();				/* CC get info */
extern void cc_limiter();				/* CC generic limiter */

#endif	/* __STDC__ */

#else	/* !_KERNEL */

/*
 * Used in audit report command to map covert channel
 * event numbers to printable strings.
 */

struct cc_names {
	int	cc_number;
	char	*cc_name;
};

/*
 * Used in audit report command to initialize
 * an array of cc_names.  This must be kept
 * in sync with the #defines above.
 */

#define CC_NAMES			\
	0,	"unused",		\
	1,	"CC_ALLOC_INODE",	\
	2,	"CC_ALLOC_IPC",		\
	3,	"CC_CACHE_MACLVL",	\
	4,	"CC_CACHE_MACSEC",	\
	5,	"CC_CACHE_PAGE",	\
	6,	"CC_RE_DB",		\
	7,	"CC_RE_FLOCK",		\
	8,	"CC_RE_INODE",		\
	9,	"CC_RE_LOG",		\
	10,	"CC_RE_NAMEFS",		\
	11,	"CC_RE_PIPE",		\
	12,	"CC_RE_PROC",		\
	13,	"CC_RE_SAD",		\
	14,	"CC_RE_SCSI",		\
	15,	"CC_SPEC_DIROFF",	\
	16,	"CC_SPEC_DIRRM",	\
	17,	"CC_SPEC_SYNC",		\
	18,	"CC_SPEC_UNLINK",	\
	19,	"CC_SPEC_WAKEUP",	\
	20,	"CC_RE_MSG",		\
	21,	"CC_RE_SEM",		\
	22,	"CC_RE_TIMERS",		\
	23,	"CC_SPEC_TIMERS",	\
	24,	"CC_CACHE_DNLC",	\
	-1,	""

#endif	/* _KERNEL */

#endif	/* _ACC_MAC_COVERT_H */
