/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xau:AuUnlock.c	1.2"
/*
 * Xau - X Authorization Database Library
 *
 * $XConsortium: AuUnlock.c,v 1.5 91/04/17 11:00:11 rws Exp $
 *
 * Copyright 1988 Massachusetts Institute of Technology
 *
 *
 * Author:  Keith Packard, MIT X Consortium
 */

#include <X11/Xauth.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef X_NOT_POSIX
#include <errno.h>
#else
#include <sys/errno.h>
#endif

#if NeedFunctionPrototypes
XauUnlockAuth (
_Xconst char *file_name)
#else
XauUnlockAuth (file_name)
char	*file_name;
#endif
{
    char	creat_name[1025], link_name[1025];
    char	*strcpy (), *strcat ();

    if (strlen (file_name) > 1022)
	return;
    (void) strcpy (creat_name, file_name);
    (void) strcat (creat_name, "-c");
    (void) strcpy (link_name, file_name);
    (void) strcat (link_name, "-l");
    /*
     * I think this is the correct order
     */
    (void) unlink (creat_name);
    (void) unlink (link_name);
}
