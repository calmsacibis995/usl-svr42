/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)head.usr:remop.h	1.1.3.2"
#ident  "$Header: remop.h 1.3 91/06/21 $"

/*
 * ROI - Remote Operations Interface
 */

	/* Login names of authorized users */

#define	WORKDIR		"/var/spool/roi/users/"

	/* Network services primitives */

#define NSIDIR		"/usr/sadm/roi/nsi/"

	/* Various files for ROI operations */

#define ALIAS_FILE	"/var/sadm/roi/config/alias"
#define NEW_ALIAS_FILE	"/var/sadm/roi/config/new_alias"
#define NSERVICES	"/var/sadm/roi/config/nservices"
#define NEW_NSERVICES	"/var/sadm/roi/config/new_nservices"
#define NSADMIN		"/var/sadm/roi/config/nsadmin"
#define NEW_NSADMIN	"/var/sadm/roi/config/new_nsadmin"

	/* Default values for aging and timeout parameters */

#define	AGING		"2.0.0"
#define	TIMEOUT		"1.0.0"

	/* Operation types under ROI */

#define	SYNCHRONOUS	1
#define QUEUED		2

	/* Description of ROI states */

#define	ROI_REJECT	"reject"
#define ROI_FAILED	"failed"
#define ROI_CANCEL	"cancel"
#define ROI_TMOUT	"tmout"
#define ROI_INPROG	"inprog"
#define ROI_QUEUED	"queued"
#define ROI_SUCC	"succ"
#define ROI_NOSTAT	"nostat"



/*
 * stand-alone jobs
 */

#define DEP_NONE	0


/*
 * Jobs that have dependencies on other jobs
 */

/* the first job in a dependency list */

#define	DEP_START	1	

/* the intermediate jobs in a dependency list */

#define DEP_MID		2

/* the last job in a dependency list */

#define DEP_END		3	


/* 
 * structure describing a remote
 * operation job
 */

struct remop {
	char	*machine;	/* remote system name */
	int	sid;		/* service job id */
	int	adminid;	/* administrative job id */
	int	primid;		/* primitive job id */
	int	depend_flg;	/* flag to indicate position
				 * within a dependency list */
	int	dependid;	/* dependent job id */
	int	exit_status;	/* field to store the exit status 
				 * of a remote operation */
	int	filler;		/* not used */
	struct remop *next;	/* next remop structure in the list */
};

/*	
 * Contents of TOS files. Tokens of the
 * form name=value
 */ 

#define	DMACH		"DMACH="	/* Remote machine name */
#define OMACH		"OMACH="        /* Local machine name */
#define NTWKSVC		"NTWKSVC="      /* Name of network service */
#define	STIME		"STIME="        /* Originated time of a job */
#define	SJID		"SJID="		/* Service job id */
#define	AJID		"AJID="		/* Administrative job id */
#define	PJID		"PJID="		/* Primitive job id */
#define SVC		"SVC="		/* Service identifier */
#define	PRIM		"PRIM="		/* Primitive type */
#define	DEPEND		"DEPEND="	/* dependent job id */
#define FIRST		"FIRST="        /* Job id of the first job
					 * in a dependency list */
#define	NEXT		"NEXT="		/* The following job in a
					 * dependency list */
#define	OPERAND		"OPERAND="      /* operand of a remote operation */
#define	NOTIFY		"NOTIFY="       /* path name to the notify executable */
#define	USER		"USER="		/* logname */
#define	NSID		"NSID="         /* network service specific job id */
#define	_CLEANUP	"_CLEANUP="     /* indicates that a cleanup operation
					 * is required */
#define _DTJID		"_DTJID="	/* part of a DT job */
#define _LDTJID		"_LDTJID="	/* last dt job <tos file> */
#define WD		"WD="		/* current working directory */

/*
 * Job id types
 */
#define	SID	1
#define ADMINID	2
#define PRIMID	3

/* Machine name length */

#define	DST_LEN	 20

/* Service identifier length */

#define SVC_LEN  10

/* Primitive type length */

#define PRIM_LEN 10

/* Network Service name length */

#define NS_LEN   10


#define DIRECTORY(f, s)	((stat((f), &s)==0) && ((s.st_mode&(S_IFMT))==S_IFDIR))
#define REGFILE(f, s)	((stat((f), &s)==0) && ((s.st_mode&(S_IFMT))==S_IFREG))

/*
 *
 * Exit code definitions for network service interface
 *
 */

enum nsi_status {
	NSI_COMPLETE,	  /* job completed */
	NSI_QUEUED,	  /* job queued */
	NSI_FAIL,	  /* job failed */
	NSI_SVC_BAD,	  /* service command failed */
	NSI_BAD_TOS	  /* TOS file error */
};

/*
 * Network failure
 */

#define	NTWK_FAIL	255

/*
 * Job status record
 */

struct	job_record {
	long	rtime;		/* request time */
	int	sid;		/* service job id */
	int	adminid;	/* administrative job id */
	int	primid;		/* primitive job id */
	int	stat;		/* current job status */
	char	dst[DST_LEN]; 	/* destination node name */
	char	svc[SVC_LEN]; 	/* service identifier */
	char	prim[PRIM_LEN]; /* primitive identifier */
	char	ns[NS_LEN];     /* network service */
};

#define	STAT_SIZE	sizeof(struct job_record)

/* Operations on job status files */

#define READ	1
#define	UPDATE	2
#define APPEND  4

/*
 * Job states
 */
enum job_stat {
	ST_REJECTED,
	ST_FAILED,
	ST_CANCELLED,
	ST_TIMEOUT,
	ST_INPROGRESS,
	ST_QUEUED,
	ST_SUCCEEDED,
	ST_MAX
};


/* Link list of name=value structures */

typedef	struct	stringll {
	char	*name;
	char	*value;
	struct	stringll  *next;
} stringll_t;

#define	NULL_STRING	(stringll_t *)0

#ifdef	__STDC__
int remop(const char *, const char *, const char *,
	struct remop *, const char *, const char *);
int mgroup(const char *, struct remop **);
int roistat(int, const char *, struct job_record *);
int roitosparse(const char *, struct stringll **);
char *roitosval(struct stringll *, const char *);
int roijobids(int, int *);
char *roigetuser(char *);
#else
int  remop();
int  mgroup();
int  roistat();
int  roitosparse();
char *roitosval();
int  roijobids();
char *roigetuser();
#endif
