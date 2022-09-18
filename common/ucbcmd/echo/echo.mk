#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/echo/echo.mk	1.1"
#ident	"$Header: $"
#	Portions Copyright(c) 1988, Sun Microsystems, Inc.
#	All Rights Reserved


#	Makefile for echo

include $(CMDRULES)

INSDIR = $(ROOT)/$(MACH)/usr/ucb

OWN = bin

GRP = bin

MAKEFILE = echo.mk

MAINS = echo

OBJECTS =  echo.o

SOURCES =  echo.c

ALL:		$(MAINS)

echo:		echo.o 
	$(CC) -o echo  echo.o   $(LDFLAGS) $(PERFLIBS)


echo.o:		 

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

all : ALL

install: ALL
	$(INS) -f $(INSDIR) -u $(OWN) -g $(GRP) -m 0555 $(MAINS) 

