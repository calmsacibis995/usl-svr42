#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)eac:i386/eaccmd/mapkey/mapkey.mk	1.3.1.1"

include	$(CMDRULES)

DIR	= $(BIN)
INSDIR	= $(USRBIN)
INSDIR1	= $(USRLIB)
INSDIR4	= $(USRLIB)/keyboard

EXES	= mapkey mapstr 
SRCS	= mapkey.c mapstr.c mapio.c
OBJS =   mapkey.o mapstr.o mapio.o
FLATFILE = keys strings scomap scostrings

all:	$(EXES) $(FLATFILE)

$(INSDIR) $(INSDIR1)   $(INSDIR4):
	-mkdir $@
	-$(CH) chmod 755 $@
	-$(CH) chown bin $@
	-$(CH) chgrp bin $@

install: $(INSDIR) $(INSDIR1)   $(INSDIR4)	all
	$(INS) -f $(INSDIR) -m 0711 -u bin -g bin mapkey
	$(INS) -f $(INSDIR) -m 0711 -u bin -g bin mapstr
	-@ for i in $(FLATFILE) ; do \
		rm -f $(INSDIR4)/$$i ;\
		if [ -f $$i ] ; then \
			cat $$i | sed -e '/ident/d' > $(INSDIR4)/$$i ;\
			$(CH)chmod 644 $(INSDIR4)/$$i ;\
			$(CH)chgrp bin $(INSDIR4)/$$i ;\
			$(CH)chown bin $(INSDIR4)/$$i ;\
		fi ;\
	done 
clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(EXES)

$(OBJS):	$(FRC)

FRC:

mapkey:		mapkey.o  mapio.o
	$(CC) $(CFLAGS) -o mapkey mapkey.o mapio.o $(LDFLAGS)

mapstr:		mapstr.o 
	$(CC) $(CFLAGS) -o mapstr mapstr.o $(LDFLAGS)

