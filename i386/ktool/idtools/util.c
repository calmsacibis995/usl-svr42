/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)idtools:i386/ktool/idtools/util.c	1.1"
#ident	"$Header:"

#include "inst.h"
#include <stdio.h>

char current[LINESZ];
char errbuf[LINESZ];
char linebuf[LINESZ];
short eflag;

/* print error message and set flag indicating error */

error(b)
int b;
{
        if (b)
                fprintf(stderr, "LINE: %s\n", linebuf);
        fprintf(stderr, "ERROR: %s\n", errbuf);
        eflag++;
}



/* print warning message */

warning(b)
int b;
{
        if (b)
                fprintf(stderr, "LINE: %s\n", linebuf);
        fprintf(stderr, "WARNING: %s\n", errbuf);
}



/* fatal error - exit */

fatal(b)
int b;
{
        if (b)
                fprintf(stderr, "LINE: %s\n", linebuf);
        fprintf(stderr, "FATAL ERROR: %s\n", errbuf);
        exit(1);
}


/* handle errors from getinst() */

insterror(errcode, ftype, dev)
int errcode;
int ftype;
char *dev;
{
	insterrmsg(errcode, ftype, dev);
	fatal(errcode != IERR_OPEN);
}


/* construct full path name */

getpath(flag, buf, def)
int flag;
char *buf, *def;
{
	switch (flag) {
	case 0:
		strcpy(buf, def);
		break;
	case 1:
		if (chdir(buf) != 0) {
			fprintf(stderr, "Cannot chdir to %s.\n", buf);
			fatal(0);
		}
		getcwd(buf, LINESZ);
		chdir(current);
		break;
	}
}
