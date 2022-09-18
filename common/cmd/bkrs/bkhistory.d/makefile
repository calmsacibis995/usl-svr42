#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)bkrs:common/cmd/bkrs/bkhistory.d/makefile	1.8.6.3"
#ident "$Header: makefile 1.3 91/05/23 $"

include $(CMDRULES)

SRC1=bkhistory.c msgs.c
OBJ=$(SRC1:.c=.o)

BKLIB=../bklib.d/bklib.a
LIBBR=../libbrmeth.d/libbrmeth.a
IOLIB=../libadmIO.d/libadmIO.a

OAMBASE=$(USROAM)

LOCAL=hdrs
GENERAL=../hdrs
LDLIBS=$(LDLIBPATH) -lTL -lgen
LOCALINC=-I $(LOCAL) -I $(GENERAL)
LINTFLAGS=$(DEFLIST)
OWN=root
GRP=sys

PRODUCT=bkhistory

all: $(PRODUCT)

bkhistory:  $(OBJ) $(BKLIB) $(IOLIB) $(LIBBR)
	$(CC) $(LDFLAGS) -o $(@) $(OBJ) $(BKLIB) $(LIBBR) $(IOLIB) $(LDLIBS) \
		$(NOSHLIBS)

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
	$(INS) -f $(SBIN) -u $(OWN) -g $(GRP) -m 0100 $(PRODUCT)
	$(INS) -f $(USRSBIN) -u $(OWN) -g $(GRP) -m 0100 $(PRODUCT)

bkhistory.o: $(INC)/stdio.h
bkhistory.o: $(INC)/fcntl.h
bkhistory.o: $(INC)/string.h
bkhistory.o: $(INC)/sys/types.h
bkhistory.o: $(INC)/sys/stat.h
bkhistory.o: $(INC)/time.h
bkhistory.o: $(INC)/pwd.h
bkhistory.o: $(INC)/grp.h
bkhistory.o: $(INC)/table.h
bkhistory.o: $(GENERAL)/bkhist.h
bkhistory.o: $(GENERAL)/bktypes.h
bkhistory.o: $(GENERAL)/backup.h
bkhistory.o: $(GENERAL)/brtoc.h
bkhistory.o: $(LOCAL)/errors.h
