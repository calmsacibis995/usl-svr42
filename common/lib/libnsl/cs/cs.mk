#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)libnsl:common/lib/libnsl/cs/cs.mk	1.1.5.4"
#ident "$Header: cs.mk 1.3 91/03/14 $"

include $(LIBRULES)

# Makefile for the libnsl connection server interface 

LOCALDEF=-DNO_IMPORT $(PICFLAG)

MAINS = csi.o 

SRCS = $(MAINS:.o=.c)

all:		$(MAINS)
		cp $(MAINS) ..
lintit:
	$(LINT) $(LINTFLAGS) $(SRCS)

clean:
	rm -f *.o

clobber:	clean
	rm -f $(MAINS)
