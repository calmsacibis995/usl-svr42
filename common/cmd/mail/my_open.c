/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/my_open.c	1.5.2.2"
#ident "@(#)my_open.c	2.6 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	my_open,my_close,my_fopen,my_fclose - debugging open/close functions

    SYNOPSIS
	int my_open(char *s, int n)
	int my_close(int n)
	FILE *my_fopen(char *s, char *t)
	int my_fclose(FILE *fp)

    DESCRIPTION
	my_open,my_close,my_fopen,my_fclose call the corresponding C library
	functions, but also print a debugging message.
*/

#undef open
#undef close
#undef fopen
#undef fclose

int my_open(s,n)
char *s;
int n;
{
    static const char pn[] = "my_open";
    int	fd = open(s,n);
    Dout(pn, 0, "fd = %d, filename = '%s'\n", fd, s);
    return (fd);
}

int my_close(n)
int	n;
{
    static const char pn[] = "my_close";
    int rc = close(n);

    Dout(pn, 0, "fd = %d, rc from close() = %d\n", n, rc);
    return (rc);
}

FILE *
my_fopen(s,t)
char *s, *t;
{
    static const char pn[] = "my_fopen";
    FILE	*fp = fopen(s,t);
    Dout(pn, 0, "fd = %d, filename = '%s'\n", fileno(fp), s);
    return (fp);
}

my_fclose(fp)
FILE	*fp;
{
    static const char pn[] = "my_close";
    int fd = fileno(fp);
    int rc = fclose(fp);
    Dout(pn, 0, "fd = %d, rc from fclose() = %d\n", fd, rc);
    return (rc);
}
