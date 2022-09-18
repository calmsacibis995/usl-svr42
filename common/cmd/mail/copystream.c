/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/copystream.c	1.4.2.2"
#ident "@(#)copystream.c	1.4 'attmail mail(1) command'"
#include "libmail.h"
/*
    NAME
	copystream - copy one FILE stream to another

    SYNOPSIS
	int copystream(FILE *infp, FILE *outfp)

    DESCRIPTION
	copystream() copies one stream to another. The stream
	infp must be opened for reading and the stream outfp
	must be opened for writing.

	It returns true if the stream is successively copied;
	false if any writes fail, or if SIGPIPE occurs while
	copying.
*/

static int pipecatcher;

/* ARGSUSED */
static void catchsigpipe(i)
int i;
{
    pipecatcher = 1;
}

int copystream(infp, outfp)
register FILE *infp;
register FILE *outfp;
{
    char buffer[BUFSIZ];
    register int nread;
    void (*savsig)();

    pipecatcher = 0;
    savsig = signal(SIGPIPE, catchsigpipe);

    while (((nread = fread(buffer, sizeof(char), sizeof(buffer), infp)) > 0) &&
           (pipecatcher == 0))
	if (fwrite(buffer, sizeof(char), nread, outfp) != nread)
	    {
	    (void) signal(SIGPIPE, savsig);
	    return 0;
	    }

    if (fflush(outfp) == EOF)
	{
	(void) signal(SIGPIPE, savsig);
	return 0;
	}

    (void) signal(SIGPIPE, savsig);
    return (pipecatcher == 0);
}
