#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86at:io/ws/ws.mk	1.11"
#ident	"$Header: $"

include $(UTSRULES)

KBASE    = ../..
CNFFILE  = $(CONF)/pack.d/ws/Driver.o
DFILES   = \
	ws_cmap.o \
	ws_ansi.o \
	ws_subr.o \
	ws_tcl.o \
	ws_tables.o \
	ws_8042.o

CFILES = $(DFILES:.o=.c)



all:	ID $(CNFFILE)

$(CNFFILE):	$(DFILES)
	$(LD) -r -o $@ $(DFILES)

#
# Configuration Section
#
ID:
	cd ws.cf; $(IDINSTALL) -R$(CONF) -M ws



#
# Header Install Section
#
headinstall:	\
	$(KBASE)/io/ws/8042.h \
	$(KBASE)/io/ws/chan.h \
	$(KBASE)/io/ws/tcl.h \
	$(KBASE)/io/ws/vt.h \
	$(KBASE)/io/ws/ws.h \
	$(FRC)
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/ws/vt.h
	$(INS) -f $(INC)/sys/ws -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/ws/8042.h
	$(INS) -f $(INC)/sys/ws -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/ws/chan.h
	$(INS) -f $(INC)/sys/ws -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/ws/tcl.h
	$(INS) -f $(INC)/sys/ws -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/ws/ws.h

clean:
	-rm -f *.o

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d ws


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

