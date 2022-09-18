#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sgs-cmd:i386/makefile	1.1.1.9"

include $(CMDRULES)

INS=$(SGSBASE)/sgs.install
INSDIR=$(CCSBIN)

HFILES=machdep.h $(COMDIR)/cc.h $(CPUINC)/paths.h $(CPUINC)/sgs.h

SOURCES=$(COMDIR)/cc.c ./machdep.c

OBJECTS=cc.o machdep.o

PRODUCTS=cc 

DEFLIST=-D$(ARCH) -D$(CPU)

INCLIST=-I. -I$(COMDIR) -I$(CPUINC) -I$(INC) -I$(COMINC)

CC_CMD=$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST)

all:	$(PRODUCTS)
	
$(PRODUCTS): 	$(OBJECTS)
	$(CC) $(OBJECTS) $(LINK_MODE) -o cc

cc.o: 	$(HFILES) $(COMDIR)/cc.c
	$(CC_CMD) -c $(COMDIR)/cc.c 

machdep.o:	$(HFILES) ./machdep.c
	$(CC_CMD) -c ./machdep.c

install:$(INSDIR)/$(SGS)cc 

$(INSDIR)/$(SGS)cc:cc
	cp cc cc.bak
	$(STRIP) cc
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(INSDIR)/$(SGS)cc cc
	mv cc.bak cc

lintit:	$(HFILES) $(SOURCES)
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $(SOURCES)

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(PRODUCTS)
