#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)diff:diff.mk	1.13.1.2"
#ident "$Header: diff.mk 1.3 91/04/08 $"
#	diff make file

include $(CMDRULES)

REL = current
CSID = -r`gsid diff $(REL)`
MKSID = -r`gsid diff.mk $(REL)`
LIST = lp
INSDIR = $(USRBIN)
OWN = bin
GRP = bin
INSLIB = $(USRLIB)

SOURCE = diff.c diffh.c

VPATH: all ;	ROOT=`pwd`
compile all: diff diffh

diff:	diff.c
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ diff.c $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

install: all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) diff 
	$(INS) -f $(INSLIB) -m 0555 -u $(OWN) -g $(GRP) diffh

diffh:	diffh.c
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ diffh.c $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

build:	bldmk
	get -p $(CSID) s.diff.src $(REWIRE) | ntar -d $(RDIR) -g
bldmk:  ;  get -p $(MKSID) s.diff.mk > $(RDIR)/diff.mk

listing:
	pr diff.mk $(SOURCE) | $(LIST)
listmk: ;  pr diff.mk | $(LIST)

edit:
	get -e -p s.diff.src | ntar -g

delta:
	ntar -p $(SOURCE) > diff.src
	delta s.diff.src
	rm -f $(SOURCE)

mkedit:  ;  get -e s.diff.mk
mkdelta: ;  delta s.diff.mk

clean:
	:

clobber:
	  rm -f diff diffh

delete:	clobber
	rm -f $(SOURCE)
