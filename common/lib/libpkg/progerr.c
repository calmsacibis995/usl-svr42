/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
#ident	"@(#)libpkg:common/lib/libpkg/progerr.c	1.5.5.8"
#ident "$Header: $"

#include <stdio.h>
#include <varargs.h>
#include <pwd.h>
#include <limits.h>

extern char	*prog;

int	logmode;

/*VARARGS*/
void
progerr(va_alist)
va_dcl
{
	va_list	ap;
	char	*fmt;
	FILE	*logfp = NULL;		/* log file pointer		*/
	char	lname[PATH_MAX];	/* log file name		*/
	char	*pkginst, *getenv();	/* PKGINST in environment	*/


	/*
	 * If error being reported occured during package installation,
	 * and either LOGMODE environment variable was set to "true"
	 * by user or via the -l option on the pkgadd command line
	 * and we know the package instance, do not display error
	 * message on screen.   Instead, place errors in logfile
	 * /var/sadm/install/logs/<pkginst>.log.  Otherwise, the 
	 * error message goes to stderr and logfile. 
	 */

	if(!strcmp(prog, "pkgadd")) {
		pkginst = getenv("PKGINST");
		if (pkginst) {
			(void) sprintf(lname, "/var/sadm/install/logs/%s.log", pkginst);
			if((logfp = fopen(lname, "a")) == NULL)
				fprintf(stderr, "%s: WARNING: could not open log file <%s>\n", lname);
		}
	}
		
	va_start(ap);

	fmt = va_arg(ap, char *);
	if(!logmode) {
		(void) fprintf(stderr, "%s: ERROR: ", prog);
		(void) vfprintf(stderr, fmt, ap);
		(void) fprintf(stderr, "\n");
	}
	if(logfp) {
		(void) fprintf(logfp, "%s: ERROR: ", prog);
		(void) vfprintf(logfp, fmt, ap);
		(void) fprintf(logfp, "\n");
		(void) fclose(logfp);
	}

	va_end(ap);

}
