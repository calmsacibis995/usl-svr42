#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86at:util/util.mk	1.28"
#ident 	"$Header: $:

include $(UTSRULES)

KBASE    = ..
UTFILE	 = $(CONF)/pack.d/util/Driver.o
FPFILE	 = $(CONF)/pack.d/fp/Driver.o
FILES = \
	bitmap.o \
	bitmasks.o \
	compat.o \
	ldivide.o \
	list.o \
	lmul.o \
	malloc.o \
	sysinfo.o \
	vm_mp.o \
	cmn_err.o \
	ladd.o \
	lsub.o \
	lshiftl.o \
	lsign.o \
	kperf.o \
	machdep.o \
	misc.o \
	string.o \
	subr.o\
	bs.o \
	weitek.o

CFILES =  \
	bitmap.c  \
	bitmasks.c  \
	compat.c  \
	ldivide.c  \
	list.c  \
	lmul.c  \
	malloc.c  \
	sysinfo.c  \
	vm_mp.c  \
	cmn_err.c  \
	kperf.c  \
	machdep.c  \
	subr.c \
	symbols.c \
	bs.c 


SFILES =  \
	ladd.s  \
	lsub.s  \
	lshiftl.s  \
	lsign.s  \
	string.s  \
	weitek.s



all:	ID dir $(UTFILE) $(FPFILE)

dir:
	@for i in `ls | grep -v '\.cf$$'`;\
	do\
		if [ -d $$i ]; then\
			cd  $$i;\
			echo "====== $(MAKE) -f $$i.mk" ; \
			$(MAKE) -f $$i.mk $(MAKEARGS) ; \
			cd .. ; \
		fi;\
	done

depend:: makedep
	@for i in `ls | grep -v '\.cf$$'`;\
	do\
		if [ -d $$i ]; then\
			cd  $$i;\
			echo "====== $(MAKE) -f $$i.mk depend" ; \
			$(MAKE) -f $$i.mk depend MAKEFILE=$$i.mk $(MAKEARGS) ; \
			cd .. ; \
		fi;\
	done


$(UTFILE):	$(FILES)
		$(LD) -r -o $(UTFILE) $(FILES)

$(FPFILE):	fp.o fpmdep.o
		$(LD) -r -o $(FPFILE) fp.o fpmdep.o

#
# Configuration Section
#
ID:
	cd util.cf; $(IDINSTALL) -M -R$(CONF) util
	cd fp.cf;   $(IDINSTALL) -M -R$(CONF) fp

#
# Cleanup Section
#
clean:
	-rm -f *.o symbols.m4
	@for i in `ls | grep -v '\.cf$$'`;\
	do \
		if [ -d $$i ]; then \
			cd $$i; \
			$(MAKE) -f $$i.mk clean $(MAKEARGS); \
			cd ..; \
		fi; \
	done

clobber:	clean
	@for i in `ls | grep -v '\.cf$$'`;\
	do \
		if [ -d $$i ]; then \
			cd $$i; \
			$(MAKE) -f $$i.mk clobber $(MAKEARGS); \
			cd ..; \
		fi; \
	done
	$(IDINSTALL) -e -R$(CONF) -d util
	$(IDINSTALL) -e -R$(CONF) -d fp



sysHeader = \
	$(KBASE)/util/bitmap.h \
	$(KBASE)/util/bitmasks.h \
	$(KBASE)/util/cmn_err.h \
	$(KBASE)/util/debug.h \
	$(KBASE)/util/debugreg.h \
	$(KBASE)/util/dl.h \
	$(KBASE)/util/err.h \
	$(KBASE)/util/ertyp.h \
	$(KBASE)/util/fault.h \
	$(KBASE)/util/inline.h \
	$(KBASE)/util/ipl.h \
	$(KBASE)/util/list.h \
	$(KBASE)/util/macro.h \
	$(KBASE)/util/map.h \
	$(KBASE)/util/pfdat.h \
	$(KBASE)/util/param.h \
	$(KBASE)/util/region.h \
	$(KBASE)/util/sysinfo.h \
	$(KBASE)/util/sysmacros.h \
	$(KBASE)/util/types.h \
	$(KBASE)/util/var.h \
	$(KBASE)/util/fp.h \
	$(KBASE)/util/kdb/kdebugger.h

vmHeader = \
	$(KBASE)/util/cpu.h \
	$(KBASE)/util/debugger.h \
	$(KBASE)/util/mp.h

headinstall: $(sysHeader) $(vmHeader) $(FRC)
	[ -d $(INC)/sys ] || mkdir $(INC)/sys
	@for file in $(sysHeader);\
	do\
		$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $$file; \
	done
	[ -d $(INC)/vm  ] || mkdir $(INC)/vm
	@for file in $(vmHeader);\
	do\
		$(INS) -f $(INC)/vm -m 644 -u $(OWN) -g $(GRP) $$file; \
	done
	cd kdb ; $(MAKE) -f kdb.mk headinstall $(MAKEARGS) ; cd ..
	cd mod ; $(MAKE) -f mod.mk headinstall $(MAKEARGS) ; cd ..
	cd merge ; $(MAKE) -f merge.mk headinstall $(MAKEARGS) ; cd ..
	cd weitek ; $(MAKE) -f weitek.mk headinstall $(MAKEARGS) ; cd ..

.s.o:
	$(AS) $(ASFLAGS) -o $@ -- $(LOCALDEF) $(GLOBALDEF) $<

#
# Special Header dependencies 
#
misc.o: 	spl.o in_outb.o _misc.o
	$(LD) -r -o $@ spl.o in_outb.o _misc.o

_misc.o: misc.s symbols.m4
	$(AS) $(ASFLAGS) -o misc.o -- $(LOCALDEF) $(GLOBALDEF) misc.s && \
	mv misc.o _misc.o

spl.o: spl.s symbols.m4

in_outb.o: in_outb.s symbols.m4

symbols.m4:  symbols.o 
	rm -f symbols.o
	$(CC) $(CFLAGS) $(DEFLIST) -S symbols.c && \
	awk -f syms.awk < symbols.s | \
	sed -e '1,$$s;__SYMBOL__;;' > symbols.m4 && \
	rm -f symbols.s

fp.o: 
	$(CC) $(CFLAGS) $(DEFLIST) -UDEBUG -c fp.c


FRC: 
 
include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

