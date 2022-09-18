/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/smtp_decl.h	1.2.2.2"
#ident "@(#)smtp_decl.h	1.2 'attmail mail(1) command'"

#ifndef	SIGRETURN
#include "xmail.h"
#endif

/* Function prototypes for the smtp process */
#ifdef TLI
	/* This is for the mxnetdir proto below */
#include <netconfig.h>
#include <netdir.h>
#endif
extern SIGRETURN death proto((int));
extern char * convertaddr proto((char *, char *, int));
extern char * convertto proto((char *, int, char *));
extern char * fileoftype proto((char, char *));
extern int mxnetdir proto((struct netconfig *, struct nd_hostserv *, struct nd_addrlist **));
extern int tgets proto((char *, int, FILE *));
extern int to822 proto((int (*)(), FILE *, FILE *, char *, char *, char *));
extern int tputs proto((char *, FILE *));
extern namelist * appendname proto((namelist *, char *));
extern namelist * newname proto((char *));
extern void addhostdom proto((string *, char *, char *));
extern void bomb proto((int));
extern void converse proto((int, char *, namelist *, char *, FILE *, FILE *, FILE *));
extern void do_data proto((int, FILE *, FILE *, char *, namelist *, char *));
extern void donext proto((int *, char **, namelist **, char **, FILE *, char **, char **));
extern void expect proto((int, FILE *, FILE *));
extern void init_batch proto((int, char **));
extern void rm_file proto((void));
extern void setalarm proto((int, char *));
extern void setup proto((char *, FILE **, FILE **));
extern void setupargs proto((int, char **, int *, char **, namelist **, char **, char **, char **));
extern void t_log proto((char *));
