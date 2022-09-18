#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/eqn/eqn.d/eqn.mk	1.1"
#ident	"$Header: $"
#     Portions Copyright(c) 1988, Sun Microsystems, Inc.
#     All Rights Reserved


#	makefile eqn. 

include $(CMDRULES)

LOCALINC = -I. -I..

YFLAGS = -d

OWN = bin

GRP = bin

SOURCE= e.y e.h diacrit.c eqnbox.c font.c fromto.c funny.c glob.c integral.c \
 io.c lex.c lookup.c mark.c matrix.c move.c over.c paren.c \
 pile.c shift.c size.c sqrt.c text.c

FILES= e.o ../diacrit.o ../eqnbox.o ../font.o ../fromto.o ../funny.o \
 ../glob.o ../integral.o ../io.o ../lex.o ../lookup.o ../mark.o ../matrix.o \
 ../move.o ../over.o ../paren.o ../pile.o ../shift.o ../size.o ../sqrt.o \
 ../text.o

FILES1= e.o diacrit.o eqnbox.o font.o fromto.o funny.o glob.o integral.o \
 io.o lex.o lookup.o mark.o matrix.o move.o over.o paren.o \
 pile.o shift.o size.o sqrt.o text.o

INSDIR = $(ROOT)/$(MACH)/usr/ucb

all:	eqn

eqn:	$(FILES)
	$(CC) $(FILES1) -o eqn $(LDFLAGS) $(PERFLIBS)

e.c:	e.def

e.def:	../e.y
	$(YACC) $(YFLAGS) ../e.y
	mv y.tab.c e.c
	mv y.tab.h e.def

e.o:	e.c ../e.h

../diacrit.o:	../diacrit.c ../e.h

../eqnbox.o:	../eqnbox.c ../e.h

../font.o:	../font.c ../e.h

../fromto.o:	../fromto.c ../e.h

../funny.o:	../funny.c ../e.h

../glob.o:	../glob.c ../e.h

../integral.o:	../integral.c ../e.h

../io.o:	../io.c ../e.h

../lex.o:	../lex.c ../e.h

../lookup.o:	../lookup.c ../e.h

../mark.o:	../mark.c ../e.h

../matrix.o:	../matrix.c ../e.h

../move.o:	../move.c ../e.h

../over.o:	../over.c ../e.h

../paren.o: ../paren.c ../e.h

../pile.o:	../pile.c ../e.h

../shift.o: ../shift.c ../e.h

../size.o:	../size.c ../e.h

../sqrt.o:	../sqrt.c ../e.h

../text.o:	../text.c ../e.h

install: all
	$(INS) -f $(INSDIR) -u $(OWN) -g $(GRP) -m 00555 eqn

clean:	
	rm -f $(FILES1) e.c e.def 

clobber: clean
	rm -f eqn
