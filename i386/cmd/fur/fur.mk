#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)fur:i386/cmd/fur/fur.mk	1.3"
#ident	"$Header: $"

include	$(CMDRULES)

INSDIR = $(USRBIN)

OBJS = rel.o fill.o fur.o

INS = ../sgs/sgs.install

LELF = ../sgs/libelf/$(CPU)/$(LIBELF)
CINC = ../sgs/inc/$(CPU)/$(COMINC)

all:	$(SGS)fur

install: all $(INSDIR)
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(INSDIR)/$(SGS)fur $(SGS)fur

$(INSDIR):
	-mkdir -p $@

$(SGS)fur: $(OBJS)
	$(CC) -I$(CINC) $(LDFLAGS) $(ROOTLIBS) -o $(SGS)fur $(OBJS) $(LELF)

fill.o: fill.c 

rel.o: rel.c \
       $(INC)/libelf.h \
       $(INC)/sys/elf_386.h \
       ./fur.h 
	$(CC) $(CFLAGS) -I$(CINC) $(DEFLIST) -c rel.c

fur.o: fur.c \
	$(INC)/fcntl.h \
	$(INC)/errno.h \
	$(INC)/stdlib.h \
	$(INC)/stdarg.h \
	$(INC)/string.h \
	$(INC)/stdio.h \
	$(INC)/libelf.h \
	./fur.h 
	$(CC) $(CFLAGS) -I$(CINC) $(DEFLIST) -c fur.c


clean:
	-rm -f *.o

clobber: clean
	-rm -f $(SGS)fur
