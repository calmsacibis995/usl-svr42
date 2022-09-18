/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:user/expdate.c	1.2.11.2"
#ident  "$Header: expdate.c 2.0 91/07/13 $"

#include	<sys/types.h>
#include	<stdio.h>
#include	<userdefs.h>
#include	<users.h>
#include	<locale.h>
#include	<pfmt.h>

extern void exit();
extern int valid_expire();

/* Validate an expiration date */
main(argc, argv)
	char *argv[];
{
	if (argc != 2) {
		(void) pfmt(stderr, MM_ERROR, ":0:synopsis: expdate date\n");
		exit(EX_SYNTAX);
	}
	exit(valid_expire(argv[1], 0 ) == INVALID ? EX_FAILURE : EX_SUCCESS);
	/*NOTREACHED*/
}
