/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/sendmail.c	1.14.3.6"
#ident "@(#)sendmail.c	2.38 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	sendmail - High level sending routine

    SYNOPSIS
	sendmail(int argc, char **argv)

    DESCRIPTION
	Generate a list of recipients based on the argument list and send
	mail to the list. Argc and argv point to the list of names AFTER
	all options have been removed.

	Note that no message is sent if there is absolutely no input.
	If there is ANY input, even a blank line, then a message will be
	sent.
*/

void sendmail(argc, argv)
char **argv;
{
    static const char pn[] = "sendmail";
    char	line[LSIZE];	/* holds a line of a letter */
    char	*tp;
    char	last1c;
    string	*tmpstr = s_new();
    int		i, n;
    int		ttyf = FALSE;
    int		hdrtyp = 0;
    int		ctf = FALSE;
    Hdrs	*hptr;
    Msg		msg;
    int		msgseen;
    string	*fromS = s_new(), *fromU = s_new();
    char	datestring[60];		/* Date in mail(1) format */
    char	RFC822datestring[60];	/* Date in RFC822 format */

    Dout(pn, 0, "entered\n");
    initsurrfile();
    init_Msg(&msg);
    saveint = setsig(SIGINT, savdead);
    mktmp(&msg.tmpfile);
    /* Format time */
    mkdate(datestring);
    /* asctime: Fri Sep 30 00:00:00 1986\n */
    /*          0123456789012345678901234  */
    /* date:    Fri Sep 30 00:00 EDT 1986  */
    /*          0123456789012345678901234  */
    /* RFCtime: Fri, 28 Jul 89 10:30 EDT   */
    tp = datestring;
    sprintf(RFC822datestring, "%.3s, %.2s %.3s %.4s %.5s %.3s",
	/* Fri */	tp,
	/* 28  */	tp+8,
	/* Jul */	tp+4,
	/* 89  */	tp+23,
	/* 10:30 */	tp+11,
	/* EDT */	tp+17);

    /* Write out the from line header for the letter */
    tmpstr = s_xappend(tmpstr, my_name, " ", datestring, (char*)0);
    save_a_hdr(&msg.hdrinfo, s_to_c(tmpstr), H_FROM, (char*)0);
    tmpstr = s_xappend(tmpstr, " remote from ", remotefrom, (char*)0);
    save_a_hdr(&msg.hdrinfo, s_to_c(tmpstr), H_RFROM, (char*)0);
    s_reset(tmpstr);

    /*
     * Read mail message, allowing for lines of infinite
     * length. This is tricky: have to watch for newlines.
     */
    ttyf = isatty(fileno(stdin));
    fromU = s_append(fromU, my_name);

    /* If debugging, forget the message */
    /* and pretend a message has been seen. */
    if (flgT || flglb)
	{
	msgseen = 1;
	n = 0;
	line[0] = '\0';
	}

    /* scan header & save relevant info. */
    else
	{
	int	pushrest = 0;
	msgseen = 0;	/* reset when first read of message body succeeds */
	last1c = ' ';	/* anything other than newline */
	msg.hdrinfo.fnuhdrtype = 0;
	while ((n = getline (line, sizeof line, stdin)) > 0)
	    {
	    last1c = line[n-1];
	    if (pushrest)
		{
		/* This throws away the ends of LONG LONG headers. It would be better */
		/* if we could append to the previously created header field. */
		pushrest = (last1c != '\n');
		continue;
		}
	    pushrest = (last1c != '\n');

	    if ((hdrtyp = isheader (line, &ctf, 1, msg.hdrinfo.fnuhdrtype)) == FALSE)
		break;

	    msgseen = 1;
	    switch (hdrtyp)
		{
		case H_RVERS:
		    /* Are we dealing with a delivery report? */
		    /* ret_on_error = 0 ==> do not return on failure */
		    msg.ret_on_error = 0;
		    Dout(pn, 0, "Report-Version Header seen: ret_on_error = 0\n");
		    break;

		case H_FROM:
		    /* Convert From headers into >From headers */
		    hdrtyp = H_FROM1;
		    /* FALLTHROUGH */

		case H_FROM1:
		    if (substr(line, "forwarded by") > -1)
			break;
		    /* If first ">From " line, check for '...remote from...' */
		    /* and put its information into a Received: header. */
		    if (msg.hdrinfo.hdrs[H_FROM1] == (Hdrs*)NULL)
			{
			int rf = substr(line, " remote from ");
			if (rf >= 0)
			    {
			    register char *s = line+rf+13;
			    tmpstr = s_append(tmpstr, "from ");
			    for ( ; *s && *s != '\n' && *s != '\r'; s++)
				s_putc(tmpstr, *s);
			    tmpstr = s_xappend(tmpstr, " by ", thissys,
				maildomain(), "; ", RFC822datestring, (char*)0);
			    }
			}

		    pickFrom(line, &fromU, &fromS);
		    if (s_to_c(msg.Rpath)[0] != '\0')
			msg.Rpath = s_append(msg.Rpath, "!");
		    msg.Rpath = s_append(msg.Rpath, s_to_c(fromS));
		    break;

		case H_MTYPE:
		    if (flgm)
			{
			/* suppress if message-type argument */
			continue;
			}
		    break;

		case H_CONT:
		    trimnl(line);
		    save_cont_hdr(&msg.hdrinfo, line);
		    continue;
		}

	    save_a_txthdr(&msg.hdrinfo, line, hdrtyp);
	    }
	}

    if (s_to_c(tmpstr)[0])
	save_mta_hdr(&msg.hdrinfo, s_to_c(tmpstr), H_RECEIVED, (char*)0);

    for (i = 1; i < argc; ++i)
	{
	if (argv[i][0] == '-')
	    {
	    if (argv[i][1] == '\0')
		errmsg(E_SYNTAX,":410:Hyphens MAY NOT be followed by spaces\n");
	    else
		errmsg(E_SYNTAX,":411:Options MUST PRECEDE persons\n");
	    done(0);
	    }

	/* Ensure no NULL names in list */
	if (argv[i][0] == '\0' || argv[i][strlen(argv[i])-1] == '!')
	    {
	    errmsg(E_SYNTAX,":412:Null names are not allowed\n");
	    done(0);
	    }

	/* Copy to list in mail entry? */
	if (flgt)
	    save_mta_hdr(&msg.hdrinfo, argv[i], H_TO, (char*)0);

	/* Add the recipient, but don't check for duplication. */
	add_recip(&msg, argv[i], FALSE, (Recip*)0, FALSE, TRUE, 0, 0, 0);
	}

    /* Force a From: and Date: header, if desired, on LOCAL MAIL only. */
    if (msg.hdrinfo.hdrs[H_FROM1] == 0)
	{
	if (msg.hdrinfo.hdrs[H_FROM2] == 0 && mgetenv("ADD_FROM"))
	    {
	    s_restart(tmpstr);
	    tmpstr = s_xappend(tmpstr, my_name, "@", thissys, maildomain(), (char*)0);
	    save_mta_hdr(&msg.hdrinfo, s_to_c(tmpstr), H_FROM2, (char*)0);
	    }
	if (msg.hdrinfo.hdrs[H_DATE] == 0 && mgetenv("ADD_DATE"))
	    save_mta_hdr(&msg.hdrinfo, RFC822datestring, H_DATE, (char*)0);
	}

    if (msg.hdrinfo.hdrs[H_RECEIVED] == 0 && mgetenv("ADD_RECEIVED"))
	{
	s_restart(tmpstr);
	tmpstr = s_xappend(tmpstr, "from ", my_name, " by ", mailsystem(1),
	    maildomain(), "; ", RFC822datestring, (char*)0);
	save_mta_hdr(&msg.hdrinfo, s_to_c(tmpstr), H_RECEIVED, (char*)0);
	}

    /* determine the return path */
    if (s_to_c(msg.Rpath)[0] != '\0')
	msg.Rpath = s_append(msg.Rpath, "!");
    msg.Rpath = s_append(msg.Rpath, s_to_c(fromU));
    msg.orig = s_append(msg.orig, s_to_c(msg.Rpath));

    /* Decide if this message came from someone who */
    /* shouldn't see errors. */
    if ((cascmp(s_to_c(fromU), "postmaster") == 0) ||
	(cascmp(s_to_c(fromU), "mailer-daemon") == 0) ||
	(cascmp(s_to_c(fromU), "mailer-demon") == 0) ||
	(cascmp(s_to_c(fromU), "uucp") == 0) ||
	(cascmp(s_to_c(fromU), "mmdf") == 0))
	msg.ret_on_error = 0;

    /* push out message type if so requested */
    if (flgm)	/* message-type */
	{
	trimnl(flgm);
	save_mta_hdr(&msg.hdrinfo, flgm, H_MTYPE, (char*)0);
	}

    if (n == 0 || (ttyf && (strncmp(line, ".\n", 2) == SAME)) )
	{
	if (!msgseen)
	    {
	    /* no input whatsoever */
	    goto cleanup;
	    }
	else
	    {
	    /*
	     * No body: put content-type and -length only if
	     * explicitly present. (see below....)
	     */
	    goto wrapsend;
	    }
	}

    if (n == 1 && last1c == '\n')
	{	/* blank line -- suppress */
	n = getline(line, sizeof line, stdin);
	if (n == 0 || (ttyf && (strncmp(line, ".\n", 2) == SAME)) )
	    {
	    /*
	     * no content: put content-type and -length only if
	     * explicitly present. (see below....)
	     */
	    goto wrapsend;
	    }
	}

    if (debug > 0)
	{
	Dout(pn, 0, "header scan complete, readahead %d = \"%*s\"\n", n, n, line);
	Dout(pn, 0, "beginning body processing\n");
	}

    /*
     *	Are we returning mail from a delivery failure of an old-style
     *	(SVR3.1 or SVR3.0) rmail? If so, we won't return THIS on failure
     *	[This line should occur as the FIRST non-blank non-header line]
     */
    if (strncmp("***** UNDELIVERABLE MAIL sent to",line,32) == SAME)
	{
	msg.ret_on_error = 0; /* 0 says do not return on failure */
	Dout(pn, 0, "found old-style UNDELIVERABLE line. ret_on_error => 0\n");
	}

    /* scan body of message */
    while (n > 0)
	{
	if (ttyf && (strcmp(line, ".\n") == SAME))
	    break;
	if (msg.binflag != C_Binary)
	    msg.binflag = istext((unsigned char *)line, n, msg.binflag);

	if (fwrite(line, sizeof(char), n, msg.tmpfile.tmpf) != n)
	    {
	    errmsg(E_TMP,"");
	    done(0);
	    }
	msg.msgsize += n;
	n = ttyf
	    ? getline (line, sizeof line, stdin)
	    : fread (line, 1, sizeof line, stdin);
	}

wrapsend:
    /* Polish it off. */
    Dout(pn, 0, "body copy complete, msg size=%ld\n", msg.msgsize);

    /* Modify or create value of H_CTYPE. */
    if ((hptr = msg.hdrinfo.hdrs[H_CTYPE]) != (Hdrs*)NULL)
	{
	/* If there is nothing associated with the Content-Type header already, */
	/* force it to an appropriate value. */
	if (strlen(hptr->value) == 0)
	    {
	    free(hptr->value);
	    hptr->value = strdup((msg.binflag == C_Text)  ? Text :
				 (msg.binflag == C_GText) ? GenericText :
							    Binary);
	    if (!hptr->value)
		{
		errmsg(E_MEM,":407:malloc failed in %s(): %s\n", pn, strerror(errno));
		done(1);
		}
	    }

	/* We have a Content-Type header already, but it may be wrong. */
	/* If it's already anything OTHER than 'text', don't change it. */
	else if ((msg.binflag != C_Text) && (strcmp(hptr->value, Text) == SAME))
	    {
	    free(hptr->value);
	    hptr->value = strdup((msg.binflag == C_GText) ? GenericText : Binary);
	    if (!hptr->value)
		{
		errmsg(E_MEM,":407:malloc failed in %s(): %s\n", pn, strerror(errno));
		done(1);
		}
	    }
	}

    /* Create the Content-Type header with an appropriate value. */
    else if (msg.msgsize > 0)
	save_mta_hdr(&msg.hdrinfo, (msg.binflag == C_Text)  ? Text :
			       (msg.binflag == C_GText) ? GenericText :
							  Binary, H_CTYPE, (char*)0);

    /* Set 'place-holder' value of content length to true value */
    if ((hptr = msg.hdrinfo.hdrs[H_CLEN]) != (Hdrs*)NULL)
	{
	char buf[30];
	sprintf(buf,"%ld", msg.msgsize);
	free(hptr->value);
	hptr->value = strdup(buf);
	if (!hptr->value)
	    {
	    errmsg(E_MEM,":407:malloc failed in %s(): %s\n", pn, strerror(errno));
	    done(1);
	    }
	}
    else if (msg.msgsize > 0)
	{
	char buf[30];
	sprintf(buf,"%ld", msg.msgsize);
	save_mta_hdr(&msg.hdrinfo, buf, H_CLEN, (char*)0);
	}

    /* If Content-Type == Generic-Text, add locale header */
    if ((msg.binflag == C_GText) && (msg.hdrinfo.hdrs[H_ENCDTYPE] != (Hdrs*)NULL))
	{
	string *locale = s_copy("euc/locale=");
	char *ctype_env = getenv("LC_CTYPE");
	char *lang_env = getenv("LANG");
	locale = s_append(locale, ctype_env ? ctype_env : lang_env ? lang_env : "unknown");
	s_terminate(locale);
	save_mta_hdr(&msg.hdrinfo, s_to_c(locale), H_ENCDTYPE, (char*)0);
	s_free(locale);
	}

    if (fclose(msg.tmpfile.tmpf) == EOF)
	{
	errmsg(E_TMP,"");
	done(0);
	}

    msg.tmpfile.tmpf = doopen(msg.tmpfile.lettmp, "r+", E_TMP);

    (void) nw_sendlist(&msg);
cleanup:
    fini_Msg(&msg);
    s_free(fromS);
    s_free(fromU);
    (void) setsig(SIGINT, saveint);
    done(0);
    /* NOTREACHED */
}
