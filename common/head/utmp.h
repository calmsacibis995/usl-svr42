/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sgs-head:utmp.h	1.5.2.1"

#ifndef _UTMP_H
#define _UTMP_H

#include <sys/types.h>

#define	UTMP_FILE	"/var/adm/utmp"
#define	WTMP_FILE	"/var/adm/wtmp"
#define	ut_name	ut_user

struct utmp
  {
	char ut_user[8] ;		/* User login name */
	char ut_id[4] ; 		/* /etc/inittab id(usually line #) */
	char ut_line[12] ;		/* device name (console, lnxx) */
#if defined(m88k)
	pid_t ut_pid ;			/* OCS 1.0 - process id */
#else
	short ut_pid ;			/* leave short for compatiblity - process id */
#endif
	short ut_type ; 		/* type of entry */
#if defined(m88k)
	short ut_pad ;			/* OCS 1.0 */
#endif
	struct exit_status
	  {
	    short e_termination ;	/* Process termination status */
	    short e_exit ;		/* Process exit status */
	  }
	ut_exit ;			/* The exit status of a process
					 * marked as DEAD_PROCESS.
					 */
	time_t ut_time ;		/* time entry was made */
#if defined(m88k)
	char ut_host[24] ;		/* host name, if remote OCS 1.0 */
#endif
  } ;

/*	Definitions for ut_type						*/

#define	EMPTY		0
#define	RUN_LVL		1
#define	BOOT_TIME	2
#define	OLD_TIME	3
#define	NEW_TIME	4
#define	INIT_PROCESS	5	/* Process spawned by "init" */
#define	LOGIN_PROCESS	6	/* A "getty" process waiting for login */
#define	USER_PROCESS	7	/* A user process */
#define	DEAD_PROCESS	8
#define	ACCOUNTING	9

#if defined(m88k)
#define FTP		128
#define REMOTE_LOGIN	129
#define REMOTE_PROCESS	130
#define	UTMAXTYPE	REMOTE_PROCESS	/* Largest legal value of ut_type */
#else
#define	UTMAXTYPE	ACCOUNTING	/* Largest legal value of ut_type */
#endif

/*	Special strings or formats used in the "ut_line" field when	*/
/*	accounting for something other than a process.			*/
/*	No string for the ut_line field can be more than 11 chars +	*/
/*	a NULL in length.						*/

#define	RUNLVL_MSG	"run-level %c"
#define	BOOT_MSG	"system boot"
#define	OTIME_MSG	"old time"
#define	NTIME_MSG	"new time"

#if defined(__STDC__)
extern void endutent(void);
extern struct utmp *getutent(void);
extern struct utmp *getutid(const struct utmp *);
extern struct utmp *getutline(const struct utmp *);
extern struct utmp *pututline(const struct utmp *); 
extern void setutent(void);
extern int utmpname(const char *);
#else
extern void endutent();
extern struct utmp *getutent();
extern struct utmp *getutid();
extern struct utmp *getutline();
extern struct utmp *pututline(); 
extern void setutent();
extern int utmpname();
#endif

#endif /* _UTMP_H */
