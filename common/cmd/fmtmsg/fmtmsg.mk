#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)fmtmsg:fmtmsg.mk	1.12.3.2"
#ident "$Header: fmtmsg.mk 1.2 91/04/09 $"

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

INCSYS=$(INC)
HDRS=$(INC)/fmtmsg.h $(INC)/stdio.h $(INC)/string.h $(INC)/errno.h
FILE=fmtmsg
INSTALLS=fmtmsg
SRC=main.c
OBJ=$(SRC:.c=.o)
LOCALINC=-I.
LINTFLAGS=$(DEFLIST)

all		: $(FILE) 

install		: all
		$(INS) -f $(INSDIR) -u $(OWN) -g $(GRP) $(INSTALLS)

clobber		: clean
		rm -f $(FILE)

clean		:
		rm -f $(OBJ)

strip		: $(FILE)
		$(STRIP) $(FILE)

lintit		: $(SRC)
		$(LINT) $(LINTFLAGS) $(SRC)

$(FILE)		: $(OBJ)
		$(CC) $(OBJ) -o $@ $(LDFLAGS) $(LDLIBS) $(SHLIBS)

$(OBJ)		: $(HDRS)
