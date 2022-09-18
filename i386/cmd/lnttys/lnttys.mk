#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lnttys:lnttys.mk	1.3.4.1"
#ident "$Header: lnttys.mk 1.2 91/04/18 $"

include $(CMDRULES)

OWN = root
GRP = root

all: lnttys.sh lnsxts.sh lnxts.sh
	cp lnttys.sh lnttys
	cp lnsxts.sh lnsxts
	cp lnxts.sh lnxts

install: all
	$(INS) -f $(USRSBIN) -m 0744 -u $(OWN) -g $(GRP) lnttys
	$(INS) -f $(USRSBIN) -m 0744 -u $(OWN) -g $(GRP) lnsxts
	$(INS) -f $(USRSBIN) -m 0744 -u $(OWN) -g $(GRP) lnxts

clean:

clobber: clean
	rm -f lnttys lnsxts lnxts
