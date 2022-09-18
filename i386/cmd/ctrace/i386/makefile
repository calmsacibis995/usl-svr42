#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ctrace:i386/makefile	1.8"
#	ctrace makefile

include $(CMDRULES)

# installation directories:
INSDIR=$(CCSBIN)
CTLIB=$(CCSLIB)/ctrace
CRLIB=/usr/ccs/lib/ctrace

DEFLIST = -DRUNTIME=\"$(CRLIB)/runtime.c\"
COMDIR=../common
CPUINC=../../sgs/inc/$(CPU)
INCLIST=-I . -I $(CPUINC) -I $(COMDIR)
CC_CMD = $(CC) -c $(CFLAGS) $(DEFLIST) -I$(INC)
INS=../../install/install.sh
LIBELF=
LINK_MODE=

# Must derive y.tab.h from acgram.y
# add -t for yacc debug (YYDEBUG)
# add -v for yacc y.output file
YFLAGS=
YYDEBUG=
YACC_CMD=       $(YACC) $(YFLAGS) -d

LFLAGS=

LINTFLAGS=-b

SOURCE	 = $(COMDIR)/constants.h $(COMDIR)/global.h $(COMDIR)/main.c \
	   $(COMDIR)/parser.y $(COMDIR)/scanner.l $(COMDIR)/lookup.c \
	   $(COMDIR)/trace.c ./runtime.c $(COMDIR)/ctcr
CFILES =   $(COMDIR)/main.c parser.c scanner.c \
	   $(COMDIR)/lookup.c $(COMDIR)/trace.c
HFILE =    $(COMDIR)/constants.h $(COMDIR)/global.h
OBJECTS =  main.$o parser.$o scanner.$o lookup.$o trace.$o
PRODUCT=ctrace

.MUTEX:		parser.h $(OBJECTS)

all:	$(PRODUCT) 

$(PRODUCT):	parser.h $(OBJECTS:$o=o)
		$(CC) $(CFLAGS) $(OBJECTS:$o=o) $(LINK_MODE) -o $(PRODUCT)

main.o:		$(COMDIR)/main.c $(HFILE) $(CPUINC)/sgs.h
		$(CC_CMD) '-DTRACEVERS="01.01"' $(INCLIST) $(COMDIR)/main.c

parser.c:	$(COMDIR)/parser.y $(HFILE)
		$(YACC_CMD) $(COMDIR)/parser.y
		mv y.tab.c parser.c
		if cmp -s y.tab.h parser.h; then rm y.tab.h; \
		else cp y.tab.h parser.h; fi

parser.h:	parser.c

scanner.c:	$(COMDIR)/scanner.l
		$(LEX) $(LFLAGS) $(COMDIR)/scanner.l
		mv lex.yy.c scanner.c

parser.o:	parser.c
		$(CC_CMD) $(INCLIST) $(YYDEBUG) parser.c

scanner.o:	parser.h scanner.c
		$(CC_CMD) $(INCLIST) scanner.c	

lookup.o:	parser.h $(COMDIR)/lookup.c
		$(CC_CMD) $(INCLIST) $(COMDIR)/lookup.c

trace.o:	$(COMDIR)/trace.c
		$(CC_CMD) $(INCLIST) $(COMDIR)/trace.c
	
install: 	all
		cp ctrace ctrace.bak
		cp $(COMDIR)/ctcr ctcr
		$(STRIP) ctrace
		/bin/sh $(INS) -f $(INSDIR) ctrace
		/bin/sh $(INS) -f $(INSDIR) ctcr
		rm -f $(INSDIR)/ctc
		ln $(INSDIR)/ctcr $(INSDIR)/ctc
		if [ ! -d $(CTLIB) ] ;\
		then \
			mkdir $(CTLIB);\
		fi
		/bin/sh $(INS) -f $(CTLIB) runtime.c
		mv ctrace.bak ctrace
		rm -f ctcr

clean:
		rm -f *.o y.output
		rm -f lint.out
		rm -f *.ln

clobber: 	clean
		rm -f ctrace parser.[ch] scanner.c y.tab.h

lintit:		$(CFILES)
		$(LINT) $(LINTFLAGS) $(INCLIST) -I$(INC) $(CFILES)

ctrace.ln:	$(CFILES)
		rm -f $(OBJECTS:$o=ln)
		$(LINT) $(LINTFLAGS) -c $(INCLIST) -I$(INC) $(CFILES)
		cat $(OBJECTS:$o=ln) >ctrace.ln
