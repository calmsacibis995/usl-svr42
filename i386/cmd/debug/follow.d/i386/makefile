#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)debugger:follow.d/i386/makefile	1.2"

include $(CMDRULES)

BASENAME = follow

TARGET = ../../$(BASENAME)

include ../../util/common/std.make

CCSOURCES = 

LIBRARIES = 

CSOURCES = \
	$(COMMON)/follow.c


include DEPEND

all:	$(TARGET)

$(TARGET):	$(OBJECTS)
	rm -f $(TARGET)
	$(CC) -o $(TARGET) $(LINK_MODE) $(OBJECTS) $(LIBRARIES) $(LDLIBS)

install:	$(CCSLIB)/$(BASENAME)

$(CCSLIB)/$(BASENAME):	$(TARGET)
	$(STRIP) $(TARGET)
	cp $(TARGET) $(CCSLIB)/$(BASENAME)
