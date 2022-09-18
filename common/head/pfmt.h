/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PFMT_H
#define _PFMT_H

#ident	"@(#)sgs-head:pfmt.h	1.6"

#define MM_STD		0
#define MM_NOSTD	0x100
#define MM_GET		0
#define MM_NOGET	0x200

#define MM_ACTION	0x400

#define MM_NOCONSOLE	0
#define MM_CONSOLE	0x800

/* Classification */
#define MM_NULLMC	0
#define MM_HARD		0x1000
#define MM_SOFT		0x2000
#define MM_FIRM		0x4000
#define MM_APPL		0x8000
#define MM_UTIL		0x10000
#define MM_OPSYS	0x20000

/* Most commonly used combinations */
#define MM_SVCMD	MM_UTIL|MM_SOFT

/* Severity */
struct sev_tab {
	int severity;
	char *string;
};
#define MM_ERROR	0
#define MM_HALT		1
#define MM_WARNING	2
#define MM_INFO		3

#ifndef _STDIO_H
#include <stdio.h>
#endif
#ifndef _VARARGS_H
#ifdef __STDC__
#include <stdarg.h>
#else 
#include <varargs.h>
#endif /* __STDC__ */
#endif /* var_args */

#ifdef __STDC__
int pfmt(FILE *, long, const char *, ...);
int lfmt(FILE *, long, const char *, ...);
int __pfmt_print(FILE *, long, const char *, const char **, const char **, va_list);
int __lfmt_log(const char *, const char *, va_list, long, int);
int vpfmt(FILE *, long, const char *, va_list);
int vlfmt(FILE *, long, const char *, va_list);
const char *setcat(const char *);
int setlabel(const char *);
int addsev(int, const char *);
const char *__gtxt(const char *, int, const char *);
char *gettxt(const char *, const char *);
#else
char *setcat();
char *__gtxt();
char *gettxt();
#endif

#define DB_NAME_LEN		15
#define MAXLABEL		25

#endif /* _PFMT_H */
