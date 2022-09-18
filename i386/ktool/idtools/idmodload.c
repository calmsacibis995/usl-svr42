/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)idtools:i386/ktool/idtools/idmodload.c	1.5"
#ident	"$Header:"

#include "inst.h"
#include "defines.h"
#include "devconf.h"
#include <stdio.h>
#include <sys/mod.h>

#define USAGE	"Usage:\tidmodload [-r root] [-f modlist] [-#]\n"
#define MODLIST	"/etc/loadmods"
#define	SYNTAX	"%d:%d:%[^:]:%s"
#define ENTLEN	128

/* descriptions of module types used in error message */
char *type_name[] = {
	"None",
	"Character Device Driver",
	"Block Device Driver",
	"STREAMS Module",
	"File System Type",
	"STREAMS Device Driver",
	"Miscellaneous Module",
};

#define TYPENAME(d, t)	((d)->mdev.modtype[0] == '\0' ? \
				type_name[t] : (d)->mdev.modtype)

extern char *optarg;
extern char instroot[];
extern char pathinst[];
extern char current[];
driver_t *valid_mod();
int	debug = 0;

main(argc, argv)
int argc;
char *argv[];
{
	int c;
	driver_t *drv;
	char modname[NAMESZ];
	char buf[ENTLEN];
	char modlist[LINESZ], root[LINESZ];
	char typed[20];
	unsigned int type, cmd;
	int nfield;
	FILE *fp;
	int error, lerror;
	short rflag;
	char errmsg[128];

	rflag = 0;
	strcpy(modlist, MODLIST);

	while ((c = getopt(argc, argv, "f:r:?#")) != EOF)
		switch (c) {
		case 'f':
			strcpy(modlist, optarg);
			break;
		case 'r':
			rflag++;
			strcpy(root, optarg);
			break;
		case '#':
			debug++;
			break;
		default:
			fprintf(stderr, "%s", USAGE);
			exit(1);
		}

	getcwd(current, LINESZ);
	getpath(rflag, root, ROOT);
	strcpy(instroot, root);
	sprintf(pathinst, "%s/%s", root, CFDIR);

	ignore_entries = 1;

	getdevconf(NULL);

	if ((fp = fopen(modlist, "r")) == NULL) {
		fprintf(stderr, "%s: Cannot open %s for read.\n",
			argv[0], modlist);
		exit(1);
	}

	lerror = error = 0;

	while (fgets(buf, LINESZ, fp) != NULL) {
		if (buf[0] == '#')
			continue;
		nfield = sscanf(buf, SYNTAX, &type, &cmd, &modname, &typed);
		if (nfield != 4) {
			fprintf(stderr, "%s\n", buf);
			fprintf(stderr, "WARNING: number of fields is incorrect, ignored.\n");
			continue;
		}
		if (strlen(modname) >= NAMESZ) {
			fprintf(stderr, "%s: %s: module name too long.\n",
				argv[0], modname);
			error++;
			continue;
		}

		if ((drv = valid_mod(modname, argv[0])) == NULL) {
			if (debug)
				fprintf(stderr,
"%s: invalid module name, not loadable, or turned off in configuration file\n",
					modname);
			continue;
		}

		if (modload(modname) < 0) {
			if (lerror == 0)
				fprintf(stderr, "\nmodload:\n");
			sprintf(errmsg,
			"\tThe %s \"%s\" failed to load.\n\tThe error was",
				TYPENAME(drv, type), modname);
			perror(errmsg);
			lerror++;
		}
	}
	if (lerror)
		fprintf(stderr,
"WARNING: Any application(s) or commands(s) that uses one or more of these\n\
\tmodules may not work correctly.\n\
To fix: See the Troubleshooting section of the documentation.\n");

	fclose(fp);

	if (error || lerror)
		exit(1);

	exit(0);
}

driver_t *
valid_mod(dev, cmd)
char *dev;
char *cmd;
{
	register driver_t *drv;

	if ((drv = mfind(dev)) == NULL) {
		fprintf(stderr, "%s: %s: unknown module.\n", cmd, dev);
		return(NULL);
	}

	if (drv->n_ctlr == 0 || ! drv->loadable)
		return(NULL);

	return(drv);
}
