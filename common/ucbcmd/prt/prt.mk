#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/prt/prt.mk	1.1"
#ident	"$Header: $"
#       Portions Copyright(c) 1988, Sun Microsystems, Inc.
#       All Rights Reserved


#	Makefile for prt.

include $(CMDRULES)

LIBS = lib/libcom.a 

INSDIR = $(ROOT)/$(MACH)/usr/ucb

OWN = bin

GRP = bin

CMDS = prt	

all:	$(LIBS) $(CMDS)

prt:	prt.o	$(LIBS)
	$(CC) $(LDFLAGS) prt.o $(LIBS) -o prt $(PERFLIBS)

prt.o:	prt.c

$(LIBS):
	cd lib; $(MAKE) -f lib.mk

install:	all
	$(INS) -f $(INSDIR) -u $(OWN) -g $(GRP) -m 0555 $(CMDS)

clean:
	-rm -f *.o lib/*.o

clobber:	clean
	-rm -f $(CMDS) $(LIBS)
