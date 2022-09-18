#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)pkgtools:inst.mk	1.3"

#	Makefile for libinst

include $(CMDRULES)

LOCALINC =  -I../hdrs

MAKEFILE = Makefile

LIBRARY = libinst.a

OBJECTS = $(SOURCES:.c=.o)

SOURCES =  copyf.c dockdeps.c echo.c eptstat.c finalck.c lockinst.c ocfile.c \
	pathdup.c pkgdbmerg.c procmap.c psvr4ck.c ptext.c putparam.c \
	qreason.c qstrdup.c setadmin.c setlist.c srcpath.c

all:		 $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS)

clean:
	rm -f $(OBJECTS)
	- if [ -d $(ROOT)/$(MACH)/xenv ] ;\
		then $(MAKE) -f Makefile3.2 $(MAKEARGS) clean; fi
	: do nothing

clobber:
	rm -f $(OBJECTS) $(LIBRARY)
	- if [ -d $(ROOT)/$(MACH)/xenv ] ;\
		then $(MAKE) -f Makefile3.2 $(MAKEARGS) clobber; fi
	: do nothing

