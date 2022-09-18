/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*	Portions Copyright(c) 1988, Sun Microsystems, Inc.	*/
/*	All Rights Reserved.					*/

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

#ident	"@(#)cron:common/cmd/cron/cron.c	1.17.7.5"
#ident  "$Header: $"

/***************************************************************************
 * Command: cron
 * Inheritable Privileges: P_DEV,P_MACREAD,P_MACWRITE,P_SETPLEVEL,P_SETUID,
 *			   P_AUDIT,P_SYSOPS,P_DACREAD
 *       Fixed Privileges: None
 * Notes:
 *
 ***************************************************************************/

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stropts.h>
#include <sys/poll.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <stdio.h>
#include <varargs.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <deflt.h>
#include <unistd.h>
#include <locale.h>
#include <priv.h>
#include <mac.h>
#include <pfmt.h>
#include <string.h>
#include <sys/secsys.h>
#include "cron.h"
#include <ia.h>
#include <audit.h>
#include <ulimit.h>


#define MAIL		"/usr/bin/mail"	/* mail program to use */
#define CONSOLE		"/dev/console"	/* where to write error messages when cron dies	*/

#define TMPINFILE	"/tmp/crinXXXXXX"  /* file to put stdin in for cmd  */
#define	TMPDIR		"/tmp"
#define	PFX		"crout"
#define TMPOUTFILE	"/tmp/croutXXXXXX" /* file to place stdout, stderr */

#define INMODE		00400		/* mode for stdin file	*/
#define OUTMODE		00600		/* mode for stdout file */
#define ISUID		06000		/* mode for verifing at jobs */

#define INFINITY	2147483647L	/* upper bound on time	*/
#define CUSHION		120L
#define	MAXRUN		25		/* max total jobs allowed in system */
#define ZOMB		100		/* proc slot used for mailing output */

#define	JOBF		'j'
#define	NICEF		'n'
#define	USERF		'u'
#define WAITF		'w'

#define BCHAR		'>'
#define	ECHAR		'<'

#define	DEFAULT		0
#define	LOAD		1

/* Defined actions for crabort() routine */
#define	NO_ACTION	000
#define	REMOVE_NPIPE	001
#define	CONSOLE_MSG	002


static const char
	GETCRONDERR[] =	":602:Cannot get level for /etc/cron.d.\n",
	LPMERR[] =	":587:Process terminated to enforce least privilege\n",
	LTDBERR[] =	":603:lvlin() on SYS_ALL failed.\n",
	NOLVLPROC[] =	":588:lvlproc() failed\n",
	MLDMODERR[] =	":604:Cannot change multi-level directory mode.\n",
	NOMLD[] =	":605:\"%s\" directory is not a multi-level directory.\n",
	MLDERR[] =	":606:\"%s\" multi-level directory is corrupt.\n",
	NOREADMLD[] =	":607:Cannot read the \"%s\" multi-level directory.\n",
	NOREADEFF[] =	":608:Cannot read a \"%s\" effective directory.\n",
	BADCD[] =	":609:Cannot change directory to \"%s\" multi-level directory.\n",
	SETERR[] =	":610:Cannot set required privilege.\n",
	LCKERR[] =	":611:Cannot open lockfile.\n",
	SLCKERR[] =	":612:Cannot set lock on file.\n",
	ILCKERR[] =	":613:File descriptor for lockfile.\n",
	FLCKERR[] =	":614:flock or its data is invalid.\n",
	ULCKERR[] =	":615:Unknown error in attempt to lock file.\n",
	NOREADDIR[] =	":73:Canot read the crontab directory.",
	BADJOBOPEN[] =	":74:Unable to read your at job.\n",
	BADSHELL[] =	":9:Because your login shell is not /usr/bin/sh, you cannot use %s\n",
	BADSTAT[] =	":75:Cannot access your crontab file.  Resubmit it.\n",
	CANTCDHOME[] =	":76:Cannot change directory to your home directory.\nYour commands will not be executed.\n",
	CANTEXECSH[] =	":77:Unable to exec the shell for one of your commands.\n",
	EOLN[] =	":78:Unexpected end of line.\n",
	NOREAD[] =	":79:Cannot read your crontab file.  Resubmit it.\n",
	NOSTDIN[] =	":80:Unable to create a standard input file for one of your crontab commands.\nThat command was not executed\n.",
	OUTOFBOUND[] =	":81:Number too large or too small for field\n",
	STDERRMSG[] =	":82:\n\n*************************************************\nCron: The previous message is the standard output\n      and standard error of one of your cron commands.\n",
	STDOUTERR[] =	":83:One of your commands generated output or errors, but cron was unable to mail you this output.\nRemember to redirect standard output and standard error for each of your commands.",
	UNEXPECT[] =	":84:Unexpected symbol found\n",
	ERRORWAS[] = 	":85:The error was \"%s\"\n",
	ERRMSG[] =	":37:%s: %s\n",
	CANTRUNJOB[] =	":96:Couldn't run your \"%s\" job\n\n",
	YOURJOB[] =	":104:Your \"%s\" job",
	BADLVLPROC[] =	":616:lvlproc() failed: %s\n",
	BADSECADVIS[] =	":684:secadvise() failed",
	RESCHEDAT[] =	":114:Rescheduling \"%s\" job",
	RESCHEDCRON[] =	":746:Rescheduling cron job: \"%s\"",
	LOGFERR[] =	":685:Unable to stat or open cron logfile.";

#define DIDFORK didfork
#define NOFORK !didfork

#define	ERR_CRONTABENT	0	/* error in crontab file entry */
#define	ERR_UNIXERR	1	/* error in some system call */
#define	ERR_CANTEXECCRON 2	/* error in setting up "cron" job environment*/
#define	ERR_CANTEXECAT	3	/* error in setting up "at" job environment */


static const char FORMAT[] = "%a %b %e %H:%M:%S %Y";
static const char FORMATID[] = ":22";
static uid_t privid;

char	timebuf[80];

struct event {	
	time_t time;	/* time of the event	*/
	short etype;	/* what type of event; 0=cron, 1=at	*/
	char *cmd;	/* command for cron, job name for at	*/
	struct usr *u;	/* ptr to the owner (usr) of this event	*/
	struct event *link; 	/* ptr to another event for this user */
	union { 
		struct { /* for crontab events */
			char *minute;	/*  (these	*/
			char *hour;	/*   fields	*/
			char *daymon;	/*   are	*/
			char *month;	/*   from	*/
			char *dayweek;	/*   crontab)	*/
			char *input;	/* ptr to stdin	*/
		} ct;
		struct { /* for at events */
			short exists;	/* for revising at events	*/
			int eventid;	/* for el_remove-ing at events	*/
		} at;
	} of; 
};

struct usr {	
	char *name;	/* name of user (e.g. "root")	*/
	char *home;	/* home directory for user	*/
	uid_t uid;	/* user id	*/
	gid_t gid;	/* group id	*/
	level_t lid;	/* MAC level identifier */
#ifdef ATLIMIT
	int aruncnt;	/* counter for running jobs per uid */
#endif
#ifdef CRONLIMIT
	int cruncnt;	/* counter for running cron jobs per uid */
#endif
	int ctid;	/* for el_remove-ing crontab events */
	short ctexists;	/* for revising crontab events	*/
	struct event *ctevents;	/* list of this usr's crontab events */
	struct event *atevents;	/* list of this usr's at events */
	struct usr *nextusr; 
};	/* ptr to next user	*/

struct	queue
{
	int njob;	/* limit */
	int nice;	/* nice for execution */
	int nwait;	/* wait time to next execution attempt */
	int nrun;	/* number running */
}	
	qd = {100, 2, 60},		/* default values for queue defs */
	qt[NQUEUE];
struct	queue	qq;
int	wait_time = 60;

struct	runinfo
{
	pid_t	pid;
	short	que;
	struct  usr *rusr;	/* pointer to usr struct */
	char 	*outfile;	/* file where stdout & stderr are trapped */
	short	jobtype;	/* what type of event: 0=cron, 1=at */
	char	*jobname;	/* command for "cron", jobname for "at" */
	int	mailwhendone;	/* 1 = send mail even if no ouptut */
}	rt[MAXRUN];

int mac_installed;	/* flag to see if MAC is installed */
level_t cron_lid;	/* level identifier of cron */
level_t	low_lid;	/* low lid corresponding to lid of /etc/cron.d */
short didfork = 0;	/* flag to see if I'm process group leader */
int msgfd;		/* file descriptor for receiving pipe end */
int ecid=1;		/* for giving event classes distinguishable id names 
			   for el_remove'ing them.  MUST be initialized to 1 */
short jobtype;		/* at or batch job */
int delayed;		/* is job being rescheduled or did it run first time */
int notexpired;		/* time for next job has not come */
int cwd;		/* current working directory */
int running;		/* zero when no jobs are executing */
int lckfd;              /* file descriptor for lock file */
struct event *next_event;	/* the next event to execute	*/
struct usr *uhead;	/* ptr to the list of users	*/
struct usr *ulast;	/* ptr to last usr table entry */
struct flock lck;       /* file and record locking structure */
time_t init_time,num(),time();
extern char *xmalloc();

/* user's default environment for the shell */
char homedir[100]="HOME=";
char logname[50]="LOGNAME=";
char tzone[100]="TZ=";
char *envinit[]={
	homedir,
	logname,
	"PATH=/sbin:/usr/bin:/usr/sbin:/usr/lbin:",
	"SHELL=/usr/bin/sh",
	tzone,
	0};
extern char **environ;

/* added for xenix */
#define DEFTZ		"ESTEDT"
int 	log = 0;
char 	hzname[10];
/* end of xenix */

static FILE *logf;
char logf_backup[MAXPATHLEN];
int logf_size = 0;	/* max bytes in log file before aging it */
int logf_lines = 0;	/* last number of lines to remain in log file */
			/* after aging handling is performed	*/

void cronend();
void timeout();
uid_t	cron_uid;

/*
 * Executables:
 *	-r-xr-s--- root cron  SYS_PRIVATE /usr/sbin/cron
 *							P_MACREAD (i)
 *							P_MACWRITE (i)
 *							P_SETPLEVEL(i)
 *							P_SETUID   (i)
 *							P_SYSOPS   (i)
 *
 * Other:
 *	drwxrwx--- root cron  SYS_PUBLIC /etc/cron.d
 *	prw-rw---- root cron  SYS_PUBLIC /etc/cron.d/NPIPE
 *	-rw-rw---- root cron SYS_PRIVATE /etc/cron.d/queuedefs
 *	-rw-rw---- root cron SYS_PRIVATE /etc/default/cron

 *	-rw-rw---- root cron SYS_PRIVATE /var/cron/log
 *
 *	drwxrwx--- root cron  SYS_PUBLIC /var/spool/cron
 *	drwxrwxrwt root cron  SYS_PUBLIC /var/spool/cron/atjobs
 *	drwxrwxrwt root cron  SYS_PUBLIC /var/spool/cron/crontabs
 *
 * Notes:
 *	1. cron always runs as group "cron".
 *	2. The user executing the cron daemon must have the following
 *	   privileges:	P_MACREAD, P_MACWRITE, P_SETFLEVEL,
 *			P_SETPLEVEL, P_SETUID and P_SYSOPS.
 */

main(argc,argv)
int argc;
char **argv;
{
	time_t t,t_old;
	time_t last_time;
	time_t ne_time;		/* amt of time until next event execution */
	time_t next_time();
	time_t lastmtime = 0L;
	int tz_old;		/* used to check if TZ changed since last time */
	int tz_new;		/* ditto */
	struct tm *tmz;
	struct usr *u,*u2;
	struct event *e,*e2,*eprev;
	struct stat buf;
	long seconds;
	pid_t rfork;
	int retval;

	/*make process EXEMPT*/
	if (procprivl(SETPRV, pm_work(P_AUDIT), (priv_t)0) == -1)
		crabort(LPMERR, CONSOLE_MSG, "", "", "");
	auditevt(ANAUDIT, NULL, sizeof(aevt_t));
        if (procprivl(CLRPRV, pm_work(P_AUDIT), (priv_t)0) == -1)
                crabort(LPMERR, CONSOLE_MSG, "", "", "");

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore");
	(void)setlabel("UX:cron");

	/* Clear all working privileges from working set */
	if (procprivl(CLRPRV, pm_work(P_ALLPRIVS), (priv_t)0) == -1)
		crabort(LPMERR, CONSOLE_MSG, "", "", "");

	if (procprivl(SETPRV, pm_work(P_SETPLEVEL),
			      pm_work(P_SETUID),
			      (priv_t)0) == -1)
		crabort(LPMERR, CONSOLE_MSG, "", "", "");

	if (lvlproc(MAC_GET, &cron_lid) == -1) {
		if (errno == ENOPKG) {
			mac_installed = 0;
			/* only hold on to P_SETUID, P_SYSOPS privileges */
			if (procprivl(CLRPRV, pm_max(P_SETPLEVEL),
					      pm_max(P_MACREAD),
					      pm_max(P_MACREAD),
					      pm_max(P_MACWRITE),
					      pm_max(P_DEV),
					      (priv_t)0) == -1)
				crabort(LPMERR, CONSOLE_MSG, "", "", "");
		} else
			crabort(NOLVLPROC, CONSOLE_MSG, "", "", "");
	} else {
		/* start off with MLD virtual mode */
		mac_installed = 1;
		if (mldmode(MLD_VIRT) == -1)
			crabort(MLDMODERR, CONSOLE_MSG, "", "", "");
		/* Get level of /etc/cron.d directory */
		if (lvlfile(CROND, MAC_GET, &low_lid) == -1)
			crabort(GETCRONDERR, CONSOLE_MSG, "", "", "");
	}

	/* remember cron's id */
	cron_uid = geteuid();

begin:
	/* fork unless 'nofork' is specified */
	if((argc <= 1) || (strcmp(argv[1],"nofork"))) {
		(void)procprivl(SETPRV, pm_work(P_SYSOPS), (priv_t)0);
		rfork = fork();
		if (procprivl(CLRPRV, pm_work(P_SYSOPS), (priv_t)0) == -1)
			crabort(LPMERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");
		if (rfork) {
			if (rfork == (pid_t)-1) {
				sleep(30);
				goto begin; 
			}
			exit(0); 
		}
		didfork++;
		setpgrp();
	}

	umask(022);
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, cronend);

	defaults();
	initialize(1);

	/* 
	 * If opening log file for first time, chmod to 664
	 * so that a login besides 'root' is able to open
	 * this file if cron has to be manually restarted
	 * at some point in the future.
	 */
	if(stat(ACCTFILE,&buf) < 0) {
		if (errno != ENOENT)
			crabort(LOGFERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");
		else {	/* File does not exist */
			if((logf=fopen(ACCTFILE, "w")) == NULL) 
				crabort(LOGFERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");
			if (chmod(ACCTFILE, 0664) < 0)
				pfmt(stderr, MM_ERROR, ":686:chmod failed\n");
		}
	}
	else {
		/* Append to log file */
		if((logf=fopen(ACCTFILE,"a")) == NULL)
			pfmt(stderr, MM_ERROR, ":92:Cannot open %s: %s\n",ACCTFILE,
				strerror(errno));
	}

	quedefs(DEFAULT);	/* load default queue definitions */
	msg(":86:*** cron started ***   pid = %d", getpid());
	timeout();		/* set up alarm clock trap */
	t_old = time((long *)0);  /* time interval since midnight 1/1/70 GMT */ 
	tmz = localtime(&t_old);  /* get local time information */
	tz_old = tmz->tm_isdst;	  /* 1 if daylight savings time in effect */
	last_time = t_old;
	while (TRUE) {			/* MAIN LOOP	*/
		t = time((long *) 0);
		tmz = localtime(&t);	/* get localtime information */
		tz_new = tmz->tm_isdst; /* has TZ changed since last job? */
		if((t_old > t) || (t-last_time > CUSHION) || (tz_new != tz_old)) {
			/* the time was set backwards or forward */
			/* or the alternate time zone has changed */
			el_delete();
			u = uhead;
			while (u!=NULL) {
				rm_ctevents(u);
				e = u->atevents;
				while (e!=NULL) {
					free(e->cmd);
					e2 = e->link;
					free(e);
					e = e2; 
				}
				u2 = u->nextusr;
				u = u2; 
			}
			close(msgfd);
			initialize(0);
			t = time((long *) 0); 
		}
		t_old = t;
		if (next_event == NULL) {
			if (el_empty()) ne_time = INFINITY;
			else {	
				next_event = (struct event *) el_first();
				ne_time = next_event->time - t; 
			}
		} else {
			ne_time = next_event->time - t;
#ifdef DEBUG
			cftime(timebuf, FORMAT, &next_event->time);
			fprintf(stderr, "next_time=%ld %s\n",
				next_event->time, timebuf);
#endif
		}
		seconds = (ne_time < (long) 0) ? (long) 0 : ne_time;
		if(ne_time > (long) 0)
			idle(seconds == 1L ? 2L : seconds);
		if(notexpired) {
			notexpired = 0;
			last_time = INFINITY;
			continue;
		}
		if(stat(QUEDEFS,&buf))
			msg(":87:Cannot stat QUEDEFS file");
		else
			if(lastmtime != buf.st_mtime) {
				quedefs(LOAD);
				lastmtime = buf.st_mtime;
			}
		last_time = next_event->time;	/* save execution time */

		if (mac_installed) {
			/*
			 * Execute event at the correct level.
			 * Don't execute if not at the right level.
			 */
			retval = lvlproc(MAC_SET, &next_event->u->lid);
			if (retval == 0) {
				ex(next_event);
				if (lvlproc(MAC_SET, &cron_lid) == -1)
					crabort(NOLVLPROC, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");
			}
		} else
			ex(next_event);
		switch(next_event->etype) {
		/* add cronevent back into the main event list */
		case CRONEVENT:
			if(delayed) {
				delayed = 0;
				break;
			}
			next_event->time = next_time(next_event);
			el_add( next_event,next_event->time,
			    (next_event->u)->ctid ); 
			break;
		/* remove at or batch job from system */
		default:
			eprev=NULL;
			e=(next_event->u)->atevents;
			while (e != NULL)
				if (e == next_event) {
					if (eprev == NULL)
						(e->u)->atevents = e->link;
					else	eprev->link = e->link;
					free(e->cmd);
					free(e);
					break;	
				}
				else {	
					eprev = e;
					e = e->link; 
				}
			break;
		}
		next_event = NULL; 
	}
}

initialize(firstpass)
{
	char *getenv();
	static int flag = 0;
	int omask;
	int fd;
	static int fds[2];

#ifdef DEBUG
	fprintf(stderr,"in initialize\n");
#endif
	init_time = time((long *) 0);
	el_init(8,init_time,(long)(60*60*24),10);
	if(firstpass) {
		/* for mail(1), make sure messages come from root */
		(void)putenv("LOGNAME=root");


		/* 
		 * Since we're creating a file in a directory
		 * whose level is not the same as this process'
		 * we need to set macwrite privilege.
		 */
		procprivl(SETPRV, pm_work(P_MACWRITE), (priv_t)0);
		omask = umask(0);
		if((lckfd=creat(LCK_CRON, 0660)) < 0) {
			pfmt(stderr, MM_ERROR, ERRMSG, LCK_CRON, strerror(errno));
			crabort(LCKERR, CONSOLE_MSG, "", "", "");
		}
		(void)umask(omask);
                procprivl(CLRPRV, pm_work(P_MACWRITE), (priv_t)0);

		/* 
		 * Setup so that we lock the entire file
		 */
		lck.l_type = F_WRLCK;   /* set write lock */
		lck.l_whence = 0;       /* from start of file */
		lck.l_start = 0L;
		lck.l_len = 0L;         /* till end of max file */

		/*
		 * Set lock on the file
		 */
		if (fcntl(lckfd, F_SETLK, &lck) < 0) {
			switch(errno) {
				case EACCES:
				case EAGAIN:
					crabort(SLCKERR, CONSOLE_MSG, "", "", "");
					break;
				case EBADF:
					crabort(ILCKERR, CONSOLE_MSG, "", "", "");
					break;
				case EINVAL:
				case EFAULT:
					crabort(FLCKERR, CONSOLE_MSG, "", "", "");
					break;
				default:
					crabort(ULCKERR, CONSOLE_MSG, "", "", "");
					break;
			}
			close(lckfd);
			pfmt(stderr, MM_ERROR, ":617:fcntl() failed: %s\n",
				strerror(errno));
			exit(1);
		}
	}
	else {	/* reimplementing pipe */
		(void) close(fds[0]);
		(void) close(fds[1]);
	}

	
	if(mac_installed) 
		/*
		 * Set process level to th
		 * level of /etc/cron.d dir
		 */
		if(lvlproc(MAC_SET, &low_lid) == -1)
			crabort(NOLVLPROC, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");

	if(firstpass)
		if(access(NPIPE,F_OK) == -1) {
			/*
			 * Create mount point at low level.
			 * Create pipe at low level.
			 * Attach name to pipe.
			 * Push connld on mounted end.
			 */
			omask = umask(0);
			if((fd = creat(NPIPE, 0660)) < 0)
				crabort(":618:Cannot create %s",
					REMOVE_NPIPE|CONSOLE_MSG, NPIPE,
					strerror(errno), "");
			(void)close(fd);
			(void)umask(omask);
		}
		else {
			if(NOFORK)
				/* didn't fork... init(1M) is waiting */
				sleep(30);
		}

	/*
	 * ioctl() uses cred of fd at creation time.
	 * Pushing connld to be run at multi-levels
	 * requires P_MACWRITE associated with the process at time
	 * of the ioctl().  So P_MACWRITE is turned on
	 * before the pipe() call, and turned off after the ioctl()
	 * call.
	 */
	(void)procprivl(SETPRV, pm_work(P_MACWRITE), (priv_t)0);
	if (pipe(fds) < 0)
		crabort(":619:pipe() failed: %s\n", REMOVE_NPIPE|CONSOLE_MSG,
			strerror(errno), "", "");

	if (fattach(fds[1], NPIPE) != 0)
		crabort(":620:Cannot attach pipe: %s\n", REMOVE_NPIPE|CONSOLE_MSG,
			strerror(errno), "", "");

	if (ioctl(fds[1], I_PUSH, "connld") != 0)
		crabort(":621:Cannot push connld: %s\n", REMOVE_NPIPE|CONSOLE_MSG,
			strerror(errno), "", "");

	if (procprivl(CLRPRV, pm_work(P_MACWRITE),
			      (priv_t)0) == -1)
		crabort(LPMERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");

	if (mac_installed)
		/*
		 * Reset process level to original level 
		 */
		if (lvlproc(MAC_SET, &cron_lid) == -1)
			crabort(NOLVLPROC, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");

	msgfd = fds[0];
	sprintf(tzone,"TZ=%s",getenv("TZ"));

	/* read directories, create users list,
	   and add events to the main event list	*/
	uhead = NULL;
	read_dirs();
	next_event = NULL;
	if(flag)
		return;
	/* this must be done to make popen work....i dont know why */
	freopen("/dev/null","r",stdin);
	flag = 1;
}

/*
 * read_dirs() reads the crontabs and atjobs directories for jobs to
 * include to its event list at initialization time.  If these directories
 * are multi-level directories, each effective directory is examined.
 */
read_dirs()
{
	struct	stat	buf;
	int	mod_ctab(), mod_atjob();

	if (mac_installed) {
		/* turn on real mode to test for MLDs */
		if (mldmode(MLD_REAL) == -1)
			crabort(MLDMODERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");

		(void) stat(CRONDIR, &buf);
		if (S_ISMLD & buf.st_flags)
			read_mldir(CRON, mod_ctab);
		else
			read_sldir(CRON, mod_ctab);

		(void) stat(ATDIR, &buf);
		if (S_ISMLD & buf.st_flags)
			read_mldir(AT, mod_atjob);
		else
			read_sldir(AT, mod_atjob);

		/* return to virtual mode */
		if (mldmode(MLD_VIRT) == -1)
			crabort(MLDMODERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");

	/* this is the base system */
	} else {
		read_sldir(CRON, mod_ctab);
		read_sldir(AT, mod_atjob);
	}
}

/*
 * read_mldir() is called in MLD real mode.
 * It performs the multi-level directory reading of a spool directory.
 */
read_mldir(type, func)
	int	type;
	int	(*func)();
{
	DIR	*dir, *opendir();
	DIR	*mldir;
	level_t lid;
	register struct	dirent *dp;
	char	*dname = (type == CRON) ? CRONDIR : ATDIR;

	if (chdir(dname) == -1)
		crabort(BADCD, REMOVE_NPIPE|CONSOLE_MSG,
			(type == CRON) ? "crontab" : "at", "" ,"");
	if ((mldir = opendir(".")) == NULL)
		crabort(NOREADMLD, REMOVE_NPIPE|CONSOLE_MSG,
			(type == CRON) ? "crontab" : "at", "" ,"");

	/*
	 * For cron to examine all effective directories,
	 * the level of the effective directories is retrieved
	 * using lvlfile().  Since lvlfile() on a file requires
	 * MAC read access, cron turns on its working
	 * P_MACREAD privilege.
	 */
	(void)procprivl(SETPRV, pm_work(P_MACREAD), (priv_t)0);

	while((dp = readdir(mldir)) != NULL) {
		/* skip the dot entries */
		if (strcmp(dp->d_name, ".") == 0
		||  strcmp(dp->d_name, "..") == 0)
			continue;

		/* skip entries which LIDs cannot be retrieved */
		if (lvlfile(dp->d_name, MAC_GET, &lid) == -1) 
			continue;

		/* if no level is associated with entry continue */
		if (lvlproc(MAC_SET, &lid) == -1)
			continue;

		/*
		 * If not a directory, continue with next directory.
		 */
		if (chdir(dp->d_name) == -1)
			continue;


		/*
		 * Setting cwd here implies that we'll remain in
		 * real mode; a problem with this is that if at any
		 * time this section of code accesses an MLD, the
		 * wrong action takes place.  for now, set to
		 * virtual mode.
		 * cwd = [AT|CRON]/dp->d_name;
		 */

		if ((dir=opendir("."))==NULL)
			crabort(NOREADEFF, REMOVE_NPIPE|CONSOLE_MSG,
				(type == CRON) ? "crontab" : "at", "" ,"");


		if (mldmode(MLD_VIRT) == -1)
			crabort(MLDMODERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");

		dscan(dir, func);

		if (mldmode(MLD_REAL) == -1)
			crabort(MLDMODERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");

		closedir(dir);

		if (chdir("..") == -1)
			crabort(BADCD, REMOVE_NPIPE|CONSOLE_MSG, "crontab", "", "");
	} /* end-while */

	/* reset cron's level */
	if (lvlproc(MAC_SET, &cron_lid) == -1)
		crabort(NOLVLPROC, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");
	if (procprivl(CLRPRV, pm_work(P_MACREAD), (priv_t)0) == -1)
		crabort(LPMERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");

	closedir(mldir);
}

/*
 * read_sldir() is called in MLD real mode, if MAC is installed.
 */

read_sldir(type, func)
	int	type;
	int	(*func)();
{
	DIR	*dir, *opendir();
	char	*dname = (type == CRON) ? CRONDIR : ATDIR;

	/* turn on MLD virtual mode */
	if (mac_installed) {
		if (mldmode(MLD_VIRT) == -1)
			crabort(MLDMODERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");
	}

	if (chdir(dname) == -1) crabort(BADCD, REMOVE_NPIPE|CONSOLE_MSG,
		(type == CRON) ? "crontab" : "at", "", "");
	if ((dir = opendir("."))==NULL)
		crabort(NOREADDIR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");
	dscan(dir, func);
	closedir(dir);

	/* reset MLD real mode */
	if (mac_installed) {
		if (mldmode(MLD_REAL) == -1)
			crabort(MLDMODERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");
	}
}

dscan(df,fp)
DIR	*df;
int	(*fp)();
{

	register	i, dn;
	register	struct	dirent	*dp;
	level_t		lid = 0;	/* Set default level for when MAC */
					/* is not installed 		  */

	while((dp=readdir(df)) != NULL) {
		/* skip entries which LID cannot be retrieved */
		if (mac_installed) {
			if (lvlfile(dp->d_name, MAC_GET, &lid) == -1)
				continue;
		}
		(*fp) (dp->d_name, lid);
	}
}

mod_ctab(name, lid)
char	*name;
level_t	lid;
{

	struct	passwd	*pw;
	struct	stat	buf;
	struct	usr	*u,*find_usr();
	char	namebuf[132];
	char	*pname;

	pw=getpwnam(name);

	if(pw == NULL)
		return;
	if(cwd != CRON) {
		strcat(strcat(strcpy(namebuf,CRONDIR),"/"),name);
		pname = namebuf;
	} else
		pname = name;
	/* a warning message is given by the crontab command so there is
	   no need to give one here......use this code if you only want users
	   with a login shell of /usr/bin/sh to use cron
	if((strcmp(pw->pw_shell,"")!=0) && (strcmp(pw->pw_shell,SHELL)!=0)){
			mail(name,BADSHELL,ERR_CANTEXECCRON, "cron");
			unlink(pname);
			return;
	}
	*/
	if(stat(pname,&buf)) {
		mail(name,BADSTAT,ERR_UNIXERR);
		unlink(pname);
		return;
	}
	if((u=find_usr(name, lid)) == NULL) {
#ifdef DEBUG
		fprintf(stderr,"new user (%s) with a crontab\n",name);
#endif
		u = (struct usr *) xmalloc(sizeof(struct usr));
		u->name = xmalloc(strlen(name)+1);
		strcpy(u->name,name);
		u->home = xmalloc(strlen(pw->pw_dir)+1);
		strcpy(u->home,pw->pw_dir);
		u->uid = pw->pw_uid;
		u->gid = pw->pw_gid;
		u->lid = lid;
		u->ctexists = TRUE;
		u->ctid = ecid++;
		u->ctevents = NULL;
		u->atevents = NULL;
#ifdef ATLIMIT
		u->aruncnt = 0;
#endif
#ifdef CRONLIMIT
		u->cruncnt = 0;
#endif
		u->nextusr = uhead;
		uhead = u;
		readcron(u);
	} else {
		u->uid = pw->pw_uid;
		u->gid = pw->pw_gid;
		if(strcmp(u->home,pw->pw_dir) != 0) {
			free(u->home);
			u->home = xmalloc(strlen(pw->pw_dir)+1);
			strcpy(u->home,pw->pw_dir);
		}
		u->ctexists = TRUE;
		if(u->ctid == 0) {
#ifdef DEBUG
			fprintf(stderr,"%s now has a crontab\n",u->name);
#endif
			/* user didnt have a crontab last time */
			u->ctid = ecid++;
			readcron(u);
			return;
		}
#ifdef DEBUG
		fprintf(stderr,"%s has revised his crontab\n",u->name);
#endif
		rm_ctevents(u);
		el_remove(u->ctid,0);
		readcron(u);
	}
}


mod_atjob(name, lid)
char	*name;
level_t	lid;
{

	char	*ptr;
	time_t	tim;
	struct	passwd	*pw;
	struct	stat	buf;
	struct	usr	*u,*find_usr();
	struct	event	*e;
	char	namebuf[132];
	char	*pname;

	ptr = name;
	if(((tim=num(&ptr)) == 0) || (*ptr != '.'))
		return;
	ptr++;
	if(!isalpha(*ptr))
		return;
	jobtype = *ptr - 'a';
	if(cwd != AT) {
		strcat(strcat(strcpy(namebuf,ATDIR),"/"),name);
		pname = namebuf;
	} else
		pname = name;
	if(stat(pname,&buf) || jobtype >= NQUEUE) {
		unlink(pname);
		return;
	}
	if(!(buf.st_mode & ISUID)) {
		unlink(pname);
		return;
	}
	pw=getpwuid(buf.st_uid);

	if(pw == NULL)
		return;
	/* a warning message is given by the at command so there is no
	   need to give one here......use this code if you only want users
	   with a login shell of /usr/bin/sh to use cron
	if((strcmp(pw->pw_shell,"")!=0) && (strcmp(pw->pw_shell,SHELL)!=0)){
			mail(pw->pw_name,BADSHELL,ERR_CANTEXECAT, "at");
			unlink(pname);
			return;
	}
	*/
	if((u=find_usr(pw->pw_name, lid)) == NULL) {
#ifdef DEBUG
		fprintf(stderr,"new user (%s) with an at job = %s\n",pw->pw_name,name);
#endif
		u = (struct usr *) xmalloc(sizeof(struct usr));
		u->name = xmalloc(strlen(pw->pw_name)+1);
		strcpy(u->name,pw->pw_name);
		u->home = xmalloc(strlen(pw->pw_dir)+1);
		strcpy(u->home,pw->pw_dir);
		u->uid = pw->pw_uid;
		u->gid = pw->pw_gid;
		u->lid = lid;
		u->ctexists = FALSE;
		u->ctid = 0;
		u->ctevents = NULL;
		u->atevents = NULL;
#ifdef ATLIMIT
		u->aruncnt = 0;
#endif
#ifdef CRONLIMIT
		u->cruncnt = 0;
#endif
		u->nextusr = uhead;
		uhead = u;
		add_atevent(u,name,tim);
	} else {
		u->uid = pw->pw_uid;
		u->gid = pw->pw_gid;
		if(strcmp(u->home,pw->pw_dir) != 0) {
			free(u->home);
			u->home = xmalloc(strlen(pw->pw_dir)+1);
			strcpy(u->home,pw->pw_dir);
		}
		e = u->atevents;
		while(e != NULL)
			if(strcmp(e->cmd,name) == 0) {
				e->of.at.exists = TRUE;
				break;
			} else
				e = e->link;
		if (e == NULL) {
#ifdef DEBUG
			fprintf(stderr,"%s has a new at job = %s\n",u->name,name);
#endif
			add_atevent(u,name,tim);
		}
	}
}



add_atevent(u,job,tim)
struct usr *u;
char *job;
time_t tim;
{
	struct event *e;

	e=(struct event *) xmalloc(sizeof(struct event));
	e->etype = jobtype;
	e->cmd = xmalloc(strlen(job)+1);
	strcpy(e->cmd,job);
	e->u = u;
#ifdef DEBUG
	fprintf(stderr,"add_atevent: user=%s, job=%s, time=%ld\n",
		u->name,e->cmd, e->time);
#endif
	e->link = u->atevents;
	u->atevents = e;
	e->of.at.exists = TRUE;
	e->of.at.eventid = ecid++;
	if(tim < init_time)		/* old job */
		e->time = init_time;
	else
		e->time = tim;
	el_add(e, e->time, e->of.at.eventid); 
}


char line[CTLINESIZE];		/* holds a line from a crontab file	*/
int cursor;			/* cursor for the above line	*/

readcron(u)
struct usr *u;
{
	/* readcron reads in a crontab file for a user (u).
	   The list of events for user u is built, and 
	   u->events is made to point to this list.
	   Each event is also entered into the main event list. */

	FILE *fopen(),*cf;		/* cf will be a user's crontab file */
	time_t next_time();
	struct event *e;
	int start,i;
	char *next_field();
	char namebuf[132];
	char *pname;

	/* read the crontab file */
	if(cwd != CRON) {
		strcat(strcat(strcpy(namebuf,CRONDIR),"/"),u->name);
		pname = namebuf;
	} else
		pname = u->name;
	(void)seteuid(u->uid);
	cf = fopen(pname, "r");
	if (seteuid(cron_uid) == -1)
		crabort(LPMERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");
	if (cf == (FILE *)NULL) {
		mail(u->name,NOREAD,ERR_UNIXERR);
		return; 
	}
	while (fgets(line,CTLINESIZE,cf) != NULL) {
		/* process a line of a crontab file */
		cursor = 0;
		while(line[cursor] == ' ' || line[cursor] == '\t')
			cursor++;
		if(line[cursor] == '#')
			continue;
		e = (struct event *) xmalloc(sizeof(struct event));
		e->etype = CRONEVENT;
		if ((e->of.ct.minute=next_field(0,59,u)) == NULL) goto badline;
		if ((e->of.ct.hour=next_field(0,23,u)) == NULL) goto badline;
		if ((e->of.ct.daymon=next_field(1,31,u)) == NULL) goto badline;
		if ((e->of.ct.month=next_field(1,12,u)) == NULL) goto badline;
		if ((e->of.ct.dayweek=next_field(0,6,u)) == NULL) goto badline;
		if (line[++cursor] == '\0') {
			mail(u->name,EOLN,ERR_CRONTABENT);
			goto badline; 
		}
		/* get the command to execute	*/
		start = cursor;
again:
		while ((line[cursor]!='%')&&(line[cursor]!='\n')
		    &&(line[cursor]!='\0') && (line[cursor]!='\\')) cursor++;
		if(line[cursor] == '\\') {
			cursor += 2;
			goto again;
		}

                /* 
                 * Check if file based privilege system, if yes prepend
		 * the command to be executed with 
		 *	TFADMIN=/sbin/tfadmin  export TFADMIN; 
		 * so that commands run from crontab will run via tfadmin
                 * if the crontab entry is prefixed with "$TFADMIN".
                 */
                privid = (uid_t)secsys(ES_PRVID, 0);
                if (privid < 0) {       /* File base privilege mechanism */
                       e->cmd = xmalloc(cursor-start+39);
                       strcpy(e->cmd,"TFADMIN=/sbin/tfadmin export TFADMIN; ");
                       strncat(e->cmd, line+start, cursor-start);
                       e->cmd[cursor-start+39] = '\0';
                }
                else {                  /* ID based privilege system */
                        e->cmd = xmalloc(cursor-start+1);
                        strncpy(e->cmd,line+start,cursor-start);
                        e->cmd[cursor-start] = '\0';
                }

		/* see if there is any standard input	*/
		if (line[cursor] == '%') {
			e->of.ct.input = xmalloc(strlen(line)-cursor+1);
			strcpy(e->of.ct.input,line+cursor+1);
			for (i=0; i<(int)strlen(e->of.ct.input); i++)
				if (e->of.ct.input[i] == '%') e->of.ct.input[i] = '\n'; 
		}
		else e->of.ct.input = NULL;
		/* have the event point to it's owner	*/
		e->u = u;
		/* insert this event at the front of this user's event list   */
		e->link = u->ctevents;
		u->ctevents = e;
		/* set the time for the first occurance of this event	*/
		e->time = next_time(e);
		/* finally, add this event to the main event list	*/
		el_add(e,e->time,u->ctid);
#ifdef DEBUG
		cftime(timebuf, FORMAT, &e->time);
		fprintf(stderr,"inserting cron event %s at %ld (%s)\n",
			e->cmd,e->time,timebuf);
#endif
		continue;

badline: 
		free(e); 
	}

	fclose(cf);
}


mail(usrname,msg,format, a1, a2, a3)
char *usrname,*msg, *a1, *a2, *a3;
int format;
{
		/* mail mails a user a message.	*/

		FILE *pipe,*popen();
		char *temp,*i,*strrchr();
		struct passwd	*ruser_ids;
		pid_t fork_val;
		int saveerrno = errno;

	#ifdef TESTING
		return;
	#endif
		
		(void)procprivl(SETPRV, pm_work(P_SYSOPS), (priv_t)0);
		fork_val = fork();
		if (procprivl(CLRPRV, pm_work(P_SYSOPS), (priv_t)0) == -1)
			crabort(LPMERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");
		if (fork_val == (pid_t)-1) {
			running++;
			return;
		}
		if (fork_val == 0) {
			if ((ruser_ids = getpwnam(usrname)) == (struct passwd *)NULL)
					exit(0);

			setuid(ruser_ids->pw_uid);
			temp = xmalloc(strlen(MAIL)+strlen(usrname)+2);
			pipe = popen(strcat(strcat(strcpy(temp,MAIL)," "),usrname),"w");
			if (pipe!=NULL) {
			    fprintf(pipe,"To: %s\nSubject: ", usrname);
			    switch (format) {
				case ERR_CRONTABENT:
				    pfmt(pipe, MM_NOSTD, ":94:Your crontab file has an error in it");
				    fputs("\n\n", pipe);
				    i = strrchr(line,'\n');
				    if (i != NULL) *i = ' ';
				    fprintf(pipe, "\t%s\n\t", line);
				    pfmt(pipe, MM_NOSTD, msg, a1, a2, a3);
  			    putc('\n', pipe);
			    pfmt(pipe, MM_NOSTD, ":95:This entry has been ignored.\n"); 
				    break;	
			case ERR_UNIXERR:
		  	    pfmt(pipe, MM_NOSTD, msg, a1, a2, a3);
		  	    fputs("\n\n", pipe);
			    pfmt(pipe, MM_NOSTD, ERRORWAS, strerror(saveerrno));
			    break;

			case ERR_CANTEXECCRON:
			    pfmt(pipe, MM_NOSTD, CANTRUNJOB, "cron");
			    pfmt(pipe, MM_NOSTD, msg, a1, a2, a3);
			    putc('\n', pipe);
			    pfmt(pipe, MM_NOSTD, ERRORWAS, strerror(saveerrno));

			case ERR_CANTEXECAT:
			    pfmt(pipe, MM_NOSTD, CANTRUNJOB, "at");
			    pfmt(pipe, MM_NOSTD, msg, a1, a2, a3);
			    putc('\n', pipe);
			    pfmt(pipe, MM_NOSTD, ERRORWAS, strerror(saveerrno));
		    }
		    pclose(pipe); 
		}
		free(temp);
		exit(0);
	}
	/* decremented in idle() */
	running++;
}



char
*next_field(lower,upper,u)
int lower,upper;
struct usr *u;
{
	/* next_field returns a pointer to a string which holds 
	   the next field of a line of a crontab file.
	   if (numbers in this field are out of range (lower..upper),
	       or there is a syntax error) then
			NULL is returned, and a mail message is sent to
			the user telling him which line the error was in.     */

	char *s;
	int num,num2,start;

	while ((line[cursor]==' ') || (line[cursor]=='\t')) cursor++;
	start = cursor;
	if (line[cursor] == '\0') {
		mail(u->name,EOLN,ERR_CRONTABENT);
		return(NULL); 
	}
	if (line[cursor] == '*') {
		cursor++;
		if ((line[cursor]!=' ') && (line[cursor]!='\t')) {
			mail(u->name,UNEXPECT,ERR_CRONTABENT);
			return(NULL); 
		}
		s = xmalloc(2);
		strcpy(s,"*");
		return(s); 
	}
	while (TRUE) {
		if (!isdigit(line[cursor])) {
			mail(u->name,UNEXPECT,ERR_CRONTABENT);
			return(NULL); 
		}
		num = 0;
		do { 
			num = num*10 + (line[cursor]-'0'); 
		}			while (isdigit(line[++cursor]));
		if ((num<lower) || (num>upper)) {
			mail(u->name,OUTOFBOUND,ERR_CRONTABENT);
			return(NULL); 
		}
		if (line[cursor]=='-') {
			if (!isdigit(line[++cursor])) {
				mail(u->name,UNEXPECT,ERR_CRONTABENT);
				return(NULL); 
			}
			num2 = 0;
			do { 
				num2 = num2*10 + (line[cursor]-'0'); 
			}				while (isdigit(line[++cursor]));
			if ((num2<lower) || (num2>upper)) {
				mail(u->name,OUTOFBOUND,ERR_CRONTABENT);
				return(NULL); 
			}
		}
		if ((line[cursor]==' ') || (line[cursor]=='\t')) break;
		if (line[cursor]=='\0') {
			mail(u->name,EOLN,ERR_CRONTABENT);
			return(NULL); 
		}
		if (line[cursor++]!=',') {
			mail(u->name,UNEXPECT,ERR_CRONTABENT);
			return(NULL); 
		}
	}
	s = xmalloc(cursor-start+1);
	strncpy(s,line+start,cursor-start);
	s[cursor-start] = '\0';
	return(s);
}


time_t
next_time(e)
struct event *e;
{
	/* returns the integer time for the next occurance of event e.
	   the following fields have ranges as indicated:
	PRGM  | min	hour	day of month	mon	day of week
	------|-------------------------------------------------------
	cron  | 0-59	0-23	    1-31	1-12	0-6 (0=sunday)
	time  | 0-59	0-23	    1-31	0-11	0-6 (0=sunday)
	   NOTE: this routine is hard to understand. */

	struct tm *tm,*localtime();
	int tm_mon,tm_mday,tm_wday,wday,m,min,h,hr,carry,day,days,
	d1,day1,carry1,d2,day2,carry2,daysahead,mon,yr,db,wd,today;
	time_t t;
	static int firstpass = 1, dst;

	t = time((long *) 0);
	tm = localtime(&t);

	tm_mon = next_ge(tm->tm_mon+1,e->of.ct.month) - 1;	/* 0-11 */
	tm_mday = next_ge(tm->tm_mday,e->of.ct.daymon);		/* 1-31 */
	tm_wday = next_ge(tm->tm_wday,e->of.ct.dayweek);	/* 0-6  */
	today = TRUE;
	if ( (strcmp(e->of.ct.daymon,"*")==0 && tm->tm_wday!=tm_wday)
	    || (strcmp(e->of.ct.dayweek,"*")==0 && tm->tm_mday!=tm_mday)
	    || (tm->tm_mday!=tm_mday && tm->tm_wday!=tm_wday)
	    || (tm->tm_mon!=tm_mon)) today = FALSE;

	m = tm->tm_min+1;
	if ((tm->tm_hour + 1) <= next_ge(tm->tm_hour%24, e->of.ct.hour)) {
		m = 0;
	}
	min = next_ge(m%60,e->of.ct.minute);
	carry = (min < m) ? 1:0;
	h = tm->tm_hour+carry;
	hr = next_ge(h%24,e->of.ct.hour);
	carry = (hr < h) ? 1:0;
	if ((!carry) && today) {
		/* this event must occur today	*/
		if (tm->tm_min>min)
			t +=(time_t)(hr-tm->tm_hour-1)*HOUR + 
			    (time_t)(60-tm->tm_min+min)*MINUTE;
		else t += (time_t)(hr-tm->tm_hour)*HOUR +
			(time_t)(min-tm->tm_min)*MINUTE;
		return(t-(long)tm->tm_sec); 
	}

	min = next_ge(0,e->of.ct.minute);
	hr = next_ge(0,e->of.ct.hour);

	/* calculate the date of the next occurance of this event,
	   which will be on a different day than the current day.	*/

	/* check monthly day specification	*/
	d1 = tm->tm_mday+1;
	day1 = next_ge((d1-1)%days_in_mon(tm->tm_mon,tm->tm_year)+1,e->of.ct.daymon);
	carry1 = (day1 < d1) ? 1:0;

	/* check weekly day specification	*/
	d2 = tm->tm_wday+1;
	wday = next_ge(d2%7,e->of.ct.dayweek);
	if (wday < d2) daysahead = 7 - d2 + wday;
	else daysahead = wday - d2;
	day2 = (d1+daysahead-1)%days_in_mon(tm->tm_mon,tm->tm_year)+1;
	carry2 = (day2 < d1) ? 1:0;

	/* based on their respective specifications,
	   day1, and day2 give the day of the month
	   for the next occurance of this event.	*/

	if ((strcmp(e->of.ct.daymon,"*")==0) && (strcmp(e->of.ct.dayweek,"*")!=0)) {
		day1 = day2;
		carry1 = carry2; 
	}
	if ((strcmp(e->of.ct.daymon,"*")!=0) && (strcmp(e->of.ct.dayweek,"*")==0)) {
		day2 = day1;
		carry2 = carry1; 
	}

	yr = tm->tm_year;
	if ((carry1 && carry2) || (tm->tm_mon != tm_mon)) {
		/* event does not occur in this month	*/
		m = tm->tm_mon+1;
		mon = next_ge(m%12+1,e->of.ct.month)-1;		/* 0..11 */
		carry = (mon < m) ? 1:0;
		yr += carry;
		/* recompute day1 and day2	*/
		day1 = next_ge(1,e->of.ct.daymon);
		db = days_btwn(tm->tm_mon,tm->tm_mday,tm->tm_year,mon,1,yr) + 1;
		wd = (tm->tm_wday+db)%7;
		/* wd is the day of the week of the first of month mon	*/
		wday = next_ge(wd,e->of.ct.dayweek);
		if (wday < wd) day2 = 1 + 7 - wd + wday;
		else day2 = 1 + wday - wd;
		if ((strcmp(e->of.ct.daymon,"*")!=0) && (strcmp(e->of.ct.dayweek,"*")==0))
			day2 = day1;
		if ((strcmp(e->of.ct.daymon,"*")==0) && (strcmp(e->of.ct.dayweek,"*")!=0))
			day1 = day2;
		day = (day1 < day2) ? day1:day2; 
	}
	else { /* event occurs in this month	*/
		mon = tm->tm_mon;
		if (!carry1 && !carry2) day = (day1 < day2) ? day1 : day2;
		else if (!carry1) day = day1;
		else day = day2;
	}

	/* now that we have the min,hr,day,mon,yr of the next
	   event, figure out what time that turns out to be.	*/

	days = days_btwn(tm->tm_mon,tm->tm_mday,tm->tm_year,mon,day,yr);
	t += (time_t)(23-tm->tm_hour)*HOUR + (time_t)(60-tm->tm_min)*MINUTE
	    + (time_t)hr*HOUR + (time_t)min*MINUTE + (time_t)days*DAY;
	return(t-(long)tm->tm_sec);
}



#define	DUMMY	100
next_ge(current,list)
int current;
char *list;
{
	/* list is a character field as in a crontab file;
	   	for example: "40,20,50-10"
	   next_ge returns the next number in the list that is
	   greater than or equal to current.
	   if no numbers of list are >= current, the smallest
	   element of list is returned.
	   NOTE: current must be in the appropriate range.	*/

	char *ptr;
	int n,n2,min,min_gt;

	if (strcmp(list,"*") == 0) return(current);
	ptr = list;
	min = DUMMY; 
	min_gt = DUMMY;
	while (TRUE) {
		if ((n=(int)num(&ptr))==current) return(current);
		if (n<min) min=n;
		if ((n>current)&&(n<min_gt)) min_gt=n;
		if (*ptr=='-') {
			ptr++;
			if ((n2=(int)num(&ptr))>n) {
				if ((current>n)&&(current<=n2))
					return(current); 
			}
			else {	/* range that wraps around */
				if (current>n) return(current);
				if (current<=n2) return(current); 
			}
		}
		if (*ptr=='\0') break;
		ptr += 1; 
	}
	if (min_gt!=DUMMY) return(min_gt);
	else return(min);
}

del_atjob(name,usrname,lid)
char	*name;
char	*usrname;
level_t lid;
{

	struct	event	*e, *eprev;
	struct	usr	*u, *find_usr();

	if((u = find_usr(usrname, lid)) == NULL)
		return;
	e = u->atevents;
	eprev = NULL;
	while(e != NULL)
		if(strcmp(name,e->cmd) == 0) {
			if(next_event == e)
				next_event = NULL;
			if(eprev == NULL)
				u->atevents = e->link;
			else
				eprev->link = e->link;
			el_remove(e->of.at.eventid, 1);
			free(e->cmd);
			free(e);
			break;
		} else {
			eprev = e;
			e = e->link;
		}
	if(!u->ctexists && u->atevents == NULL) {
#ifdef DEBUG
		fprintf(stderr,"%s removed from usr list\n",usrname);
#endif
		if(ulast == NULL)
			uhead = u->nextusr;
		else
			ulast->nextusr = u->nextusr;
		free(u->name);
		free(u->home);
		free(u);
	}
}

del_ctab(name, lid)
char	*name;
level_t lid;
{

	struct	usr	*u, *find_usr();

	if((u = find_usr(name, lid)) == NULL)
		return;
	rm_ctevents(u);
	el_remove(u->ctid, 0);
	u->ctid = 0;
	u->ctexists = 0;
	if(u->atevents == NULL) {
#ifdef DEBUG
		fprintf(stderr,"%s removed from usr list\n",name);
#endif
		if(ulast == NULL)
			uhead = u->nextusr;
		else
			ulast->nextusr = u->nextusr;
		free(u->name);
		free(u->home);
		free(u);
	}
}


rm_ctevents(u)
struct usr *u;
{
	struct event *e2,*e3;

	/* see if the next event (to be run by cron)
	   is a cronevent owned by this user.		*/
	if ( (next_event!=NULL) && 
	    (next_event->etype==CRONEVENT) &&
	    (next_event->u==u) )
		next_event = NULL;
	e2 = u->ctevents;
	while (e2 != NULL) {
		free(e2->cmd);
		free(e2->of.ct.minute);
		free(e2->of.ct.hour);
		free(e2->of.ct.daymon);
		free(e2->of.ct.month);
		free(e2->of.ct.dayweek);
		if (e2->of.ct.input != NULL) free(e2->of.ct.input);
		e3 = e2->link;
		free(e2);
		e2 = e3; 
	}
	u->ctevents = NULL;
}


struct usr *find_usr(uname, lid)
char *uname;
level_t lid;
{
	struct usr *u;

	u = uhead;
	ulast = NULL;
	while (u != NULL) {
		if (strcmp(u->name,uname) == 0 && u->lid == lid) return(u);
		ulast = u;
		u = u->nextusr; 
	}
	return(NULL);
}


ex(e)
struct event *e;
{

	register i,j;
	short sp_flag;
	int fd;
	pid_t rfork;
	FILE *atcmdfp;
	char atline[BUFSIZ];
	int ulim;
	char mailvar[4];
	char *at_cmdfile, *cron_infile;
	char *mktemp();
	char *tempnam();
	struct stat buf;
	struct queue *qp;
	struct runinfo *rp;
	int retval;
	uinfo_t uinfo = NULL;
	actl_t	actl;
	aevt_t	aevt;
	arec_t	admp;
	acronrec_t	acron_rec;
	extern void copylog();
	off_t size;


	/*
	 * In case log file has grown too big (size > SIZE bytes),
	 * keep only the last 100 lines and copy the rest of the to an
	 * aged log file.
	 * 
	 * copylog(log_file, aged_log_file, lines_to_remain_in_logfile)
	 */
	
	/* Check if log file has outgrown threshold value */ 
	if (stat(ACCTFILE, &buf) < 0 || buf.st_size == 0 || buf.st_size < logf_size) 
		;
	else {
		fclose(logf);
		(void) copylog(ACCTFILE, logf_backup, logf_lines);
		/* Reopen log file */
		if((logf=fopen(ACCTFILE,"a")) == NULL)
			pfmt(stderr, MM_ERROR, ":92:Cannot open %s: %s\n",ACCTFILE,
				strerror(errno));
	}

	qp = &qt[e->etype];	/* set pointer to queue defs */
	if(qp->nrun >= qp->njob) {
		msg(":97:%c queue max run limit reached",e->etype+'a');
		resched(qp->nwait);
		return;
	}
	for(rp=rt; rp < rt+MAXRUN; rp++) {
		if(rp->pid == 0)
			break;
	}
	if(rp >= rt+MAXRUN) {
		msg(":98:MAXRUN (%d) procs reached",MAXRUN);
		resched(qp->nwait);
		return;
	}
#ifdef ATLIMIT
	if((e->u)->uid != 0 && (e->u)->aruncnt >= ATLIMIT) {
		msg(":99:ATLIMIT (%d) reached for uid %d",ATLIMIT,(e->u)->uid);
		resched(qp->nwait);
		return;
	}
#endif
#ifdef CRONLIMIT
	if((e->u)->uid != 0 && (e->u)->cruncnt >= CRONLIMIT) {
		msg(":100:CRONLIMIT (%d) reached for uid %d",CRONLIMIT,(e->u)->uid);
		resched(qp->nwait);
		return;
	}
#endif

	rp->outfile = tempnam(TMPDIR,PFX);
	rp->jobtype = e->etype;
	if (e->etype == CRONEVENT) {
		if ((rp->jobname = (char *)malloc(strlen(e->cmd)+1)) != NULL)
			(void) strcpy(rp->jobname, e->cmd);
		rp->mailwhendone = 0;	/* "cron" jobs only produce mail if there's output */

	} else {
		at_cmdfile = xmalloc(strlen(ATDIR)+strlen(e->cmd)+2);
		strcat(strcat(strcpy(at_cmdfile,ATDIR),"/"),e->cmd);
		(void)seteuid(e->u->uid);
		atcmdfp = fopen(at_cmdfile, "r");
		if (seteuid(cron_uid) == -1)
			crabort(LPMERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");
		if (atcmdfp == NULL) {
			mail((e->u)->name,BADJOBOPEN,ERR_CANTEXECAT);
			(void)seteuid(e->u->uid);
			unlink(e->cmd);
			if (seteuid(cron_uid) == -1)
				crabort(LPMERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");
			return;
		}
		if ((rp->jobname = (char *)malloc(strlen(at_cmdfile)+1)) != NULL)
			(void) strcpy(rp->jobname, at_cmdfile);
		
		/*
		 * Skip over the first two lines.
		 */
		fscanf(atcmdfp,"%*[^\n]\n");
		fscanf(atcmdfp,"%*[^\n]\n");
		if (fscanf(atcmdfp,": notify by mail: %3s%*[^\n]\n",mailvar) == 1) {
			/*
			 * Check to see if we should always send mail
			 * to the owner.
			 */
			rp->mailwhendone = (strcmp(mailvar, "yes") == 0);
		} else
			rp->mailwhendone = 0;
		/*
		 * Read in user's ulimit from atjob file.  After forking
		 * a child process to exec the at job, set the ulimit for
		 * that child process to the ulimit obtained here.
		 */
		ulim = 0;
		while ((fgets(atline, sizeof(atline), atcmdfp)) != NULL)
			if (strncmp(atline, "ulimit ", 6) == 0) {
				(void)sscanf(atline, "ulimit %d", &ulim);
				break;
			}
		(void)fclose(atcmdfp);
	}
	(void)procprivl(SETPRV, pm_work(P_SYSOPS), (priv_t)0);
	rfork = fork();
	if (procprivl(CLRPRV, pm_work(P_SYSOPS), (priv_t)0) == -1)
		crabort(LPMERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");
	if (rfork == (pid_t)-1) {
		msg(":101:fork() failed: %s", strerror(errno));
		resched(wait_time);
		sleep(30);
		return;
	}
	if(rfork) {	/* parent process */
		++qp->nrun;
		++running;
		rp->pid = rfork;
		rp->que = e->etype;
#ifdef ATLIMIT
		if(e->etype != CRONEVENT)
			(e->u)->aruncnt++;
#endif
#if ATLIMIT && CRONLIMIT
		else
			(e->u)->cruncnt++;
#else
#ifdef CRONLIMIT
		if(e->etype == CRONEVENT)
			(e->u)->cruncnt++;
#endif
#endif
		rp->rusr = (e->u);
		logit((char)BCHAR,rp,0);
		return;
	}
	/*
	 * This is the child.  If the child must be aborted, just
	 * exit.  Don't call crabort() which is only applicable to
	 * the parent.
	 */
	for (i=0; i<20; i++)
		close(i);
	if (e->etype != CRONEVENT ) { /* open jobfile as stdin to shell */
	
		/*
		 * It's an at job - if we read in a value for user's
		 * ulimit from the at job file, we set child process'
		 * ulimit to that value.
		 */
		if (ulim) {
			(void)procprivl(SETPRV, pm_work(P_SYSOPS), (priv_t)0);
			(void)ulimit(UL_SETFSIZE, ulim);
			if (procprivl(CLRPRV, pm_work(P_SYSOPS), (priv_t)0) == -1)
				exit(1);
		}
		if (stat(at_cmdfile,&buf))
			exit(1);
		if (!(buf.st_mode&ISUID)) { 
		/* if setuid bit off, original owner has given this file to someone else */
			(void)seteuid(e->u->uid);
			unlink(at_cmdfile);
			(void)seteuid(cron_uid);
			exit(1); 
		}
		(void)seteuid(e->u->uid);
		retval = open(at_cmdfile, O_RDONLY);
		if (seteuid(cron_uid) == -1)
			exit(1);
		if (retval == -1) {
			mail((e->u)->name,BADJOBOPEN,ERR_CANTEXECCRON);
			(void)seteuid(e->u->uid);
			unlink(at_cmdfile);
			(void)seteuid(cron_uid);
			exit(1); 
		}
		(void)seteuid(e->u->uid);
		unlink(at_cmdfile); 
		if (seteuid(cron_uid) == -1)
			exit(1);
	}

        if (procprivl(SETPRV, pm_work(P_AUDIT), (priv_t)0) == -1)
		exit(1);
	if ((auditctl(ASTATUS, &actl, sizeof(actl_t)) == 0) && (actl.auditon)) {

		/*
		 * Retrieve the default user mask, of the user associated with
		 * the cron job being processed.  Since /etc/security/ia/index
		 * is read only for root, set dacread priv for when cron is started
		 * by administrator
		 */

		if (procprivl(SETPRV, pm_work(P_DACREAD), pm_work(P_MACREAD), (priv_t)0) == -1)
			exit(1);
		if ( (ia_openinfo(e->u->name, &uinfo)) || (uinfo == NULL) )
			exit(1);
		if (procprivl(CLRPRV, pm_work(P_DACREAD), pm_work(P_MACREAD), (priv_t)0) == -1)
			exit(1);
		ia_get_mask(uinfo, aevt.emask);

		/*Set the user mask of the current process, cron, to the */
		/*above mask. Cron is exempt for auditing so there is no */
		/*net effect for the cron process. The soon to be exec'ed*/
		/*cron job will inherit this new user mask.              */
		if (auditevt(ASETME, &aevt, sizeof(aevt_t)) == -1) {
			ia_closeinfo(uinfo);
			exit(1);
		}
		ia_closeinfo(uinfo);

		/*Populate and write the audit cron record*/
		acron_rec.uid=e->u->uid;
		acron_rec.gid=e->u->gid;
		acron_rec.lid=e->u->lid;
		if (strlen(e->cmd) < (size_t)ADT_CRONSZ)
			strcpy(acron_rec.cronjob,e->cmd);
		else
		{
			strncpy(acron_rec.cronjob,e->cmd,ADT_CRONSZ-1);
			acron_rec.cronjob[ADT_CRONSZ-1]='\0';
		}
		admp.rtype=ADT_CRON;
		/*Note: the 0 (success) value does not indicate the success*/
		/*of the cron job.                                         */
		admp.rstatus=0;
		admp.rsize=sizeof(acronrec_t);
		admp.argp=(char *)&acron_rec;
		auditdmp(&admp, sizeof(arec_t));

		/*make process auditable*/
		auditevt(AYAUDIT, NULL, sizeof(aevt_t));
        	if (procprivl(CLRPRV, pm_work(P_AUDIT), (priv_t)0) == -1)
         	       exit(1);
	}


	/*
	 * set correct user and group identification and initialize
	 * the supplementary group access list
	 */
	if (setgid(e->u->gid) == -1
		|| initgroups(e->u->name, e->u->gid) == -1
		|| setuid(e->u->uid) == -1)
		exit(1);
	/*
	 * Clear all privileges in the working set only if the system is running
	 * a file based privilege mechanism.  This is done so
	 * that, when running on an ID based privilege system,
	 * root can obtain all privileges (cron's maximum set) 
	 * as a result of the recalculation performed during
	 * the above setuid.
	 */
	if (privid < 0)		
		(void) procprivl(CLRPRV, pm_work(P_ALLPRIVS), (priv_t)0);

	sp_flag = FALSE;
	if (e->etype == CRONEVENT)
		/* check for standard input to command	*/
		if (e->of.ct.input != NULL) {
			cron_infile = mktemp(TMPINFILE);
			if ((fd=creat(cron_infile,INMODE)) == -1) {
				mail((e->u)->name,NOSTDIN,ERR_CANTEXECCRON);
				exit(1); 
			}
			if (write(fd,e->of.ct.input,strlen(e->of.ct.input))
			    != strlen(e->of.ct.input)) {
				mail((e->u)->name,NOSTDIN,ERR_CANTEXECCRON);
				unlink(cron_infile);
				exit(1); 
			}
			close(fd);
			/* open tmp file as stdin input to sh	*/
			if (open(cron_infile,O_RDONLY)==-1) {
				mail((e->u)->name,NOSTDIN,ERR_CANTEXECCRON);
				unlink(cron_infile);
				exit(1); 
			}
			unlink(cron_infile); 
		}
		else if (open("/dev/null",O_RDONLY)==-1) {
			open("/",O_RDONLY);
			sp_flag = TRUE; 
		}

	/* redirect stdout and stderr for the shell	*/
	if (creat(rp->outfile,OUTMODE)!=-1) dup(1);
	else if (open("/dev/null",O_WRONLY)!=-1) dup(1);
	if (sp_flag) close(0);
	strcat(homedir,(e->u)->home);
	strcat(logname,(e->u)->name);
	environ = envinit;
	if (chdir((e->u)->home)==-1) {
		mail((e->u)->name,CANTCDHOME,
		  e->etype == CRONEVENT ? ERR_CANTEXECCRON : ERR_CANTEXECAT);
		exit(1); 
	}
#ifdef TESTING
	exit(1);
#endif
	if((e->u)->uid != 0)
		nice(qp->nice);

	if (privid < 0)		
		(void) procprivl(CLRPRV, pm_max(P_ALLPRIVS), (priv_t)0);

	if (e->etype == CRONEVENT)
		execl(SHELL,"sh","-c",e->cmd,0);
	else /* type == ATEVENT */
		execl(SHELL,"sh",0);

	mail((e->u)->name,CANTEXECSH,
	    e->etype == CRONEVENT ? ERR_CANTEXECCRON : ERR_CANTEXECAT);
	exit(1);
}


idle(tyme)
long tyme;
{

	long t;
	time_t	now;
	pid_t	pid;
	int	prc;
	long	alm;
	struct	runinfo	*rp;

	t = tyme;
	while(t > 0L) {
		if(running) {
			if(t > wait_time)
				alm = wait_time;
			else
				alm = t;
#ifdef DEBUG
			fprintf(stderr,"in idle - setting alarm for %ld sec\n",alm);
#endif
			alarm((unsigned) alm);
			pid = wait(&prc);
			alarm(0);
#ifdef DEBUG
			fprintf(stderr,"wait returned %x\n",prc);
#endif
			if(pid == (pid_t)-1) {
				if(msg_wait())
					return;
			} else {
				for(rp=rt;rp < rt+MAXRUN; rp++)
					if(rp->pid == pid)
						break;
				if(rp >= rt+MAXRUN) {
					msg(":102:Unexpected pid returned %d (ignored)",pid);
					/* incremented in mail() */
					running--;
				}
				else {
					if (mac_installed) {
						if (lvlproc(MAC_SET, &rp->rusr->lid) == -1)
							crabort(LPMERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");
					}

					if(rp->que == ZOMB) {
						running--;
						rp->pid = 0;
						(void)seteuid(rp->rusr->uid);
						unlink(rp->outfile);
						if (seteuid(cron_uid) == -1)
							crabort(LPMERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");
						free(rp->outfile);
					}
					else
						cleanup(rp,prc);

					if (mac_installed) {
						if (lvlproc(MAC_SET, &cron_lid) == -1)
							crabort(NOLVLPROC, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");
					}
				}
			}
		} else {
			msg_wait();
			return;
		}
		now = time((long *) 0);
		t = (long)next_event->time - now;
	}
}


cleanup(pr,rc)
struct	runinfo	*pr;
{

	int	fd;
	char	line[5+UNAMESIZE+CTLINESIZE];
	struct	usr	*p;
	struct	stat	buf;
	struct	passwd	*ruser_ids;
	FILE	*mailpipe;
	FILE	*st;
	int	nbytes;
	char	iobuf[BUFSIZ];

	logit((char)ECHAR,pr,rc);
	--qt[pr->que].nrun;
	pr->pid = 0;
	--running;
	p = pr->rusr;
#ifdef ATLIMIT
	if(pr->que != CRONEVENT)
		--p->aruncnt;
#endif
#if ATLIMIT && CRONLIMIT
	else
		--p->cruncnt;
#else
#ifdef CRONLIMIT
	if(pr->que == CRONEVENT)
		--p->cruncnt;
#endif
#endif
	if(!stat(pr->outfile,&buf)) {
		if(buf.st_size > 0 || pr->mailwhendone) {
			/* mail user stdout and stderr */
			(void)procprivl(SETPRV, pm_work(P_SYSOPS), (priv_t)0);
			pr->pid = fork();
			if (procprivl(CLRPRV,pm_work(P_SYSOPS),(priv_t)0)==-1)
				crabort(LPMERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");
			if (pr->pid == 0) {

				/*
				 * Get uid for real user and become that person.
				 * We do this so that mail won't come from root since
				 * this could be a security hole.
				 * If failure, quit - don't send mail as root.
				 */
				if ((ruser_ids = getpwnam(p->name)) == 
					(struct passwd *)NULL)
					exit(0);
				setuid(ruser_ids->pw_uid);

				(void) strcpy(line, MAIL);
				(void) strcat(line, " ");
				(void) strcat(line, p->name);
				mailpipe = popen(line, "w");
				if (mailpipe == NULL)
					exit(127);
				(void) fprintf(mailpipe, "To: %s\nSubject: ", p->name);
				if (pr->jobtype == CRONEVENT) {
					(void) pfmt(mailpipe, MM_NOSTD,
					    ":103:Output from \"cron\" command\n\n");
					(void) pfmt(mailpipe, MM_NOSTD, YOURJOB,
					    "cron");
					if (pr->jobname != NULL) {
						(void) fprintf(mailpipe,
						    "\n\n%s\n\n", pr->jobname);
					}
				} else {
					(void) pfmt(mailpipe, MM_NOSTD,
					    ":105:Output from \"at\" job\n\n");
					(void) pfmt(mailpipe, MM_NOSTD, YOURJOB, "at");
					if (pr->jobname != NULL)
						(void) fprintf(mailpipe,
						    " \"%s\"", pr->jobname);
				}
				(void) fprintf(mailpipe, " ");
				if (buf.st_size > 0
				    && (st = fopen(pr->outfile, "r")) != NULL) {
					(void) pfmt(mailpipe, MM_NOSTD,
					    ":106:produced the following output:\n\n");
					while ((nbytes = fread(iobuf,
					    sizeof (char), BUFSIZ, st)) != 0)
						(void) fwrite(iobuf,
						    sizeof (char), nbytes,
						    mailpipe);
					(void) fclose(st);
				} else
					(void) pfmt(mailpipe, MM_NOSTD,
					    ":107:completed.\n");
				(void) pclose(mailpipe);
				exit(0);
			}
			pr->que = ZOMB;
			running++;
		} else {
			(void)seteuid(pr->rusr->uid);
			unlink(pr->outfile);
			if (seteuid(cron_uid) == -1)
				crabort(LPMERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");
			free(pr->outfile);
		}
	}
}

#define	MSGSIZE	sizeof(struct message)

msg_wait()
{

	long	t;
	time_t	now;
	struct	passwd *pw;
	struct	message	*pmsg;
	int	cnt;
	int 	subsize;	/* To be set to size of sub structure */
	struct pollfd pollfd;
	struct s_strrecvfd *recbuf;
	struct obj_attr obj;
	level_t level = 0;	/* Set default for when MAC is not installed */
	char cront_file[MAXPATHLEN];

	/* Get size of the subjects attributes structure */
	if ((subsize = secadvise(0, SA_SUBSIZE, 0)) < 0)
		crabort(BADSECADVIS, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");

	recbuf = (struct s_strrecvfd *) xmalloc(sizeof(struct s_strrecvfd) 
			+ subsize - sizeof(struct sub_attr));

	pollfd.fd = msgfd;
	pollfd.events = POLLIN;
	pollfd.revents = 0;

	if (poll(&pollfd, 1, 0) <= 0  &&  running)
		return(0);

	if(next_event == NULL)
		t = INFINITY;
	else {
		now = time((long *) 0);
		t = next_event->time - now;
		if(t <= 0L)
			t = 1L;
	}
#ifdef DEBUG
	fprintf(stderr,"in msg_wait - setting alarm for %ld sec\n", t);
#endif
	alarm((unsigned) t);
	pmsg = &msgbuf;
	errno = 0;

	recbuf->fd = 0;

	/*
	 * Retrieve credentials from the sending process.
	 */
	(void)procprivl(SETPRV, pm_work(P_MACWRITE), (priv_t)0);
	if (ioctl(msgfd, I_S_RECVFD, recbuf) != 0) {
		if(errno != EINTR) {
			pfmt(stderr, MM_ERROR, ":622:ioctl() I_S_RECVFD failed: %s\n",
				strerror(errno));
			notexpired = 1;
			(void)free(recbuf);
		}
		if(next_event == NULL)
			notexpired = 1;
		return(1);
	}
	if (procprivl(CLRPRV, pm_work(P_MACWRITE), (priv_t)0) == -1)
		crabort(LPMERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");

	if((cnt = read(recbuf->fd, pmsg, MSGSIZE)) != MSGSIZE) {
		if(errno != EINTR) {
			msg(":108:Read error: %s", strerror(errno));
			notexpired = 1;
		}
		if(next_event == NULL)
			notexpired = 1;
		(void)close(recbuf->fd);
		(void)free(recbuf);
		return(1);
	}
	if (mac_installed) {
		(void)procprivl(SETPRV, pm_work(P_MACREAD), (priv_t)0);
		if(flvlfile(recbuf->fd, MAC_GET, &level) != 0)
			crabort(":623:Cannot get level for unique pipe\n",
				REMOVE_NPIPE|CONSOLE_MSG, "", "", "");
		if (procprivl(CLRPRV, pm_work(P_MACREAD), (priv_t)0) == -1)
			crabort(LPMERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");
	}
	(void)close(recbuf->fd);


	/*
	 * Get information on login whose jobs are to be manipulated.
	 * This is done by getting information on login after which
	 * the crontab file is named or, in the case of at, the owner
	 * of the at job file.
	 */
	if((pw = getpwnam(pmsg->logname)) == NULL) {
		msg(":747:Cannot get data on login %s: %s", pmsg->logname, strerror(errno));
		notexpired = 1;
		return(1);
	}
	obj.uid = pw->pw_uid;	
	obj.gid = pw->pw_gid;
	obj.mode = 0700;
	obj.lid = level;	/* Level of process that sent msg to cron */

	if(secadvise(&obj, SA_WRITE, &(recbuf->s_attrs)) < 0)
		if(errno == EFAULT)
			crabort(":625:Can't determine privileges of sending process\n",
				REMOVE_NPIPE|CONSOLE_MSG, "", "", "");
		else {
			switch(pmsg->action) {
			case DELETE:
				msg(":626:Attempt by uid %d to delete at/cron job rejected",
					recbuf->uid);
				break;
			case ADD:
				msg(":627:Attempt by uid %d to add/change at/cron job rejected",
					recbuf->uid);
				break;
			default:
				msg(":628:Attempt by uid %d to manipulate at/cron job rejected",
					recbuf->uid);
				break;
			}
			notexpired = 1;
			(void) free(recbuf);
			return(1);
		}

	(void) free(recbuf);

	alarm(0);
	if(pmsg->etype != NULL) {
		if (mac_installed) {
			if (lvlproc(MAC_SET, &level) == -1)
				pmsg->etype = NULL;
		}
		switch(pmsg->etype) {
		case AT:
			if(pmsg->action == DELETE) {
				if(log)
					msg(":629:Deleting at job %s for login %s", pmsg->fname, pmsg->logname);
				del_atjob(pmsg->fname,pmsg->logname, level);
			}
			else {
				if(log)
					msg(":630:Adding/modifying at job %s for login %s", pmsg->fname, pmsg->logname);
				mod_atjob(pmsg->fname, level);
			}
			break;
		case CRON:
			if(pmsg->action == DELETE) {
				if(log)
					msg(":631:Deleting cron job(s) for %s", pmsg->fname);
				del_ctab(pmsg->fname, level);
			}
			else {
				if(log)
					msg(":632:Adding/modifying cron job(s) for %s", pmsg->fname);
				mod_ctab(pmsg->fname, level);
			}
			break;
		default:
			msg(":109:Message received - bad format");
			break;
		}
		if (mac_installed && pmsg->etype) {
			if (lvlproc(MAC_SET, &cron_lid) == -1)
				crabort(NOLVLPROC, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");
		}
		if (next_event != NULL) {
			if (next_event->etype == CRONEVENT)
				el_add(next_event,next_event->time,(next_event->u)->ctid);
			else /* etype == ATEVENT */
				el_add(next_event,next_event->time,next_event->of.at.eventid);
			next_event = NULL;
		}
		fflush(stdout);
		pmsg->etype = NULL;
		notexpired = 1;
		return(1);
	}
}


void
timeout(s)
int s;
{
	signal(SIGALRM, timeout);
}

void
cronend()
{
	crabort(":110:SIGTERM", REMOVE_NPIPE, "", "", "");
}


/*
 * crabort() - handle exits out of cron 
 */
crabort(mssg, action, a1, a2, a3)
	char	*mssg;
	int	action;
	char	*a1, *a2, *a3;
{
	FILE	*c;

	/* reset level of cron for the rest of crabort() */
	if (mac_installed) {
		if (action & REMOVE_NPIPE) {
			if (lvlproc(MAC_SET, &low_lid) == -1)
				pfmt(stderr, MM_ERROR, BADLVLPROC, strerror(errno));
			/*
			 * NPIPE should vanish when cron finishes so detach
			 * unlink it.
			 */
			(void) fdetach(NPIPE);

			if(unlink(NPIPE) < 0)
				pfmt(stderr, MM_ERROR, ":633:Cannot unlink %s: %s\n",
					NPIPE, strerror(errno));
			if (lvlproc(MAC_SET, &cron_lid) == -1)
				pfmt(stderr, MM_ERROR, BADLVLPROC, strerror(errno));
		} else
			(void)lvlproc(MAC_SET, &cron_lid);
	} else {
		if (action & REMOVE_NPIPE) {
			/*
			 * NPIPE should vanish when cron finishes so detach
			 * and unlink it.
			 */
			(void) fdetach(NPIPE);
			if(unlink(NPIPE) < 0)
				pfmt(stderr, MM_ERROR, ":633:Cannot unlink %s: %s\n",
					NPIPE, strerror(errno));
		}
	}

	if (action & CONSOLE_MSG) {
		(void)procprivl(SETPRV, pm_work(P_DEV), (priv_t)0);
		/* write error msg to console */
		if ((c=fopen(CONSOLE,"w")) != NULL) {
			pfmt(c, MM_NOSTD, ":112:cron aborted: ");
			pfmt(c, MM_NOSTD, mssg, a1, a2, a3);
			if (mssg[strlen(mssg) - 1] != '\n')
				putc('\n', c);
			fclose(c); 
		}

		if (procprivl(CLRPRV, pm_work(P_DEV), (priv_t)0) == -1)
			pfmt(stderr, MM_ERROR, ":634:No least privilege at crabort(): %s\n",
				strerror(errno));

		/* write error message to stderr */
		pfmt(stderr, MM_NOSTD, ":112:cron aborted: ");
		pfmt(stderr, MM_NOSTD, mssg, a1, a2, a3);
			if (mssg[strlen(mssg) - 1] != '\n')
				putc('\n', stderr);

	}

	/* always log the message */
	msg(mssg, a1, a2, a3);

	msg(":113:******* CRON ABORTED ********");
	exit(1);
}

msg(va_alist)
va_dcl
{

	va_list args;
	char *fmt;
	time_t	t;

	t = time((long *) 0);

	va_start(args);
	fmt = va_arg(args, char *);
	(void)vpfmt(logf, MM_NOSTD, fmt, args);
	va_end(args);

	(void)cftime(timebuf, gettxt(FORMATID, FORMAT), &t);
	(void)fprintf(logf, " %s\n", timebuf);
	(void)fflush(logf);
}


logit(cc,rp,rc)
char	cc;
struct	runinfo	*rp;
{
	time_t t;
	int    ret;

	if (!log)
                return;

	t = time((long *) 0);
	if(cc == BCHAR)
		fprintf(logf, "%c  CMD: %s\n",cc, next_event->cmd);
	cftime(timebuf, gettxt(FORMATID, FORMAT), &t);
	fprintf(logf,"%c  %s %u %c %s",
		cc,(rp->rusr)->name, rp->pid, QUE(rp->que),timebuf);
	if((ret=TSTAT(rc)) != 0)
		fprintf(logf," ts=%d",ret);
	if((ret=RCODE(rc)) != 0)
		fprintf(logf," rc=%d",ret);
	putc('\n', logf);
	fflush(logf);
}

resched(delay)
int	delay;
{
	time_t	nt;

	/* run job at a later time */
	nt = next_event->time + delay;
	if(next_event->etype == CRONEVENT) {
		next_event->time = next_time(next_event);
		if(nt < next_event->time)
			next_event->time = nt;
		el_add(next_event,next_event->time,(next_event->u)->ctid);
		delayed = 1;
		msg(RESCHEDCRON, next_event->cmd);
		return;
	}
	add_atevent(next_event->u, next_event->cmd, nt);
	msg(RESCHEDAT, next_event->cmd);
}

#define	QBUFSIZ		80

quedefs(action)
int	action;
{
	register i;
	int	j;
	char	name[MAXNAMELEN];
	char	qbuf[QBUFSIZ];
	FILE	*fd;

	/* set up default queue definitions */
	for(i=0;i<NQUEUE;i++) {
		qt[i].njob = qd.njob;
		qt[i].nice = qd.nice;
		qt[i].nwait = qd.nwait;
	}
	if(action == DEFAULT)
		return;
	if((fd = fopen(QUEDEFS,"r")) == NULL) {
		msg(":115:Cannot open quedefs file: %s", strerror(errno));
		msg(":116:Using default queue definitions");
		return;
	}
	while(fgets(qbuf, QBUFSIZ, fd) != NULL) {
		if((j=qbuf[0]-'a') < 0 || j >= NQUEUE || qbuf[1] != '.')
			continue;
		i = 0;
		while(qbuf[i] != NULL)
			name[i] = qbuf[i++];
		name[i] = NULL;
		parsqdef(&name[2]);
		qt[j].njob = qq.njob;
		qt[j].nice = qq.nice;
		qt[j].nwait = qq.nwait;
	}
	fclose(fd);
}

parsqdef(name)
char *name;
{
	register i;

	qq = qd;
	while(*name) {
		i = 0;
		while(isdigit(*name)) {
			i *= 10;
			i += *name++ - '0';
		}
		switch(*name++) {
		case JOBF:
			qq.njob = i;
			break;
		case NICEF:
			qq.nice = i;
			break;
		case WAITF:
			qq.nwait = i;
			break;
		}
	}
}

/***	defaults -- read defaults	- M000 -
 *     		    from /etc/default/cron
 */

defaults()
{
	FILE *def_fp;
	char *getenv();
	register int  flags;
	register char *deflog;
	register char *defbackup;
	register char *deflines;
	register char *defsize;
	char *hz, *tz;

	/*
	 * get HZ value for environment
	 */
	if ((hz = getenv("HZ")) == (char *)NULL )
		sprintf(hzname, "HZ=%d", HZ);
	else
		sprintf(hzname, "HZ=%s", hz);
	/*
	 * get TZ value for environment
	 */
	sprintf(tzone, "TZ=%s", ((tz = getenv("TZ")) != NULL) ? tz : DEFTZ);

	if ((def_fp = defopen(DEFFILE)) != NULL) {
		if (((deflog = defread(def_fp, "CRONLOG")) == NULL) ||
		     (*deflog == 'N') || (*deflog == 'n'))
			log = 0;
		else
			log = 1;

		if ((defbackup = defread(def_fp, "BACKUP")) == NULL)
			strcpy(logf_backup, BACKUP);
		else
			strcpy(logf_backup, defbackup);

		if ((deflines = defread(def_fp, "LINES")) == NULL) 
			logf_lines = LINES;
		else
			logf_lines=atoi(deflines);

		if ((defsize = defread(def_fp, "SIZE")) == NULL)
			logf_size = SIZE;
		else
			logf_size = atoi(defsize);

		(void) defclose(def_fp);
	}
	return;
}
