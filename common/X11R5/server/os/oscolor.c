/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:os/oscolor.c	1.2"
/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/
/* $XConsortium: oscolor.c,v 1.17 89/12/18 15:41:43 rws Exp $ */
#ifndef BSD
#include "rgb.h"
#include <search.h>

/* Looks up the color in the database.  Note that we are assuming there
 * is only one database for all the screens.  If you have multiple databases,
 * remove the dbminit() in OsInit(), and open the appropriate database each
 * time. Or implement a database package that allows you to have more than
 * one database open at a time.
 */
extern int rgb_dbm;
extern int rgb_size;
extern RGB_BASE *rgb_db;

#ifndef SVR4
char *bsearch();
#endif

/*
 * Compare strings (at most n bytes) Ignores case
 *    	returns: s1>s2; >0  s1==s2; 0  s1<s2; <0
 */

int
stringcmp(node1, node2)
#ifdef __STDC__
const void *node1, *node2;
#else
RGB_BASE *node1, *node2;
#endif
{
  register char *s1 = ((RGB_BASE *)node1)->name;
  register char *s2 = ((RGB_BASE *)node2)->name;

  if(s1 == s2) {
    return(0);
  }
  while(toupper(*s1) == toupper(*s2++)) {
    if(*s1++ == '\0') {
      return(0);
    }
  }
  return((toupper(*s1) - toupper(*--s2)));
}

/*ARGSUSED*/
int
OsLookupColor(screen, name, len, pred, pgreen, pblue)
    int		screen;
    char	*name;
    unsigned	len;
    unsigned short	*pred, *pgreen, *pblue;

{
  RGB_BASE rgb, *prgb;

  if(!rgb_dbm)
    return(0);

  if (rgb_size == -1) {
    return(0);
  }
  len = (len >= RGB_NSIZE)? RGB_NSIZE-1 : len;
  strncpy (rgb.name, name, len);
  rgb.name[len] = '\0';
  if ((prgb = (RGB_BASE *)bsearch (&rgb, rgb_db, rgb_size,
		      sizeof (RGB_BASE), stringcmp)) == (RGB_BASE *)0) {
    return (0);
  }
  *pred = prgb->red;
  *pgreen = prgb->green;
  *pblue = prgb->blue;
  return (1);

}
#else
#ifdef NDBM
#include <ndbm.h>
#else
#include <dbm.h>
#endif
#include "rgb.h"
#include "os.h"

/* Looks up the color in the database.  Note that we are assuming there
 * is only one database for all the screens.  If you have multiple databases,
 * remove the dbminit() in OsInit(), and open the appropriate database each
 * time. Or implement a database package that allows you to have more than
 * one database open at a time.
 */
#ifdef NDBM
extern DBM *rgb_dbm;
#else
extern int rgb_dbm;
#endif

extern void CopyISOLatin1Lowered();

/*ARGSUSED*/
int
OsLookupColor(screen, name, len, pred, pgreen, pblue)
    int		screen;
    char	*name;
    unsigned	len;
    unsigned short	*pred, *pgreen, *pblue;

{
    datum		dbent;
    RGB			rgb;
    char	*lowername;

    if(!rgb_dbm)
	return(0);

    /* convert name to lower case */
    lowername = (char *)ALLOCATE_LOCAL(len + 1);
    if (!lowername)
	return(0);
    CopyISOLatin1Lowered ((unsigned char *) lowername, (unsigned char *) name,
			  (int)len);

    dbent.dptr = lowername;
    dbent.dsize = len;
#ifdef NDBM
    dbent = dbm_fetch(rgb_dbm, dbent);
#else
    dbent = fetch (dbent);
#endif

    DEALLOCATE_LOCAL(lowername);

    if(dbent.dptr)
    {
	bcopy(dbent.dptr, (char *) &rgb, sizeof (RGB));
	*pred = rgb.red;
	*pgreen = rgb.green;
	*pblue = rgb.blue;
	return (1);
    }
    return(0);
}
#endif
