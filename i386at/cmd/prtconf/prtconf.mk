#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)prtconf:prtconf.mk	1.5.6.2"
#ident "$Header"

include $(CMDRULES)

INSDIR1	= $(USRSBIN)
DIRS	= $(INSDIR1)

O_CFILES = prtconf

OWN = root
GRP = sys

all: $(O_CFILES)

$(O_CFILES): prtconf.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(DEFLIST) -o $@ $@.c

clean:
	rm -f *.o

clobber: clean
	rm -f $(O_CFILES)

lintit:

size strip: all

$(DIRS):
	-mkdir -p $@

install: all $(DIRS)
	$(INS) -u $(OWN) -g $(GRP) -m 555 -f $(INSDIR1) prtconf
