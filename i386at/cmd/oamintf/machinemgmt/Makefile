#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:i386at/cmd/oamintf/machinemgmt/Makefile	1.4"
#ident "$Header: Makefile 1.1 91/08/28 $"

include $(CMDRULES)

.MUTEX: all install

INSDIR1	= $(USRSADM)/sysadm/bin
DIRS	= $(INSDIR1)

O_CFILES = diskcfg 
O_SHFILES=\
	chgstate
O_DFILES=\
	machine.menu \
	Form.power Text.power \
	Form.reboot Text.reboot \
	Text.whoson

HELPFILES=\
	Help \
	power.Help reboot.Help whoson.Help

HELPDEST=\
	machinemgmt/Help \
	power/Help reboot/Help whoson/Help

OWN = root
GRP = sys

.MUTEX:	 all xinstall

all: $(O_SHFILES) $(O_CFILES)

$(O_CFILES): diskcfg.o 
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f $(O_SHFILES) $(O_CFILES)

lintit:

size strip: all

$(DIRS):
	-mkdir -p $@

install: all $(DIRS) xinstall
	$(INS) -u $(OWN) -g $(GRP) -m 555 -f $(INSDIR1) diskcfg

PROTO=Prototype
OBJECTS=$(HELPFILES) $(O_SHFILES) $(O_DFILES)
OBJDEST=$(HELPDEST)
include ../Install
