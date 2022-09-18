#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/plot/libplot/plot/plot.mk	1.1"
#ident	"$Header: $"

#	Copyright (c) 1983, 1984 1985, 1986, 1987, 1988, Sun Microsystems, Inc.
#	All Rights Reserved.

#     Makefile for plot

include $(CMDRULES)

LOCALINC = -I$(ROOT)/$(MACH)/usr/ucbinclude 

ARFLAGS = cr

MAKEFILE = plot.mk

MAINS = ../libplot.a

OBJECTS = arc.o box.o circle.o close.o cont.o dot.o erase.o label.o \
        line.o linmod.o move.o open.o point.o putsi.o space.o 

SOURCES = arc.c box.c circle.c close.c cont.c dot.c erase.c label.c \
        line.c linmod.c move.c open.c point.c putsi.c space.c 

ALL:          $(MAINS)

$(MAINS):	$(OBJECTS)	
	$(AR) $(ARFLAGS) $(MAINS) `$(LORDER) $(OBJECTS) | $(TSORT)`
	
clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)


