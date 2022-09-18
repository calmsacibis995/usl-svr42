#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/smtp/src/comm.mk	1.6"
# "@(#)comm.mk	1.6 'attmail mail(1) command'"
# To build the SMTP programs:
# This is a common smtp makefile used by src.mk, svr3.mk, svr4.mk, sun41.mk

STRIP=		$(PFX)strip
INS=		install

SMTPLIB=	smtplib.a
SMTPLIBSRC=     aux.c config.c dup2.c extra.c from822.c from822ad.c \
		header.c mail.c mx.c netio.c qlib.c regcomp.c \
		regerror.c regexec.c rewrite.c s5sysname.c smtplog.c to822.c \
		xperror.c
SMTPLIBOBJS=	$(SMTPLIBSRC:.c=.o)

LIBMAIL=	$(SMTPLIB) ../../libmail.a

SMTPQER=	smtpqer
SMTPQEROBJ=	smtpqer.o

SMTPSCHED=	smtpsched
SMTPSCHEDOBJ=	smtpsched.o batch.o

SMTP=		smtp
SMTPOBJ=	smtp.o converse.o donext.o smtpaux.o to822addr.o

SMTPD=		smtpd
SMTPDOBJ=	smtpd.o conversed.o

INSMTPD=	in.smtpd
INSMTPDOBJ=	in.smtpd.o conversed.o privnetio.o

SMTPLOOP=	smtploop
SMTPLOOPOBJ=	smtploop.o

OBJS=		$(SMTP) $(SMTPD) $(SMTPQER) $(SMTPSCHED) $(SMTPLOOP) $(INSMTPD)

.MUTEX:	$(SMTPD) $(INSMTPD)

all:	$(OBJS) smtp.str

config.c:
	rm -f config.c
	echo 'char *UPASROOT = "$(REALMAILSURRCMD)/";' >> config.c
	echo 'char *MAILROOT = "$(REALMAIL)/";' >> config.c
	echo 'char *SMTPQROOT = "$(REALSMTPQ)/";' >> config.c

$(SMTPLIB): $(SMTPLIBOBJS)
	$(AR) $(ARFLAGS) $(SMTPLIB) $?
	$(RANLIB) $(SMTPLIB)

$(SMTP): $(LIBMAIL) $(SMTPOBJ)
	$(CC) $(LDFLAGS) -o $(SMTP) $(SMTPOBJ) $(LIBMAIL) $(NETLIB)

$(SMTPD): $(LIBMAIL) $(SMTPDOBJ)
	$(CC) $(LDFLAGS) -o $(SMTPD) $(SMTPDOBJ) $(LIBMAIL) $(NETLIB)

$(SMTPQER): $(LIBMAIL) $(SMTPQEROBJ)
	$(CC) $(LDFLAGS) -o $(SMTPQER) $(SMTPQEROBJ) $(LIBMAIL) $(NETLIB)

$(SMTPSCHED): $(LIBMAIL) $(SMTPSCHEDOBJ)
	$(CC) $(LDFLAGS) -o $(SMTPSCHED) $(SMTPSCHEDOBJ) $(LIBMAIL) $(NETLIB)

$(SMTPLOOP): $(SMTPLOOPOBJ) ../../libmail.a
	$(CC) $(LDFLAGS) -o $(SMTPLOOP) $(SMTPLOOPOBJ) ../../libmail.a

in.smtpd.o: smtpd.c
	$(CC) -c $(CFLAGS) $(DEFLIST) -DSOCKET -DINETD $(USEPRIV) smtpd.c
	mv smtpd.o in.smtpd.o

privnetio.o: netio.c
	$(CC) -c $(CFLAGS) $(DEFLIST) $(USEPRIV) netio.c
	mv netio.o privnetio.o

$(INSMTPD): $(INSMTPDOBJ)
	$(CC) $(LDFLAGS) -o $(INSMTPD) $(INSMTPDOBJ) $(LIBMAIL) $(NETLIB)

$(USR_LIBMAIL):
	[ -d $@ ] || mkdir -p $@
	$(CH)chmod 775 $@
	$(CH)chown root $@
	$(CH)chgrp mail $@

$(MAILSURRCMD): $(USR_LIBMAIL)
	[ -d $@ ] || mkdir -p $@
	$(CH)chmod 775 $@
	$(CH)chown root $@
	$(CH)chgrp mail $@

$(MAIL):
	[ -d $@ ] || mkdir -p $@
	$(CH)chmod 775 $@
	$(CH)chown root $@
	$(CH)chgrp mail $@

$(SMTPQ):
	[ -d $@ ] || mkdir -p $@
	$(CH)chmod 775 $@
	$(CH)chown smtp $@
	$(CH)chgrp mail $@

install: all $(MAILSURRCMD) $(MAIL) $(SMTPQ)
	$(INS) -f $(MAILSURRCMD) -m $(INSMTPDMODES) -u $(INSMTPDOWN) -g $(GRP) ./$(INSMTPD)
	echo  $(MAILSURRCMD)/$(INSMTPD) requires dev and setuid privileges
	$(INS) -f $(MAILSURRCMD) -m 2555 -u $(OWN) -g $(GRP) ./$(SMTP)
	$(INS) -f $(MAILSURRCMD) -m 0555 -u $(OWN) -g $(GRP) ./$(SMTPD)
	$(INS) -f $(MAILSURRCMD) -m 0555 -u $(OWN) -g $(GRP) ./$(SMTPLOOP)
	$(INS) -f $(MAILSURRCMD) -m 2555 -u $(OWN) -g $(GRP) ./$(SMTPQER)
	$(INS) -f $(MAILSURRCMD) -m 2555 -u $(OWN) -g $(GRP) ./$(SMTPSCHED)
	$(INS) -f $(LOCALE)      -m 0555 -u $(OWN) -g $(GRP) ./smtp.str

smtp.str: pfmt.msgs
	sed -e 's/^:[0-9]*://' < pfmt.msgs > smtp.str

clean:
	rm -f *.o core config.c

clobber: clean
	rm -f $(SMTPLIB) $(OBJS) smtp.str

strip:	$(OBJS)
	$(STRIP) $(OBJS)
