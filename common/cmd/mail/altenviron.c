/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/altenviron.c	1.2.2.2"
#ident "@(#)altenviron.c	1.2 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	init_altenviron - set up alternate environment

    SYNOPSIS
	void init_altenviron()

    DESCRIPTION
	init_altenviron() initializes the alternate environment
	used in pipeletter() to execute surrogate commands.
*/

#define MAXENV 10
#define CHUNKSIZE 10
static char	*_altenviron[MAXENV];
static int	envcnt = MAXENV-1;
char		**altenviron = &_altenviron[0];

void init_altenviron()
{
    register int i = 0;
    register char *p, *s, *exportlist;

    /* Force some variables to a fixed environment. */
#ifdef SVR3
    altenviron[i++] = "PATH=/usr/lib/mail/surrcmd:/bin:/usr/bin";
    altenviron[i++] = "SHELL=/bin/sh";
#else
    altenviron[i++] = "PATH=/usr/lib/mail/surrcmd:/usr/bin";
    altenviron[i++] = "SHELL=/usr/bin/sh";
#endif

    /* Pass on some variables from the user's environment. */
    if (ismail)
        {
	if ((p = getenv("HOME")) != (char*)NULL)
	    altenviron[i++] = p - 5;
	}

    else
        altenviron[i++] = "HOME=/";

    if ((p = getenv("TZ")) != (char*)NULL)
	altenviron[i++] = p - 3;
    if ((p = getenv("LOGNAME")) != (char*)NULL)
	altenviron[i++] = p - 8;

    /* Now run through $SURR_EXPORT for others to export. */
    if ((exportlist = mgetenv("SURR_EXPORT")) != (char*)NULL)
	{
	for (s = strtok(exportlist, ","); s; s = strtok((char*)0, ","))
	    {
	    if ((p = getenv(s)) != (char*)NULL)
		if ((i < envcnt) || expand_argvec(&altenviron, CHUNKSIZE, _altenviron, &envcnt))
		    altenviron[i++] = p - strlen(s) - 1;
	    }
	}

    /* Now run through $CNFG_EXPORT for mailcnfg variables to export. */
    if ((exportlist = mgetenv("CNFG_EXPORT")) != (char*)NULL)
	{
	for (s = strtok(exportlist, ","); s; s = strtok((char*)0, ","))
	    {
	    if ((p = mgetenv(s)) != (char*)NULL)
		if ((i < envcnt) || expand_argvec(&altenviron, CHUNKSIZE, _altenviron, &envcnt))
		    altenviron[i++] = p - strlen(s) - 1;
	    }
	}

    altenviron[i] = (char*)NULL;
}
