/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtm:olwsm/xutil.c	1.7"
#endif

/*
 *      Taken from OPEN LOOK(tm) File Manager
 *
 *      xutil.c
 *
 */

#include <stdio.h>
#include <X11/Intrinsic.h>

#include "error.h"
#include <misc.h>
#include <wsm.h>  

#define ENVHELPPATH     "OLWSMHELPPATH"

extern char * getenv();
extern char *OlGetMessage();

static char* MakePath();

static char * MakePath(dir, base)
char * dir;
char * base;
{
  char * path;
  
  if (dir != NULL && dir[0] != '\0')
    {
      path = MALLOC(strlen(dir) + strlen(base) + 2);
      (void) strcpy(path, dir);
      (void) strcat(path, "/");
      (void) strcat(path, base);
    }
  else
    {
      path = MALLOC(strlen(base) + 1);
      (void) strcpy(path, base);
    }
  
  return (path);
  
} /* end of MakePath */

