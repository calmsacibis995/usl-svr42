/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:GetHost.c	1.2"
/*
 * $XConsortium: GetHost.c,v 1.2 90/12/14 19:24:38 converse Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Jim Fulton, MIT X Consortium
 *
 * _XGetHostname - similar to gethostname but allows special processing.
 */

int XmuGetHostname (buf, maxlen)
    char *buf;
    int maxlen;
{
    int len;

#ifdef USG
#define NEED_UTSNAME
#endif

#ifdef NEED_UTSNAME
#include <sys/utsname.h>
    /*
     * same host name crock as in server and xinit.
     */
    struct utsname name;

    uname (&name);
    len = strlen (name.nodename);
    if (len >= maxlen) len = maxlen - 1;
    strncpy (buf, name.nodename, len);
    buf[len] = '\0';
#else
    buf[0] = '\0';
    (void) gethostname (buf, maxlen);
    buf [maxlen - 1] = '\0';
    len = strlen(buf);
#endif /* hpux */
    return len;
}

