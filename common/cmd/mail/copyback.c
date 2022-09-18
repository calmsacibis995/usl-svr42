/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/copyback.c	1.11.2.3"
#ident "@(#)copyback.c	2.14 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	copyback - copy temp or whatever back to /var/mail

    SYNOPSIS
	void copyback()

    DESCRIPTION
	Copy the reduced contents of lettmp back to
	the mail file. First copy any new mail from
	the mail file to the end of lettmp.
*/

void copyback(pletinfo)
Letinfo	*pletinfo;
{
	static const char pn[] = "copyback";
	string		*savefile = 0;	/* holds filename of save file */
	register int	i, n;
	int		new = 0;
	mode_t		mailmode, omask;
	struct stat	stbuf;
	FILE		*malf;
	void (*hstat)(), (*istat)(), (*qstat)();

	Dout(pn, 0, "Entered\n");
	istat = signal(SIGINT, SIG_IGN);
	qstat = signal(SIGQUIT, SIG_IGN);
	hstat = signal(SIGHUP, SIG_IGN);
	lock(my_name, 0);
	stat(mailfile, &stbuf);
	mailmode = stbuf.st_mode;

	/*
		Has new mail arrived?
	*/
	if (stbuf.st_size != pletinfo->let[pletinfo->nlet].adr) {
		malf = doopen(mailfile, "r",E_FILE);
		fseek(malf, pletinfo->let[pletinfo->nlet].adr, 0);
		fclose(pletinfo->tmpfile.tmpf);
		pletinfo->tmpfile.tmpf = doopen(pletinfo->tmpfile.lettmp,"a",E_TMP);
		/*
			Append new mail assume only one new letter
		*/
		if (!copystream(malf, pletinfo->tmpfile.tmpf)) {
			fclose(malf);
			fclose(pletinfo->tmpfile.tmpf);
			errmsg(E_TMP,"");
			done(0);
		}
		fclose(malf);
		fclose(pletinfo->tmpfile.tmpf);
		pletinfo->tmpfile.tmpf = doopen(pletinfo->tmpfile.lettmp,"r+",E_TMP);
		if (pletinfo->nlet == (MAXLET-2)) {
			errmsg(E_SPACE,"");
			done(0);
		}
		pletinfo->let[++pletinfo->nlet].adr = stbuf.st_size;
		new = 1;
	}

	/*
		Copy mail back to mail file
	*/
	omask = umask(0117);

	/*
		The invoker must own the mailfile being copied to
	*/
	if ((stbuf.st_uid != my_euid) && (stbuf.st_uid != my_uid)) {
		errmsg(E_OWNR,"");
		done(0);
	}

	/* 
		If user specified the '-f' option we dont do 
		the routines to handle :saved files.
		As we would (incorrectly) restore to the user's
		mailfile upon next execution!
	*/
	if (flgf) {
		savefile = s_copy(mailfile);
	} else {
		savefile = s_xappend(savefile, mailsave, my_name, (char*)0);
	}

	if ((malf = fopen(s_to_c(savefile), "w")) == NULL) {
		if (flgf) {
			errmsg(E_FILE,":350:Cannot re-write the alternate file %s: %s\n", s_to_c(savefile), strerror(errno));
		} else {
			errmsg(E_FILE,":349:Cannot open savefile %s: %s\n", s_to_c(savefile), strerror(errno));
		}
		s_free(savefile);
		done(0);
	}

	if ((chown(s_to_c(savefile),mf_uid,mf_gid) == -1) &&
	    (posix_chown(s_to_c(savefile)) == -1))
	{
		errmsg(E_FILE,":351:chown() failed on savefile %s: %s\n", s_to_c(savefile), strerror(errno));
		s_free(savefile);
		done(0);
	}
	(void) umask(omask);
	n = 0;

	for (i = 0; i < pletinfo->nlet; i++) {
		/*
			Note: any action other than an undelete, or a 
			plain read causes the letter acted upon to be 
			deleted
		*/
		if (pletinfo->let[i].change == ' ') {
			if (copylet(pletinfo, i, malf, ORDINARY, 1, 1) == FALSE) {
				errmsg(E_FILE,":352:Cannot copy mail to savefile %s: %s\n", s_to_c(savefile), strerror(errno));
				pfmt(stderr, MM_INFO, 
					":25:A copy of your mailfile is in '%s'\n",
					pletinfo->tmpfile.lettmp);
				s_free(savefile);
				done(1);	/* keep temp file */
			}	
			n++;
		}
	}
	fclose(malf);

	if (!flgf) {
		if (unlink(mailfile) != 0) {
			errmsg(E_FILE,":353:Cannot unlink mailfile: %s\n", strerror(errno));
			s_free(savefile);
			done(0);
		}
		chmod(s_to_c(savefile),mailmode);
		if (rename(s_to_c(savefile), mailfile) != 0) {
			errmsg(E_FILE,":354:Cannot rename savefile to mailfile: %s\n", strerror(errno));
			s_free(savefile);
			done(0);
		}
	}

	/*
		Empty mailbox?
	*/
	if (n == 0) {
		delempty((mode_t)stbuf.st_mode, mailfile);
	}

	if (new && !flgf) {
		pfmt(stdout, MM_NOSTD, ":30:New mail arrived\n");
	}

	unlock();
	(void) signal(SIGINT, istat);
	(void) signal(SIGQUIT, qstat);
	(void) signal(SIGHUP, hstat);
	s_free(savefile);
}
