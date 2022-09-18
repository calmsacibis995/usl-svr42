/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:group/groupdel.c	1.3.18.2"
#ident  "$Header: groupdel.c 2.0 91/07/13 $"

/*
 * Command:	groupdel
 *
 * Usage:	groupdel group
 *
 * Inheritable Privileges:	P_MACWRITE,P_SETFLEVEL,P_DACWRITE,
 *				P_MACREAD,P_DACREAD
 *       Fixed Privileges:	None
 *
 * Notes:	Delete a group definition from the system.
 *
 *		Arguments are:
 *
 *			group - a character string group name
 *
 *		P_MACWRITE is required for renaming tmp file to /etc/group.
 *		P_SETFLEVEL is required to reset level of that file.
 */

/* LINTLIBRARY */
#include <sys/types.h>
#include <stdio.h>
#include <userdefs.h>
#include <priv.h>
#include "messages.h"
#include <pfmt.h>
#include <locale.h>

char *msg_label = "UX:groupdel";

extern void errmsg(), exit();
extern int del_group();

 /*
  * Procedure:     main
  *
  * Restrictions:  none
  */

main(argc, argv)
int argc;
char **argv;
{
	register char *group;			/* group name from command line */
	register retval = 0;

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxcore");
	(void) setlabel(msg_label);

	if( argc != 2 ) {
		errmsg( M_DUSAGE );
		exit( EX_SYNTAX );
	}

	group = argv[1];

	switch( retval = del_group( group ) ) {
	case EX_UPDATE:
		errmsg( M_UPDATE, "deleted" );
		break;
		
	case EX_NAME_NOT_EXIST:
		errmsg( M_NO_GROUP, group );
		break;
	}

	exit(retval);
	/*NOTREACHED*/
}
