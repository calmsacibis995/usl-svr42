/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oampkg:common/cmd/oampkg/pkginfo/pkginfo.c	1.16.11.14"
#ident  "$Header: $"

#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <dirent.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <pkginfo.h>
#include <sys/types.h>
#include <pkgstrct.h>
#include <pkglocs.h>
#include <errno.h>
#include <pkgdev.h>
#include <unistd.h>


extern char	*optarg;
extern char	*pkgdir;
extern int	optind;
extern char	*errstr;

extern void	*calloc(), 
		exit(),
		logerr(),
		progerr();
extern int	getopt(),
		access(),
		devtype(),
		srchcfile(),
		pkghead(),
		gpkgmap();
extern char	*pkgparam();

#define nblock(size)	((size + (BLKSIZ - 1)) / BLKSIZ)
#define	BLKSIZ		512
#define MAXCATG	64

/*
 * The rules for displaying information on sets are:
 *
 *      If the category has been specified via "pkginfo -c set", information 
 *	on set installation packages installed on the system will be produced.
 *
 *      If a set name has been specified on the command line and if the set
 *	has been installed on the system, information on the set member packages
 *	that are also installed on the system is displayed.  
 *
 *      If set information on a set that is located on an installation media is 
 *	requested, information on all set member packages is displayed.  In the
 *	case of a tape, all options to pkginfo are allowed since information on
 *	the set member packages is readily available on the first archive on the
 *	tape (contains all set member packages' pkginfo and pkgmap files).  If
 *	the set is made up of multiple diskettes only short reports are possible.
 *	This is because pkginfo cannot access information files for the set member
 *	packages that are located on subsequent diskettes.  In this case, pkginfo
 *	can only produce a short listing (the default) by using the SIP's setinfo
 *	file.
 */

#define IFDISPLAY	ncatg || pkgcnt || (strcmp(info.catg, "set") != 0)

char	*prog;
char	*pkginst;

static char	*device = NULL;
static char	*paramlist[] = {
	"DESC", "PSTAMP", "INSTDATE", "VSTOCK", "SERIALNUM", "HOTLINE", 
	"EMAIL", NULL 
};

static char	contents[PATH_MAX];
static int	errflg = 0;
static int	qflag = 0;
static int	iflag = -1;
static int	pflag = -1;
static int	lflag = 0;
static int	Lflag = 0;
static int	Nflag = 0;
static int	xflag = 0;
static struct 	cfent	entry;
static char	**pkg = NULL;
static int	pkgcnt = 0;
static char	*ckcatg[MAXCATG] = {NULL};
static int	ncatg = 0;
static char	*ckvers = NULL;
static char	*ckarch = NULL;
static char	setinfo[PATH_MAX];	/* setinfo full pathname	*/
static char	*setinst = NULL;	/* save set pkg instance name	*/
static FILE	*fp;			/* used to open setinfo file	*/
struct cfstat	*choice;
struct pkgdev	pkgdev;			/* used to determine type of device */
int	output;
int	cflag = 0;

static struct cfstat {
	char	pkginst[15];
	short	exec;
	short	dirs;
	short	link;
	short	partial;
	short	spooled;
	short	installed;
	short	info;
	short	shared;
	short	setuid;
	long	tblks;
	struct cfstat *next;
} *data;
static struct pkginfo info;

static struct cfstat	*fpkg();
static int	iscatg(),
		select(); 
static void	usage(), 
		look_for_installed(), 
		report(), 
		rdcontents(),
		getinfo(),
		pkgusage(),
		dumpinfo(); 
static	char	arguments[BUFSIZ];

/* 
 *New for diskette media and sets.
 */
int	cmdarg = 0;
char	*mem_pkginst, 
	*mem_name,
	*mem_catg;

main(argc,argv)
int	argc;
char	**argv;
{
	int	c;
	int	dflag = 0;
	int	args = 0;

	prog = strrchr(argv[0], '/');
	if(!prog++)
		prog = argv[0];

	while ((c = getopt(argc,argv,"LNxv:a:d:qpilc:?")) != EOF) {
		args++;
		switch(c) {
		  case 'v':
			ckvers = optarg;
			cmdarg++;
			(void) sprintf(arguments, "%s -v \"%s\"", arguments, ckvers); 
			break;

		  case 'a':
			ckarch = optarg;
			cmdarg++;
			(void) sprintf(arguments, "%s -a \"%s\"", arguments, ckarch); 
			break;

		  case 'd':
			/* -d could specify stream or mountable device */
			dflag++;
			device = optarg;
			break;

		  case 'q':
			qflag++;
			(void) sprintf(arguments, "%s -q", arguments); 
			break;

		  case 'i':
			iflag = 1;
			if(pflag > 0)
				usage();
			pflag = 0;
			(void) sprintf(arguments, "%s -i", arguments); 
			break;

		  case 'p':
			pflag = 1;
			if(iflag > 0)
				usage();
			iflag = 0;
			(void) sprintf(arguments, "%s -p", arguments); 
			break;

		  case 'N':
			Nflag++;
			cmdarg++;
			(void) sprintf(arguments, "%s -N", arguments); 
			break;

		  case 'L':
			if(xflag || lflag) {
				progerr("-L and -l/-x flags are incompatable");
				usage();
			}
			Lflag++;
			cmdarg++;
			(void) sprintf(arguments, "%s -L", arguments); 
			break;

		  case 'l':
			if(xflag) {
				progerr("-l and -x flags are incompatable");
				usage();
			}
			cmdarg++;
			lflag++;
			(void) sprintf(arguments, "%s -l", arguments); 
			break;

		  case 'x':
			if(lflag) {
				progerr("-l and -x flags are not compatable");
				usage();
			}
			xflag++;
			(void) sprintf(arguments, "%s -x", arguments); 
			break;

		  case 'c':
			cflag++;
			ckcatg[ncatg++] = strtok(optarg, " \t\n,");
			while(ckcatg[ncatg] = strtok(NULL, " \t\n,"))
				ncatg++;
			(void) sprintf(arguments, "%s -c %s", arguments, optarg); 
			break;

		  default:
			usage();
		}
	}
	
	pkg = &argv[optind];
	pkgcnt = (argc - optind);

	if(pkg[0] && !strcmp(pkg[0], "all")) {
		pkgcnt = 0;
		pkg[0] = NULL;
	} 

	if(pkgdir == NULL)
		pkgdir = PKGLOC;

	/* convert device appropriately */
	if(pkghead(device))
		exit(1);

	/*
	 * If device was specified, get information on it.
	 */
	if(devtype(device, &pkgdev)) {
		progerr("bad device <%s> specified", device);
		exit(1);
	}

	look_for_installed();

	if(lflag && !strcmp(pkgdir, PKGLOC)) {
		/* look at contents file */
		(void) sprintf(contents, "%s/contents", PKGADM);
		rdcontents();
	}

	report();

	/*
	 * :XENIX Support:
	 * Only display these if no options or if -l or -i options 
	 * were specified. In this case, also show packages installed 
	 * via the "custom" utility.
	 */
	/*
	if(ckvers || ckarch || dflag || qflag || pflag || xflag || cflag )
		;
	*/
	if(!args && pkgcnt == 0) {
		if(access("/usr/bin/displaypkg", X_OK||R_OK) == 0) {
			(void) fflush(stdout);
			system ("/usr/bin/displaypkg XENIX");
		}
	}

	(void) pkghead(NULL);
	exit(errflg ? 1 : 0);
}

char *fmt = "%10s:  %s\n";

static void
report()
{
	struct cfstat *dp;
	int	nsetpkgs;
	int	set = 0;
	int	i, n;
	char	member[BUFSIZ];
	char	buffer[BUFSIZ];
	char	cmd[BUFSIZ];
	char	*pmember;
	char	**pkglist;
	int	diskette = 0;
	int	gotit = 0;
	int	set_installed = 0;

	output = 0;
	for(;;) {
		choice = (struct cfstat *)0;
		for(dp=data; dp; dp=dp->next) {
			/* get information about this package */
			if(dp->installed < 0)
				continue; /* already used */
			if(Lflag && pkgcnt) {
				choice = dp;
				break;
			} else if(!choice ||
			  (strcmp(choice->pkginst, dp->pkginst) > 0))
				choice = dp;
		}
		if(!choice)
			break; /* no more packages */

		if(pkginfo(&info, choice->pkginst, ckarch, ckvers)) {
			choice->installed = (-1);
			continue;
		}

		/* is it in an appropriate category? */
		if(iscatg(info.catg)) {
			choice->installed = (-1);
			continue;
		}



		/*
		 * If the category for the current package is "set", no -c option 
		 * was specified and one or more package instances were specified
		 * on the command line, report information on the set's member 
		 * packages rather than on the SIP(s) themselves.  If the set is
		 * installed, report only on those set member packages that are 
		 * also installed.  If the set is not installed and the packages are
		 * spooled or on tape media, report on all that set's member packages.
		 * If the packages are located on file system or datastream diskettes,
		 * we'll only be able to do a short report by using the setinfo file
		 * to simulate what we would get from the set member package's pkginfo
		 * file.
		 */ 
		if (!strcmp(info.catg, "set"))
			/* Save SIP name for later */
			setinst = strdup(info.pkginst);
		if (setinst && !cflag && pkgcnt > 0) {
			if(info.status == PI_INSTALLED)
				set_installed++;
			set++;
			/*
			 * Get member package names from setinfo file.
			 * Skip reporting on the set installation 
			 * package itself.
			 */
			(void) sprintf(setinfo, "%s/%s/setinfo", pkgdir, choice->pkginst);
			if((fp = fopen(setinfo, "r")) == NULL) {
				progerr("Could not open setinfo file for <%s>",
						choice->pkginst);
				exit(1);
			}

			/* 
			 * Get names of set member packages from setinfo file.  Check if
			 * the package is on a diskette media. If so, we'll only be able
			 * to do a short listing by using the contents of the setinfo file.
			 * For this case, if -a, -l, -L, -n or -v options were specified
			 * exit with an error.  If installed or on tape media, just build
			 * a pkginfo command line with all set member packages on it to be
			 * exec'ed. NOTE: This may require additional smarts when CD-ROM
			 * comes into the picture.
			 */
			if(pkgdev.bdevice) {
				/* Diskettes */
				diskette++;
				/* Allocate array for set member names */
				pkglist = (char **)calloc(120, sizeof(char **));
			}
			(void) memset(member, '\0', BUFSIZ);
			for(nsetpkgs = 0; fgets(buffer,BUFSIZ,fp) != NULL; nsetpkgs++) {
				if(buffer[0] == '#' || buffer[0] == ' ' || buffer[0] == '\n')
					continue;
				if (!diskette) {
					pmember =  strdup(strtok(buffer, "\t"));
					if(set_installed) {
						(void) pkginfo(&info, NULL);
						(void) pkginfo(&info, pmember, NULL, NULL);
						if(info.status != PI_INSTALLED)
							continue;
						else
							gotit++;
					}	
					(void) strcat(member, pmember);
					(void) strcat(member, " ");
				}
				else {
					/*
					 * For diskette media use the SIP's setinfo file to produce a list
					 * of set member packages.  No information beyond category, pkginst
					 * and package name is available.
					 */
					if (cmdarg && !cflag) {
						progerr("detailed set member package information is not available");
						exit(1);
					}
					mem_pkginst = strtok(buffer, "\t");
					if (mem_pkginst == NULL) {
						progerr("could not obtain set member package instance name");
						exit(1);
					}
					(void) strtok(NULL, "\t");	/* skip parts field */
					(void) strtok(NULL, "\t");	/* skip default y/n field */
					mem_catg = strtok(NULL, "\t");
					if (mem_catg == NULL) {
						progerr("could not obtain set member package category");
						exit(1);
					}
					mem_name = strdup(strtok(NULL, "\n"));
					if (mem_name == NULL) {
						progerr("could not obtain set member package name");
						exit(1);
					}
					(void) fprintf(stdout, "%-11.11s %-14.14s %s\n", mem_catg,
						mem_pkginst, mem_name);
				}
			}
			if (diskette) {
				pkglist[nsetpkgs] = NULL;
				(void) fprintf(stderr, "\n");
			}

			(void) fclose(fp);	/* Close setinfo file */

			/*
			 * For installed SIP or for one that is located on tape,
			 * build pkginfo command line that will be exec'ed for set
			 * member package instances.  For installed sets, only
			 * report on those member packages that are also installed.
			 */
			if (!diskette && gotit) {
				(void)sprintf(cmd, "pkginfo -d %s %s %s", pkgdir, arguments, member);
				if(n = esystem(cmd, -1, -1)) {
					rpterr();
					progerr("unexpected error re-execing pkginfo command");
					exit(1);
				}
				/*
				 * Reset set members string to null in case we're asking
				 * for information on more than one set. Otherwise, we'll
				 * get information on this set's packages displayed along
				 * with the next set's members.
				 */
				 (void) strcpy(member, NULL);
			}

			/* 
			 * Set set installation package instance in pkg[] to null so
			 * that we don't get an error message stating that information 
			 * for it was not found.
			 */
			for(i=0; i < pkgcnt; i++)
				if (!strcmp(pkg[i], choice->pkginst)) {
					pkg[i] = NULL;
					break;
				}
			/*
			 * Force the "for(dp=data; ... )" loop to go on to the next
			 * package instance specified on the command line, if any.
			 */
			choice->installed = -1;

		}

		/*
		 * If this was a set installation package, then the report
		 * was done from the exec'ed pkginstall -- skip it here.
		 */
		if (!set) {
			if (doreport())
				return;
		} else
			set--;
	}
	/* verify that each package listed on command line was output */
	for(i=0; i < pkgcnt; ++i) {
		if(pkg[i]) {
			logerr("ERROR: information for \"%s\" was not found", 
				pkg[i]);
			errflg++;
		}
	}
	(void) pkginfo(&info, NULL); /* free up all memory and open fds */
}

static int
doreport()
{
	int i;

	if(!pflag &&  
		/* don't include partially installed packages */
		(choice->partial || (info.status == PI_PARTIAL) || 
			(info.status == PI_UNKNOWN))) {
		/*
		 * If we're reporting only fully installed
		 * package don't incrament error flag.
		 */
		if(!iflag)
			errflg++;
		choice->installed = (-1);
		return(0);
	}

	if(Nflag && (info.status == PI_PRESVR4)) {
		/* don't include preSVR4 packages */
		choice->installed = (-1);
		return(0);
	}

	if(!iflag && ((info.status == PI_INSTALLED) ||
			(info.status == PI_PRESVR4))) {
		/* don't include completely installed packages */
		choice->installed = (-1);
		return(0);
	}

	output++;
	dumpinfo(choice);
	choice->installed = (-1);
	if(pkgcnt && !qflag) {
		i = select(choice->pkginst);
		if(i >= 0)
			pkg[i] = NULL;
	}

	if(!output)
		errflg++;

	if(qflag)
		return(1);

	return(0);
}

static void
dumpinfo(dp)
struct cfstat *dp;
{
	register int i;
	char *pt, category[128];

	if(qflag)
		return; /* print nothing */

	if(Lflag) {
		if(IFDISPLAY)
			(void) puts(info.pkginst);
		return;
	} else if(xflag) {
		if(IFDISPLAY) {
			(void) printf("%-14.14s  %s\n", info.pkginst, info.name);
			if(info.arch || info.version) 
				(void) printf("%14.14s  ", "");
			if(info.arch)
				(void) printf("(%s) ", info.arch);
			if(info.version)
				(void) printf("%s", info.version);
			if(info.arch || info.version)
				(void) printf("\n");
		}
		return;
	} else if(!lflag) {
		if(info.catg)
			(void) sscanf(info.catg, "%[^, \t\n]", category);
		else if(info.status == PI_PRESVR4)
			(void) strcpy(category, "preSVR4");
		else
			(void) strcpy(category, "(unknown)");
		if(IFDISPLAY)
			(void) fprintf(stdout, "%-11.11s %-14.14s %s\n", category,
					info.pkginst, info.name);
		return;
	}
	if(IFDISPLAY) {
		if(info.pkginst)
			(void) printf(fmt, "PKGINST", info.pkginst);
		if(info.name)
			(void) printf(fmt, "NAME", info.name);
		if(lflag && info.catg)
			(void) printf(fmt, "CATEGORY", info.catg);
		if(lflag && info.arch)
			(void) printf(fmt, "ARCH", info.arch);
		if(info.version)
			(void) printf(fmt, "VERSION", info.version);
		if(info.basedir)
			(void) printf(fmt, "BASEDIR", info.basedir);
		if(info.vendor)
			(void) printf(fmt, "VENDOR", info.vendor);
	
		if(info.status == PI_PRESVR4)
			(void) printf(fmt, "STATUS", "preSVR4");
		else {
			for(i=0; paramlist[i]; ++i) {
				if((pt = pkgparam(info.pkginst, paramlist[i])) && *pt)
					(void) printf(fmt, paramlist[i], pt);
			}
			if(info.status == PI_SPOOLED)
				(void) printf(fmt, "STATUS", "spooled");
			else if(info.status == PI_PARTIAL)
				(void) printf(fmt, "STATUS", "partially installed");
			else if(info.status == PI_INSTALLED)
				(void) printf(fmt, "STATUS", "completely installed");
			else
				(void) printf(fmt, "STATUS", "(unknown)");
		}
	}
	(void) pkgparam(NULL, NULL);

	if(!lflag) {
		(void) putchar('\n');
		return;
	}

	/*
	 * If -l option was used and no pkginst specified, we must check that this
	 * is not a SIP instance so that we don't display FILES information on the
	 * set installation package itself.  In the case where category "set" was
	 * specified to -c option and this is a SIP, we want to display FILES
	 * information on the SIP itself.
	 */
	if (strcmp(setinst, info.pkginst) || cflag)  {
		if(info.status != PI_PRESVR4) {
			if(strcmp(pkgdir, PKGLOC))
				getinfo(dp);
	
			if(dp->spooled)
				(void) printf("%10s:  %5d spooled pathnames\n", 
					"FILES", dp->spooled);
			if(dp->installed)
				(void) printf("%10s:  %5d installed pathnames\n", 
					"FILES", dp->installed);
			if(dp->partial)
				(void) printf("%18d partially installed pathnames\n", 
					dp->partial);
			if(dp->shared)
				(void) printf("%18d shared pathnames\n", 
					dp->shared);
			if(dp->link)
				(void) printf("%18d linked files\n", dp->link);
			if(dp->dirs)
				(void) printf("%18d directories\n", dp->dirs);
			if(dp->exec)
				(void) printf("%18d executables\n", dp->exec);
			if(dp->setuid)
				(void) printf("%18d setuid/setgid executables\n", 
					dp->setuid);
			if(dp->info)
				(void) printf("%18d package information files\n", 
					dp->info+1); /* pkgmap counts! */
			
			if(dp->tblks)
				(void) printf("%18ld blocks used (approx)\n", 
					dp->tblks);
		}
		(void) putchar('\n');
	}
	/* reset */
	setinst = NULL;
}

static struct cfstat *
fpkg(pkginst)
char *pkginst;
{
	struct cfstat *dp, *last;

	dp = data;
	last = (struct cfstat *)0;
	while(dp) {
		if(!strcmp(dp->pkginst, pkginst))
			return(dp);
		last = dp;
		dp = dp->next;
	}
	dp = (struct cfstat *)calloc(1, sizeof(struct cfstat));
	if(!dp) {
		progerr("no memory, malloc() failed");
		exit(1);
	}
	if(!last)
		data = dp;
	else
		last->next = dp; /* link list */
	(void) strncpy(dp->pkginst, pkginst, 14);
	return(dp);
}
	
#define SEPAR	','

static int
iscatg(list)
char *list;
{
	register int i;
	register char *pt;
	int	match;

	if(!ckcatg[0])
		return(0); /* no specification implies all packages */
	if(info.status == PI_PRESVR4) {
		for(i=0; ckcatg[i]; ) {
			if(!strcmp(ckcatg[i++], "preSVR4"))
				return(0);
		}
		return(1);
	}
	if(!list)
		return(1); /* no category specified in pkginfo is a bug */

	match = 0;
	do {
		if(pt = strchr(list, ','))
			*pt = '\0';

		for(i=0; ckcatg[i]; ) {
			if(!strcmp(list, ckcatg[i++])) {
				match++;	
				break;
			}
		}

		if(pt)
			*pt++ = ',';
		if(match)
			return(0);
		list = pt; /* points to next one */
	} while(pt);
	return(1);
}

static void
look_for_installed()
{
	struct dirent *drp;
	struct stat	status;
	DIR	*dirfp;
	char	path[PATH_MAX];
	int	n;

	if(!strcmp(pkgdir, PKGLOC) && (dirfp = opendir(PKGOLD))) {
		while(drp = readdir(dirfp)) {
			if(drp->d_name[0] == '.')
				continue;
			n = strlen(drp->d_name);
			if((n > 5) && !strcmp(&drp->d_name[n-5], ".name")) {
				(void) sprintf(path, "%s/%s", PKGOLD, 
					drp->d_name);
				if(lstat(path, &status))
					continue;
				if((status.st_mode & S_IFMT) != S_IFREG)
					continue;
				drp->d_name[n-5] = '\0';
				if(!pkgcnt || (select(drp->d_name) >= 0))
					(void) fpkg(drp->d_name);
			}
		}
		(void) closedir(dirfp);
	}

	if((dirfp = opendir(pkgdir)) == NULL)
		return;

	while(drp = readdir(dirfp)) {
		if(drp->d_name[0] == '.')
			continue;

		if(pkgcnt && (select(drp->d_name) < 0))
			continue;

		(void) sprintf(path, "%s/%s/pkginfo", pkgdir, drp->d_name);
		if(access(path, 0))
			continue; /* doesn't appear to be a package */
		(void) fpkg(drp->d_name);
	}
	(void) closedir(dirfp);
}

static int
select(p)
char *p;
{
	register int i;

	for(i=0; i < pkgcnt; ++i) {
		if(pkg[i] && (pkgnmchk(p, pkg[i], 1) == 0))
			return(i);
	}
	return(-1);
}

static void
rdcontents()
{
	FILE *fp1;
	struct cfstat *dp;
	struct pinfo *pinfo;
	int n;

	if((fp1 = fopen(contents, "r")) == NULL) {
		progerr("unable open \"%s\" for reading", contents);
		exit(1);
	}

	/* check the contents file to look for referenced packages */
	while((n = srchcfile(&entry, "*", fp1, NULL)) > 0) {
		for(pinfo=entry.pinfo; pinfo; pinfo=pinfo->next) {
			/* see if entry is used by indicated packaged */
			if(pkgcnt && (select(pinfo->pkg) < 0))
				continue;

			dp = fpkg(pinfo->pkg);
			pkgusage(dp, &entry);

			if(entry.npkgs > 1)
				dp->shared++;

			if(pinfo->status)
				dp->partial++;
			else
				dp->installed++;
		}
	}
	if(n < 0) {
		progerr("bad entry read in contents file");
		logerr("pathname: %s", entry.path);
		logerr("problem: %s", errstr);
		exit(1);
	}

	(void) fclose(fp1);
}

static void
getinfo(dp)
struct cfstat	*dp;
{
	FILE *fp;
	int n;
	char pkgmap[256];

	(void) sprintf(pkgmap, "%s/%s/pkgmap", pkgdir, dp->pkginst);
	if((fp = fopen(pkgmap, "r")) == NULL) {
		progerr("unable open \"%s\" for reading", pkgmap);
		exit(1);
	}
	dp->spooled = 1; /* pkgmap counts! */
	while((n = gpkgmap(&entry, fp)) > 0) {
		dp->spooled++;
		pkgusage(dp, &entry);
	}
	if(n < 0) {
		progerr("bad entry read in pkgmap file");
		logerr("pathname: %s", entry.path);
		logerr("problem: %s", errstr);
		exit(1);
	}
	(void) fclose(fp);
}
	
static void
pkgusage(dp, pentry)
struct cfstat	*dp;
struct cfent	*pentry;
{
	if(pentry->ftype == 'i') {
		dp->info++;
		return;
	} else if(pentry->ftype == 'l') {
		dp->link++;
	} else {
		if((pentry->ftype == 'd') || (pentry->ftype == 'x'))
			dp->dirs++;
		if(pentry->ainfo.mode & 06000)
			dp->setuid++;
		if(!strchr("dxcbp", pentry->ftype) && 
		(pentry->ainfo.mode & 0111))
			dp->exec++;
	}

	if(strchr("ifve", pentry->ftype)) 
		dp->tblks += nblock(pentry->cinfo.size);
}

static void
usage()
{
	(void) fprintf(stderr, "usage:\n");
	(void) fprintf(stderr,
	"  %s [-q] [-p|-i] [-x|-l] [options] [pkg ...]\n", prog);
	(void) fprintf(stderr,
	"  %s -d device [-q] [-x|-l] [options] [pkg ...]\n", prog);
	(void) fprintf(stderr, "where\n");
	(void) fprintf(stderr, "  -q #quiet mode\n");
	(void) fprintf(stderr, "  -p #select partially installed packages\n");
	(void) fprintf(stderr, "  -i #select completely installed packages\n");
	(void) fprintf(stderr, "  -x #extracted listing\n"); 
	(void) fprintf(stderr, "  -l #long listing\n"); 
	(void) fprintf(stderr, "and options may include:\n");
	(void) fprintf(stderr, "  -c category,[category...]\n");
	(void) fprintf(stderr, "  -a architecture\n");
	(void) fprintf(stderr, "  -v version\n");
	exit(1);
}
