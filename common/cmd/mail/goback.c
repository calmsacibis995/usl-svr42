/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/goback.c	1.11.2.3"
#ident "@(#)goback.c	2.26 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	goback - reply to a letter

    SYNOPSIS
	int goback(Letinfo *pletinfo, int letnum, char *others,
	    int usefrom, int readtty, int sendmsg)

    DESCRIPTION
	This routine handles replying to and passing on a letter "letnum".
	If "usefrom" is set, the UNIX From lines will be used to generate an address.
	In addition, any addresses in "others" will be used. If "sendmsg" is
	set, a copy of the letter will be sent as well.

    RETURNS
	TRUE if the message was successfully delivered
*/

int goback(pletinfo, letnum, others, usefrom, readtty, sendmsg)
Letinfo	*pletinfo;
int	letnum;
char	*others;
int	usefrom;
int	readtty;
int	sendmsg;
{
    static const char pn[] = "goback";
    register int i, w;
    t_Content	binflg;
    int		usercnt = 0;
    string	*buf = s_new();
    char	line[LSIZE];
    string	*work = s_new();
    string	*wuser = s_new();
    const char	*from = header[H_FROM].tag;
    const char	*gtfrom = header[H_FROM1].tag;
    char	fromlen = strlen(from);
    char	gtfromlen = strlen(gtfrom);
    Msg		msg;
    char	datestring[60];		/* Date in mail(1) format */
    char	*p;
    int		err = 0;

    Dout(pn, 0, "Entered, letnum=%d, others='%s', usefrom=%d, readtty=%d, sendmsg=%d\n",
	letnum, others, usefrom, readtty, sendmsg);

    initsurrfile();

    if (usefrom)
	{
	fclose(pletinfo->tmpfile.tmpf);
	pletinfo->tmpfile.tmpf = doopen(pletinfo->tmpfile.lettmp,"r+",E_TMP);
	fseek(pletinfo->tmpfile.tmpf, pletinfo->let[letnum].adr, 0);

	/* Generate the return address. The UNIX From lines are of these forms:
	    "[>]From user date remote from system"
	    "[>]From user date forwarded by otheruser"
	    "[>]From user date"
	   The "forwarded by" form is ignored.
	*/
	for (fgets(line,sizeof(line),pletinfo->tmpfile.tmpf);
	     strncmp(line, from, fromlen) == SAME || strncmp(line, gtfrom, gtfromlen) == SAME;
	     fgets(line,sizeof(line),pletinfo->tmpfile.tmpf))
	    {
	    if ((i = substr(line, "remote from")) != -1)
		{
		const char *cp;
		s_restart(buf);
		for (cp = skipspace(line + i + 11); *cp && *cp != '\n'; cp++)
			s_putc(buf, *cp);

		s_putc(buf, '!');
		s_terminate(buf);
		work = s_append(work, s_to_c(buf));
		w = (line[0] == '>') ? 6 : 5;
		s_restart(wuser);
		while (line[w] != ' ')
			s_putc(wuser, line[w++]);
		s_terminate(wuser);
		}

	    else if ((i = substr(line, "forwarded by")) == -1)
		{
		w = (line[0] == '>') ? 6 : 5;
		s_restart(wuser);
		while (line[w] != ' ')
			s_putc(wuser, line[w++]);
		s_terminate(wuser);
		}
	    }

	work = s_append(work, s_to_c(wuser));
	Dout(pn, 0, "Return address = '%s'\n", s_to_c(work));
	usercnt = 1;
	}

    printf("mail %s %s\n", others, s_to_c(work));
    init_Msg(&msg);
    if (usefrom)
	add_recip(&msg, s_to_c(work), FALSE, (Recip*)0, FALSE, FALSE, 0, 0, 0);
    mktmp(&msg.tmpfile);
    /* From postmaster date ... */
    mkdate(datestring);
    s_restart(buf);
    buf = s_xappend(buf, my_name, " ", datestring, (char*)0);
    save_a_hdr(&msg.hdrinfo, s_to_c(buf), H_FROM, (char*)0);
    s_restart(buf);
    buf = s_xappend(buf, my_name, " ", datestring, " remote from ", remotefrom, (char*)0);
    save_a_hdr(&msg.hdrinfo, s_to_c(buf), H_RFROM, (char*)0);
    if (flgt)
	save_a_hdr(&msg.hdrinfo, s_to_c(work), H_TO, (char*)0);
    s_free(work);
    s_free(wuser);
    s_free(buf);

    for (p = strtok(others, " \t\n"); p; p = strtok((char*)0, " \t\n"))
	{
	if (p[0] == '$')
	    {
	    char *env = getenv(p+1);
	    if (env)
		{
		add_recip(&msg, env, FALSE, (Recip*)0, FALSE, FALSE, 0, 0, 0);
		if (flgt)
		    save_a_hdr(&msg.hdrinfo, p, H_TO, (char*)0);
		usercnt++;
		}

	    else
		{
		pfmt(stderr, MM_ERROR, ":152:%s has no value or is not exported.\n", p+1);
		err++;
		}
	    }

	else if (p[0] == '-')
	    {
	    pfmt(stdout, MM_ERROR, ":406:Only users may be specified, not options\n");
	    err++;
	    }

	else
	    {
	    add_recip(&msg, p, FALSE, (Recip*)0, FALSE, FALSE, 0, 0, 0);
	    if (flgt)
		save_a_hdr(&msg.hdrinfo, p, H_TO, (char*)0);
	    usercnt++;
	    }
	}

    if (usercnt == 0)
	{
	pfmt(stdout, MM_ERROR, ":422:Invalid command: must have a user specified\n");
	err++;
	}

    binflg = pletinfo->let[letnum].binflag;
    if (!err && readtty)
	{
	int n;
	while ((n = getline(line, sizeof line, stdin)) > 0)
	    {
	    if (strcmp(line, ".\n") == SAME)
		break;
	    if (binflg != C_Binary)
		binflg = istext((unsigned char *)line, n, binflg);
	    if (fwrite(line, sizeof(char), n, msg.tmpfile.tmpf) != n)
		{
		errmsg(E_TMP,"");
		err++;
		}
	    }
	}

    if (!err && sendmsg)
	err += !copylet(pletinfo, letnum, msg.tmpfile.tmpf, ORDINARY, 1, 1);

    msg.msgsize = ftell(msg.tmpfile.tmpf);
    msg.binflag = binflg;
    if (!err && msg.msgsize > 0)
	{
	sprintf(line,"%ld", msg.msgsize);
	save_a_hdr(&msg.hdrinfo, line, H_CLEN, (char*)0);
	save_a_hdr(&msg.hdrinfo, (binflg == C_Text)  ? Text :
			       (binflg == C_GText) ? GenericText :
						     Binary, H_CTYPE, (char*)0);
	if (binflg == C_GText)
	    {
	    string *locale = s_copy("euc/locale=");
	    char *ctype_env = getenv("LC_CTYPE");
	    char *lang_env = getenv("LANG");
	    locale = s_append(locale, ctype_env ? ctype_env : lang_env ? lang_env : "unknown");
	    s_terminate(locale);
	    save_a_hdr(&msg.hdrinfo, s_to_c(locale), H_ENCDTYPE, (char*)0);
	    s_free(locale);
	    }
	}

    if (fclose(msg.tmpfile.tmpf) == EOF)
	{
	errmsg(E_TMP,"");
	err++;
	}

    msg.tmpfile.tmpf = doopen(msg.tmpfile.lettmp, "r+", E_TMP);
    if (!msg.tmpfile.tmpf)
	err++;

    msg.orig = s_append(msg.orig, my_name);
    msg.Rpath = s_append(msg.Rpath, my_name);
    if (!err)
	err += !nw_sendlist(&msg);

    fini_Msg(&msg);
    return !err;
}
