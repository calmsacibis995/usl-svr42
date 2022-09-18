#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/plot/libplot/tf77/tf77.mk	1.1"
#ident	"$Header: $"

#	Copyright (c) 1983, 1984 1985, 1986, 1987, 1988, Sun Microsystems, Inc.
#	All Rights Reserved.

#     Makefile for tf77

include $(CMDRULES)

LOCALINC = -I$(ROOT)/$(MACH)/usr/ucbinclude 

ARFLAGS = cr

MAKEFILE = tf77.mk

MAINS = ../libtf77.a

OBJECTS = arc.o box.o circle.o clospl.o cont.o dot.o erase.o label.o \
        line.o linemd.o move.o open.o point.o space.o

SOURCES = arc.c box.c circle.c clospl.c cont.c dot.c erase.c label.c \
        line.c linemd.c move.c open.c point.c space.c

ALL:          $(MAINS)

$(MAINS):	$(OBJECTS)	
	$(AR) $(ARFLAGS) $(MAINS) `$(LORDER) $(OBJECTS) | $(TSORT)`
	
clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)


