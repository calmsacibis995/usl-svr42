/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/libmail.h	1.8.2.4"
#ident "@(#)libmail.h	1.13 'attmail mail(1) command'"

#ifndef __LIBMAIL_H
# define __LIBMAIL_H

#include "stdc.h"
#include <maillock.h>
#include "s_string.h"

extern	string *abspath ARGS((const char *path, const char *dot, string *to));
extern	int bang_collapse ARGS((char *s));
extern	char *basename ARGS((const char *path));
extern	int cascmp ARGS((const char *s1, const char *s2));
extern	int casncmp ARGS((const char *s1, const char *s2, int n));
extern	void closeallfiles ARGS((int firstfd));
extern	int copystream ARGS((FILE *infp, FILE *outfp));
extern	int delempty ARGS((mode_t m, const char *mailname));
extern	void errexit ARGS((int exitval, int sverrno, char *fmt, ...));
extern	int expand_argvec ARGS((char ***pargvec, int chunksize, char **_argvec, int *pargcnt));
extern	int	islocal ARGS((const char *user, uid_t *puid));
extern	pid_t loopfork ARGS((void));
extern	char *maildomain ARGS((void));
extern	char *mailsystem ARGS((int));
extern	char *Mgetenv ARGS((const char *env));
extern	char *mgetenv ARGS((const char *env));
extern	int newer ARGS((const char *file1, const char *file2));
extern	void notify ARGS((char *user, char *msg, int check_mesg_y, char *etcdir));
extern	int parse_execarg ARGS((char *p, int i, int *pargcnt, char ***argvec, int chunksize, char **_argvec));
extern	int pclosevp ARGS((FILE *fp));
extern	FILE *popenvp ARGS((char *file, char **argv, char *mode, int resetid));
extern	int posix_chown ARGS((const char *arg));
extern	char **setup_exec ARGS((char *s));
extern	const char *skipspace ARGS((const char *p));
extern	const char *skiptospace ARGS((const char *p));
extern	int sortafile ARGS((char *infile, char *outfile));
extern	void strmove ARGS((char *to, const char *from));
extern	int substr ARGS((const char *string1, const char *string2));
extern	int systemvp ARGS((const char *file, const char **argv, int resetid));
extern	void trimnl ARGS((char *s));
extern	char *Xgetenv ARGS((const char *env));
extern	char *xgetenv ARGS((const char *env));
extern	int xsetenv ARGS((char *file));

/*
   The following pointer must be defined in the
   main program and given an appropriate value
*/
extern const char *progname;

#include "config.h"

#ifndef	MFMODE
# define	MFMODE		0660		/* create mode for `/var/mail' files */
#endif

#endif /* __LIBMAIL_H */
