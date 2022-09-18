#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)eac:i386/eaccmd/maplocale/maplocale.mk	1.6.1.4"
#ident  "$Header: maplocale.mk 1.2 91/07/08 $"

include $(CMDRULES)

#

OWN = bin
GRP = bin

UUDECODE = uudecode

DIRS = $(USRBIN) $(ETC)/default $(USRLIB)/lang/english/us/88591

DEFAULT = default
DATAFILES = collate ctype currency messages numeric time

OBJ = maplocale.o map_sco_loc.o map_isc_loc.o

all: maplocale $(DATAFILES)

install: $(DIRS) all
	 $(INS) -f $(USRBIN) -m 0711 -u $(OWN) -g $(GRP) maplocale
	 cp default $(ETC)/default/lang
	 -for i in $(DATAFILES) ; do \
		 rm -f $(USRLIB)/lang/english/us/88591/$$i ;\
		 if [ -f $$i ] ; then \
			 cp $$i $(USRLIB)/lang/english/us/88591/$$i ;\
			 $(CH)chmod 0644 $(USRLIB)/lang/english/us/88591/$$i ;\
			 $(CH)chgrp $(GRP) $(USRLIB)/lang/english/us/88591/$$i;\
			 $(CH)chown $(OWN) $(USRLIB)/lang/english/us/88591/$$i;\
		fi ;\
	 done 

$(DIRS):
	- [ -d $@ ] || mkdir -p $@ ;\
		$(CH)chmod 0755 $@ ;\
		$(CH)chown $(OWN) $@ ;\
		$(CH)chgrp $(GRP) $@

clean: 
	rm -f $(OBJ)

clobber: clean
	rm -f maplocale $(DATAFILES)

lintit:
	$(LINT) $(LINTFLAGS) maplocale.c map_isc_loc.c map_sco_loc.c

maplocale: $(OBJ)
	$(CC) -o maplocale $(OBJ) $(LDFLAGS) $(LDLIBS) $(SHLIBS) 

maplocale.o: maplocale.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/fcntl.h \
	$(INC)/locale.h \
	$(INC)/nl_types.h \
	$(INC)/langinfo.h \
	$(INC)/ctype.h \
	$(INC)/unistd.h \
	maplocale.h

map_isc_loc.o: map_isc_loc.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/mman.h \
	$(INC)/sys/stat.h \
	$(INC)/fcntl.h \
	$(INC)/locale.h \
	$(INC)/nl_types.h \
	$(INC)/langinfo.h \
	$(INC)/ctype.h \
	$(INC)/unistd.h \
	maplocale.h

map_sco_loc.o: map_sco_loc.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/fcntl.h \
	$(INC)/locale.h \
	$(INC)/nl_types.h \
	$(INC)/langinfo.h \
	$(INC)/ctype.h \
	$(INC)/unistd.h \
	maplocale.h

collate: collate.src
	cat collate.src | grep -v "^#ident " | $(UUDECODE)
#ident  "$Header: maplocale.mk 1.2 91/07/08 $"

ctype: ctype.src
	cat ctype.src | grep -v "^#ident " | $(UUDECODE)
#ident  "$Header: maplocale.mk 1.2 91/07/08 $"

currency: currency.src
	cat currency.src | grep -v "^#ident " | $(UUDECODE)
#ident  "$Header: maplocale.mk 1.2 91/07/08 $"

numeric: numeric.src
	cat numeric.src | grep -v "^#ident " | $(UUDECODE)
#ident  "$Header: maplocale.mk 1.2 91/07/08 $"

messages: messages.src
	cat messages.src | grep -v "^#ident " | $(UUDECODE)
#ident  "$Header: maplocale.mk 1.2 91/07/08 $"

time: time.src
	cat time.src | grep -v "^#ident " | $(UUDECODE)
#ident  "$Header: maplocale.mk 1.2 91/07/08 $"

