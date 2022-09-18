#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/smtp/src/svr3.mk	1.4"
# "@(#)svr3.mk	1.4 'attmail mail(1) command'"
# To build the SMTP programs:
# For a System V Release 3 3B2 running WIN/3B 3.0 TCP/IP.
# For a System V Release 3.2 Vax, add -DARPA_INET_H to CFLAGS.
# System V Release 3 Flags
CFLAGS=		-O -v -DSVR3 -DSOCKET -DBIND -DSYS_INET_H -I../..
NETLIB=		-lnet -lnsl_s
LDFLAGS=
USR_SPOOL=	$(ROOT)/usr/spool
MAIL=		$(ROOT)/usr/mail
REALMAIL=	/usr/mail
REALSMTPQ=	/usr/spool/smtpq
USRLIB=		/usr/lib
INSMTPDMODES=	4555
INSMTPDOWN=	root
OWN=		smtp
GRP=		mail

USR_LIBMAIL=	$(USRLIB)/mail
MAILSURRCMD=	$(USR_LIBMAIL)/surrcmd
SMTPQ=		$(USR_SPOOL)/smtpq
REALMAILSURRCMD=	/usr/lib/mail/surrcmd
LOCALE=		$(USRLIB)/locale/C/MSGFILES
USEPRIV=
RANLIB=		:

include comm.mk
