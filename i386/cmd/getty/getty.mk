#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)getty:getty.mk	1.5.6.3"
#ident  "$Header: getty.mk 1.1 91/07/09 $"

include $(CMDRULES)


OWN = 
GRP = 

LOCALDEF = -DSYS_NAME -DMERGE386

all: getty

getty: getty.o
	$(CC) -o getty getty.o $(LDFLAGS) $(LDLIBS) 

getty.o: getty.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/termio.h \
	$(INC)/signal.h \
	$(INC)/sys/stat.h \
	$(INC)/utmp.h \
 	crtctl.h \
	$(INC)/sys/utsname.h \
	$(INC)/ctype.h \
	$(INC)/unistd.h \
	$(INC)/errno.h \
	$(INC)/fcntl.h \
	$(INC)/sys/sysmsg.h \
	$(INC)/sys/param.h \
	$(INC)/errno.h \
	$(INC)/fcntl.h \
	$(INC)/stdio.h

test:
	rtest getty

install: all
	$(INS) -f $(USRSBIN) -o getty $(USRSBIN)

clean:
	-rm -f getty.o

clobber: clean
	-rm -f getty

lintit:
	$(LINT) $(LINTFLAGS) getty.c
