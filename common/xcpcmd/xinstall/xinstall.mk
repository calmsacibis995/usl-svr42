#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)xcpxinstall:common/xcpcmd/xinstall/xinstall.mk	1.3.2.4"
#ident  "$Header: xinstall.mk 1.2 91/07/11 $"

include $(CMDRULES)


OWN = bin
GRP = bin

DIRS = $(SBIN) $(USRLIB)/custom $(ETC)/perms
FILES = xinstall fixperm fixshlib custom help inst

all: $(FILES)

fixperm: fixpermR4.o
	$(CC) -o fixperm fixpermR4.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

fixshlib: fixshlib.o
	$(CC) -o fixshlib fixshlib.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

xinstall: xinstall.sh
	cp xinstall.sh xinstall

custom: customR4.sh
	cp customR4.sh custom

help: helpR4.src
	cp helpR4.src help

inst:
	cp instR4.perm inst

fixpermR4.o: fixpermR4.c \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/ctype.h \
	$(INC)/sys/types.h \
	$(INC)/a.out.h \
	$(INC)/ar.h \
	$(INC)/pwd.h \
	$(INC)/grp.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/sysmacros.h

fixshlib.o: fixshlib.c \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/ctype.h \
	$(INC)/fcntl.h \
	$(INC)/unistd.h \
	$(INC)/sys/types.h

$(DIRS):
	[ -d $@ ] || mkdir -p $@ ;\
		$(CH)chmod 0755 $@ ;\
		$(CH)chown $(OWN) $@ ;\
		$(CH)chgrp $(GRP) $@

install: $(DIRS) all
	$(INS) -f $(ETC)/perms inst
	$(INS) -f $(SBIN) xinstall
	$(INS) -f $(USRSBIN) fixperm
	$(INS) -f $(USRSBIN) fixshlib
	$(INS) -f $(SBIN) custom
	$(INS) -f $(USRLIB)/custom help

clean:
	-rm -f *.o 

clobber: clean
	-rm -f $(FILES)

lintit:
	$(LINT) $(LINTFLAGS) *.c

