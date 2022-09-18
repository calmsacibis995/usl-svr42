#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/smtp/src/src.mk	1.23.3.7"
# "@(#)src.mk	1.30 'attmail mail(1) command'"
# To build the SMTP programs:
# For System V Release 4.1 DT (using inetd and the connection server)
# System V Release 4.1 DT Flags
#

# first, in case $CMDRULES is empty, we define
# the things that $CMDRULES is supposed to supply.
CFLAGS= -O $(LOCALDEF) $(LOCALINC)
USRLIB= /usr/lib
MBINDIR=/usr/bin
USRBIN= /usr/bin
ETC= /etc
VAR= /var
USRSHARE= /usr/share

include $(CMDRULES)

# System V Release 4-ES Flags
LOCALDEF=	-Xa -DSVR4 -DSVR4_1 -DTLI -DBIND -DNETINET_IN_H
LOCALINC=	-I../..
NETLIB=		$(LIBELF) -lresolv -lsocket -Bdynamic -lnsl -Bstatic -lelf -lc -Bdynamic
USR_SPOOL=	$(VAR)/spool
MAIL=		$(VAR)/mail
REALMAIL=	/var/mail
REALSMTPQ=	/var/spool/smtpq
INSMTPDMODES=	555
INSMTPDOWN=	root
OWN=		bin
GRP=		mail

USR_LIBMAIL=	$(USRLIB)/mail
MAILSURRCMD=	$(USR_LIBMAIL)/surrcmd
SMTPQ=		$(USR_SPOOL)/smtpq
REALMAILSURRCMD=	/usr/lib/mail/surrcmd
LOCALE=		$(USRLIB)/locale/C/MSGFILES
USEPRIV=	-DPRIV
RANLIB=		:

include comm.mk
