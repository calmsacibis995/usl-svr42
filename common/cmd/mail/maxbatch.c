/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/maxbatch.c	1.2.2.2"
#ident "@(#)maxbatch.c	1.2 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	maxbatchsize - return the size of the B= batch specification

    SYNOPSIS
	int maxbatchsize(string *B)

    DESCRIPTION
	maxbatchsize() looks at the string B. If it looks like
	B=*, then the batchsize is the maximum exec size (previously
	always 5120) minus the size of the alternate environment.
	If B looks like B=number, then the batchsize is that number.
	Negative numbers become 0.
*/

int maxbatchsize(B)
string *B;
{
    static int altenvsize = -1;
    static int argmax = -1;
    int ret;

    /* Calculate the size of the alternate environment. */
    if (altenvsize == -1)
	{
	register int i;
	altenvsize = 0;
	/* Save the size for other use */
	for (i = 0; altenviron[i] != (char*)NULL; i++)
	    altenvsize += strlen(altenviron[i]) + 1;
	}

    /* Calculate the maximum size of the argument list. */
    if (argmax == -1)
	{
	if ((argmax = atoi(Mgetenv("ARG_MAX"))) > 0)
	    /*EMPTY*/;

#ifdef _SC_ARG_MAX
	else if ((argmax = sysconf(_SC_ARG_MAX)) > 0)
	    /*EMPTY*/;
#endif

	else
	    argmax = 5120;
	}

    /* Max out the argument list. */
    if (s_to_c(B)[2] == '*')
	ret = argmax - altenvsize;

    /* Believe the number unless it's too large. */
    else
	{
	ret = atoi(s_to_c(B) + 2);
	if (ret > argmax - altenvsize)
	    ret = argmax - altenvsize;
	}

    if (ret < 0)
	ret = 0;
    return ret;
}
