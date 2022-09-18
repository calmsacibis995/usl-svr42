#ident	"@(#)ucb:common/ucblib/libmp/libmp.mk	1.2"
#ident	"$Header: $"
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.



#
# makefile for ucblibdbm.a
#
#

include $(LIBRULES)

PROF=
NONPROF=
INC1=$(ROOT)/$(MACH)/usr/ucbinclude
INCSYS=$(INC)/sys
INSDIR=$(ROOT)/$(MACH)/usr/ucblib
ARFLAGS = cr
LOCALINC = -I$(INC1)
LIBRARY = libmp.a


OBJECTS = gcd.o madd.o mdiv.o mout.o msqrt.o mult.o pow.o util.o

SOURCES =  gcd.c madd.c mdiv.c mout.c msqrt.c mult.c pow.c util.c

ALL:	$(LIBRARY)
	$(LORDER) *.o|$(TSORT) >objlist
	$(AR) $(ARFLAGS) libmp.a `cat objlist`

all:	ALL

$(LIBRARY):	$(OBJECTS)

gcd.o:	$(INC1)/mp.h

madd.o:	$(INC1)/mp.h

mdiv.o:	$(INC1)/mp.h \
	$(INC)/stdio.h

mout.o:	$(INC1)/mp.h \
	$(INC)/stdio.h

msqrt.o:	$(INC1)/mp.h

mult.o:	$(INC1)/mp.h


pow.o:	$(INC1)/mp.h

util.o:	$(INC1)/mp.h \
	$(INC)/stdio.h

GLOBALINCS = $(INC1)/mp.h $(INC)/stdio.h

install: ALL
	$(INS) -f $(INSDIR) -m 644 $(LIBRARY)

clean:
	rm -f $(OBJECTS) objlist

clobber: clean
	rm -f $(LIBRARY)
