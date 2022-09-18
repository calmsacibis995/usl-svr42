#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)bkrs:common/cmd/bkrs/bkoper.d/makefile	1.16.5.2"
#ident "$Header: makefile 1.3 91/05/23 $"

include $(CMDRULES)

SRC1=bkoper.c list.c msgs.c utils.c
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
LDLIBS=$(LDLIBPATH) -lTL -lgen -ladm
LOCALINC=-I $(LOCAL) -I $(GENERAL)
LINTFLAGS=-u $(DEFLIST)
OWN=root
GRP=sys

PRODUCT=bkoper

all: $(PRODUCT)

bkoper: $(OBJ) $(BKLIB) $(IOLIB) $(LIBBR)
	$(CC) $(LDFLAGS) -o $(@) $(OBJ) $(BKLIB) $(LIBBR) $(IOLIB) $(LDLIBS) \
		$(SHLIBS)

clean:
	rm -f $(OBJ)

clobber:	clean
	rm -f $(PRODUCT)

lintit:
	$(LINT) $(LINTFLAGS) $(SRC1)

strip:	$(PRODUCT)
	$(STRIP) $(PRODUCT)

install: $(PRODUCT)
	$(INS) -f $(USRSBIN) -u $(OWN) -g $(GRP) -m 0100 $(PRODUCT)

bkoper.o: $(INC)/stdio.h
bkoper.o: $(INC)/sys/types.h
bkoper.o: $(INC)/ctype.h
bkoper.o: $(INC)/time.h
bkoper.o: $(INC)/bkrs.h
bkoper.o: $(GENERAL)/bktypes.h
bkoper.o: $(GENERAL)/backup.h
bkoper.o: $(GENERAL)/bkmsgs.h
bkoper.o: $(GENERAL)/bkerrors.h
bkoper.o: $(GENERAL)/bkstatus.h
bkoper.o: $(LOCAL)/bkoper.h
bkoper.o: $(LOCAL)/errors.h
list.o: $(INC)/sys/types.h
list.o: $(INC)/sys/stat.h
list.o: $(INC)/fcntl.h
list.o: $(INC)/time.h
list.o: $(INC)/stdio.h
list.o: $(INC)/table.h
list.o: $(INC)/devmgmt.h
list.o: $(GENERAL)/bktypes.h
list.o: $(GENERAL)/backup.h
list.o: $(GENERAL)/bkmsgs.h
list.o: $(LOCAL)/bkoper.h
list.o: $(GENERAL)/bkstatus.h
list.o: $(LOCAL)/errors.h
list.o: $(INC)/errno.h
utils.o: $(INC)/sys/types.h
utils.o: $(INC)/string.h
utils.o: $(INC)/pwd.h
