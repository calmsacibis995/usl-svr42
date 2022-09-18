/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/donext.c	1.2.2.2"
#ident "@(#)donext.c	1.5 'attmail mail(1) command'"
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include "s_string.h"
#include "smtp.h"
#include "smtp_decl.h"
#ifdef SVR4_1
#include <pfmt.h>
#include <errno.h>
#endif

int	batching = 0;
static int	batchc;
static char **	batchv;
extern int	debug;

/* init_batch -- Save the list of control files (for batching) */
void init_batch(argc, argv)
int argc;
char **argv;
{
	batching = 1;
	batchc = argc;
	batchv = argv;
}

/*
 * Read control file to get arguments for command.  Ignore the first line.
 * Read the second for the arguments for the command.
 */
static char **modified_getcmd(ctl, argc)
char *ctl;
int *argc;
{
	static string *args;
	static char *av[1024];
	char junk[1024];
	FILE *fp;
	int ac = 0;
	char *cp;

	*argc = 0;
	fp = fopen(ctl, "r");
	if (fp == NULL) {
#ifdef SVR4_1
		(void) pfmt(stderr, MM_ERROR, ":4:error opening %s\n", ctl);
#else
		(void) fprintf(stderr, "smtp: error opening %s\n", ctl);
#endif
		return NULL;
	}
	(void) fgets(junk, sizeof junk, fp);

	/*
	 *  make command line
	 */
	av[ac++] = "";
	args = s_reset(args);
	if (s_read_line(fp, args) == NULL) {
#ifdef SVR4_1
		(void) pfmt(stderr, MM_ERROR, ":5:error reading ctl file %s\n", ctl);
#else
		(void) fprintf(stderr, "smtp: error reading ctl file %s\n", ctl);
#endif
		fclose(fp);
		return NULL;
	}
	fclose(fp);
	for (cp = s_to_c(args); *cp && ac < 1023; ) {
		av[ac++] = cp++;
		while (*cp && !isspace(*cp))
			cp++;
		while (isspace(*cp))
			*cp++ = 0;
	}
	av[ac] = 0;
	*argc = ac;
	return av;
}

void rm_file()
{
	(void) unlink(fileoftype('E', *batchv));
	(void) unlink(*batchv);
	(void) unlink(fileoftype('D', *batchv));
}

void donext(unixformat, sender, recips, domain, fp, addr, host)
int *unixformat;
char **sender;
namelist **recips;
char **domain, **addr, **host;
FILE *fp;
{
	FILE *fp2;
	int argc;
	char **argv;

	/* Process the next control file */
	batchc--;
	batchv++;
	if (batchc == 0) {
		/* Signal that there are no more control files */
		*sender = (char *) 0;
		return;
	}

	if (debug)
		fprintf(stderr, "donext -> %s\n", *batchv);

	/*  Read the next control file */
	argv = modified_getcmd(*batchv, &argc);
	if (argv == NULL) {
#ifdef SVR4_1
		(void) pfmt(stderr, MM_ERROR, ":6:bad control file %s\n", *batchv);
#else
		(void) fprintf(stderr, "smtp: bad control file %s\n", *batchv);
#endif
		exit(1);
	}

	/*  Open D. file on stdin */
	fp = freopen(fileoftype('D', *batchv), "r", fp);
	if (fp == NULL) {
#ifdef SVR4_1
		pfmt(stderr, MM_ERROR, ":48:error reading data file: %s\n", strerror(errno));
#else
		perror("smtp: error reading data file");
#endif
		exit(1);
	}

	/*  Open E. file on stdout */
	fp2 = freopen(fileoftype('E', *batchv), "w", stdout);
	if (fp2 == NULL) {
#ifdef SVR4_1
		pfmt(stderr, MM_ERROR, ":49:error opening error file: %s\n", strerror(errno));
#else
		perror("smtp: error opening error file");
#endif
		exit(1);
	}

	/* Setup internal variables based upon ctrl file contents */
	setupargs(argc, argv, unixformat, sender, recips, domain, addr, host);
}

/*
 *  Called when the current input file is bad.  To avoid tripping over this
 *  file again, the control file is renamed (so that it can be returned later).
 */
void bad_input_file()
{
	if (batchv) {
		(void) link(*batchv, fileoftype('c', *batchv));
		(void) unlink(*batchv);
	}
}
