#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)libiaf:common/lib/libiaf/iaf/iaf.mk	1.4.2.3"
#ident "$Header: iaf.mk 1.3 91/03/14 $"

include $(LIBRULES)

# 
# Identification and Authentication Library: IAF routines
#

LOCALDEF=-DNO_IMPORT $(PICFLAG)

INCLUDES=	\
		$(INC)/audit.h \
		$(INC)/errno.h \
		$(INC)/fcntl.h \
		$(INC)/grp.h \
		$(INC)/ia.h \
		$(INC)/iaf.h \
		$(INC)/mac.h \
		$(INC)/signal.h \
		$(INC)/stdio.h \
		$(INC)/stdlib.h \
		$(INC)/string.h \
		$(INC)/stropts.h \
		$(INC)/unistd.h \
		$(INC)/sys/conf.h \
		$(INC)/sys/param.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/types.h \
		$(INC)/sys/ulimit.h \
		$(INC)/sys/vnode.h

LIBOBJS = getava.o \
	  invoke.o \
	  putava.o \
	  retava.o \
	  setava.o \
	  setenv.o \
	  setid.o

OBJS =	../getava.o \
	../invoke.o \
	../putava.o \
	../retava.o \
	../setava.o \
	../setenv.o \
	../setid.o

all:       $(INCLUDES) $(LIBOBJS)

install:

clean:
	-rm -f *.o

clobber:	clean
	-rm -f $(OBJS)

.c.o:
	$(CC) $(CFLAGS) $(DEFLIST) -c $*.c && cp $(*F).o ..

lintit:
	$(LINT) *.c
