/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oampkg:common/cmd/oampkg/pkgrm/main.c	1.11.12.6"
#ident  "$Header: $"

#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <pkgstrct.h>
#include <pkgdev.h>
#include <pkginfo.h>
#include <pkglocs.h>
#include <unistd.h>
#include <pwd.h>
#include "install.h"
#include "msgdefs.h"

extern int	optind, errno;
extern char	*optarg, *pkgdir;

extern void	exit(),
		progerr(),
		ptext(),
		echo(),
		trap(),
		setadmin(),
		quit();
extern char	*pkgparam(), 
		*getenv(),
		**gpkglist();
extern int	getopt(),
		chdir(),
		ckyorn(),
		devtype(),
		pkgmount(),
		pkgumount(),
		pkgexecv(),
		rrmdir(),
		presvr4();

#define	ASK_SETCONFIRM \
"Do you want to remove this set"

#define	ASK_PKGCONFIRM \
"Do you want to remove this package"

#define	MENUQUIT	0

struct admin
	adm;		/* holds info about installation admin */
struct pkgdev
	pkgdev;		/* holds info about the installation device */
int	reboot;		/* non-zero if reboot required after installation */
int	ireboot;	/* non-zero if immediate reboot required */
int	failflag;	/* non-zero if fatal error has occurred */
int	warnflag;	/* non-zero if non-fatal error has occurred */
int	intrflag;	/* non-zero if any pkg installation was interrupted */
int	admnflag;	/* non-zero if any pkg installation was interrupted */
int	nullflag;	/* non-zero if any pkg installation was interrupted */
int	nointeract;	/* non-zero if no interaction with user should occur */
int	setnointeract;	/* non-zero if no interaction with user should occur */
int	setflag = 0;
int	npkgs;		/* the number of packages yet to be installed */
int	rpkgs;
int	started;
char	*pkginst;	/* current package (source) instance to process */
char	*prog;		/* the basename of argv[0] */
char	*tmpdir;	/* location to place temporary files */

void	(*func)(), ckreturn();

static int	interrupted;
static char	*admnfile;	/* file to use for installation admin */

static int	
	doremove(), pkgremove();

static void
usage()
{
	(void) fprintf(stderr, "usage:\n");
	(void) fprintf(stderr, "\t%s [-a admin] [pkg [pkg ...]]\n", prog);
	(void) fprintf(stderr, "\t%s -n [-a admin] pkg [pkg ...]\n", prog);
	(void) fprintf(stderr, "\t%s -s spool [pkg [pkg ...]]\n", prog);
	exit(1);
}

main(argc,argv)
int	argc;
char	**argv;
{
	int	i, c, n;
	int	repeat;
	char	ans[INPBUF], 
		**pkglist,	/* points to array of packages */
		*device = 0;

	/*************** New definitions for set removal processing ***************/
	char 	buffer[BUFSIZ];		/* buffer holds line from setinfo file	  */
	char	*setdir;		/* directory where package info is stored */
	char	setinfo[PATH_MAX];	/* pathname to setinfo file		  */
	char	*setinst = NULL;	/* pkginst name of SIP			  */ 
	char	**spkg;			/* will point to set packages array	  */	
	char	tmpcp[BUFSIZ];		/* temporary place for set package member */
	FILE	*sfp;			/* pointer to setinfo file		  */
	int	duppkg = 0;		/* incramented for duplicate pkginst	  */ 
	int	nsetpkgs;		/* index thru packages in a set		  */
	int	inset;			/* index through set packages		  */
	int	setpkg;			/* flag signals SIP specified on cmd line */ 
	struct	pkginfo info;

	pid_t	uid;			/* invoking login real uid		  */
	struct	passwd	*pwentry;

	prog = strrchr(argv[0], '/');
	if(!prog++)
		prog = argv[0];

	/*
	 * Check if real uid invoking this command has privilege to access
	 * the directory where the system's contents file resides.  If it
	 * does, to ensure that all tasks that may be invoked from package
	 * scripts (i.e., postinstall) we set the uid for the remainder of
	 * the installation process to root.
	 */
	uid = getuid();
	pwentry = getpwuid(uid);
	if(access(PKGADM, W_OK) < 0) {
		progerr("Login <%s> may not remove software, %s", 
			 pwentry->pw_name, strerror(errno));
		quit(1);
	}
	if(setuid(0) < 0) {
		progerr("Call to setuid failed, %s", strerror(errno));
		quit(1);
	}

	while((c = getopt(argc, argv, "s:a:n?")) != EOF) {
		switch(c) {
		  case 's':
			device = optarg;
			break;

		  case 'a':
			admnfile = optarg;
			break;

		  case 'n':
			nointeract++;
			break;

		  default:
			usage();
		}
	}
	if(admnfile && device)
		usage();
	if(nointeract && (optind == argc))
		usage();

	func = signal(SIGINT, trap);
	if(func != SIG_DFL)
		(void) signal(SIGINT, func);
	(void) signal(SIGHUP, trap);

	tmpdir = getenv("TMPDIR");
	if(tmpdir == NULL)
		tmpdir = P_tmpdir;

	/*
	 * initialize installation admin parameters 
	 */
	if(device == NULL)
		setadmin(admnfile);

	if(devtype((device ? device : PKGLOC), &pkgdev) || pkgdev.dirname == NULL) {
		progerr("bad device <%s> specified", device);
		quit(1);
	}

	pkgdir = pkgdev.dirname;
	repeat = ((optind >= argc) && pkgdev.mount);

again:

	if(pkgdev.mount) {
		if(n = pkgmount(&pkgdev, NULL, 0, 0, 1))
			quit(n);
	}

	if(chdir(pkgdev.dirname)) {
		progerr(ERR_CHDIR, pkgdev.dirname);
		quit(99);
	}
	

	pkglist = gpkglist(pkgdev.dirname, &argv[optind], NULL);
	if(pkglist == NULL) {
		if(errno == ENOPKG) {
			/* check for existence of pre-SVR4 package */
			progerr(ERR_NOPKGS, pkgdev.dirname);
			quit(1);
		} else {
			switch(errno) {
			  case ESRCH:
				quit(1);
				break;

			  case EINTR:
				quit(3);
				break;

			  case MENUQUIT:
				quit(0);
				break;

			  default:
				quit(99);
			}
		}
	}

	interrupted = 0;
	setpkg = 0;
	for(npkgs=0; pkglist[npkgs]; npkgs++)
		;
	for(rpkgs=0; pkglist[rpkgs]; rpkgs++) {

		duppkg = 0;

		/* Determine if this pkginst is the name of a set.  If so,
		 * get the names of the packages making up the set and 
		 * verify that they are installed since not may be required.
		 */
		(void) pkginfo(&info, NULL);

		/* Get information on this package instance */
		(void) pkginfo(&info, pkglist[rpkgs], NULL, NULL);
		/*
		 * Check if the current package is a Set Installation Package (SIP).
		 * If so, call doremove() to only prompt for removal of the set and
		 * return directly rather than execing the pkgremove command.  This
		 * is accomplished by setting 'setflag' before calling doremove().
		 * If setflag is set, doremove() will return a '1' after prompting
		 * for removal.
		 */

		if(!strcmp(info.catg, "set")) {
			setpkg++;
			pkginst = setinst = strdup(pkglist[rpkgs]);
			setflag++;
			n = doremove();
			setflag--;
			switch(n) {
				case 0:
					/*
					 * We've removed a package successfully, let's
					 * go on to next one or, if none, exit.
					 */
					npkgs--;
					setpkg--;
					continue;
					break;
				case 1:
					/*
					 * We've just returned from doremove() invoked
					 * to only prompt for removal of a set.  Go on 
					 * to handle set members then the SIP itself.
					 */
					break;
				default:
					/*
					 * An error occurred in doremove(), handle it.
					 */
					ckreturn(n);
					break;
			}

			/* Clear memory usage by pkginfo */
			(void) pkginfo(&info, NULL, NULL, NULL);
			
			/* Get package names from setinfo file */
			setdir = PKGLOC;
			sprintf(setinfo, "%s/%s/%s", setdir, pkglist[rpkgs], SETINFO);
			if ((sfp = fopen(setinfo, "r")) == NULL) {
				progerr("could not open setinfo file for %s\n", pkglist[rpkgs]);
				return(2);
			}
			spkg = (char **)calloc(128, sizeof(char **));
			for(nsetpkgs = 0; fgets(buffer, BUFSIZ, sfp) != NULL; nsetpkgs++) {
				(void) strcpy(tmpcp, strtok(buffer, "	 "));
				/*
				 * Verify that this package from the set is installed,
				 * if not it reflects a non-required package from this
				 * set that was not installed -- skip it.
				 */
				if(pkginfo(&info, tmpcp, NULL, NULL )) {
					nsetpkgs--;
					continue;	/* go on to next package in set */
				}
				/* Clear memory usage by pkginfo */
				(void) pkginfo(&info, NULL, NULL, NULL);

				/*
				 * Verify that this package was not already specified
				 * directly on the command line up until the time that
				 * SIP was specified -- if so, ignore it.
				 */
				for (i=0; i < rpkgs; i++) {
					if (strcmp(pkglist[i], tmpcp) == 0) {
						nsetpkgs--;
						duppkg++;
						break;
					}
				}
				if (!duppkg)
					spkg[nsetpkgs] = strdup(tmpcp);
				else
					duppkg--;

			}

		} 
		else {
			/*
			 * Handle non-SIP packages only.
			 * This section handles non set installation packages
			 * and enhances pkgrm to detect when duplicate packages
			 * are specified on the command line for removal.  If
			 * one is encountered, pkgrm simply ignores it.
			 */
			for (i=0; i < rpkgs; i++) {
				if ((strcmp(pkglist[rpkgs], pkglist[i])) == 0) {
					duppkg++;
					break;
				}
			}
			if (!duppkg)
				pkginst = pkglist[rpkgs];
			else {
				/*
				 * If duplicate reset duplicate flag, decrement
				 * package count and skip to next package
				 * instance on the command line.
				 */
				duppkg--;
				npkgs--;
				continue;
			}
		}
		started = 0;
		if(ireboot) {
			ptext(stderr, MSG_SUSPEND, pkginst);
			continue;
		}
		if(interrupted) {
			if(npkgs == 1)
				echo(MSG_1MORETODO);
			else
				echo(MSG_MORETODO, npkgs);
			if(nointeract)
				quit(0);
			if(n = ckyorn(ans, NULL, NULL, NULL, ASK_CONTINUE))
				quit(n);
			if(ans[0] != 'y')
				quit(0);
		}
		interrupted = 0;

		/*
		 * If a SIP was detected then setpkg should have
		 * been set and the list of member packages in
		 * the set should be listed in spkg.
		 */
		nsetpkgs--;
		if (setpkg) {
			for(inset = nsetpkgs; inset >= 0; inset--) {
				pkginst = spkg[inset];
				started = 0;
				if(ireboot) {
					ptext(stderr, MSG_SUSPEND, pkginst);
					continue;
				}
				if(interrupted) {
					if(i == 1)
						echo(MSG_1MORETODO);
					else
						echo(MSG_MORETODO, npkgs + inset);
					if(nointeract)
						quit(0);
					if(n = ckyorn(ans, NULL, NULL, NULL, ASK_CONTINUE))
						quit(n);
					if(ans[0] != 'y')
						quit(0);
				}
				interrupted = 0;
				/*
				 * For set member package removal, if nointeract is not set
				 * we don't want the user to get prompted as to whether each
				 * set member package should be removed.  Instead, we want 
				 * each package from the set to be removed without confirmation.
				 */
				if(!nointeract)
					setnointeract = 1;
				ckreturn(doremove());
				setnointeract = 0;
			}
			setpkg--;
			pkginst = pkglist[rpkgs];
		}
		/*
		 * After all of its set member packages have been
		 * removed, prevent the prompt for the SIP itself.
		 */
		if(!nointeract && setinst && !strcmp(pkginst, setinst))
			setnointeract = 1;
		ckreturn(doremove());
		setnointeract = 0;
		npkgs--;
	}
	if(pkgdev.mount) {
		(void) chdir("/");
		if(pkgumount(&pkgdev)) {
			progerr("unable to unmount <%s>", 
				pkgdev.bdevice);
			quit(99);
		}
	}
	if(!ireboot && repeat)
		goto again;
	quit(0);
	/*NOTREACHED*/
}

static int
doremove()
{
	struct pkginfo info;
	char	ans[INPBUF];
	char	ask_confirm[INPBUF];
	int	n;

	info.pkginst = NULL;
	if(pkginfo(&info, pkginst, NULL, NULL)) {
		progerr("instance <%s> does not exist", pkginst);
		return(2);
	}

	if(!nointeract && !setnointeract) {
		if(setflag) {
			if(info.status == PI_SPOOLED)
				echo("\nThe following set is currently spooled:");
			else
				echo("\nThe following set is currently installed:");
		}
		else {
			if(info.status == PI_SPOOLED)
				echo("\nThe following package is currently spooled:");
			else
				echo("\nThe following package is currently installed:");
		}
		echo("   %-14.14s  %s", info.pkginst, info.name);
		if(info.arch || info.version) 
			(void) fprintf(stderr, "   %14.14s  ", "");
		if(info.arch)
			(void) fprintf(stderr, "(%s) ", info.arch);
		if(info.version)
			(void) fprintf(stderr, "%s", info.version);
		if(info.arch || info.version)
			(void) fprintf(stderr, "\n");

		if(setflag)
			(void) sprintf(ask_confirm, "%s", ASK_SETCONFIRM);
		else
			(void) sprintf(ask_confirm, "%s", ASK_PKGCONFIRM);

		if(n = ckyorn(ans, NULL, NULL, NULL, ask_confirm))
			quit(n);
		if(ans[0] == 'n')
			return(0);
	}

	if(info.status == PI_PRESVR4)
		return(presvr4(pkginst));

	if(info.status == PI_SPOOLED) {
		/* removal from a directory */
		echo("\nRemoving spooled package instance <%s>", pkginst);
		return(rrmdir(pkginst));
	}

	if(setflag)
		return(1);

	return(pkgremove());
}

/*
 *  function which checks the indicated return value
 *  and indicates disposition of installation
 */
void
ckreturn(retcode) 
int	retcode;
{

	switch(retcode) {
	  case  0:
	  case 10:
	  case 20:
		break; /* empty case */

	  case  1:
	  case 11:
	  case 21:
		failflag++;
		interrupted++;
		break;

	  case  2:
	  case 12:
	  case 22:
		warnflag++;
		interrupted++;
		break;

	  case  3:
	  case 13:
	  case 23:
		intrflag++;
		interrupted++;
		break;

	  case  4:
	  case 14:
	  case 24:
		admnflag++;
		interrupted++;
		break;

	  case  5:
	  case 15:
	  case 25:
		nullflag++;
		interrupted++;
		break;

	  default:
		failflag++;
		interrupted++;
		return;
	}
	if(retcode >= 20)
		ireboot++;
	else if(retcode >= 10)
		reboot++;
}

#define MAXARGS 15

static int
pkgremove()
{
	void	(*tmpfunc)();
	char	*arg[MAXARGS], path[PATH_MAX];
	int	n, nargs;

	(void) sprintf(path, "%s/pkgremove", PKGBIN);

	nargs = 0;
	arg[nargs++] = path;
	if(nointeract)
		arg[nargs++] = "-n";
	if(admnfile) {
		arg[nargs++] = "-a";
		arg[nargs++] = admnfile;
	}
	arg[nargs++] = "-N";
	arg[nargs++] = prog;
	arg[nargs++] = pkginst;
	arg[nargs++] = NULL;

	tmpfunc = signal(SIGINT, func);
	n = pkgexecv(NULL, NULL, arg);
	(void) signal(SIGINT, tmpfunc);

	(void) signal(SIGINT, func);
	return(n);
}
