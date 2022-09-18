/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:user/isbusy.c	1.3.9.2"
#ident  "$Header: isbusy.c 2.0 91/07/13 $"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <utmp.h>

/* Maximium logname length - hard coded in utmp.h */
#define	MAXID	8

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

struct utmp *getutent();

/*
 * Procedure:	isbusy
 *
 * Restrictions:
 *		getutent:	None
 *
 * Notes:	Is this login being used? 
*/
isbusy(login)
	register char *login;
{
	register struct utmp *utptr;

	while ((utptr = getutent()) != NULL)
		if (!strncmp( login, utptr->ut_user, MAXID ) &&
			utptr->ut_type != DEAD_PROCESS)
			return TRUE;

	return FALSE;
}
