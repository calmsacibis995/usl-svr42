/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/smtp.h	1.6.2.2"
#ident "@(#)smtp.h	1.6 'attmail mail(1) command'"
/* smtp constants and the like */

/* tunable constants */
#define MAXSTR 10240			/* maximum string length */
#define NAMSIZ MAXSTR			/* max file name length */

typedef struct xnamelist namelist;
struct xnamelist {
	namelist *next;
	char *name;
};

/* spooling constants */
#define SMTP		"/smtp"
#define SMTPBATCH	"/smtpbatch"

/* For function prototypes */
#if defined(__STDC__)
#define	proto(x)	x
#else
#define	proto(x)	()
#endif

/* Need to prepend the program name to all perror() messages */
#define	perror(x)	xperror(x)

extern void xperror proto((char *));
