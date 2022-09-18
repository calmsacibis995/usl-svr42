#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)face:src/proc/proc.mk	1.1.4.4"
#ident	"$Header: $"


include $(CMDRULES)

LIBRARY = libproc.a
HEADER1=../filecab/inc
LOCALINC=-I$(HEADER1)
OBJECTS= suspend.o

$(LIBRARY):	$(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS)

suspend.o:	suspend.c $(HEADER1)/wish.h

###### Standard makefile targets #######

all:		$(LIBRARY)

install:	all

clean:
		rm -f *.o

clobber:	clean
		rm -f $(LIBRARY)

.PRECIOUS:	$(LIBRARY)
