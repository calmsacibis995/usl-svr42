/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xau:AuLock.c	1.3"
/*
 * Xau - X Authorization Database Library
 *
 * $XConsortium: AuLock.c,v 1.8 91/12/16 19:56:07 gildea Exp $
 *
 * Copyright 1988 Massachusetts Institute of Technology
 *
 * Author:  Keith Packard, MIT X Consortium
 */

#include <X11/Xauth.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

extern int errno;

#if NeedFunctionPrototypes
int
XauLockAuth (
_Xconst char *file_name,
int	retries,
int	timeout,
long	dead)
#else
int
XauLockAuth (file_name, retries, timeout, dead)
char	*file_name;
int	retries;
int	timeout;
long	dead;
#endif
{
    char	creat_name[1025], link_name[1025];
    char	*strcpy (), *strcat ();
    long	time ();
    unsigned	sleep ();
    struct stat	statb;
    long	now;
    int		creat_fd = -1;

    if (strlen (file_name) > 1022)
	return LOCK_ERROR;
    (void) strcpy (creat_name, file_name);
    (void) strcat (creat_name, "-c");
    (void) strcpy (link_name, file_name);
    (void) strcat (link_name, "-l");
    if (stat (creat_name, &statb) != -1) {
	now = time ((long *) 0);
	/*
	 * NFS may cause ctime to be before now, special
	 * case a 0 deadtime to force lock removal
	 */
	if (dead == 0 || now - statb.st_ctime > dead) {
	    (void) unlink (creat_name);
	    (void) unlink (link_name);
	}
    }
    
    while (retries > 0) {
	if (creat_fd == -1) {
	    creat_fd = creat (creat_name, 0666);
	    if (creat_fd == -1) {
		if (errno != EACCES)
		    return LOCK_ERROR;
	    } else
		(void) close (creat_fd);
	}
	if (creat_fd != -1) {
	    if (link (creat_name, link_name) != -1)
		return LOCK_SUCCESS;
	    if (errno == ENOENT) {
		creat_fd = -1;	/* force re-creat next time around */
		continue;
	    }
	    if (errno != EEXIST)
		return LOCK_ERROR;
	}
	(void) sleep ((unsigned) timeout);
	--retries;
    }
    return LOCK_TIMEOUT;
}
