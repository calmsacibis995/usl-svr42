/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)idtools:i386/ktool/idtools/idinstall.c	1.23"
#ident	"$Header:"

/*
 *       The idinstall command is called by a driver software package's
 *       (DSP) Install script and its function is to install, remove
 *       or update a DSP.  The command syntax is as follows:
 *
 *	       idinstall -[adugGM] -[ek] -[bmsoptnirhclAT] [-R dir] [-f file] [-P pkg_name] dev_name
 *			     |	            	 |	        |              			|
 *			  action           	DSP           rootdir    		internal device name
 *				        component(*)
 *
 *	       -a  Add the DSP components
 *	       -d  Remove the DSP components
 *	       -u  Update the DSP components
 *	       -M  Add/Update components as needed, based on modification times
 *	       -G  Get the DSP components in current format
 *		   (on stdout; -o and -b disallowed)
 *	       -g  Get the DSP components in original format 
 *		   (on stdout; -o and -b disallowed)
 *
 *	       -e  Disable free space checking (default on -g, -M, -d)
 *	       -k  Do not remove component from current directory on -a & -u
 *	       -f  Use the specified file for the reserved major number list
 *	       -P  Update the "contents" file with the package name specified.
 *
 *	       -m Master component
 *	       -s System component
 *	       -o Driver.o component
 *	       -p Space.c component
 *	       -t Stubs.c component
 *	       -b Modstub.o component
 *	       -n Node (special	file) component
 *	       -i Inittab component (Init)
 *	       -r Device Initialization	component (Rc)
 *	       -h Device Shutdown component (Sd)
 *             -c Mfsys component -- for compatibility; converted to Master
 *             -l Sfsys component -- for compatibility; converted to System
 *	       -A Sassign component
 *	       -T Mtune component
 *
 *             -R directory: use this directory instead of /etc/conf
 *
 *
 *	       (*) If no component is specified	the default is all.
 *
 * exit 0 - success
 *	1 - error
 */

#include "inst.h"
#include "defines.h"
#include "devconf.h"
#include "mdep.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#ifndef CROSS
#include <ftw.h>
int rm_entry();
#endif

/* components */
#define	MASTER		0x1
#define	SYSTEM  	0x2
#define NNODE		0x4
#define INITF		0x8
#define	RCFILE		0x10
#define	SHUTDOWN	0x20
#define	MFSYS		0x40
#define	SFSYS		0x80
#define	DRIVER		0x100
#define	SPACE		0x200
#define	STUBS		0x400
#define SASSIGN		0x800
#define MTUNE		0x1000
#define MODSTUB		0x2000
#define	ALL		0x3fff

/* copy type flags */
#define CP_BIN		0x01	/* binary, not text */
#define CP_MASTER	0x02	/* Master file, may need to be converted */
#define CP_SYSTEM	0x04	/* System file, may need to be converted */
#define CP_MFSYS	0x08	/* Mfsys file, convert to Master */
#define CP_SFSYS	0x10	/* Sfsys file, convert to System */

/* error messages */
#define USAGE		"Usage:  idinstall -[adugGM] [-ek] [-optbmsnirhAT] \
[-R rootdir] [-f maj_list] [-P pkg_name] module_name"
#define NOLINK		"Cannot link <%s> to <%s>: '%s'"
#define TOOLONG		"Line too long in %s file"

#define DIR_MODE	0755	/* File mode for directories */
#define FILE_MODE	0644	/* File mode for files */

#define	CONF		"/etc/conf"
static char *confdirs[] = {
	"mdevice.d",
	"sdevice.d",
	"node.d",
	"init.d",
	"rc.d",
	"sd.d",
	"sassign.d",
	"mtune.d",
	"mod.d"
};

#define TRUE	1
#define FALSE	0
#define SIZE 	512
char fbuf[SIZE];

char device[15];	/* device name */
char ppath[512];		/* relative device driver package directory */
char fullpath[SIZE];	/* complete path to device driver package directory */
char root[SIZE] = CONF;	/* root directory containing cf.d and packages */
char resfile[SIZE];	/* file for reserved major numbers list */
char errbuf[SIZE];
char linebuf[SIZE];
char pkginst[41];	/* name of the package which includes this module */
extern char instroot[];

int debug=0;		/* debug flag */
int madedir=0;		/* Flag to remove partial install on error */
int compat=0;		/* Flag for processing version 0 file. */

char actflag=0;	/* must have one of a, d, u, g, G, M */
char aflag=0;	/* -a flag specified, Add the component(s) */
char dflag=0;	/* -d flag specified, Delete the component(s) */
char uflag=0;	/* -u flag specified, Update the component(s) */
char Gflag=0;	/* -G flag specified, Get (to stdout) current component(s) */
char gflag=0;	/* -g flag specified, Get (to stdout) original component(s) */
char Mflag=0;	/* -M flag specified, Add/Update if out-of-date */

char eflag=0;	/* -e flag specified, disable free-space check */
char kflag=0;	/* -k flag specified, do not remove local file */
char Rflag=0;	/* -R flag specified, use argument as ID root directory */
char fflag=0;	/* -f flag specified, use argument as res_major file */
char Pflag=0;	/* -P flag specified, add all the new files to contents file */

int partflag=0;	/* component flag; none means all */

char old_master_dma[32];	/* When converting old format Master & System
				   files, save the dma field from the Master
				   file to add to the System file. */

struct devlist {
	short	start[2];
	short	end[2];
	struct devlist *link[2];
	struct mdev mdev;
	int	reserved;
	int	used;
} *devlist[2];
#define BDEVL	0
#define CDEVL	1

struct stat pstat;
extern void exit();
extern char *optarg;
extern int optind;
extern char *sys_errlist[];
extern int  noloadable;

struct devlist *enter_dev();
void idcat(), idrm();
void nmkdir();

char contentsf[L_tmpnam];	/* The tmp file used to update contents file. */
FILE *cfp;			/* File descriptor for the tmp file. */
int cfopen = 0;			/* flag to remove temp file when error occur. */

#define RECOVERY_CODE	0	/* The recovery code is broken in a large
				   number of ways, so for now, disable it. */


main(argc, argv)
int argc;
char *argv[];
{
	int	isthere, m;

	while ((m = getopt(argc, argv, "z?#abdugGMekmsnirhlcAToptR:f:P:")) != EOF)
		switch (m) {
		case 'a':
			aflag++; 
			actflag++;
			break;
		case 'd':
			dflag++; 
			actflag++;
			break;
		case 'u':
			uflag++; 
			actflag++;
			break;
		case 'g':
			gflag++; 
			actflag++;
			break;
		case 'G':
			Gflag++; 
			actflag++;
			break;
		case 'M':
			Mflag++; 
			actflag++;
			break;

		case 'e':
			eflag++; 
			break;

		case 'k':
			kflag++; 
			break;

		case 'R':
			Rflag++;
			strcpy(root, optarg);
			break;
			
		case 'm':
			partflag |= MASTER;
			break;
		case 's':
			partflag |= SYSTEM;
			break;
		case 'o':
			partflag |= DRIVER;
			break;
		case 'p':
			partflag |= SPACE;
			break;
		case 't':
			partflag |= STUBS;
			break;
		case 'b':
			partflag |= MODSTUB;
			break;
		case 'n':
			partflag |= NNODE;
			break;
		case 'i':
			partflag |= INITF;
			break;
		case 'r':
			partflag |= RCFILE;
			break;
		case 'h':
			partflag |= SHUTDOWN;
			break;
		case 'l':
			partflag |= SFSYS;
			break;
		case 'c':
			partflag |= MFSYS;
			break;
		case 'A':
			partflag |= SASSIGN;
			break;
		case 'T':
			partflag |= MTUNE;
			break;
		case 'f':
			fflag++;
			strcpy(resfile, optarg);
			break;
		case 'P':
			Pflag++;
			strcpy(pkginst, optarg);
			tmpnam(contentsf);
			if ((cfp = fopen(contentsf, "w")) == NULL) {
				fprintf(stderr,
			"Cannot open file %s for writing.\n", contentsf);
				exit(1);
			}
			cfopen++;
			break;
		case '#':
			debug++;
			break;
		case '?':
			fprintf(stderr, "%s\n", USAGE);
			exit(1);
		}

	if (actflag != 1) {
		fprintf(stderr,
		"Must have exactly one of -a, -d, -u, -g, -G, -M options.\n"); 
		error(USAGE, 0);
	}
	if (argc - optind != 1) {
		fprintf(stderr, "Must specify exactly one device name.\n"); 
		error(USAGE, 0);
	}
	strcpy(device, argv[optind]);
	if (device[0] == '\0') {
		fprintf(stderr, "Device name must be nonblank.\n"); 
		error(USAGE, 0);
	}

	if (!fflag)
		sprintf(resfile, "%s/%s/res_major", root, CFDIR);

	/* Tell getinst() where files reside */
	strcpy(instroot, root);
	/* (Don't need to set pathinst[] since we only use MDEV_D.) */

	if (!(aflag || uflag))
		eflag++;
	if (!(aflag || uflag))
		kflag++;

	if (!eflag) {
		if (access("/etc/conf/bin/idspace", 0) == 0 &&
		    system("/etc/conf/bin/idspace") != 0)
			error("Insufficient disk space to reconfigure.", 0);
	}

	if ((partflag & (MFSYS|SFSYS)) && !(aflag || uflag)) {
		fprintf(stderr,
		 "-l/-c no longer supported except with -a or -u; use -m/-s\n");
		error(USAGE, 0);
	}

	if (partflag == 0)
		partflag = ALL;

	strcpy(ppath, "/pack.d/");	/* dir. for package (rel. to root) */
	strcat(ppath, device);
	strcpy(fullpath, root);
	strcat(fullpath, ppath);

	if(debug){
		printf ("ppath = %s, fullpath = %s\n", ppath, fullpath);
		printf ("parts= %x, act= %x, dev= %s\n", partflag, actflag, device);
	}

	if (stat(fullpath, &pstat) < 0) {
		if (errno != ENOENT)
			error ("Cannot stat device driver directory.");
		isthere = FALSE;
	} else
		isthere = TRUE;

	if (aflag || Mflag) {
		if (partflag & (DRIVER|SPACE|STUBS|MODSTUB)) {
			if (!isthere) {
				strcpy(fbuf, root);
				strcat(fbuf, "/pack.d");
				if (stat(fbuf, &pstat) < 0)
					nmkdir(fbuf);
				nmkdir(fullpath);
				madedir++;
#if RECOVERY_CODE
				if (aflag)
					mksave();
#endif
			} else if (aflag)
				error("Device package already exists.", 0);
		}
	}
	if ((uflag || gflag || Gflag) && !isthere)
		error("Cannot open driver package directory.", 0);
#if RECOVERY_CODE
	if (uflag || Mflag)
		mksave();
#endif

	if (aflag || Mflag || uflag) {
		if (aflag && partflag == ALL) {
			if (stat("Driver.o",&pstat) < 0)
				error("Local directory does not contain a Driver object (Driver.o) file.", 0);
			isthere = (stat("Master", &pstat) == 0 ? 1 : 0);
			isthere += (stat("System", &pstat) == 0 ? 2 : 0);
			isthere += (stat("Mfsys", &pstat) == 0 ? 4 : 0);
			isthere += (stat("Sfsys", &pstat) == 0 ? 8 : 0);
			switch (isthere) {
			case 3:		/* Master+System */
			case 12:	/* Mfsys+Sfsys */
				break;
			case 15:	/* Master+System+Mfsys+Sfsys */
				error("Cannot have both Mfsys/Sfsys and Master/System.", 0);
			default:
				error("Local directory must contain both Master and System files (or Mfsys and Sfsys).", 0);
			}
		}
		if (partflag & DRIVER) {
			if (ccopy("Driver.o", ppath, "Driver.o", CP_BIN, 0))
				idunlink("Driver.o");
		}
		if (partflag & SPACE) {
			if (ccopy("Space.c", ppath, "space.c", 0, 0))
				idunlink("Space.c");
		}
		if (partflag & STUBS) {
			if (ccopy("Stubs.c", ppath, "stubs.c", 0, 0))
				idunlink("Stubs.c");
		}
		if (partflag & MODSTUB) {
			if (ccopy("Modstub.o", ppath, "Modstub.o", CP_BIN, 0))
				idunlink("Modstub.o");
		}
		if (partflag & MASTER) {
			if (ccopy("Master", "/mdevice.d", device, CP_MASTER,
								  MDEV_VER))
				idunlink("Master");
		}
		if (partflag & SYSTEM) {
			if (ccopy("System", "/sdevice.d", device, CP_SYSTEM,
								  SDEV_VER))
				idunlink("System");
		}
		if (partflag & NNODE) {
			if (ccopy("Node", "/node.d", device, 0, NODE_VER))
				idunlink("Node");
		}
		if (partflag & INITF) {
			if (ccopy("Init", "/init.d", device, 0, 0))
				idunlink("Init");
		}
		if (partflag & RCFILE) {
			if (ccopy("Rc", "/rc.d", device, 0, 0))
				idunlink("Rc");
		}
		if (partflag & SHUTDOWN) {
			if (ccopy("Sd", "/sd.d", device, 0, 0))
				idunlink("Sd");
		}
		if (partflag & MFSYS) {
			if (ccopy("Mfsys", "/mdevice.d", device, CP_MFSYS, 0))
				idunlink("Mfsys");
		}
		if (partflag & SFSYS) {
			if (ccopy("Sfsys", "/sdevice.d", device, CP_SFSYS, 0))
				idunlink("Sfsys");
		}
		if (partflag & SASSIGN) {
			if (ccopy("Sassign", "/sassign.d", device, 0,
								   SASSIGN_VER))
				idunlink("Sassign");
		}
		if (partflag & MTUNE) {
			if (ccopy("Mtune", "/mtune.d", device, 0, MTUNE_VER))
				idunlink("Mtune");
		}
	}
	if (dflag) {
		if (partflag == ALL){
#if RECOVERY_CODE
			mksave();
#endif
			rmpack(1,1);	/* Save and be quiet on err */
		} else {
			if (partflag & DRIVER)
				idrm("pack.d", "Driver.o", 0);
			if (partflag & SPACE)
				idrm("pack.d", "space.c", 0);
			if (partflag & STUBS)
				idrm("pack.d", "stubs.c", 0);
			if (partflag & MODSTUB)
				idrm("pack.d", "Modstub.o", 0);
			if (partflag & MASTER)
				idrm("mdevice.d", NULL, 0);
			if (partflag & SYSTEM)
				idrm("sdevice.d", NULL, 0);
			if (partflag & NNODE)
				idrm("node.d", NULL, 0);
			if (partflag & INITF)
				idrm("init.d", NULL, 0);
			if (partflag & RCFILE)
				idrm("rc.d", NULL, 0);
			if (partflag & SHUTDOWN)
				idrm("sd.d", NULL, 0);
			if (partflag & SASSIGN)
				idrm("sassign.d", NULL, 0);
			if (partflag & MTUNE)
				idrm("mtune.d", NULL, 0);
		}
	}
	if (Gflag || gflag) {
		if (partflag == ALL) {
			fprintf(stderr,"Must have one of -p, -t, -m, s, -n, -i, -r, -h, -A, -T\noptions when using -g or -G.\n"); 
			error(USAGE, 0);
		}
		if (partflag & (DRIVER | MODSTUB)) {
			fprintf(stderr, "-o and -b options not allowed with -g and -G.\n"); 
			error(USAGE, 0);
		}
		if (partflag & SPACE)
			idcat("pack.d", "space.c");
		if (partflag & STUBS)
			idcat("pack.d", "stubs.c");
		if (partflag & MASTER)
			idcat("mdevice.d", NULL);
		if (partflag & SYSTEM)
			idcat("sdevice.d", NULL);
		if (partflag & NNODE)
			idcat("node.d", NULL);
		if (partflag & INITF)
			idcat("init.d", NULL);
		if (partflag & RCFILE)
			idcat("rc.d", NULL);
		if (partflag & SHUTDOWN)
			idcat("sd.d", NULL);
		if (partflag & SASSIGN)
			idcat("sassign.d", NULL);
		if (partflag & MTUNE)
			idcat("mtune.d", NULL);
	}
	if (Pflag) {
		char cmdline[LINESZ];

		fclose(cfp);
		if (debug)
			fprintf(stderr, "temp file is %s\n", contentsf);
		if (aflag || uflag || Mflag) {
			sprintf(cmdline, "installf %s - < %s",
				pkginst, contentsf);
			system(cmdline);
		}
		if (dflag) {
			sprintf(cmdline, "removef %s - < %s",
				pkginst, contentsf);
			system(cmdline);
		}
		unlink(contentsf);
	}
	exit(0);
}
	

rmpack(savflg, quiet)
int savflg;
int quiet;
{
	int i;
	char rmdir[512];

	if (debug)
		printf("Removing device driver directory and its contents.\n");
#ifndef CROSS
	if (Pflag)
		while (ftw(fullpath, rm_entry, 20) > 0)
			;
#endif
	sprintf(rmdir, "rm -rf %s", fullpath);
	if (system(rmdir) != 0 && !quiet) {
		fprintf(stderr,
			"idinstall: Cannot remove driver package directory\n");
		exit(1);
	}

	for (i=0; i<(sizeof(confdirs)/sizeof(char *)); i++) {
		strcpy(rmdir, confdirs[i]);
		idrm(rmdir, NULL, savflg);
	}
}
	
#ifndef CROSS
int
rm_entry(name, stat, flag)
char *name;
struct stat *stat;
int flag;
{
	if (debug)
		fprintf(stderr, "removing contents entry for %s\n", name);
	fprintf(cfp, "%s\n", name);
	return(0);
}
#endif
	
void
idrm(dirname,filname,savflg) 
char *dirname;
char *filname;
int savflg;
{
	char delfile[128];
	int save_checked;
	struct stat sfstat;

	save_checked = 0;
	sprintf(delfile, "%s/%s/%s", root, dirname, device);

id_remove:

	if (filname != NULL){
		strcat(delfile, "/");
		strcat(delfile, filname);
	}

	if (stat(delfile, &sfstat) != 0)
		return;

	if (debug)
		printf ("removing %s\n", delfile);
#if RECOVERY_CODE
	char savfile[40];

	if (savflg && !stat(delfile, &pstat)){
		strcpy(savfile, "/etc/.last_dev_del/");
		strcat(savfile, dirname);
		strcat(savfile, "/");
		strcat(savfile, device);
		if (link(delfile, savfile) < 0) {
			sprintf(errbuf, NOLINK, delfile, savfile, sys_errlist[errno]);
			error(errbuf, 0);
		}
	}
#endif /* RECOVERY_CODE */
	if (unlink(delfile) < 0) {
		fprintf(stderr, "WARNING: cannot remove file %s\n", delfile);
		return;
	}

	if (Pflag) {
		if (debug)
			fprintf(stderr, "removing contents file entry %s\n",
				delfile);
		fprintf(cfp, "%s\n", delfile);
	}
	if (save_checked)
		return;

	save_checked++;
	sprintf(delfile, "%s/.%s/%s", root, dirname, device);
	goto id_remove;
}

void
idcat(dirname, filname)
char *dirname;
char *filname;
{
	char cmdline[512];
	char catfile[512];
	char sfile[512];
	struct stat sfstat;

	sprintf(catfile, "%s/%s/%s", root, dirname, device);
	if (gflag) {
		sprintf(sfile, "%s/.%s/%s", root, dirname, device);
		if (stat(sfile, &sfstat) == 0) {
			if (debug)
				fprintf(stderr, "Use saved file: %s\n", sfile);
			strcpy(catfile, sfile);
		}
	}
		
	if (filname != NULL){
		strcat(catfile, "/");
		strcat(catfile, filname);
	}
	sprintf(cmdline, "cat -s %s", catfile);
	if (debug)
		fprintf(stderr, "%s\n", cmdline);
	if (system(cmdline) != 0)
		error("Cannot find driver component.\n", 0);
}


/* print error message */

error(msg, show_line)
char *msg;
int show_line;
{
	fprintf(stderr, "idinstall: %s\n", msg);
	if (show_line)
		fprintf(stderr, "LINE: %s", linebuf);
	if (madedir) {
		rmpack(0, 1);	/* don't save, and be quiet */
#if RECOVERY_CODE
		unlink ("/etc/.last_dev_add");
#endif
	}
	if (cfopen) {
		fclose(cfp);
		unlink(contentsf);
	}
	exit(1);
}

insterror(errcode, ftype, dev)
int errcode;
int ftype;
char *dev;
{
	insterrmsg(errcode, ftype, dev);
	error(errbuf, errcode != IERR_OPEN);
}


idcp(src, destd, destf, flags, cur_ver)
char *src, *destd, *destf;
int flags;
int cur_ver;
{
	char tpath[512], spath[512];
	FILE *from, *to;
	int version = 0;
	int ver_written = 0;
	int errstat = 0;
	int n;

	strcpy(tpath, root);
	strcat(tpath,destd);
	strcat(tpath, "/");
	strcat(tpath,destf);
#if RECOVERY_CODE
	if (uflag || Mflag) {
		strcpy(spath,"/etc/.last_dev_del");
		strcat(spath,destd);
		mkdir(spath, DIR_MODE);
		strcat(spath, "/");
		strcat(spath,destf);
		if (stat(tpath, &pstat) == 0) {
			if (link(tpath,spath) < 0) {
				sprintf(errbuf, NOLINK, tpath, spath,
					sys_errlist[errno]);
				error(errbuf, 0);
			}
			unlink(tpath);
		}
	}
#endif /* RECOVERY_CODE */
	if(debug)
		printf("copying %s\n", tpath);

	if ((from = fopen(src, "r")) == NULL)
		error("Cannot copy files - read open failed.", 0);
	close(creat(tpath, FILE_MODE));
	if ((to = fopen(tpath, "w")) == NULL)
		error("Cannot copy files - write open failed.", 0);

	if (flags & CP_BIN) {
		while ((n = fread(linebuf, 1, sizeof linebuf, from)) > 0)
			fwrite(linebuf, 1, n, to);
	} else {
		while (fgets(linebuf, sizeof linebuf, from) != NULL) {
			if (linebuf[strlen(linebuf) - 1] != '\n') {
				sprintf(errbuf, TOOLONG, src);
				errstat = 2;
				break;
			}
			if (INSTRING("*#\n", linebuf[0]))
				;
			else if (strncmp(linebuf, "$entry",  6) == 0)
				;
			     else if (strncmp(linebuf, "$version", 8) == 0 &&
				   (linebuf[8] == ' ' || linebuf[8] == '\t')) {
				      version = atoi(linebuf + 9);
				      if (version > cur_ver) {
				      	      sprintf(errbuf, TOONEW, src);
					      errstat = 2;
					      break;
				      }
				      if (version != cur_ver)
					      continue;
			          } else {
				      if (flags & CP_MFSYS) {
					      convert_mfsys(linebuf);
					      cur_ver = MDEV_VER;
					      flags |= CP_MASTER;
				      } else if (flags & CP_SFSYS) {
					        convert_sfsys(linebuf);
						cur_ver = SDEV_VER;
						flags |= CP_SYSTEM;
					     }
				      if (version != cur_ver) {
						save_original(src, destd, destf);
					 	if (!ver_written) {
						    fprintf(to, "$version %d\n",
							cur_ver);
						    ver_written = 1;
						}
						if (flags & CP_SYSTEM)
						    convert_system(linebuf, to,
								version);
						else if (flags & CP_MASTER)
							convert_master(linebuf,
								to, version);
				      }
				      if (flags & CP_MASTER) {
					  if (process_master(linebuf, to))
						break;
					  continue;
				      }
				}
			fputs(linebuf, to);
		}
	}

	if (ferror(from)) {
		sprintf(errbuf, "Cannot copy files - read error on %s.", src);
		errstat = 1;
	}

	fclose(to);
	fclose(from);

	if (errstat) {
		sprintf(errbuf + strlen(errbuf), "\n*** %s removed.", tpath);
		error(errbuf, errstat - 1);
	}

	if (Pflag) {
		fprintf(cfp, "%s%s/%s v 0%o root sys\n",
			root, destd, destf, FILE_MODE);
		if (debug)
			fprintf(stderr, "%s%s/%s v 0%o root sys\n", 
				root, destd, destf, FILE_MODE);
	}
}

convert_mfsys(line)
	char	line[];
{
	char	name[32], pfx[32];

	if (sscanf(line, "%32s %32s", name, pfx) != 2) {
		sprintf(errbuf, EMSG_NFLDS, "Mfsys");
		error(errbuf, 1);
	}
	sprintf(line, "%s\t-\tF\t%s\t0\t0\t0\t0\t-1\n", name, pfx);
	if (debug)
		printf("Mfsys file converted to version 0 Master file:\n%s",
			linebuf);
}

convert_sfsys(line)
	char	line[];
{
	line[strlen(line) - 1] = '\0';
	strcat(line, "\t0\t0\t0\t0\t0\t0\t0\t0\n");
	if (debug)
		printf("Sfsys file converted to version 0 System file:\n%s",
			linebuf);
}

convert_master(line, to_file, version)
	char	line[];
	FILE	*to_file;
	int	version;
{
	char	name[32], func[32], flg[32], pfx[32], bmaj[32], cmaj[32],
		umin[32], umax[32], dma[32];
	char	nflg[32], *op, *np;
	int	nflds;
	int	is_blk, is_chr;

	/* Version 0 Master file - convert to new format */
	nflds = sscanf(line, "%32s %32s %32s %32s %32s %32s %32s %32s %32s",
		       name, func, flg, pfx, bmaj, cmaj, umin, umax, dma);
	if (nflds != 9) {
		sprintf(errbuf, EMSG_NFLDS, "Master");
		error(errbuf, 1);
	}

	/*
	 * Set special compatibility flag to cause idconfig to relax
	 * strict error checking which wasn't done in the past.
	 */
	np = nflg;
	*np++ = COMPAT;
	compat=1;

	/* Strip off obsolete flags */
	op = flg;
	while ((*np = *op++) != '\0') {
		if (!INSTRING(OLD_MFLAGS, *np) && *np != '-')
			np++;
	}

	/*
	 * Conversion of execsw modules has cross-dependencies between the
	 * Master and System files, so must be done manually.
	 */
	if (INSTRING(nflg, EXECSW))
		error("Old execsw module Master/System must be converted by hand.", 1);

	is_blk = INSTRING(nflg, BLOCK);
	is_chr = INSTRING(nflg, CHAR);

	/*
	 * Convert function list characters to new-style entry point names.
	 * In many cases, the old code ignored some of the function list
	 * characters for certain module types, providing no error checking.
	 * To allow for incorrectly-specified modules which "got away with it",
	 * we have to skip any such cases, since our interpretation of $entry
	 * is much stricter.
	 */
	for (op = func; *op != '\0';) {
		switch (*op++) {
		case 'o':
			if (is_blk || is_chr)
				fprintf(to_file, "$entry open\n");
			break;
		case 'c':
			if (is_blk || is_chr)
				fprintf(to_file, "$entry close\n");
			break;
		case 'r':
			if (is_chr)
				fprintf(to_file, "$entry read\n");
			break;
		case 'w':
			if (is_chr)
				fprintf(to_file, "$entry write\n");
			break;
		case 'i':
			if (is_chr)
				fprintf(to_file, "$entry ioctl\n");
			break;
		case 'L':
			if (is_chr)
				fprintf(to_file, "$entry chpoll\n");
			break;
		case 'M':
			if (is_chr)
				fprintf(to_file, "$entry mmap\n");
			break;
		case 'S':
			if (is_chr)
				fprintf(to_file, "$entry segmap\n");
			break;
		case 'z':
			if (is_blk)
				fprintf(to_file, "$entry size\n");
			break;
		case 'I':
			fprintf(to_file, "$entry init\n");
			break;
		case 's':
			fprintf(to_file, "$entry start\n");
			break;
		case 'p':
			fprintf(to_file, "$entry poll\n");
			break;
		case 'h':
			fprintf(to_file, "$entry halt\n");
			break;
		case 'E':
			fprintf(to_file, "$entry kenter\n");
			break;
		case 'X':
			fprintf(to_file, "$entry kexit\n");
			break;
		case '-':
			/*
			 * The following entry points have historically been
			 * ignored, so we continue to do so.
			 */
		case 'f':
		case 'e':
		case 'x':
			break;
		default:
			error("Illegal function indicator in Master file", 1);
		}
	}

	/*
	 * Block device drivers must have strategy and print entry points,
	 * but they were not specified explicitly.
	 */
	if (INSTRING(nflg, BLOCK))
		fprintf(to_file, "$entry strategy print\n");

	/*
	 * Filesystem modules must have init routines.
	 */
	if (INSTRING(nflg, FILESYS) && !INSTRING(func, 'I'))
		fprintf(to_file, "$entry init\n");

	/*
	 * Dispatcher modules must have _init routines.
	 */
	if (INSTRING(nflg, DISP))
		fprintf(to_file, "$entry _init\n");

	sprintf(line, "%s\t%s\t%s\t0\t%s\t%s\n",
		name, pfx, nflg, bmaj, cmaj);
	strcpy(old_master_dma, dma);
	if (debug)
		printf("Version %d Master file converted to version %d:\n%s",
			version, MDEV_VER, linebuf);
}

/*ARGSUSED*/
convert_system(line, to_file, version)
	char	line[];
	FILE	*to_file;
	int	version;
{
	/* Version 0 System file - append dmachan from old Master file */
	if (old_master_dma[0] == '\0')
		error("Can't convert old System file w/o old Master file.", 1);
	sprintf(line + strlen(line) - 1, "\t%s\n", old_master_dma);
	if (debug)
		printf("Version %d System file converted to version %d:\n%s",
			version, SDEV_VER, linebuf);
}

int
process_master(line, to_file)
	char	line[];
	FILE	*to_file;
{
	struct mdev st_mast;
	int	is_blk, is_chr;
	int	stat;

	/*
	 * We haven't defined the legal entry-point types, and we don't
	 * care about them anyway, so just tell rdinst() to ignore
	 * $entry lines.
	 */
	ignore_entries = 1;

	/*
	 * Ignore all the configuration files entry for loadable modules.
	 */

	noloadable = 1;

	devlist[0] = devlist[1] = NULL;

	/* If this Master file entry is for a block or character device,
	 * and it is not marked 'required', we have to assign major number(s).
	 */
	if ((stat = rdinst(MDEV, line, (char *)&st_mast, 0)) != 1) {
		if (stat == I_MORE) {
			fputs(line, to_file);
			return 0;
		}
		insterror(stat, MDEV, st_mast.name);
	}

	if (INSTRING(st_mast.mflags, COMPAT) && !compat) {
		sprintf(errbuf, 
		"Illegal usage of 'C' flag for %s Master file.\n", 
		st_mast.name);
		error(errbuf, 1);
	}
	compat=0;

	is_blk = INSTRING(st_mast.mflags, BLOCK);
	is_chr = INSTRING(st_mast.mflags, CHAR);
	if (is_blk || is_chr) {
		get_current_devs();
		rd_res_major();
		assign_devs(&st_mast, is_blk, is_chr);
	}

	wrtmdev(&st_mast, to_file);
	return 1;
}

assign_devs(mdev, is_blk, is_chr)
	struct mdev *mdev;
	int	is_blk, is_chr;
{
	if (INSTRING(mdev->mflags, KEEPMAJ)) {
		if (is_blk)
			chk_res_major(BDEVL, mdev->name, mdev->blk_start, 
				mdev->blk_end);
		if (is_chr)
			chk_res_major(CDEVL, mdev->name, mdev->chr_start, 
				mdev->chr_end);
	} else {
		int	n_blk, n_chr;

		if ((mdev->blk_start | mdev->chr_start) != 0) {
			fprintf(stderr,
"Warning: %s Master file has non-zero major numbers, but no 'k' flag;\n",
				mdev->name);
			fprintf(stderr,
"\tignoring input numbers and assigning new major numbers.\n");
		}
		n_blk = mdev->blk_end - mdev->blk_start + 1,
		n_chr = mdev->chr_end - mdev->chr_start + 1,
		mdev->blk_start = mdev->blk_end = 0;
		mdev->chr_start = mdev->chr_end = 0;
		find_free_majors(is_blk, n_blk, is_chr, n_chr, mdev);
	}
}

get_current_devs()
{
	struct mdev md;
	int	is_blk, is_chr;
	int	stat;

	getinst(MDEV_D, RESET, NULL);

	while ((stat = getinst(MDEV_D, NEXT, (char *)&md)) == 1) {
		is_blk = INSTRING(md.mflags, BLOCK);
		is_chr = INSTRING(md.mflags, CHAR);
		if (is_blk || is_chr)
			(void) enter_dev(&md, is_blk, is_chr, 0);
	}

	if (stat != 0)
		insterror(stat, MDEV_D, md.name);
}

struct devlist *
enter_dev(mdev, is_blk, is_chr, reserved)
	struct mdev *mdev;
	int	is_blk, is_chr;
	int	reserved;
{
	struct devlist *dv;

	dv = (struct devlist *)malloc(sizeof(struct devlist));
	if (dv == NULL)
		error("Not enough memory to search mdevice entries", 0);

	dv->mdev = *mdev;
	dv->reserved = reserved;
	dv->used = 0;
	dv->link[0] = dv->link[1] = NULL;

	/* Link into block and char device lists, in order */
	if (is_blk)
		link_dev(dv, BDEVL, mdev->blk_start, mdev->blk_end);
	if (is_chr)
		link_dev(dv, CDEVL, mdev->chr_start, mdev->chr_end);

	return dv;
}

link_dev(dv, list, start, end)
	struct devlist *dv;
	int	list;
	int	start, end;
{
	struct devlist **dvp;

	dv->start[list] = start;
	dv->end[list] = end;
	dvp = &devlist[list];
	while (*dvp) {
		if ((*dvp)->start[list] < start) {
			dvp = &(*dvp)->link[list];
			continue;
		}
		if ((*dvp)->start[list] == start) {
			dv->used++;
		}
		break;
	}

	dv->link[list] = *dvp;
	*dvp = dv;
	if (debug)
		printf("Entered %d-%d onto %s device list, used=%d\n", 
			start, end, list == BDEVL ? "block" : "char", dv->used);
}

unlink_dev(dv, list)
	struct devlist *dv;
	int	list;
{
	struct devlist **dvp;

	dvp = &devlist[list];
	while (*dvp != dv)
		dvp = &(*dvp)->link[list];
	*dvp = dv->link[list];
}


find_free_majors(is_blk, n_blk, is_chr, n_chr, mdev)
	int	is_blk, n_blk, is_chr, n_chr;
	struct mdev *mdev;
{
	struct devlist **bdp, **cdp;
	int	blk, chr;
	int	same;

	same = (is_blk && is_chr && INSTRING(mdev->mflags, UNIQ));

	bdp = &devlist[BDEVL];
	cdp = &devlist[CDEVL];
	blk = chr = 0;

	do {
		if (is_blk)
			blk = dev_search(&bdp, n_blk, BDEVL, same? chr : 0);
		if (is_chr)
			chr = dev_search(&cdp, n_chr, CDEVL, same? blk : 0);
	} while (same && blk != chr);

	if (is_blk)
		mdev->blk_end = (mdev->blk_start = blk) + n_blk - 1;
	if (is_chr)
		mdev->chr_end = (mdev->chr_start = chr) + n_chr - 1;

	if (debug) {
		if (is_blk)
			printf("assigned free block majors %d-%d\n",
				mdev->blk_start, mdev->blk_end);
		if (is_chr)
			printf("assigned free char majors %d-%d\n",
				mdev->chr_start, mdev->chr_end);
	}
}

/* find first hole in devlist at or past `maj', big enough for `n_maj' slots */
int
dev_search(dvpp, n_maj, list, maj)
	struct devlist ***dvpp;
	int	n_maj, list, maj;
{
	while (**dvpp != NULL) {
		if (maj + n_maj <= (**dvpp)->start[list]){
			maj = ((**dvpp)->start[list] - n_maj);
			break;
		}
		if (maj <= (**dvpp)->end[list])
			maj = (**dvpp)->end[list] + 1;
		*dvpp = &(**dvpp)->link[list];
	}
	return maj;
}


#if RECOVERY_CODE
mksave()
{
	int i, fd, from, to, ct;
	struct stat dstat;
	char cfile[40];
	char pfile[80];

	unlink ("/etc/.last_dev_add");
	if (!stat("/etc/.last_dev_del",&pstat))
		system ("rm -rf /etc/.last_dev_del > /dev/null 2>&1");
	if (madedir) {
		if (debug)
			printf ("making /etc/.last_dev_add\n");
		if ((fd=creat("/etc/.last_dev_add", FILE_MODE))<0)
			error("Cannot create recovery files.", 0);
		write(fd, device, sizeof(device));
		close(fd);
	}
	else if (dflag || uflag || Mflag) {
		if (debug)
			printf ("making /etc/.last_dev_del\n");
		mkdir("/etc/.last_dev_del", DIR_MODE);
		if ((fd=creat("/etc/.last_dev_del/dev", FILE_MODE))<0)
			error("Cannot create recovery file - /etc/.last_dev_del/dev", 0);
		write(fd, device, sizeof(device));
		close(fd);

		if((from = open("/etc/conf/cf.d/mdevice", 0)) < 0)
			error("Cannot create recovery files - read mdevice.", 0);
		if((to = creat("/etc/.last_dev_del/mdevice", FILE_MODE)) < 0)
			error("Cannot create recovery files - create mdevice.", 0);
		while((ct = read(from, fbuf, SIZE)) != 0)
			if(ct < 0 || write(to, fbuf, ct) != ct)
				error("Cannot create recovery files - copy mdevice.", 0);
		close(to);
		close(from);

		/* create all the other directories that may be needed in case 
		 * of removal or update of a DSP.
		 */

		if (debug)
			printf ("making /etc/.last_dev_del/pack.d\n");
		mkdir("/etc/.last_dev_del/pack.d", DIR_MODE);

		strcpy(cfile, root);
		for (i = sizeof(confdirs) / sizeof(char *); i-- > 0;) {
			strcpy(pfile, cfile);
			strcat(pfile, "/");
			strcat(pfile, confdirs[i]);
			strcat(pfile, "/");
			strcat(pfile, device);
			if (!stat(pfile, &dstat)) {
				strcpy(pfile, "/etc/.last_dev_del/");
				strcat(pfile, confdirs[i]);
				if (debug)
					printf("making %s\n", pfile);
				mkdir(pfile, DIR_MODE);
			}
		}
	}
}
#endif /* RECOVERY_CODE */


idunlink(unl)
char *unl;
{
	if (!kflag)
		unlink(unl);
}


int
ccopy(src, destd, destf, flags, cur_ver)
char *src, *destd, *destf;
int flags;
int cur_ver;
{
	struct stat sstat, dstat;
	char dpath[512];

	if (stat(src, &sstat) != 0)
		return FALSE; /* component file not present */

	strcpy(dpath, root);
	strcat(dpath, destd);

	/* If necessary, make directory */
	if (stat(dpath, &dstat) != 0)
		nmkdir(dpath);

	strcat(dpath, "/");
	strcat(dpath, destf);

	if (Mflag) { /* check modification times */
		if (stat(dpath, &dstat) == 0) {
			if (dstat.st_mtime > sstat.st_mtime)
				return FALSE;
		}
	}

	idcp(src, destd, destf, flags, cur_ver);
	return TRUE;
}

rd_res_major()
{
	struct devlist *res_maj[2];
	FILE *fp;
	struct mdev mdev;
	int is_blk, is_chr, rtn, nfield;
	char type, range[RANGESZ], name[NAMESZ];

	if ((fp = fopen(resfile, "r")) == NULL) {
		sprintf(errbuf, "%s: can not open for mode %s", resfile, "r");
		error(errbuf, 0);
	}

	if (debug)
		fprintf(stderr, "open file %s for reserved majors\n", resfile);

	while (fgets(linebuf, 80, fp) != NULL) {
		if (linebuf[0] == '#')
			continue;
		nfield = sscanf(linebuf, "%c %s %s", &type, range, name);
		if (nfield != 3) {
			sprintf(errbuf, "number of fields is incorrect");
			error(errbuf, 1);
		}
		
		if (debug)
			fprintf(stderr, "Reserved: %c\t%s\t%s\n", type, range, name);

		strcpy(mdev.name, name);
		is_blk = is_chr = 0;

		switch (type) {
		case 'b':
			is_blk = 1;
			rtn = getmajors(range, &mdev.blk_start, &mdev.blk_end);
			break;
		case 'c':
			is_chr = 1;
			rtn = getmajors(range, &mdev.chr_start, &mdev.chr_end);
			break;
		default:
			sprintf(errbuf, "unknown entry");
			error(errbuf, 1);
			break;
		}

		if (rtn != 0) {
			sprintf(errbuf, "illegal major number entry");
			error(errbuf, 1);
		}

		enter_dev(&mdev, is_blk, is_chr, 1);
	}
}

chk_res_major(list, name, start, end)
{
	struct devlist **dvp;
	char modname[15];
	char errmsg[160];

	for (dvp = &devlist[list]; *dvp != NULL; dvp = &(*dvp)->link[list])
		if ((*dvp)->start[list] == start)
			if ((*dvp)->reserved && !(*dvp)->used)
				if ((*dvp)->end[list] == end) {
					if (debug)
						fprintf(stderr, "reserved major number (%d-%d) for %s is correct\n", start, end, name);

					return 0;
				} else {
					sprintf(errmsg,
"%s Master file has %s major number range (%d-%d)\n\twhich don't match with the reserved range (%d-%d)",
					name, list == BDEVL? "block" : "char",
					start, end, (*dvp)->start[list],
					(*dvp)->end[list]);
					error(errmsg, 0);
				}
			else {
				if ((*dvp)->reserved)
					strcpy(modname, (*dvp)->link[list]->mdev.name);
				else
					strcpy(modname, (*dvp)->mdev.name);

				sprintf(errmsg,
"\"%s\" is using the %s major number reserved by \"%s\" (%d-%d)",
					modname, list == BDEVL? "block" : "char",
					name, start, end);
				error(errmsg, 0);
			}
		
	sprintf(errmsg,
"%s major number reserved by \"%s\" (%d-%d) is not in res_major file",
		list == BDEVL? "block" : "char", name, start, end);
	error(errmsg, 0);
}

save_original(src, destd, destf)
char *src, *destd, *destf;
{
	char odestd[128], cmdline[512];
	struct stat dstat, fstat;

	if (debug)
		fprintf(stderr, "Save original version of %s\n", src);

	sprintf(odestd, "%s/.%s", root, &destd[1]);

	if (stat(odestd, &dstat) != 0)
		nmkdir(odestd);
	
	sprintf(cmdline, "cp %s %s/%s", src, odestd, destf);
	if (debug)
		fprintf(stderr, "%s\n", cmdline);
	if (system(cmdline) != 0)
		error("Cannot save the original file.\n", 0);
}

void
nmkdir(dpath)
char *dpath;
{
	if (debug)
		fprintf(stderr, "making %s directory.\n", dpath);
	if (mkdir(dpath, DIR_MODE) != 0) {
		sprintf(errbuf, "Can't make %s directory.\n", dpath);
		error(errbuf, 0);
	}
	if (Pflag) {
		fprintf(cfp, "%s d 0%o root sys\n",
			dpath, DIR_MODE);
		if (debug)
			fprintf(stderr, "%s d 0%o root sys\n",
				dpath, DIR_MODE);
	}
}
