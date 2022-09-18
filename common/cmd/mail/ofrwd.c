/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/ofrwd.c	1.3.2.2"
#ident "@(#)ofrwd.c	1.6 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	oldforwarding - check for old style forwarding information

    SYNOPSIS
	void oldforwarding(char *user)

    DESCRIPTION
	Check /var/mail/user for "Forward to". If it exists there
	and /var/mail/:forward/user doesn't exist, move the file.
	If both exist, then rewrite /var/mail/user with the "Forward to"
	moved into a message.

	This check is done when delivering locally and when printing mail.
	It is assumed that the mailbox has been locked before entry.
*/

void oldforwarding(user)
char *user;
{
    FILE *fp;
    char buf[1024];
    string *usermail = 0;
    string *userfwrd = 0;
    struct stat statbuf;

    usermail = s_xappend(usermail, maildir, user, (char*)0);

    if (!(fp = fopen(s_to_c(usermail), "r")))
	{
	s_free(usermail);
	return;
	}

    /* read line from /usr/mail */
    if (!fgets(buf, sizeof(buf), fp))
	{
	(void) fclose(fp);
	s_free(usermail);
	return;
	}

    /* Does "Forward to " exist? */
    if (strncmp(buf, "Forward to ", 11) != 0)
	{
	(void) fclose(fp);
	s_free(usermail);
	return;
	}

    /* if :forward exists then rewrite mailbox as letter */
    userfwrd = s_xappend(userfwrd, mailfwrd, user, (char*)0);
    if ((stat(s_to_c(userfwrd), &statbuf) != -1) && (statbuf.st_size > 0))
	{
	string	*usersave = 0;
	mode_t	omask = umask(0117);
	FILE	*sfp;
	char	datestring[60];		/* Date in mail(1) format */

	usersave = s_xappend(usersave, mailsave, user, (char*)0);
	if ((sfp = fopen(s_to_c(usersave), "w")) == NULL)
	    {
	    /* no recovery possible */
	    fclose(fp);
	    s_free(usermail);
	    s_free(userfwrd);
	    s_free(usersave);
	    (void) umask(omask);
	    return;
	    }

	rewind(fp);
	mkdate(datestring);
	(void) fprintf(sfp, "From postmaster %s\n\n", datestring);
	(void) fprintf(sfp, "The following invalid forwarding information was found\n");
	(void) fprintf(sfp, "in your mailbox. Use 'mail -F \"names\"' to install forwarding.\n\n\t");
	if (!copystream(fp, sfp))
	    {
	    /* no recovery possible */
	    fclose(fp);
	    fclose(sfp);
	    (void) unlink(s_to_c(usersave));
	    s_free(usermail);
	    s_free(userfwrd);
	    s_free(usersave);
	    (void) umask(omask);
	    return;
	    }

	if (fclose(sfp) == EOF)
	    {
	    /* no recovery possible */
	    fclose(fp);
	    (void) unlink(s_to_c(usersave));
	    s_free(usermail);
	    s_free(userfwrd);
	    s_free(usersave);
	    (void) umask(omask);
	    return;
	    }

	fclose(fp);
	if (unlink(mailfile) != 0)
	    {
	    chmod(s_to_c(usersave), MFMODE | S_ISUID);
	    /* no recovery possible if this fails */
	    (void) rename(mailsave, mailfile);
	    }
	s_free(usersave);
	(void) umask(omask);
	}

    /* else rename to :forward */
    else
	{
	/* If the old forward file was installed by mail, then make */
	/* the new forward file is installed properly. */
	if ((stat(s_to_c(usermail), &statbuf) != -1) &&
	    ((statbuf.st_mode & S_ISGID) == S_ISGID) &&
	    (statbuf.st_gid == my_egid))
	    chmod(s_to_c(usermail), MFMODE | S_ISUID);
	    
	/* no recovery possible if this fails */
	(void) rename(s_to_c(usermail), s_to_c(userfwrd));
	}

    s_free(usermail);
    s_free(userfwrd);
    return;
}
