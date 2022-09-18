/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xau:AuGetAddr.c	1.2"
/*
 * Xau - X Authorization Database Library
 *
 * $XConsortium: AuGetAddr.c,v 1.11 91/01/08 15:09:05 gildea Exp $
 *
 * Copyright 1988 Massachusetts Institute of Technology
 *
 *
 * Author:  Keith Packard, MIT X Consortium
 */

#include <X11/Xauth.h>
#include <X11/Xos.h>

static
binaryEqual (a, b, len)
register char	*a, *b;
register int	len;
{
    while (len--)
	if (*a++ != *b++)
	    return 0;
    return 1;
}

#if NeedFunctionPrototypes
Xauth *
XauGetAuthByAddr (
#if NeedWidePrototypes
unsigned int	family,
unsigned int	address_length,
#else
unsigned short	family,
unsigned short	address_length,
#endif
_Xconst char*	address,
#if NeedWidePrototypes
unsigned int	number_length,
#else
unsigned short	number_length,
#endif
_Xconst char*	number,
#if NeedWidePrototypes
unsigned int	name_length,
#else
unsigned short	name_length,
#endif
_Xconst char*	name)
#else
Xauth *
XauGetAuthByAddr (family, address_length, address,
			  number_length, number,
			  name_length, name)
unsigned short	family;
unsigned short	address_length;
char	*address;
unsigned short	number_length;
char	*number;
unsigned short	name_length;
char	*name;
#endif
{
    FILE    *auth_file;
    char    *auth_name;
    Xauth   *entry;

    auth_name = XauFileName ();
    if (!auth_name)
	return 0;
    if (access (auth_name, R_OK) != 0)		/* checks REAL id */
	return 0;
    auth_file = fopen (auth_name, "r");
    if (!auth_file)
	return 0;
    for (;;) {
	entry = XauReadAuth (auth_file);
	if (!entry)
	    break;
	/*
	 * Match when:
	 *   either family or entry->family are FamilyWild or
	 *    family and entry->family are the same
	 *  and
	 *   either address or entry->address are empty or
	 *    address and entry->address are the same
	 *  and
	 *   either number or entry->number are empty or
	 *    number and entry->number are the same
	 *  and
	 *   either name or entry->name are empty or
	 *    name and entry->name are the same
	 */

	if ((family == FamilyWild || entry->family == FamilyWild ||
	     (entry->family == family &&
	      address_length == entry->address_length &&
	      binaryEqual (entry->address, address, (int)address_length))) &&
	    (number_length == 0 || entry->number_length == 0 ||
	     (number_length == entry->number_length &&
	      binaryEqual (entry->number, number, (int)number_length))) &&
	    (name_length == 0 || entry->name_length == 0 ||
	     (entry->name_length == name_length &&
 	      binaryEqual (entry->name, name, (int)name_length))))
	    break;
	XauDisposeAuth (entry);
    }
    (void) fclose (auth_file);
    return entry;
}
