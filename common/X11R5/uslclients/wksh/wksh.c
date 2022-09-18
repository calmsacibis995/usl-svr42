/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1991, 1992 UNIX System Laboratories, Inc. */
/*	All Rights Reserved     */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF          */
/*	UNIX System Laboratories, Inc.			        */
/*	The copyright notice above does not evidence any        */
/*	actual or intended publication of such source code.     */

#ident	"@(#)wksh:wksh.c	1.4"

#include <stdio.h>

#define CONSTCHAR (const char *)
#define TRUE 1
#define FALSE 0

#ifndef WKSHLIBDIR
#ifdef SVR4
#define WKSHLIBDIR "/usr/X/lib/wksh"
#else
#ifdef sun4
#define WKSHLIBDIR "/usr/openwin/lib/wksh"
#else
#ifdef SCO
#define WKSHLIBDIR "/usr/lib/X11/wksh"
#else
#define WKSHLIBDIR "/usr/lib/wksh"
#endif
#endif
#endif
#endif

#ifndef WKSHBINDIR
#ifdef SVR4
#define WKSHBINDIR "/usr/X/bin"
#else
#ifdef sun4
#define WKSHBINDIR "/usr/openwin/bin"
#else
#ifdef SCO
#define WKSHBINDIR "/usr/bindir/X11"
#else
#define WKSHBINDIR "/usr/bin"
#endif
#endif
#endif
#endif

/*
 * Bootstrap wksh by calling either olwksh or xmwksh and forcing it to execute
 * an rc file that calls the wksh libdirrary init function and does some
 * other minor housekeeping.
 *
 * The rc file then sees if there was a previous user rc file (in $_HOLDENV_)
 * and if so executes it.
 *
 * The user can choose motif vs. open look by making the first
 * command line option -motif or -openlook.  If neither of these options
 * is chosen, then next it looks to see if the variable WKSH_TOOLKIT is
 * set to either the string MOTIF or OPENLOOK.  Failing this, it defaults
 * to OPENLOOK (hey, this is a USL product, what do you think?)
 */

int
main(argc, argv)
int argc;
char *argv[];
{
	char *getenv();
	char *env;
	char *libdir, *bindir;
	char *buf;
	char envbuf[1024];
	char *getenv();
#ifndef MEMUTIL
	char *malloc();
#endif /* MEMUTIL */
#define GUI_UNKNOWN  0
#define GUI_OPENLOOK 1
#define GUI_MOTIF    2
	int gui = GUI_UNKNOWN;
	int need_compress = 0;

	/*
	 * Set the ENV variable to the standard wksh rc file, which
	 * will do a libdirload of the wksh shared object file and add all
	 * the wksh commands and widgets to the xksh.  If the user already
	 * had an ENV file, then set the variable _HOLDENV_ to it so the
	 * standard wksh rc file can execute it later.
	 */
	env = getenv((const char *)"ENV");
	buf = (char *)malloc((env ? strlen(env) : 0) + 12);
	strcpy(buf, "_HOLDENV_=");
	strcat(buf, env ? env : "");
	putenv(buf); 

	/*
	 * if the first argument is "-motif" or "-openlook" then
	 * use that widget set.
	 */

	need_compress = 0;
	if (argc > 1) {
		if (strcmp(argv[1], CONSTCHAR "-motif") == 0) {
			gui = GUI_MOTIF;
			need_compress = TRUE;
		} else if (strcmp(argv[1], CONSTCHAR "-openlook") == 0) {
			gui = GUI_OPENLOOK;
			need_compress = TRUE;
		}
	}
	/*
	 * if one of the special args was found, move down all other
	 * args to get rid of it.
	 */
	if (need_compress) {
		register int i;

		for (i = 1; i < argc; i++)
			argv[i] = argv[i+1];
		argc--;
	}
	/*
	 * If there is still no gui named, look for the variable
	 * WKSH_TOOLKIT to determine which to use.
	 */
	if (gui == GUI_UNKNOWN) {
		char *guivar = getenv(CONSTCHAR "WKSH_TOOLKIT");
		if (guivar && strcmp(guivar, CONSTCHAR "OPENLOOK") == 0) {
			gui = GUI_OPENLOOK;
		} else if (guivar && strcmp(guivar, CONSTCHAR "MOTIF") == 0) {
			gui = GUI_MOTIF;
		}
	}

	if ((libdir = getenv((const char *)"WKSHLIBDIR")) == NULL) {
		libdir = WKSHLIBDIR;
		sprintf(envbuf, "WKSHLIBDIR=%s", libdir);
		putenv(strdup(envbuf));
	}
	if ((bindir = getenv((const char *)"WKSHBINDIR")) == NULL)
		bindir = WKSHBINDIR;

	if (gui == GUI_MOTIF) {
		sprintf(envbuf, (const char *) "ENV=%s/xmwksh.rc", libdir);
		putenv(strdup(envbuf));
		sprintf(envbuf, (const char *) "%s/xmwksh", bindir);
		execv(envbuf, argv);
	} else {
		sprintf(envbuf, (const char *) "ENV=%s/olwksh.rc", libdir);
		putenv(strdup(envbuf));
		sprintf(envbuf, (const char *) "%s/olwksh", bindir);
		execv(envbuf, argv);
	}
	fprintf(stderr, (const char *)"%s: Bootstrap of wksh failed: '%s'\n", argv[0], envbuf);
	perror("Reason");
	return(1);	/* failure */
}
