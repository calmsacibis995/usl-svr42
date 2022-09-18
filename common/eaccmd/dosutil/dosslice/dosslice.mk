#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)eac:common/eaccmd/dosutil/dosslice/dosslice.mk	1.1"

include $(CMDRULES)


OWN = bin
GRP = bin

DOSOBJECTS = $(CFILES:.c=.o)
CMDS = dosslice 

.MUTEX: $(CMDS)


all: $(CMDS)

CFILES= dosslice.c

dosslice: dosslice.o
	$(CC) -o dosslice dosslice.o

dosslice.o: dosslice.c \
	$(INC)/sys/types.h \
	$(INC)/stdio.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/vtoc.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/fdisk.h \
	$(INC)/fcntl.h \
	$(INC)/pwd.h

install: all
	 $(INS) -f $(USRBIN) -m 0711 -u $(OWN) -g $(GRP) dosslice
	
clean:
	rm -f *.o

clobber: clean
	rm -f $(CMDS)

lintit:
	$(LINT) $(LINTFLAGS) *.c
