/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/doFopt.c	1.11.2.4"
#ident "@(#)doFopt.c	2.20 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	doFopt - Handles installing and removing of forwarding

    SYNOPSIS
	void doFopt()

    DESCRIPTION
	Install or remove forwarding.
*/

void doFopt()
{
    static const char pn[] = "doFopt";
    static char	SP[2] = " ";
    char	*p = (char*)skipspace(flgF), *q;
    char	fwrdbuf[1024];
    string	*hold = s_new();
    string	*fwrdfile = 0;
    struct stat	statb;
    FILE	*malf;

    Dout(pn, 0, "Entered, flgF='%s'\n", flgF);

    if (cksvdir())
	{
	pfmt(stdout, MM_ERROR, ":444:Cannot install forwarding because of failsafe forwarding to %s\n", failsafe);
	s_free(hold);
	s_free(fwrdfile);
	done(0);
	}

    error = 0;
    fwrdfile = s_xappend(fwrdfile, mailfwrd, my_name, (char*)0);

    if (p[0] != '\0')
	{
	Dout(pn, 1, "Adding forwarding\n");
	/*
	    Remove excess blanks/tabs from flgF.
	    Accept comma or white space as delimiter.
	*/
	do  {
	    /* are we looking at a command? */
	    if ((*p == '|') || (strncmp(p, ">|", 2) == SAME))
		{
		pfmt(stdout, MM_INFO, ":507:Installing forwarding to command: %s\n", p);
		hold = s_xappend(hold, SP, p, (char*)0);
		*p = '\0';
		break;
		}

	    /* find the end of the current field */
	    if ((q = strpbrk(p,", \t")) != (char *)NULL)
		*q = '\0';

	    /* remember the field */
	    hold = s_xappend(hold, SP, p, (char*)0);
	    if (islocal(p, (uid_t*)0))
		pfmt(stdout, MM_INFO, ":505:Installing forwarding to local address: %s\n", p);
	    else
		pfmt(stdout, MM_INFO, ":506:Installing forwarding to alias or remote address: %s\n", p);

	    /* find the next field */
	    if (!q)
		break;
	    p = q+1 + strspn(q+1,", \t");
	} while (*p != '\0');

	lock(my_name, 0);
	createmf(my_uid, s_to_c(fwrdfile));
	malf = doopen(s_to_c(fwrdfile), "w",E_FILE);
	pfmt(stdout, MM_INFO, ":44:Forwarding to%s\n", s_to_c(hold));
	if ((fprintf(malf, "Forward to%s\n",s_to_c(hold)) == EOF) ||
	    (fclose(malf) == EOF))
	    {
	    error = E_FILE;
	    errmsg(E_FILE, ":355:Cannot write forwarding file %s\n", s_to_c(fwrdfile));
	    (void) unlink(s_to_c(fwrdfile));
	    s_free(hold);
	    s_free(fwrdfile);
	    done(0);
	    }

	/* Turn on setuid bit on fwrdfile */
	stat(s_to_c(fwrdfile), &statb);
	statb.st_mode |= S_ISUID;
	statb.st_mode &= ~(S_IXUSR|S_IWGRP|S_IWOTH);
	chmod (s_to_c(fwrdfile), statb.st_mode);
	}

    else
	{
	Dout(pn, 1, "Removing forwarding\n");
	if (areforwarding(my_name, fwrdbuf, sizeof(fwrdbuf)))
	    {
	    lock(my_name, 0);
	    malf = doopen(s_to_c(fwrdfile), "w", E_FILE);
	    fclose(malf);
	    pfmt(stdout, MM_INFO, ":45:Forwarding removed\n");
	    }

	else
	    pfmt(stderr, MM_ERROR, ":46:No forwarding to remove\n");

	/* Turn off setuid bit on fwrdfile if set */
	if (stat(s_to_c(fwrdfile), &statb) != -1)
	    {
	    statb.st_mode &= ~S_ISUID;
	    statb.st_mode |= S_IWGRP;
	    if (!delempty((mode_t)statb.st_mode, s_to_c(fwrdfile)))
		chmod(s_to_c(fwrdfile), statb.st_mode);
	    }
	}

    unlock();
    s_free(hold);
    s_free(fwrdfile);
    done(0);
}
