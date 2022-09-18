#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)idtools:i386at/ktool/unixsyms/unixsyms.mk	1.8"
#ident	"$Header: $"

include	$(CMDRULES)

HOSTINC = /usr/include

INSDIR = $(CONF)/bin

CMDS = unixsyms
XCMDS = $(PFX)unixsyms
XTARGS = xenv_unixsyms

FILES = unixsyms.c addsym.c

XENV_OBJS = xenv_unixsyms.o xenv_addsym.o

SYS_DIR = ./sys
SYS_ELF = ./sys/elf.h
LIBELF = ./libelf.h

.MUTEX:	unixsyms xenv_unixsyms

all:	$(CMDS) xenv

xenv:	$(XTARGS)

install: all $(INSDIR) xenv_install
	for cmd in $(CMDS) ; do \
	$(INS) -f $(INSDIR) $$cmd ;\
	done

xenv_install: xenv $(INSDIR)
	if [ "$(PFX)" ]; then \
	for xcmd in $(XCMDS) ; do \
	$(INS) -f $(INSDIR) $$xcmd ;\
	done ;\
	fi

$(INSDIR):
	-mkdir -p $@

unixsyms: $(FILES) \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/sys/fcntl.h \
	$(INC)/errno.h \
	$(INC)/sys/errno.h \
	$(INC)/malloc.h \
	$(INC)/libelf.h \
	$(LIBELF)
	$(CC) -I. -I$(INC) $(CFLAGS) $(MORECPP) $(LDFLAGS) $(ROOTLIBS) \
		-o unixsyms $(FILES) -lelf

xenv_unixsyms: $(XENV_OBJS) 
	if [ "$(PFX)" ]; then \
		$(HCC) -I$(HOSTINC) -I$(INC) $(CFLAGS) $(MORECPP) $(LDFLAGS) \
			-o $(PFX)unixsyms $(XENV_OBJS) \
			$(TOOLS)/usr/ccs/lib/libelf$(PFX).a; \
		touch xenv_unixsyms; \
	fi

xenv_addsym.o: addsym.c \
	$(INC)/libelf.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/param.h 
	$(HCC) -I$(INC) $(CFLAGS) $(MORECPP) -c addsym.c &&  \
	mv addsym.o xenv_addsym.o

xenv_unixsyms.o: unixsyms.c \
	$(INC)/libelf.h \
	$(LIBELF) \
	$(HOSTINC)/stdio.h \
	$(HOSTINC)/fcntl.h \
	$(HOSTINC)/malloc.h \
	$(HOSTINC)/string.h
	$(HCC) -I. -I$(HOSTINC) -I$(INC) $(CFLAGS) $(MORECPP) -c unixsyms.c && \
	mv unixsyms.o xenv_unixsyms.o


clean:
	-rm -f *.o $(XTARGS) $(LIBELF)
	-rm -rf sys

clobber:	clean
	-rm -f $(CMDS) $(XCMDS)

$(SYS_ELF): $(INC)/sys/elf.h
	[ -d $(SYS_DIR) ] || mkdir $(SYS_DIR)
	cp $(INC)/sys/elf.h sys/elf.h
	
$(LIBELF): $(SYS_ELF) $(INC)/libelf.h
	cp $(INC)/libelf.h libelf.h

FRC:
