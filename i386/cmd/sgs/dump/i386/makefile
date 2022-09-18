#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)dump:i386/makefile	1.7"

#
#
#	DUMP MAKEFILE
#
#
include $(CMDRULES)

LINTFLAGS=
LINK_MODE=

LDLIBS=

INS=$(SGSBASE)/sgs.install
INSDIR=$(CCSBIN)

HFILES=$(COMDIR)/dumpmap.h $(COMDIR)/dump.h $(COMDIR)/data32.h \
       $(CPUINC)/sgs.h $(CPUINC)/paths.h \
       $(COMINC)/libelf.h $(COMINC)/dwarf.h

SOURCES=$(COMDIR)/dump.c $(COMDIR)/fcns.c $(COMDIR)/debug.c 
OBJECTS=dump.o fcns.o debug.o 
PRODUCTS=dump

DEFLIST=-D$(ARFORMAT) -DELF_TARGET_ALL
INCLIST=-I$(CPUINC) -I$(COMINC)
CC_CMD=$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST)



all:	dump

dump: 	$(OBJECTS) $(LIBELF) $(LDLIBS)
	$(CC) $(OBJECTS) $(LIBELF) $(LDLIBS) $(LINK_MODE) -o dump

dump.o: 	$(HFILES) $(COMDIR)/dump.c
	$(CC_CMD) -c $(COMDIR)/dump.c 


fcns.o: $(HFILES) $(COMDIR)/fcns.c
	$(CC_CMD) -c $(COMDIR)/fcns.c

debug.o: $(HFILES) $(COMDIR)/debug.c
	$(CC_CMD) -c $(COMDIR)/debug.c


install:	$(INSDIR)/$(SGS)dump 

$(INSDIR)/$(SGS)dump:	dump
	cp dump dump.bak
	$(STRIP) dump
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(INSDIR)/$(SGS)dump dump
	mv dump.bak dump

lintit:	$(HFILES) $(SOURCES)
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $(SOURCES)

clean:
	-rm -f $(OBJECTS)

shrink clobber: clean
	-rm -f $(PRODUCTS)

