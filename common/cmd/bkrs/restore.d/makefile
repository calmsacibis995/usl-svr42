#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)bkrs:common/cmd/bkrs/restore.d/makefile	1.13.6.3"
#ident "$Header: makefile 1.3 91/05/23 $"

include $(CMDRULES)

SRC1= auto.c cancel.c file.c mail.c msgs.c request.c restore.c utils.c view.c
OBJ=$(SRC1:.c=.o)

SADMDIR=$(USRSADM)
BKBIN=$(SADMDIR)/bkup/bin
OAMBASE=$(SADMDIR)/sysadm
ETCBKUP=$(ETC)/bkup

BKLIB=../bklib.d/bklib.a
RSLIB=../rslib.d/rslib.a
LIBBR=../libbrmeth.d/libbrmeth.a
IOLIB=../libadmIO.d/libadmIO.a
LOCAL=hdrs
GENERAL=../hdrs
LDLIBS=$(LDLIBPATH) -lTL -lgen -ladm
LOCALINC=-I $(LOCAL) -I $(GENERAL)
LINTFLAGS=-u $(DEFLIST)
OWN=root
GRP=sys

PRODUCT=restore urestore

all: $(PRODUCT)

restore: $(OBJ) $(BKLIB) $(LIBBR) $(IOLIB) $(RSLIB)
	$(CC) $(LDFLAGS) -o $(@) $(OBJ) $(RSLIB) $(BKLIB) $(LIBBR) $(IOLIB) \
		$(LDLIBS) $(NOSHLIBS)

urestore: restore
	-ln restore urestore

clean:
	rm -f $(OBJ)

clobber:	clean
	rm -f $(PRODUCT)

lintit:
	$(LINT) $(LINTFLAGS) $(SRC1)

strip:	$(PRODUCT)
	$(STRIP) $(PRODUCT)

install: $(PRODUCT)
	$(INS) -f $(SBIN) -u $(OWN) -g $(GRP) -m 0100 restore
	$(INS) -f $(USRSBIN) -u $(OWN) -g $(GRP) -m 0100 restore
	$(INS) -f $(USRSBIN) -u $(OWN) -g $(GRP) -m 4111 urestore

auto.o: $(INC)/time.h
auto.o: $(INC)/stdio.h
auto.o: $(INC)/table.h
auto.o: $(INC)/sys/utsname.h
auto.o: $(INC)/bkrs.h
auto.o: $(GENERAL)/backup.h
auto.o: $(GENERAL)/bktypes.h
auto.o: $(GENERAL)/bkhist.h
auto.o: $(GENERAL)/restore.h
auto.o: $(GENERAL)/rsstatus.h
auto.o: $(GENERAL)/brarc.h
cancel.o: $(INC)/fcntl.h
cancel.o: $(INC)/stdio.h
cancel.o: $(INC)/pwd.h
cancel.o: $(INC)/errno.h
cancel.o: $(INC)/table.h
cancel.o: $(GENERAL)/rsstatus.h
cancel.o: $(LOCAL)/errors.h
file.o: $(INC)/stdio.h
mail.o: $(INC)/fcntl.h
mail.o: $(INC)/stdio.h
mail.o: $(INC)/errno.h
mail.o: $(INC)/table.h
mail.o: $(GENERAL)/rsnotify.h
mail.o: $(LOCAL)/errors.h
request.o: $(INC)/sys/types.h
request.o: $(INC)/stdio.h
request.o: $(INC)/fcntl.h
request.o: $(INC)/errno.h
request.o: $(INC)/table.h
request.o: $(GENERAL)/restore.h
request.o: $(LOCAL)/rs.h
request.o: $(GENERAL)/rsstatus.h
request.o: $(LOCAL)/errors.h
restore.o: $(INC)/fcntl.h
restore.o: $(INC)/sys/types.h
restore.o: $(INC)/ctype.h
restore.o: $(INC)/stdio.h
restore.o: $(INC)/table.h
restore.o: $(INC)/errno.h
restore.o: $(INC)/bkrs.h
restore.o: $(GENERAL)/restore.h
restore.o: $(GENERAL)/bkhist.h
restore.o: $(LOCAL)/errors.h
restore.o: $(LOCAL)/rs.h
utils.o: $(INC)/string.h
utils.o: $(INC)/sys/types.h
utils.o: $(INC)/pwd.h
utils.o: $(INC)/table.h
view.o: $(INC)/fcntl.h
view.o: $(INC)/sys/types.h
view.o: $(INC)/sys/stat.h
view.o: $(INC)/time.h
view.o: $(INC)/pwd.h
view.o: $(INC)/grp.h
view.o: $(INC)/table.h
view.o: $(GENERAL)/restore.h
view.o: $(GENERAL)/bkhist.h
view.o: $(GENERAL)/brtoc.h
