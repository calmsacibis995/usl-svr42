#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)tar:common/cmd/tar/tar.mk	1.4.7.5"
#ident  "$Header: tar.mk 1.3 91/07/01 $"

include $(CMDRULES)

LDLIBS = -lcmd

all: tar

tar: tar.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(NOSHLIBS) $(LDLIBS) 

install: all
	-rm -rf $(ETC)/tar
	$(INS) -f $(USRSBIN) -m 0555 -u bin -g bin tar
	-$(SYMLINK) /usr/sbin/tar $(ETC)/tar
	-mkdir ./tmp
	-$(CP) tar.dfl ./tmp/tar
	$(INS) -f $(ETC)/default -m 0444 -u root -g sys ./tmp/tar
	-rm -rf ./tmp

clean:
	rm -f *.o

clobber: clean
	rm -f tar

lintit:
	$(LINT) $(LINTFLAGS) tar.c
