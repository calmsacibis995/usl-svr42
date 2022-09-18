#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)libTL:common/lib/libTL/libTL.mk	1.10.7.2"
#ident "$Header: libTL.mk 1.3 91/03/14 $"

include $(LIBRULES)

SRC= TLappend.c TLassign.c TLclose.c TLdelete.c TLgetentry.c TLgetfield.c \
	TLopen.c TLread.c TLsearches.c TLsubst.c TLsync.c TLwrite.c \
	description.c entry.c field.c file.c parse.c search.c space.c \
	table.c utils.c
OBJ=$(SRC:.c=.o)

PRODUCT=libTL.a
LOCALHDRS=hdrs

LOCALINC=-I$(LOCALHDRS)
ARFLAGS=cr

all: $(PRODUCT)

$(PRODUCT): $(OBJ)
	$(AR) $(ARFLAGS) $(PRODUCT) `$(LORDER) $(OBJ) | $(TSORT)`

TLtest: $(PRODUCT) TLtest.o
	$(CC) $(CFLAGS) $(DEFLIST) -o $(@) TLtest.o $(PRODUCT) $(LDFLAGS) \
		$(LDLIBS) $(PERFLIBS)

touch:
	touch $(SRC)

install: $(PRODUCT)
	$(INS) -f $(USRLIB) -m 0644 $(PRODUCT) 

clean:
	rm -rf $(OBJ)

clobber: clean
	rm -rf $(PRODUCT)

strip: $(PRODUCT)

lintit:
	$(LINT) -u $(CFLAGS) $(SRC)

$(OBJ) TLtest.o : $(INC)/table.h $(LOCALHDRS)/internal.h
