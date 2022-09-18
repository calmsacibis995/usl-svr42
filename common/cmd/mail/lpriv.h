/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/lpriv.h	1.2.2.3"
#ident "@(#)lpriv.h	1.4 'attmail mail(1) command'"
/*
    "lpriv.h" is used by the mail programs which use trusted functions
*/

#ifndef LPRIV_H
#define LPRIV_H

#include <dirent.h>

#ifdef __STDC__
extern int check4mld(const char *dir);
extern DIR *realmode_opendir(const char *dir);
extern FILE *realmode_fopen(const char *f, const char *mode);
#else
extern int check4mld();
extern DIR *realmode_opendir();
extern FILE *realmode_fopen();
#endif
extern const char *progname;

#endif /* LPRIV_H */
