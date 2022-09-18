/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ps:ps.c	1.62.38.5"
#ident  "$Header: ps.c 1.2 91/06/27 $"

/***************************************************************************
 * Command: ps
 * Inheritable Privileges: P_FILESYS,P_MACREAD,P_SETPLEVEL,
 *                         P_OWNER,P_MACWRITE,P_DACWRITE
 *       Fixed Privileges: P_DACREAD
 *
 * Notes:  print things about processes.
 *
 * 
 *
 ***************************************************************************/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <ftw.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/mnttab.h>
#include <dirent.h>
#include <sys/signal.h>
#include <sys/fault.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/procfs.h>
#include <locale.h>
#include <sys/proc.h>
#include <mac.h>
#include <deflt.h>
#include <priv.h>
#include <pfmt.h>

#define	TRUE	1
#define	FALSE	0

#define DEFFILE "mac"	/* /etc/default file containing the default LTDB lvl */
#define LTDB_DEFAULT 2  /* default LID for non-readable LTDB */

#define NTTYS	20	/* max ttys that can be specified with the -t option  */
#define SIZ	30	/* max processes that can be specified with -p and -g */
#define ARGSIZ	30	/* size of buffer holding args for -t, -p, -u options */

#ifndef MAXLOGIN
#define MAXLOGIN 8	/* max number of chars in login that will be printed */
#endif

/* Structure for storing user info */
struct udata {
	uid_t	uid;		/* numeric user id */
	char	name[MAXLOGIN];	/* login name, may not be null terminated */
};

/* udata and devl granularity for structure allocation */
#define UDQ	50

/* Pointer to user data */
struct udata *ud;
int	nud = 0;	/* number of valid ud structures */
int	maxud = 0;	/* number of ud's allocated */

struct udata uid_tbl[SIZ];	/* table to store selected uid's */
int	nut = 0;		/* counter for uid_tbl */

struct prpsinfo info;	/* process information structure from /proc */

int	retcode = 1;
int	lflg;
int	eflg;
int	uflg;
int	aflg;
int	dflg;
int	pflg;
int	fflg;
int	cflg;
int	jflg;
int	gflg;
int	sflg;
int	tflg;
int	zflg;
int	Zflg;
int	errflg;
int	isatty();
char	*gettty();
char	*ttyname();
char	argbuf[ARGSIZ];
char	*parg;
char	*p1;			/* points to successive option arguments */
static char	stdbuf[BUFSIZ];

static const char
	badnonnum[] = ":361:%s is an invalid non-numeric argument for the -%c option\n",
	tellsys[] = ":133:Consult your system administrator\n",
	badlvlproc[] = ":651:lvlproc() set lid_user failed\n",
	errmsg[] = ":37:%s: %s\n",
	abortlp[] = ":652:Abort to enforce least privilege\n";

/*
 * print level information
 *	-1: don't print level
 *	LVL_ALIAS: print alias name, if it exists
 *	LVL_FULL: print fully qualified level name
 */
level_t	pr_lid;
int	lvlformat;
#define	SHOWLVL()	(lvlformat != -1)

level_t curr_lvl;	/* level of ls process when exec'ed by user */
level_t ltdb_lvl;	/* level of the LTDB */

/*
 * /etc/ps_data stores information for quick access by ps
 * and whodo.  To avoid synchronization problems when
 * reading and writing this file, a temporary file is
 * created at the level of /etc, ownership and group set to known
 * ids, information stored in the temp file, and finally
 * the temp file is renamed to /etc/ps_data.
 * This whole procedure requires P_SETPLEVEL privilege to
 * change the process level to that of directory /etc,
 * P_OWNER (possibly) to change owner and group, and
 * P_DACWRITE to write to directory /etc.
 * These privileges are inheritable, which means that if
 * ps is to be fast, a process with these privileges needs
 * to create the ps_data file first.  Note also that
 * update of ps_data depends on /etc/passwd and /dev.
 * Therefore, ps should be run with these privileges
 * every so often to assure that ps_data is around.
 * An entry is added to the root crontab to execute ps -a
 * every half hour.
 *
 * The only fixed privilege is P_DACREAD which allows
 * reading of processes that are not owned by the caller.
 * Setuid is no longer necessary, even in the base.
 *
 */

int	ndev;			/* number of devices */
int	maxdev;			/* number of devl structures allocated */

#define DNSIZE	14
struct devl {			/* device list	 */
	char	dname[DNSIZE];	/* device name	 */
	dev_t	dev;		/* device number */
} *devl;

char	*tty[NTTYS];	/* for t option */
int	ntty = 0;
pid_t	pid[SIZ];	/* for p option */
int	npid = 0;
pid_t	grpid[SIZ];	/* for g option */
int	ngrpid = 0;
pid_t	sessid[SIZ];	/* for s option */
int	nsessid = 0;

char	*rname  = NULL;		/* remote system name */
char	*rroot  = NULL;		/* remote system root path */
char	*procdir = "/proc";	/* standard /proc directory */
int	rd_only = 0;		/* flag for remote filesystem read-only */
char	*rpath();		/* determines root directory for remote */
void	usage();		/* print usage message and quit */

/*
 * Procedure:     main
 *
 * Restrictions:
                 setlocale: None
                 pfmt: None
                 setbuf: None
                 getopt: None
                 strerror: None
                 isatty: None
                 ttyname: None
                 chroot(2): None
                 chdir(2): None
                 lvlfile(2): None
                 printf: None
                 opendir: None
                 open(2): None
                 ioctl(2): None
*/
main(argc, argv)
	int	argc;
	char	**argv;
{
	register char	**ttyp = tty;
	char	*name;
	char	*p;
	int	c;
	int	ret = 0;
	uid_t	puid;		/* puid: process user id */
	pid_t	ppid;		/* ppid: parent process id */
	pid_t	ppgrp;		/* ppgrp: process group id */
	pid_t	psid;		/* psid: session id */
	int	i, found;
	extern char	*optarg;
	extern int	optind;

	extern level_t  curr_lvl;
	extern level_t  ltdb_lvl;

	int	pgerrflg = 0;	/* err flg: non-numeric arg w/p & g options */
	void	getdev();

	unsigned	size;

	DIR *dirp;
	struct dirent *dentp;
	char	pname[100];
	int	pdlen;
	level_t	tmp_lid;	/* temporary LID place holder */

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore");
	(void)setlabel("UX:ps");


	setbuf(stdout, stdbuf);
	while ((c = getopt(argc, argv, "Zzjlfceadt:p:g:u:r:n:s:")) != EOF)
		switch (c) {
		case 'l':		/* long listing */
			lflg++;
			break;
		case 'f':		/* full listing */
			fflg++;
			break;
		case 'j':
			jflg++;
			break;
		case 'c':
			/*
			 * Format output to reflect scheduler changes:
			 * high numbers for high priorities and don't
			 * print nice or p_cpu values.  'c' option only
			 * effective when used with 'l' or 'f' options.
			 */
			cflg++;
			break;
		case 'e':		/* list for every process */
			eflg++;
			tflg = uflg = pflg = gflg = sflg = 0;
			break;
		case 'a':
			/*
			 * Same as 'e' except no process group leaders
			 * and no non-terminal processes.
			 */
			aflg++;
			break;
		case 'd':	/* same as e except no proc grp leaders */
			dflg++;
			break;
		case 'n':	/* no longer needed; retain as no-op */
			pfmt(stderr, MM_WARNING, ":362:-n option ignored\n");
			break;
		case 't':		/* terminals */
#define TSZ 30
			tflg++;
			p1 = optarg;
			do {
				parg = argbuf;
				if (ntty >= NTTYS)
					break;
				getarg();
				if ((p = (char *)malloc(TSZ)) == NULL) {
					pfmt(stderr, MM_ERROR,
						":31:Out of memory: %s\n",
						strerror(errno));
					exit(1);
				}
				size = TSZ;
				if (isdigit(*parg)) {
					(void) strcpy(p, "tty");
					size -= 3;
				}
				(void) strncat(p, parg, (int)size);
				*ttyp++ = p;
				ntty++;
			} while (*p1);
			break;
		case 'p':		/* proc ids */
			pflg++;
			p1 = optarg;
			parg = argbuf;
			do {
				if (npid >= SIZ)
					break;
				getarg();
				if (!num(parg)) {
					pgerrflg++;
					pfmt(stderr, MM_ERROR, badnonnum, parg, 'p');
				}
				pid[npid++] = (pid_t)atol(parg);
			} while (*p1);
			break;
		case 's':		/* session */
			sflg++;
			p1 = optarg;
			parg = argbuf;
			do {
				if (nsessid >= SIZ)
					break;
				getarg();
				if (!num(parg)) {
					pgerrflg++;
					pfmt(stderr, MM_ERROR, badnonnum, parg, 's');
				}
				sessid[nsessid++] = (pid_t)atol(parg);
			} while (*p1);
			break;
		case 'g':		/* proc group */
			gflg++;
			p1 = optarg;
			parg = argbuf;
			do {
				if (ngrpid >= SIZ)
					break;
				getarg();
				if (!num(parg)) {
					pgerrflg++;
					pfmt(stderr, MM_ERROR, badnonnum, parg, 'g');
				}
				grpid[ngrpid++] = (pid_t)atol(parg);
			} while (*p1);
			break;
		case 'u':		/* user name or number */
			uflg++;
			p1 = optarg;
			parg = argbuf;
			do {
				getarg();
				if (nut < SIZ)
					(void) strncpy(uid_tbl[nut++].name,
					  parg, MAXLOGIN);
			} while (*p1);
			break;
		case 'r':		/* remote system name or root path */
			rname = optarg;
			break;
		case 'z':
			zflg++;
			break;
		case 'Z':
			Zflg++;
			break;
		default:			/* error on ? */
			errflg++;
			break;
		}

	if (errflg || optind < argc || pgerrflg){
		if (optind < argc)
			pfmt(stderr, MM_ERROR, ":1:Incorrect usage\n");
		usage();
	}

	/*
	 * The z and Z options are mutually exclusive.
	 */
	if (zflg && Zflg) {
		pfmt(stderr, MM_ERROR, ":653:invalid combination of options -%c and -%c.\n",
			'Z', 'z');
		usage();
	}

	if (zflg)
		lvlformat = LVL_ALIAS;
	else if (Zflg)
		lvlformat = LVL_FULL;
	else
		lvlformat = -1;

	if (tflg)
		*ttyp = 0;
	/*
	 * If an appropriate option has not been specified, use the
	 * current terminal as the default.
	 */
	if (!(aflg || eflg || dflg || uflg || tflg || pflg || gflg || sflg)) {
		if (rname != NULL) {
			pfmt(stderr, MM_ERROR,
			  ":363:One of -esdatpug must be used with -r sysname\n");
			usage();
		}
		name = NULL;
		for (i = 2; i >= 0; i--)
			if (isatty(i)) {
				name = ttyname(i);
				break;
			}
		if (name == NULL) {
			pfmt(stderr, MM_ERROR,
			  ":364:Cannot find controlling terminal\n");
			exit(1);
		}
		*ttyp++ = name + 5;
		*ttyp = 0;
		ntty++;
		tflg++;
	}
	if (eflg)
		tflg = uflg = pflg = sflg = gflg = aflg = dflg = 0;
	if (aflg || dflg)
		tflg = 0;

	/*
	 * Determine root path for remote machine.
	 */
	if (rname != NULL) {
		if ((rroot = rpath(rname)) == NULL)
			exit(1);
		/*
		 * Move over to remote machine.
		 */
		if (chroot(rroot) != 0 || chdir("/") != 0) {
			pfmt(stderr, MM_ERROR,
				":365:Cannot change root to %s\n", rroot);
			exit(1);
		}

	}

	if (!readata()) {	/* get data from psfile */
		getdev();
		getpasswd();
		wrdata();
	}

	uconv();


	/*
	 * establish up front if the enhanced security package was installed 
         * the routine read_ltdb will:
	 *
	 * - Check to make sure MAC is installed
	 * - Attempt to read the LTDB at the current process level
	 * - If unsuccessful, change level to the level of the LTDB abd
	 *   attempt to read the LTDB again.
	 *   If successful, read_ltdb will return 0 (LTLD is readable at the
         *     current process level, > 0 (also the level of the LTDB) 
         *     indicating the LTDB is at a level not dominated by the process
         *     at it's default level BUT the process was able to change level
         *     to the level of the LTDB and read it. A return of -1 indicates
         *     the LTDB was unreadable or MAC is not installed.
	 */

	if (SHOWLVL()) {
		if ((ret = read_ltdb ()) < 0) 
		{
			pfmt(stderr, MM_ERROR, "uxlibc:1:Illegal option -- %c\n",
				zflg ? 'z' : 'Z');
			pfmt(stderr, MM_ERROR, ":593:System service not installed\n");
			usage();
		} else
			ltdb_lvl = ret;
	}

	if (lflg)
		pfmt(stdout, MM_NOSTD, ":366: F S");
	if (fflg) {
		if (lflg)
			printf(" ");
		pfmt(stdout, MM_NOSTD, ":367:     UID");
	} else if (lflg)
		pfmt(stdout, MM_NOSTD, ":368:   UID");
	pfmt(stdout, MM_NOSTD, ":369:   PID");
	if (lflg || fflg)
		pfmt(stdout, MM_NOSTD, ":370:  PPID");
	if (jflg)
		pfmt(stdout, MM_NOSTD, ":371:  PGID   SID");
	if (cflg)
		pfmt(stdout, MM_NOSTD, ":372:  CLS PRI");
	else if (lflg || fflg) {
		pfmt(stdout, MM_NOSTD, ":373:  C");
		if (lflg)
			pfmt(stdout, MM_NOSTD, ":374: PRI NI");
	}
	if (lflg)
		pfmt(stdout, MM_NOSTD, ":375:     ADDR     SZ    WCHAN");
	if (fflg)
		pfmt(stdout, MM_NOSTD, ":376:    STIME");
	pfmt(stdout, MM_NOSTD, ":654: TTY      TIME COMD");
	if (SHOWLVL())
		pfmt(stdout, MM_NOSTD, ":655:      LEVEL");
	printf("\n");


	/*
	 * Determine which processes to print info about by searching
	 * the /proc directory and looking at each process.
	 */
	if ((dirp = opendir(procdir)) == NULL) {
		(void) pfmt(stderr, MM_ERROR,
			":378:Cannot open PROC directory %s%s: %s\n",
		  rroot ? rroot : "", procdir, strerror(errno));
		exit(1);
	}

	(void) strcpy(pname, procdir);
	pdlen = strlen(pname);
	pname[pdlen++] = '/';

	/* for each active process --- */
	while (dentp = readdir(dirp)) {
		int	procfd;		/* filedescriptor for /proc/nnnnn */

		if (dentp->d_name[0] == '.')		/* skip . and .. */
			continue;
		(void) strcpy(pname + pdlen, dentp->d_name);
retry:
		if ((procfd = open(pname, O_RDONLY)) == -1)
			continue;

		/*
		 * Get the info structure for the process and close quickly.
		 */
		if (ioctl(procfd, PIOCPSINFO, (char *) &info) == -1) {
			int	saverr = errno;

			(void) close(procfd);
			if (saverr == EACCES)
				continue;
			if (saverr == EAGAIN)
				goto retry;
			if (saverr != ENOENT)
				(void) pfmt(stderr, MM_ERROR, 
				  ":379:PIOCPSINFO on %s: %s\n",
				  pname, strerror(saverr));
			continue;
		}

		/*
		 * Get the level of a process.  If this fails, no
		 * level information is displayed.
		 */
		if (SHOWLVL()) {
			if (info.pr_state == SZOMB || info.pr_state == SIDL
			||  lvlfile(pname, MAC_GET, &pr_lid) == -1)
				pr_lid = 0; /* ingnore printing of level */
		}

		(void) close(procfd);

		found = 0;
		if (info.pr_state == 0)		/* can't happen? */
			continue;
		puid = info.pr_uid;
		ppid = info.pr_pid;
		ppgrp = info.pr_pgrp;
		psid = info.pr_sid;

		/*
		 * Omit process group leaders for 'a' and 'd' options.
		 */
		if ((ppid == psid) && (dflg || aflg))
			continue;
		if (eflg || dflg)
			found++;
		else if (pflg && search(pid, npid, ppid))
			found++;	/* ppid in p option arg list */
		else if (uflg && ufind(puid))
			found++;	/* puid in u option arg list */
		else if (gflg && search(grpid, ngrpid, ppgrp))
			found++;	/* grpid in g option arg list */
		else if (sflg && search(sessid, nsessid, psid))
			found++;	/* sessid in s option arg list */
		if (!found && !tflg && !aflg )
			continue;
		if (prcom(puid, found)) {
			printf("\n");
			retcode = 0;
		}
	}

	(void) closedir(dirp);
	exit(retcode);
	/* NOTREACHED */
}

/*
 * Procedure:     read_ltdb
 *
 * Restrictions:
 *		lvlin: none
 *		lvlproc: none
 *		defopen: none
 *
 *
 * Inputs: None
 * Returns: 0 on success w/o changing levels, > 0 if the LTDB is at a level
 *          which is not dominated by this process, but was readable after
 *          lvlproc () call and -1 on failure. 
 *          Levels of LTDB and the current process
 *	    are set here. These are used later when the per-file LIDs are
 *	    translated into text levels.
 *
 *         
 *
 * Notes: Attempt to read the LTDB at the current process level, if the
 *        call to lvlin fails, get the level of the LTDB (via defread),
 *        change the process level to that of the LTDB and try again.
 *   
 *        If the lvlin or lvlproc calls fail we assume MAC is not installed
 *        and exit with a nonzero status. If the call succeeds return the
 *        level of the LTDB for later use.
 *
 */
read_ltdb ()
{
	FILE    *def_fp;
	static char *ltdb_deflvl;
	level_t test_lid, ltdb_lid;

/*
	Check to see if MAC is installed. If lvlproc fails with ENOPKG
	MAC is not installed ...
*/

	if ((lvlproc(MAC_GET, &test_lid) != 0) && (errno == ENOPKG)) 
		return (-1);

/*
 *	If lvlin is successful the LTDB is readable at the current process
 *	level, no other processing is required, return 0....
*/

	if (lvlin ("SYS_PRIVATE", &test_lid) == 0)  
		return (0);
/*
 *	If the LTDB was unreadable:
 *	- Get the LTDB's level (via defread)
 *	- Get the current level of the process
 *	- Set p_setplevel privilege
 *	- Change process level to the LTDB's level
 *	- Try lvlin again, read of LTDB should succeed since process level
 *        is equal to level of the LTDB
 *
 *	Failure to obtain the process level or read the LTDB is a fatal
 *      error, return -1.
 */


	if ((def_fp = defopen(DEFFILE)) != NULL) {
		if (ltdb_deflvl = defread(def_fp, "LTDB"))
			ltdb_lid = (level_t) atol (ltdb_deflvl);
		(void) defclose(NULL);
	}
	
	if ((def_fp == NULL) || !(ltdb_deflvl))
		ltdb_lid = (level_t) LTDB_DEFAULT;


	if (ltdb_lid <= 0)
		return (-1);

	if (lvlproc (MAC_GET, &curr_lvl) != 0)
		return (-1);

	lvlproc (MAC_SET, &ltdb_lid);


	if (lvlin ("SYS_PRIVATE", &test_lid) != 0) 
		return (-1);


	if (lvlproc (MAC_SET, &curr_lvl) != 0)
		return (-1);


	return (ltdb_lid);

}
/*
 * Procedure:     usage
 *
 * Restrictions:
                 pfmt: None
                 gettxt: None
*/

/* print usage message and quit */
static void	usage()		
{
	static char usage1[] = "ps [ -edalfcj[Z|z] ] [ -r sysname ] [ -t termlist ]";
	static char usage2[] = "   [ -u uidlist ] [ -p proclist ] [ -g grplist ] [ -s sidlist ]";
	static char usage1id[] = ":656";
	static char usage2id[] = ":381";

	pfmt(stderr, MM_ACTION, ":382:Usage:\n\t%s\n\t%s\n",
		gettxt(usage1id, usage1), gettxt(usage2id, usage2));
	exit(1);
}

/*
 * Procedure:     readata
 *
 * Restrictions:
 *               open(2): MACREAD
 *               stat(2): None
 *               pfmt: None
 *               strerror: None
 * Notes:
 *
 * readata reads in the open devices (terminals) and stores 
 * info in the devl structure.
 */
static char	psfile[] = "/etc/ps_data";
static char	pstmpfile[] = "/etc/ps_XXXXXX";

int readata()
{
	struct stat sbuf1, sbuf2;
	int fd;
	struct flock wflock;

/* Ascertain ps_data is at SYS_PUBLIC */

	procprivl(CLRPRV,pm_work(P_MACREAD),(priv_t)0);

	fd = open(psfile, O_RDONLY);

	procprivl(SETPRV,pm_work(P_MACREAD),(priv_t)0);

	if(fd == -1)		/* Restore privs before returning error.*/
		return(0);

	if (fstat(fd, &sbuf1) < 0
	  || sbuf1.st_size == 0
	  || stat("/dev", &sbuf2) == -1
	  || sbuf1.st_mtime <= sbuf2.st_mtime
	  || sbuf1.st_mtime <= sbuf2.st_ctime
	  || stat("/dev/tp", &sbuf2) == -1
	  || sbuf1.st_mtime <= sbuf2.st_mtime
	  || sbuf1.st_mtime <= sbuf2.st_ctime
	  || stat("/etc/passwd", &sbuf2) == -1
	  || sbuf1.st_mtime <= sbuf2.st_mtime
	  || sbuf1.st_mtime <= sbuf2.st_ctime) {
		if (!rd_only) {		/* if read-only, believe old data */
			(void) close(fd);
			return 0;
		}
	}

	/* Read /dev data from psfile. */
	if (psread(fd, (char *) &ndev, sizeof(ndev)) == 0)  {
		(void) close(fd);
		return 0;
	}

	if ((devl = (struct devl *)malloc(ndev * sizeof(*devl))) == NULL) {
		pfmt(stderr, MM_ERROR,
			":383:Not enough memory for device table: %s\n",
			strerror(errno));
		exit(1);
	}
	if (psread(fd, (char *)devl, ndev * sizeof(*devl)) == 0)  {
		(void) close(fd);
		return 0;
	}

	/* Read /etc/passwd data from psfile. */
	if (psread(fd, (char *) &nud, sizeof(nud)) == 0)  {
		(void) close(fd);
		return 0;
	}
	if ((ud = (struct udata *)malloc(nud * sizeof(*ud))) == NULL) {
		pfmt(stderr, MM_ERROR,
			":384:Not enough memory for udata table: %s\n",
			strerror(errno));
		exit(1);
	}
	if (psread(fd, (char *)ud, nud * sizeof(*ud)) == 0)  {
		(void) close(fd);
		return 0;
	}

	(void) close(fd);
	return 1;
}

/*
 * Procedure:     getdev
 *
 * Restrictions:
 *               ftw: None
 *               pfmt: None
 *               strerror: None
 * Notes:  getdev() uses ftw() to pass pathnames under /dev to gdev()
 * along with a status buffer.
 */
void getdev()
{
	int	gdev();
	int	rcode;

	ndev = 0;
	rcode = ftw("/dev", gdev, 17);

	switch (rcode) {
	case 0:
		return;		/* successful return, devl populated */
	case 1:
		pfmt(stderr, MM_ERROR, ":385:ftw() encountered problem\n");
		break;
	case -1:
		pfmt(stderr, MM_ERROR, ":386:ftw() failed: %s\n", strerror(errno));
		break;
	default:
		pfmt(stderr, MM_ERROR, ":387:ftw() unexpected return, rcode=%d\n",
		 rcode);
		break;
	}
	exit(1);
}

/*
 * Procedure:     gdev
 *
 * Restrictions:
 *               pfmt: None
 *
 * gdev() puts device names and ID into the devl structure for character
 * special files in /dev.  The "/dev/" string is stripped from the name
 * and if the resulting pathname exceeds DNSIZE in length then the highest
 * level directory names are stripped until the pathname is DNSIZE or less.
 */
int gdev(objptr, statp, numb)
	char	*objptr;
	struct stat *statp;
	int	numb;
{
	register int	i;
	int	leng, start;
	static struct devl ldevl[2];
	static int	lndev, consflg;

	switch (numb) {

	case FTW_F:	
		if ((statp->st_mode & S_IFMT) == S_IFCHR) {
			/* Get more and be ready for syscon & systty. */
			while (ndev + lndev >= maxdev) {
				maxdev += UDQ;
				devl = (struct devl *) ((devl == NULL) ? 
				  malloc(sizeof(struct devl ) * maxdev) : 
				  realloc(devl, sizeof(struct devl ) * maxdev));
				if (devl == NULL) {
					pfmt(stderr, MM_ERROR,
					    ":388:Not enough memory for %d devices\n",
					    maxdev);
					exit(1);
				}
			}
			/*
			 * Save systty & syscon entries if the console
			 * entry hasn't been seen.
			 */
			if (!consflg
			  && (strcmp("/dev/systty", objptr) == 0
			    || strcmp("/dev/syscon", objptr) == 0)) {
				(void) strncpy(ldevl[lndev].dname,
				  &objptr[5], DNSIZE);
				ldevl[lndev].dev = statp->st_rdev;
				lndev++;
				return 0;
			}

			leng = strlen(objptr);
			/* Strip off /dev/ */
			if (leng < DNSIZE + 4)
				(void) strcpy(devl[ndev].dname, &objptr[5]);
			else {
				start = leng - DNSIZE - 1;

				for (i = start; i < leng && (objptr[i] != '/');
				  i++)
					;
				if (i == leng )
					(void) strncpy(devl[ndev].dname,
					  &objptr[start], DNSIZE);
				else
					(void) strncpy(devl[ndev].dname,
					  &objptr[i+1], DNSIZE);
			}
			devl[ndev].dev = statp->st_rdev;
			ndev++;
			/*
			 * Put systty & syscon entries in devl when console
			 * is found.
			 */
			if (strcmp("/dev/console", objptr) == 0) {
				consflg++;
				for (i = 0; i < lndev; i++) {
					(void) strncpy(devl[ndev].dname,
					  ldevl[i].dname, DNSIZE);
					devl[ndev].dev = ldevl[i].dev;
					ndev++;
				}
				lndev = 0;
			}
		}
		return 0;

	case FTW_D:
	case FTW_DNR:
	case FTW_NS:
		return 0;

	default:
		pfmt(stderr, MM_ERROR, 
			":389:gdev() error, %d, encountered\n", numb);
		return 1;
	}
}

/*
 * Procedure:     getpasswd
 *
 * Restrictions:
                 getpwent: none
                 endpwent: None
                 pfmt: None
 * Notes
 * Get the passwd file data into the ud structure.
 */
getpasswd()
{
	struct passwd *pw, *getpwent();
	void	endpwent();

	ud = NULL;
	nud = 0;
	maxud = 0;

	while ((pw = getpwent()) != NULL) {
		while (nud >= maxud) {
			maxud += UDQ;
			ud = (struct udata *) ((ud == NULL) ? 
			  malloc(sizeof(struct udata ) * maxud) : 
			  realloc(ud, sizeof(struct udata ) * maxud));
			if (ud == NULL) {
				pfmt(stderr, MM_ERROR,
				  ":390:Not enough memory for %d users\n", maxud);
				exit(1);
			}
		}
		/*
		 * Copy fields from pw file structure to udata.
		 */
		ud[nud].uid = pw->pw_uid;
		(void) strncpy(ud[nud].name, pw->pw_name, MAXLOGIN);
		nud++;
	}
	endpwent();
}

/*
 * This macro is used in wrdata() to restore the original user
 * security label. 
 * It is a macro so that the code needs not be duplicated in various
 * places.
 */
#define	RESTORE_SECSTATE(lidp) { \
	if (mac) { \
		if (lvlchanged && lvlproc(MAC_SET, lidp) == -1) { \
			pfmt(stderr, MM_ERROR, badlvlproc); \
			pfmt(stderr, MM_ERROR, errmsg, "/etc/ps_data", strerror(errno)); \
			pfmt(stderr, MM_ACTION, tellsys); \
			exit(1); \
		} \
	}} 

/*
 * Procedure:     wrdata
 *
 * Restrictions:
                 lvlproc(2): None
                 pfmt: None
                 strerror: None
                 lvlfile(2): None
                 mktemp: None
                 open(2): None
                 chown(2): None
                 rename(2): None
                 unlink(2): None
*/
wrdata()
{
	int	fd;
	struct flock wflock;
	level_t	lid_user, lid_file;
	int	mac = 1;		/* mac installed */
	int	lvlchanged;

	(void) umask(02);

	if (lvlproc(MAC_GET, &lid_user) == -1) {
		if (errno == ENOPKG)	/* mac is not installed */
			mac = 0;
		else {			/* problem with MAC */
			pfmt(stderr, MM_ERROR, badlvlproc);
			pfmt(stderr, MM_ERROR, errmsg, "/etc/ps_data", strerror(errno));
			pfmt(stderr, MM_ACTION, tellsys);
			return;
		}
	/* will set level of file to level of directory it resides in */
	} else if (lvlfile("/etc", MAC_GET, &lid_file) == -1) {
		pfmt(stderr, MM_ERROR, ":657:lvlfile() get lid of /etc failed\n");
		pfmt(stderr, MM_ERROR, errmsg, "/etc/ps_data", strerror(errno));
		pfmt(stderr, MM_ACTION, tellsys);
		return;
	}
		
	lvlchanged = (mac && (lvlproc(MAC_SET, &lid_file) == -1))?0:1;

	if (mktemp(pstmpfile) == (char *)NULL ||
	    pstmpfile[0] == '\0') {
		pfmt(stderr, MM_ERROR, ":658:mktemp() failed\n");
		pfmt(stderr, MM_ERROR, errmsg, "/etc/ps_data", strerror(errno));
		pfmt(stderr, MM_ACTION, tellsys);
		RESTORE_SECSTATE(&lid_user);
		return;
	}
		
	if ((fd = open(pstmpfile, O_WRONLY|O_CREAT, 0664)) == -1) {
		/* only privileged user can write ps_data file */
		RESTORE_SECSTATE(&lid_user);
		return;
	}

	/*
	 * Make owner root, group sys.
	 * This is for compatibility.
	 */
	(void)chown(pstmpfile, (uid_t)0, (gid_t)3);

	/* write /dev data */
	pswrite(fd, (char *) &ndev, sizeof(ndev));
	pswrite(fd, (char *)devl, ndev * sizeof(*devl));

	/* write /etc/passwd data */
	pswrite(fd, (char *) &nud, sizeof(nud));
	pswrite(fd, (char *)ud, nud * sizeof(*ud));

	(void) close(fd);
	if (rename(pstmpfile, psfile) == -1) {
		pfmt(stderr, MM_ERROR, ":659:rename() failed\n");
		pfmt(stderr, MM_ERROR, errmsg, "/etc/ps_data", strerror(errno));
		pfmt(stderr, MM_ACTION, tellsys);
		(void)unlink(pstmpfile);
	}

	RESTORE_SECSTATE(&lid_user);
}

/*
 * getarg() finds the next argument in list and copies arg into argbuf.
 * p1 first pts to arg passed back from getopt routine.  p1 is then
 * bumped to next character that is not a comma or blank -- p1 NULL
 * indicates end of list.
 */

getarg()
{
	char	*parga;

	parga = argbuf;
	while (*p1 && *p1 != ',' && *p1 != ' ')
		*parga++ = *p1++;
	*parga = '\0';

	while (*p1 && (*p1 == ',' || *p1 == ' '))
		p1++;
}

/*
 * gettty returns the user's tty number or ? if none.
 */
char *gettty(ip)
	register int	*ip;	/* where the search left off last time */
{
	register int	i;

	if (info.pr_ttydev != PRNODEV && *ip >= 0) {
		for (i = *ip; i < ndev; i++) {
			if (devl[i].dev == info.pr_ttydev) {
				*ip = i + 1;
				return devl[i].dname;
			}
		}
	}
	*ip = -1;
	return "?";
}

/*
 * Procedure:     prcom
 *
 * Restrictions:
                 lvlout: MACREAD
                 lvlproc: none
 *
 * Notes:
 * Print info about the process.
 */
prcom(puid, found)
	uid_t	puid;
	int	found;
{
	register char	*cp;
	register char	*tp;
	long	tm;
	int	i, wcnt, length;
	wchar_t	wchar;
	register char	**ttyp, *str;

	/*
	 * If process is zombie, call print routine and return.
	 */
	if (info.pr_zomb) {
		if (tflg && !found)
			return 0;
		else {
			przom(puid);
			return 1;
		}
	}

	/*
	 * Get current terminal.  If none ("?") and 'a' is set, don't print
	 * info.  If 't' is set, check if term is in list of desired terminals
	 * and print it if it is.
	 */
	i = 0;
	tp = gettty(&i);
	if (aflg && *tp == '?')
		return 0;
	if (tflg && !found) {
		int match = 0;

		/*
		 * Look for same device under different names.
		 */
		while (i >= 0 && !match) {
			for (ttyp = tty; (str = *ttyp) != 0 && !match; ttyp++)
				if (strcmp(tp, str) == 0)
					match = 1;
			if (!match)
				tp = gettty(&i);
		}
		if (!match)
			return 0;
	}

	if (lflg) {
		printf("%2x %c", info.pr_flag & 0377, info.pr_sname);  /* F S */
		if (fflg)
			printf(" ");
	}
	if (fflg) {
		if ((i = getunam(puid)) >= 0)
			printf("%8.8s", ud[i].name);
		else
			printf("%8ld", puid);
	} else if (lflg)
		printf("%6ld", puid);
	printf("%6ld", info.pr_pid);				/* PID */
	if (lflg || fflg)
		printf("%6ld", info.pr_ppid);			/* PPID */
	if (jflg) {
		printf("%6ld", info.pr_pgrp);			/* PGID */
		printf("%6ld", info.pr_sid);			/* SID  */
	}
	if (cflg) {
		printf("%5s", info.pr_clname);			/* CLS  */
		printf("%4d", info.pr_pri);			/* PRI	*/
	} else if (lflg || fflg) {
		printf("%3d", info.pr_cpu & 0377);		/* C   */
		if (lflg) {
			/*
			 * Print priorities the old way (lower numbers
			 * mean higher priority) and print nice value
			 * for time sharing procs.
			 */
			printf("%4d", info.pr_oldpri);
			if (strcmp(info.pr_clname, "TS") == 0)
				printf("%3d", info.pr_nice);
			else
				printf(" %2.2s", info.pr_clname);
		}
	}
	if (lflg) {
		printf("%9x%7d", info.pr_addr, info.pr_size);	/* ADDR SZ */
		if (info.pr_wchan)
			printf("%9x", info.pr_wchan);		/* WCHAN */
		else
			printf("         ");
	}
	if (fflg)						/* STIME */
		prtime(info.pr_start);
	printf(" %-7.14s", tp);					/* TTY */
	tm = info.pr_time.tv_sec;
	if (info.pr_time.tv_nsec > 500000000)
		tm++;
	printf(" %2ld:%.2ld", tm / 60, tm % 60);

	if (!fflg) {						/* CMD */
		wcnt = namencnt(info.pr_fname, 16, 8);
		printf(" %.*s", wcnt, info.pr_fname);
		goto prlevel;
	}

	/*
	 * PRARGSZ == length of cmd arg string.
	 */
	for (cp = info.pr_psargs; cp < &info.pr_psargs[PRARGSZ]; ) {
		if (*cp == 0) 
			break;
		length = mbtowc(&wchar, cp, MB_LEN_MAX);
		if (length < 0 || !wisprint(wchar)) {
	        	printf(" [ %.8s ]", info.pr_fname);
			wcnt = 8;
			goto prlevel;
		}
		cp += length;
	}
	wcnt = namencnt(info.pr_psargs, PRARGSZ, lflg ? 35 : PRARGSZ);
	printf(" %.*s", wcnt, info.pr_psargs);
prlevel:

#define	INITBUFSIZ	512

	/*
	 * The zero LID is not a valid level identifier; it is used here
	 * to denote that the level for this entry should be ignored.
	 * This is useful for cases like zombies.
	 */
	if (SHOWLVL() && (pr_lid != (level_t)0)) {
		/*
		 * Pre-allocate buffer to store level name.  This should
		 * save a lvlout call in most cases.  Double size
		 * dynamically if level won't fit in buffer.
		 * Return 1 on error, i.e. success, because ps likes to
		 * return success if anything at all is printed.
		 */
		static char	lvl_buf[INITBUFSIZ];
		static char	*lvl_name = lvl_buf;
		static int	lvl_namesz = INITBUFSIZ;

		procprivl(CLRPRV,pm_work(P_MACREAD),(priv_t)0);

		/*
		 *
		 * if ltdb_lvl is > 0 then the LTDB is at a level which
                 *    is unreadable by this process at its current level,
                 *    change level to the level of the LTDB and issue lvlout
                 */

		if (ltdb_lvl > 0) {
			(void) lvlproc(MAC_SET, &ltdb_lvl);
		}

		while (lvlout(&pr_lid, lvl_name, lvl_namesz, lvlformat)
		      == -1) {
			if ((lvlformat == LVL_FULL) && (errno == ENOSPC)) {
				char *tmp_name;
				if ((tmp_name = malloc(lvl_namesz*2))
				==  (char *)NULL) {
					pfmt(stderr, MM_ERROR,
		":660:Not enough memory to print level for pid %u: %s\n",
						info.pr_pid, strerror(errno));
					procprivl(SETPRV,pm_work(P_MACREAD),(priv_t)0);

/*	Restore process to it's proper level & drop p_setplevel		*/

					if (ltdb_lvl > 0) {
						(void) lvlproc(MAC_SET, &curr_lvl);
					}
					return 1;
				}
				lvl_namesz *= 2;
				if (lvl_name != lvl_buf)
					free(lvl_name);
				lvl_name = tmp_name;
			} else {
				pfmt(stderr, MM_ERROR,
	":661:Cannot translate level to text format for pid %u\n",
					info.pr_pid);
				procprivl(SETPRV,pm_work(P_MACREAD),(priv_t)0);

/*	Restore process to it's proper level & drop p_setplevel		*/

				if (ltdb_lvl > 0) {
					(void) lvlproc(MAC_SET, &curr_lvl);
				}
				return 1;
			}
		}
		procprivl(SETPRV,pm_work(P_MACREAD),(priv_t)0);


/*	Restore process to it's proper level & drop p_setplevel		*/

		if (ltdb_lvl > 0) {
			(void) lvlproc(MAC_SET, &curr_lvl);
		}

		/* align if command name is less than 8 chars wide */
		while (wcnt < 8) {
			putc(' ', stdout);
			wcnt++;
		}
		printf("  %s", lvl_name);
	}
	return 1;
}

/*
 * Returns 1 if arg is found in array arr, of length num; 0 otherwise.
 */
int search(arr, number, arg)
	pid_t arr[];
	register int number;
	register pid_t arg;
{
	register int i;

	for (i = 0; i < number; i++)
		if (arg == arr[i])
			return 1;
	return 0;
}

/*
 * Procedure:     uconv
 *
 * Restrictions:
 *               pfmt: None
 */

uconv()
{
	uid_t pwuid;
	int found, i, j;

	/*
	 * Search name array for oarg.
	 */
	for (i = 0; i < nut; i++) {
		found = -1;
		for (j = 0; j < nud; j++) {
			if (strncmp(uid_tbl[i].name, ud[j].name,
			  MAXLOGIN) == 0) {
				found = j;
				break;
			}
		}
		/*
		 * If not found and oarg is numeric, search number array.
		 */
		if (found < 0
		  && uid_tbl[i].name[0] >= '0'
		  && uid_tbl[i].name[0] <= '9') {
			pwuid = (uid_t)atol(uid_tbl[i].name);
			for (j = 0; j < nud; j++) {
				if (pwuid == ud[j].uid) {
					found = j;
					break;
				}
			}
		}

		/*
		 * If found, enter found index into tbl array.
		 */
		if (found != -1) {
			uid_tbl[i].uid = ud[found].uid;
			(void) strncpy(uid_tbl[i].name, ud[found].name,
			  MAXLOGIN);
		} else {
			pfmt(stderr, MM_ERROR,
			  ":393:Unknown user %s\n", uid_tbl[i].name);
			for (j = i + 1; j < nut; j++) {
				(void) strncpy(uid_tbl[j-1].name,
				  uid_tbl[j].name, MAXLOGIN);
			}
			nut--;
			if (nut <= 0) 
				exit(1);
			i--;
		}
	}
}

/*
 * For full command listing (-f flag) print user name instead of number.
 * Search table of userid numbers and if puid is found, return the
 * corresponding name.  Otherwise search /etc/passwd.
 */
int getunam(puid)
	register uid_t puid;
{
	register int i;

	for (i = 0; i < nud; i++)
		if (ud[i].uid == puid)
			return i;
	return -1;
}

/*
 * Return 1 if puid is in table, otherwise 0.
 */
int ufind(puid)
	register uid_t puid;
{
	register int i;

	for (i = 0; i < nut; i++)
		if (uid_tbl[i].uid == puid)
			return 1;
	return 0;
}

/*
 * Procedure:     psread
 *
 * Restrictions:
                 read(2): None
                 pfmt: None
                 unlink(2): None
 * Notes:
 * Special read; unlinks psfile on read error.
 */
int psread(fd, bp, bs)
	int fd;
	char *bp;
	unsigned int bs;
{
	int rbs;

	if ((rbs = read(fd, bp, bs)) != bs) {
		pfmt(stderr, MM_ERROR,
			":394:psread() error on read, rbs=%d, bs=%d\n",
			rbs, bs);
		(void) unlink(psfile);
		return 0;
	}
	return 1;
}

/*
 * Procedure:     pswrite
 *
 * Restrictions:
                 write(2): None
                 pfmt: None
                 unlink(2): None
 * Notes:
 * Special write; unlinks pstmpfile on write error.
 */
pswrite(fd, bp, bs)
int	fd;
char	*bp;
unsigned	bs;
{
	int	wbs;

	if ((wbs = write(fd, bp, bs)) != bs) {
		pfmt(stderr, MM_ERROR,
			":395:pswrite() error on write, wbs=%d, bs=%d\n",
			wbs, bs);
		(void) unlink(pstmpfile);
	}
}

/*
 * Procedure:     prtime
 *
 * Restrictions:
 *               cftime: MACREAD opens /usr/lib/locale/<language>/LC_TIME 
 *                       which should be at SYS_PUBLIC
 *               gettxt: None
 *               printf: None
 * Notes:
 * Print starting time of process unless process started more than 24 hours
 * ago, in which case the date is printed.
 */
prtime(st)
	timestruc_t st;
{
	char sttim[26];
	static time_t tim = 0L;
	time_t starttime;

	if (tim == 0L)
		tim = time((time_t *) 0);
	starttime = st.tv_sec;
	if (st.tv_nsec > 500000000)
		starttime++;
	
	procprivl(CLRPRV,pm_work(P_MACREAD),(priv_t)0);
	if (tim - starttime > 24*60*60) {
		cftime(sttim, gettxt(":396", "%b %d"), &starttime);
		sttim[7] = '\0';
	} else {
		cftime(sttim, gettxt(":397", "%H:%M:%S"), &starttime);
		sttim[8] = '\0';
	}
	procprivl(SETPRV,pm_work(P_MACREAD),(priv_t)0);
	printf("%9.9s", sttim);
}

/*
 * Procedure:     przom
 *
 * Restrictions:
                 printf: None
                 pfmt: None
*/
przom(puid)
	uid_t	puid;
{
	int	i;
	long	tm;

	if (lflg)
		printf("%2x %c", info.pr_flag & 0377, info.pr_sname);  /* F S */
	if (fflg) {
		if ((i = getunam(puid)) >= 0)
			printf("%8.8s", ud[i].name);
		else
			printf("%8ld", puid);
	} else if (lflg)
		printf("%6ld", puid);
	printf("%6ld", info.pr_pid);				/* PID */
	if (lflg || fflg)
		printf("%6ld", info.pr_ppid);			/* PPID */
	if (jflg) {
		printf("%6ld", info.pr_pgrp);			/* PGID */
		printf("%6ld", info.pr_sid);			/* SID  */
	}
	if (cflg) {
		printf("     ");		/* zombies have no class */
		printf("%4d   ", info.pr_pri);			/* PRI	*/
	} else if (lflg || fflg) {
		printf("%3d", info.pr_cpu & 0377);		/* C   */
		if (lflg)
			printf("%4d", info.pr_oldpri);
	}
	if (fflg)
		printf("         ");				/* STIME */
	if (lflg)
		printf("                            ");	/* NI ADDR SZ WCHAN */
	tm = info.pr_time.tv_sec;
	if (info.pr_time.tv_nsec > 500000000)
		tm++;
	printf("         %2ld:%.2ld", tm / 60, tm % 60); /* TTY TIME */
	pfmt(stdout, MM_NOSTD, ":398: <defunct>");
}

/*
 * Returns true iff string is all numeric.
 */
int num(s)
	register char	*s;
{
	register int c;

	if (s == NULL)
		return 0;
	c = *s;
	do {
		if (!isdigit(c))
			return 0;
	} while (c = *++s);
	return 1;
}

/*
 * Function to compute the number of printable bytes in a multibyte
 * command string ("internationalization").
 */
int namencnt(cmd, eucsize, scrsize)
	register char *cmd;
	int eucsize;
	int scrsize;
{
	register int eucwcnt = 0, scrwcnt = 0;
	register int neucsz, nscrsz;
	wchar_t  wchar;

	while (*cmd != '\0') {
		if ((neucsz = mbtowc(&wchar, cmd, MB_LEN_MAX)) < 0)
			return 8; /* default to use for illegal chars */
		if ((nscrsz = scrwidth(wchar)) == 0)
			return 8;
		if (eucwcnt + neucsz > eucsize || scrwcnt + nscrsz > scrsize)
			break;
		eucwcnt += neucsz;
		scrwcnt += nscrsz;
		cmd += neucsz;
	}
	return eucwcnt;
}

/*
 * Procedure:     rpath
 *
 * Restrictions:
                 fopen: MACREAD
                 pfmt: None
                 strerror: None
                 fread: None
                 fclose: None
*/
char *rpath(name)	/* given remote system name, return its root directory path */
	register char	*name;
{
	struct mnttab mnttab;
	register FILE *fp;
	register char	*dirp;
	register char	*path = NULL;
	int	status;

	if (strchr(name, '/'))	/* if user specified a pathname, use it */
		return name;

	/*
	 * Open the mount table.
	 */
	 procprivl(CLRPRV,pm_work(P_MACREAD),(priv_t)0);
	if ((fp = fopen(MNTTAB, "r")) == NULL) {
		pfmt(stderr, MM_ERROR, ":92:Cannot open %s: %s\n",
			MNTTAB, strerror(errno));
		 procprivl(SETPRV,pm_work(P_MACREAD),(priv_t)0);
		return NULL;
	}

	 procprivl(SETPRV,pm_work(P_MACREAD),(priv_t)0);
	/*
	 * Search for a remote filesystem with last component matching name.
	 */
	while ((status = getmntent(fp, &mnttab)) == NULL) {
		if ((strcmp(mnttab.mnt_fstype, "nfs")==0 || 
			strcmp(mnttab.mnt_fstype, "rfs")==0)	/* remote flag */
		  && (dirp = strrchr(mnttab.mnt_mountp, '/'))
		  && strcmp(++dirp, name) == 0) {
			dirp = mnttab.mnt_mountp;
			path = (char *)malloc((unsigned)(strlen(dirp) + strlen(procdir) + 2));
			if (path == NULL) {
				pfmt(stderr, MM_ERROR,
				  ":399:Not enough memory for remote pathname\n");
				exit(1);
			}
			(void) strcpy(path, dirp);
			(void) strcat(path, procdir);
			if (isprocdir(path)) {	/* found a /proc directory */
				if ( strstr(mnttab.mnt_mntopts, "rw") == NULL)
					rd_only = 1; /* read-only */
				(void) strcpy(path, dirp);
				break;
			}
			free(path);
			path = NULL;
		}
	}
	(void) fclose(fp);
	if (path == NULL)
		pfmt(stderr, MM_ERROR, ":400:Cannot find path to system %s\n", name);
	return path;
}

/*
 * Procedure:     isprocdir
 *
 * Restrictions:
 *               stat(2): None
 * Notes
 * Return true iff dir is a /proc directory.
 *
 * This works because of the fact that "/proc/0" and "/proc/00" are the
 * same file, namely process 0, and are not linked to each other.  Ugly.
 */
static int	isprocdir(dir)		/* return TRUE iff dir is a PROC directory */
	char	*dir;
{
	extern int stat();
	struct stat stat1;	/* dir/0  */
	struct stat stat2;	/* dir/00 */
	char	path[200];
	register char	*p;

	/*
	 * Make a copy of the directory name without trailing '/'s
	 */
	if (dir == NULL)
		(void) strcpy(path, ".");
	else {
		(void) strncpy(path, dir, (int)sizeof(path) - 4);
		path[sizeof(path)-4] = '\0';
		p = path + strlen(path);
		while (p > path && *--p == '/')
			*p = '\0';
		if (*path == '\0')
			(void) strcpy(path, ".");
	}

	/*
	 * Append "/0" to the directory path and stat() the file.
	 */
	p = path + strlen(path);
	*p++ = '/';
	*p++ = '0';
	*p = '\0';
	if (stat(path, &stat1) != 0)
		return FALSE;

	/*
	 * Append "/00" to the directory path and stat() the file.
	 */
	*p++ = '0';
	*p = '\0';
	if (stat(path, &stat2) != 0)
		return FALSE;

	/*
	 * See if we ended up with the same file.
	 */
	if (stat1.st_dev != stat2.st_dev
	  || stat1.st_ino   != stat2.st_ino
	  || stat1.st_mode  != stat2.st_mode
	  || stat1.st_nlink != stat2.st_nlink
	  || stat1.st_uid   != stat2.st_uid
	  || stat1.st_gid   != stat2.st_gid
	  || stat1.st_size  != stat2.st_size)
		return FALSE;

	/*
	 * Return TRUE iff we have a regular file with a single link.
	 */
	return (stat1.st_mode & S_IFMT) == S_IFREG && stat1.st_nlink == 1;
}
