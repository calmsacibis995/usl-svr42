#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)defadm:defadm.mk	1.1.1.2"
#ident "$Header: defadm.mk 1.3 91/04/08 $"

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = root
GRP = sys

LDLIBS = -lcmd

MAKEFILE = defadm.mk

MAINS = defadm

OBJECTS =  defadm.o

SOURCES =  defadm.c

all:		$(MAINS)

defadm:		defadm.o	
	$(CC) -o $@ defadm.o $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

defadm.o:	 $(INC)/sys/types.h $(INC)/deflt.h	\
		 $(INC)/dirent.h $(INC)/fcntl.h	\
		 $(INC)/errno.h	$(INC)/string.h \
		 $(INC)/unistd.h $(INC)/stdio.h  \
		 $(INC)/sys/stat.h $(INC)/sys/param.h \
		 $(INC)/signal.h 

clean:
	rm -f $(OBJECTS)
	
clobber: clean
	rm -f $(MAINS)

install:	all $(INSDIR)
	$(INS) -f $(INSDIR) -m 0500 -u $(OWN) -g $(GRP) $(MAINS)

strip:
	$(STRIP) $(MAINS)

remove:
	cd $(INSDIR);  rm -f $(MAINS)

$(INSDIR):
	mkdir $(INSDIR);  $(CH)chmod 755 $(INSDIR);  $(CH)chown bin $(INSDIR)

partslist:
	@echo $(MAKEFILE) $(LOCALINCS) $(SOURCES)  |  tr ' ' '\012'  |  sort

product:
	@echo $(MAINS)  |  tr ' ' '\012'  | \
		sed -e 's;^;$(INSDIR)/;' -e 's;//*;/;g'

productdir:
	@echo $(INSDIR)
