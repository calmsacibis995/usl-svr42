#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/smtp/src/svr4.mk	1.5"
# "@(#)svr4.mk	1.5 'attmail mail(1) command'"
# To build the SMTP programs:
# For a `standard' System V Release 4 (using TLI and netdir):
# System V Release 4 Flags
CFLAGS=		-O -v -Xa -DSVR4 -DTLI -DBIND -DNETINET_IN_H -I../..
NETLIB=		-lresolv -lsocket -Bdynamic -lnsl -Bstatic -lelf -lc -Bdynamic
LDFLAGS=	
USR_SPOOL=	$(ROOT)/var/spool
MAIL=		$(ROOT)/var/mail
REALMAIL=	/var/mail
REALSMTPQ=	/var/spool/smtpq
USRLIB=		/usr/lib
INS=		install
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
