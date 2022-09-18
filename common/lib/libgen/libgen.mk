#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libgen:libgen.mk	1.10.1.2"

#	Makefile for libgen

include $(LIBRULES)

INS = install

MAKEFILE = libgen.mk


LIBRARY = libgen.a

OBJECTS =  basename.o bgets.o bufsplit.o copylist.o dirname.o eaccess.o \
	gmatch.o isencrypt.o mkdirp.o p2open.o pathfind.o \
	reg_compile.o reg_step.o regcmp.o regex.o rmdirp.o strccpy.o \
	strecpy.o strfind.o strrspn.o strtrns.o

SOURCES =  basename.c bgets.c bufsplit.c copylist.c dirname.c eaccess.c \
	gmatch.c isencrypt.c mkdirp.c p2open.c pathfind.c \
	reg_compile.c reg_step.c regcmp.c regex.c rmdirp.c strccpy.c \
	strecpy.c strfind.c strrspn.c strtrns.c

all:		 $(LIBRARY)

$(LIBRARY): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) `$(LORDER) $(OBJECTS) | $(TSORT)`

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(LIBRARY)

basename.o: basename.c \
	./synonyms.h \
	$(INC)/string.h

bgets.o: bgets.c \
	./synonyms.h \
	$(INC)/sys/types.h \
	$(INC)/stdio.h

bufsplit.o: bufsplit.c \
	./synonyms.h \
	$(INC)/sys/types.h

copylist.o: copylist.c \
	./synonyms.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/stdio.h

dirname.o: dirname.c \
	./synonyms.h \
	$(INC)/string.h

eaccess.o: eaccess.c \
	./synonyms.h

gmatch.o: gmatch.c \
	./synonyms.h \
	$(INC)/stdlib.h \
	$(INC)/limits.h \
	$(INC)/ctype.h \
	./_wchar.h \
	./_range.h

isencrypt.o: isencrypt.c \
	./synonyms.h \
	$(INC)/sys/types.h \
	$(INC)/locale.h

mkdirp.o: mkdirp.c \
	./synonyms.h \
	$(INC)/sys/types.h \
	$(INC)/stdio.h \
	$(INC)/errno.h \
	$(INC)/string.h

p2open.o: p2open.c \
	./synonyms.h \
	$(INC)/stdio.h \
	$(INC)/signal.h \
	$(INC)/fcntl.h \
	$(INC)/errno.h

pathfind.o: pathfind.c \
	./synonyms.h \
	$(INC)/limits.h \
	$(INC)/string.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	./libgen.h

reg_compile.o: reg_compile.c \
	./synonyms.h \
	$(INC)/ctype.h \
	$(INC)/stdlib.h \
	$(INC)/limits.h \
	./_wchar.h \
	./_range.h \
	./_regexp.h

reg_step.o: reg_step.c \
	./synonyms.h \
	$(INC)/ctype.h \
	$(INC)/stdlib.h \
	$(INC)/limits.h \
	./_wchar.h \
	./_regexp.h

regcmp.o: regcmp.c \
	./synonyms.h \
	$(INC)/ctype.h \
	$(INC)/stdlib.h \
	$(INC)/limits.h \
	$(INC)/stdarg.h \
	./_wchar.h \
	./_range.h

regex.o: regex.c \
	./synonyms.h \
	$(INC)/ctype.h \
	$(INC)/stdlib.h \
	$(INC)/limits.h \
	$(INC)/stdarg.h \
	./_wchar.h

rmdirp.o: rmdirp.c \
	./synonyms.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/stdio.h \
	$(INC)/errno.h \
	$(INC)/string.h

strccpy.o: strccpy.c \
	./synonyms.h

strecpy.o: strecpy.c \
	./synonyms.h \
	$(INC)/ctype.h \
	$(INC)/string.h \
	$(INC)/stdio.h

strfind.o: strfind.c \
	./synonyms.h

strrspn.o: strrspn.c \
	./synonyms.h \
	$(INC)/string.h

strtrns.o: strtrns.c \
	./synonyms.h

install: all
	$(INS) -f $(CCSLIB) -m 644 -u bin -g bin $(LIBRARY) 
	$(INS) -f $(ROOT)/$(MACH)/usr/include -m 644 -u bin -g bin regexpr.h
	$(INS) -f $(ROOT)/$(MACH)/usr/include -m 644 -u bin -g bin libgen.h

lintit: $(SOURCES)
	$(LINT) $(LINTFLAGS) $(SOURCES)
