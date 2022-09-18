/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
	rmnttry - attempt to perform queued mounts
*/

#ident	"@(#)rmount:rmnttry.c	1.1.16.3"
#ident  "$Header: rmnttry.c 1.2 91/06/27 $"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <nserve.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mnttab.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <wait.h>
#include "rmount.h"
#include "rmnttab.h"
#include <errno.h>
#include <mac.h>

char 		*cmd;

extern char	*fqn();		/* provides fully qualified names */
extern int	rfs_up();	/* from libns */

static int	request();
static int	rd_mnttab();
static int	trymount();
static void	killpid();

int
main(argc, argv)
	int		argc;
	char		**argv;
{
	struct rmnttab	rtab;		/* rmnttab structure */
	int		fails	= 0;	/* number of failed mounts */
	int		chg	= 0;	/* change flag for rmnttab */
	int		i, j;		/* indices for command line names */
	char		**ulist;	/* list of unused resources */
	FILE		*tmp;		/* temp file to write unchanged entries
				   		in rmnttab */
	int		rfd;		/* file descriptor to create rmnttab */
	FILE		*rp;		/* rmnttab file pointer */
	struct stat	stbuf;
	struct mnttab	*mnt;		/* program's copy of mnttab */
	unsigned int	mnt_count;	/* number of entries in mnttab */
	char		*msg_mo	= "%s: warning: %s already mounted on %s\n";
	char		*msg_ub	= "%s: warning: mount point %s used by %s\n";

	cmd = argv[0];

#ifdef   OLDSEC
	if (geteuid() !=  0) {
		Fprintf(stderr, "%s: must be super-user\n", cmd);
		exit(2);
	}
#endif /*OLDSEC*/

	lock();		/* create semaphore file for entire session */

	/* if RFS is not running, then truncate the rmnttab table */
	if (rfs_up() != 0) {
		if ((rfd = creat(RMNTTAB, MASK)) < 0) {
			Fprintf(stderr, "%s: cannot create %s\n",cmd,RMNTTAB);
			fails = 2;	
		}
		Close(rfd);
		unlock();
		return fails;
	}

	switch ( fails = rd_rmnttab(&rp, &stbuf) ) {
		case 1:		/* rmnttab does not exist, exit quietly */
			unlock();
			return 0;
		case 2:		/* open or stat fails */
			unlock();
			return fails;
		case 0:
			break;
		default:
			unlock();
			Fprintf(stderr, "%s: error in reading rmnttab\n", cmd);
			return fails;
	}

	if (!rd_mnttab(&mnt, &mnt_count)) {/* keep quiet in case of error */
		unlock();
		return 2;
	}

	if ((tmp=fopen(TMP, "w")) == NULL) {
		perror(cmd);
		Fprintf(stderr,"%s: cannot open temp file to write\n", cmd);
		return 2;
	}

	/* scan through rmnttab, trying either all entries
	 * or specific entries appearing in argv.
	 */

	while (getrmntent(rp, &rtab) == 0) 
		if (argc == 1 || request(rtab.rmnt_special, argv)) {

			/*  attempt to mount this resource but first see if it
			    or its mount point is already in /etc/mnttab  */

			for (i = 0; i < mnt_count; i++) {
				if (strcmp( mnt[i].mnt_special,
					rtab.rmnt_special) == 0) {

					Printf(	msg_mo,
						cmd,
						rtab.rmnt_special, 
						mnt[i].mnt_mountp);
					chg++;
					break;
				}
			    	else if (strcmp(mnt[i].mnt_mountp,
						rtab.rmnt_mountp) == 0) {
					Printf( msg_ub,
						cmd,
						rtab.rmnt_mountp, 
						mnt[i].mnt_special);
					chg++;
					break;
				}
			}  /* end of for */

			if (i == mnt_count)	/* it's not in mnttab */
				/* try the mount */
				if (!trymount(&rtab))
					chg++;
				else {
				/* mark fail for proper return code */
					fails++;
					putrmntent(tmp, &rtab);
				}
		}
		else {	/* resource in rmnttab but not requested to mount */
			fails++;
			putrmntent(tmp, &rtab);
		}

	/* check for unused command line resources */
	for (ulist = &argv[1], i=1, j=0; i < argc; ++i)
		if (argv[i] && *argv[i])
			ulist[j++] = argv[i];
	if (j) {
		Fprintf(stderr, "%s: resources not queued:", cmd);
		/* if none of the resources are queued, return 1 */
		if ( (i-1) == j ) 
			fails++;
		for (i=0; i < j; ++i)
			Fprintf(stderr, " %s", ulist[i]);
		Fputc('\n', stderr);
	}

	/* if rmnttab requires no change, don't write it out */
	if (!chg) {
		Fclose(rp);
		Fclose(tmp);
		Unlink(TMP);
		unlock();
		return fails ? 1 : 0;
	}

	if (fails)
		fails = 1;	/* set proper return code */

	/* write rmnttab */

	Fclose(rp);
	Fclose(tmp);
	Rename(TMP, RMNTTAB);
	Chmod(RMNTTAB, MASK);
	Chown(RMNTTAB, stbuf.st_uid, stbuf.st_gid);
	unlock();
	return fails;
}


/*
	request - if resource is a command line argument, return 1
		and set the that command line argument to an  empty string.
		otherwise, return 0
*/

int
request(resource, av)
	char	*resource;
	char	**av;
{
	char	buf[MAXDNAME];

	while (*++av) {
		(void)fqn(*av, buf);
		if (strcmp(resource, buf) == 0) {
			**av = '\0';
			return 1;
		}
	}
	return 0;
}


/*
	rd_mnttab - read the mount table
	return:
		0 if error
		1 if correct
*/

int
rd_mnttab(mntp, mnt_countp)
	struct mnttab	**mntp;		/* pointer to program's mount table */  
	unsigned int	*mnt_countp;	/* pointer to number of entries */
{
	unsigned int 	i;
	char		*msg_a	= "%s: cannot allocate space for mnt array\n";
	char		*msg_s	= "%s: cannot allocate space for mnt_special\n";
	char		*msg_m	= "%s: cannot allocate space for mnt_mountp\n";
	struct mnttab	*mnt	= NULL;
	struct mnttab	mtab;
	FILE		*mp;		/* mnttab file pointer */
	
	if ((mp = fopen(MNTTAB, "r")) == NULL) {
		Fprintf(stderr, "%s: cannot open %s\n", cmd, MNTTAB);
		return 0;
	}

	for (i = 0; getmntent(mp, &mtab) == 0; i++) {

		if (!(mnt = (struct mnttab *)
		  realloc(mnt, sizeof(struct mnttab) * (i + 1)))) {
			Fprintf(stderr, msg_a, cmd);
			Fclose(mp);
			return 0;
		}

		if (!(mnt[i].mnt_special
			  = (char *)malloc(strlen(mtab.mnt_special) + 1))) {
			Fprintf(stderr, msg_s, cmd);
			Fclose(mp);
			return 0;
		}
		Strcpy(mnt[i].mnt_special, mtab.mnt_special);

		if (!(mnt[i].mnt_mountp
			  = (char *)malloc(strlen(mtab.mnt_mountp) + 1))) {
			Fprintf(stderr, msg_m, cmd);
			Fclose(mp);
			return 0;
		}
		Strcpy(mnt[i].mnt_mountp, mtab.mnt_mountp);
	}
	*mntp = mnt;
	*mnt_countp = i;
	Fclose(mp);
	return 1;
}
/*
	trymount - perform an /sbin/mount; return its exit status.

	TIMEOUT is the maximum time in seconds for /sbin/mount to complete
*/

/* trymount initializes for signal handler, killpid() */
static pid_t		pid;/* PID of forked /sbin/mount. */
static struct rmnttab	*rmnt4sig_p;
 
int
trymount(rp)
	struct rmnttab	*rp;
{
	int	status;
	pid_t	w;
	void	(*istat)();
	void	(*qstat)();
	void	(*astat)();

	/* validate level, if given */
	if(rp->rmnt_lvl) {
		level_t	level;

		if( lvlin(rp->rmnt_lvl, &level) == -1){
			if(errno == EINVAL){
				Fprintf(stderr, "%s: bad level: %s\n",
					cmd, rp->rmnt_lvl);
				return -1;
			} else {
				perror("lvlin");
				return -1;
			}
		}
	}

	/* initialize structure used by signal handler, killpid() */
	rmnt4sig_p = rp;

	astat = signal(SIGALRM, killpid);
	if ((pid = fork()) == (pid_t)0) {
		int fd;

		if ((fd = open("/dev/null", O_WRONLY)) < 0)
			Fprintf(stderr, "%s: Can't open /dev/null\n", cmd);
		else {
			Close(1); Fcntl(fd, F_DUPFD, 1);/* redirect stdout */
			Close(2); Fcntl(fd, F_DUPFD, 2);/* redirect stderr */
			Close(fd);

			if (ismac() && rp->rmnt_lvl) {
				(void) execl(	"/sbin/mount",
						"mount",
						"-F",	rp->rmnt_fstype,
						"-o",	rp->rmnt_mntopts,
						"-l",	rp->rmnt_lvl,
							rp->rmnt_special,
							rp->rmnt_mountp,
						(char *)0);
			} else { /* invoke without -l level option */
				(void) execl(	"/sbin/mount",
						"mount",
						"-F",	rp->rmnt_fstype,
						"-o",	rp->rmnt_mntopts,
							rp->rmnt_special,
							rp->rmnt_mountp,
						(char *)0);
			}
		}
		_exit(127);
	}
	else {
		(void)alarm(TIMEOUT);		/* set mount time limit */
		Signal(SIGALRM, killpid);
	}

	istat = signal(SIGINT,  SIG_IGN);
	qstat = signal(SIGQUIT, SIG_IGN);
	while((w = wait(&status)) != pid && w != (pid_t)-1)
		;
	(void)alarm(0);
	Signal(SIGALRM, astat);
	Signal(SIGINT,  istat);
	Signal(SIGQUIT, qstat);
	return w == -1 ? w : status;
}


/*
	killpid - kill the /sbin/mount command when it's been running too long
*/

void
killpid()
{
	char *msg =
		"%s: \"/sbin/mount -F %s -o %s %s %s\" timed out.\n";
	char *msgL =
		"%s: \"/sbin/mount -F %s -o %s -l %s %s %s\" timed out.\n";

	(void)kill(pid, SIGKILL);
	if (rmnt4sig_p->rmnt_lvl) {
		Fprintf(stderr, msg,  cmd,
				rmnt4sig_p->rmnt_fstype,
				rmnt4sig_p->rmnt_mntopts,
				rmnt4sig_p->rmnt_lvl,
				rmnt4sig_p->rmnt_special,
				rmnt4sig_p->rmnt_mountp);
	} else {
		Fprintf(stderr, msgL, cmd,
				rmnt4sig_p->rmnt_fstype,
				rmnt4sig_p->rmnt_mntopts,
				rmnt4sig_p->rmnt_special,
				rmnt4sig_p->rmnt_mountp);
	}
}
