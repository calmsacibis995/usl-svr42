#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)bkrs:common/cmd/bkrs/rsnotify.d/makefile	1.7.5.2"
#ident "$Header: makefile 1.3 91/05/23 $"

include $(CMDRULES)

SRC1=rsnotify.c msgs.c
OBJ=$(SRC1:.c=.o)

BKLIB=../bklib.d/bklib.a
LIBBR=../libbrmeth.d/libbrmeth.a
IOLIB=../libadmIO.d/libadmIO.a

SADMDIR=$(USRSADM)
BKBIN=$(SADMDIR)/bkup/bin
OAMBASE=$(SADMDIR)/sysadm
ETCBKUP=$(ETC)/bkup

LOCAL=hdrs
GENERAL=../hdrs
LDLIBS=$(LDLIBPATH) -lTL -lgen
LOCALINC=-I $(LOCAL) -I $(GENERAL)
LINTFLAGS=$(DEFLIST)
OWN=root
GRP=sys

PRODUCT=rsnotify

all: $(PRODUCT)

rsnotify:  $(OBJ) $(BKLIB) $(LIBBR) $(IOLIB)
	$(CC) $(LDFLAGS) -o $(@) $(OBJ) $(BKLIB) $(LIBBR) $(IOLIB) $(LDLIBS) \
		$(SHLIBS)

touch: 
	touch $(SRC1)

clean:
	rm -f $(OBJ)

clobber: clean
	rm -f $(PRODUCT)

lintit:
	$(LINT) $(LINTFLAGS) $(SRC1)

strip:	$(PRODUCT)
	$(STRIP) $(PRODUCT)

install: $(PRODUCT)
	$(INS) -f $(USRSBIN) -u $(OWN) -g $(GRP) -m 0100 rsnotify

rsnotify.o: $(INC)/stdio.h
rsnotify.o: $(INC)/fcntl.h
rsnotify.o: $(INC)/string.h
rsnotify.o: $(INC)/sys/types.h
rsnotify.o: $(INC)/sys/stat.h
rsnotify.o: $(INC)/time.h
rsnotify.o: $(INC)/pwd.h
rsnotify.o: $(INC)/table.h
rsnotify.o: $(GENERAL)/rsnotify.h
rsnotify.o: $(LOCAL)/errors.h
