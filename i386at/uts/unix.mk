#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86at:unix.mk	1.26"
#ident 	"$Header: $"

include $(UTSRULES)

KBASE    = .
LOCALDEF = -DREM
INSPERM  = -m 644 -u $(OWN) -g $(GRP)
KERNEL   = $(CONF)/pack.d/kernel/Driver.o
DIRS     = \
	$(INC) \
	$(INC)/sys \
	$(INC)/sys/fs \
	$(INC)/sys/fs/proc \
	$(INC)/sys/ws \
	$(INC)/vm \
	$(INC)/net \
	$(INC)/netinet \
	$(INC)/nfs \
	$(INC)/rpc \
	$(INC)/des \
	$(INC)/fs \
	$(INC)/klm


# The util target is first since it must be built before
# svc due to dependency. Stretch util and svc as far as
# possible in order to support the use of PARALLEL makes.

all: FRC util acc fs io mem proc net svc vpix 
	$(MAKE) -f unix.mk unix $(MAKEARGS)

ALL: FRC util acc fs io mem proc net vpix svc unix idbuild


idbuild:
	$(PFX)idbuild -B -K -I$(INC)

acc:: FRC
	cd acc; $(MAKE) -f acc.mk  $(MAKEARGS)

fs: FRC
	cd fs; $(MAKE) -f fs.mk $(MAKEARGS)

io: FRC
	cd io; $(MAKE) -f io.mk $(MAKEARGS)

mem: FRC
	cd mem; $(MAKE) -f mem.mk $(MAKEARGS)

net: FRC
	cd net; $(MAKE) -f net.mk $(MAKEARGS)

proc: FRC
	cd proc; $(MAKE) -f proc.mk $(MAKEARGS)

svc: FRC
	cd svc; $(MAKE) -f svc.mk  $(MAKEARGS)

util: FRC
	cd util; $(MAKE) -f util.mk $(MAKEARGS)

boot: FRC
	cd boot; $(MAKE) -f boot.mk $(MAKEARGS)

vpix: FRC
	cd vpix; $(MAKE) -f vpix.mk $(MAKEARGS)

depend:
	cd acc; $(MAKE) -f acc.mk depend MAKEFILE=acc.mk $(MAKEARGS)
	cd boot; $(MAKE) -f boot.mk depend MAKEFILE=boot.mk $(MAKEARGS)
	cd fs; $(MAKE) -f fs.mk depend MAKEFILE=fs.mk $(MAKEARGS)
	cd io; $(MAKE) -f io.mk depend MAKEFILE=io.mk $(MAKEARGS)
	cd mem; $(MAKE) -f mem.mk depend MAKEFILE=mem.mk $(MAKEARGS)
	cd net; $(MAKE) -f net.mk depend MAKEFILE=net.mk $(MAKEARGS)
	cd proc; $(MAKE) -f proc.mk depend MAKEFILE=proc.mk $(MAKEARGS)
	cd svc; $(MAKE) -f svc.mk depend MAKEFILE=svc.mk $(MAKEARGS)
	cd util; $(MAKE) -f util.mk depend MAKEFILE=util.mk $(MAKEARGS)
	cd vpix; $(MAKE) -f vpix.mk depend MAKEFILE=vpix.mk $(MAKEARGS)

unix:   ID svc/syms.o svc/start.o
	$(LD) -r -o $(KERNEL) svc/syms.o svc/start.o
	@echo === unix.mk complete

ID:
	cd kernel.cf; $(IDINSTALL) -R$(CONF) -M kernel
	[ -d $(CONF)/cf.d ] || mkdir $(CONF)/cf.d
	$(INS) -f $(CONF)/cf.d -u $(OWN) -g $(GRP) res_major
	$(INS) -f $(CONF)/cf.d -u $(OWN) -g $(GRP) kernmap
	echo "`grep -v '^#' deflist` $(LOCALDEF) $(GLOBALDEF)" | \
	sed -e 's/ -D_KERNEL_HEADERS//' >$(CONF)/cf.d/deflist

clean:
	cd acc; $(MAKE) -f acc.mk clean $(MAKEARGS)
	cd fs; $(MAKE) -f fs.mk clean $(MAKEARGS)
	cd io; $(MAKE) -f io.mk clean $(MAKEARGS)
	cd mem; $(MAKE) -f mem.mk clean $(MAKEARGS)
	cd proc; $(MAKE) -f proc.mk clean $(MAKEARGS)
	cd svc; $(MAKE) -f svc.mk clean $(MAKEARGS)
	cd util; $(MAKE) -f util.mk clean $(MAKEARGS)
	cd net; $(MAKE) -f net.mk clean $(MAKEARGS)
	cd vpix; $(MAKE) -f vpix.mk clean $(MAKEARGS)
	-rm -f *.o

clobber:
	cd acc; $(MAKE) -f acc.mk "CONF=$(CONF)" clobber
	cd fs; $(MAKE) -f fs.mk "CONF=$(CONF)" clobber
	cd io; $(MAKE) -f io.mk "CONF=$(CONF)" clobber
	cd mem; $(MAKE) -f mem.mk "CONF=$(CONF)" clobber
	cd proc; $(MAKE) -f proc.mk "CONF=$(CONF)" clobber
	cd svc; $(MAKE) -f svc.mk "CONF=$(CONF)" clobber
	cd util; $(MAKE) -f util.mk "CONF=$(CONF)" clobber
	cd net; $(MAKE) -f net.mk "CONF=$(CONF)" clobber
	cd vpix; $(MAKE) -f vpix.mk "CONF=$(CONF)" clobber
	-rm -f *.o
	-$(IDINSTALL) -e -R$(CONF) -d kernel


install:all

FRC:

$(DIRS):
	-mkdir -p $@

headinstall: $(DIRS) FRC
	cd acc; $(MAKE) -f acc.mk headinstall $(MAKEARGS)
	cd fs; $(MAKE) -f fs.mk headinstall $(MAKEARGS)
	cd io; $(MAKE) -f io.mk headinstall $(MAKEARGS)
	cd mem; $(MAKE) -f mem.mk headinstall $(MAKEARGS)
	cd proc; $(MAKE) -f proc.mk headinstall $(MAKEARGS)
	cd svc; $(MAKE) -f svc.mk headinstall $(MAKEARGS)
	cd util; $(MAKE) -f util.mk headinstall $(MAKEARGS)
	cd net; $(MAKE) -f net.mk headinstall $(MAKEARGS)
	cd vpix; $(MAKE) -f vpix.mk headinstall $(MAKEARGS)

