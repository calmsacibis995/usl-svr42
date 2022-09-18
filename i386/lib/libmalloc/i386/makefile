#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libmalloc:i386/makefile	1.4"
#
# makefile for libmalloc
#

include $(LIBRULES)

.SUFFIXES: .p
LIBP= $(CCSLIB)/libp
LINTFLAGS=-u -m -lmalloc
SGSBASE=../../../cmd/sgs
COMDIR=../common
SOURCES=$(COMDIR)/malloc.c
OBJECTS=malloc.o
POBJECTS=malloc.p
FRC=
INCLIST=-I. -I$(COMDIR) -I$(INC)

.MUTEX: noprof prof

all: noprof prof

noprof: $(OBJECTS)
	$(RM) -rf objects
	mkdir objects
	$(CP) $(OBJECTS) objects
	cd objects; $(AR) $(ARFLAGS) libmalloc.a *

prof: $(POBJECTS)
	$(RM) -rf pobjects
	mkdir pobjects
	for i in $(POBJECTS);\
	do\
		$(CP) $$i pobjects/`basename $$i .p`.o;\
	done
	cd pobjects; $(AR) $(ARFLAGS) libmalloc.a *

malloc.o malloc.p: $(INC)/malloc.h $(COMDIR)/mallint.h \
		   $(COMDIR)/malloc.c machdep.h

malloc.o:	
	@echo malloc.c:
	$(CC) $(CFLAGS) -c $(INCLIST) $(COMDIR)/malloc.c

malloc.p:
	@echo malloc.c:
	$(PROF)$(CC) -p $(CFLAGS) -c $(INCLIST) $(COMDIR)/malloc.c  \
	&& $(MV) malloc.o malloc.p

lintit: $(SOURCES)
	$(LINT) $(LINTFLAGS) $(INCLIST) $(SOURCES)

install: all
	if [ ! -d $(LIBP) ];\
	then\
		mkdir $(LIBP);\
	fi
	sh $(SGSBASE)/sgs.install 644 $(OWN) $(GRP) $(CCSLIB)/libmalloc.a objects/libmalloc.a
	sh $(SGSBASE)/sgs.install 644 $(OWN) $(GRP) $(LIBP)/libmalloc.a pobjects/libmalloc.a

clean: 
	$(RM) -fr pobjects objects

clobber: clean
	$(RM) -f $(POBJECTS) $(OBJECTS)
