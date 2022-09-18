#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)pkgtools:pkgmk.mk	1.3"

include $(CMDRULES)

PROC = pkgmk
OBJECTS = $(PROC)

## options used to build this command
LOCALINC = -I ../hdrs
LDLIBS = -L ../libinst -linst -L $(BASE)/libpkg -lpkg -L $(BASE)/libadm -ladm -L $(BASE)/libcmd -lcmd -lgen -L $(BASE)/include -l stubs

## objects which make up this process
OFILES=\
	splpkgmap.o main.o quit.o mkpkgmap.o getinst.o

all:	$(PROC)

$(PROC): $(OFILES)
	$(CC) -o $(PROC) $(OFILES) $(LDFLAGS) $(LDLIBS) $(SHLIBS)
	chmod 775 $(PROC)

clean:
	rm -f $(OFILES)

clobber: clean
	rm -f $(PROC)

