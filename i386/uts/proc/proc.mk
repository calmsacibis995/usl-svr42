#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:proc/proc.mk	1.15"
#ident	"$Header: $"

include $(UTSRULES)


KBASE = ..
PROC = $(CONF)/pack.d/proc/Driver.o

FILES = \
	acct.o \
	core.o \
	cred.o \
	cswitch.o \
	exec.o \
	execargs.o \
	execmdep.o \
	execseg.o \
	exit.o \
	fork.o \
	grow.o \
	license.o \
	lock.o \
	pgrp.o \
	pid.o \
	procmdep.o \
	procset.o \
	scalls.o \
	session.o \
	sig.o \
	sigcalls.o \
	disp.o \
	priocntl.o \
	slp.o

CFILES =  \
	acct.c  \
	core.c  \
	cred.c  \
	exec.c  \
	execargs.c  \
	execmdep.c  \
	execseg.c  \
	exit.c  \
	fork.c  \
	grow.c  \
	license.c  \
	lock.c  \
	pgrp.c  \
	pid.c  \
	procmdep.c  \
	procset.c  \
	scalls.c  \
	session.c  \
	sig.c  \
	sigcalls.c  \
	disp.c  \
	priocntl.c  \
	slp.c


SFILES =  \
	cswitch.s 

.s.o:
	$(AS) $<


all:	ID dir $(PROC)

ID:
	cd proc.cf; $(IDINSTALL) -R$(CONF) -M proc

$(PROC): $(FILES)
	$(LD) -r -o $(PROC) $(FILES) sysclass.o

dir:
	@for i in `ls`;\
	do\
		if [ -d $$i -a $$i != proc.cf ];then\
			cd  $$i;\
			echo "====== $(MAKE) -f $$i.mk" ; \
			$(MAKE) -f $$i.mk $(MAKEARGS) ; \
			cd .. ; \
		fi;\
	done

depend:: makedep
	@for i in `ls`;\
	do\
		if [ -d $$i -a $$i != proc.cf ];then\
			cd  $$i;\
			echo "====== $(MAKE) -f $$i.mk depend" ; \
			$(MAKE) -f $$i.mk depend MAKEFILE=$$i.mk $(MAKEARGS) ; \
			cd .. ; \
		fi;\
	done

clean:
	-rm -f *.o
	@for i in `ls`; \
	do \
		if [ -d $$i -a $$i != proc.cf ]; then \
			cd $$i; \
			$(MAKE) -f $$i.mk clean $(MAKEARGS); \
			cd ..; \
		fi; \
	done

clobber:	clean
	@for i in `ls`; \
	do \
		if [ -d $$i -a $$i != proc.cf ]; then \
			cd $$i; \
			$(MAKE) -f $$i.mk clobber $(MAKEARGS); \
			cd ..; \
		fi; \
	done
	-$(IDINSTALL) -e -R$(CONF) -d proc

headinstall: \
	$(KBASE)/proc/acct.h \
	$(KBASE)/proc/auxv.h \
	$(KBASE)/proc/class.h \
	$(KBASE)/proc/cred.h \
	$(KBASE)/proc/disp.h \
	$(KBASE)/proc/exec.h \
	$(KBASE)/proc/lock.h \
	$(KBASE)/proc/mman.h \
	$(KBASE)/proc/priocntl.h \
	$(KBASE)/proc/proc.h \
	$(KBASE)/proc/procset.h \
	$(KBASE)/proc/reg.h \
	$(KBASE)/proc/regset.h \
	$(KBASE)/proc/seg.h \
	$(KBASE)/proc/session.h \
	$(KBASE)/proc/siginfo.h \
	$(KBASE)/proc/signal.h \
	$(KBASE)/proc/swnotify.h \
	$(KBASE)/proc/tss.h \
	$(KBASE)/proc/ucontext.h \
	$(KBASE)/proc/unistd.h \
	$(KBASE)/proc/user.h \
	$(KBASE)/proc/wait.h \
	$(FRC)
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/acct.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/auxv.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/class.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/cred.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/disp.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/exec.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/lock.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/mman.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/priocntl.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/proc.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/procset.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/reg.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/regset.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/seg.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/session.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/siginfo.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/signal.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/swnotify.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/tss.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/ucontext.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/unistd.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/user.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/wait.h
	@for i in `ls`; \
	do \
		if [ -d $$i -a $$i != proc.cf ]; then \
			cd $$i; \
			$(MAKE) -f $$i.mk headinstall $(MAKEARGS); \
			cd ..; \
		fi; \
	done



FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

