/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:cmd/lpadmin/startup.c	1.6.6.2"
#ident  "$Header: startup.c 1.2 91/06/27 $"

#include "stdio.h"

#include "lp.h"
#include "msgs.h"

#include "lpadmin.h"


/*
 * Procedure:     startup
 *
 * Restrictions:
 *               mopen: None
 * notes - OPEN CHANNEL TO SPOOLER
*/

void			startup ()
{
	trap_signals ();

	if (mopen() == -1)
		scheduler_active = 0;
	else
		scheduler_active = 1;

	return;
}
