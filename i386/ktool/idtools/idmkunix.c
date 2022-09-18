/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)idtools:i386/ktool/idtools/idmkunix.c	1.12"
#ident	"$Header:"

/*
 *  Config for Installable Drivers and Tunable Parameters 
 *
 *  ELF support changes:
 *      Use idcc -c instead of idcpp, idcomp, and idas directly.
 */

#include "inst.h"
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* directories */
#define IN	0
#define OUT	1
#define FULL	2

/* error messages */
#define USAGE	"Usage: idmkunix [-i in_dir] [-o out_dir] [-r root_dir] [-Ddefine] [-Udefine]\n\t[-I include_path] [-O out_file] [-Q] [-C ccs_root]\n"
#define	EXECF	"Cannot exec process '%s'. Errno = %d\n"
#define FORKF	"Cannot fork process '%s'. Errno = %d\n"
#define FOPEN	"Cannot open '%s' for mode '%s'\n"
#define EXIST	"Directory '%s' does not exist\n"
#define EFILE	"'Driver.o' does not exist in driver package '%s'\n"
#define SUFFIX	"'%s' does not contain the proper suffix for compilation\n"
#define BADCOMP "'%s' will not compile properly\n"
#define LINK	"Cannot link-edit %s\n"
#define MOVE	"space.o or stubs.o move failed\n"
#define FCNT	"Cannot set close-on-exec flag, fildes '%d'\n"

/* CCS components */
#define LD	"ld"
#define CC	"cc"
char *ld = "/bin/idld";
char *cc = "/bin/idcc";

static char *prefix;

char ld_path[256];
char cc_path[256];
char acomp_path[256];
char as_path[256];

char root[256];			/* rootdir passed in on command line */

char deflist[256] = "-DSYSV -D_KERNEL -DINKERNEL";
char inclist[1024];

#define SYSINC	"/usr/include"

/* input file names */
char	*pfile = "direct";	/* list of configured driver packages */
char	*dfile = "deflist";	/* list of cc -D options */
char	*lfile = "modlist";	/* list of loadable modules */

/* output file names */
char	*ifile = "ifile";	/* ifile for link editor */

/* flags */
int iflag = 0;			/* specified input directory */
int oflag = 0;			/* specified output directory */
int debug = 0;			/* debugging flag */
int qflag = 0;			/* quick linkedit specified */
int outflag = 0;		/* output file name specified */
int mflag = 0;

/* buffers */
char input[LINESZ];		/* input directory */
char output[LINESZ];		/* output directory */
char cfdir[256];		/* configuration directory */
char lddir[256];		/* linkedit directory */
char header[256];		/* include directory */
char outfile[256];		/* output file name */
char current[LINESZ];		/* current directory */
char errbuf[LINESZ];		/* hold error messages */

extern int errno;
extern char *optarg;

extern char *malloc();
extern char *getenv();
extern char *getcwd();
extern void exit();
void linkedit();
char *lastchar();
static char *getpref();

struct modlist *modlist = NULL;

main(argc, argv)
int argc;
char *argv[];
{
	int m;
	struct modlist *mod;

	prefix = getpref(argv[0]);

	while ((m = getopt(argc, argv, "?#i:o:r:C:D:I:M:O:QU:")) != EOF)
	{
		switch (m)
		{
		case '#':
			debug++;
			break;
		case 'i':
			strcpy(input, optarg);
			iflag++;
			break;
		case 'o':
			strcpy(output, optarg);
			oflag++;
			break;
		case 'r':
			strcpy(root, optarg);
			break;
		case 'D':
			strcat(deflist, " -D");
			strcat(deflist, optarg);
			break;
		case 'U':
			strcat(deflist, " -U");
			strcat(deflist, optarg);
			break;
		case 'I':
			strcat(inclist, " -I");
			strcat(inclist, optarg);
			break;
		case 'O':
			strcpy(outfile, optarg);
			outflag++;
			break;
		case 'Q':
			qflag++;
			break;
		case 'M':
			mflag++;
			mod = (struct modlist *)malloc(sizeof(struct modlist));
			strcpy(mod->name, optarg);
			mod->next = modlist;
			modlist = mod;
			break;
		case '?':
			fprintf(stderr, USAGE);
			exit(1);
		}
	}

	/* Construct path names for CCS components,
	 * and input and output directories.
	 */
	(void)getcwd(current, sizeof(current));

	if (strcmp(prefix, "") == 0) {
		/* native build */
		strcpy(ld_path, ld);
		strcpy(cc_path, cc);
		sprintf(as_path, "%s", "/bin");
		sprintf(acomp_path, "%s", "/lib");
	} else {
		/* cross environment */
		sprintf(ld_path, "%s%s", prefix, LD);
		sprintf(cc_path, "%s%s", prefix, CC);
	}

	sprintf(cfdir, "%s/%s", root, CFPATH);

	getpath(iflag, input, cfdir);
	getpath(oflag, output, cfdir);

	if (!outflag)
		sprintf(outfile, "%s%s", output, "/unix");

	sprintf(inclist + strlen(inclist),
		" -I%s -I%s%s ", output, root, SYSINC );

	sprintf(lddir, "%s%s%s", root, ROOT, "/pack.d");

	if (!mflag)
		readmod();
		
	if (debug) {
		fprintf(stderr, "Input: %s\nOutput: %s\n",
			input, output);
	}

	preconf();

	if (!mflag) {
		if (!qflag) {
			/* search driver package directories */
			search();
	
			/* compile files in output directory */
			compout();
		}
	
		/* link edit object modules */
		linkedit();
	}

	exit(0);
}


/* print error message and exit */

error()
{
	fprintf(stderr, "ERROR: %s\n", errbuf);

	(void)exit(1);
}



/* construct full path name */

getpath(flag, buf, def)
int flag;
char *buf, *def;
{
	switch (flag) {
	case 0:		/* use default value */
		strcpy(buf, def);
		break;
	case 1:		/* path name specified */
		if (chdir(buf) != 0) {
			sprintf(errbuf, EXIST, buf);
			error();
		}
		(void)getcwd(buf, 512);  /* sizeof input[], output[] */
		(void)chdir(current);
		break;
	}
}



/* open a file */

FILE *
open1(file, mode, dir)
char *file, *mode;
int dir;
{
	char path[LINESZ];
	FILE *fp;
	char *p;
	int ret;

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
		fprintf(stderr, "Open: mode=%s path=%s\n", mode, p);

	if ((fp = fopen(p, mode)) == NULL) {
		sprintf(errbuf, FOPEN, p, mode);
		error();
	}

	if ((ret = fcntl(fileno(fp), F_SETFD, 1)) == -1) {
		sprintf(errbuf, FCNT, fileno(fp));
		error();
	}
	return(fp);
}


/* Compile the .c files listed in the 'direct' file.
 *   output is ifile, which lists the resulting .o files
 *   plus any .o files from the 'direct' file.
 */
search()
{
	FILE *ifp, *pp;
	char buff[512];
	char direct[512];
	char *cptr;

	chdir(output);
	unlink(ifile);
	ifp = open1(ifile, "w", OUT);		/* ifile */
	chmod(ifile, 0644);

	chdir(input);
	pp = open1(pfile, "r", IN);

	chdir(lddir);

	while (fgets(buff, sizeof buff, pp) != NULL)
	{
		/* extract string (remove white space) */
		sscanf(buff, "%s", direct);

		cptr = lastchar(direct);
		if( *cptr == 'c' )
		{
			compile(direct);
			*cptr = 'o';
		}
		fprintf(ifp, "%s\n", direct);
	}
	fclose(pp);
	fclose(ifp);

	chdir(current);
}


/* Create a process and execute command.
 * Return 0 on success, -1 on failure.
 */

proc(cmd)
char *cmd;
{
	if (debug)
		fprintf(stderr, "proc: %s\n", cmd);

	if (system(cmd) != 0)
		return -1;
	return 0;
}


/*  
 *  ELF support put in, .o file is assumed
 */
compile(file)
char *file;
{
	char dfile_path[512];
	char cc_cmd[512];
	char mv_cmd[512];
	char ofile[512];
	char *p;
	char *mv="/bin/mv";
	char *mvtarget;
	int rc;

	if (dfile[0] == '/')
		strcpy(dfile_path, dfile);
	else
		sprintf(dfile_path, "%s/%s", output, dfile);

	/* Construct cc command */
	if (strcmp(prefix, "") == 0) {
		/* native build */
		sprintf(cc_cmd, "%s -Y0,%s -Ya,%s -c %s %s ",
			cc_path,
			acomp_path,
			as_path,
			deflist,
			inclist);
	} else {
		/* cross environment */
		sprintf(cc_cmd, "%s -c %s %s ",
			cc_path,
			deflist,
			inclist);
	}
	if (access(dfile_path, F_OK) == 0)
		sprintf(cc_cmd + strlen(cc_cmd), "`cat %s` ", dfile_path);
	sprintf(cc_cmd + strlen(cc_cmd), " %s", file);

	/* Derive output object file name */
	strcpy(ofile, file);
	p = lastchar(ofile);
	if (p == ofile)		/* null filename */
		return;
	if (p[-1] != '.' || (*p != 'c' && *p !='s')) {
		sprintf(errbuf, SUFFIX, file);
		error();
	}
	*p = 'o';

	/* get basename of file */
	mvtarget = p;
	while (mvtarget > ofile && mvtarget[-1] != '/')
		--mvtarget;

	if ((rc = proc(cc_cmd)) != 0) {
		if (debug)
			fprintf(stderr, "cc: rc=%d\n", rc);
	} else { 
		/* check if mv is appropriate */
		if (mvtarget != ofile) {
			sprintf(mv_cmd, "%s -f %s %s", mv, mvtarget, ofile);
			if (proc(mv_cmd) != 0)
				sprintf(errbuf, MOVE);
		} else {
			if (debug)
				fprintf(stderr, 
					"mv not needed for this compile\n");
		}
	}

	if (rc) {
		sprintf(errbuf, BADCOMP, file);
		error();
	}
}



/* compile files in output directory */

compout()
{
	if (chdir(output) != 0) {
		sprintf(errbuf, EXIST, output);
		error();
	}

	compile("conf.c");
}



/* link edit object modules and create 'unix' */

void
linkedit()
{
	char linkcmd[512];
	char ifile_path[512];

	if (ifile[0] == '/')
		strcpy(ifile_path, ifile);
	else
		sprintf(ifile_path, "%s/%s", output, ifile);

	sprintf(linkcmd, 
		"%s -dn -o %s -e _start -M%s/kernmap `cat %s` %s/conf.o",
		ld_path, outfile, input, ifile_path, output);

	if (chdir(lddir) != 0) {
		sprintf(errbuf, "can not chdir to %s\n", lddir);
		error();
	}
	if (proc(linkcmd) != 0){
		chdir(current);
		sprintf(errbuf, LINK, outfile);
		error();
	}
	chdir(current);
}


/* return a pointer to the last character in a string */

char *
lastchar(str)
char *str;
{
	char *p;

	for (p = str; *p != NULL; p++);
	return (p == str ? p : p - 1);
}
/* return the prefix of "idmkunix" */

static char *
getpref( cp )
char *cp;	/* how idmkunix was called */
{
	static char	tprefix[128];  /* enough room for prefix and \0 */
	int		cmdlen,
			preflen;
	char		*prefptr,
			*name;

	name= "idmkunix";
	if ((prefptr= strrchr(cp,'/')) == NULL)
		prefptr=cp;
	else
		prefptr++;
	cmdlen= strlen(prefptr);
	preflen= cmdlen - strlen(name);
	if ( (preflen < 0 )		/* if invoked with a name shorter
					   than name */
	    || (strcmp(prefptr + preflen, name) != 0)) {
		(void)fprintf(stderr, "command name must end in \"%s\"\n", name);
		exit(1);
		/*NOTREACHED*/
	} else {
		(void) strncpy(tprefix,prefptr,preflen);
		tprefix[preflen]='\0';
		return(tprefix);
	}
}

#define DIR_MODE 0755
#define MOD_MODE 0644

preconf()
{
	char direct[512], modifile[512], moddir[512];
	char linkcmd[512];
	struct modlist *mod;
	struct stat statb;
	FILE *ifp;

	if (mflag)
		sprintf(moddir, "%s/etc/conf/mod.d", root);
	else
		sprintf(moddir, "%s/etc/conf/modnew.d", root);

	if (stat(moddir, &statb) < 0)
		if (mkdir(moddir, DIR_MODE) != 0) {
			sprintf(errbuf, "Can't make directory %s", moddir);
			error();
		}
	
	for (mod = modlist; mod != NULL; mod = mod->next) {
		char outfile[LINESZ];

		sprintf(direct, "%s/%s", lddir, mod->name);
		chdir(direct);
		
		sprintf(modifile, "%s/mod_ifile", direct);
		if ((ifp = fopen(modifile, "w")) == NULL) {
			sprintf(errbuf, FOPEN, modifile, "w");
			error();
		}

		if (stat("Driver.o", &statb) == 0) {
			fprintf(ifp, "Driver.o\n");
		} else {
			sprintf(errbuf, "%s: required file missing in %s\n",
				"Driver.o", direct);
			error();
		}

		if (stat("space.c", &statb) == 0) {
			compile("space.c");
			fprintf(ifp, "space.o\n");
		}

		if (stat("mod_conf.c", &statb) == 0) {
			compile("mod_conf.c");
			fprintf(ifp, "mod_conf.o\n");
		} else {
			sprintf(errbuf, "%s: module is not loadable\n",
				direct);
			error();
		}

		if (stat("mod_sec.s", &statb) == 0) {
			compile("mod_sec.s");
			fprintf(ifp, "mod_sec.o\n");
		} else {
			chdir(current);
			sprintf(errbuf, "%s: module is not loadable\n",
				direct);
			error();
		}

		fclose(ifp);

		chdir(direct);
		sprintf(outfile, "%s/%s", moddir, mod->name);
		sprintf(linkcmd, "%s -r -dn -o %s `cat mod_ifile`",
			ld_path, outfile);
		if (proc(linkcmd) != 0) {
			chdir(current);
			sprintf(errbuf, "Can't link loadable module image for %s\n",
				mod->name);
			error();
		}
		chmod(outfile, MOD_MODE);
	}
	chdir(current);
}

readmod()
{
	FILE *fp;
	char buff[NAMESZ], name[NAMESZ];
	struct modlist *mod;

	fp = open1(lfile, "r", IN);
	while (fgets(buff, NAMESZ, fp) != NULL) {
		sscanf(buff, "%s", name);
		mod = (struct modlist *)malloc(sizeof(struct modlist));
		strcpy(mod->name, name);
		mod->next = modlist;
		modlist = mod;
	}
	fclose(fp);
}
