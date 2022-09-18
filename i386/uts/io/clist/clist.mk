#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:io/clist/clist.mk	1.9"
#ident "$Header: clist.mk 1.2 91/03/20 $"

include $(UTSRULES)

KBASE    = ../..
CLIST 	 = $(CONF)/pack.d/clist/Driver.o
MODSTUB  = $(CONF)/pack.d/clist/Modstub.o
OFILES 	 = \
	clist.o \
	tty.o \
	tt1.o

CLIST2 	 = $(CONF)/pack.d/clist_gd/Driver.o
OFILES2	 = \
	clist_gd.o \
	partab.o

CFILES =  \
	clist.c \
	tty.c \
	tt1.c \
	clist_gd.c \
	partab.c

all:	ID $(CLIST) $(CLIST2) $(MODSTUB)

$(CLIST): $(OFILES)
	$(LD) -r -o $@  $(OFILES)

$(CLIST2): $(OFILES2)
	$(LD) -r -o $@  $(OFILES2)

$(MODSTUB): clist_stub.o
	$(LD) -r -o $@ clist_stub.o

#
# Configuration Section
#
ID:
	cd clist.cf; $(IDINSTALL) -R$(CONF) -M clist
	cd clist_gd.cf; $(IDINSTALL) -R$(CONF) -M clist_gd

headinstall:

clean:
	-rm -f $(OFILES) clist_stub.o
	-rm -f $(OFILES2)

clobber:	clean
	-$(IDINSTALL) -e -R$(CONF) -d clist
	-$(IDINSTALL) -e -R$(CONF) -d clist_gd


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

