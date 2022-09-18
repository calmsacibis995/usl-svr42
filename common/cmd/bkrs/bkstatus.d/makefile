#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)bkrs:common/cmd/bkrs/bkstatus.d/makefile	1.8.5.2"
#ident "$Header: makefile 1.3 91/05/23 $"

include $(CMDRULES)

SRC1=bkstatus.c msgs.c
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
FLAGS=
LDLIBS=$(LDLIBPATH) -lTL -lgen
LOCALDEF=-I $(LOCAL) -I $(GENERAL)
LINTFLAGS=$(DEFLIST)
OWN=root
GRP=sys

PRODUCT=bkstatus

all: $(PRODUCT)

bkstatus:  $(OBJ) $(BKLIB) $(IOLIB) $(LIBBR)
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
	$(INS) -f $(USRSBIN) -u $(OWN) -g $(GRP) -m 0100 bkstatus

bkstatus.o: $(INC)/stdio.h
bkstatus.o: $(INC)/fcntl.h
bkstatus.o: $(INC)/string.h
bkstatus.o: $(INC)/errno.h
bkstatus.o: $(INC)/sys/types.h
bkstatus.o: $(INC)/sys/stat.h
bkstatus.o: $(INC)/time.h
bkstatus.o: $(INC)/pwd.h
bkstatus.o: $(INC)/ctype.h
bkstatus.o: $(INC)/table.h
bkstatus.o: $(GENERAL)/bkstatus.h
bkstatus.o: $(GENERAL)/backup.h
bkstatus.o: $(GENERAL)/bktypes.h
bkstatus.o: $(LOCAL)/errors.h
