/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/extra.c	1.1.2.2"
#ident "@(#)extra.c	1.1 'attmail mail(1) command'"

#include <stdio.h>
#include "mail.h"
#include "smtp.h"

extern char *sysname_read proto((void));

char *full_domain_name(domain)
char *domain;
{
	register char *p;
	static char namebuf[256];

	(void) xsetenv(MAILCNFG);
	p = xgetenv("CLUSTER");
	if (p)
		(void) strcpy(namebuf, p);
	else
		(void) strcpy(namebuf, s_to_c(s_copy(sysname_read())));
	if (domain == NULL)
		domain = maildomain();
	if (domain)
		(void) strcat(namebuf, domain);
	return namebuf;
}

char *message_id()
{
	register struct tm *t;
	static char msgid[128];
	static time_t was = (time_t) 0;
	static int uniqueval;
	char unique[3];
	time_t now;

	now = time((long *) 0);
	t = localtime(&now);
	if ((now/60) == (was/60)) {
		uniqueval++;
	} else {
		was = now;
		uniqueval = 0;
	}
	unique[0] = 'A' + (uniqueval / 26);
	unique[1] = 'A' + (uniqueval % 26);
	unique[2] = '\0';
	sprintf(msgid, "<%2.2d%2.2d%2.2d%2.2d%2.2d.%2.2s%5.5d@%.100s>",
		t->tm_year, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min,
		unique, getpid(), full_domain_name((char *)NULL));
	return msgid;
}
