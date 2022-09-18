/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/rewrite.c	1.2.2.2"
#ident "@(#)rewrite.c	1.5 'attmail mail(1) command'"

#include <stdio.h>
#include "s_string.h"
#include "smtp.h"
#include "hlist.h"

extern int cistrncmp proto((char *, char *, int));

static char ws[] = " \t\n";

/*
 *  scan_ws() -- Scan over white space.  "p" is assumed to point to
 *	the beginning of the white space.
 */
static char *scan_ws(p)
register char *p;
{
	return p + strspn(p, ws);
}

/*
 *  scan_nonws() -- Scan over non-white space.  "p" is assumed to point to
 *	the beginning of the non-white space.
 */
static char *scan_nonws(p)
register char *p;
{
	return p + strcspn(p, ws);
}

/*
 *  scan_comm() -- Scan over a comment, taking into account
 *	nested comments.  "p" is assumed to point to the opening '('.
 */
static char *scan_comm(p)
register char *p;
{
	register int level = 0;
	do {
		if (*p == '(')
			level++;
		else if (*p == ')')
			level--;
		else if ((*p == '\\') && p[1])
			p++;
		else if (*p == '\0')
			break;
		p++;
	} while (level > 0);
	return p;
}

/*
 *  scan_qstr() -- Scan over a quoted string.  "p" is assumed
 *	to point to the opening '"'.
 */
static char *scan_qstr(p)
register char *p;
{
	int loop = 1;
	p++;
	do {
		if ((*p == '\\') && p[1])
			p++;
		else if (*p == '\0')
			break;
		else if (*p == '"')
			loop = 0;
		p++;
	} while (loop);
	return p;
}

/*
 *  rewrite_address() -- Scans the string from, and rewrites:
 *	   @x,@y:d%c%b@a ->	x!y!a!b!c!d
 *	   d%c%b@a	 ->	a!b!c!d
 *	   c!d%b@a	 ->	a!b!c!d
 */
static char *rewrite_address(from)
char *from;
{
	static string *buf;
	char *sp, *ep;
	int end;

	buf = s_reset(buf);

	/*
	 * parse leading @a,@b,@c:
	 */
	while (*from == '@') {
		/* find end of string */
		for (ep= ++from; *ep!=':' && *ep!=',' && *ep!='\0'; ep++)
			;
		end = *ep;
		*ep = '\0';
		s_append(buf, from);
		s_append(buf, "!");
		from = end ? ep+1 : ep;
	}

	/*
	 *  parse the rest (whatever it may be)
	 */
	for (sp = from + strlen(from); sp >= from; sp--) {
		if (*sp == '@' || *sp == '%') {
			/* hack to get rid of forwarding crap */
			s_append(buf, sp+1);
			s_append(buf, "!");
			*sp = '\0';
		}
	}

	s_append(buf, from);
	return s_to_c(buf);
}

/*
 *  rewrite_subfield() -- Scans the string p, and rewrites each
 *	non-comment using rewrite_address.
 */
static char *rewrite_subfield(p)
register char *p;
{
	static string *s;
	register char *sp;

	s = s_reset(s);
	while (*p) {
		if (*p == '(') {
			/* Append the comment to the output string */
			sp = p;
			p = scan_comm(p);
			for (; sp < p; sp++)
				s_putc(s, *sp);
			s_terminate(s);
		} else if (strchr(ws, *p)) {
			/* Append the white space to the output string */
			/* Change '\n' => ' ' */
			for (; *p && strchr(ws, *p); p++)
				if (*p == '\n')
					s_putc(s, ' ');
				else
					s_putc(s, *p);
			s_terminate(s);
		} else {
			/* Append the (rewritten) address to the output string */
			char c;
			sp = p;
			p = scan_nonws(p);
			c = *p;
			*p = '\0';
			s_append(s, rewrite_address(sp));
			*p = c;
		}
	}
	return s_to_c(s);
}

/*
 *  rewrite_anglebracket() -- Take a subfield of the form:
 *		Joe Isuzu <joe@ISUZU.COM>
 *	and rewrite it to the form
 *		joe@ISUZU.COM (Joe Isuzu)
 *	lap and rap point to the left and right angle brackets
 *	respectivly.
 */
static char *rewrite_anglebracket(p, lap, rap)
char *p, *lap, *rap;
{
	static string *s;

	s = s_reset(s);
	*lap = *rap = '\0';
	s_append(s, lap+1);
	if (p < lap) {
		register char *p2 = lap;
		while (p2 > p && strchr(ws, *--p2))
			;
		p2[1] = '\0';
		s_append(s, " (");
		s_append(s, p);
		s_append(s, ")");
	}
	return s_to_c(s);
}

/*
 *  rewrite_header() -- Rebuilds the current header, rewriting as we go.
 *	Trys to find comma-separated subfields, and rewrites each one
 *	individually.
 */
static string *rewrite_header(hp)
char *hp;
{
	register char *p, *sp;
	int notdone, leftangle, rightangle, length;
	char *lap, *rap, *lastline, *lastcomma;
	string *s;

	s = s_new();
	p = strchr(hp, ':');
	*p++ = '\0';
	s_append(s, hp);
	s_append(s, ": ");
	length = strlen(s_to_c(s));

	for (notdone = 1; notdone; ) {
		/* skip leading white space */
		sp = p = scan_ws(p);

		/* scan over next comma-separated address */
		leftangle = rightangle = 0;
		for (;;) {
			if (*p == '\0')
				break;
			/* DON'T break on a comma if it is in the middle */
			/* of an <@a,@b,@c:xxx> style address */
			if ((*p == ',') && ((leftangle ^ rightangle) == 0))
				break;
			if (*p == '(')
				p = scan_comm(p);
			else if (*p == '"')
				p = scan_qstr(p);
			else {
				if (*p == '<') {
					leftangle = 1;
					lap = p;
				}
				if ((*p == '>') && leftangle) {
					rightangle = 1;
					rap = p;
				}
				p++;
			}
		}
		if (*p == '\0')
			notdone = 0;
		*p++ = '\0';

		/* rewrite this part of the field, and append to string */
		if (rightangle)
			sp = rewrite_anglebracket(sp, lap, rap);
		sp = rewrite_subfield(sp);

		/* if the line is to long, add a newline */
		if ((int)(length + strlen(sp)) >= 80) {
			s_append(s, "\n\t");
			length = 8;
		}
		s_append(s, sp);
		length += strlen(sp);

		/* if more to go, add a comma to seperate addresses */
		if (notdone) {
			s_append(s, ", ");
			length += 2;
		}
	}
	s_append(s, "\n");
	return s;
}

/*
 *  fix_headers -- Invokes rewrite_header() on all From:/To:/Cc:/Bcc:
 *	headers in the header list.
 */
void fix_headers()
{
	register hlist *hp;
	extern hlist dummy, *list;

	for (hp = list->next->next; hp != &dummy; hp = hp->next) {
		if (   (cistrncmp(s_to_c(hp->line), "from:", 5) == 0)
		    || (cistrncmp(s_to_c(hp->line), "to:", 3) == 0)
		    || (cistrncmp(s_to_c(hp->line), "cc:", 3) == 0)
		    || (cistrncmp(s_to_c(hp->line), "bcc:", 4) == 0)) {
			string *s;
			s = rewrite_header(s_to_c(hp->line));
			s_free(hp->line);
			hp->line = s;
		}
	}
}
