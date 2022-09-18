#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)pkgtools:pkg.mk	1.3"

include $(LIBRULES)

#	Makefile for libpkg

LOCALINC =  -Ihdrs

MAKEFILE = libpkg.mk

LIBRARY = libpkg.a

OBJECTS = $(SOURCES:.c=.o)

SOURCES =  canonize.c ckparam.c ckvolseq.c cvtpath.c devtype.c dstream.c \
	gpkglist.c gpkgmap.c isdir.c logerr.c mappath.c pkgexecl.c pkgexecv.c \
	pkgmount.c pkgtrans.c pkgxpand.c ppkgmap.c privent.c progerr.c \
	putcfile.c rrmdir.c runcmd.c srchcfile.c tputcfent.c verify.c

all:		$(LIBRARY)

$(LIBRARY): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS)

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(LIBRARY)


