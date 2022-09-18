#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)devmgmt:common/cmd/devmgmt/putdev/putdev.mk	1.5.9.5"
#ident "$Header: putdev.mk 1.2 91/04/05 $"

include $(CMDRULES)

DIR1=$(SBIN)
DIR2=$(USRBIN)
OWN=root
GRP=sys
HDRS=$(INC)/stdio.h $(INC)/string.h $(INC)/ctype.h $(INC)/stdlib.h \
	$(INC)/errno.h $(INC)/unistd.h $(INC)/mac.h \
	$(INC)/devmgmt.h
FILE=putdev
OBJECTS=putdev
SRC=main.c
OBJ=$(SRC:.c=.o)
LOCALINC=-I.
LDLIBS=-ladm
LINTFLAGS=$(DEFLIST)

all:		 $(FILE) 

install:	all
		-rm -f $(DIR1)/putdev
		$(INS) -f $(DIR1) -m 555 -u $(OWN) -g $(GRP) $(OBJECTS) 
		-rm -f $(DIR2)/putdev
		$(INS) -f $(DIR2) -m 555 -u $(OWN) -g $(GRP) $(OBJECTS) 

clobber		: clean
		rm -f $(FILE)

clean		:
		rm -f $(OBJ)

lintit		:
		for i in $(SRC); \
		do \
		    $(LINT) $(LINTFLAGS) $$i; \
		done

$(FILE)		: $(OBJ)
		$(CC) $(OBJ) -o $@ $(LDFLAGS) $(LDLIBS) $(NOSHLIBS)

$(OBJ)		: $(HDRS)
