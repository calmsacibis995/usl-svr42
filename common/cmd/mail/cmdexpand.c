/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/cmdexpand.c	1.6.2.4"
#ident "@(#)cmdexpand.c	1.9 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	cmdexpand - expand mail surrogate command string

    SYNOPSIS
	string *cmdexpand(Msg *pmsg, Recip *r, string *instr, char **lbraslist, char **lbraelist,
		string *outstr)

    DESCRIPTION
	 cmdexpand() will expand the mail surrogate command.
	 It will make the following changes to the string:


		%D	->	local domain name
		%L	->	local system name
		%U	->	local uname
		%X	->	smarter host
		\1 - \9	->	pointers into orig and recip
		%l	->	content length
		%H	->	header length
		%n	->	recipient name
		%R	->	return path to originator
		%C	->	content type
		%c	->	content type
		%S	->	subject
		%[a-z]	->	Mgetenv(%x)
		\x	->	x
		%%	->	%
*/

#define BS '\\'

string *cmdexpand(pmsg, r, instr, lbraslist, lbraelist, outstr)
Msg *pmsg;
Recip *r;
string *instr;
char **lbraslist;
char **lbraelist;
string *outstr;
{
    static const char pn[] = "cmdexpand";
    register char *ip;
    register char *brap;
    Hdrs	*hptr;
    register int i;

    if (!instr)
	return 0;

    ip = s_to_c(instr);
    outstr = s_reset(outstr);

    Dout(pn, 7, "instr = '%s'\n", s_to_c(instr));
    for ( ; *ip; ip++)
	{
	switch (*ip)
	    {
	    case '%':
		switch (*++ip)
		    {
		    case 'C':
			outstr = s_append(outstr, (pmsg->binflag == C_Text) ? Text : Binary);
			break;

		    case 'c':
			if ((hptr = pmsg->hdrinfo.hdrs[H_CTYPE]) !=
					(Hdrs *)NULL) {
				outstr = s_append(outstr, hptr->value);
			}
			break;

		    case 'l':
			if ((hptr = pmsg->hdrinfo.hdrs[H_CLEN]) !=
					(Hdrs *)NULL) {
				outstr = s_append(outstr, hptr->value);
			} else
				outstr = s_append(outstr, "0");
			break;

		    case 'H': {
			char buf[40]; /* large enough to hold a long */
			sprintf(buf, "%ld", sizeheader(pmsg));
			outstr = s_append(outstr, buf);
			}
			break;

		    case 'D':
			outstr = s_append(outstr, maildomain());
			break;

		    case 'L':
			outstr = s_append(outstr, thissys);
			break;

		    case 'n':
			outstr = s_append(outstr, s_to_c(r->name));
			break;

		    case 'O':
			outstr = s_append(outstr, s_to_c(recip_parent(r)->name));
			break;

		    case 'R':
			outstr = s_append(outstr, s_to_c(pmsg->Rpath));
			break;

		    case 'U':
			outstr = s_append(outstr, mailsystem(1));
			break;

		    case 'X':
			outstr = s_append(outstr, Mgetenv("SMARTERHOST"));
			break;

		    case 'S':
			if ((hptr = pmsg->hdrinfo.hdrs[H_SUBJ]) !=
					(Hdrs *)NULL) {
				outstr = s_append(outstr, hptr->value);
			}
			break;

		    default:
			if (islower(*ip))
			    {
			    char x[3];
			    x[0] = '%';
			    x[1] = *ip;
			    x[2] = '\0';
			    outstr = s_append(outstr, Mgetenv(x));
			    }

			else
			    s_putc(outstr, *ip);
			break;
		    }
		break;

	    case BS:
		switch (*++ip)
		    {
		    default:	/* \x -> \x */
			s_putc(outstr, BS);
			s_putc(outstr, *ip);
			break;

		    /* \1 - \0 becomes braslist[0] - braslist[9] */
		    case '1': case '2': case '3': case '4': case '5':
		    case '6': case '7': case '8': case '9': case '0':
			i = (*ip == '0') ? 9 : (*ip - '1');
			if (lbraslist[i])
			    for (brap = lbraslist[i]; brap < lbraelist[i]; brap++)
				s_putc(outstr, *brap);
			break;
		    }
		break;

	    default:
		s_putc(outstr, *ip);
	    }
	}

    s_terminate(outstr);
    Dout(pn, 7, "outstr = '%s'\n", s_to_c(outstr));
    return outstr;
}
