#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)bkrs:common/cmd/bkrs/rsstatus.d/makefile	1.10.5.3"
#ident "$Header: makefile 1.3 91/05/23 $"

include $(CMDRULES)

SRC1=rsstatus.c admin_auth.c msgs.c
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

PRODUCT=rsstatus ursstatus rsstatus.tab

all: $(PRODUCT)

rsstatus:  $(OBJ) $(BKLIB) $(IOLIB) $(LIBBR)
	$(CC) $(LDFLAGS) -o $(@) $(OBJ) $(BKLIB) $(LIBBR) $(IOLIB) $(LDLIBS) \
		$(SHLIBS)

ursstatus: rsstatus
	ln rsstatus ursstatus

rsstatus.tab: table
	grep -v "^#ident" table > rsstatus.tab

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
	$(INS) -f $(SBIN) -u $(OWN) -g $(GRP) -m 0100 rsstatus
	$(INS) -f $(USRSBIN) -u $(OWN) -g $(GRP) -m 0100 rsstatus
	$(INS) -f $(USRSBIN) -u $(OWN) -g $(GRP) -m 4111 ursstatus
	$(INS) -f $(ETCBKUP) -u $(OWN) -g $(GRP) -m 0600 rsstatus.tab

rsstatus.o: $(INC)/stdio.h
rsstatus.o: $(INC)/fcntl.h
rsstatus.o: $(INC)/string.h
rsstatus.o: $(INC)/errno.h
rsstatus.o: $(INC)/sys/types.h
rsstatus.o: $(INC)/sys/stat.h
rsstatus.o: $(INC)/time.h
rsstatus.o: $(INC)/pwd.h
rsstatus.o: $(INC)/table.h
rsstatus.o: $(GENERAL)/rsstatus.h
rsstatus.o: $(GENERAL)/bktypes.h
rsstatus.o: $(LOCAL)/errors.h
