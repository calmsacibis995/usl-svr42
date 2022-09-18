#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)bkrs:common/cmd/bkrs/backup.d/makefile	1.1.7.2"
#ident "$Header: makefile 1.3 91/05/23 $"

include $(CMDRULES)

SRC1=backup.c msgs.c print.c queue.c bkregvalid.c
OBJ=$(SRC1:.c=.o)

SADMDIR=$(USRSADM)
BKBIN=$(SADMDIR)/bkup/bin
OAMBASE=$(SADMDIR)/sysadm
ETCBKUP=$(ETC)/bkup
OWN=root
GRP=bin

BKLIB=../bklib.d/bklib.a
LIBBR=../libbrmeth.d/libbrmeth.a
IOLIB=../libadmIO.d/libadmIO.a
LOCAL=hdrs
GENERAL=../hdrs
FLAGS=
LDLIBS=$(LDLIBPATH) -lTL -lgen
LOCALINC=-I $(LOCAL) -I $(GENERAL)
LINTFLAGS=-u $(DEFLIST)

PRODUCT=backup

all: $(PRODUCT)

backup: $(OBJ) $(BKLIB) $(IOLIB) $(LIBBR)
	$(CC) $(LDFLAGS) -o $(@) $(OBJ) $(BKLIB) $(LIBBR) $(IOLIB) $(LDLIBS) \
		$(SHLIBS)

clean:
	rm -f $(OBJ)

clobber:	clean
	rm -f $(PRODUCT)

lintit:
	$(LINT) $(SRC1)

strip:	$(PRODUCT)
	$(STRIP) $(PRODUCT)

install: $(PRODUCT)
	$(INS) -f $(USRSBIN) -u $(OWN) -g $(GRP) -m 0100 $(PRODUCT)

backup.o: $(INC)/sys/types.h
backup.o: $(INC)/time.h
backup.o: $(INC)/stdio.h
backup.o: $(INC)/setjmp.h
backup.o: $(INC)/pwd.h
backup.o: $(GENERAL)/backup.h
backup.o: $(GENERAL)/bkmsgs.h
backup.o: $(GENERAL)/bkreg.h
backup.o: $(INC)/bkrs.h
backup.o: $(LOCAL)/errors.h
backup.o: $(INC)/signal.h
backup.o: $(INC)/errno.h
print.o: $(INC)/time.h
print.o: $(INC)/stdio.h
print.o: $(GENERAL)/backup.h
print.o: $(GENERAL)/bkmsgs.h
queue.o: $(INC)/sys/types.h
queue.o: $(INC)/time.h
queue.o: $(GENERAL)/backup.h
queue.o: $(GENERAL)/bkmsgs.h
bkregvalid.o: $(INC)/stdio.h
bkregvalid.o: $(INC)/fcntl.h
bkregvalid.o: $(INC)/table.h
bkregvalid.o: $(INC)/string.h
bkregvalid.o: $(GENERAL)/backup.h
bkregvalid.o: $(INC)/sys/types.h
bkregvalid.o: $(GENERAL)/bkmsgs.h
bkregvalid.o: $(LOCAL)/errors.h
bkregvalid.o: $(GENERAL)/bkerrors.h
bkregvalid.o: $(GENERAL)/bkreg.h
