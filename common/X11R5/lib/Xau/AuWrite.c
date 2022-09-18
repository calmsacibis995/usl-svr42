/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xau:AuWrite.c	1.2"
/*
 * Xau - X Authorization Database Library
 *
 * $XConsortium: AuWrite.c,v 1.5 91/01/30 12:28:19 gildea Exp $
 *
 * Copyright 1988 Massachusetts Institute of Technology
 *
 *
 * Author:  Keith Packard, MIT X Consortium
 */

#include <X11/Xauth.h>

static
write_short (s, file)
unsigned short	s;
FILE		*file;
{
    unsigned char   file_short[2];

    file_short[0] = (s & (unsigned)0xff00) >> 8;
    file_short[1] = s & 0xff;
    if (fwrite ((char *) file_short, (int) sizeof (file_short), 1, file) != 1)
	return 0;
    return 1;
}

static
write_counted_string (count, string, file)
unsigned short	count;
char	*string;
FILE	*file;
{
    if (write_short (count, file) == 0)
	return 0;
    if (fwrite (string, (int) sizeof (char), (int) count, file) != count)
	return 0;
    return 1;
}

int
XauWriteAuth (auth_file, auth)
FILE	*auth_file;
Xauth	*auth;
{
    char    *malloc ();

    if (write_short (auth->family, auth_file) == 0)
	return 0;
    if (write_counted_string (auth->address_length, auth->address, auth_file) == 0)
	return 0;
    if (write_counted_string (auth->number_length, auth->number, auth_file) == 0)
	return 0;
    if (write_counted_string (auth->name_length, auth->name, auth_file) == 0)
	return 0;
    if (write_counted_string (auth->data_length, auth->data, auth_file) == 0)
	return 0;
    return 1;
}
