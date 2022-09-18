#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:i386/cmd/bkrs/libadmIO.d/makefile	1.6.8.3"
#ident	"$Header: makefile 1.2 91/05/23 $"

include $(CMDRULES)

SRC=libadmIO.c 
OBJ=$(SRC:.c=.o)

GINCSYS=$(ROOT)/$(MACH)/usr/include/sys
LOCAL=hdrs
GENERAL=../hdrs
SADMDIR=$(USRSADM)
BKBIN=$(SADMDIR)/bkup/bin
OAMBASE=$(SADMDIR)/sysadm
ETCBKUP=$(ETC)/bkup
LIBDIR=$(USRLIB)/libadmIO
LOCALINC=-I $(LOCAL) -I $(GENERAL)
LIBRARY = libadmIO.a
LDLIBS=$(LDLIBPATH)
LINTFLAGS=$(DEFLIST)
ARFLAGS=cr

PRODUCTS=libadmIO.a

all: $(PRODUCTS)

libadmIO.a: libadmIO.o
	$(AR) $(ARFLAGS) $(@) libadmIO.o

touch: 
	touch $(SRC)

clean:
	rm -f $(OBJ)

clobber: clean
	rm -f $(PRODUCTS)

lintit:
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(SRC)

strip:	$(PRODUCTS)
	$(STRIP) $(PRODUCTS)

install: $(PRODUCTS)
	$(INS) -f $(USRLIB) -m 644 $(LIBRARY)
	$(INS) -f $(GINCSYS) -m 444 $(GENERAL)/libadmIO.h

$(LIBRARY)(libadmIO.o):	$(INC)/sys/types.h \
			$(INC)/stdio.h \
			$(INC)/fcntl.h \
			$(INC)/sys/errno.h \
			$(INC)/signal.h \
			$(INC)/string.h \
			$(GENERAL)/libadmIO.h  \
			$(INC)/sys/stat.h \
			$(INC)/sys/statvfs.h \
			$(INC)/sys/mkdev.h \
			$(INC)/sys/stropts.h \
			$(INC)/malloc.h \
			$(INC)/memory.h 
