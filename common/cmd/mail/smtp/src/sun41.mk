#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/smtp/src/sun41.mk	1.5"
# "@(#)sun41.mk	1.6 'attmail mail(1) command'"
# To build the SMTP programs:
# For Sun/OS 4.1
# Sun OS 4.1 Flags
CC=		/usr/5bin/cc
CFLAGS=		-O -DSUN41 -DSOCKET -DBIND -DNETINET_IN_H -DARPA_INET_H -I../..
NETLIB=		-lresolv
LDFLAGS=
USR_SPOOL=	$(ROOT)/usr/spool
MAIL=		$(ROOT)/usr/mail
REALMAIL=	/var/spool/mail
REALSMTPQ=	/var/spool/smtpq
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
RANLIB=		ranlib

include comm.mk
