#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:net/transport/transport.mk	1.8"
#ident 	"$Header: $"

include $(UTSRULES)

KBASE = ../..
INSPERM  = -u $(OWN) -g $(GRP)
SOCKMOD = $(CONF)/pack.d/sockmod/Driver.o
TIMOD = $(CONF)/pack.d/timod/Driver.o
TIRDWR = $(CONF)/pack.d/tirdwr/Driver.o
NTTY = $(CONF)/pack.d/ntty/Driver.o

FILE = sockmod.o timod.o tirdwr.o ntty.o

CFILES = $(FILE:.o=.c)

all:	ID $(SOCKMOD) $(TIMOD) $(TIRDWR) $(NTTY)

ID:
	cd sockmod.cf; $(IDINSTALL) -R$(CONF) -M sockmod
	cd timod.cf;   $(IDINSTALL) -R$(CONF) -M timod
	cd tirdwr.cf;  $(IDINSTALL) -R$(CONF) -M tirdwr
	cd ntty.cf; $(IDINSTALL) -R$(CONF) -M ntty

$(SOCKMOD):	sockmod.o
	$(LD) -r -o $@ sockmod.o

$(TIMOD):	timod.o
	$(LD) -r -o $@ timod.o

$(TIRDWR):	tirdwr.o
	$(LD) -r -o $@ tirdwr.o

$(NTTY):	ntty.o
	$(LD) -r -o $@ ntty.o

clean:
	-rm -f *.o

clobber:	clean
	$(IDINSTALL) -e -R$(CONF) -d sockmod
	$(IDINSTALL) -e -R$(CONF) -d timod
	$(IDINSTALL) -e -R$(CONF) -d tirdwr
	$(IDINSTALL) -e -R$(CONF) -d ntty

headinstall: \
	$(KBASE)/net/transport/socket.h \
	$(KBASE)/net/transport/socketvar.h \
	$(KBASE)/net/transport/sockio.h \
	$(KBASE)/net/transport/sockmod.h \
	$(KBASE)/net/transport/timod.h \
	$(KBASE)/net/transport/tiuser.h \
	$(KBASE)/net/transport/un.h \
	$(FRC)
	$(INS) -f $(INC)/sys -m 644 $(INSPERM) $(KBASE)/net/transport/socket.h
	$(INS) -f $(INC)/sys -m 644 $(INSPERM) $(KBASE)/net/transport/socketvar.h
	$(INS) -f $(INC)/sys -m 644 $(INSPERM) $(KBASE)/net/transport/sockio.h
	$(INS) -f $(INC)/sys -m 644 $(INSPERM) $(KBASE)/net/transport/sockmod.h
	$(INS) -f $(INC)/sys -m 644 $(INSPERM) $(KBASE)/net/transport/timod.h
	$(INS) -f $(INC)/sys -m 644 $(INSPERM) $(KBASE)/net/transport/tiuser.h
	$(INS) -f $(INC)/sys -m 644 $(INSPERM) $(KBASE)/net/transport/un.h


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

