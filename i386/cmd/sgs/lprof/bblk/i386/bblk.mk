#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lprof:bblk/i386/bblk.mk	1.3"

include $(CMDRULES)

PROF_SAVE	=
XPROF_INCS	=
SRCBASE		= common

LINTFLAGS 	=
LINK_MODE	=

BBLKBASE	= ..
BCPUDIR		= $(BBLKBASE)/$(CPU)
COMDIR		= $(BBLKBASE)/common
SGSBASE		= ../../..
CMDBASE		= ../../../..
LPBASE		= $(SGSBASE)/lprof
PLBBASE		= $(LPBASE)/libprof
INCBASE		= $(LPBASE)/hdr
INS		= $(SGSBASE)/sgs.install
INSDIR		= $(CCSBIN)
HFILES		= $(COMDIR)/comdef.h $(COMDIR)/comext.h $(BCPUDIR)/macdef.h
INCDIRS		= \
		-I $(BCPUDIR) \
		-I $(COMDIR) \
		-I $(SGSBASE)/inc/$(CPU) \
		-I $(SGSBASE)/inc/common \
		-I $(INCBASE) \
		-I $(PLBBASE)/$(CPU) \
		$(XPROF_INCS)
COMMSOURCES	= $(COMDIR)/comglb.c
MACHSOURCES	= $(BCPUDIR)/macglb.c $(BCPUDIR)/mac.c $(BCPUDIR)/parse.c
OBJECTS		= comglb.o macglb.o mac.o parse.o
PRODUCTS	= ../../basicblk

LIBELF		= $(SGSBASE)/libelf/$(CPU)/libelf.a
PROFLIBD	= $(PLBBASE)
#LIBSYMINT	= -L$(PROFLIBD) -lsymint
LDFLAGS		=
LIBS		= $(LIBELF)

all: $(PRODUCTS)

$(PRODUCTS): $(OBJECTS)
	$(CC) -o $(PRODUCTS) $(CFLAGS) $(OBJECTS) \
	$(LDFLAGS) $(LIBS) $(LDLIBS) $(LINK_MODE)

comglb.o: $(HFILES) $(COMDIR)/comglb.c
	$(CC) $(CFLAGS) -c $(INCDIRS) $(COMDIR)/comglb.c

mac.o: $(HFILES) $(BCPUDIR)/mac.c
	$(CC) $(CFLAGS) -c $(INCDIRS) $(BCPUDIR)/mac.c

macglb.o: $(HFILES) $(BCPUDIR)/macglb.c
	$(CC) $(CFLAGS) -c $(INCDIRS) $(BCPUDIR)/macglb.c

parse.o: $(HFILES) $(BCPUDIR)/parse.c
	$(CC) $(CFLAGS) $(INCDIRS) -c $(BCPUDIR)/parse.c

install: all
	cp $(PRODUCTS) basicblk.bak
	$(STRIP) $(PRODUCTS)
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(CCSLIB)/$(SGS)basicblk $(PRODUCTS)
	mv basicblk.bak $(PRODUCTS)

lintit:
	$(LINT) $(LINTFLAGS) $(INCDIRS) $(SOURCES)
	$(LINT) $(LINTFLAGS) -c $(INCDIRS) $(MACHSOURCES)

clean:
	rm -f *.o

clobber: clean
	rm -f $(PRODUCTS)
