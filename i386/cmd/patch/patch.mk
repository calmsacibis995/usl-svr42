#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)patch:i386/cmd/patch/patch.mk	1.4"
#ident	"$Header: $"

include $(CMDRULES)

INSDIR = $(USRSBIN)/pkginst

SMALL = 
LARGE =  

c = patch.c pch.c inp.c util.c

obj = patch.o pch.o inp.o util.o 

lintflags = -phbvxac

SHELL = /bin/sh

all: patch

patch: $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

# won't work with csh
install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 555 -u bin -g bin patch

clean:
	rm -f *.o *.orig core

realclean:
	rm -f patch *.o *.orig core 

# The following lint has practically everything turned on.  Unfortunately,
# you have to wade through a lot of mumbo jumbo that can't be suppressed.
# If the source file has a /*NOSTRICT*/ somewhere, ignore the lint message
# for that spot.

lint:
	lint $(lintflags) $(defs) $(c) > patch.fuzz

#patch.o: config.h common.h patch.c inp.h pch.h util.h version.h
#pch.o: config.h common.h pch.c pch.h util.h
#inp.o: config.h common.h inp.c inp.h util.h
#util.o: config.h common.h util.c util.h

