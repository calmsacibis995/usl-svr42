/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)tfm:err.c	1.4.2.2"
#ident  "$Header: err.c 1.2 91/06/27 $"
#include <stdio.h>
#include <locale.h>
#include <pfmt.h>
#include "err.h"

void	exit();

/*
** Function: tfm_report()
**
** This function handles printing of errors in international format.
** The error message text is taken from the message buffer which is
** set up by the failing routine.  The severity is set by the failing
** routine.
*/

void
tfm_report(msgbuf)
struct	msg	*msgbuf;
{
	(void)pfmt(stderr,msgbuf->sev,msgbuf->text,msgbuf->args[0],
							msgbuf->args[1],
							msgbuf->args[2],
							msgbuf->args[3]);
	if(msgbuf->act == ERR_QUIT){
		exit(1);
	}
	msgbuf->sev = ERR_NONE;
	msgbuf->act = ERR_CONTINUE;
	msgbuf->text[0] = 0;
}

