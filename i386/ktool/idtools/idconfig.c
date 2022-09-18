/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)idtools:i386/ktool/idtools/idconfig.c	1.29"
#ident	"$Header:"

/* Config for Installable Drivers and Tunable Parameters */

/*
 *  This program is adapted from the program of the same name
 *  written for the AT&T PC 6300+ computer.  Several things have
 *  been changed, however:
 *
 *      *  Some of the 'type' fields read in the 'mdevice' master
 *         file have been eliminated or reassigned to new meanings.
 *         Other type flags have been added.  The flags were changed
 *         to support STREAMS devices and File System types; features
 *         not included in the 6300+.
 *
 *      *  All the stuff included to support 'sharable' device
 *         drivers in a merged UNIX/DOS environment has been
 *         removed for now.
 *
 *      *  Idconfig now generates config information for filesystems.
 *
 *	*  Idconfig now generates a dispatcher class table
 *	   and exec switch table to support kernel dispatcher
 *	   and exec modules.
 *
 */

#include "inst.h"
#include "defines.h"
#include "devconf.h"
#include "mdep.h"

/*
 * In a cross-environment, make sure these headers are for the host system
 */
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <sys/stat.h>

/*
 * In a cross-environment, make sure this header is for the target system
 */
#include <sys/mod.h>

#define USAGE	"Usage: idconfig [-i|o|r|C dir] [-T|d|t|a|c|h|p file] [[-M mod] ...]\n"

typedef struct tune tune_t;
typedef struct per_assign assign_t;

/* Parameter table - store mtune and stune */
struct tune {
	struct mtune	mtune;		/* config info from mtune */
	long		value;		/* value from stune */
	short		conf;		/* was it specified in stune? */
	tune_t		*next;		/* next tune struct in tune_info list */
} *tune_info;

/* Sassign table */
struct per_assign {
	struct sassign	sasgn;		/* config info from sassign */
	driver_t	*driver;	/* driver for this object */
	assign_t	*next;		/* next assign in assign_info list */
} *assign_info;


/* extern declarations */
ctlr_t *sfind();
driver_t *mfind();
tune_t *tfind();
FILE    *open1();       /* open a file */
char    *uppermap();
int defentries(), rddev(), drvpostproc(), rdmtune(), rdstune(), rdsassign();
int prconf(), prdef(), print_vfssw(), prfiles(), prloadables(), ckerror();

extern  char *optarg;   /* used by getopt */

/* flags */
extern	short   eflag;  /* error in configuration */
short   debug   = 0;    /* debug flag */
short   rflag   = 0;    /* root directory */
short   iflag   = 0;    /* directory containing input files */
short   oflag   = 0;    /* directory for output files */
short   mflag   = 0;    /* loadable module specified */

/* buffers */
char    root[512];              /* root directory containing cf and packages */
char    input[512];             /* directory containing input files */
char    output[512];            /* directory for output files */
char    path[512];              /* construct path names */
extern char    current[];           /* current directory */

/* extern variables which control getinst() filenames */
extern char instroot[];
extern char pathinst[];

/* output file names */
char    *cfile = "conf.c";      /* configuration table file */
char    *hfile = "config.h";    /* configuration header file */
char    *pfile = "direct";      /* pathnames of driver file for link-edit */
char    *lfile = "modlist";     /* list of loadable modules */
char    *mfile = "mod_conf.c";  /* per loadable module configuration file */
char    *sfile = "mod_sec.s";   /* per loadable module special section file */
char    *gfile = "mod_reg";     /* per loadable module registration file */


struct LIST  {
        int (*add)();                   /* functions called by main program */
	char *name;			/* name of function */
} list[] = {
	defentries,	"defentries",	/* Setup entry-point definitions */
        rdmtune,        "rdmtune",      /* Master tunable parameter file */
        rdstune,        "rdstune",      /* System tunable parameter file */
	rddev,		"rddev",	/* Master and System device files */
	drvpostproc,	"drvpostproc",	/* Per-driver post-processing */
        rdsassign,      "rdsassign",    /* System assign file */
        ckerror,        "ckerror",      /* check for errors */
	prconf,		"prconf",	/* conf.c */
        prdef,          "prdef",        /* config.h */
        prfiles,        "prfiles",      /* remaining output files */
	prloadables,	"prloadables",	/* output files for loadable modules */
        ckerror,        "ckerror",      /* check for errors */
        NULL,           ""
};

unsigned short	file_mode = 0664;	/* File mode for new files */
unsigned short	cur_umask;

/* Built-in entry-point types */
struct entry_def *edef_open, *edef_close, *edef_read, *edef_write, *edef_ioctl;
struct entry_def *edef_chpoll, *edef_mmap, *edef_segmap, *edef_size;
struct entry_def *edef_strategy, *edef_print, *edef_intr, *edef_msgio;
struct entry_def *edef_exec, *edef_core, *edef__init, *edef_init;

/* "Entry-point" types for variables */
struct entry_def *edef_devflag, *edef_extmodname;
struct entry_def *edef_magic, *edef_info, *edef__tty, *edef__wrapper;

/* switch table sizes */
int bdevswsz;
int cdevswsz;
int vfsswsz;
int fmodswsz;

int bdev_reserve, cdev_reserve, vfs_reserve, fmod_reserve;

struct modlist *modlist;
extern int noloadable;

main(argc,argv)
int argc;
char *argv[];
{
        int m;
        struct LIST *p;
	struct modlist *mod;

        while((m=getopt(argc, argv, "?M:SV:#si:o:r:D:T:d:t:a:c:h:p:m:")) != EOF )
                switch(m) {

                case 'T':
                        ftypes[MTUN].fname = optarg;
                        break;
                case 'd':
                        ftypes[SDEV].fname = optarg;
                        break;
                case 't':
                        ftypes[STUN].fname = optarg;
                        break;
                case 'a':
                        ftypes[SASN].fname = optarg;
                        break;
                case 'c':
                        cfile = optarg;
                        break;
                case 'h':
                        hfile = optarg;
                        break;
                case 'p':
                        pfile = optarg;
                        break;
                case 'D':
                case 'm':
                case 's':
                        /* Obsolete; ignore */
                        break;
                case '#':
                        debug++;
                        break;
		case 'V':
			debug = atoi(optarg);
			break;
                case 'r':
                        strcpy(root, optarg);
                        rflag++;
                        break;
                case 'i':
                        strcpy(input, optarg);
                        iflag++;
                        break;
                case 'o':
                        strcpy(output, optarg);
                        oflag++;
                        break;
		case 'M':
			mflag++;
			if (strlen(optarg) >= NAMESZ)
				fatal("argv[0]: module name \"%s\" specified is longer than %d", 
					optarg, NAMESZ - 1);
			mod = (struct modlist *)malloc(sizeof(struct modlist));
			strncpy(mod->name, optarg, NAMESZ);
			mod->next = modlist;
			modlist = mod;
			break;
		case 'S':
			noloadable = 1;
			break;
                case '?':
                        fprintf(stderr, USAGE);
                        exit(1);
                }

	if (mflag)
		noloadable = 0;

	eflag = 0;

        /* Get full path names for root, input, and output directories */
        getcwd(current, LINESZ);
        getpath(rflag, root, ROOT);
        sprintf(path, "%s/%s", root, CFDIR);
        getpath(iflag, input, path);
        getpath(oflag, output, path);

        if (debug)
                fprintf(stdout, "Root: %s\nInput: %s\nOutput: %s\n\n",
                        root, input, output);

	/* Tell getinst() about directory names */
	strcpy(instroot, root);
	strcpy(pathinst, input);

	/* Modify file_mode by umask */
	cur_umask = umask(0);
	umask(cur_umask);
	file_mode &= ~cur_umask;

        /* call each function */
        for (p = &list[0]; p->add != NULL; p++) {
                if (debug)
                        fprintf(stdout, "Main: Before %s\n", p->name);
                (*p->add)();
        }

        exit(0);
}


/* check if error occurred */

ckerror()
{
        if (eflag) {
                sprintf(errbuf, "Errors encountered. Configuration terminated.\n");
                fatal(0);
        }
}



/* This routine is used to search the Parameter table
 * for the keyword that was specified in the configuration.  If the
 * keyword cannot be found in this table, a NULL is returned.
 * If the keyword is found, a pointer to that entry is returned.
 */
tune_t *
tfind(keyword)
char *keyword;
{
        register tune_t *tune;

	for (tune = tune_info; tune != NULL; tune = tune->next) {
                if (equal(keyword, tune->mtune.name))
                        return(tune);
        }
        return(NULL);
}



/* This routine is used to map lower case alphabetics into upper case. */

char *
uppermap(device,caps)
char *device;
char *caps;
{
        register char *ptr;
        register char *ptr2;
        ptr2 = caps;
        for (ptr = device; *ptr != NULL; ptr++) {
                if ('a' <= *ptr && *ptr <= 'z')
                        *ptr2++ = *ptr + 'A' - 'a';
                else
                        *ptr2++ = *ptr;
        }
        *ptr2 = NULL;
        return (caps);
}



/* open a file */

FILE *
open1(file, mode, dir)
char *file, *mode;
int dir;
{
        FILE *fp;
        char *p;

        switch (dir) {
        case IN:
                sprintf(path, "%s/%s", input, file);
                p = path;
                break;
        case OUT:
                sprintf(path, "%s/%s", output, file);
                p = path;
                break;
        case FULL:
                p = file;
                break;
        }

        if (debug)
                fprintf(stdout, "Open: mode=%s path=%s\n", mode, p);

        if ((fp = fopen(p, mode)) == NULL) {
                sprintf(errbuf, FOPEN, p, mode);
                fatal(0);
        }
        return(fp);
}


/* set up entry-point definitions */

#define DEFINE_ENTRY(edef_var, suffix) \
	if (((edef_var) = define_entry(suffix, 0)) == NULL) { \
		sprintf(errbuf, TABMEM, "entry-point"); \
		fatal(1); \
	}
#define DEFINE_VAR(edef_var, suffix) \
	if (((edef_var) = define_entry(suffix, 1)) == NULL) { \
		sprintf(errbuf, TABMEM, "build-in variable"); \
		fatal(1); \
	}
#define DEFINE_FTAB(suffix, tabname, ret_type) \
	if (define_ftab(suffix, tabname, ret_type) == NULL) { \
		sprintf(errbuf, TABMEM, "function table"); \
		fatal(1); \
	}

defentries()
{
	/* Define built-in entry-point types */
	DEFINE_ENTRY(edef_open, "open");
	DEFINE_ENTRY(edef_close, "close");
	DEFINE_ENTRY(edef_read, "read");
	DEFINE_ENTRY(edef_write, "write");
	DEFINE_ENTRY(edef_ioctl, "ioctl");
	DEFINE_ENTRY(edef_chpoll, "chpoll");
	DEFINE_ENTRY(edef_msgio, "msgio");
	DEFINE_ENTRY(edef_mmap, "mmap");
	DEFINE_ENTRY(edef_segmap, "segmap");
	DEFINE_ENTRY(edef_size, "size");
	DEFINE_ENTRY(edef_strategy, "strategy");
	DEFINE_ENTRY(edef_print, "print");
	DEFINE_ENTRY(edef_intr, "intr");
	DEFINE_ENTRY(edef_exec, "exec");
	DEFINE_ENTRY(edef_core, "core");
	DEFINE_ENTRY(edef_init, "init");
	DEFINE_ENTRY(edef__init, "_init");

	/* Define built-in variable names */
	DEFINE_VAR(edef_devflag, "devflag");
	DEFINE_VAR(edef_extmodname, "extmodname");
	DEFINE_VAR(edef_magic, "magic");
	DEFINE_VAR(edef_info, "info");
	DEFINE_VAR(edef__tty, "_tty");
	DEFINE_VAR(edef__wrapper, "_wrapper");

	/* Define built-in function tables */

	DEFINE_FTAB("init", "io_init", "void");
	DEFINE_FTAB("start", "io_start", "void");
	DEFINE_FTAB("poll", "io_poll", "void");
	DEFINE_FTAB("halt", "io_halt", "void");
	DEFINE_FTAB("kenter", "io_kenter", "void");
	DEFINE_FTAB("kexit", "io_kexit", "void");
}


/* read mtune - Master tune file */

rdmtune()
{
	struct mtune mtune;
        register tune_t *tun;
	int stat;

	getinst(MTUN, RESET, NULL);

	while ((stat = getinst(MTUN, NEXT, &mtune)) == 1) {
		if ((tun = (tune_t *)malloc(sizeof(tune_t))) == NULL) {
			sprintf(errbuf, TABMEM, "mtune");
			fatal(1);
		}
                tun->mtune = mtune;
		tun->value = mtune.def;
		tun->conf = 0;
		tun->next = tune_info;
		tune_info = tun;
        }

	if (stat != 0)
		insterror(stat, MTUN, "");
}


/* check System device conflicts */

chksdev(drv, sdp)
	driver_t *drv;
	struct sdev *sdp;
{
	if (INSTRING(drv->mdev.mflags, ONCE) && (sfind(sdp->name) != NULL)) {
		sprintf(errbuf, ONESPEC, sdp->name);
		error(1);
		return(0);
	}

	/* Check for out-of-range values and conflicts in interrupt vectors,
	 * I/O addresses, and controller memory addresses.
	 * Skip this check for execsw modules because the fields are interpreted
	 * differently. Filesystem and scheduler modules are also skipped since
	 * they don't use those fields.
	 */
	if (!INSTRING(drv->mdev.mflags, EXECSW) && 
		!INSTRING(drv->mdev.mflags, FILESYS) &&
		!INSTRING(drv->mdev.mflags, DISP) &&
		!mdep_check(sdp, drv))
		return(0);

	return(1);
}


/* read Master and System info */

rddev()
{
	return getdevconf(chksdev);
}


/* per-driver post-processing */

drvpostproc()
{
        register driver_t *drv;
	struct entry_list *elistp;
	char fname[512];

	mdep_drvpostproc();

        /* check for missing required devices
	 * and get function lists for configured drivers */
	for (drv = driver_info; drv != NULL; drv = drv->next) {
		if (INSTRING(drv->mdev.mflags, REQ) && drv->n_ctlr == 0) {
                        sprintf(errbuf, MISS, drv->mdev.name);
                        fatal(0);
                }
		if (drv->n_ctlr == 0)
			continue;
		if (drv->loadable)
			lookup_entry(edef__wrapper->suffix,
				 &drv->mdev.entries, 0);
		sprintf(fname, "%s/pack.d/%s/Driver.o", root, drv->mdev.name);
		/* Determine which entry-points are actually present */
		if (get_funcs(fname, drv->mdev.prefix) == -1) {
			sprintf(errbuf, RDSYM, fname);
			fatal(0);
		}
		/* Make sure all specified entry-points are present */
		elistp = drv->mdev.entries;
		for (; elistp != NULL; elistp = elistp->next) {
			if (!elistp->edef->has_sym) {
				if (strcmp(elistp->edef->suffix, "_wrapper"))
					sprintf(errbuf, ENTRYNP, fname,
						elistp->edef->suffix);
				else
					sprintf(errbuf,
						"%s is not loadable", fname);
				fatal(0);
			}
		}
		/* Determine which variables are present */
		drv->vars = 0;
		if (edef_extmodname->has_sym)
			drv->vars |= V_EXTMODNAME;
		if (edef_magic->has_sym)
			drv->vars |= V_MAGIC;
		if (edef_info->has_sym)
			drv->vars |= V_INFO;
		/* For version 0 compatibility, FCOMPAT flag implies devflag */
		/* None version 0 drivers should always have devflag */
		if (!INSTRING(drv->mdev.mflags, COMPAT) ||
			INSTRING(drv->mdev.mflags, FCOMPAT))
			drv->vars |= V_DEVFLAG;
	}
}


/* rdstune - System tunable parameter file */

rdstune()
{
	struct stune stune;
	register tune_t *tune;
	int stat;

	getinst(STUN, RESET, NULL);

	while ((stat = getinst(STUN, NEXT, &stune)) == 1) {
		/* find tunable in Parameter table */
		tune = tfind(stune.name);
		if (tune == NULL) {
			sprintf(errbuf, TUNE, stune.name);
			error(0);
			continue;
		}

                /* check if already specified */
                if (tune->conf) {
                        sprintf(errbuf, RESPEC, stune.name);
                        error(0);
                        continue;
                }

                /* check whether parameter is within min and max */
                if (stune.value < tune->mtune.min ||
		    stune.value > tune->mtune.max) {
                        printf(errbuf, PARM, stune.name,
			       stune.value, tune->mtune.min, tune->mtune.max);
                        error(1);
			continue;
                }

                /* store value in Parameter table */
		tune->value = stune.value;

                /* indicate tunable parameter specified */
                tune->conf = 1;
        }

	if (stat != 0)
		insterror(stat, STUN, "");
}


/* read sassign - System assignment file */

rdsassign()
{
	struct sassign sassign;
	register assign_t *assign;
	int highminor;
	int stat;

	getinst(SASN, RESET, NULL);

	while ((stat = getinst(SASN, NEXT, &sassign)) == 1) {
                if ((assign = (assign_t *)malloc(sizeof(assign_t))) == NULL) {
			sprintf(errbuf, TABMEM, "sassign");
                        fatal(1);
                }

		if ((assign->driver = mfind(sassign.major)) == NULL) {
			sprintf(errbuf, UNK, sassign.major);
			error(1);
			return(NULL);
		}
		if (!INSTRING(assign->driver->mdev.mflags, BLOCK) &&
		    !INSTRING(assign->driver->mdev.mflags, CHAR)) {
			sprintf(errbuf, DEVREQ, sassign.major);
			error(1);
		}

		highminor = max_minor(assign->driver);
		if (sassign.minor < 0 || sassign.minor > highminor) {
			sprintf(errbuf, MINOR, 0, highminor);
			error(1);
		}

		assign->sasgn = sassign;
		assign->next = assign_info;
		assign_info = assign;
        }

	if (stat != 0)
		insterror(stat, SASN, sassign.device);
}


int
max_minor(driver)
	driver_t *driver;
{
	register tune_t *t_maxminor;

	if (driver->vars & V_DEVFLAG) {
		if ((t_maxminor = tfind("MAXMINOR")) == NULL) {
			sprintf(errbuf, NOMAXMINOR, (long)OMAXMIN);
			warning(0);
		} else 
			return (int)(t_maxminor->conf ?
				     t_maxminor->value : t_maxminor->mtune.def);
	}
	return OMAXMIN;
}



/* print out configuration header file */

prdef()
{
        register FILE *fp;
        register driver_t *drv;
        register ctlr_t *ctlr;
        char caps[PFXSZ];

        fp = open1(hfile, "w", OUT);
        chmod(hfile, file_mode);

        /* go through Master table */
        {
	register int j;
	int HowMany;

        fprintf(fp, "/* defines for each device */\n");

        for (drv = driver_info; drv != NULL; drv = drv->next) {

                /* skip devices that are not configured */
                if (drv->n_ctlr == 0)
                        continue;

		/* skip devices with no prefix */
		if (drv->mdev.prefix[0] == '\0' ||
		    drv->mdev.prefix[0] == '-')
			continue;

                uppermap(drv->mdev.prefix, caps);
                fprintf(fp, "\n#define\t%s\t\t1\n", caps);
                fprintf(fp, "#define\t%s_CNTLS\t%hd\n", caps, drv->n_ctlr);
                fprintf(fp, "#define\t%s_UNITS\t%hd\n", caps, drv->tot_units);

		if (INSTRING(drv->mdev.mflags, BLOCK)) {
			HowMany = drv->mdev.blk_end - drv->mdev.blk_start + 1;
                	fprintf(fp, "#define\t%s_BMAJORS\t%hd\n",
					caps, HowMany);
			for (j = 0; j < HowMany; j++)
				fprintf(fp, "#define\t%s_BMAJOR_%hd\t%hd\n",
					caps, j, drv->mdev.blk_start + j);
		}
		if (INSTRING(drv->mdev.mflags, CHAR)) {
			HowMany = drv->mdev.chr_end - drv->mdev.chr_start + 1;
                	fprintf(fp, "#define\t%s_CMAJORS\t%hd\n",
					caps, HowMany);
			for (j = 0; j < HowMany; j++)
				fprintf(fp, "#define\t%s_CMAJOR_%hd\t%hd\n",
					caps, j, drv->mdev.chr_start + j);
		}

		mdep_prdrvconf(fp, drv, caps);
        }
        }

        /* go through per-controller table */
        {
        fprintf(fp, "\n\n/* defines for each controller */\n");

        for (ctlr = ctlr_info; ctlr != NULL; ctlr = ctlr->next) {
		drv = ctlr->driver;

		/* skip special modules */
		if (INSTRING(drv->mdev.mflags, FILESYS) ||
		    INSTRING(drv->mdev.mflags, EXECSW) ||
		    INSTRING(drv->mdev.mflags, DISP))
			continue;

		/* skip devices with no prefix */
		if (drv->mdev.prefix[0] == '\0' ||
		    drv->mdev.prefix[0] == '-')
			continue;

                uppermap(drv->mdev.prefix, caps);

		fprintf(fp, "\n#define\t%s_%hd\t\t%hd\n",
			caps, ctlr->num, ctlr->sdev.units);

		mdep_prctlrconf(fp, ctlr, caps);
        }
        }

        /* go through tunable Parameter table */
        {
        register tune_t *tune;
        
        fprintf(fp, "\n/* defines for each tunable parameter */\n");

	for (tune = tune_info; tune != NULL; tune = tune->next) {
                fprintf(fp, "#define\t%s\t%ld\n",
                        tune->mtune.name, tune->value);
	}
        }

	/* pseudo-tunables */
	fprintf(fp, "\n");
	fprintf(fp, "#define BDEVSWSZ\t%d\n", bdevswsz);
	fprintf(fp, "#define CDEVSWSZ\t%d\n", cdevswsz);
	fprintf(fp, "#define VFSSWSZ\t%d\n", vfsswsz);
	fprintf(fp, "#define FMODSWSZ\t%d\n", fmodswsz);

        fclose(fp);
}


void print_execsw();

/* print out configuration table file (conf.c) */
prconf()
{
	register FILE *fp;
	register driver_t *drv, *rdrv;
	driver_t nodrv;
	assign_t *assign;
	char *pfx, *name;
	char *mflags;
	int olddev_done = 0;
	int is_blk, is_chr;
	int i, j;
	int driver, module;
	char	xbf[256];	/* buffer for external symbol definitions */
	char	caps[NAMESZ];	/* buffer for upper case device prefix */
	int ndrv_static;	/* number of statically linked modules */
	tune_t *tune;
	int empty_slot;

	fp = open1(cfile, "w", OUT);
	chmod(cfile, file_mode);

	fprintf(fp, "#include <sys/types.h>\n");
	fprintf(fp, "#include <sys/sysmacros.h>\n");
	fprintf(fp, "#include <sys/conf.h>\n");
	fprintf(fp, "#include <sys/class.h>\n");
	fprintf(fp, "#include <sys/exec.h>\n");
	fprintf(fp, "#include <sys/vfs.h>\n");

	fprintf(fp, "#include <vm/bootconf.h>\n");

	fprintf(fp, "\nextern int nodev(), nxio(), nulldev(), nosys();\n");
	fprintf(fp, "#define nulliob (struct iobuf *)0\n");
	fprintf(fp, "#define nulltty (struct tty *)0\n");
	fprintf(fp, "#define nullinfo (struct streamtab *)0\n");
	fprintf(fp, "int nodevflag = 0;\n");

/*
 * Print the function tables for miscellaneous entry points.
 */
	print_func_tables(fp);

	fprintf(fp, "\n\n");

/*
 * Search the Master table and generate an extern statement for
 * any routines that are needed.
 *
 * Declare the required streamtab structures here, as well.
 */
	ndrv_static = 0;
	for (drv = driver_info; drv != NULL; drv = drv->next) {
		/* if not configured or loadable, continue */
		if (drv->n_ctlr == 0 || drv->loadable)
			continue;

		ndrv_static++;

		pfx = drv->mdev.prefix;
		mflags = drv->mdev.mflags;

		/* declare streamtab structures here */

		if (INSTRING(mflags, STREAM)) {
			if (!(drv->vars & V_INFO)) {
				sprintf(errbuf, STRTAB, drv->mdev.name);
				error(0);
			}
			fprintf(fp, "extern struct streamtab %sinfo;\n", pfx);
		}

		/* declare exec switch structures here */

		if (INSTRING(mflags, EXECSW)) {
			if (drv->n_ctlr != 1) {
				sprintf(errbuf, ONESPEC, drv->mdev.name);
				error(0);
			}
			if (!(drv->vars & V_MAGIC)) {
				sprintf(errbuf, EXMAG, drv->mdev.name);
				error(0);
			}
			if (!drv_has_entry(drv, edef_exec)) {
				sprintf(errbuf, EXRT, drv->mdev.name);
				error(0);
			}
			fprintf(fp, "extern short %smagic[%d];\n", pfx,
				drv->ctlrs->sdev.units);
			fprintf(fp, "extern int %sexec();\n", pfx);
			if (drv_has_entry(drv, edef_core))
				fprintf(fp, "extern int %score();\n", pfx);
		}

		/* skip special modules */
		if (INSTRING(mflags, FILESYS) ||
		    INSTRING(mflags, EXECSW) ||
		    INSTRING(mflags, DISP))
			continue;

		/* is this a block device? */
		is_blk = INSTRING(mflags, BLOCK);
		is_chr = INSTRING(mflags, CHAR);

		/* the rest only applies to drivers and streams modules */
		if (!is_blk && !is_chr && !INSTRING(mflags, STREAM))
			continue;

		if (drv->vars & V_DEVFLAG)
			fprintf(fp, "extern int %sdevflag;\n", pfx);
		else if (!olddev_done) {
			fprintf(fp, "int olddevflag = D_OLD;\n");
			olddev_done = 1;
		}

		/* the rest only applies to drivers */
		if (!is_blk && !is_chr)
			continue;

		strcpy(xbf, "extern int ");

		if (drv_has_entry(drv, edef_open))
			sprintf(xbf + strlen(xbf), "%sopen(), ", pfx);
		else if (is_blk) {
			sprintf(errbuf, OPRT, drv->mdev.name);
			error(0);
		}
		if (drv_has_entry(drv, edef_close))
			sprintf(xbf + strlen(xbf), "%sclose(), ", pfx);
		else if (is_blk) {
			sprintf(errbuf, CLRT, drv->mdev.name);
			error(0);
		}
		if (drv_has_entry(drv, edef_read))
			sprintf(xbf + strlen(xbf), "%sread(), ", pfx);
		if (drv_has_entry(drv, edef_write))
			sprintf(xbf + strlen(xbf), "%swrite(), ", pfx);
		if (drv_has_entry(drv, edef_ioctl))
			sprintf(xbf + strlen(xbf), "%sioctl(), ", pfx);
		if (is_blk) {
			if (!drv_has_entry(drv, edef_strategy)) {
				sprintf(errbuf, STRAT, drv->mdev.name);
				error(0);
			}
			sprintf(xbf + strlen(xbf), "%sstrategy(), ", pfx);
			if (drv_has_entry(drv, edef_print))
				sprintf(xbf + strlen(xbf), "%sprint(), ", pfx);
			if (drv_has_entry(drv, edef_size))
				sprintf(xbf + strlen(xbf), "%ssize(), ", pfx);
		}
		if (drv_has_entry(drv, edef_chpoll))
			sprintf(xbf + strlen(xbf), "%schpoll(), ", pfx);
		if (drv_has_entry(drv, edef_mmap))
			sprintf(xbf + strlen(xbf), "%smmap(), ", pfx);
		if (drv_has_entry(drv, edef_segmap))
			sprintf(xbf + strlen(xbf), "%ssegmap(), ", pfx);

		/* If there were no decls, trunc xbf[] to 0 length */
		if (xbf[strlen(xbf)-2] != ',')
			xbf[0] = '\0';
		else
			strcpy(&xbf[strlen(xbf)-2], ";");
		fprintf(fp, "%s\n", xbf);

		if (INSTRING(mflags, TTYS))
			fprintf(fp, "extern struct tty %s_tty[];\n", pfx);

		mdep_devsw_decl(fp, drv);
	}

/* 
 * get the number of reserved slot for bdevsw, cdevsw, vfssw, and fmodsw
 */
	if ((tune = tfind("BDEV_RESERVE")) == NULL)
		bdev_reserve = DEF_BDEV_RESERVE;
	else
		bdev_reserve = tune->value;

	if ((tune = tfind("CDEV_RESERVE")) == NULL)
		cdev_reserve = DEF_CDEV_RESERVE;
	else
		cdev_reserve = tune->value;

	if ((tune = tfind("VFS_RESERVE")) == NULL)
		vfs_reserve = DEF_VFS_RESERVE;
	else
		vfs_reserve = tune->value;

	if ((tune = tfind("FMOD_RESERVE")) == NULL)
		fmod_reserve = DEF_FMOD_RESERVE;
	else
		fmod_reserve = tune->value;

/*
 * Go through block device table and indicate addresses of required routines.
 * If a particular device is not present, fill in "nxio/nodev" entries.
 */

	memset((char *)&nodrv, 0, sizeof nodrv);
	strcpy(nodrv.mdev.prefix, "no");
	strcpy(nodrv.mdev.name, "nodev");
	nodrv.vars = V_DEVFLAG;

	empty_slot = 0;
	fprintf(fp, "\nstruct bdevsw bdevsw[] = {\n");
	for (rdrv = bdevices, j = 0; rdrv != NULL; ++j) {
		if (j >= rdrv->mdev.blk_start && !rdrv->loadable)
			drv = rdrv;
		else {
			empty_slot++;
			drv = &nodrv;
		}

		pfx = drv->mdev.prefix;
		name = drv->mdev.name;
		mflags = drv->mdev.mflags;
		is_blk = INSTRING(mflags, BLOCK);

		if (j != 0)
			fprintf(fp, ",\n");
		fprintf(fp, "/* %2d */  {\t", j);

		if (drv_has_entry(drv, edef_open))
			fprintf(fp, "%sopen,", pfx);
		else {
			if (drv == rdrv)
				fprintf(fp, "nodev,");
			else
				fprintf(fp, "nxio,");
		}
		if (drv_has_entry(drv, edef_close))
			fprintf(fp, "\t%sclose,", pfx);
		else
			fprintf(fp, "\tnodev,");

		if (is_blk)
			fprintf(fp, "\t%sstrategy,", pfx);
		else
			fprintf(fp, "\tnodev,");
		if (is_blk && drv_has_entry(drv, edef_print))
			fprintf(fp, "\t%sprint,\n", pfx);
		else
			fprintf(fp, "\tnodev,\n");

		if (is_blk && drv_has_entry(drv, edef_size))
			fprintf(fp, "\t\t%ssize,", pfx);
		else
			fprintf(fp, "\t\tnulldev,");

		fprintf(fp, "\t\"%s\",\n", name);

		fprintf(fp, "\t\tnulliob,");

		mdep_bdevsw(fp, drv);

		if (drv->vars & V_DEVFLAG)
			fprintf(fp, "\t&%sdevflag }", pfx);
		else
			fprintf(fp, "\t&olddevflag }");

		if (j == rdrv->mdev.blk_end)
			rdrv = rdrv->bdev_link;
	}

	if (empty_slot >= bdev_reserve)
		bdevswsz = j;
	else
		bdevswsz = j + bdev_reserve - empty_slot;

	if (bdevswsz == 0)
		fprintf(fp, "\t{ nodev }");

	while (j < bdevswsz) {
		fprintf(fp, ",\n/* %2d */  {\t", j);
		fprintf(fp, "nxio,\tnodev,\tnodev,\tnodev,\n");
		fprintf(fp, "\t\tnulldev,\t\"nodev\",\n");
		fprintf(fp, "\t\tnulliob,\t&nodevflag }");
		j++;
	}
		
	fprintf(fp,"\n};\n\n");
	fprintf(fp, "int bdevcnt = %d;\n", j);
	fprintf(fp, "int bdevswsz = %d;\n", bdevswsz);
	fprintf(fp, "\nstruct bdevsw shadowbsw[%d];\n", bdevswsz == 0 ? 1 : bdevswsz);

/*
 * Go through character device table and indicate addresses of required
 * routines, or indicate "nulldev" if routine is not present.  If a
 * particular device is not present, fill in "nxio/nodev" entries.
 *
 * Add streamtab pointers for STREAMS drivers; they don't need
 * any other fields in this table to be filled in. 
 */
	empty_slot = 0;
	fprintf(fp, "\nstruct cdevsw cdevsw[] = {\n");
	for (rdrv = cdevices, j = 0; rdrv != NULL; ++j) {
		if (j >= rdrv->mdev.chr_start && !rdrv->loadable)
			drv = rdrv;
		else {
			empty_slot++;
			drv = &nodrv;
		}

		pfx = drv->mdev.prefix;
		name = drv->mdev.name;
		mflags = drv->mdev.mflags;

                if (j != 0)
			fprintf(fp, ",\n");
                fprintf(fp, "/* %2d */  {\t", j);

		/*
		 * OPEN & CLOSE for char devices are special:
		 * if they are missing, they get nulldev instead of nodev;
		 * but if there's no driver at all, they should be nodev
		 * for CLOSE and nxio for OPEN.
		 */
		if (drv_has_entry(drv, edef_open))
			fprintf(fp, "%sopen,", pfx);
		else if (drv == rdrv)
			fprintf(fp, "nulldev,");
		else
			fprintf(fp, "nxio,");
		if (drv_has_entry(drv, edef_close))
			fprintf(fp, "\t%sclose,", pfx);
		else if (drv == rdrv)
			fprintf(fp, "\tnulldev,");
		else
			fprintf(fp, "\tnodev,");

		if (drv_has_entry(drv, edef_read))
			fprintf(fp, "\t%sread,", pfx);
		else
			fprintf(fp, "\tnodev,");
		if (drv_has_entry(drv, edef_write))
			fprintf(fp, "\t%swrite,", pfx);
		else
			fprintf(fp, "\tnodev,");
		if (drv_has_entry(drv, edef_ioctl))
			fprintf(fp, "\t%sioctl,\n", pfx);
		else
			fprintf(fp, "\tnodev,\n");
		if (drv_has_entry(drv, edef_mmap))
			fprintf(fp, "\t\t%smmap,", pfx);
		else
			fprintf(fp, "\t\tnodev,");
		if (drv_has_entry(drv, edef_segmap))
			fprintf(fp, "\t%ssegmap,", pfx);
		else
			fprintf(fp, "\tnodev,");
		if (drv_has_entry(drv, edef_chpoll))
			fprintf(fp, "\t%schpoll,", pfx);
		else
			fprintf(fp, "\tnodev,");
		if (INSTRING(mflags, TTYS))
			fprintf(fp, "\t\t%s_tty,\n", pfx);
		else
			fprintf(fp, "\t\tnulltty,\n");
		if (INSTRING(mflags, STREAM))
			fprintf(fp, "\t\t&%sinfo,", pfx);
		else
			fprintf(fp, "\t\tnullinfo,");
		fprintf(fp, "\t\"%s\",", name);

		mdep_cdevsw(fp, drv);

		if (drv->vars & V_DEVFLAG)
			fprintf(fp, "\t&%sdevflag }", pfx);
		else
			fprintf(fp, "\t&olddevflag }");

		if (j == rdrv->mdev.chr_end)
			rdrv = rdrv->cdev_link;
        }

	if (empty_slot >= cdev_reserve)
		cdevswsz = j;
	else
		cdevswsz = j + cdev_reserve - empty_slot;

	if (cdevswsz == 0)
		fprintf(fp, "\t{ nodev }");
	while (j < cdevswsz) {
		fprintf(fp, ",\n/* %2d */  {\t", j);
		fprintf(fp, "nxio,\tnodev,\tnodev,\tnodev,\tnodev,\n");
		fprintf(fp, "\t\tnodev,\tnodev,\tnodev,\tnulltty,\tnullinfo,\n");
		fprintf(fp, "\t\t\"nodev\",\t&nodevflag }");
		j++;
	}
        fprintf(fp, "\n};\n\n");
        fprintf(fp, "int cdevcnt = %d;\n", j);
        fprintf(fp, "int cdevswsz = %d;\n", cdevswsz);
	fprintf(fp, "\nstruct cdevsw shadowcsw[%d];\n", cdevswsz == 0 ? 1 : cdevswsz);

/*
 * Print the fmodsw table.  STREAMS installables are processed as follows:
 *
 * if mdevice flags field has:
 *      "S", this is a module, give it fmodsw entry (retain for back compat.)
 *      "Sm", this is a module, give it fmodsw entry (correct form)
 *      "Sc", this is a driver, no fmodsw entry.
 *      "Smc", this is a driver & module, give it fmodsw entry.
 */

        fprintf(fp, "\nstruct fmodsw fmodsw[] = {\n");
	j = 0;
	for (drv = driver_info; drv != NULL; drv = drv->next) {
		/* not configured or loadable */
		if (drv->n_ctlr == 0 || drv->loadable)	
                        continue;
                driver = module = 0;
                if (INSTRING(drv->mdev.mflags, STREAM)) {
                        if (INSTRING(drv->mdev.mflags, CHAR))
                                driver++;
                        if (INSTRING(drv->mdev.mflags, MOD))
                                module++;
                        if (!driver)
                                module++;
                        if (module) {
                                /* This one's a stream module. */
                                if (j++ != 0 )
					fprintf(fp, ",\n");
				fprintf(fp, "\t{ \"%s\", &%sinfo,",
					drv->mdev.name, drv->mdev.prefix);
				if (drv->vars & V_DEVFLAG)
					fprintf(fp, "\t&%sdevflag }",
						drv->mdev.prefix);
				else
					fprintf(fp, "\t&olddevflag }");
                	}
        	}
	}

	fmodswsz = j + fmod_reserve;

        if (fmodswsz == 0)
                fprintf(fp, "\t{ \"\" }");
	for (i = j; i < fmodswsz; i++)
		fprintf(fp, ",\n\t{ \"\",\tNULL,\tNULL }");
        fprintf(fp, "\n};\n\n");
        fprintf(fp, "int fmodcnt = %d;\n\n", j);
        fprintf(fp, "int fmodswsz = %d;\n\n", fmodswsz);

/*
 * Print out device variable assignments.
 */

	for (assign = assign_info; assign != NULL; assign = assign->next) {
		fprintf(fp, "dev_t\t%sdev = makedevice(%hd, %hd);\n",
			assign->sasgn.device, assign->driver->mdev.blk_start,
			assign->sasgn.minor);
		fprintf(fp, "struct bootobj %sfile = { \"\", ",
			assign->sasgn.device);
		fprintf(fp, "\"%s\", 0, %d, %d };\n",
			assign->sasgn.objname, assign->sasgn.low,
			assign->sasgn.blocks);
	}

/*
 * Print out dispatcher class table.
 */

	fprintf(fp, "\nextern void sys_init();\n");
	for (drv = driver_info; drv != NULL; drv = drv->next) {
		if (drv->n_ctlr == 0 || !INSTRING(drv->mdev.mflags, DISP))
			continue;
		if (!drv_has_entry(drv, edef__init)) {
			sprintf(errbuf, DINITRT, drv->mdev.name);
			error(0);
		}
		fprintf(fp, "extern void %s_init();\n", drv->mdev.prefix);

		if (drv->vars & V_EXTMODNAME) {
			fprintf(fp, "extern char %sextmodname[];\n",
				drv->mdev.prefix);
		}
	}

	fprintf(fp, "class_t class[] = {\n");
	fprintf(fp, "\t{ \"SYS\", sys_init },\n");
	j = 1;
	for (drv = driver_info; drv != NULL; drv = drv->next) {
		if (drv->n_ctlr == 0 || !INSTRING(drv->mdev.mflags, DISP))
			continue;
		if (drv->vars & V_EXTMODNAME) {
			fprintf(fp, "\t{ %sextmodname, %s_init },\n",
				drv->mdev.prefix, drv->mdev.prefix);
		} else {
			uppermap(drv->mdev.prefix, caps);
			fprintf(fp, "\t{ \"%s\", %s_init },\n",
				caps, drv->mdev.prefix);
		}
		j++;
	}
	fprintf(fp, "};\n\n");

	fprintf(fp, "int nclass = %d;\n\n", j);

	print_execsw(fp);

	print_vfssw(fp);

	mdep_prconf(fp);

	mdep_prvec(fp);
	print_static(fp, ndrv_static);

        fclose(fp);
}


print_func_tables(fp)
FILE *fp;
{
	extern struct ftab_def *ftab_defs;
	struct ftab_def *ftab;

	for (ftab = ftab_defs; ftab != NULL; ftab = ftab->next)
		print_func_table(fp, ftab);
}

print_func_table(fp, ftab)
FILE *fp;
register struct ftab_def *ftab;
{
	register driver_t *drv;

	/* First pass for declarations */
	fprintf(fp, "\n");
        for (drv = driver_info; drv != NULL; drv = drv->next) {
		/* For loadable modules or not configured modules, just skip it */
		if (drv->n_ctlr == 0 || drv->loadable)
                        continue;
		/* For "init" table, skip filesystem modules */
		if (ftab->entry == edef_init &&
		    INSTRING(drv->mdev.mflags, FILESYS))
			continue;
		if (drv_has_entry(drv, ftab->entry)) {
			fprintf(fp, "extern %s %s%s();\n",
				    ftab->ret_type,
				    drv->mdev.prefix, ftab->entry->suffix);
		}
        }

	/* Second pass to generate table */
        fprintf(fp, "\n%s (*%s[])() = {\n", ftab->ret_type, ftab->tabname);
        for (drv = driver_info; drv != NULL; drv = drv->next) {
		/* For loadable modules or not configured modules, just skip it */
		if (drv->n_ctlr == 0 || drv->loadable)
                        continue;
		/* For "init" table, skip filesystem modules */
		if (ftab->entry == edef_init &&
		    INSTRING(drv->mdev.mflags, FILESYS))
			continue;
		if (drv_has_entry(drv, ftab->entry)) {
			fprintf(fp, "\t%s%s,\n",
				    drv->mdev.prefix, ftab->entry->suffix);
		}
        }
        fprintf(fp, "\t(%s (*)())0\n};\n", ftab->ret_type);
}


void
print_execsw(fp)
FILE *fp;
{
	register driver_t *drv;
	register ctlr_t *ctlr;
	register char *pfx;
	int n_magic, dflt_hdlr;
	int j, k;

        fprintf(fp, "struct execsw execsw[] = {\n");
	j = 0;
	for (drv = driver_info; drv != NULL; drv = drv->next) {
		if (!INSTRING(drv->mdev.mflags, EXECSW) || drv->n_ctlr == 0)
			continue;
		ctlr = drv->ctlrs;
		n_magic = ctlr->sdev.units;
		dflt_hdlr = (ctlr->sdev.itype != 0);
		pfx = drv->mdev.prefix;
		for (k = 0; k < n_magic + dflt_hdlr; k++) {
			if (j++ != 0)
				fprintf(fp, ",\n");
			if (k < n_magic)
				fprintf(fp, "\t{ %smagic+%d", pfx, k);
			else
				fprintf(fp, "\t{ NULL"); /* dflt_hdlr */
			fprintf(fp, ", %sexec, ", pfx);
			if (drv_has_entry(drv, edef_core))
				fprintf(fp, "%score }", pfx);
			else
				fprintf(fp, "nosys }");
		}
	}
	if (j == 0)	/* empty table */
		fprintf(fp, "\t{ 0, 0, 0 }");
        fprintf(fp, "\n};\n\n");  /* end of execsw initialization */

        fprintf(fp, "int nexectype = %d;\n", j);
}




/* Create file listing path names of configured driver packages. */

prfiles()
{
        driver_t *drv;
        FILE *pp;

        pp = open1(pfile, "w", OUT);
        chmod(pfile, file_mode);

	/* process "kernel" module first */
	if (sfind("kernel"))
		wr_file(1, 0, "kernel", pp);

        /* process other modules */
	for (drv = driver_info; drv != NULL; drv = drv->next) {
		if (strcmp(drv->mdev.name, "kernel") == 0)
			continue;
		wr_file(drv->n_ctlr, drv->loadable, drv->mdev.name, pp);
	}

        fclose(pp);
}

/*
 * Write files in the directory 'name' to the file of objects for
 * the kernel we're building.
 */
wr_file(conf, loadable, dirname, ifp)
int conf;
short loadable;
char *dirname;
FILE *ifp;
{
        /* pick up stubs.c if driver's not configured */
        if (!conf) {
		(void) try_file(ifp, dirname, "stubs.c");
                return; /* done with this one */
        }

        if (loadable) {
		(void) try_file(ifp, dirname, "Modstub.o");
                return; /* done with this one */
        }

        /* check for Driver.o and add to file */
	if (!try_file(ifp, dirname, "Driver.o")) {
		/* Require a Driver.o if 'Y' in sdevice  */
		sprintf(errbuf, "Cannot find Driver.o for %s.", dirname);
		error(0);
        }

        /* add space.c to file */
	(void) try_file(ifp, dirname, "space.c");
}

int
try_file(ifp, dirname, fname)
FILE *ifp;
char *dirname;
char *fname;
{
	struct stat statb;

	sprintf(linebuf, "%s/%s", dirname, fname);
	sprintf(path, "%s/pack.d/%s", root, linebuf);
	if (stat(path, &statb) == 0) {
		fprintf(ifp, "%s\n", linebuf);
		return 1;
	}
	return 0;
}


/*
 *  Print the configuration info for file system types that
 *  are to be included in the system.
 */
print_vfssw(fp)
FILE *fp;
{
	register driver_t *drv;
        register int j, i;

/* First, search through the fstype table and print an
 * extern declaration for all the switch functions that
 * are supposed to be provided in the configured file types.
 */
        /*  
         *  start declaring the file system external init routines
         *  format is 
         *  extern int prefix+init();
         */

	fprintf(fp, "\n");

        /* loop for each configured type */
        for (drv = driver_info; drv != NULL; drv = drv->next) {
		if (!INSTRING(drv->mdev.mflags, FILESYS) || drv->n_ctlr == 0
			|| drv->loadable)
			continue;

		if (!drv_has_entry(drv, edef_init)) {
			sprintf(errbuf, FINITRT, drv->mdev.name);
			error(0);
		}
                fprintf(fp, "extern int %sinit();\n", drv->mdev.prefix);

		if (drv->vars & V_EXTMODNAME) {
			fprintf(fp, "extern char %sextmodname[];\n",
				drv->mdev.prefix);
		}
        }

/*
 * Next, set up and initialize the vfssw data structure table.
 */
        fprintf(fp, "\nstruct vfssw vfssw[] = {\n");
	/* first, initialize the NULL fs type  */
        fprintf(fp, "\t{ \"EMPTY\",\t(int (*)())0 }");
	j = 1;

        for (drv = driver_info; drv != NULL; drv = drv->next) {
		if (!INSTRING(drv->mdev.mflags, FILESYS) || drv->n_ctlr == 0
			|| drv->loadable)
			continue;

		if (drv->vars & V_EXTMODNAME) {
			fprintf(fp, ",\n\t{ %sextmodname, %sinit }",
				drv->mdev.prefix, drv->mdev.prefix);
		} else {
			fprintf(fp, ",\n\t{ \"%s\", %sinit }",
				drv->mdev.name, drv->mdev.prefix);
		}
		j++;
        }

	vfsswsz = j + vfs_reserve;

	for (i = j; i < vfsswsz; i++)
		fprintf(fp, ",\n\t{ \"\", (int (*)())0 }");

        fprintf(fp, "\n};\n\n");

        fprintf(fp, "int nfstype = %d;\n", j);
        fprintf(fp, "int vfsswsz = %d;\n", vfsswsz);
	fprintf(fp, "\n");
}

/* routines for handling dynamic loadable modules */

short	no_def_delay;

int
prloadables()
{
	FILE	*pp;
	char	moddir[512];
	driver_t *drv;

	if (tfind("DEF_UNLOAD_DELAY") == NULL)
		no_def_delay++;

	if (!mflag)
		pp = open1(lfile, "w", OUT);
	
	for (drv = driver_info; drv != NULL; drv = drv->next) {
		if (drv->n_ctlr == 0 || !drv->loadable)
			continue;

		if (mflag) {
			struct modlist *mod;

			for (mod = modlist; mod != NULL; mod = mod->next)
				if (strcmp(mod->name, drv->mdev.name) == 0)
					break;
			if (mod == NULL)
				continue;
		}
		sprintf(moddir, "%s/%s/%s", root, PKDIR, drv->mdev.name);
		if (chdir(moddir) != 0) {
			sprintf(errbuf, EXIST, moddir);
			fatal(0);
		}
		getcwd(moddir, 512);
		chdir(current);
		
		prmod(moddir, drv);
		prmodreg(moddir, drv);
		fdep_prsec(moddir, drv);
		if (!mflag)
			fprintf(pp, "%s\n", drv->mdev.name);
	}
	if (!mflag)
		fclose(pp);
}

prmod(dir, drv)
char *dir;
driver_t *drv;
{
	register FILE *fp;
	char file[512];
	char *pfx, *mflags;
	unsigned short type;
	char caps[PFXSZ];
	char delay_tune[TUNESZ];

	sprintf(file, "%s/%s", dir, mfile);
	fp = open1(file, "w", FULL);
	chmod(file, file_mode);

	type = 0;
	pfx = drv->mdev.prefix;
	mflags = drv->mdev.mflags;

	if (INSTRING(mflags, BLOCK))
		type |= MODBDEV;

	if (INSTRING(mflags, CHAR))
		type |= MODCDEV;

	if (INSTRING(mflags, STREAM))
		type |= MODSTR;

	if (INSTRING(mflags, FILESYS))
		type |= MODFS;

	if (INSTRING(mflags, HARDMOD))
		type |= MODHARD;

	if ((type & MODSTR) && !(type & MODCDEV))
		type |= MODMOD;

	fprintf(fp, "#include <config.h>\n");
	fprintf(fp, "#include <sys/types.h>\n");
	if (type & MODINTR) {
		fprintf(fp, "#include <sys/param.h>\n");
		fprintf(fp, "#include <sys/moddrv.h>\n");
	}

	if (type & MODMOD) {
		fprintf(fp, "#include <sys/stream.h>\n");
		fprintf(fp, "#include <sys/conf.h>\n");
	}
		
	if (type & MODFS) {
		fprintf(fp, "#include <sys/vfs.h>\n");
		fprintf(fp, "#include <sys/modfs.h>\n");
	}
		
	uppermap(drv->mdev.prefix, caps);
	sprintf(delay_tune, "%s_UNLOAD_DELAY", caps);
	if (tfind(delay_tune) == NULL)
		fprintf(fp, "#define %s_UNLOAD_DELAY\t%s\n", caps, no_def_delay ? "0" : "DEF_UNLOAD_DELAY");

	fprintf(fp, "\n");

	if (type & MODSTR) {
		if (!(drv->vars & V_INFO)) {
			sprintf(errbuf, STRTAB, drv->mdev.name);
			error(0);
		}
		fprintf(fp, "extern struct streamtab %sinfo;\n", pfx);
	}

	if (type & MODDRV) {
		fprintf(fp, "extern int nodev();\n");
		fprintf(fp, "extern int nulldev();\n");
	}

	if (type & MODINTR) {
		if (drv_has_entry(drv, edef_intr))
			fprintf(fp, "extern void %sintr();\n", pfx);
		else
			fprintf(fp, "extern void intnull();\n");
	}

	if ((type & MODDRV) && !(type & MODSTR)) {
		if (drv_has_entry(drv, edef_open))
			fprintf(fp, "extern int %sopen();\n", pfx);
		else if (type & MODBDEV) {
			sprintf(errbuf, OPRT, drv->mdev.name);
			error(0);
		}

		if (drv_has_entry(drv, edef_close))
			fprintf(fp, "extern int %sclose();\n", pfx);
		else if (type & MODBDEV) {
			sprintf(errbuf, CLRT, drv->mdev.name);
			error(0);
		}

		if (type & MODBDEV) {
			if (!drv_has_entry(drv, edef_strategy)) {
				sprintf(errbuf, STRAT, drv->mdev.name);
				error(0);
			}
			fprintf(fp, "extern int %sstrategy();\n", pfx);

			if (drv_has_entry(drv, edef_print))
				fprintf(fp, "extern int %sprint();\n", pfx);

			if (drv_has_entry(drv, edef_size))
				fprintf(fp, "extern int %ssize();\n", pfx);
		}
		if (drv_has_entry(drv, edef_read))
			fprintf(fp, "extern int %sread();\n", pfx);
		if (drv_has_entry(drv, edef_write))
			fprintf(fp, "extern int %swrite();\n", pfx);
		if (drv_has_entry(drv, edef_ioctl))
			fprintf(fp, "extern int %sioctl();\n", pfx);
		if (drv_has_entry(drv, edef_mmap))
			fprintf(fp, "extern int %smmap();\n", pfx);
		if (drv_has_entry(drv, edef_segmap))
			fprintf(fp, "extern int %ssegmap();\n", pfx);
		if (drv_has_entry(drv, edef_chpoll))
			fprintf(fp, "extern int %schpoll();\n", pfx);
	}

	if (type & MODFS) {
		fprintf(fp, "extern vfsops_t %s_vfsops;\n", pfx);
		fprintf(fp, "extern unsigned long %s_fsflags;\n", pfx);
	}

	if (!(type & (MODFS | MODHARD)))
		if (drv->vars & V_DEVFLAG)
			fprintf(fp, "extern int %sdevflag;\n", pfx);
		else
			fprintf(fp, "extern int nodevflag;\n");

	if (type & MODDRV) {
		fprintf(fp, "\nstruct mod_drv_data %s_mod_drvdata = {\n", pfx);

		if (type & MODBDEV) {
			fprintf(fp, "\t{%sopen,", pfx);
			fprintf(fp, " %sclose,", pfx);
			fprintf(fp, " %sstrategy,", pfx);
			fprintf(fp, " %sprint,", pfx);

			if (drv_has_entry(drv, edef_size))
				fprintf(fp, " %ssize,", pfx);
			else
				fprintf(fp, " nulldev,");
			fprintf(fp, "\n\t\t");

			fprintf(fp, " \"%s\",", drv->mdev.name);
			fprintf(fp, " NULL,");

			mdep_bdevsw(fp, drv);

			if (drv->vars & V_DEVFLAG)
				fprintf(fp, " &%sdevflag },\n", pfx);
			else
				fprintf(fp, " &nodevflag },\n");

			fprintf(fp, "\t%s_BMAJOR_0, %s_BMAJORS,\n", caps, caps);
		} else {
			fprintf(fp, "\t{nodev, nodev, nodev, nodev, nodev, \n\t\tNULL, NULL, ");
			mdep_bdevsw(fp, drv);
			fprintf(fp, "NULL },\n");
			fprintf(fp, "\t0, 0,\n");
		}

		if (type & MODCDEV && !(type & MODSTR)) {
			if (drv_has_entry(drv, edef_open))
				fprintf(fp, "\t{%sopen,", pfx);
			else
				fprintf(fp, "\t{nulldev,");

			if (drv_has_entry(drv, edef_close))
				fprintf(fp, "%sclose,", pfx);
			else
				fprintf(fp, "nulldev,");

			if (drv_has_entry(drv, edef_read))
				fprintf(fp, "%sread,", pfx);
			else
				fprintf(fp, "nodev,");

			if (drv_has_entry(drv, edef_write))
				fprintf(fp, "%swrite,", pfx);
			else
				fprintf(fp, "nodev,");

			if (drv_has_entry(drv, edef_ioctl))
				fprintf(fp, "%sioctl,\n", pfx);
			else
				fprintf(fp, "nodev,\n");

			if (drv_has_entry(drv, edef_mmap))
				fprintf(fp, "\t\t%smmap,", pfx);
			else
				fprintf(fp, "\t\tnodev,");

			if (drv_has_entry(drv, edef_segmap))
				fprintf(fp, "%ssegmap,", pfx);
			else
				fprintf(fp, "nodev,");

			if (drv_has_entry(drv, edef_chpoll))
				fprintf(fp, "%schpoll,", pfx);
			else
				fprintf(fp, "nodev,");

			if (INSTRING(mflags, TTYS))
				fprintf(fp, "%s_tty,\n", pfx);
			else
				fprintf(fp, "NULL,\n");

			fprintf(fp, "NULL, \t\t\"%s\",", drv->mdev.name);

			mdep_cdevsw(fp, drv);

			if (drv->vars & V_DEVFLAG)
				fprintf(fp, "&%sdevflag },\n", pfx);
			else
				fprintf(fp, "&nodevflag },\n");
		} else {
			if (type & MODSTR)
				fprintf(fp, "\t{nulldev, nulldev,");
			else
				fprintf(fp, "\t{nodev, nodev,");
			fprintf(fp, " nodev, nodev, nodev, nodev,\n\t\tnodev, nodev,");
			if (type & MODSTR) 
				fprintf(fp, " NULL, &%sinfo, \"%s\", ", 
					pfx, drv->mdev.name);
			else
				fprintf(fp, " NULL, NULL, NULL, ");

			mdep_cdevsw(fp, drv);

			if (type & MODSTR) 
				if (drv->vars & V_DEVFLAG)
					fprintf(fp, "&%sdevflag },\n", pfx);
				else
					fprintf(fp, "&nodevflag },\n");
			else
				fprintf(fp, "NULL },\n");
			
		}
		if (type & MODCDEV)
			fprintf(fp, "\t%s_CMAJOR_0, %s_CMAJORS };\n", caps, caps);
		else
			fprintf(fp, "\t0, 0 };\n");
	}

	if (type & MODINTR) {
		fprintf(fp, "\nstatic struct intr_info %s_intrinfo[] = {\n", pfx);
		mdep_printr(fp, drv, caps);
		fprintf(fp, "};\n");
		fprintf(fp, "\nstruct mod_drvintr %s_attach_info = {\n", pfx);
		fprintf(fp, "\t%s_intrinfo,\n", pfx);
		if (drv_has_entry(drv, edef_intr))
			fprintf(fp, "\t%sintr\n};\n", pfx);
		else
			fprintf(fp, "\tintnull\n};\n");
	}

	if (type & MODMOD) {
		fprintf(fp, "\nstruct fmodsw %s_mod_strdata = {\n", pfx);
		fprintf(fp, "\t\"%s\",\n\t&%sinfo,\n", drv->mdev.name, pfx);
		if (drv->vars & V_DEVFLAG)
			fprintf(fp, "\t&%sdevflag\n};\n", pfx);
		else
			fprintf(fp, "\t&nodevflag\n};\n");
	}

	if (type & MODFS) {
		fprintf(fp, "\nstruct mod_fs_data %s_mod_fsdata = {\n", pfx);
		fprintf(fp, "\t\"%s\",\n\t&%s_vfsops,\n\t&%s_fsflags,\n};\n",
			drv->mdev.name, pfx, pfx, pfx);
	}

	fprintf(fp, "\ntime_t %s_conf_data = %s_UNLOAD_DELAY;\n", pfx, caps);

	fclose(fp);
}

prmodreg(dir, drv)
char *dir;
driver_t *drv;
{
	FILE *fp;
	char file[512];
	char *mflags;
	int i;
	int entry;

	sprintf(file, "%s/%s", dir, gfile);
	fp = open1(file, "w", FULL);
	chmod(file, file_mode);

	entry = 0;
	mflags = drv->mdev.mflags;
	if (INSTRING(mflags, BLOCK)) {
		for (i = drv->mdev.blk_start; i <= drv->mdev.blk_end; i++)
			fprintf(fp, "%d:%d:%s:%d\n", MOD_TY_BDEV,
				MOD_C_MREG, drv->mdev.name, i);
		entry++;
	}

	if (INSTRING(mflags, CHAR) && !INSTRING(mflags, STREAM)) {
		for (i = drv->mdev.chr_start; i <= drv->mdev.chr_end; i++)
			fprintf(fp, "%d:%d:%s:%d\n", MOD_TY_CDEV,
				MOD_C_MREG, drv->mdev.name, i);
		entry++;
	}

	if (INSTRING(mflags, STREAM) && INSTRING(mflags, CHAR)) {
		for (i = drv->mdev.chr_start; i <= drv->mdev.chr_end; i++)
			fprintf(fp, "%d:%d:%s:%d\n", MOD_TY_SDEV,
				MOD_C_MREG, drv->mdev.name, i);
		entry++;
	}

	if (INSTRING(mflags, STREAM) && !INSTRING(mflags, CHAR)) {
		fprintf(fp, "%d:%d:%s:%s\n", MOD_TY_STR, MOD_C_MREG, 
			drv->mdev.name, drv->mdev.name);
		entry++;
	}

	if (INSTRING(mflags, FILESYS)) {
		fprintf(fp, "%d:%d:%s:%s\n", MOD_TY_FS, MOD_C_MREG, 
			drv->mdev.name, drv->mdev.name);
		entry++;
	}

	if (!entry)
		fprintf(fp, "%d:%d:%s:%s\n", MOD_TY_MISC, MOD_C_MREG, 
			drv->mdev.name, drv->mdev.name);
	fclose(fp);
}

print_static(fp, ndrv)
FILE *fp;
int ndrv;
{
	driver_t *drv;
	char *static_modules;
	int j;
	char *buf;
	
	if ((static_modules = (char *)malloc(ndrv * NAMESZ)) == NULL) {
		sprintf(errbuf, "can't allocate memory for static_modules.");
		error(0);
		return(1);
	}
	
	for (drv = driver_info, buf = static_modules; drv != NULL; drv = drv->next) {
		/* not configured or loadable */
		if (drv->n_ctlr == 0 || drv->loadable)	
                        continue;
		strcpy(buf, drv->mdev.name);
		buf += NAMESZ;
	}

	qsort((void *)static_modules, ndrv, NAMESZ, strcmp);

	fprintf(fp, "\nchar *static_modules[] = {\n");
	for (j = 0, buf = static_modules; j < ndrv; j++, buf += NAMESZ)
		fprintf(fp, "\t\"%s\",\n", buf);
	fprintf(fp, "\tNULL };\n");
}
