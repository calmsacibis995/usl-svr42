#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)bkrs:common/cmd/bkrs/bkexcept.d/makefile	1.10.6.2"
#ident "$Header: makefile 1.3 91/05/23 $"

include $(CMDRULES)

SRC1=bkexcept.c msgs.c
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

PRODUCT=bkexcept bkexcept.tab

all: $(PRODUCT)

bkexcept:  $(OBJ) $(BKLIB) $(IOLIB) $(LIBBR)
	$(CC) $(LDFLAGS) -o $(@) $(OBJ) $(BKLIB) $(LIBBR) $(IOLIB) $(LDLIBS) \
		$(SHLIBS)

bkexcept.tab: table
	grep -v "^#ident" table >bkexcept.tab

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
	$(INS) -f $(USRSBIN) -u $(OWN) -g $(GRP) -m 0100 bkexcept
	$(INS) -f $(BKBIN) -u $(OWN) -g $(GRP) -m 0400 exconv.sed
	$(INS) -f $(ETCBKUP) -u $(OWN) -g $(GRP) -m 0600 bkexcept.tab

bkexcept.o: $(INC)/stdio.h
bkexcept.o: $(INC)/signal.h
bkexcept.o: $(INC)/fcntl.h
bkexcept.o: $(INC)/string.h
bkexcept.o: $(INC)/errno.h
bkexcept.o: $(INC)/sys/types.h
bkexcept.o: $(INC)/sys/stat.h
bkexcept.o: $(INC)/table.h
bkexcept.o: $(GENERAL)/bkexcept.h
bkexcept.o: $(GENERAL)/bktypes.h
bkexcept.o: $(LOCAL)/errors.h
