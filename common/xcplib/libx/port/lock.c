/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)xcplibx:common/xcplib/libx/port/lock.c	1.4"
#ident  "$Header: lock.c 1.1 91/07/04 $"

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

/***	lock --  lock a process in primary memory
 *
 *	SYNOPSIS
 *	  int lock(op);
 *
 *	if (op)
 *		plock(PROCLOCK);
 *	else
 *		plock(UNLOCK);
 */

#include <sys/lock.h>

int
lock(op)
int op;
{
	return(plock( op? PROCLOCK : UNLOCK));
}
