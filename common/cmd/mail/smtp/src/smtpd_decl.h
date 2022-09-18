/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/smtpd_decl.h	1.1.2.3"
#ident "@(#)smtpd_decl.h	1.2 'attmail mail(1) command'"

/* Function prototypes for the smtpd process */
extern char *convertaddr proto((char *));
extern int alarmsend proto(());
extern int chmod proto((char *, int));
extern int cmdparse proto((char *, int));
extern int death proto((int));
extern int do_mail proto((FILE *, FILE *));
extern int do_rcpt proto((FILE *, FILE *));
extern int doit proto((int, int));
extern int gotodir proto((char *));
extern int init_xfr proto(());
extern int mkctlfile proto((char, char *, char *));
extern int mkdatafile proto((char *));
extern int mkdir proto((char *, int));
extern int parse_rcpt proto((char *, int));
extern int shellchars proto((char *));
extern int tgets proto((char *, int, FILE *));
extern int tputs proto((char *, FILE *));
extern int wait proto((int *));
extern long ulimit proto((int, long));
extern void bitch proto((char *, FILE *));
extern void converse proto((FILE *, FILE *, int));
extern void do_data proto((FILE *, FILE *));
extern void do_helo proto((FILE *, FILE *));
extern void from822 proto((char *, char *(*)(), FILE *, FILE *, char *));
extern void process proto((void));
extern void quit proto((FILE *, FILE *));
extern void smtplog proto((char *));
extern void smtpsched proto((char *, char *, int));
extern void syslog proto((int, ...));
#ifndef SUN41
extern int umask proto((int));
#endif
