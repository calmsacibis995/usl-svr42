#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)debugger:tutorial.d/i386/makefile	1.3"

include $(CMDRULES)

include ../../util/common/std.make

FILES1 = ../common/files \
	../common/macro.c.doc \
	../common/macro.h.doc \
	../common/main.c.doc \
	../common/sget.c.doc \
	../common/test1 \
	../common/test2 \
	../common/walk.c.doc

FILES2 = file1 \
	s.main.c \
	../common/testfile

TARGET = file1 main.c s.main.c

all:	$(FILES1) $(FILES2)

s.main.c:	../common/main.c.doc
	sed -e '1s/".*"/"\%\W\%"/' ../common/main.c.doc >main.c
	admin -n -imain.c s.main.c

file1:
	touch file1

install:	$(FILES1) $(FILES2)
	if [ ! -d $(CCSLIB)/tutorial ]; then \
		mkdir $(CCSLIB)/tutorial; \
	fi
	rm -rf $(CCSLIB)/tutorial/*
	cp ../common/files $(CCSLIB)/tutorial/files
	cp ../common/macro.c.doc $(CCSLIB)/tutorial/macro.c
	cp ../common/macro.h.doc $(CCSLIB)/tutorial/macro.h
	cp ../common/main.c.doc $(CCSLIB)/tutorial/main.c
	cp ../common/sget.c.doc $(CCSLIB)/tutorial/sget.c
	cp ../common/test1 $(CCSLIB)/tutorial/test1
	cp ../common/test2 $(CCSLIB)/tutorial/test2
	cp ../common/walk.c.doc $(CCSLIB)/tutorial/walk.c
	$(CH)chmod 444 $(CCSLIB)/tutorial/*
	mkdir $(CCSLIB)/tutorial/src
	cp $(FILES2) $(CCSLIB)/tutorial/src
	$(CH)chmod 444 $(CCSLIB)/tutorial/src/*
