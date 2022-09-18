/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mktmp.c	1.3.2.2"
#ident "@(#)mktmp.c	1.3 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	mktmp - make temporary file for letter

    SYNOPSIS
	void mktmp(Tmpfile *ptmpfile)

    DESCRIPTION
	mktmp() creates a temp file for use by mail.
*/

void mktmp(ptmpfile)
Tmpfile *ptmpfile;
{
	static const char pn[] = "mktmp";
	mode_t	omask;

	ptmpfile->lettmp = tempnam(tmpdir,"mail");
	if (!ptmpfile->lettmp) {
		pfmt(stderr, MM_ERROR, "%s: Can't create temp file: %s\n", strerror(errno));
		error = E_TMP;
		Dout(pn, 0, "Can't create temp file\n", progname);
		done(0);		
	}

	/* Protect the temp file from prying eyes...*/
	omask = umask(077);
	ptmpfile->tmpf = doopen(ptmpfile->lettmp, "w+", E_TMP);
	(void) umask (omask);
}
