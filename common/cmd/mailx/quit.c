/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mailx:quit.c	1.10.2.7"
#ident "@(#)quit.c	1.16 'attmail mail(1) command'"
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include "rcv.h"

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Rcv -- receive mail rationally.
 *
 * Termination processing.
 */

static void		writeback ARGS((void));

/*
 * Save all of the undetermined messages at the top of "mbox"
 * Save all untouched messages back in the system mailbox.
 * Remove the system mailbox, if none saved there.
 */

void
quit()
{
	int mcount, p, modify, autohold, anystat, holdbit, nohold;
	FILE *ibuf, *obuf, *fbuf, *readstat;
	register struct message *mp;
	register int c;
	char *id;
	int appending;
	char *mbox = Getf("MBOX");

	/*
	 * If we are read only, we can't do anything,
	 * so just return quickly.
	 */

	mcount = 0;
	if (readonly)
		return;
	/*
	 * See if there any messages to save in mbox.  If no, we
	 * can save copying mbox to /tmp and back.
	 *
	 * Check also to see if any files need to be preserved.
	 * Delete all untouched messages to keep them out of mbox.
	 * If all the messages are to be preserved, just exit with
	 * a message.
	 *
	 * If the luser has sent mail to himself, refuse to do
	 * anything with the mailbox, unless mail locking works.
	 */

#ifndef CANLOCK
	if (selfsent) {
		pfmt(stdout, MM_NOSTD, ":324:You have new mail.\n");
		return;
	}
#endif

	/*
	 * Adjust the message flags in each message.
	 */

	anystat = 0;
	autohold = value("hold") != NOSTR;
	appending = value("append") != NOSTR;
	holdbit = autohold ? MPRESERVE : MBOX;
	nohold = MBOX|MSAVED|MDELETED|MPRESERVE;
	if (value("keepsave") != NOSTR)
		nohold &= ~MSAVED;
	for (mp = &message[0]; mp < &message[msgCount]; mp++) {
		if (mp->m_flag & MNEW) {
			receipt(mp);
			mp->m_flag &= ~MNEW;
			mp->m_flag |= MSTATUS;
		}
		if (mp->m_flag & MSTATUS)
			anystat++;
		if ((mp->m_flag & MTOUCH) == 0)
			mp->m_flag |= MPRESERVE;
		if ((mp->m_flag & nohold) == 0)
			mp->m_flag |= holdbit;
	}
	modify = 0;
	if (Tflag != NOSTR) {
		if ((readstat = fopen(Tflag, "w")) == NULL)
			Tflag = NOSTR;
	}
	for (c = 0, p = 0, mp = &message[0]; mp < &message[msgCount]; mp++) {
		if (mp->m_flag & MBOX)
			c++;
		if (mp->m_flag & MPRESERVE)
			p++;
		if (mp->m_flag & MODIFY)
			modify++;
		if (Tflag != NOSTR && (mp->m_flag & (MREAD|MDELETED)) != 0) {
			id = hfield("message-id", mp, addone);
			if (id != NOSTR)
				fprintf(readstat, "%s\n", id);
			else {
				id = hfield("article-id", mp, addone);
				if (id != NOSTR)
					fprintf(readstat, "%s\n", id);
			}
		}
	}
	if (Tflag != NOSTR)
		fclose(readstat);
	if (p == msgCount && !modify && !anystat) {
		if (p == 1)
			pfmt(stdout, MM_NOSTD, heldonemsg, mailname);
		else
			pfmt(stdout, MM_NOSTD, heldmsgs, p, mailname);
		return;
	}
	if (c == 0) {
		writeback();
		return;
	}

	/*
	 * Create another temporary file and copy user's mbox file
	 * therein.  If there is no mbox, copy nothing.
	 * If s/he has specified "append" don't copy the mailbox,
	 * just copy saveable entries at the end.
	 */

	mcount = c;
	if (!appending) {
		if ((obuf = fopen(tempQuit, "w")) == NULL) {
			pfmt(stderr, MM_ERROR, badopen,
				tempQuit, strerror(errno));
			return;
		}
		if ((ibuf = fopen(tempQuit, "r")) == NULL) {
			pfmt(stderr, MM_ERROR, badopen,
				tempQuit, strerror(errno));
			removefile(tempQuit);
			fclose(obuf);
			return;
		}
		removefile(tempQuit);
		if ((fbuf = fopen(mbox, "r")) != NULL) {
			copystream(fbuf, obuf);
			fclose(fbuf);
		}
		if (ferror(obuf)) {
			pfmt(stderr, MM_ERROR, badwrite,
				tempQuit, strerror(errno));
			fclose(ibuf);
			fclose(obuf);
			return;
		}
		fclose(obuf);
		close(creat(mbox, MBOXPERM));
		if ((obuf = fopen(mbox, "w")) == NULL) {
			pfmt(stderr, MM_ERROR, badopen,
				mbox, strerror(errno));
			fclose(ibuf);
			return;
		}
	} else		/* we are appending */
		if ((obuf = fopen(mbox, "a")) == NULL) {
			pfmt(stderr, MM_ERROR, badopen,
				mbox, strerror(errno));
			return;
		}
	for (mp = &message[0]; mp < &message[msgCount]; mp++)
		if (mp->m_flag & MBOX)
			if (send(mp, obuf, 0, 0, 1) < 0) {
				pfmt(stderr, MM_ERROR, badwrite, 
					mbox, strerror(errno));
				if (!appending)
					fclose(ibuf);
				fclose(obuf);
				return;
			}

	/*
	 * Copy the user's old mbox contents back
	 * to the end of the stuff we just saved.
	 * If we are appending, this is unnecessary.
	 */

	if (!appending) {
		rewind(ibuf);
		copystream(ibuf, obuf);
		fclose(ibuf);
		fflush(obuf);
	}
	if (ferror(obuf)) {
		pfmt(stderr, MM_ERROR, badwrite, mbox, strerror(errno));
		fclose(obuf);
		return;
	}
	fclose(obuf);
	if (mcount == 1)
		pfmt(stdout, MM_NOSTD, ":325:Saved 1 message in %s\n",
			mbox);
	else
		pfmt(stdout, MM_NOSTD, ":326:Saved %d messages in %s\n",
			mcount, mbox);

	/*
	 * Now we are ready to copy back preserved files to
	 * the system mailbox, if any were requested.
	 */
	writeback();
}

/*
 * Preserve all the appropriate messages back in the system
 * mailbox, and print a nice message indicated how many were
 * saved.  Incorporate any new mail that we found.
 */
static void
writeback()
{
	register struct message *mp;
	register int p, success = 0;
	struct stat st;
	FILE *obuf = 0, *fbuf = 0, *rbuf = 0;
	void (*fhup)(), (*fint)(), (*fquit)(), (*fpipe)();

	fhup = sigset(SIGHUP, SIG_IGN);
	fint = sigset(SIGINT, SIG_IGN);
	fquit = sigset(SIGQUIT, SIG_IGN);
	fpipe = sigset(SIGPIPE, SIG_IGN);

	lockmail();
	(void) PRIV(fbuf = fopen(mailname, "r+"));
	if (fbuf == NULL) {
		pfmt(stderr, MM_ERROR, badopen, mailname, strerror(errno));
		goto die;
	}
	/* lock(fbuf, "r+", 1); */
	fstat(fileno(fbuf), &st);
	if (st.st_size > mailsize) {
		pfmt(stdout, MM_NOSTD, newmailarrived);
		sprintf(tempResid, "%s/:saved/%s", maildir, myname);
		(void) PRIV(rbuf = fopen(tempResid, "w+"));
		if (rbuf == NULL) {
			pfmt(stderr, MM_ERROR, badopen,
				tempResid, strerror(errno));
			goto die;
		}
#ifdef APPEND
		fseek(fbuf, mailsize, 0);
		copystream(fbuf, rbuf);
#else
		p = st.st_size - mailsize;
		while (p-- > 0) {
			register int c = getc(fbuf);
			if (c == EOF) {
				pfmt(stderr, MM_ERROR, badread,
					mailname, strerror(errno));
				goto die;
			}
			putc(c, rbuf);
		}
#endif
		fseek(rbuf, 0L, 0);
	}

	(void) PRIV(obuf = fopen(mailname, "w"));
	if (obuf == NULL) {
		pfmt(stderr, MM_ERROR, badopen,
			mailname, strerror(errno));
		goto die;
	}
#ifndef APPEND
	if (rbuf != NULL)
		copystream(rbuf, obuf);
#endif
	p = 0;
	for (mp = &message[0]; mp < &message[msgCount]; mp++)
		if ((mp->m_flag&MPRESERVE)||(mp->m_flag&MTOUCH)==0) {
			p++;
			if (send(mp, obuf, 0, 0, 1) < 0) {
				pfmt(stderr, MM_ERROR, badwrite, 
					mailname, strerror(errno));
				goto die;
			}
		}
#ifdef APPEND
	if (rbuf != NULL)
		copystream(rbuf, obuf);
#endif
	fflush(obuf);
	if (ferror(obuf)) {
		pfmt(stderr, MM_ERROR, badwrite,
			mailname, strerror(errno));
		goto die;
	}
	alter(mailname);
	if (p == 1)
		pfmt(stdout, MM_NOSTD, heldonemsg, mailname);
	else if (p > 1)
		pfmt(stdout, MM_NOSTD, heldmsgs, p, mailname);
	success = 1;

die:
	if (success && obuf && (fsize(obuf) == 0) &&
	    (value("keep") == NOSTR)) {
		struct stat	statb;
		if (stat(mailname, &statb) >= 0)
			(void) PRIV(delempty(statb.st_mode, mailname));
	}
	if (rbuf) {
		fclose(rbuf);
		(void) PRIV(removefile(tempResid));
	}
	if (obuf)	
		fclose(obuf);
	if (fbuf)
		fclose(fbuf);
	unlockmail();
	sigset(SIGHUP, fhup);
	sigset(SIGINT, fint);
	sigset(SIGQUIT, fquit);
	sigset(SIGPIPE, fpipe);
}

void
lockmail()
{
    (void) PRIV(maillock(lockname,10));
}

void
unlockmail()
{
    (void) PRIV(mailunlock());
}
