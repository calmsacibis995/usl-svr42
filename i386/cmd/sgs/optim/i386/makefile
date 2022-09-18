#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)optim:i386/makefile	1.1.3.6"
#
#	i386 OPTIMIZER MAKEFILE
#
#
# Some optimizations by default will help 486 performance while hurting
# 386 performance.  To change the default setting (optimize for the 386),
# add -DI386_OPTS to CFLAGS below.
#
include $(CMDRULES)

LINTFLAGS=$(DEFLIST) $(INCLIST)

INS=$(SGSBASE)/sgs.install
INSDIR=$(CCSLIB)

SOURCES=$(COMDIR)/inter.c $(COMDIR)/optim.c sched.c ebboptim.c \
	debug.c imull.c inline.c leave_opt.c local.c mystrstr.c optutil.c \
	peep.c regal.c w1opt.c w2opt.c w3opt.c w4opt.c w6opt.c \
	postpeep.c database.c loops.c
OBJECTS=inter.o optim.o sched.o ebboptim.o \
	debug.o imull.o inline.o leave_opt.o local.o mystrstr.o optutil.o \
	peep.o regal.o w1opt.o w2opt.o w3opt.o w4opt.o w6opt.o \
	postpeep.o database.o loops.o
ASMS=inter.s optim.s sched.s ebboptim.s \
	debug.s imull.s inline.s leave_opt.s local.s mystrstr.s optutil.s \
	peep.s regal.s w1opt.s w2opt.s w3opt.s w4opt.s w6opt.s \
	postpeep.s database.s
PRODUCTS=optim

DEFLIST=
INCLIST= -I. -I$(COMDIR) -I$(CPUINC) -I$(COMINC)  -I$(INC)
CC_CMD=$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST)
LD_CMD=$(CC) $(LDFLAGS)
LINT_CMD=$(LINT) $(LINTFLAGS)

all:		$(PRODUCTS)

optim:		$(OBJECTS)
		$(LD_CMD) $(OBJECTS) $(LDLIBS) -o optim

inter.o:	$(COMDIR)/optim.h defs.h $(COMDIR)/inter.c
		$(CC_CMD) $(COMDIR)/inter.c

optim.o:	$(COMDIR)/optim.h defs.h $(COMDIR)/optim.c
		$(CC_CMD) $(COMDIR)/optim.c

database.o:	defs.h database.h
		$(CC_CMD) database.c	

loops.o:	defs.h $(COMDIR)/optim.h optutil.h
		$(CC_CMD) loops.c

debug.o:	defs.h debug.h regal.h debug.c
		$(CC_CMD) debug.c

ebboptim.o:	defs.h sched.h $(COMDIR)/optim.h ebboptim.c
		$(CC_CMD) ebboptim.c

imull.o:	$(COMDIR)/optim.h defs.h optutil.h imull.c
		$(CC_CMD) imull.c

inline.o:	$(COMDIR)/optim.h defs.h optutil.h $(CPUINC)/paths.h \
		inline.c
		$(CC_CMD) inline.c

leave_opt.o:	$(COMDIR)/optim.h defs.h optutil.h leave_opt.c
		$(CC_CMD) leave_opt.c

local.o:	$(COMDIR)/optim.h defs.h optutil.h $(CPUINC)/sgs.h \
		$(CPUINC)/paths.h regal.h $(COMINC)/storclass.h local.c
		$(CC_CMD) local.c

mystrstr.o:	mystrstr.c
		$(CC_CMD) mystrstr.c

optutil.o:	$(COMDIR)/optim.h defs.h optutil.h optutil.c
		$(CC_CMD) optutil.c

peep.o:		$(COMDIR)/optim.h defs.h optutil.h peep.c
		$(CC_CMD) peep.c

postpeep.o:	defs.h sched.h $(COMDIR)/optim.h postpeep.c
		$(CC_CMD) postpeep.c

regal.o:	$(COMDIR)/optim.h defs.h optutil.h regal.h \
		$(COMINC)/storclass.h regal.c
		$(CC_CMD) regal.c

sched.o:	defs.h sched.h optutil.h $(COMDIR)/optim.h database.h \
		sched.c
		$(CC_CMD) sched.c

w1opt.o:	$(COMDIR)/optim.h defs.h optutil.h w1opt.c
		$(CC_CMD) w1opt.c

w2opt.o:	$(COMDIR)/optim.h defs.h optutil.h w2opt.c
		$(CC_CMD) w2opt.c

w3opt.o:	$(COMDIR)/optim.h defs.h optutil.h w3opt.c
		$(CC_CMD) w3opt.c

w4opt.o:	$(COMDIR)/optim.h defs.h optutil.h w4opt.c
		$(CC_CMD) w4opt.c

w6opt.o:	$(COMDIR)/optim.h defs.h optutil.h w6opt.c
		$(CC_CMD) w6opt.c

install:	$(INSDIR)/$(SGS)optim

$(INSDIR)/$(SGS)optim:	all
		cp optim optim.bak
		-$(STRIP) optim
		/bin/sh $(INS) 755 $(OWN) $(GRP) $(INSDIR)/$(SGS)optim optim
		mv optim.bak optim

save:		$(INSDIR)/$(SGS)optim
		-rm -f $(INSDIR)/$(SGS)optim.back
		cp $(INSDIR)/$(SGS)optim $(INSDIR)/$(SGS)optim.back

uninstall:	$(INSDIR)/$(SGS)optim.back
		-rm -f $(INSDIR)/$(SGS)optim
		cp $(INSDIR)/$(SGS)optim.back $(INSDIR)/optim

lintit:		$(SOURCES)
		$(LINT_CMD) $(SOURCES)

clean:
		-rm -f $(OBJECTS)
		-rm -f $(ASMS)

shrink:		clobber

clobber:	clean
		-rm -f $(PRODUCTS)

new:		clean  all
