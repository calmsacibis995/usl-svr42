#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)file:common/cmd/file/file.mk	1.11.1.2"
#ident "$Header: file.mk 1.2 91/04/09 $"
#

include $(CMDRULES)

INSDIR = $(USRBIN)
MINSDIR = $(ETC)
OWN = bin
GRP = bin

LDLIBS= -lcmd  -lw 

all:	file

file:	file.c
	$(CC) $(CFLAGS) $(DEFLIST) file.c -o $@ $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) file
	$(INS) -f $(MINSDIR) -m 0444 -u $(OWN) -g $(GRP) magic

clean:
	-rm -f file.o

clobber: clean
	-rm -f file
