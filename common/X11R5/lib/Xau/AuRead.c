/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xau:AuRead.c	1.2"
/*
 * Xau - X Authorization Database Library
 *
 * $XConsortium: AuRead.c,v 1.5 91/01/08 15:09:31 gildea Exp $
 *
 * Copyright 1988 Massachusetts Institute of Technology
 *
 *
 * Author:  Keith Packard, MIT X Consortium
 */

#include <X11/Xauth.h>

static
read_short (shortp, file)
unsigned short	*shortp;
FILE		*file;
{
    unsigned char   file_short[2];

    if (fread ((char *) file_short, (int) sizeof (file_short), 1, file) != 1)
	return 0;
    *shortp = file_short[0] * 256 + file_short[1];
    return 1;
}

static
read_counted_string (countp, stringp, file)
unsigned short	*countp;
char	**stringp;
FILE	*file;
{
    unsigned short  len;
    char	    *data, *malloc ();

    if (read_short (&len, file) == 0)
	return 0;
    if (len == 0) {
	data = 0;
    } else {
    	data = malloc ((unsigned) len);
    	if (!data)
	    return 0;
    	if (fread (data, (int) sizeof (char), (int) len, file) != len) {
	    free (data);
	    return 0;
    	}
    }
    *stringp = data;
    *countp = len;
    return 1;
}

Xauth *
XauReadAuth (auth_file)
FILE	*auth_file;
{
    Xauth   local;
    Xauth   *ret;
    char    *malloc ();

    if (read_short (&local.family, auth_file) == 0)
	return 0;
    if (read_counted_string (&local.address_length, &local.address, auth_file) == 0)
	return 0;
    if (read_counted_string (&local.number_length, &local.number, auth_file) == 0) {
	if (local.address) free (local.address);
	return 0;
    }
    if (read_counted_string (&local.name_length, &local.name, auth_file) == 0) {
	if (local.address) free (local.address);
	if (local.number) free (local.number);
	return 0;
    }
    if (read_counted_string (&local.data_length, &local.data, auth_file) == 0) {
	if (local.address) free (local.address);
	if (local.number) free (local.number);
	if (local.name) free (local.name);
	return 0;
    }
    ret = (Xauth *) malloc (sizeof (Xauth));
    if (!ret) {
	if (local.address) free (local.address);
	if (local.number) free (local.number);
	if (local.name) free (local.name);
	if (local.data) free (local.data);
	return 0;
    }
    *ret = local;
    return ret;
}
