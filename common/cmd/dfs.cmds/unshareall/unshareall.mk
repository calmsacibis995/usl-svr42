#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)dfs.cmds:unshareall/unshareall.mk	1.3.5.2"
#ident "$Header: unshareall.mk 1.2 91/04/05 $"

include $(CMDRULES)

INSDIR = $(USRSBIN)
OWN = bin
GRP = bin
FRC =

all: unshareall

unshareall: unshareall.sh 
	cp unshareall.sh unshareall
	chmod 555 unshareall

install: all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) unshareall

clean:

clobber: clean
	rm -f unshareall
FRC:
