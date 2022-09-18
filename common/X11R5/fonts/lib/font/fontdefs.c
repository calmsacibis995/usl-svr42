/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5fontlib:font/fontdefs.c	1.7"
/*copyright     "%c%"*/

#include <stdio.h>

#include "Xos.h"
#include "Xrenderer.h"

char *get_xwinhome();

char * GetFontDefaults();
char *GetConfigFileName();

extern char *fontconfig_file;

#ifndef FONTCONFIG
#define FONTCONFIG "Xwinfont"
#endif


char *
GetConfigFileName()
{
int i;
register char *cp=NULL;
char *fontconfig;
char *envirpath;
char line[128];
	envirpath = (char *) getenv("XWINFONTCONFIG");
	if ((envirpath != (char *) 0) && (strlen(envirpath)> (unsigned)0)) {
		fontconfig = envirpath;
		return(fontconfig);
		}
	cp = (char *) get_xwinhome ("defaults");
	if (cp !=  (char *) 0) {
		strcpy(line,cp);
		strcat(line,"/");
		strcat(line,FONTCONFIG);
    	}
	else
		strcpy(line,"/usr/X/defaults/Xwinfont");
        cp = (char *)line[0];
        i = strlen(line)+1;
        fontconfig = (char *)xalloc(i);
        if (!fontconfig) return NULL;
        memcpy(fontconfig,line,i);
        return(fontconfig);
}

char *
GetFontDefaults(defaultFontPath)
char *defaultFontPath;
{
    char *filefontpath;
    char *envirpath;
    char *compilepath;

    compilepath = defaultFontPath;
    envirpath = (char *) getenv("XWINFONTPATH");
    if ((envirpath != (char *) 0)  && (strlen(envirpath)> (unsigned)0)) {
	defaultFontPath = envirpath;
    } else {
	filefontpath  = (char *) GetConfigFontPath(defaultFontPath);
	if (filefontpath != NULL) defaultFontPath=filefontpath;
    }
    return(defaultFontPath);
}



/* Returns the location of X 
 * If the environment variable XWINHOME is set the value returned is
 * $(XWINHOME)/name.  Else, if this module has been compiled with
 * XWINHOME set then the value returned is $(XWINHOME)/name.
 * Else, "/usr/X/name" is returned.
 */

char *
get_xwinhome (name)

char *name;
{
    static char *path = (char *)0;
    static char *env = (char *)0;

    if (name[0] == '/') {
	return (name);
    }
    if (env == (char *)0) {
	if ((env = (char *)getenv ("XWINHOME")) == (char *)0) {
#ifdef XWINHOME
            env = XWINHOME;
#else
            env = "/usr/X";
#endif /* XWINHOME */
	}
    }
    if (path != (char *)0) {
	xfree (path);
    }
    path = (char *) xalloc (strlen(env) + strlen(name) + 2);
 
    (void)strcpy (path, env);
    (void)strcat (path, "/");
    (void)strcat (path, name);
    return (path);
}
