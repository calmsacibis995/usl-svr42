/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mkdead.c	1.10.2.2"
#ident "@(#)mkdead.c	2.14 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	mkdead - create dead.letter

    SYNOPSIS
	void mkdead(Msg *pmsg)

    DESCRIPTION
	mkdead() creates a dead.letter file.
*/

void mkdead(pmsg)
Msg *pmsg;
{
	static const char pn[] = "mkdead";
	int aret;
	static char dead[] = "/dead.letter";	/* name of dead.letter */
	char *dotdead = &dead[1];
	char	*hmdead;			/* pointer to $HOME/dead.letter */
	FILE *malf = 0;
	mode_t omask;

	/*
		Make certain that there's something to copy.
	*/
	if (!pmsg)
		return;

	/*
		Try to create dead letter in current directory
		or in home directory
	*/
	omask = umask(DEADMASK);
	if ((aret = legal(dotdead)) != 0)
		malf = fopen(dotdead, "a");
	if ((malf == NULL) || (aret == 0)) {
		/*
			try to create in $HOME
		*/
		if((hmdead = malloc((unsigned)(strlen(home) + strlen(dead) + 1))) == NULL) {
			pfmt(stderr, MM_ERROR,
				":10:Out of memory: %s\n", strerror(errno));
			Dout(pn, 0, "Cannot malloc.\n");
			goto out;
		}
		cat(hmdead, home, dead);
		if ((aret=legal(hmdead)) != 0)
			malf = fopen(hmdead, "a");
		if ((malf == NULL) || (aret == 0)) {
			pfmt(stderr, MM_ERROR,
				":123:Cannot create %s: %s\n",
				dotdead, strerror(errno));
			Dout(pn, 0, "Cannot create %s\n", dotdead);
		out:
			error = E_FILE;
			Dout(pn, 0, "error set to %d\n", error);
			(void) umask(omask);
			return;
		}  else {
			chmod(hmdead, DEADPERM);
			pfmt(stderr, MM_INFO,
				":124:Mail saved in '%s'\n", hmdead);
		}
	} else {
		chmod(dotdead, DEADPERM);
		pfmt(stderr, MM_INFO,
			":124:Mail saved in '%s'\n", dotdead);
	}
	(void) umask(omask);

	/*
		Copy letter into dead letter box
	*/
	if (!mcopylet(pmsg, malf, ORDINARY))
		errmsg(E_DEAD,"");
	fclose(malf);
}
