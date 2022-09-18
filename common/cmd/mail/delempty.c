/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/delempty.c	1.4.2.2"
#ident "@(#)delempty.c	1.5 'attmail mail(1) command'"
#include "libmail.h"
/*
    NAME
	delempty - delete an empty mail box

    SYNOPSIS
	int delempty(mode_t mode, const char *mailname)

    DESCRIPTION
	Delete an empty mail box if it's allowed. Check
	the value of mgetenv("DEL_EMPTY_MFILE") for
	"yes" (always), "no" (never) or the default (based
	on the mode).	
*/

int delempty(mode, mailname)
mode_t mode;
const char *mailname;
{
	char *del_empty = Mgetenv("DEL_EMPTY_MFILE");
	int del_len = strlen(del_empty);
	int do_del = 0;

	/* "yes" means always remove the mailfile */
	if (casncmp(del_empty, "yes", del_len) == 0)
		do_del = 1;
	/* "no" means never remove the mailfile */
	else if (casncmp(del_empty, "no", del_len) == 0)
		/* EMPTY */;
	/* all other values say to check for mode 0660 */
	else if ((mode & 07777) == MFMODE)
		do_del = 1;

	if (do_del)
		unlink(mailname);
	return do_del;
}
