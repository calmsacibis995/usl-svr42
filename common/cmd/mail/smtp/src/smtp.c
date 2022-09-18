/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/smtp.c	1.7.2.2"
#ident "@(#)smtp.c	1.12 'attmail mail(1) command'"
/*
 * smtp -- client, send mail to remote smtp server
 * Set up for 4.2BSD networking system.
 * TODO:
 *	better mapping of error numbers.
 *	allow partial delivery to multiple recipients when only some
 *		fail (maybe)
 * 	send stuff from cmds.h instead of hard-coded here
 */

#ifdef SVR4_1
static char USAGE1[] =
	":10:usage: %s [-D] [-N] [-u] [-d domain] [-H helohost] sender targethost recip ...\n";
static char USAGE2[] =
	":11:usage: %s [-D] [-N] ctlfiles ...\n";
static char USAGE3[] =
	":12:%s: invalid control file\n";
#else
static char USAGE1[] =
	"usage: %s [-D] [-u] [-d domain] [-H helohost] sender targethost recip ...\n";
static char USAGE2[] =
	"usage: %s [-D] ctlfiles ...\n";
static char USAGE3[] =
	"%s: invalid control file\n";
#endif

#include <stdio.h>
#include <signal.h>
#include <string.h>
#ifdef SVR4
#include <stdlib.h>
#endif
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include "addrformat.h"
#include "miscerrs.h"
#include "smtp.h"
#include "s_string.h"
#include "smtp_decl.h"
#include "aux.h"
#ifdef SVR4_1
#include <locale.h>
#include <pfmt.h>
#include <mac.h>
#endif

char	*progname;
char	*helohost;
char	*usage_msg;
int	debug = 0;
int	no_nameserver = 0;

/*
 * main - parse arguments and handle options
 */
main(argc, argv)
int argc;
char *argv[];
{
	int unixformat = 0;
	char *domain = 0;
	char *sender = 0;
	char *host = 0;
	char *addr = 0;
	char *p;
	namelist *recips;
	FILE *sfi, *sfo;

#ifdef SVR4_1
	(void) setcat("uxsmtp");
	(void) setlocale(LC_ALL, "");
	(void) mldmode(MLD_VIRT);
#endif
	signal(SIGCLD, SIG_DFL); /* Allow us to wait on children */
	/*
	 *  Check if we have been invoked as "smtpbatch".
	 *  If so, read arguments from the first control file in the list.
	 */
	p = strrchr(argv[0], '/');
	p = (p == NULL) ? argv[0] : (p+1);
	progname = p;
	if (strcmp(p, "smtpbatch") == 0) {
		register int c;
		extern int optind;
#ifdef SVR4_1
		(void) setlabel("UX:smtpbatch");
#endif
		while ((c = getopt(argc, argv, "DN")) != EOF) {
			switch (c) {
			case 'D':
				debug = 1;
				break;
			case 'N':
				no_nameserver = 1;
				break;
			default:
#ifdef SVR4_1
				(void) pfmt(stderr, MM_ACTION, USAGE2, progname);
#else
				(void) fprintf(stderr, USAGE2, progname);
#endif
				bomb(E_USAGE);
			}
		}
		usage_msg = USAGE3;
		if ((argc - optind) <= 0) {
#ifdef SVR4_1
			(void) pfmt(stderr, MM_ACTION, USAGE2, progname);
#else
			(void) fprintf(stderr, USAGE2, progname);
#endif
			bomb(E_USAGE);
		}
		init_batch(argc-optind+1, &argv[optind-1]);
		donext(&unixformat, &sender, &recips, &domain,
		       stdin, &addr, &host);
	} else {
#ifdef SVR4_1
		(void) setlabel("UX:smtp");
#endif
		/* Process standard argument list */
		usage_msg = USAGE1;
		setupargs(argc, argv, &unixformat, &sender,
			  &recips, &domain, &addr, &host);
	}

	if (sender) {
		/*
		*  open connection
		*/
		setup(addr ? addr : host, &sfi, &sfo);

		/*
		 *  hold the conversation
		 */
		converse(unixformat, sender, recips, domain, sfi, sfo, stdin);
	}

	exit(0);
}

void setupargs(argc, argv, unixformat, sender, recips, domain, addr, host)
int argc;
char **argv;
int *unixformat;
char **sender;
namelist **recips;
char **domain, **addr, **host;
{
	register int c;
	int errflg = 0;
	string *hh;
	string *replyaddr=s_new();
	extern int optind;
	extern char *optarg;

	optind = 1;		/* Reinitialize getopt(3) -- because it is */
	optarg = (char *)0;	/* used several times when batching */

	*unixformat = 0;
	*sender = *domain = *addr = *host = (char *) 0;

	while ((c = getopt(argc, argv, "a:uDd:H:N")) != EOF)
		switch (c) {
		case 'a':
			*addr = optarg;
			break;
		case 'u':
			*unixformat = 1;
			break;
		case 'D':
			debug = 1;
			break;
		case 'N':
			no_nameserver = 1;
			break;
		case 'd':		/* domain name */
			*domain = optarg;
			break;
		case 'H':		/* host for HELLO message */
			helohost = optarg;
			break;
		case '?':
		default:
			errflg++;
			break;
		}
	if (errflg || (argc - optind) < 3) {
#ifdef SVR4_1
		(void) pfmt(stderr, MM_ACTION, usage_msg, progname);
#else
		(void) fprintf(stderr, usage_msg, progname);
#endif
		bomb(E_USAGE);
	}

	/*
	 *  figure out what to call ourselves
	 */
	if (helohost==NULL)
		helohost=s_to_c(s_copy(sysname_read()));

	/*
	 *  if there is no domain in the helo host name
	 *  and the -d option is specified, domainify
	 *  the helo host
	 */
	if (strchr(helohost, '.') == 0 && *domain) {
		hh = s_copy(helohost);
		s_append(hh, *domain);
		helohost = s_to_c(hh);
	}

	/*
	 *  put our address onto the reply address
	 */
	if (strchr(argv[optind], '!') == 0 || !*domain){
		s_append(replyaddr, helohost);
		s_append(replyaddr, "!");
	}
	s_append(replyaddr, argv[optind]);
	optind++;

	/*
	 *  convert the arguments to 822 form
	 */
	*sender = convertaddr(s_to_c(replyaddr), *domain ? *domain : "", SOURCEROUTE);
	*host = argv[optind++];
	*recips = newname(convertto(argv[optind++], *unixformat, *host));
	for (; optind < argc; optind++)
		*recips = appendname(*recips, convertto(argv[optind], *unixformat, *host));
}

namelist *
newname(s)
char *s;
{
	namelist *np;

	np = (namelist *) malloc(sizeof(namelist));
	if (np == NULL)
		bomb(1);
	np->name = s;
	np->next = NULL;
	return np;
}

/* could add at beginning, but let's maintain original order */
namelist *
appendname(nl, s)
	char *s;
	namelist *nl;
{
	register namelist *tl;

	if (nl == NULL)
		bomb(1);	/* shouldn't happen */
	for (tl=nl; tl->next!=NULL; tl=tl->next)
		;
	tl->next = newname(s);
	return nl;
}

/*
 *  convert a destination address to outgoing format
 *
 *	if unixformat, just leave it alone
 *
 *	if not add the destination host name.
 */
char *
convertto(recip, unixformat, desthost)
	char *recip;
	char *desthost;
{
	static string *buf;

	if(unixformat)
		return recip;
	
	buf = s_reset(buf);
	s_append(buf, desthost);
	s_append(buf, "!");
	s_append(buf, recip);
	return convertaddr(s_to_c(buf), "", SOURCEROUTE);
}
