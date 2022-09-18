/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/cksaved.c	1.7.2.2"
#ident "@(#)cksaved.c	2.9 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	cksaved - check for an orphaned save file

    SYNOPSIS
	void cksaved(char *user)

    DESCRIPTION
	cksaved() looks to see if there is a saved-mail file sitting
	around which should be reinstated. These files should be sitting
	around only in the case of a crash during rewriting a mail message.

	The strategy is simple: if the file exists it is appended to
	the end of $MAIL.  It is better that a user potentially sees the
	mail twice than to lose it.

	If $MAIL doesn't exist, then a simple rename() will suffice.
*/

void cksaved(user)
char	*user;
{
	struct stat stbuf;
	string *command = s_new();
	string *save = s_copy(mailsave), *mail = s_copy(maildir);

	mail = s_append(mail, user);
	save = s_append(save, user);

	/*
		If no save file, or size is 0, return.
	*/
	if ((stat(s_to_c(save),&stbuf) != 0) || (stbuf.st_size == 0)) {
		s_free(mail);
		s_free(save);
		return;
	}

	/*
		Ok, we have a savefile. If no mailfile exists,
		then we want to restore to the mailfile,
		else we append to the mailfile.
	*/
	lock(user, 0);
	if (stat(s_to_c(mail),&stbuf) != 0) {
		/*
			Restore from the save file by linking
			it to $MAIL then unlinking save file
		*/
		chmod(s_to_c(save), MFMODE);
		if (rename(s_to_c(save), s_to_c(mail)) != 0) {
			unlock();
			pfmt(stderr, MM_ERROR,
				":17:Cannot rename saved file: %s\n", strerror(errno));
			s_free(mail);
			s_free(save);
			s_free(command);
			return;
		}

		command = s_xappend(command, "echo \"Your mailfile was just restored by the mail program.\nPermissions of your mailfile are set to 0660.\"| mail ", user, (char*)0);
	}

	else {
		FILE *Istream, *Ostream;
		if ((Ostream = fopen(s_to_c(mail),"a")) == NULL) {
			pfmt(stderr, MM_ERROR, ":2:Cannot open %s: %s\n",
				s_to_c(mail), strerror(errno));
			unlock();
			s_free(mail);
			s_free(save);
			s_free(command);
			return;
		}
		if ((Istream = fopen(s_to_c(save),"r")) == NULL) {
			pfmt(stderr, MM_ERROR, ":2:Cannot open %s: %s\n",
				s_to_c(save), strerror(errno));
			fclose(Ostream);
			unlock();
			s_free(mail);
			s_free(save);
			s_free(command);
			return;
		}
		if (!copystream(Istream, Ostream)) {
			pfmt(stderr, MM_ERROR, ":464:Copy of save file failed: %s\n",
				strerror(errno));
			s_free(mail);
			s_free(save);
			s_free(command);
			return;
		}
		fclose(Istream);
		fclose(Ostream);

		if (unlink(s_to_c(save)) != 0) {
			pfmt(stderr, MM_ERROR, ":19:Unlink of save file failed: %s\n",
				strerror(errno));
			s_free(mail);
			s_free(save);
			s_free(command);
			return;
		}

		command = s_xappend(command, "echo \"Your mail save file has just been appended to your mail box by the mail program.\" | mail ", user, (char*)0);
	}

	/*
		Try to send mail to the user whose file
		is being restored.
	*/
	unlock();
	systm(s_to_c(command));
	s_free(mail);
	s_free(save);
	s_free(command);
}
