#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)rfuadmin:rfuadmin.mk	1.3.10.2"
#ident "$Header: rfuadmin.mk 1.2 91/04/10 $"

include $(CMDRULES)

#	rfuadmin make file

OWN = bin
GRP = bin

INSDIR = $(ETC)/rfs

all: rfuadmin

rfuadmin:
	sh rfuadmin.sh

install: all
	-@if [ ! -d "$(INSDIR)" ] ; \
	then \
	mkdir $(INSDIR) ; \
	fi ;
	 $(INS) -f $(INSDIR) -m 0554 -u $(OWN) -g $(GRP) rfuadmin
	-rm -f $(USRNSERVE)/rfuadmin
	-@if [ ! -d "$(USRNSERVE)" ] ; \
	then \
	mkdir $(USRNSERVE) ; \
	fi ;
	-$(SYMLINK) /etc/rfs/rfuadmin $(USRNSERVE)/rfuadmin

clean:
	rm -f rfuadmin

clobber: clean
