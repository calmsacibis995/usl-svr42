#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sccs:cmd/cmd.mk	6.16.3.4"
#
#

include $(CMDRULES)

HDR = ../hdr


LINT_LIBS = ../lib/llib-lcom.ln \
	../lib/llib-lcassi.ln \
	../lib/llib-lmpw.ln

LIBS = ../lib/comobj.a \
	../lib/cassi.a \
	../lib/mpwlib.a

HELPLOC = $(CCSLIB)/help

LINT_FILES = admin.out	\
	comb.out	\
	delta.out	\
	get.out		\
	prs.out		\
	unget.out	\
	val.out		\
	vc.out		\
	what.out

C_CMDS = admin	\
	cdc	\
	comb	\
	delta	\
	get	\
	prs	\
	rmdel	\
	sact	\
	unget	\
	val	\
	vc	\
	what

SGSBASE=../..
INC=
INCSYS=
INCLIST=-I$(SGSBASE)/sgs/inc/common

CMDS = $(C_CMDS)	\
	sccsdiff

all:	$(CMDS) help help2

$(CMDS): $(LIBS)

admin:	admin.o	$(LIBS)
	$(CC) $(LDFLAGS) admin.o $(LINK_MODE) $(LIBS) -o admin

admin.o:	admin.c
	$(CC) -c $(INCLIST) $(CFLAGS) admin.c

cdc:	rmchg
	-ln	rmchg cdc

comb:	comb.o	$(LIBS)
	$(CC) $(LDFLAGS) comb.o $(LINK_MODE) $(LIBS) -o comb

comb.o:	comb.c
	$(CC) -c $(INCLIST) $(CFLAGS) comb.c

delta:	delta.o	$(LIBS)
	$(CC) $(LDFLAGS) delta.o $(LINK_MODE) $(LIBS) -o delta

delta.o:	delta.c
	$(CC) -c $(INCLIST) $(CFLAGS) delta.c

get:	get.o	$(LIBS)
	$(CC) $(LDFLAGS) get.o $(LINK_MODE) $(LIBS) -o get

get.o:	get.c
	$(CC) -c $(INCLIST) $(CFLAGS) get.c

help:	help.o
	$(CC) $(LDFLAGS) help.o -o help

help.o:	help.c
	$(CC) -c $(INCLIST) $(CFLAGS) help.c

help2:	help2.o	$(LIBS)
	$(CC) $(LDFLAGS) help2.o $(LINK_MODE) $(LIBS) -o help2

help2.o: help2.c
	$(CC) -c $(INCLIST) $(CFLAGS) help2.c

prs:	prs.o	$(LIBS)
	$(CC) $(LDFLAGS) prs.o $(LINK_MODE) $(LIBS) -o prs

prs.o:	prs.c
	$(CC) -c $(INCLIST) $(CFLAGS) prs.c
	
rmdel:	rmchg $(LIBS)
	-ln rmchg rmdel

rmchg:	rmchg.o $(LIBS)
	$(CC) $(LDFLAGS) rmchg.o $(LINK_MODE) $(LIBS) -o rmchg

rmchg.o:	rmchg.c
	$(CC) -c $(INCLIST) $(CFLAGS) rmchg.c

sact:	unget
	-ln unget sact

sccsdiff:	sccsdiff.sh
	cp sccsdiff.sh sccsdiff
	chmod +x sccsdiff

unget:	unget.o	$(LIBS)
	$(CC) $(LDFLAGS) unget.o $(LINK_MODE) $(LIBS) -o unget

unget.o:	unget.c
	$(CC) -c $(INCLIST) $(CFLAGS) unget.c

val:	val.o	$(LIBS)
	$(CC) $(LDFLAGS) val.o $(LINK_MODE) $(LIBS) -o val

val.o:	val.c
	$(CC) -c $(INCLIST) $(CFLAGS) val.c

vc:	vc.o	$(LIBS)
	$(CC) $(LDFLAGS) vc.o $(LINK_MODE) $(LIBS) -o vc

vc.o:	vc.c
	$(CC) -c $(INCLIST) $(CFLAGS) vc.c

what:	what.o	$(LIBS)
	$(CC) $(LDFLAGS) what.o $(LINK_MODE) $(LIBS) -o what

what.o:	what.c
	$(CC) -c $(INCLIST) $(CFLAGS) what.c

$(LIBS):
	cd ../lib; $(MAKE) -f lib.mk

install:	all
	$(STRIP) $(C_CMDS)
	$(STRIP) help help2
	$(CH)-chmod 775 $(CMDS) help help2
	$(CH)-chgrp $(GRP) $(CMDS) help2
	$(CH)-chown $(OWN) $(CMDS) help2
	-mv $(CMDS) $(CCSBIN)
	if [ ! -d $(HELPLOC) ] ; then mkdir $(HELPLOC) ; fi
	if [ ! -d $(HELPLOC)/lib ] ; then mkdir $(HELPLOC)/lib ; fi
	-mv help2 $(HELPLOC)/lib
	-mv help $(CCSBIN)

clean:
	-rm -f *.o
	-rm -f $(LINT_FILES)
	-rm -f rmchg

clobber:	clean
	-rm -f $(CMDS) help help2

.SUFFIXES : .o .c .e .r .f .y .yr .ye .l .s .out

.c.out:
	rm -f $*.out
	$(LINT) $< $(LINTFLAGS) $(LINT_LIBS) > $*.out

lintit:	$(LINT_FILES)
	@echo "Library $(LLIBRARY) is up to date\n"

