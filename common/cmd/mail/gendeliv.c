/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/gendeliv.c	1.14.2.6"
#ident "@(#)gendeliv.c	2.38 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	gendeliv - generate a Delivery report

    SYNOPSIS
	void gendeliv(Msg *parent, int wherefrom)

    DESCRIPTION
	Generate either a Delivery report for a given
	message and list of recipients.
*/

static void saveoutput ARGS((FILE *ofp, Msg *pmsg, char *oname));
static char *stripfmt ARGS((const char*));
static string *extract_to_name ARGS((Msg *parent, int hdr));

void gendeliv(parent, wherefrom)
Msg *parent;
int wherefrom;
{
    static const char pn[] = "gendeliv";
    Msg		msg;

    Dout(pn, 0, "entered\n");

    /* check Delivery-Options: */
    if (ckdlivopts(parent) & NODELIVERY)
	return;

    init_Msg(&msg);
    init_retmail(parent, &msg, 0);
    add_retmail(&msg, wherefrom, 0);
    send_retmail(&msg);
    fini_Msg(&msg);
}

void init_retmail(parent, pmsg, rc)
Msg *parent;
Msg *pmsg;
int rc;
{
    static const char pn[] = "init_retmail";
    string	*tmpstr = 0;
    char	buf[1024], buf2[1024];
    long	ltmp;
    char	*p;
    string	*toname;
    Hdrs	*hptr;
    char	datestring[60];

    Dout(pn, 0, "entered\n");
    pmsg->parent = parent;
    /* Determine the address to send the ack/nak to. */
    toname = extract_to_name(parent, rc == 0);
    add_recip(pmsg, s_to_c(toname), FALSE, (Recip*)0, FALSE, FALSE, 0, 0, 0);

    mktmp(&pmsg->tmpfile);

    /* From postmaster date ... */
    mkdate(datestring);
    tmpstr = s_xappend(tmpstr, "postmaster ", datestring, (char*)0);
    save_a_hdr(&pmsg->hdrinfo, s_to_c(tmpstr), H_FROM, (char*)0);
    tmpstr = s_xappend(tmpstr, " remote from ", remotefrom, (char*)0);
    save_a_hdr(&pmsg->hdrinfo, s_to_c(tmpstr), H_RFROM, (char*)0);

    /* Report-Version: */
    save_a_hdr(&pmsg->hdrinfo, "2", H_RVERS, (char*)0);

    /* >To: */
    save_a_hdr(&pmsg->hdrinfo, s_to_c(toname), H_TCOPY, (char*)0);

    /* To: ? */
    save_a_hdr(&pmsg->hdrinfo, s_to_c(toname), H_TO, (char*)0);

    /* Date: */
    /* strip year out of date string, insert 'GMT', and put year back... */
    time(&ltmp);
    strcpy(buf, asctime(gmtime(&ltmp)));
    p = strrchr(buf,' ');
    strcpy(buf2,++p);
    *p = '\0';
    strcat(buf,"GMT ");
    strcat(buf, buf2);
    trimnl(buf);
    save_a_hdr(&pmsg->hdrinfo, buf, H_DATE, (char*)0);

    if ((hptr = parent->hdrinfo.hdrs[H_DATE]) != (Hdrs*)NULL)
	save_a_hdr(&pmsg->hdrinfo, hptr->value, H_ODATE, (char*)0);

    else
	{
	/* If no H_DATE line in original message, use date */
	/* in last UNIX H_FROM1 or H_FROM line */
	if ((hptr = getlasthdr(parent->hdrinfo.hdrs[H_FROM1])) == (Hdrs *)NULL)
	    hptr = parent->hdrinfo.hdrs[H_FROM];

	Dout(pn, 0,"date from H_FROM = '%s'\n", hptr->value);
	strncpy(buf, hptr->value, sizeof(buf));

	/* Find date portion of line. */
	/* Assumes line is of form - */
	/*       'name date [remote from sys|forwarded by name]' */
	if ((p = strchr(buf,' ')) == (char *)NULL)
	    strcpy(buf, "No valid datestamp in original.");

	else
	    {
	    register int i = 0;
	    strmove(buf,p);
	    /* Walk backwards from end of string to 3rd blank, */
	    /* and then check for 'remote from' or 'forwarded by' */
	    /* If either found, truncate there, else use entire */
	    /* string. */
	    for (p = buf + strlen(buf) - 1; p > buf; p--)
		if ((*p == ' ') && (++i == 3))
			break;

	    if ((i != 3) || (p <= buf))
		strcpy(buf, "No valid datestamp in original.");

	    else
		{
		if ((strncmp((p+1),"remote from", 11) == 0) ||
		    (strncmp((p+1),"forwarded by", 12) == 0))
		    *p = '\0';
		}
	    }

	save_a_hdr(&pmsg->hdrinfo, buf, H_ODATE, (char*)0);
	}

    if ((hptr = parent->hdrinfo.hdrs[H_SUBJ]) != (Hdrs *)NULL)
	save_a_hdr(&pmsg->hdrinfo, hptr->value, H_OSUBJ, (char*)0);

    if (((hptr = parent->hdrinfo.hdrs[H_MSVC]) != (Hdrs *)NULL) &&
	((strlen(hptr->value) != 4) || (casncmp("mail", hptr->value, 4) != 0)))
	save_a_hdr(&pmsg->hdrinfo, hptr->value, H_OMSVC, (char*)0);

    if ((hptr = parent->hdrinfo.hdrs[H_MTSID]) != (Hdrs *)NULL)
	save_a_hdr(&pmsg->hdrinfo, hptr->value, H_CMTSID, (char*)0);

    if ((hptr = parent->hdrinfo.hdrs[H_UAID]) != (Hdrs *)NULL)
	save_a_hdr(&pmsg->hdrinfo, hptr->value, H_CUAID, (char*)0);

    if ((hptr = parent->hdrinfo.hdrs[H_AFWDFROM]) != (Hdrs *)NULL)
	for ( ; hptr != (Hdrs *)NULL; hptr = hptr->next)
	    if (hptr->hdrtype == H_AFWDFROM)
		save_a_hdr(&pmsg->hdrinfo, hptr->value, H_OAFWDFROM, (char*)0);

    save_a_hdr(&pmsg->hdrinfo, "", H_EOH, (char*)0);

    pmsg->ret_on_error = 0;
    pmsg->Rpath = s_append(pmsg->Rpath, s_to_c(toname));
    pmsg->orig = s_append(pmsg->orig, s_to_c(toname));
    pmsg->binflag = parent->binflag;

    s_free(toname);
    s_free(tmpstr);
}

void add_retmail(pmsg, wherefrom, rc)
Msg *pmsg;
int wherefrom;
int rc;
{
    static const char pn[] = "add_retmail";
    Recip	*r;
    Msg		*parent = pmsg->parent;
    string	*tmpstr = s_new();
    char	buf[1024], buf2[1024], cbuf[1024];
    Hdrs	*hptr;

    Dout(pn, 0, "entered\n");

    cbuf[0] = '\0';
    if ((hptr = parent->hdrinfo.hdrs[H_TCOPY]) != (Hdrs *)NULL) {
	    /* Pick comment field off of ">To:" line and put into cbuf */
	    getcomment(hptr->value, cbuf);
    }

    if (rc == 0)
	{
	pmsg->type = Msg_deliv;
	for (r = recips_head(parent, wherefrom)->next;
	     r != (Recip*)NULL;
	     r = r->next)
	    {
	    char datestring[60];
	    mkdate(datestring);
	    s_restart(tmpstr);
	    tmpstr = s_xappend(tmpstr, thissys, "!", s_to_c(r->name), " ", cbuf, " on ", datestring, (char*)0);
	    save_a_hdr(&pmsg->hdrinfo, s_to_c(tmpstr), H_DLVRTO, (char*)0);
	    }
	}

    else
	{
	/* If error is set already, use that as the indication of why we failed. */
	if (error != 0)
	    rc = error;

	sprintf(buf2, "due to %s", mta_ercode(rc));
	save_a_hdr(&pmsg->hdrinfo, buf2, H_NDLVRTO, (char*)0);
	if (ckdlivopts(parent) & RETURN)
	    {
	    save_cont_hdr(&pmsg->hdrinfo, "     ORIGINAL MESSAGE ATTACHED");
	    pmsg->type = Msg_nondeliv;
	    }

	else
	    pmsg->type = Msg_deliv;

	if (rc == E_FRWL)
	    {
	    r = recips_head(parent, wherefrom)->next;
	    s_restart(tmpstr);
	    tmpstr = s_xappend(tmpstr, "\t", progname, ": Forwarding loop detected in ",
		s_to_c(r->name), "'s mailfile.", (char*)0);
	    save_cont_hdr(&pmsg->hdrinfo, s_to_c(tmpstr));
	    }

	else if ((rc == E_SURG) || (rc == E_TRAN))
	    {
	    if (parent->surg_rc < 0)
		{
		switch (parent->surg_rc)
		    {
		    case SURG_RC_DEF:		/* return code if no surrogate run */
			sprintf(buf, "     (%s: Error # %d '%s', rc = %s)",
				progname, rc, stripfmt(errlist[rc]), "unknown failure");
			break;
		    case SURG_RC_FORK:		/* return code if surrogate fork failed */
			sprintf(buf, "     (%s: Error # %d '%s', rc = %s)",
				progname, rc, stripfmt(errlist[rc]), "fork failure");
			break;
		    case SURG_RC_ERR:		/* return code if surrogates failed */
			sprintf(buf, "     (%s: Error # %d '%s', rc = %s)",
				progname, rc, stripfmt(errlist[rc]), "temp file failure");
			break;
		    default:
			sprintf(buf, "     (%s: Error # %d '%s', errno = %d)",
				progname, rc, stripfmt(errlist[rc]), -parent->surg_rc);
			break;
		    }
		}

	    else
		sprintf(buf, "     (%s: Error # %d '%s', rc = %d)",
			progname, rc, stripfmt(errlist[rc]), parent->surg_rc);

	    save_cont_hdr(&pmsg->hdrinfo, buf);
	    for (r = recips_head(parent, wherefrom)->next; r != (Recip*)NULL; r = r->next)
		{
		s_restart(tmpstr);
		tmpstr = s_xappend(tmpstr, s_to_c(r->name), " ", cbuf, (char*)0);
		save_a_hdr(&pmsg->hdrinfo, s_to_c(tmpstr), H_ENROUTE, (char*)0);
		}

	    r = recips_head(parent, wherefrom)->next;

	    /* Tell them the command used */
	    if (r != (Recip*)NULL || parent->SURRcmd)
		{
		save_cont_hdr(&pmsg->hdrinfo, "     ======= Surrogate command =======");

		if (r->SURRcmd)
		    {
		    s_restart(r->SURRcmd);
		    Dout(pn, 1, "using saved recipient's SURRcmd\n");
		    save_cont_hdr(&pmsg->hdrinfo, s_to_c(r->SURRcmd));
		    }

		else if (parent->SURRcmd)
		    {
		    Dout(pn, 1, "using saved SURRcmd\n");
		    save_cont_hdr(&pmsg->hdrinfo, s_to_c(parent->SURRcmd));
		    }
		}

	    /* Include stderr from surrogate, if any */
	    if (r != (Recip*)NULL && r->SURRoutput)
		{
		string *o;
		Dout(pn, 1, "using saved recipient's SURRoutput\n'%s'", s_to_c(r->SURRoutput));
		s_restart(r->SURRoutput);
		for ( ; (o = s_tok(r->SURRoutput, "\n")) != (string*)0; s_free(o))
		    save_cont_hdr(&pmsg->hdrinfo, s_to_c(o));
		}

	    else
		{
		Dout(pn, 1, "writing saved SURRoutfile\n");
		if (errno == E_SURG)
		    saveoutput(parent->SURRoutfile, pmsg, "stdout");
		saveoutput(parent->SURRerrfile, pmsg, "stderr");

		if (!parent->SURRoutfile && !parent->SURRerrfile)
		    save_cont_hdr(&pmsg->hdrinfo, "     ==== stdout & stderr unavailable ====");
		}
	    }

	else if (rc == E_DENY)
	    {
	    sprintf(buf, "     (%s: Error # %d '%s')",
		progname, rc, stripfmt(errlist[rc]));
	    save_cont_hdr(&pmsg->hdrinfo, buf);
	    r = recips_head(parent, wherefrom)->next;
	    if (r->cmdl)
		{
		char *p;
		save_cont_hdr(&pmsg->hdrinfo, "     Reason:");
		for (p = strtok(s_to_c(r->cmdl), "\n"); p; p = strtok((char*)0, "\n"))
		    {
		    string *s = s_copy("          ");
		    s = s_append(s, p);
		    save_cont_hdr(&pmsg->hdrinfo, s_to_c(s));
		    s_free(s);
		    }
		}
	    }

	else
	    {
	    sprintf(buf, "     (%s: Error # %d '%s')",
		progname, rc, stripfmt(errlist[rc]));
	    save_cont_hdr(&pmsg->hdrinfo, buf);
	    }

	for (r = recips_head(parent, wherefrom)->next; r != (Recip*)NULL; r = r->next)
	    {
	    s_restart(tmpstr);
	    tmpstr = s_xappend(tmpstr, s_to_c(r->name), " ", cbuf, (char*)0);
	    save_a_hdr(&pmsg->hdrinfo, s_to_c(tmpstr), H_ENROUTE, (char*)0);
	    }
	}

    s_free(tmpstr);
}

void send_retmail(pmsg)
Msg *pmsg;
{
    static const char pn[] = "send_retmail";
    char	buf[1024];
    Hdrs	*hptr;

    Dout(pn, 0, "entered\n");
    pmsg->msgsize = (pmsg->type == Msg_nondeliv) ?
		    (pmsg->parent->msgsize + sizeheader(pmsg->parent)) : 0;
    sprintf(buf, "%ld", pmsg->msgsize);
    save_a_hdr(&pmsg->hdrinfo, buf, H_CLEN, (char*)0);
    if (pmsg->msgsize > 0)
	{
	save_a_hdr(&pmsg->hdrinfo, (pmsg->binflag == C_Text)  ? Text :
				 (pmsg->binflag == C_GText) ? GenericText :
							      Binary, H_CTYPE, (char*)0);
	if ((hptr = pmsg->parent->hdrinfo.hdrs[H_ENCDTYPE]) != (Hdrs*)NULL)
	    save_a_hdr(&pmsg->hdrinfo, hptr->value, H_ENCDTYPE, (char*)0);
	}

    (void) sendlist(pmsg, 0);
}

/* Write the output from the command with appropriate headers. */
static void saveoutput(ofp, pmsg, oname)
FILE *ofp;
Msg *pmsg;
char *oname;
{
    /* buf2 must hold all of buf plus 5 spaces and a ":" */
    char buf[1024+1], buf2[1024+7];
    if (ofp)
	{
	sprintf(buf, "     ==== Start of %s ===", oname);
	save_cont_hdr(&pmsg->hdrinfo, buf);
	rewind (ofp);
	buf[0] = buf[1024] = '\0';
	while (fgets(buf, sizeof(buf)-1, ofp) != (char *)NULL)
	    {
	    trimnl(buf);
	    sprintf(buf2, "     :%s", buf);
	    save_cont_hdr(&pmsg->hdrinfo, buf2);
	    }

	sprintf(buf, "     ====  End of %s  ===", oname);
	save_cont_hdr(&pmsg->hdrinfo, buf);
	}
}

/* Strip the :n: information and trailing newline from the 4.0 pfmt() strings */
static char *stripfmt(s)
const char *s;
{
    static char retbuf[80];
    register const char *cptr = strchr(s, ':');
    register char *bptr;

    /* skip the :n: information */
    if (cptr)
	    cptr = strchr(cptr + 1, ':');
    if (cptr)
	    cptr++;
    else
	    cptr = s;

    /* copy the buffer and stop at the newline */
    for (bptr = retbuf; (*bptr = *cptr) != '\0'; bptr++, cptr++)
	if ((*bptr == '\n') || (bptr == retbuf+79))
	    {
	    *bptr = '\0';
	    break;
	    }
    return retbuf;
}

/*
    NAME
	extract_to_name - Determine the address to send the ack/nak to.

    SYNOPSIS
	string *extract_to_name(Msg *parent, int nak)

    DESCRIPTION
	Extract a name from an appropriate header field.
	For ACKs, we look for the header
		Sender:
		From:
		Errors-To:
	For NAKs, we look for the header
		Reply-To:
		From:
		Sender:

	The name is most likely in standard RFC-822 format, which is either
		comment.... "<" address ">"
	    or
		address
	    or
		address (comment)

	In each case, we are only interested in the "address" part.
	We use a regular expression match to extract the appropriate part.

	If no address can be found, the calculated return path will be used.
*/
static string *extract_to_name(parent, nak)
Msg *parent;
int nak;
{
    static const char pn[] = "extract_to_name";
    register int i;
    Hdrs	*hptr;
    string	*ret;
    unsigned char re_map[256];
    int		hdr;
    static const int hdrtypes[4][2] =
		{   /* error    reply */
		    { H_SENDER, H_REPLYTO },
		    { H_FROM2,  H_FROM2 },
		    { H_ERRTO,  H_SENDER },
		    { 0,        0 }
		};

    for (i = 0; i < 256; i++)
	re_map[i] = (char)i;

    for (i = 0; (hdr = hdrtypes[i][nak]) != 0; i++)
	{
	/* First look for the header. */
	if (((hptr = parent->hdrinfo.hdrs[hdr]) != (Hdrs*)NULL) &&
	    (hptr->value[0] != '\0'))
	    {
	    int val_len = strlen(hptr->value);
	    static char pat1[] = ".*<[ \t]*([^ \t]+)[ \t]*>";
	    static char pat2[] = "^[ \t]*([^ \t]+)[ \t]*([(].*[)])?";
	    re_re *regex;
	    char *match[10][2];

	    /* Look for "comment <address>" */
	    regex = re_recomp(pat1, pat1 + sizeof(pat1) - 1, re_map);
	    if (!regex)
		{
		Dout(pn, 0, "Cannot compile regular expression 1!");
		continue;
		}

	    if (re_reexec(regex, hptr->value, hptr->value + val_len, match))
		{
		/* shorten the header field temporarily */
		char savec = match[1][1][0];
		match[1][1][0] = '\0';
		/* only believe domain addresses */
		if (strchr(match[1][0], '@'))
		    {
		    /* copy the substring */
		    ret = s_copy(match[1][0]);
		    /* restore the header */
		    match[1][1][0] = savec;
		    re_refree(regex);
		    return ret;
		    }
		/* restore the header */
		match[1][1][0] = savec;
		}
	    re_refree(regex);

	    /* Look for "address (comment)" */
	    regex = re_recomp(pat2, pat2 + sizeof(pat2) - 1, re_map);
	    if (!regex)
		{
		Dout(pn, 0, "Cannot compile regular expression 2!");
		continue;
		}

	    if (re_reexec(regex, hptr->value, hptr->value + val_len, match))
		{
		/* shorten the header field temporarily */
		char savec = match[1][1][0];
		match[1][1][0] = '\0';
		/* only believe domain addresses */
		if (strchr(match[1][0], '@'))
		    {
		    /* copy the substring */
		    ret = s_copy(match[1][0]);
		    /* restore the header */
		    match[1][1][0] = savec;
		    re_refree(regex);
		    return ret;
		    }
		/* restore the header */
		match[1][1][0] = savec;
		}
	    re_refree(regex);
	    pfmt(stdout, MM_ERROR, ":511:Unusable address in '%s' header field\n", header[hdr].tag);
	    }
	}

    /* default to using the calculated return path */
    ret = s_dup(parent->Rpath);
    return ret;
}
