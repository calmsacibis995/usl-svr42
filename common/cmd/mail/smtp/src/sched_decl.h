/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/sched_decl.h	1.2.2.2"
#ident "@(#)sched_decl.h	1.4 'attmail mail(1) command'"

extern FILE *parseline1 proto((char *));
extern char *fileoftype proto((char, char *));
extern int checkage proto((char *));
extern int docmd proto((char *, char **));
extern int dodir proto((char *, int));
extern int dofile proto((char *, char *));
extern int doremove proto((char *));
extern int dormail proto((char *, char *));
extern int dosmtp proto((char *, char *));
extern int inconsistent proto((char *));
extern int lock proto((char *));
extern int returnmail proto((char *, int));
extern int toomany proto((int));
extern void addtobatch proto((char *, int));
extern void dobatch proto((void));
extern void log proto((char *, ...));
extern void smtplog proto((char *));
extern void unlock proto((char *));

#ifndef _STDLIB_H
extern int atoi proto((char *));
extern char *malloc proto((unsigned));
extern void free proto((char *));
#endif
