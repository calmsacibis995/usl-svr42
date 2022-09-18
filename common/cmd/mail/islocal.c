/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/islocal.c	1.7.2.3"
#ident "@(#)islocal.c	1.9 'attmail mail(1) command'"
#include "libmail.h"
/*
    NAME
	islocal - see if user exists on this system

    SYNOPSIS
	int islocal(const char *user, uid_t *puid)

    DESCRIPTION
	Return an indication if the given name has or can have a
	mailbox on the system. If so, also return their uid.
*/

int islocal(user, puid)
const char *user;
uid_t *puid;
{
	string	*fname;
	struct stat statb;
	struct passwd *pwd_ptr;

	/* Check for existing mailfile first */
	fname = s_copy(maildir);
	fname = s_append(fname, user);
	if (stat(s_to_c(fname),&statb) == 0) {
		if (puid) *puid = statb.st_uid;
		s_free(fname);
		return 1;
	}

	/* Check for existing forwarding file next */
	s_restart(fname);
	fname = s_xappend(fname, mailfwrd, user, (char*)0);
	if (stat(s_to_c(fname),&statb) == 0) {
		if (puid) *puid = statb.st_uid;
		s_free(fname);
		return 1;
	}

	/* If no existing mailfile, check passwd file */
	setpwent();	
	if ((pwd_ptr = getpwnam(user)) == NULL) {
		s_free(fname);
		return 0;
	}
	if (puid) *puid = pwd_ptr->pw_uid;
	s_free(fname);
	return 1;
}
