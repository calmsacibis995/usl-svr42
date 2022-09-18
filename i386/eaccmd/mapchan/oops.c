/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)eac:i386/eaccmd/mapchan/oops.c	1.2.2.2"
#ident  "$Header: oops.c 1.2 91/07/04 $"
/*
 *
 *	Copyright (C) The Santa Cruz Operation, 1984, 1985, 1986, 1987, 1988.
 *	Copyright (C) Microsoft Corporation, 1984, 1985, 1986, 1987, 1988.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */
#include <stdio.h>

/*
 * print an error message on stderr.
 * this is only for syntactic brevity.
 */
error(fmt, args)
char *fmt;
int args;
{
	vfprintf(stderr, fmt, &args);
}

/*
 * print an error message and quit. 
 * again, only for syntactic brevity.
 */
oops(fmt, args)
char *fmt;
int args;
{
	vfprintf(stderr, fmt, &args);
	exit(1);
}
