#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)cs:reportsch/reportsch.mk	1.5.1.2"
#ident "$Header: reportsch.mk 1.2 91/03/25 $"

# Makefile for the reportscheme service

include $(CMDRULES)

INSDIR = $(USRSBIN)
OWN = root
GRP = sys
LINTFLAGS = $(DEFLIST)

PROD = reportscheme

MAINS = reportsch.o

SRCS = $(MAINS:.o=.c)

all:	$(PROD)

lintit:
	$(LINT) $(LINTFLAGS) $(SRCS)


$(PROD):	$(MAINS)
		$(CC) -o $(PROD) $(MAINS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

clean:
	rm -f $(MAINS)

clobber: clean
	rm -f $(MAINS)

strip: all
	$(STRIP) $(MAINS)

size: all
	$(SIZE) $(MAINS)

install: all
	$(INS) -f $(INSDIR) -m 0755 -u $(OWN) -g $(GRP) reportscheme
