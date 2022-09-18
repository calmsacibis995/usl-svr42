/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/compat.c	1.3.2.5"
#ident "@(#)compat.c	1.10 'attmail mail(1) command'"
#include "libmail.h"
#ifndef isalpha
# include <ctype.h>
#endif

#ifndef va_arg
# if defined(__STDC__) || defined(__cplusplus)
#  include <stdarg.h>
# else
#  include <varargs.h>
# endif
#endif

/*
    This file contains reimplementations of several SVR4.1
    functions not in earlier releases.
*/

/*
    NAME
	pfmt, lfmt, vpfmt, addsev, gettxt, setlabel, setcat - print/retrieve error message

    SYNOPSIS
	int pfmt(FILE *out, long error_class, const char *fmt, ...)
	int lfmt(FILE *out, long error_class, const char *fmt, ...)
	int vpfmt(FILE *out, long error_class, const char *fmt, va_list args)
	void addsev(int, const char *msg)
	char *gettxt(const char *id, const char *msg)
	int setlabel(const char *label)
	const char *setcat(const char *catalog)

    DESCRIPTION
	Print out the error message, preceded with the name of the
	program (set by setlabel) and the class of the error.

	In 4.1, setcat specifies the catalog of messages to be used; it's
	ignored here.

    NAME
	strerror - return error message

    SYNOPSIS
	char *strerror(int errno)

    DESCRIPTION
	Return the error message associated with the given error number.

    NAME
	strstr - find string

    SYNOPSIS
	char *strstr(const char*, const char*)

    DESCRIPTION
	This routine looks for substring in string.
	If found, it returns a pointer to the substring,
	otherwise it returns NULL.
*/

#ifndef SVR4_1	/* needed for SVR3 and SVR4 */
/* PRINTFLIKE3 */
int
#ifdef __STDC__
pfmt(FILE *out, long error_class, const char *fmt, ...)
#else
# ifdef lint
pfmt(Xout, Xerror_class, Xfmt, va_alist)
FILE	*Xout;
long	Xerror_class;
const char	*Xfmt;
va_dcl
# else
pfmt(va_alist)
va_dcl
# endif
#endif
{
#ifndef __STDC__
    FILE *out;
    long error_class;
    char *fmt;
#endif
    va_list args;

#ifndef __STDC__
# ifdef lint
    out = Xout;
    error_class = Xerror_class;
    fmt = Xfmt;
# endif
#endif

#ifdef __STDC__
    va_start(args, fmt);
#else
    va_start(args);
    out = va_arg(args, FILE*);
    error_class = va_arg(args, long);
    fmt = va_arg(args, char*);
#endif
    return vpfmt(out, error_class, fmt, args);
}

/* PRINTFLIKE3 */
int
#ifdef __STDC__
lfmt(FILE *out, long error_class, const char *fmt, ...)
#else
# ifdef lint
lfmt(Xout, Xerror_class, Xfmt, va_alist)
FILE	*Xout;
long	Xerror_class;
const char	*Xfmt;
va_dcl
# else
lfmt(va_alist)
va_dcl
# endif
#endif
{
#ifndef __STDC__
    FILE *out;
    long error_class;
    char *fmt;
#endif
    va_list args;

#ifndef __STDC__
# ifdef lint
    out = Xout;
    error_class = Xerror_class;
    fmt = Xfmt;
# endif
#endif

#ifdef __STDC__
    va_start(args, fmt);
#else
    va_start(args);
    out = va_arg(args, FILE*);
    error_class = va_arg(args, long);
    fmt = va_arg(args, char*);
#endif
    return vpfmt(out, error_class, fmt, args);
}

const char *setcat(catalog)
const char *catalog;
{
    return catalog;
}

static char programlabel[MAXLABEL];

int setlabel(nlabel)
const char *nlabel;
{
    strncpy(programlabel, nlabel, MAXLABEL);
    programlabel[MAXLABEL-1] = '\0';
    return 1;
}

#ifdef __STDC__
int vpfmt(FILE *out, long error_class, const char *fmt, va_list args)
#else
int vpfmt(out, error_class, fmt, args)
FILE *out;
long error_class;
const char *fmt;
va_list args;
#endif
{
    int ret, ret2, ret3;
    if ((fmt[0] == ':') && isdigit(fmt[1]))
	{
	fmt += 2;
	while (isdigit(fmt[0]))
	    fmt++;
	fmt++;
	}

    if (programlabel[0] != '\0')
	ret = fprintf (out, "%s: ", programlabel);
    else
	ret = 0;

    switch (error_class)
	{
	case MM_ACTION: ret2 = fputs("ACTION: ", out); break;
	case MM_ERROR:	ret2 = fputs("ERROR: ", out); break;
	case MM_INFO:	ret2 = fputs("INFO: ", out); break;
	case MM_NOSTD:	break;
	case MM_WARNING:ret2 = fputs("WARNING: ", out); break;
	}

    ret3 = vfprintf(out, fmt, args);
    if (ferror(out))
	return EOF;
    return ret + ret2 + ret3;
}
#endif

#ifdef SVR3	/* needed for SVR3 */
char *strerror(num)
int num;
{
    static char buf[50]; /* large enough for 'Error #%d' */
    extern char *sys_errlist[];
    extern int sys_nerr;
    if (num >= 0 && num < sys_nerr)
	return sys_errlist[num];
    (void) sprintf(buf, "Error #%d", num);
    return buf;
}

char *strstr(s1, s2)
const char *s1;
const char *s2;
{
    int ret = substr(s1, s2);
    return (ret == -1) ? 0 : (s1 + ret);
}

/* ARGSUSED */
int addsev(dummy, msg)
int dummy;
const char *msg;
{
    (void) fputs(msg, stderr);
    (void) putc('\n', stderr);
    return 0;
}

/* ARGSUSED */
char *gettxt(id, msg)
const char *id;
const char *msg;
{
    return msg;
}
#endif

#ifndef __STDC__
char *setlocale(a, b)
int a;
const char *b;
{
    return "";
}
#endif
