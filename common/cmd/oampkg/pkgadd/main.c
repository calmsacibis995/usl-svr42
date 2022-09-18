/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)oampkg:common/cmd/oampkg/pkgadd/main.c	1.12.24.41"
#ident  "$Header: $"

#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <pkgdev.h>
#include <pkginfo.h>
#include <pkglocs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <siginfo.h>
#include <unistd.h>
#include <wait.h>
#include <pwd.h>
#include <errno.h>
#include "install.h"
#include "msgdefs.h"

extern char	*optarg;
extern int	optind, errno, ckquit;
extern int	ds_totread;

extern	int	logmode;
extern	int	quietmode;
extern	int	noprompt;

extern void	trap(),
		progerr(),
		logerr(),
		quit(),
		exit(),
		nqtext(),
		ptext(),
		ds_order(),
		ds_putinfo(),
		setadmin(),
		echo();
extern int	devtype(),
		presvr4(),
		ckyorn(),
		pkgexecv(),
		ds_init(),
		ds_close(),
		ds_findpkg(),
		ds_readbuf(),
		ds_pkgonstream(),
		isdir(),
		ispipe(),
		getvol(),
		_getvol(),
		getopt(),
		access(),
		mkdir(),
		chdir(),
		rrmdir(),
		pkgexecl(),
		pkgmount(),
		pkgumount(),
		pkgtrans();
extern char	*pkgparam(),
		*devattr(),
		**gpkglist(),
		*getenv(),
		*getcwd();

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
int	npkgs;		/* the number of packages yet to be installed */
int	svlogmod = 0;	/* for resetting logmode */

char	*pkginst;	/* current package (source) instance to process */
char	*prog;		/* the basename of argv[0] */
char	*respfile;	/* response pathname (or NULL) */
char	*tmpdir;	/* location to place temporary files */
char	*ids_name;	/* name of data stream device */
char	*vol;		/* value returned from devattr() */
void	(*func)();

static char	*device,
		*admnfile,	/* file to use for installation admin */
		*respdir,	/* used if respfile is a directory spec */
		*spoolto;	/* optarg specified with -s (or NULL) */
static int	interrupted,	/* if last pkg installation was quit */
		askflag;	/* non-zero if this is the "pkgask" process */
		
char	setrespdir[PATH_MAX];	/* directory where set response files go */
char	setlistf[PATH_MAX];	/* file containing set member packages	*/
char	buffer[BUFSIZ];		/* for reading in set package name	*/
char 	*pkgname = NULL;	/* name of package from setinfo file	*/
char 	*pkgpart = NULL;	/* package parts from setinfo file	*/

extern	char	*pkgdir;

siginfo_t 	infop;

static int	givemsg,
		inprogress();

#define MAXARGS		20
#define	PBLK		512	/* 512 byte "physical" block */
#define	ERR_POSTINSTALL	"set postinstall script did not complete successfully"
#define ERR_PKGMAP	"unable to open pkgmap file <%s>"
#define	ERR_STATVFS	"unable to stat file system for /var/tmp"
#define	ERR_VARTMP 	"installation requires %d free blocks in /var/tmp and only %d are available"
#define	ERR_NODEFAULT	"quietmode may not be specified with <%s> admin file."
#define	ERR_CHOWN	"could not change owner of package image temporary directory"
#define	DEFAULT		"default"	/* admin file "nocheck" */
#define	MENUQUIT	0
#define U_ZERO		000


static void	usage();
static int	pkginstall();
void	ckreturn();

main(argc,argv)
int	argc;
char	**argv;
{
	int	i, c, n, j, rc = 0,
		repeat, svquiet = 0;
	char	ans[INPBUF], 
		path[PATH_MAX],
		**spkglist,		/* array will contain all packages on media */
		**pkglist;		/* points to array of packages */


	struct	stat status;		/* for existence of setpkglist setlistf	*/
	pid_t	pid;			/* pid of pkgadd, used to name setlistf	*/
	char	buffer[BUFSIZ];		/* for reading in set package name	*/
	char	*cwd;			/* current working directory		*/
	char 	errlogfile[PATH_MAX];	/* /var/sadm/install/logs/<pkginst>.log	*/
	char	*lastpkg;		/* last package instance in a set	*/
	char	**savelist;		/* save command line package names args	*/
	char 	package[PATH_MAX];	/* for datastream, /var/tmp/dstr../pkginst */
	char 	pkgadd_id[BUFSIZ];	/* pkgadd id process for naming setlistf */
	char 	pkginstenv[BUFSIZ];	/* pkgadd instance for environment	*/
	char	*pkgstderr = NULL;	/* stderr for set postinstall script	*/
	char	*pkgstdout = NULL;	/* stdout for set postinstall script	*/
	char	prompt[BUFSIZ];		/* prompt for subseq floppies of set	*/ 
	char	set_post[PATH_MAX];	/* set postinstall script name		*/
	char	setinfo[PATH_MAX];	/* setinfo file full pathname		*/
	char	setinst[BUFSIZ];	/* set instance name			*/
	char	setinstance[BUFSIZ];	/* set instance name			*/
	char	setnm[BUFSIZ];		/* full set name			*/
	char	*setname;		/* fullname of set installation package */
	char	**smem_inst;		/* holds pkginst/parts/fullname of pkg	*/
	char	**smem_name;		/* holds pkginst/parts/fullname of pkg	*/
	char	**smem_part;		/* holds pkginst/parts/fullname of pkg	*/
	FILE	*sfp;			/* used to access setinfo file		*/
	FILE	*tfp;			/* used to access setlistf		*/ 
	int 	k;			/* index through pkglist for saving	*/
	int	nsetpkgs;		/* index through packages in set	*/
	int	rflag = 0;		/* flag that tells if -r option used	*/
	int	svindex = 1;		/* index through savelist		*/
	int	setflag = 0;		/* flag tells us if we are currently in */
	int	setpkgs = 0;		/* index through pkginst->pkgname array	*/
					/* the process of installing a set 	*/
					/* set member package.			*/
	pid_t	uid;			/* invoking login real uid 		*/
	extern	pid_t dd_pid;		/* to kill dd process writing to pipe	*/

	struct	passwd	*pwentry;
	struct	pkginfo	info;

	prog = strrchr(argv[0], '/');
	if(!prog++)
		prog = argv[0];
	askflag = !strcmp(prog, "pkgask");

	/*
	 * Let's initialize the name of the file that will be used to exchange
	 * set member package instance names if the current package being 
	 * installed is a Set Installation Package (SIP).
	 */
	pid = getpid();
	(void) sprintf(setlistf, "/var/tmp/set.%d", pid);

	logmode = 0;
	quietmode = 0;

	device = NULL;
	while ((c = getopt(argc,argv,"s:d:a:r:nclpq?")) != EOF) {
		switch(c) {
		  case 's':
			spoolto = optarg;
			break;

		  case 'd':
			device = optarg;
			break;

		  case 'a':
			admnfile = optarg;
			break;

		  case 'r':
			respfile = optarg;
			rflag++;
			if(isdir(respfile) == 0)
				respdir = respfile;
			break;

		  case 'n':
			nointeract++;
			break;

		  case 'l':		/* log error messages */
			logmode++;
			break;

		  case 'q':
			quietmode++;	/* display prompts and error messages only */ 
			break;

		  case 'p':
			noprompt++;
			break;

		  default:
			usage();
		}
	}

	/*
	 * If invoking command is pkgask, quietmode and 
	 * logmode are invalid.
	 */
	if(askflag && (spoolto || nointeract || quietmode || logmode))
		usage();

	if(spoolto && (nointeract || admnfile || respfile))
		usage();

	func = signal(SIGINT, trap);
	if(func != SIG_DFL)
		(void) signal(SIGINT, func);
	(void) signal(SIGHUP, trap);

	/*
	 * Log mode can be achieved via -l option or by having previously
	 * set and exported the environment variable LOGMODE to "true".  
	 * If PKGINST is set in environment, this mode will cause error
	 * messages produced during the installation to be logged in 
	 * file /var/sadm/install/logs/<pkginst>.log rather than being
	 * displayed on screen.
	 */
	if(logmode)
		(void) putenv("LOGMODE=true");

	/*
	 * Quiet mode can be achieved via the -q option or by having previously
	 * set and exported the environment variable QUIETMODE to "true".  This
	 * mode will display only prompts and error messages.  Quietmode can only
	 * be used in conjunction with the default admin file.  If incorrect admin
	 * file is specified and logmode is set, let's unset it before calling
	 * progerr so that the error message is forced to stderr.
	 */
	if (admnfile && quietmode) {
		if(strcmp(admnfile, DEFAULT)) {
			logmode = 0;
			progerr(ERR_NODEFAULT, admnfile);
			quit(1);
		}
	}
	if(quietmode)
		(void) putenv("QUIETMODE=true");

	/*
	 * initialize installation admin parameters 
	 */
	setadmin(admnfile);

	/*
	 * process response file argument
	 */
	if(respfile) {
		if(respfile[0] != '/') {
			progerr("response file <%s> must be full pathname",
				respfile);
			quit(1);
		}
		if(respdir == NULL) {
			if(askflag) {
				if(access(respfile, 0) == 0) {
					progerr("response file <%s> must not exist",
						respfile);
					quit(1);
				}
			} 
		}
	} else if(askflag) {
		progerr("response file (to write) is required");
		usage();
		quit(1);
	}

	if(device == NULL) {
		device = devattr("spool", "pathname");
		if(device == NULL) {
			progerr("unable to determine device to install from");
			quit(1);
		}
	}

	if(spoolto)
		quit(pkgtrans(device, spoolto, &argv[optind], 0));

	tmpdir = getenv("TMPDIR");
	if(tmpdir == NULL)
		tmpdir = P_tmpdir;

	if(devtype(device, &pkgdev)) {
		progerr("bad device <%s> specified", device);
		quit(1);
	}

	ids_name = NULL;
again:
	svindex = 1;	/* Reset index into savelist */
	if(!askflag)
		givemsg = 1;

	if(ids_name) {

		/*
		 * If pkgadd is invoked without a package instance specified on the
		 * command line, a menu of packages available on the media is displayed.
		 * Once all packages that were selected for installation are installed
		 * the user is again given the a prompt to quit or install more packages
		 * from the device being used.  If the user selects to install more 
		 * packages, we will not be able to open the device if it is a tape 
		 * device unless we terminate the process that was reading it now.  If 
		 * we don't do this, the tape device cannot be accessed.  Since dd_pid
		 * is originally initialized to -1, the first time through this code
		 * will not be run.  
		 *
		 * For a datastream diskette device this is not required. This is because
		 * the dd process gets stopped after each package is installed to allow
		 * ds_readbuf() to initiate a new dd process for checking if a datastream
		 * header is present on the diskette.  This allows us to differentiate
		 * between file system and datastream diskettes. 
		 */
		if(dd_pid > 0 && !pkgdev.bdevice) {
			(void) sigsend(P_PID, dd_pid, SIGKILL);
			if(waitid(P_PID, dd_pid, &infop, WSTOPPED||WEXITED) < 0) {
				/*
				 * If child dd process is no longer around
				 * disregard error from waitid(().
				 */
				if (errno != ECHILD) {
					progerr("could not terminate dd process");
					quit(1);
				}
			}
			dd_pid = -1;
		}
		(void)ds_close(1);

		/*
		 * If member package directory still exists,
		 * remove it.
		 */
		if(stat(pkgdev.dirname, &status) == 0) {
			if((cwd = getcwd(NULL, PATH_MAX+1)) == NULL) {
				progerr("could not determine current working directory");
				quit(99);
			}
			if(strcmp(pkgdev.dirname, cwd))
				(void) rrmdir(pkgdev.dirname);
		}
	}

retry:
	if(pkgdev.bdevice) {
		if(n = _getvol(pkgdev.bdevice, NULL, NULL, "Insert %v into %p.", pkgdev.norewind, 0)) {
			if(n == 3)
				quit(3);
			if(n == 2)
				progerr("unknown device <%s>", pkgdev.name);
			else {
				progerr("unable to obtain package volume");
				logerr("getvol() returned <%d>", n);
			}
			quit(99);
		}
		if((vol = devattr(device, "volume")) != NULL) 
			inprogress(vol);
		else if(isdir(device) != 0 && isfile(device) != 0 && ispipe(device) != 0) {
			if(logmode)
				logmode--;

			progerr("device <%s> is not listed in device table.", device);
			quit(99);
		}
			
		if(ds_readbuf(pkgdev.cdevice))
			ids_name = pkgdev.cdevice;
	}

	if(pkgdev.cdevice && !pkgdev.bdevice)
		ids_name = pkgdev.cdevice;
	else if(pkgdev.pathname)
		ids_name = pkgdev.pathname;

	uid = getuid();
	if(ids_name) {
		/* initialize datastream */
		pkgdev.dirname = tempnam(tmpdir, "dstream");
		/*
		 * For case when invoking user is not privileged to
		 * use pkgadd, we want to allow use of the command
		 * just to get a listing of package/sets available
		 * for installation.  To do so, we must make sure that
		 * the real-uid is able to write to pkgdev.dirname
		 * which gets created owner effective-uid (root).
		 */
		if(!pkgdev.dirname || mkdir(pkgdev.dirname, 0755)) {
			progerr(ERR_STREAMDIR);
			quit(99);
		}
		if(chown(pkgdev.dirname, uid, -1) < 0) {
			progerr(ERR_CHOWN);
			quit(99);
		}
	}

	repeat = (optind >= argc) ;

	if(!ids_name && pkgdev.mount) {
		pkgdev.rdonly++;
		if(logmode) {
			logmode--;
			svlogmod++;
		}
		if(n = pkgmount(&pkgdev, NULL, 0, 0, 0)) {
			svlogmod--;
			logmode++;
			goto again;
		}
	}

	if(ids_name) {
		/* use character device to force rewind of datastream */
		if(pkgdev.cdevice && !pkgdev.bdevice) {
			if(n = _getvol(pkgdev.name, NULL, NULL, NULL, pkgdev.norewind, 0)) {
				/* We got here if this was a datastream TAPE only */
				if(n == 3)
					quit(3);
				if(n == 2)
					progerr("unknown device <%s>", pkgdev.name);
				else {
					progerr("unable to obtain package volume");
					logerr("getvol() returned <%d>", n);
				}
				quit(99);
			}
		}
		if((vol = devattr(device, "volume")) != NULL) 
			inprogress(vol);
		else if(isdir(device) != 0 && isfile(device) != 0 && ispipe(device) != 0) {
			if(logmode)
				logmode--;
			progerr("device <%s> is not listed in device table.", device);
			quit(99);
		}
		if(chdir(pkgdev.dirname)) {
			progerr(ERR_CHDIR, pkgdev.dirname);
			quit(99);
		}

		/*
		 * For the case of when a set installation package is specified on the
		 * command line we'll need to have its member packages' information files
		 * (pkginfo and pkgmap) also available.  If this is a datastream tape,
		 * we could call ds_init() again later when we've determined the names of
		 * the member package instances to be installed as part of the set.
		 * However, this would require that the tape be rewound and reinitialized
		 * at that time.  If there's more than one set on the tape and if the
		 * packages are large, we would have to do a great deal of travelling on
		 * the tape to get that information later resulting in performance loss.
		 * To avoid this, we'll force the reading in of all packages' information
		 * files located on the tape now while we're at the beginning of the tape.
		 * Having these files around may take up some more disk space on the system,
		 * but it is not large.
		 */
		spkglist = gpkglist(pkgdev.dirname, "all", prog); 
		if(ds_init(ids_name, spkglist, pkgdev.norewind)) {
			quit(99);
		}
	}

	pkglist = gpkglist(pkgdev.dirname, &argv[optind], prog);
	if(pkglist == NULL) {
		if(errno == ENOPKG) {
			/* check for existence of pre-SVR4 package */
			(void) sprintf(path, "%s/install/INSTALL", 
				pkgdev.dirname);
			if(access(path, 0) == 0) {
				pkginst = ((optind < argc) ? 
					argv[optind++] : NULL);
				ckreturn(presvr4(&pkginst));
				if(repeat || (optind < argc))
					goto again;
				quit(0);
			}
			if (argv[optind]) {
				if(vol)
					progerr("selected package <%s> was not found on %s in <%s>", 
						argv[optind], vol, pkgdev.name);
				else
					progerr(ERR_NOPKGS, argv[optind], pkgdev.name);
			}
			else {
				if(vol)
					progerr("no packages were found on %s in <%s>", vol, pkgdev.name);
				else
					progerr("no packages were found in <%s>", pkgdev.name);
			}

			if(dd_pid > 0) {
				(void) sigsend(P_PID, dd_pid, SIGKILL);
				if(waitid(P_PID, dd_pid, &infop, WSTOPPED||WEXITED) < 0) {
					if (errno != ECHILD) {
						progerr("could not terminate dd process");
						quit(1);
					}
				}
				dd_pid = -1;
			}
			quit(1);
		} else {
			/* some internal error */
			switch(errno) {
			  case ESRCH:
				if(pkgdev.cdevice || pkgdev.bdevice) {
					fprintf(stderr, "\npkgadd: WARNING: A package you have specified for installation\n");
					fprintf(stderr, "\tis not located on the %s inserted in <%s>.\n", vol, pkgdev.name);
				}
				else {
					fprintf(stderr, "\npkgadd: ERROR: The package you have specified for installation\n");
					fprintf(stderr, "\twas not found in the device/directory specified.\n");
				}
					
				if(dd_pid > 0) {
					(void) sigsend(P_PID,  dd_pid, SIGKILL);
					if(waitid(P_PID, dd_pid, &infop, WSTOPPED||WEXITED) < 0) {
						if(errno != ECHILD) {
							progerr("could not terminate dd process");
							exit(1);
						}
					}
				}
				dd_pid = -1;
				if(pkgdev.mount)
					(void) pkgumount(&pkgdev);
				if(!pkgdev.bdevice)
					quit(1);
				goto retry;
				break;

			  case EINTR:
				quit(3);
				break;

			/*
			 * The gpkglist() routine sets errno to MENUQUIT 
			 * if user selected to quit from the menu prompt
			 * generated by gpkglist().  In that case quit
			 * out gracefully, producing no error messages.
			 */
			  case MENUQUIT:
				quit(0);
				break;

			  default:
				quit(99);
				break;
			}
		}
	}

	/*
	 * Any user (even one without privileges) is able to get to this point.
	 * This is so the PackageMgr is able to obtain a list of packages on the
	 * installation media without privileges.  It uses this to create icons
	 * for each package.  To actually proceed with installation of a package,
	 * the invoking login must have the dacwrite privilege to access the
	 * /var/sadm/install directory where the system's contents file resides.
	 * If the invoking login does not, the process aborts.  Otherwise, to
	 * ensure that all tasks that may be called from packaging scripts wind
	 * up being successfully executed, we set the uid for the remainder of the
	 * installation process to root.
	 */
	pwentry = getpwuid(uid); 

	if(access(PKGADM, W_OK) < 0) {
		progerr("Login <%s> may not install software, %s", 
				pwentry->pw_name, strerror(errno));
		quit(1);
	}
	if(setuid(0) < 0) {
		progerr("Call to setuid failed, %s", strerror(errno));
		quit(1);
	}

	if(ids_name)
		ds_order(pkglist);

	/*
	 * Save package instances specified on command line in case one
	 * of these is a Set Installation Package (SIP).  For each SIP,
	 * we will insert into pkglist[] all set member packages selected
	 * for installation.  Once these are inserted, we will place the
	 * package instances saved in savelist[] back onto pkglist[].
	 */
	savelist = (char **)calloc(120, sizeof(char **));
	for(npkgs=0; pkglist[npkgs]; npkgs++) 
		savelist[npkgs] = strdup(pkglist[npkgs]);

	interrupted = 0;
	for(i=0; pkginst = pkglist[i]; i++) {
		(void) sprintf(pkginstenv, "PKGINST=%s", pkginst);
		(void) putenv(pkginstenv);
		(void) sprintf(package, "%s/%s", pkgdev.dirname, pkginst);

		/*
		 * Set up error logfile name - remove any previous 
		 * installation's logfile.
		 */
		(void) sprintf(errlogfile, "%s/logs/%s.log", PKGADM, pkginst);
		(void) unlink(errlogfile);


		/*
		 * The next section handles set member installation when the next set
		 * member package is not located on the current datastream media. This
		 * is done by checking if the current datastream's table of contents
		 * contains an entry for the package we're about to install.  If the
		 * package is not listed there, then we know that we have to prompt
		 * for the media that contains the package.
		 */

		if(setflag && ids_name && pkgdev.bdevice) {
			/* If package is not in the table of contents */
			if(!ds_pkgonstream(pkginst)) {
				/*
				 * Get full set and package member names.
				 */
				for(j=0; j < setpkgs; j++) {
					if(!strcmp(pkginst, smem_inst[j])) {
						pkgname = strdup(smem_name[j]);
						pkgpart = strdup(smem_part[j]);
						break;
					}
				}
				(void) fprintf(stderr,"\nREADY TO PROCESS:\n");
prompt:
				(void) fprintf(stderr, "  Set:     %s (%s)\n", setname, setinst);
				(void) fprintf(stderr, "  Package: %s (%s)\n           %s 1 of %s\n", 
						pkgname, pkginst, vol ? vol : "part", pkgpart);

				if(n = _getvol(pkgdev.bdevice, NULL, NULL,
					"Insert %v into %p.", pkgdev.norewind, 0)) {
					if(n == 3)
						quit(3);
					if(n == 2)
						progerr("unknown device <%s>", pkgdev.name);
					else {
						progerr("unable to obtain package volume");
						logerr("getvol() returned <%d>", n);
					}
					quit(99);
				}
				if((vol = devattr(device, "volume")) != NULL) 
					inprogress(vol);
				else if(isdir(device) != 0 && isfile(device) != 0 && ispipe(device) != 0) {
					if(logmode)
						logmode--;
					progerr("device <%s> not listed in device table.", device);
					quit(99);
				}

				/* Verify that media inserted is datastream */
				if(!ds_readbuf(pkgdev.cdevice)) {
					if(pkgdev.bdevice) {
						(void) fprintf(stderr, "\npkgadd: WARNING: Could not find (%s) on <%s>\n",
									pkginst, pkgdev.name); 
						(void) fprintf(stderr, "\nREPROMPTING FOR:\n");
						goto prompt;
					}
					else {
						progerr("expected datastream in %s", pkgdev.bdevice); 
						quit(99);
					}
				}
				/*
				 * If this was a datastream diskette, the pkginstall quit function
				 * function has already stopped the dd process that was running for
				 * the previous diskette.  We don't need to deal with it here.
				 */
				(void) ds_close(1);

				/* Read in package off of datastream into temporary area */ 
				if(ds_init(ids_name, &pkginst, pkgdev.norewind))
					quit(99);
			}
		}

		if(ireboot && !askflag) {
			ptext(stderr, MSG_SUSPEND, pkginst);
			continue;
		}
interrupt:
		if(interrupted) {
			/*
			 * If logmode and quietmode are set and more than one package 
			 * instance is selected for installation, no error messages 
			 * are displayed on the screen.  In this case, the user would
			 * wind up with only a prompt asking whether to go on with 
			 * installation of the next packages with no clue as to what 
			 * error occured for this package.  To avoid this, if a package
			 * installation is interrupted when logmode is set we will treat
			 * it the same as nointeract.
			 */
			if(logmode && !nointeract)
				nointeract++;
			/* Temporarily unset quietmode for echo() */
			if(quietmode) {
				quietmode = 0;
				svquiet = 1;
			}
			if(npkgs == 1 && !ids_name) {
				echo(MSG_1MORETODO, 
					(askflag ? "processed" : "installed"));
			} else if (!ids_name) {
				echo(MSG_MORETODO, npkgs,
					(askflag ? "processed" : "installed"));
			}
			/* Reset quietmode if it was turned on */ 
			if(svquiet) {
				quietmode = 1;
				svquiet = 0;
			}
			if(nointeract)
				quit(0);
			ckquit = 0;

			if(ids_name)
				quit(rc ? rc : 1);
			else if(n = ckyorn(ans, NULL, NULL, NULL, ASK_CONTINUE))
				quit(n);
			ckquit = 1;
			interrupted = 0;
			if(ans[0] != 'y') {
				/* 
				 * Since quit(0) won't kill the dd process (it will kill
				 * it only if called with an argument greater than zero,
				 * terminate the dd process reading datastream (if running)
				 * here to keep this process from hanging.
				 */
				if(dd_pid > 0) {
					(void) sigsend(P_PID, dd_pid, SIGKILL);
					if(waitid(P_PID, dd_pid, &infop, WSTOPPED||WEXITED) < 0) {
						if (errno != ECHILD) {
							progerr("could not terminate dd process");
							quit(1);
						}
					}
					dd_pid = -1;
				}
				quit(0);
			} 
			else { 
				svindex++;
				continue;
			}
		}

		if(respfile) {
			if(respdir) {
				(void) sprintf(path, "%s/%s", respdir, pkginst);
				respfile = path;
				if(askflag) {
					if(access(respfile, 0) == 0) {
						progerr("response file <%s> must not exist",
							respfile);
						quit(1);
					}
				} 
			} else if(npkgs > 1) {
					progerr("More than one package referenced but only one response file");
					quit(1);
			}
		}
		
		interrupted = 0;
			
		/*
		 * Get full package name.
		 */
		pkgdir = pkgdev.dirname;
		if(!pkginfo(&info, pkginst, NULL, NULL)) {
			pkgname = strdup(info.name);
			nqtext(stderr, "\n");
			nqtext(stderr, "PROCESSING:");
			if(!strcmp(info.catg, "set")) {
				(void) strcpy(setinst, pkginst);
				(void) sprintf(setinstance, "SETINST=%s", setinst);
				(void) putenv(setinstance);
				(void) sprintf(setnm, "SETNAME=%s", pkgname);
				(void) putenv(setnm);
				setname = strdup(setnm);
				nqtext(stderr, "  Set: %s (%s) from <%s>.", pkgname, setinst, device);
			}
			else {
				if(setflag) {
					nqtext(stderr, "  Set: %s (%s) from <%s>.", setname, setinst, device);
					(void) sprintf(setinstance, "SETINST=%s", setinst);
					(void) putenv(setinstance);
				}
				nqtext(stderr, "  Package: %s (%s) from <%s>.", pkgname, pkginst, device);
			}
		}
		(void) pkginfo(&info, NULL, NULL, NULL);
		rc = pkginstall();
		/*
		 * For case of diskettes, if incorrect volume (part) was
		 * placed into the device drive pkginstall() will return
		 * 88.  For this case, allow the user to reinsert the
		 * correct volume or to quit out.
		 */
		if (rc == 88)
			goto prompt;
		ckreturn(rc);
		/*
		 * If pkginstall failed and more packages exist in
		 * pkglist, go handle installation failure.
		 */
		if ((rc == 1 || rc == 3) && npkgs > 1) {
			if(rc == 3)
				nqtext(stderr, "Installation process was interrupted by user request.\n");
			npkgs--;
			goto interrupt;
		}
		interrupted = 0;
		npkgs--;


		/***************************************************************************
		 * If this was invoked as pkgask, just run the request script for the
		 * package(s) specified in pkglist.  For sets, the response file(s)
		 * will be created in the directory specified on the command line to
		 * the -r option (for sets, this must be a directory).
		 *
		 * If this was invoked as pkgadd with the -r option specified, use the
		 * argument to that option as the directory to look in for response files
		 * for the SIP and the set member packages.  If no -r option was specified,
		 * look in /var/tmp/respdir.<pid> which was created by pkginstall during 
		 * processing of the request scripts. 
		 * 
		 * If the package just processed was a SIP, then pkginstall created a file
		 * /var/tmp/set.$PKGADD_ID.  If so, we need to read its contents to get the
		 * names of set member packages that were selected via the SIP request 
		 * script and add them to pkglist.  We also must set the environment variable
		 * "SET" to "true" so that the postinstall scripts of set  member packages do
		 * not invoke tasks (i.e., idbuild) for each package that may be done only 
		 * once after the entire set is installed..  Once we have processed the last 
		 * member package of the set, we invoke the SIP postinstall script after 
		 * resetting SET to "false".
		 ***************************************************************************/

		if (stat(setlistf, &status) == 0 && !askflag) {
			setflag = 1;
			(void) putenv("SET=true");

			/* 
			 * Populate pkginst->pkgname->pkgpart tables. 
			 */

			(void) sprintf(setinfo, "%s/%s/setinfo", pkgdev.dirname,
					setinst);
			if((sfp = fopen(setinfo, "r")) == NULL) {
				progerr("Could not open setinfo file for <%s>", setinst);
				exit(1);
			}
			smem_inst = (char **)calloc(120, sizeof(char **));
			smem_name = (char **)calloc(120, sizeof(char **));
			smem_part = (char **)calloc(120, sizeof(char **));
			for(setpkgs = 0; fgets(buffer,BUFSIZ,sfp) != NULL; setpkgs++) {
				smem_inst[setpkgs] = strdup(strtok(buffer, "\t"));
				smem_part[setpkgs] = strdup(strtok(NULL, "\t"));
				(void) strtok(NULL, "\t");	/* skip default field	*/
				(void) strtok(NULL, "\t");	/* skip category field	*/
				smem_name[setpkgs] = strdup(strtok(NULL, "\n"));
			}
			(void) fclose(sfp);

			/*
			 * Get full name of set.
			 */
			if(!pkginfo(&info, setinst, NULL, NULL))
				setname = strdup(info.name);
			(void) pkginfo(&info, NULL, NULL, NULL);

			/*
			 * If this was invoked with -r option, then enforce rule that for
			 * a SIP the argument provided to that option must be a directory.
			 */ 
			if (rflag) {
				if (respfile && !respdir)
					quit(1);
			}
			else {
				/*
				 * Not invoked with -r option, we'll need to set up a
				 * directory where the response files for set member
				 * packages will go.
				 */
				(void) sprintf(setrespdir, "/var/tmp/respdir.%d", pid);
				respdir = strdup(setrespdir);
				respfile = respdir;
			}

			/*
			 * Open file that should have been created by set package's
			 * preinstall script.  This file should contain the names of
			 * set member packages selected for installation as part of
			 * this set installation.
			 */
			if((tfp = fopen(setlistf, "r")) == NULL) {
				progerr("could not open package list file %s\n", setlistf);
				quit(1);
			}
			/* The tmp file has one pkginst name per line */
			for(nsetpkgs=i+1; fscanf(tfp, "%s", buffer) != EOF; nsetpkgs++) {
				pkglist[nsetpkgs] = strdup(strtok(buffer, "	")); 
				lastpkg = pkglist[nsetpkgs]; 	/* mark last pkg */
				npkgs++; 	/* Add to number of packages */
			}
			/*
			 * If no set member packages were selected for 
			 * installation, reset the set flag to false.
			 */
			if(nsetpkgs == i+1) {
				setflag = 0;
				(void) putenv("SET=false");
				if (!rflag)
					respfile = NULL;
				/*
				 * Since no set member packages were selected or SIP
				 * installation somehow failed (e.g., space checking),
				 * let's incrament savelist index here since it won't
				 * be done in lastpkg processing.
				 */
				k = svindex++;
			} else
				k = svindex;
			pkglist[nsetpkgs] = NULL;

			/*
			 * Place the remaining command line specified package instances back on 
			 * the pkglist after having inserted the set packages to be installed.
			 */

			for(; savelist[k]; k++, nsetpkgs++) {
				pkglist[nsetpkgs] = strdup(savelist[k]);
			}
			pkglist[nsetpkgs] = NULL;


			/* 
			 * We're through with retrieving set package names -- we can safely
			 * remove temporary file containing the set member package instances
			 * selected for installation.
			 */
			(void) unlink(setlistf);
		}
		/*
		 * If this is being installed off of a datastream device, let's
		 * remove the package instance just handled from the temporary 
		 * area.
		 */
		if(ids_name) {
			if((cwd = getcwd(NULL, PATH_MAX+1)) == NULL) {
				progerr("could not determine current working directory");
				quit(99);
			}
			if(strcmp(package, cwd))
				(void) rrmdir(package);
		}

		if(pkginst == lastpkg) {
			/* We've hit last package in set, run SIP postinstall script */
			lastpkg = NULL;
			setflag = 0;
			svindex++;
			/* 
			 * If pkgadd was not called with -r option specified, then respdir
			 * was specifically created to house this set's response files
			 * created during execution of the SIP request script.  So for the
			 * case of when other package instances are specified on the command
			 * line after the SIP, we must reset the response file flag to
			 * represent that no response files exist.
			 *
			 * In the case where the -r option was specified, pkgadd assumes
			 * that all package instances specified on the command line (and
			 * all or some of the set member packages in the case of a SIP) 
			 * have response files located in the directory specified as the
			 * argument to the -r option.  In this case do not reset the flag.
			 */
			if (!rflag)
				respfile = NULL;
			(void) putenv("SET=false");

			/* Set up path to SIP postinstall script and execute it. */
			(void) sprintf(set_post, "%s/%s/install/postinstall", pkgdir, setinst);

			if (access(set_post, 0) == 0) {
				echo("\n ## Executing set postinstall script.");
				if(logmode)
					pkgstderr = strdup(errlogfile);
				if(quietmode)
					pkgstdout = "/dev/null";
				if(k = pkgexecl(NULL, pkgstdout, pkgstderr, SHELL, set_post, NULL))
					ckreturn(n, ERR_POSTINSTALL);
			}
			echo("\n");
			echo("\nProcessing of <%s> is completed.\n", setname); 

		}
		/*
		 * If invoked as pkgask, let's remove 'setlistf' and 'setrepsdir'
		 * if they exist.
		 */
		if (askflag) {
			(void) unlink(setlistf);
			if(stat(setrespdir, &status) == 0)
				(void) rrmdir(setrespdir);
		}

		if((npkgs <= 0) && (pkgdev.mount || ids_name)) {
			(void) chdir("/");
			if(!ids_name)
				(void) pkgumount(&pkgdev);
		}

		/*
		 * For case of set member that is on file system type
		 * media we need to unmount and prompt for remount.
		 */
		if (setflag && pkgdev.fstyp && pkgdev.mount) {
			(void) chdir("/");
			if (!ids_name)
				(void) pkgumount(&pkgdev);
			if(n = _getvol(pkgdev.bdevice, NULL, NULL, "Insert %v into %p.", pkgdev.norewind, 0)) {
				if(n == 3)
					quit(3);
				if(n == 2)
					progerr("unknown device <%s>", pkgdev.name);
				else {
					progerr("unable to obtain package volume");
					logerr("getvol() returned <%d>", n);
				}
				quit(99);
			}
			pkgdev.rdonly++;
			if(logmode) {
				logmode--;
				svlogmod++;
			}
			if(n = pkgmount(&pkgdev, NULL, 0, 0, 0)) {
				svlogmod--;
				logmode++;
				goto again;
			}
		}
	}
	if(!ireboot && repeat && !pkgdev.pathname)
		goto again;
	quit(0);
	/*NOTREACHED*/
}

static int
pkginstall()
{
	void	(*tmpfunc)();
	char	*arg[MAXARGS], path[PATH_MAX];
	char	buffer[256];
	char 	pkgmap[PATH_MAX];	/* pkgmap file full path name		*/
	int	n, nargs, dparts;
	int	tmpblks = 0;		/* number of blocks largest volume takes up */
	FILE	*pfp;			/* used to access pkgmap file		*/
	struct	statvfs	statvfsbuf;	/* for checking avail space in /var/tmp	*/

	extern	pid_t dd_pid;
	char ddpid[12];

	(void) sprintf(path, "%s/pkginstall", PKGBIN);

	nargs = 0;
	arg[nargs++] = path;
	if(askflag)
		arg[nargs++] = "-i";
	else if(nointeract)
		arg[nargs++] = "-n";
	if(admnfile) {
		arg[nargs++] = "-a";
		arg[nargs++] = admnfile;
	}
	if(respfile) {
		arg[nargs++] = "-r";
		arg[nargs++] = respfile;
	}
	if(respdir) {
		/*
		 * Used for passing the name of the response file
		 * directory required for a set installation.
		 */
		arg[nargs++] = "-z";
		arg[nargs++] = respdir;
	}
	if(ids_name != NULL) {
		arg[nargs++] = "-d";
		arg[nargs++] = ids_name;
		dparts = ds_findpkg(ids_name, pkginst);
		if(dparts < 1) {
			/*
			 * If the installation media is not tape and the wrong volume
			 * (part) has been inserted into the drive, provide the user 
			 * with the opportunity to reinsert correct volume or to quit.
			 */
			if(pkgdev.bdevice) {
				(void) fprintf(stderr, "\npkgadd: WARNING: Could not find (%s) on <%s>\n",
						pkginst, pkgdev.name); 
				(void) fprintf(stderr, "\nREPROMPTING FOR:\n");
				return(88);
			}
			/*
			 * For case of tape just exit out.
			 */
			else {
				progerr("unable to find archive for <%s> in datastream",
					pkginst);
				quit(99);
			}
		}

		/*
		 * If we're installing from a datastream media, check if 
		 * if there's enough block space in /var/tmp before going
		 * any further with the installation.  If not enough space
		 * is present in /var/tmp, abort the installation with an
		 * appropriate error message.
		 */
		(void) sprintf(pkgmap, "%s/%s/%s", pkgdev.dirname, pkginst, PKGMAP);
		if((pfp = fopen(pkgmap, "r")) == NULL) {
			progerr(ERR_PKGMAP, pkgmap);
			quit(1);
		}
	
		/* 
		 * Get number of blocks the largest volume of this package will take up.
		 */
		(void) fgets(buffer, BUFSIZ, pfp);
		(void) strtok(buffer, " ");
		(void) strtok(NULL, " ");
		tmpblks = atoi(strtok(NULL, " "));
		(void) fclose(pfp);
	
		/*
		 * Check if enough free blocks exist in partition where /var/tmp
		 * is located to hold tmpblks of package volume image.
		 */
		if(statvfs(tmpdir, &statvfsbuf) < 0) {
			progerr(ERR_STATVFS);
			quit(1);
		}
			
		/* Convert to 512 physical block */
		statvfsbuf.f_bavail = statvfsbuf.f_bavail * (((statvfsbuf.f_frsize - 1) / PBLK) + 1);
		if(statvfsbuf.f_bavail < tmpblks) {
			progerr(ERR_VARTMP, tmpblks, statvfsbuf.f_bavail);
			quit(1);
		}

		arg[nargs++] = "-p";
		ds_putinfo(buffer);
		arg[nargs++] = buffer;
		/*
		 * If we're reading from pipe that is being written to by the 
		 * dd process identified by dd_pid, let's pass it to pkginstall. 
		 * We'll need it before calls to ds_close() for multipart packages.
		 */
		if (dd_pid > 0) {
			arg[nargs++] = "-x";
			(void) sprintf(ddpid, "%d", dd_pid);
			arg[nargs++] = ddpid;
		}
		/*
		 * If the datastream is a diskette we'll need to pass that
		 * information to pkginstall for set installation.  This is
		 * to prevent pkginstall from hanging after the last part of
		 * a package has been installed.
		 */
		 if (pkgdev.bdevice) {
			 arg[nargs++] = "-y";
			 arg[nargs++] = pkgdev.bdevice;
		 }

	} else if(pkgdev.mount != NULL) {
		arg[nargs++] = "-d";
		arg[nargs++] = pkgdev.bdevice;
		arg[nargs++] = "-m";
		arg[nargs++] = pkgdev.mount;
		if(pkgdev.fstyp != NULL) {
			arg[nargs++] = "-f";
			arg[nargs++] = pkgdev.fstyp;
		}
	}
	arg[nargs++] = "-N";
	arg[nargs++] = prog;
	arg[nargs++] = pkgdev.dirname;
	arg[nargs++] = pkginst;
	arg[nargs++] = NULL;

	tmpfunc = signal(SIGINT, func);
	n = pkgexecv(NULL, NULL, arg);
	(void) signal(SIGINT, tmpfunc);
	if(ids_name != NULL)
		ds_totread += dparts; /* increment number of parts written */
	return(n);
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

static void
usage()
{
	if(askflag) {
		(void) fprintf(stderr, "usage: %s ", prog);
		(void) fprintf(stderr, "-r response [-d device] ");
		(void) fprintf(stderr, "[pkg [pkg ...]]\n");
	} else {
		(void) fprintf(stderr, "usage:\n\t%s ", prog);
		(void) fprintf(stderr, "[-n] [-d device] [-a admin] [-r response] ");
		(void) fprintf(stderr, "[pkg [pkg ...]]\n");
		(void) fprintf(stderr, "\t%s -s dir [-d device] [pkg [pkg ...]]\n", prog);
	}
	exit(1);
	/*NOTREACHED*/
}

static int
inprogress(drive)
char *drive;
{
	if(givemsg == 1) {
		echo ("\n\nInstallation in progress.  Do not remove the %s.\n", drive);
		givemsg = 0;
	}
}
