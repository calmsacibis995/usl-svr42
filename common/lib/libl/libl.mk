#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libl:libl.mk	1.8.1.6"

include $(LIBRULES)

LDLIBS=
SGSBASE=../../cmd/sgs
INS=$(SGSBASE)/sgs.install
SRCLIB=./lib
SOURCES=$(SRCLIB)/allprint.c $(SRCLIB)/main.c $(SRCLIB)/reject.c $(SRCLIB)/yyless.c $(SRCLIB)/yywrap.c
OBJECTS=allprint.o main.o reject.o yyless.o yywrap.o

.MUTEX: ncform nrform

all:     $(CCSLIB)/libl.a

$(CCSLIB)/libl.a: $(OBJECTS)
	$(AR) $(ARFLAGS) tmplib.a `$(LORDER) *.o | $(TSORT)`;

install:  ncform nrform all
	/bin/sh $(INS) 644 $(OWN) $(GRP) $(CCSLIB)/libl.a tmplib.a;

allprint.o:	$(SRCLIB)/allprint.c
	$(CC) -c $(CFLAGS) $(SRCLIB)/allprint.c
main.o:		$(SRCLIB)/main.c
	$(CC) -c $(CFLAGS) $(SRCLIB)/main.c
reject.o:	$(SRCLIB)/reject.c
	$(CC) -c $(CFLAGS) $(SRCLIB)/reject.c
yyless.o:	$(SRCLIB)/yyless.c
	$(CC) -c $(CFLAGS) $(SRCLIB)/yyless.c
yywrap.o:	$(SRCLIB)/yywrap.c
	$(CC) -c $(CFLAGS) $(SRCLIB)/yywrap.c

ncform:	$(SRCLIB)/ncform
	if [ ! -d $(CCSLIB)/lex ];\
		then mkdir $(CCSLIB)/lex;\
	fi
	/bin/sh $(INS) 644 $(OWN) $(GRP) $(CCSLIB)/lex/ncform $(SRCLIB)/ncform;

nrform:	$(SRCLIB)/nrform
	/bin/sh $(INS) 644 $(OWN) $(GRP) $(CCSLIB)/lex/nrform $(SRCLIB)/nrform;

clean:
	-rm -f $(OBJECTS)

clobber:clean
	-rm -f tmplib.a 

lintit:	$(SOURCES)
	$(LINT) $(LINTFLAGS) $(SOURCES)
