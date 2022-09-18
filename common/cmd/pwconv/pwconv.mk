#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)pwconv:pwconv.mk	1.8.6.2"
#ident "$Header: pwconv.mk 1.2 91/04/15 $"

include $(CMDRULES)


OWN = root
GRP = sys

all: pwconv

pwconv: pwconv.o 
	$(CC) -o pwconv pwconv.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

pwconv.o: pwconv.c \
	$(INC)/pwd.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/time.h \
	$(INC)/shadow.h \
	$(INC)/grp.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/mac.h \
	$(INC)/priv.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h

install: pwconv 
	 rm -f $(USRBIN)/pwconv
	 $(INS) -f $(USRSBIN) -m 0500 -u $(OWN) -g $(GRP) pwconv
	 $(SYMLINK) /usr/sbin/pwconv $(USRBIN)/pwconv

clean:
	rm -f pwconv.o
	
clobber: clean
	rm -f pwconv

lintit:
	$(LINT) $(LINTFLAGS) pwconv.c

#	these targets are optional but useful

partslist:
	@echo pwconv.mk $(LOCALINCS) pwconv.c | tr ' ' '\012' | sort

product:
	@echo pwconv | tr ' ' '\012' | \
	sed -e 's;^;$(USRSBIN)/;' -e 's;//*;/;g'

productdir:
	@echo $(USRSBIN)
