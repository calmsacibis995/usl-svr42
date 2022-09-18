#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)autopush:i386/cmd/autopush/autopush.mk	1.10.5.3"
#ident "$Header: autopush.mk 1.4 91/06/05 $"
#
# auopush.mk:
# makefile for autopush(1M) command
#

include $(CMDRULES)
DIR = $(SBIN)
OWN=root
GRP=sys
INCSYS=$(INC)

all:	autopush

install:	all
		$(INS) -f $(DIR) -m 0555 -u $(OWN) -g $(GRP) autopush
		$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) autopush
		-$(SYMLINK) /sbin/autopush $(ETC)/autopush
		[ -d $(ETC)/ap ] || mkdir $(ETC)/ap
		$(CH)chmod 755 $(ETC)/ap
		$(CH)chgrp $(GRP) $(ETC)/ap
		$(CH)chown $(OWN) $(ETC)/ap
		$(INS) -f $(ETC)/ap -m 444 -u $(OWN) -g $(GRP) chan.ap

autopush:	autopush.o
		$(CC) -o autopush autopush.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

clean:
	-rm -f *.o

clobber: clean
	-rm -f autopush


autopush.o:	autopush.c \
		$(INCSYS)/sys/types.h \
		$(INCSYS)/sys/sad.h \
		$(INCSYS)/sys/conf.h \
		$(INC)/stdio.h \
		$(INC)/fcntl.h \
		$(INC)/errno.h \
		$(INC)/ctype.h \
		$(INC)/memory.h \
		$(INC)/priv.h
		$(CC) $(CFLAGS) $(DEFLIST) -c autopush.c 
