#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/prt/lib/lib.mk	1.1"
#ident	"$Header: $"
#       Portions Copyright(c) 1988, Sun Microsystems, Inc.
#       All Rights Reserved


#       Makefile for libcom.a

include $(CMDRULES)

ARFLAGS = cr

LIBRARY = libcom.a

FILES = dofile.o		\
		date_ab.o	\
		fatal.o		\
		setsig.o	\
		xopen.o		\
		fdfopen.o	\
		xmsg.o		\
		cat.o		\
		repl.o		\
		trnslat.o	\
		clean.o		\
		dname.o		\
		sname.o		\
		imatch.o	\
		userexit.o	

all: $(LIBRARY)

$(LIBRARY): $(FILES)
	$(AR) $(ARFLAGS) $(LIBRARY) `$(LORDER) *.o | $(TSORT)`

clean:
	-rm -f *.o

clobber: clean
	-rm -f $(LIBRARY)
