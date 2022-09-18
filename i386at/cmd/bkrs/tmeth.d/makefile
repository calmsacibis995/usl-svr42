#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:i386at/cmd/bkrs/tmeth.d/makefile	1.2"
#ident	"$Header: $"

include $(CMDRULES)

SRC=tmeth.c event.c
OBJ=$(SRC:.c=.o)

BKLIB=../bklib.d/bklib.a
LIBBR=../libbrmeth.d/libbrmeth.a
IOLIB=../libadmIO.d/libadmIO.a
LOCAL=hdrs
GENERAL=../hdrs
LOCALINC = -I $(LOCAL) -I $(GENERAL)
FLAGS=
LDFLAGS=$(LDLIBPATH)

PRODUCTS=tmeth

all: $(PRODUCTS)

tmeth: $(BKLIB) $(LIBBR) $(IOLIB) $(OBJ)
	$(CC) $(CFLAGS) $(DEFLIST) -o $(@) $(OBJ) $(LDFLAGS) $(LDLIBS) $(SHLIBS) -lTL $(BKLIB) $(LIBBR) $(IOLIB) -lTL -lgen

touch: 
	touch $(SRC)

clean:
	rm -f $(OBJ)

clobber: clean
	rm -f $(PRODUCTS) 

lintit:
	lint $(LINTFLAGS) $(SRC)

strip:	$(PRODUCTS)
	$(STRIP) $(PRODUCTS)

install: $(PRODUCTS)
	echo NO INSTALL for toy methods

tmeth.o: method.h
tmeth.o: $(INC)/sys/types.h
tmeth.o: $(INC)/stdio.h
tmeth.o: $(INC)/string.h
tmeth.o: $(GENERAL)/bktypes.h
tmeth.o: $(INC)/bkrs.h
event.o: $(INC)/sys/types.h
event.o: $(INC)/signal.h
