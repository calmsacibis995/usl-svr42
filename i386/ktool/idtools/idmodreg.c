/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)idtools:i386/ktool/idtools/idmodreg.c	1.7"
#ident	"$Header:"

#include "defines.h"
#include "inst.h"
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mod.h>
#include <unistd.h>
#include <errno.h>
#include <varargs.h>

#define USAGE	"Usage:\tidmodreg [-r root] [-f mreglist] [[-M module] ...] [-#]"
#define	SYNTAX	"%d:%d:%[^:]:%s"
#define ENTLEN 128

int mflag = 0;
int rflag = 0;
int fflag = 0;
int debug = 0;

char *gfile = "mod_reg";
char *rfile = "/etc/mod_register";

char current[LINESZ];
char root[LINESZ];
char errbuf[LINESZ];
char regfile[LINESZ];

int errcnt = 0;

struct modlist *modlist;
void runcmd();
void clean_up();

main(argc, argv)
int argc;
char **argv;
{
	int m;
	struct modlist *mod;
	FILE *fp;

	strcpy(regfile, rfile);

	while ((m = getopt(argc, argv, "?M:r:f:#")) != EOF)
		switch(m) {

		case 'M':
			mflag++;
			mod = (struct modlist *)malloc(sizeof(struct modlist));
			strcpy(mod->name, optarg);
			mod->next = modlist;
			modlist = mod;
			break;

		case 'r':
			rflag++;
			strcpy(root, optarg);
			break;

		case 'f':
			fflag++;
			strcpy(regfile, optarg);
			break;
			
		case '#':
			debug++;
			break;

		case '?':
			fprintf(stderr, USAGE);
			exit(1);
		}

	if (mflag)
		mod_reg();
	else {
		if (!fflag && access(regfile, F_OK) < 0)
			exit(0);
		if ((fp = fopen(regfile, "r")) == NULL) {
			sprintf(errbuf, FOPEN, regfile, "r");
			error();
		}
		proc_regfile(fp);
		fclose(fp);
	}
	if (errcnt)
		exit(1);
}

getpath(flag, buf, def)
int flag;
char *buf, *def;
{
	if (flag) {
		if (chdir(buf) != 0) {
			sprintf(errbuf, EXIST, buf);
			error();
		}
		getcwd(buf, LINESZ);
		chdir(current);
	} else
		strcpy(buf, def);
}

error()
{
	fprintf(stderr, "ERROR: %s\n", errbuf);
	exit(1);
}

proc_regfile(fp)
FILE *fp;
{
	int nfield;
	int major;
	unsigned int type;
	unsigned int cmd;
	char buf[ENTLEN], typed[20];
	struct mod_mreg mreg;
	char errmsg[80];

	while (fgets(buf, 40, fp) != NULL) {

		if (buf[0] == '#')
			continue;
		nfield = sscanf(buf, SYNTAX, &type, &cmd, mreg.md_modname, 
			typed);
		if (nfield != 4) {
			fprintf(stderr, "%s\n", buf);
			sprintf(errbuf, "number of fields is incorrect");
			errcnt++;
		}

		if (type == MOD_TY_CDEV || type == MOD_TY_BDEV ||
			type == MOD_TY_SDEV) {
			major = atoi(typed);
			mreg.md_typedata = (void *)major;
		} else
			mreg.md_typedata = (void *)typed;

		if (modadm(type, cmd, (void *)&mreg) < 0) {
			sprintf(errmsg, "modadm: cannot register module %s",
				mreg.md_modname);
			perror(errmsg);
			errcnt++;
		}
	}
}

mod_reg()
{
	char moddir[LINESZ], outfile[LINESZ];
	struct modlist *mod;
	FILE *fp;

	getcwd(current, sizeof(current));
	getpath(rflag, root, ROOT);

	if (debug)
		fprintf(stderr, "Root: %s\n", root);

	sprintf(outfile, "%s/%s/%s", root, CFDIR, gfile);

	if (access(regfile, F_OK) == 0)
		clean_up(regfile, outfile);

	for (mod = modlist; mod != NULL; mod = mod->next) {
		struct stat statb;

		sprintf(moddir, "%s/%s/%s", root, PKDIR, mod->name);
		if (chdir(moddir) != 0) {
			sprintf(errbuf, EXIST, moddir);
			error();
		}

		if (stat(gfile, &statb) < 0) {
			sprintf(errbuf, 
		"cannot find %s for %s, need to be preconfiged by idbuild",
				gfile, mod->name);
			error();
		}

		if ((fp = fopen(gfile, "r")) == NULL) {
			sprintf(errbuf, FOPEN, gfile, "r");
			error();
		}

		proc_regfile(fp);

		fclose(fp);
		runcmd("cat %s >> %s", gfile, outfile);
	}
	chdir(current);
	if (access(outfile, F_OK) == 0)
		runcmd("mv %s %s", outfile, regfile);
}

/*
 * This routine takes a variable number of arguments to pass them to 
 * the "system" library routine.
 */

void 
runcmd(va_alist)
va_dcl
{
	va_list args;
	char *fmt;
	char buf[LINESZ];

	va_start(args);
	fmt = va_arg(args, char *);
	va_end(args);
	vsprintf(buf, fmt, args);
	system(buf);
	return;
}

void
clean_up(oldf, newf)
char *oldf, *newf;
{
	FILE *ofp, *nfp;
	struct modlist *mod;
	char buf[ENTLEN], typed[20];
	char modname[NAMESZ];
	unsigned int type, cmd;
	int nfield;

	if ((ofp = fopen(oldf, "r")) == NULL) {
		sprintf(errbuf, "Cannot open file %s for reading.", oldf);
		error();
	}
	if ((nfp = fopen(newf, "w")) == NULL) {
		sprintf(errbuf, "Cannot open file %s for writing.", newf);
		error();
	}

	while (fgets(buf, 40, ofp) != NULL) {
		if (buf[0] == '#') {
			fputs(buf, nfp);
			continue;
		}
		nfield = sscanf(buf, SYNTAX, &type, &cmd, &modname, &typed);
		if (nfield != 4) {
			fprintf(stderr, "%s\n", buf);
			fprintf(stderr, "number of fields is incorrect, ignored.\n");
			continue;
		}

		for (mod = modlist; mod != NULL; mod = mod->next)
			if (strcmp(mod->name, modname) == 0)
				break;

		if (mod == NULL)
			fputs(buf, nfp);
	}

	fclose(ofp);
	fclose(nfp);
}
