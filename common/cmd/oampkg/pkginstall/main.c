/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)oampkg:common/cmd/oampkg/pkginstall/main.c	1.19.16.30"
#ident  "$Header: $"

#include <stdio.h>
#include <time.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <wait.h>
#include <dirent.h>
#include <pkgstrct.h>
#include <pkginfo.h>
#include <pkgdev.h>
#include <pkglocs.h>
#include "install.h"

extern char	*optarg, **environ;
extern char	*pkgabrv, *pkgname,
		*pkgarch, *pkgvers,
		pkgwild[];

extern int	optind, errno, noprompt,
		Mntflg, installed,
		logmode, quietmode;

extern void	*calloc(), 
		*realloc(),
		putparam(),
		ptext(),
		echo(),
		lockinst(),
		trap(),
		getbase(),
		setadmin(),
		merginfo(),
		ckrunlevel(),
		ckconflct(),
		cksetuid(),
		ckdepend(),
		ckpriv(),
		ckpartial(),
		ckspace(),
		ckdirs(),
		ckpkgdirs(),
		ckpkgfiles(),
		setlvl();
extern char	*qstrdup(),
		*mktemp(), 
		*pkgparam(), 
		*devattr(),
		*fpkgparam(), 
		*fpkginst(), 
		*getenv(),
		*getinst();
extern long	atol();
extern unsigned long
		umask();
extern int	getopt(),
		access(),
		unlink(),
		atoi(),
		mkdir(),
		chdir(),
		creat(),
		pkginfo(),
		ckvolseq(),
		ocfile(),
		pkgnmchk(),
		swapcfile(),
		pkgenv(),
		rrmdir(),
		pkgexecl(),
		sortmap(),
		reqexec(),
		setlist(),
		cftime(),
		ds_next(),
		ds_getinfo();
extern void	exit(),
		progerr(),
		logerr(),
		instvol(),
		predepend(),
		quit();

#define DEFPATH		"/sbin:/usr/sbin:/usr/bin:/opt/bin"
#define MALSIZ	4	/* best guess at likely maximum value of MAXINST */ 
#define LSIZE	256	/* maximum line size supported in copyright file */
#define	sys_pub	1	/* the SYS_PUBLIC level */
#define	FSZDIV	512	/* fsize divisor for getrsize() returned  value	*/

#define ERR_MEMORY	"memory allocation failure, errno=%d"
#define ERR_ADMNFILE	"for set installation admin file <default> must be specified"
#define ERR_BADFORMAT	"<%s> is not properly formatted for installation"
#define ERR_INTONLY	"unable to install <%s> without user interaction"
#define ERR_NOREQUEST	"package does not contain an interactive request script"
#define ERR_LOCKFILE	"unable to create lockfile <%s>"
#define ERR_PKGINFO	"unable to open pkginfo file <%s>"
#define ERR_RESPONSE	"unable to open response file <%s>"
#define ERR_PKGMAP	"unable to open pkgmap file <%s>"
#define ERR_ULIMIT	"unable to set ulimit to <%ld>, errno=%d"
#define ERR_MKDIR	"unable to make temporary directory <%s>"
#define ERR_RMDIR	"unable to remove directory <%s> and its contents"
#define ERR_CHDIR	"unable to change directory to <%s>"
#define ERR_DSTREAM	"unable to unpack datastream"
#define ERR_DSTREAMSEQ	"datastream seqeunce corruption"
#define ERR_DSTREAMCNT	"datastream early termination problem"
#define ERR_RDONLY 	"read-only parameter <%s> cannot be assigned a value"
#define ERR_REQUEST	"request script did not complete successfully"
#define ERR_PREINSTALL	"preinstall script did not complete successfully"
#define ERR_POSTINSTALL	"postinstall script did not complete successfully"
#define ERR_OPRESVR4	"unable to unlink otpions file <%s>"
#define ERR_SYSINFO	\
	"unable to process installed package information, errno=%d"
#define ERR_SETDUP      "attempt to process set within set"
#define	ERR_OPENLIST	"could not open %s"
#define	ERR_RESPDIR	"could not create %s response directory"

#define MSG_INSTALLED	"   %d package %s already properly installed."

struct admin	adm;
struct pkgdev	pkgdev;

int	nocnflct, nosetuid;
int	dbchg;
int	rprcflag;
int	iflag;
int	dparts = 0;
char	*quiet = NULL;		/* quiet mode flag			*/
char	*log = NULL;		/* log mode flag			*/
char	*package = NULL;	/* value of PKGINST			*/
char    *category;      	/* value of CATEGORY in pkginfo file	*/
char	*pkgstderr = NULL;	/* stderr for package scripts		*/
char	*pkgstdout = NULL;	/* stdout for package scripts		*/

int	reboot = 0;
int	ireboot = 0;
int	warnflag = 0;
int	failflag = 0;
int	started = 0;
int	update = 0;
int	opresvr4 = 0;
int	nointeract = 0;
int	nparts = 0;		/* number of parts in package	 */
int	maxinst = 1;

char	*errlog,
	*prog,
	*pkginst,
	*msgtext,
	*respfile,
	**class,
	**pclass,
	instdir[PATH_MAX],
	pkgloc[PATH_MAX],
	pkgbin[PATH_MAX],
	pkgsav[PATH_MAX],
	ilockfile[PATH_MAX],
	rlockfile[PATH_MAX],
	savlog[PATH_MAX],
	tmpdir[PATH_MAX];
unsigned nclass;

void	ckreturn(),
	setinst(),
	killdd();

static char	*ro_params[] = {
	"PATH", "NAME", "PKG", "PKGINST",
	"VERSION", "ARCH", "BASEDIR", "TERM",
	"INSTDATE", "CATEGORY", "SET", "INSTDIR",
	"SETINFO", "SETLIST", "RESPDIR",
	"SETNAME", "SETINST", "SETSIZE",
	"LOGMODE", "QUIETMODE", NULL
};

static int	rdonly();
static void	copyright(),
		unpack(),
		usage();


struct	rlimit	rlp, unlimited;	/* For manipulating ulimit */
int	limit;

main(argc, argv)
int	argc;
char	*argv[];
{
	struct	pkginfo *prvinfo;
	FILE	*mapfp, *tmpfp;
	FILE	*fp;
	int	c, n;
	long	clock;
	int	npkgs, part;
	char	*pt, 
		*value, 
		*srcinst, 
		*device,
		*admnfile = NULL;
	char	path[PATH_MAX],
		cmdbin[PATH_MAX],
		p_pkginfo[PATH_MAX],
		p_pkgmap[PATH_MAX],
		script[PATH_MAX],
		cbuf[64],
		param[64];
	void	(*func)();

	/*
	 * New definitions for set handling, logmode and quietmode. 
	 */
	char	errlogfile[PATH_MAX];	/* /var/sadm/install/logs/<pkginst>.log	*/
	char	line[BUFSIZ];
	char	listfile[PATH_MAX];	/* name of /var/tmp/set.<pkgadd_id>	*/
	char	reqdir[PATH_MAX];	/* pre-processed request/response files */
	char	respdir[PATH_MAX];	/* post-processed response files 	*/
	char	*argrespd = NULL;	/* name of response files directory	*/
	char    *setflag = NULL;	/* value of SET environment variable	*/
	char	setinfo[PATH_MAX];	/* name of setinfo file 		*/ 
	char	*setname;		/* full name of set			*/
	char 	*setinstance;		/* set instance name			*/ 
	char	setsize[PATH_MAX];	/* space file directory in environment 	*/
	char	*term;			/* TERM environment variable		*/
	int	didset = 0;		/* flag tells use we handled a set 	*/
	pid_t	pkgadd_id;    		/* process id of pkgadd that invoked us */
	extern	pid_t	dd_pid;		/* process reading from datastream	*/

	prog = strrchr(argv[0], '/');
	if(!prog++)
		prog = argv[0];

	(void) umask(0022);
	
	pkgadd_id = getppid();

	/*
	 * Get values from the environment.  Since we clear out the
	 * environment later on and getenv() returns pointers to the
	 * values in the current environment, let's strdup these so
	 * that we have our own copies.
	 */
	setflag = strdup(getenv("SET"));
	quiet = strdup(getenv("QUIETMODE")); 
	log = strdup(getenv("LOGMODE"));
	package = strdup(getenv("PKGINST"));
	setname = strdup(getenv("SETNAME"));
	setinstance = strdup(getenv("SETINST"));
	term = strdup(getenv("TERM"));

	logmode = 0;
	quietmode = 0;
	if(log && !strcmp(log, "true"))
		logmode++;
	if(quiet && !strcmp(quiet, "true"))
		quietmode++;

	dd_pid = -1;
	device = NULL;
	limit = 0;
	while((c = getopt(argc, argv, "x:y:z:N:Mf:p:d:m:b:r:ikna:l?")) != EOF) {
		switch(c) {

		  case 'N':
			prog = optarg;
			break;

		  case 'M':
			Mntflg++;
			break;

		  case 'p':
			dparts = ds_getinfo(optarg);
			break;

		  case 'i':
			iflag++;
			break;

		  case 'f':
			pkgdev.fstyp = optarg;
			break;

		  case 'b':
			(void) strcpy(cmdbin, optarg);
			break;

		  case 'd':
			device = optarg;
			break;

		  case 'm':
			pkgdev.mount = optarg;
			pkgdev.rdonly++;
			pkgdev.mntflg++;
			break;

		  case 'r':
			respfile = optarg;
			break;

		  case 'n':
			nointeract++;
			break;

		  case 'a':
			admnfile = optarg;
			break;

		  case 'x':
			/* 
			 * pkgadd's dd process id so that we're dealing with 
			 * the same one when we call quit() to terminate that
			 * .
			 */
			dd_pid = atol(optarg);
			break;
		
		  case 'y':
			/*
			 * If pkginstall is called with -y option, then we know
			 * that pkgadd was called to install packages off of a
			 * diskette datastream.
			 */
			pkgdev.bdevice = optarg;
			break;

		  case 'z':
			/* If pkginstall called with -z option, then we know
			 * that pkgadd/pkgask was called with a directory as
			 * an argument to the -r option.  Use it as the
			 * directory in which the response files for set member
			 * packages are to be placed.
			 */
			argrespd = optarg;
			break;

		  case 'k':
			/* Don't prompt for initial insertion of media */ 
			noprompt++;;
			break;

		  default:
			usage();
			break;
		}
	}
	if(iflag && (respfile == NULL))
		usage();

	/*
	 * Set up this pkginst's error logfile name.
	 */
	(void) sprintf(errlogfile, "%s/logs/%s.log", PKGADM, package);

	if(device) {
		if(pkgdev.mount)
			pkgdev.bdevice = device;
		else 
			pkgdev.cdevice = device;
	}
	if(pkgdev.fstyp && !pkgdev.mount) {
		progerr("-f option requires -m option");
		usage();
	}

	n = strlen(prog) - sizeof("INSTALL") + 1; /* length of INSTALL */
	if((n >= 0) && !strcmp(&prog[n], "INSTALL")) {
		srcinst = COREPKG;
		pkgdev.bdevice = argv[optind++];
		pkgdev.dirname = pkgdev.mount = argv[optind];
		pkgdev.rdonly++;
		pkgdev.mntflg++;
		(void) sprintf(cmdbin, 
			"%s/%s/reloc.1/$UBIN:%s/%s/reloc.1/$SBIN",
			pkgdev.mount, COREPKG, pkgdev.mount, COREPKG);
	} else {
		pkgdev.dirname = argv[optind++];
		srcinst = argv[optind++];
		if(optind != argc)
			usage();
	}

	(void) pkgparam(NULL, NULL);
	if(!strcmp(srcinst, COREPKG)) {
		/* if this is the core package, turn off some of the normal
		 * functionality since we are using the package itself to
		 * do this installation
		 */
		adm.mail = NULL;
		adm.space = adm.idepend = adm.runlevel = adm.action =
			adm.partial = adm.conflict = adm.setuid = "nocheck";
		adm.basedir = "default";
		/* create initial system directories */
	} else {
		/*
		 * initialize installation admin parameters 
		 */
		setadmin(admnfile);
	}

	func = signal(SIGINT, trap);
	if(func != SIG_DFL)
		(void) signal(SIGINT, func);
	(void) signal(SIGHUP, trap);

	ckdirs();
	lockinst();
	tzset();

	(void) sprintf(instdir, "%s/%s", pkgdev.dirname, srcinst);

	if(pt = getenv("TMPDIR"))
		(void) sprintf(tmpdir, "%s/installXXXXXX", pt);
	else
		(void) strcpy(tmpdir, "/tmp/installXXXXXX");
	if((mktemp(tmpdir) == NULL) || mkdir(tmpdir, 0700)) {
		progerr(ERR_MKDIR, tmpdir);
		(void) killdd();
		quit(99);
	}

	pt = strdup(getenv("TZ"));
	environ = NULL;

	/*
	 * Since we just cleared all environment variables,
	 * reset the ones required for logmode.
	 */
	putparam("LOGMODE", log);
	putparam("PKGINST", package);
	putparam("TERM", term);

	if(pt)
		putparam("TZ", pt);
	if(!cmdbin[0])
		(void) strcpy(cmdbin, PKGBIN);
	(void) sprintf(path, "%s:%s", DEFPATH, cmdbin);
	putparam("PATH", path);
	putparam("OAMBASE", "/usr/sadm/sysadm");

	(void) sprintf(p_pkginfo, "%s/%s", instdir, PKGINFO);
	(void) sprintf(p_pkgmap, "%s/%s", instdir, PKGMAP);

	/* verify existence of required information files */
	if(access(p_pkginfo, 0) || access(p_pkgmap, 0)) {
		progerr(ERR_BADFORMAT, instdir);
		(void) killdd();
		quit(99);
	}
		
	if(pkgenv(instdir, srcinst))
		quit(1);
	
	/*
	 * If this package is a set installation package then the CATEGORY 
	 * field value in the pkginfo file is the string 'set'.  Make sure
	 * that this is not an attempt at installing a set within a set by
	 * checking if the SET environment variable is set to "false".
	 */
	category = strdup(getenv("CATEGORY"));
	if (strcmp(category, "set") == 0) {
		didset++;
		(void) sprintf(listfile, "/var/tmp/set.%d", pkgadd_id); 
		if(strcmp(setflag, "true") == 0) {
			progerr(ERR_SETDUP);	/* set within a set  */
			(void) killdd();
			quit(2);		/* skip this package */
		}
		/* Create file that will contain list of member packages */
		if ((fp = fopen(listfile, "w")) == (FILE *)NULL) {
			progerr(ERR_OPENLIST, listfile);
			quit(2);		/* skip this package */
		}
		(void) fclose(fp);

		/* 
		 * If pkgadd was invoked with no -r option, then
		 * create directory that will contain response files.
		 * Otherwise, use the directory that was specified
		 * to the pkgadd command line -r option.
		 */
		if (strcmp(prog, "pkgask") != 0 && !respfile) {
			(void) sprintf(respdir, "/var/tmp/respdir.%d", pkgadd_id);
			if (mkdir(respdir, S_IRWXU) < 0) {
				/*
				 * This may be an attempt at installing a second set during
				 * one invocation of pkgadd.  In this case, the response
				 * directory already exists.  So, check if the reason for
				 * failure of the mkdir() is that the directory already exist
				 * and only exit with error if it does not.
				 */
				if (errno != EEXIST) { 
					progerr(ERR_RESPDIR, respdir);
					(void) killdd();
					quit(2);
				}
			}
		}
		else {
			if(argrespd) {
				(void) strcpy(respdir, argrespd);
			}
			else {
				progerr("the -r argument for sets must be a directory"); 
				(void) killdd();
				quit(1);
			}
		}

		(void) sprintf(setinfo, "%s/setinfo", instdir);
		(void) sprintf(setsize, "%s/install", instdir);

		/*
		 * Make environment variables known to package scripts.
		 */
		putparam("SET", setflag);
		putparam("SETLIST", listfile);
		putparam("SETINFO", setinfo);
		putparam("RESPDIR", respdir);
		putparam("SETSIZE", setsize);
	}

	/*
	 * Supply REQDIR to all packages, not only SIPs.
	 */
	(void) sprintf(reqdir, "%s/install", instdir);
	putparam("REQDIR", reqdir);

	if(setname) {
		putparam("SETNAME", setname);
		putparam("SETINST", setinstance);
	}
	putparam("QUIETMODE", quiet);

	if(pt = getenv("MAXINST"))
		maxinst = atol(pt);

	/*
	 *  verify that we are not trying to install an 
	 *  INTONLY package with no interaction
	 */
	if(pt = getenv("INTONLY")) {
		if(iflag || nointeract) {
			progerr(ERR_INTONLY, pkgabrv);
			(void) killdd();
			quit(1);
		}
	}

	echo("\n%s", pkgname);
	echo("(%s) %s", pkgarch, pkgvers);

	/*
	 *  if this script was invoked by 'pkgask', just
	 *  execute request script and quit
	 */
	if(iflag) {
		
		if(pkgdev.cdevice)
			unpack(1, 0);
		/* 
		 * During set installation, only display copyright 
		 * messages during SIP installation.
		 */
		if(strcmp(category, "set") && !setflag[0])
			copyright();

		(void) sprintf(path, "%s/install/request", instdir);
		if(access(path, 0)) {
	 		if (!setflag[0]) {
				progerr(ERR_NOREQUEST);
				(void) killdd();
				quit(1);
			}
			else
				quit(0);
		}
		ckreturn(reqexec(path, respfile, pkgstderr), ERR_REQUEST);

		if(warnflag || failflag) {
			(void) unlink(respfile);
			echo("\nResponse file <%s> was not created.", respfile);
		} else {
			if(didset) {
				echo("\nResponse files for set <%s> were created.", srcinst);
				didset = 0;
			}
			else
				echo("\nResponse file <%s> was created.", respfile);
		}
		quit(0);
	}


	/* 
	 * inspect the system to determine if any instances of the
	 * package being installed already exist on the system
	 */
	npkgs = 0;
	prvinfo = (struct pkginfo *) calloc(MALSIZ, sizeof(struct pkginfo));
	if(prvinfo == NULL) {
		progerr(ERR_MEMORY, errno);
		(void) killdd();
		quit(99);
	}
	for(;;) {
		if(pkginfo(&prvinfo[npkgs], pkgwild, NULL, NULL)) {
			if((errno == ESRCH) || (errno == ENOENT)) 
				break;
			progerr(ERR_SYSINFO, errno);
			(void) killdd();
			quit(99);
		}
		if((++npkgs % MALSIZ) == 0) {
			prvinfo = (struct pkginfo *) realloc(prvinfo, 
				(npkgs+MALSIZ) * sizeof(struct pkginfo));
			if(prvinfo == NULL) {
				progerr(ERR_MEMORY, errno);
				(void) killdd();
				quit(99);
			}
		}
	}

	if(npkgs > 0) {
		/* 
		 * since an instance of this package already exists on
		 * the system, we must interact with the user to determine
		 * if we should overwrite an instance which is already
		 * installed, or possibly install a new instance of
		 * this package.
		 */
		pkginst = getinst(prvinfo, npkgs, package);
	} else {
		/* 
		 * the first instance of a package on the system is 
		 * always identified by the package abbreviation
		 */
		pkginst = pkgabrv;
	}
	/*
	 * If the name of the selected package instance (via getinst) is
	 * different than the original instance name (e.g., pkgadd was
	 * invoked for pkga but it is being installed as pkga.2) then
	 * reset the name of the error logfile to reflect that.  We don't
	 * need to concern ourselves with the possibility of two error
	 * logfiles, since an error occuring prior to this point would have
	 * prevented us from getting here.
	 */
	if(package && strcmp(pkginst, package)) {
		(void) unlink(errlogfile);
		(void) sprintf(errlogfile, "%s/logs/%s.log", PKGADM, pkginst);
		(void) unlink(errlogfile);
	}
	
	(void) sprintf(pkgloc, "%s/%s", PKGLOC, pkginst);
	(void) sprintf(pkgbin, "%s/install", pkgloc);
	(void) sprintf(pkgsav, "%s/save", pkgloc);
	(void) sprintf(ilockfile, "%s/!I-Lock!", pkgloc);
	(void) sprintf(rlockfile, "%s/!R-Lock!", pkgloc);
	(void) sprintf(savlog, "%s/logs/%s", PKGADM, pkginst);

	putparam("PKGINST", pkginst);
	putparam("PKGSAV", pkgsav);

	/*
	 * Set up parameters to pass when exec'ing preinstall and
	 * postinstall scripts if quietmode or logmode are set.  If
	 * quietmode is set, send stdout to /dev/null.  If logmode is 
	 * set, send stderr to error logfile only.
	 */
	if(quietmode)
		pkgstdout = "/dev/null";
	if(logmode)
		pkgstderr = strdup(errlogfile);

	if(update) {
		/* read in parameter values from instance which is
		 * currently installed 
		 */
		if((fp = fopen(p_pkginfo, "r")) == NULL) {
			progerr(ERR_PKGINFO, p_pkginfo);
			(void) killdd();
			quit(99);
		}
		param[0] = '\0';
		while(value = fpkgparam(fp, param)) {
			if(!strcmp(param, "CLASSES"))
				(void) setlist(&pclass, qstrdup(value));
			else if(!getenv(param) || !strcmp("BASEDIR", param))
				putparam(param, value);
			param[0] = '\0';
		}
		(void) fclose(fp);
		putparam("UPDATE", "yes");
	}

	/*
	 *  determine if the pacakge has been partially
	 *  installed on or removed from this system
	 */
	ckpartial();

	/*
	 *  make sure current runlevel is appropriate
	 */
	ckrunlevel();

	/*
	 * determine package base directory (if this 
	 * package is relocatable)
	 */
	getbase();

	if(pkgdev.cdevice)
		/* get first volume which contains info files */
		unpack(1, 0);

	/* 
	 * During set installation, only display copyright 
	 * messages during SIP installation.
	 */
	if(strcmp(category, "set") && !setflag[0])
		copyright();

	/*
	 * If no response file has been provided, initialize response file 
	 * by executing any request script provided by this package.
	 */
	(void) sprintf(path, "%s/install/request", instdir);
	if(respfile == NULL) {
		if(n = reqexec(path, NULL, pkgstderr))
			ckreturn(n, ERR_REQUEST);
	}

	/*
	 * For SIP, display copyright after set member packages
	 * have been selected.
	 */
	if(!strcmp(category, "set"))
		copyright();

	/* look for all parameters in response file which begin
	 * with a capital letter, and place them in the
	 * environment
	 */
	if(respfile) {
		if((fp = fopen(respfile, "r")) == NULL) {
			/*
			 * If a request file exists for this package abort
			 * if a response file does not.
			 */
			if(access(path, 0) == 0) {
				progerr(ERR_RESPONSE, respfile);
				(void) killdd();
				quit(99);
			} else
				goto noresp;
		}
		param[0] = '\0';
		while(value = fpkgparam(fp, param)) {
			if(!isupper(param[0])) {
				param[0] = '\0';
				continue;
			}
			if(rdonly(param)) {
				progerr(ERR_RDONLY, param);
				param[0] = '\0';
				continue;
			}
			putparam(param, value);
			param[0] = '\0';
		}
		(void) fclose(fp);
	}
noresp:	nclass = setlist(&class, qstrdup(getenv("CLASSES")));

	/*
	 * the following two checks are done in the corresponding 
	 * ck() routine, but are repeated here to avoid re-processing
	 * the database if we are administered to not include these
	 * processes
	 */
	if(ADM(setuid, "nochange"))
		nosetuid++;
	if(ADM(conflict, "nochange"))
		nocnflct++;

	/*
	 * merg information in memory with the "contents" file; 
	 * this creates a temporary version of the "contents"
	 * file and modifies the entry in memory to reflect
	 * how the entry should look after the merg is complete
	 */

	if(ocfile(&mapfp, &tmpfp))
		quit(99);

	if((fp = fopen(p_pkgmap, "r")) == NULL) {
		progerr(ERR_PKGMAP, p_pkgmap);
		(void) killdd();
		quit(99);
	}

	/*
	 * Get the number of package parts in this
	 * package from first line in the pkgmap file.
	 */
	(void) fgets(line, BUFSIZ, fp);
	(void) strtok(line, " ");
	nparts = atoi(strtok(NULL, " "));
	(void) rewind(fp);

	(void) sortmap(fp, mapfp, tmpfp);

	if(installed > 0) {
		echo(MSG_INSTALLED, installed,
			(installed > 1) ? "pathnames are" : "pathname is");
	}

	/*
	 *  verify package information files are not corrupt
	 */
	ckpkgfiles();

	/*
	 *  verify package dependencies
	 */
	ckdepend();

	/*
	 *  check space requirements
	 */
	ckspace();

	/*
	 *  determine if any objects provided by this package
	 *  conflict with previously installed packages
	 */
	ckconflct();

	/*
	 *  determine if any objects provided by this package
	 *  will be installed with setuid or setgid enabled
	 */
	cksetuid();

	/*
	 * determine if any packaging scripts provided with
	 * this package will execute as a priviledged user
	 */
	ckpriv();

	/*
	 * if we have assumed that we were installing setuid or
	 * conflicting files, and the user chose to do otherwise,
	 * we need to read in the package map again and re-merg
	 * with the "contents" file
	 */
	if(rprcflag)
		(void) sortmap(fp, mapfp, tmpfp);
	(void) fclose(fp);

	echo("\nInstalling %s as <%s>\n", pkgname, pkginst);
	started++;

	/*
	 *  verify neccessary package installation directories exist
	 */
	ckpkgdirs();

	/*
	 *  create lockfile to indicate start of installation
	 */
	if(creat(ilockfile, 0644) < 0) {
		progerr(ERR_LOCKFILE, ilockfile);
		(void) killdd();
		quit(99);
	}

	(void) time(&clock);
	(void) cftime(cbuf, "%b %d \045Y \045I:\045M \045p", &clock);
	putparam("INSTDATE", qstrdup(cbuf));

	/*
	 *  check ulimit requirement (provided in pkginfo)
	 */
	if(pt = getenv("ULIMIT")) {
		if(!strcmp(pt, "unlimited")) {
			limit = rlp.rlim_max = rlp.rlim_cur =  RLIM_INFINITY;
		}
		else {
			limit = atol(pt);
			rlp.rlim_max = rlp.rlim_cur =  (rlim_t) (limit*FSZDIV);
		}
		if((limit <= 0) || setrlimit(RLIMIT_FSIZE, &rlp) < 0) {
			progerr(ERR_ULIMIT, limit, errno);
			(void) killdd();
			quit(99);
		}
	}

	if(opresvr4) { 
		/* we are overwriting a pre-svr4 package,
		 * so remove the file in /usr/options now
		 */
		(void) sprintf(path, "%s/%s.name", PKGOLD, pkginst);
		if(unlink(path) && (errno != ENOENT)) {
			progerr(ERR_OPRESVR4, path);
			warnflag++;
		}
	}

	/*
	 *  Execute preinstall script, if any
	 */
	(void) sprintf(script, "%s/install/preinstall", instdir);
	if(access(script, 0) == 0) {
		echo("## Executing preinstall script.");
		(void) sprintf(script, "%s/install/preinstall", instdir);
		if(n = pkgexecl(NULL, pkgstdout, pkgstderr, SHELL, script, NULL))
			ckreturn(n, ERR_PREINSTALL);
	}

	/*
	 *  Store information about package being installed;
	 *  modify installation parameters as neccessary and
	 *  copy contents of 'install' directory into $pkgloc
	 */
	merginfo();

	/* Check if postinstall script exists on package */
	(void) sprintf(script, "%s/install/postinstall", instdir);
	if(access(script, 0) != 0)
		script[0] = '\0';

	/* 
	 * If ulimit was reset for the package because of ULIMIT parameter
	 * in pkginfo file, re-establish ulimit for contents file handling.
	 */
	if(limit) {
		unlimited.rlim_max = unlimited.rlim_cur =  RLIM_INFINITY;
		if(setrlimit(RLIMIT_FSIZE, &unlimited) < 0) {
			progerr(ERR_ULIMIT, unlimited.rlim_cur, errno);
			(void) killdd();
			quit(99);
		}
	}

	/* 
	 * replace contents file with recently created temp version
	 * which contains information about the objects being installed
	 */
	(void) fclose(mapfp);
	if(swapcfile(tmpfp, (dbchg ? pkginst : NULL))) {
		quit(99);
	}

	/* Re-establish ulimit for package files */
	if((limit > 0) && setrlimit(RLIMIT_FSIZE, &rlp) < 0) {
		progerr(ERR_ULIMIT, limit, errno);
		(void) killdd();
		quit(99);
	}
	/* Install package one part (volume) at a time */
	part = 1;
	while(part <= nparts) {
		if((part > 1) && pkgdev.cdevice) {
reprompt:
			if(pkgdev.bdevice) {
				char *vol;
				vol = devattr(pkgdev.cdevice, "volume");
				(void) fprintf(stderr, "\nREADY TO PROCESS:\n");
				if(strlen(setname) > 0) 
					(void) fprintf(stderr, "  Set:     %s (%s)\n", setname, setinstance);
				(void) fprintf(stderr, "  Package: %s (%s)\n           %s %d of %d\n",
					pkgname, pkginst, vol, part, nparts);
			}
			unpack(part, nparts);
		}
		instvol(srcinst, part, nparts);
		if(part++ >= nparts)
			break;
	}

	/* Execute postinstall script if we're not currently
	 * processing a set installation package.  This should
	 * only be executed if it's a regular package.
	 */
	if (strcmp(category, "set") != 0) {
		if(script[0]) {
			(void) sprintf(script, "%s/postinstall", pkgbin);
			if(access(script, 0) == 0) {
				echo("## Executing postinstall script.");
				if(n = pkgexecl(NULL, pkgstdout, pkgstderr, SHELL, script, NULL))
					ckreturn(n, ERR_POSTINSTALL);
			}
		}
	}

	if(!warnflag && !failflag) {
		if(pt = getenv("PREDEPEND"))
			predepend(pt);
		(void) unlink(rlockfile);
		(void) unlink(ilockfile);
		(void) unlink(savlog);
	}

	/* set levels on all installation files */
	(void) setinst(PKGADM);
	(void) setinst(pkgloc);

	quit(0);
	/*NOTREACHED*/
}

void
ckreturn(retcode, msg)
int	retcode;
char	*msg;
{
	switch(retcode) {
	  case 2:
	  case 12:
	  case 22:
		warnflag++;
		if(msg)
			progerr(msg);
		/* fall through */
	  case 10:
	  case 20:
		if(retcode >= 10)
			reboot++;
		if(retcode >= 20)
			ireboot++;
		/* fall through */
	  case 0:
		break; /* okay */

	  case -1:
		retcode = 99;
	  case 99:
	  case 1:
	  case 11:
	  case 21:
	  case 4:
	  case 14:
	  case 24:
	  case 5:
	  case 15:
	  case 25:
		if(msg)
			progerr(msg);
		/* fall through */
	  case 3:
	  case 13:
	  case 23:
	  case NOSET:
		(void) killdd();
		quit(retcode);
		/* NOT REACHED */
	  default:
		if(msg)
			progerr(msg);
		(void) killdd();
		quit(1);
	}
}

#define COPYRIGHT 	"install/copyright"
#define	SETCOPYRIGHT	"/tmp/setcopyright"

static void
copyright()
{
	FILE	*fp;
	char	*pt;
	char	line[LSIZE], crpath[PATH_MAX];

	if(strcmp(category, "set"))
		(void) sprintf(crpath, "%s/%s", instdir, COPYRIGHT);
	else
		(void) sprintf(crpath, "%s", SETCOPYRIGHT);

	if((fp = fopen(crpath, "r")) == NULL) {
		if(pt=getenv("VENDOR"))
			echo(pt);
	} else {
		while(fgets(line, LSIZE, fp))
			(void) fprintf(stderr, "%s", line);
		(void) fclose(fp);
	}
}


static int
rdonly(p)
char *p;
{
	int	i;

	for(i=0; ro_params[i]; i++) {
		if(!strcmp(p, ro_params[i]))
			return(1);
	}
	return(0);
}
	
static void
unpack(part, nparts)
int	part, nparts;
{
	int n = 0;

	/*
	 * read in next part from stream, even if we decide
	 * later that we don't need it
	 */
	if(dparts < 1) {
		progerr(ERR_DSTREAMCNT);
		(void) killdd();
		quit(99);
	}
	if((access(instdir, 0) == 0) && rrmdir(instdir)) {
		progerr(ERR_RMDIR, instdir);
		(void) killdd();
		quit(99);
	}
	if(mkdir(instdir, 0755)) {
		progerr(ERR_MKDIR, instdir);
		(void) killdd();
		quit(99);
	}
	if(chdir(instdir)) {
		progerr(ERR_CHDIR, instdir);
		(void) killdd();
		quit(99);
	}
	dparts--;
	if(n = ds_next(pkgdev.cdevice, instdir)) {
		if(n != 3 || !pkgdev.bdevice)
			progerr(ERR_DSTREAM);
		(void) killdd();
		quit(99);
	}
	if(chdir(PKGADM)) {
		progerr(ERR_CHDIR, PKGADM);
		(void) killdd();
		quit(99);
	}
}

static void
setinst(dir)
char *dir;
{
	struct dirent 	*dp;
	DIR		*pdirfp;
	char		path[PATH_MAX];
	struct stat	statbuf;
	level_t		lvl;

	/* this routine sets the default level on all 
	 * files under dir as well as dir itself
	 */
	if((pdirfp = opendir(dir)) == NULL) 
		return;
	lvl = sys_pub;
	while((dp = readdir(pdirfp)) != NULL) {
		if(dp->d_name[0] == '.')
			continue;
		
		(void) sprintf(path, "%s/%s", dir, dp->d_name);
		/* set default level */
		(void) setlvl(path, &lvl);
		if(stat(path, &statbuf) == 0) {
			if(statbuf.st_mode & 040000)
				setinst(path);
		}
	}
	(void) closedir(pdirfp);
	(void) setlvl(dir, &lvl);
}

static void
usage()
{
	progerr("usage: %s [-d device] [-m mountpt [-f fstyp]] ", prog);
	progerr("[-b bindir] [-a adminf] [-r respf] [-z responsedir] ");
	progerr("directory pkginst\n");
	(void) killdd();
	exit(1);
}

extern	pid_t	dd_pid;
siginfo_t	infop;

extern void 
killdd()
{
	/*
	 * If the datastream media is diskette, the pkginstall process 
	 * hangs.  To prevent this, kill the dd process reading from 
	 * the datastream diskette device.
	 */

	if (dd_pid > 0 && pkgdev.bdevice) {
		(void) sigsend(P_PID, dd_pid, SIGKILL);
		if(waitid(P_PID, dd_pid, &infop, WSTOPPED||WEXITED) < 0) {
			/* If dd_pid is no longer around disregard */
			if (errno != ECHILD) {
				progerr("could not terminate dd process");
				exit(1); 
			}
		}
		dd_pid = -1;
	}
}
