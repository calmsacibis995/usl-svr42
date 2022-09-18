/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:os/osinit.c	1.2"
/*copyright     "%c%"*/

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.
                        All Rights Reserved
******************************************************************/
/* $XConsortium: osinit.c,v 1.28 89/12/18 15:41:25 rws Exp $ */

#ifndef BSD
#include "os.h"
#include "opaque.h"
#undef NULL
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include "rgb.h"

#ifndef ADMPATH
#define ADMPATH "/usr/adm/X%smsgs"
#endif

int rgb_size = -1;
RGB_BASE *rgb_db;

int	rgb_dbm = 0;
extern char *display;
extern char *GetXWINHome();

OsInit()
{
    int rgb_file;
    char *tmp;
    char *end_o_path_prefix;
    char path [BUFSIZ];
    struct stat stat_buf;

    if(!rgb_dbm) {
	if (CLrgbPath && (*CLrgbPath != '\0'))
	    tmp = GetXWINHome(CLrgbPath);
	else
	    tmp = GetXWINHome(rgbPath); /* rgbPath is lib/rgb */

	strcpy(path, tmp);
	end_o_path_prefix = path + strlen(path);
	
	/* open rgb data base file, ie: rgb.base */
	strcat (end_o_path_prefix, ".base");
	if ((rgb_file = open (path, O_RDONLY)) == -1) {
		ErrorF( "Couldn't open RGB_DB '%s':  exiting\n", path );
		AbortServer ();	/* necessary?  maybe we could limp along... */
	}

	if (fstat (rgb_file, &stat_buf) == -1) {
	    ErrorF( "Couldn't get size of RGB_DB '%s'\n", path );
	    AbortServer ();	/* ditto */
	}
	rgb_db = (RGB_BASE *) malloc (stat_buf.st_size);
	rgb_size = stat_buf.st_size / sizeof (RGB_BASE);
	if (read (rgb_file, rgb_db, stat_buf.st_size) != stat_buf.st_size) {
	    ErrorF( "Couldn't read %s\n", path );
	    AbortServer ();	/* ditto */
	}
	rgb_dbm = 1;
	close (rgb_file);
    }
}
#else
#include "os.h"
#include "opaque.h"
#undef NULL
#ifdef NDBM
#include <ndbm.h>
#else
#include <dbm.h>
#endif
#undef NULL
#include <stdio.h>
#include "Xos.h"
#ifndef MAXPATHLEN
/*
 * just to get MAXPATHLEN.  Define it elsewhere if you need to
 * avoid these files.
 */
#include <sys/param.h>
#endif

#ifdef SVR4
#define SYSV YES
#endif

#ifndef SYSV
#include <sys/resource.h>
#endif

#ifndef ADMPATH
#define ADMPATH "/usr/adm/X%smsgs"
#endif

#ifdef NDBM
DBM     *rgb_dbm = (DBM *)NULL;
#else
int	rgb_dbm = 0;
#endif
extern char *display;
#ifndef SYSV
int limitDataSpace = -1;
int limitStackSpace = -1;
#endif

OsInit()
{
    static Bool been_here = FALSE;
    char fname[MAXPATHLEN];

#ifdef macII
    set42sig();
#endif

    /* hack test to decide where to log errors */

    if (!been_here) {
	if (write (2, fname, 0)) 
	{
	    long t; 
	    char *ctime();
	    FILE *err;
	    fclose(stdin);
	    fclose(stdout);
	    sprintf (fname, ADMPATH, display);
	    /*
	     * uses stdio to avoid os dependencies here,
	     * a real os would use
 	     *  open (fname, O_WRONLY|O_APPEND|O_CREAT, 0666)
	     */
	    if (!(err = fopen (fname, "a+")))
		err = fopen ("/dev/null", "w");
	    if (err && (fileno(err) != 2)) {
		dup2 (fileno (err), 2);
		fclose (err);
	    }
#ifdef SYSV		/* yes, even though it is 4.2bsd.... */
	    {
	    static char buf[BUFSIZ];
	    setvbuf (stderr, buf, _IOLBF, BUFSIZ);
	    }
#else
	    setlinebuf(stderr);
#endif
	    time (&t);
	    fprintf (stderr, "start %s", ctime(&t));
	}

	if (getpgrp (0) == 0)
	    setpgrp (0, getpid ());


#ifndef SYSV
#if !defined(AIXrt) && !defined(AIX386)
	if (limitDataSpace >= 0)
	{
	    struct rlimit	rlim;

	    if (!getrlimit(RLIMIT_DATA, &rlim))
	    {
		if ((limitDataSpace > 0) && (limitDataSpace < rlim.rlim_max))
		    rlim.rlim_cur = limitDataSpace;
		else
		    rlim.rlim_cur = rlim.rlim_max;
		(void)setrlimit(RLIMIT_DATA, &rlim);
	    }
	}
	if (limitStackSpace >= 0)
	{
	    struct rlimit	rlim;

	    if (!getrlimit(RLIMIT_STACK, &rlim))
	    {
		if ((limitStackSpace > 0) && (limitStackSpace < rlim.rlim_max))
		    rlim.rlim_cur = limitStackSpace;
		else
		    rlim.rlim_cur = rlim.rlim_max;
		(void)setrlimit(RLIMIT_STACK, &rlim);
	    }
	}
#endif
#endif
	been_here = TRUE;
    }

    if (!rgb_dbm)
    {
#ifdef NDBM
	rgb_dbm = dbm_open(rgbPath, 0, 0);
#else
	if (dbminit(rgbPath) == 0)
	    rgb_dbm = 1;
#endif
	if (!rgb_dbm)
	    ErrorF( "Couldn't open RGB_DB '%s'\n", rgbPath );
    }
}
#endif
