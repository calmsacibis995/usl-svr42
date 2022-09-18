#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)eac:i386/eaccmd/pcfont/pcfont.mk	1.1"
#ident  "$Header: $" 

include $(CMDRULES)

#

OWN = bin
GRP = bin

DIRS = $(USRBIN)

OBJ=pcfont.o loadfont.o

all: pcfont

install: $(DIRS) all
	 $(INS) -f $(USRBIN) -m 0711 -u $(OWN) -g $(GRP) pcfont

$(DIRS):
	- [ -d $@ ] || mkdir -p $@ ;\
		$(CH)chmod 0755 $@ ;\
		$(CH)chown $(OWN) $@ ;\
		$(CH)chgrp $(GRP) $@

clean: 
	rm -f $(OBJ)

clobber: clean
	rm -f pcfont 

lintit:
	$(LINT) $(LINTFLAGS) pcfont.c loadfont.c

pcfont: $(OBJ)
	$(CC) -o pcfont $(OBJ) $(LDFLAGS) $(LDLIBS) $(SHLIBS) 

pcfont.o:	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/ctype.h \
	$(INC)/sys/types.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/at_ansi.h \
	$(INC)/sys/kd.h \
	pcfont.h

laodfont.o:	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/ctype.h \
	$(INC)/errno.h \
	$(INC)/sys/types.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/at_ansi.h \
	$(INC)/sys/kd.h \
	pcfont.h
