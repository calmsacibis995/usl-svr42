#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)libcmd:common/lib/libcmd/libcmd.mk	1.2.8.2"
#ident "$Header: libcmd.mk 1.4 91/05/29 $"
#	Copyright (c) 1987, 1988 Microsoft Corporation
#	  All Rights Reserved

#	This Module contains Proprietary Information of Microsoft
#	Corporation and should be treated as Confidential.


#
#	@(#) libcmd.mk 1.2 88/05/03 libcmd:libcmd.mk
#

include $(LIBRULES)

OWN=bin
GRP=bin

MAKEFILE= libcmd.mk

LIBRARY = libcmd.a

OBJECTS	=  magic.o sum.o deflt.o getterm.o privname.o systbl.o sttyname.o \
	tp_ops.o
SOURCES	= $(OBJECTS:.o=.c)

all:	$(LIBRARY)

install:	all
	$(INS) -f $(USRLIB) -m 0444 -u $(OWN) -g $(GRP)  $(LIBRARY) 

# local directory, nothing to copy

clean:
	-rm -f $(OBJECTS)

clobber: clean
	-rm -f $(LIBRARY)

FRC:

.PRECIOUS:	$(LIBRARY)

$(LIBRARY):	$(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS)

deflt.o: deflt.c \
	$(INC)/stdio.h \
	$(INC)/deflt.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/string.h \
	$(INC)/stdlib.h \
	$(INC)/sys/param.h

getterm.o: getterm.c \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/limits.h

magic.o: magic.c \
	$(INC)/stdio.h \
	$(INC)/pfmt.h \
	$(INC)/string.h \
	$(INC)/errno.h $(INC)/sys/errno.h

privname.o: privname.c \
	$(INC)/string.h \
	$(INC)/sys/types.h \
	$(INC)/sys/privilege.h

sttyname.o: sttyname.c \
	$(INC)/sys/types.h \
	$(INC)/dirent.h $(INC)/sys/dirent.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/sys/stat.h \
	$(INC)/ctype.h \
	$(INC)/string.h \
	$(INC)/unistd.h $(INC)/sys/unistd.h \
	$(INC)/sys/mman.h \
	$(INC)/malloc.h \
	$(INC)/ttymap.h

sum.o: sum.c \
	$(INC)/stdio.h \
	$(INC)/assert.h \
	$(INC)/sys/types.h \
	$(INC)/std.h $(INC)/sys/unistd.h \
	$(INC)/sum.h

tp_ops.o:	tp_ops.c \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/sys/mkdev.h \
	$(INC)/string.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/time.h \
	$(INC)/mac.h $(INC)/sys/mac.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tp.h
