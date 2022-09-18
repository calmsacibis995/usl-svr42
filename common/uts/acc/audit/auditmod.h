/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _ACC_AUDIT_AUDITMOD_H	/* wrapper symbol for kernel use */
#define	_ACC_AUDIT_AUDITMOD_H	/* subject to change without notice */

#ident	"@(#)uts-comm:acc/audit/auditmod.h	1.40.3.5"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif

#ifndef _ACC_AUDIT_AUDIT_H
#include <acc/audit/audit.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */
#include <sys/audit.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

/* auditable events control structure */
typedef struct aevt_ctl {	
	uint	     a_flags;    /* event mask flags */
	uint	     a_nlvls;    /* number of object levels */
} aevtctl_t;


/* Tunables /etc/master.d/audit */
extern	uint	adt_bsize;	/* size of audit buffer                 */
extern	uint	adt_nlvls;	/* number of object level table entries */
extern	ulong	adt_lvltbl[];	/* individual object level table        */


/* Frequently used audit structures */
extern	abufctl_t	adt_bufctl;	/* audit buffer control structure */
extern	alogctl_t	adt_logctl;	/* audit log control structure	  */
extern	actlctl_t	adt_ctl;	/* audit control structure	  */
extern	aevtctl_t	adt_evtctl;	/* audit events control structure */
extern	adtemask_t	adt_lvlemask;	/* object level event mask	  */

/*
 * Structure of the system call/audit event check functions table .
 * The index into the table maps to the system call entry point.
 */
struct adtent {
        int     (*a_chkfp)();	/* system call specific check  function */
        int    a_evtnum;	/* event number */
};

/*
 * Structure of the system call/audit event recording functions table.
 * The index into the table maps to the system call entry point.
 */
struct adtrec {
        void    (*a_recfp)();	/* system call specific recording function */
};


/* Argument pairs for writing audit records- a data pointer and a size	*/
typedef struct adt_argpairs{
        char	*datap;
	uint	size;
}adt_argp_t;

/* Arguments passed in a variable length list that is NULL terminated */
typedef struct adt_argl{
	adt_argp_t	p;
}adt_argl_t;


/* structure for the adt_gmtime() */
struct	adtime {
	int	a_sec;
	int	a_min;
	int	a_hour;
	int	a_mday;
	int	a_mon;
	int	a_year;
	int	a_wday;
	int	a_yday;
	int	a_isdst;
};

/*
 * This macro is used by the audit subsystem 
 * in order to search through the entire process list
 * and perform some action for either ALL or some specific
 * active process[es].
 */
#define ADT_DOTOPROCS(pp) \
	for ((pp)=practive; (pp)!=NULL; (pp)=(pp)->p_next) \
		if ((pp)->p_stat == SZOMB \
			|| (pp)->p_stat == SIDL) \
			continue; \
		else 
			/* invoking function specific code should follow here */

/*	
 * limits and macros for calculating log related values
 */
#define ALOGLIMIT	0x7fffffff	/* max ulimit for vn_rdwr */
#define SEC_PER_DAY	(24*60*60)
#define year_size(A)	(((A) % 4) ? 365 : 366)
#define NODELEN		8

/* Fixed System Event Mask Bit Positions */
#define FIXAMASK0	0x07ff0000
#define FIXAMASK1	0x1a010018

/*	
 * 	pathname macros and limits
 */
#define adt_pn_hold(cwd)        (cwd)->a_ref++

/*	set the initial sequence number for event sequencing	*/
#define ADT_SETSEQNUM	adt_ctl.a_seqnum = 0

/*	increment the sequence number for event sequencing	*/
#define ADT_SEQNUM	adt_ctl.a_seqnum++

/*	increment the sequence number for event sequencing and set recnum to 1 */
#define ADT_SEQRECNUM	(ADT_SEQNUM | (1 << 24))

/*	compute the sequence number and record number	*/
#define	CMN_SEQNM(p)	((p)->a_seqnum += (1 << 24))

/* set the flag to indicate that the base process data changed*/
#define	ADT_CHGBAS(p) ((p)->p_aprocp?((p)->p_aprocp->a_changed|=ADTCHGBAS):0)

/* set the flag to indicate that the multiple groups list changed*/
#define	ADT_CHGGRP(p) ((p)->p_aprocp?((p)->p_aprocp->a_changed|=ADTCHGGRP):0)

/* Errors called through adt_error()	*/
#define	BADLOG			1
#define LOGONE			2
#define	WRITE_FAILED		3
#define NO_MEM			4
#define LOGINIT_ERR		5

/* Error messages as printed 	*/
#define	BADLOG_MSG		"Bad alternate log vnode, error = "
#define	LOGONE_MSG		"Audit log vnode gone"
#define	WRITE_FAILED_MSG	"Audit log file could not be written"
#define NO_MEM_MSG		"Cannot kmem_alloc memory, size = "
#define LOGINIT_ERR_MSG		"Unable to create new audit log file, errno = "
#define	UXERR_MSG		"\nUX:audit: ERROR:"
#define	UXINFO_MSG		"\nUX:audit: INFO:"


/* Onfull Messages */
#define	LOGFULL	"\nUX:audit: INFO: %d/%d/%d %d:%d:%d current event log %s full\n"
#define	DISABLE	"\nUX:audit: INFO: Auditing disabled\n"
#define	SHTDOWN	"\nUX:audit: INFO: System shutdown\n"
#define	SWITCH	"\nUX:audit: INFO: switched to log %s\n"
#define	PROGRAM	"\nUX:audit: INFO: %s executed\n"
#define	BUFFER_DATA	"\nUX:audit: WARNING: Valid data remains in audit buffer\n"

/* Global Function Prototypes Within AUDIT Module */

#if defined (__STDC__)
extern	int	adt_buflock(void);
extern	void	adt_bufrele(int);
extern	void	adt_clrlog(void);
extern	void	adt_error(char *, int, char *, int);
extern	int	adt_loginit(void);
extern	void	adt_newemask(aproc_t *);
extern	int	adt_pn_newpath(adtcwd_t *, unsigned int);
extern	void	adt_procchg(aproc_t *, unsigned long, struct cred *);
extern	void	adt_recwr(adt_argl_t *);
extern	void	adt_shutdown(void);
extern	void	adt_lock(void);
extern	void	adt_unlock(void);

#else		/* !defined (__STDC__) */

extern	int	adt_buflock();
extern	void	adt_bufrele();
extern	void	adt_clrlog();
extern	void	adt_error();
extern	int	adt_loginit();
extern	void	adt_newemask();
extern	int	adt_pn_newpath();
extern	void	adt_procchg();
extern	void	adt_recwr();
extern	void	adt_shutdown();
extern	void	adt_lock();
extern	void	adt_unlock();

#endif		/* defined (__STDC__) */

#endif		/* _KERNEL */

#endif		/* _ACC_AUDIT_AUDITMOD_H */

