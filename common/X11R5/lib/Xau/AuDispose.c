/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xau:AuDispose.c	1.2"
/*
 * Xau - X Authorization Database Library
 *
 * $XConsortium: AuDispose.c,v 1.3 91/01/08 15:08:21 gildea Exp $
 *
 * Copyright 1988 Massachusetts Institute of Technology
 *
 *
 * Author:  Keith Packard, MIT X Consortium
 */

#include <X11/Xauth.h>

void
XauDisposeAuth (auth)
Xauth	*auth;
{
    if (auth) {
	if (auth->address) (void) free (auth->address);
	if (auth->number) (void) free (auth->number);
	if (auth->name) (void) free (auth->name);
	if (auth->data) (void) free (auth->data);
	free ((char *) auth);
    }
    return;
}
