#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)bkrs:common/cmd/bkrs/bkreg.d/makefile	1.15.5.2"
#ident "$Header: makefile 1.3 91/05/23 $"

include $(CMDRULES)

SRC1=bkreg.c msgs.c
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

PRODUCT=bkreg bkreg.tab

all: $(PRODUCT)

bkreg: $(OBJ) $(BKLIB) $(IOLIB) $(LIBBR)
	$(CC) $(LDFLAGS) -o $(@) $(OBJ) $(BKLIB) $(LIBBR) $(IOLIB) $(LDLIBS) \
		$(SHLIBS)

bkreg.tab: table
	grep -v "^#ident" table >bkreg.tab

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
	$(INS) -f $(USRSBIN) -u $(OWN) -g $(GRP) -m 0100 bkreg
	$(INS) -f $(ETCBKUP) -u $(OWN) -g $(GRP) -m 0600 bkreg.tab

bkreg.o: $(INC)/string.h
bkreg.o: $(INC)/signal.h
bkreg.o: $(INC)/sys/types.h
bkreg.o: $(INC)/sys/stat.h
bkreg.o: $(INC)/stdio.h
bkreg.o: $(INC)/fcntl.h
bkreg.o: $(INC)/ctype.h
bkreg.o: $(INC)/table.h
bkreg.o: $(INC)/errno.h
bkreg.o: $(GENERAL)/backup.h
bkreg.o: $(GENERAL)/bkreg.h
bkreg.o: $(LOCAL)/errors.h
