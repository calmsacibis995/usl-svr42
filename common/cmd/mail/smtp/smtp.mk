#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/smtp/smtp.mk	1.4.3.4"
#ident "@(#)smtp.mk	1.6 'attmail mail(1) command'"
SMSRCMAKE=src.mk
USR_LIB=/usr/lib
MAILSURRCMD=$(USR_LIB)/mail/surrcmd

include $(CMDRULES)

smtp: all

all install clean clobber strip lintit:
	@echo '\t( cd src;'; cd src; $(MAKE) -f $(SMSRCMAKE) CMDRULES=$(CMDRULES) $@; echo '\t)'
