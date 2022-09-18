#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)logins:logins.mk	1.13.11.2"
#ident "$Header: logins.mk 1.3 91/06/05 $"

include $(CMDRULES)


OWN = bin
GRP = bin

MAINS = logins

LDLIBS= -lia -liaf

all : $(MAINS) 

logins: logins.o
	$(CC) -o logins logins.o $(LDFLAGS) $(LDLIBS) $(NOSHLIBS)

logins.o: logins.c \
	$(INC)/sys/types.h \
	$(INC)/stdio.h \
	$(INC)/unistd.h $(INC)/sys/unistd.h \
	$(INC)/string.h \
	$(INC)/ctype.h \
	$(INC)/grp.h \
	$(INC)/pwd.h \
	$(INC)/shadow.h \
	$(INC)/time.h \
	$(INC)/varargs.h \
	$(INC)/ia.h \
	$(INC)/sys/time.h \
	$(INC)/mac.h $(INC)/sys/mac.h \
	$(INC)/sys/param.h \
	$(INC)/audit.h $(INC)/sys/audit.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/pfmt.h \
	$(INC)/locale.h

install: all 
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) logins

clean : 
	rm -f logins.o

clobber : clean
	rm -f $(MAINS)

lintit:
	$(LINT) $(LINTFLAGS) *.c
