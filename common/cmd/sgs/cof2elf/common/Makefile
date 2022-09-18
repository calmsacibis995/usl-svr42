#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cof2elf:common/Makefile	1.2"

INS		= $(SGSBASE)/sgs.install

SOURCES		= \
		$(COMDIR)/cof2elf.c

OBJECTS		= \
		cof2elf.o

PRODUCTS	= cof2elf

DEFLIST		= -DPORTAR

INCLIST		= -I$(CPUINC) -I$(COMINC) -I$(COMDIR) -I./

CC_CMD		= $(CC) $(CFLAGS) $(DEFLIST) $(INCLIST)
COM_CC_CMD	= $(CC_CMD) -c $(COMDIR)/$(@:.o=.c)
SGS_CC_CMD	= $(CC_CMD) -c $(@:.o=.c)

all:		$(PRODUCTS)

install:	all
	cp cof2elf cof2elf.bak
	$(STRIP) cof2elf
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(CCSBIN)/$(SGS)cof2elf cof2elf
	mv cof2elf.bak cof2elf

lintit:		$(SOURCES)
	$(LINT) $(LINTFLAGS) $(INCLIST) $(SOURCES) $(LIBELF)

clean:
	-rm -f $(OBJECTS)

shrink:		clean
	-rm -f $(PRODUCTS) 

clobber: shrink

cof2elf: $(OBJECTS) 
	$(CC_CMD) $(LINK_MODE) -o $@ $(OBJECTS) $(LIBELF) $(LDLIBS)


cof2elf.o:	$(COMDIR)/cof2elf.c
	$(COM_CC_CMD)
