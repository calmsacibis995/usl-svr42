#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)xcpbackup:backup.mk	1.1.2.2"
#ident  "$Header: backup.mk 1.2 91/07/11 $"

include $(CMDRULES)

#	Makefile for backup

OWN = 
GRP = 

MAINS = pwdmenu backup

SOURCES = pwdmenu.c backup.sh

IDENT = Backup Ignore

all: $(MAINS)

pwdmenu: pwdmenu.o 
	$(CC) -o pwdmenu pwdmenu.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

pwdmenu.o: pwdmenu.c \
	$(INC)/stdio.h \
	$(INC)/pwd.h \
	$(INC)/string.h

clean:
	rm -f pwdmenu.o

clobber: clean
	rm -f $(MAINS)

lintit:
	$(LINT) $(LINTFLAGS) pwdmenu.c

install: all
	for i in $(IDENT); \
	do \
		grep -v "^#" $$i.sh | grep -v "^$$" > $(ETC)/$$i ;\
	done
	for i in $(MAINS); do \
		$(INS) -f $(USRBIN) $$i; \
	done
	-rm -f $(USRBIN)/.backup
	-ln $(USRBIN)/backup $(USRBIN)/.backup

#	These targets are useful but optional

partslist:
	@echo $(MAKEFILE) $(SOURCES) $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo $(MAINS) | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit $(MAKEFILE) $(LOCALINCS) $(SOURCES) -o pwdmenu.o $(MAINS)
