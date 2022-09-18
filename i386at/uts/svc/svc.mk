#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86at:svc/svc.mk	1.28"
#ident 	"$Header: $"

include $(UTSRULES)

KBASE    = ..
#LOCALDEF = -D_PER_USER_LICENSE
SVC    = $(CONF)/pack.d/svc/Driver.o       
HRT    = $(CONF)/pack.d/hrt/Driver.o
PIC    = $(CONF)/pack.d/pic/Driver.o
PIT    = $(CONF)/pack.d/pit/Driver.o
NAME   = $(CONF)/pack.d/name/Driver.o
NMI    = $(CONF)/pack.d/nmi/Driver.o
# NOTE: Two files produced by this makefile, syms.o and start.o,
#	are not installed into $(CONF).  These are picked up by the
#	upper-level unix.mk as part of the `kernel' module.
MODULES = \
	$(SVC) $(NAME) $(HRT) $(PIC) $(PIT) $(NMI)

SVC_FILES = \
	clock.o \
	sysi86.o \
	cxenix.o \
	sco.o \
	isc.o \
	umem.o \
	intr.o \
	local.o \
	main.o \
	oem.o \
	oemsup.o \
	secsys.o \
	startup.o \
	sysent.o \
	timecalls.o \
	todc.o \
	trap.o \
	ttrap.o \
	uadmin.o \
	utssys.o \
	vstart.o \
	xsys.o\
	$(FRC)

CFILES =  \
	clock.c  \
	sysi86.c  \
	cxenix.c  \
	local.c  \
	main.c  \
	oem.c  \
	secsys.c  \
	startup.c  \
	sysent.c  \
	timecalls.c  \
	todc.c  \
	trap.c  \
	uadmin.c  \
	utssys.c  \
	xsys.c \
	tables1.c \
	hrtimers.c \
	hrtmdep.c 

HRT_FILES = \
	hrtimers.o \
	hrtmdep.o \
	$(FRC)


# Note: the order of STARTFILES is important and should be preserved.

STARTFILES = \
	uprt.o \
	tables2.o \
	tables1.o

all:	ID $(MODULES) dir syms.o start.o

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


$(SVC): $(SVC_FILES)
	$(LD) -r -o $(SVC) $(SVC_FILES)

$(HRT): $(HRT_FILES)
	$(LD) -r -o $(HRT) $(HRT_FILES)

$(NAME):	name.o
	$(LD) -r -o $@ name.o
	@rm -f name.o
	# remove to force a rebuild every time, to pick up RELEASE, VERSION

$(PIC):	pic.o
	$(LD) -r -o $@ pic.o

$(PIT):	pit.o
	$(LD) -r -o $@ pit.o

$(NMI):	nmi.o
	$(LD) -r -o $@ nmi.o

#
# Configuration Section
#
ID:
	cd hrt.cf;       $(IDINSTALL) -R$(CONF) -M hrt
	cd svc.cf;       $(IDINSTALL) -R$(CONF) -M svc
	cd pic.cf;       $(IDINSTALL) -R$(CONF) -M pic
	cd pit.cf;       $(IDINSTALL) -R$(CONF) -M pit
	cd name.cf;      $(IDINSTALL) -R$(CONF) -M name
	cd nmi.cf;       $(IDINSTALL) -R$(CONF) -M nmi
	cd rtx.cf;       $(IDINSTALL) -R$(CONF) -M rtx
	cd sysvendor.cf; $(IDINSTALL) -R$(CONF) -M sysvendor
	cd mpcntl.cf;    $(IDINSTALL) -R$(CONF) -M mpcntl
	cd modctl.cf;    $(IDINSTALL) -R$(CONF) -M modctl


#
# Special Section
#

$(KBASE)/util/symbols.m4: $(KBASE)/util/symbols.c
	cd $(KBASE)/util ; $(MAKE) -f util.mk symbols.m4 $(MAKEARGS)

.s.o:
	$(AS) -o $@ -m -- $(LOCALDEF) $(GLOBALDEF) $<

start.o: $(STARTFILES)
	$(LD) -r -o start.o $(STARTFILES)

#	Enhanced Application Compatibility Support

isc.o:	FRC

umem.o:	FRC

sco.o: FRC
	$(CC) $(CFLAGS) $(DEFLIST) -c sco.c \
		-DSCODATE=\"`date "+%y/%m/%d"`\"

#	End Enhanced Application Compatibility Support

syms.o:	syms.s

name.o: name.c \
	$(FRC)
	$(CC) $(CFLAGS) $(DEFLIST) -c name.c \
		-DRELEASE=`expr '"$(RELEASE)' : '\(..\{0,8\}\)'`\" \
		-DVERSION=`expr '"$(VERSION)' : '\(..\{0,8\}\)'`\"


uprt.o: uprt.s $(KBASE)/util/symbols.m4

tables2.o:	FRC
	$(CC) $(CFLAGS) $(DEFLIST) -S tables2.c
	sed '/^[	 ]*.data/c\
	.text\
	.align	8' tables2.s | \
	awk '  {if ($$1==".zero") { k=$$2 / 4; \
		for (i=1; i <= k; i++) print "\t.4byte\t0"; j=$$2 % 4; \
		for (i=1; i <= j; i++) print "\t.byte\t0" } else { print $0 }}' > tables2.tmp.s ; 
	$(CC) $(CFLAGS) $(DEFLIST) -c tables2.tmp.s && mv tables2.tmp.o tables2.o
	@rm -f tables2.tmp.[cso]

intr.o: intr.s $(KBASE)/util/symbols.m4

oemsup.o: oemsup.s

ttrap.o: ttrap.s $(KBASE)/util/symbols.m4

vstart.o: vstart.s $(KBASE)/util/symbols.m4

clean:
	-rm -f *.o *.tmp.[chs]
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
	$(IDINSTALL) -e -R$(CONF) -d svc
	$(IDINSTALL) -e -R$(CONF) -d hrt
	$(IDINSTALL) -e -R$(CONF) -d pic
	$(IDINSTALL) -e -R$(CONF) -d pit
	$(IDINSTALL) -e -R$(CONF) -d name
	$(IDINSTALL) -e -R$(CONF) -d nmi
	$(IDINSTALL) -e -R$(CONF) -d rtx
	$(IDINSTALL) -e -R$(CONF) -d sysvendor
	$(IDINSTALL) -e -R$(CONF) -d mpcntl
	$(IDINSTALL) -e -R$(CONF) -d modctl


#
# Head Install Section
Header = \
	$(KBASE)/svc/bootcntl.h \
	$(KBASE)/svc/brgmgr.h \
	$(KBASE)/svc/bootinfo.h \
	$(KBASE)/svc/callo.h \
	$(KBASE)/svc/clock.h \
	$(KBASE)/svc/eisa.h \
	$(KBASE)/svc/errno.h \
	$(KBASE)/svc/hrtcntl.h \
	$(KBASE)/svc/hrtsys.h \
	$(KBASE)/svc/info.h \
	$(KBASE)/svc/locking.h \
	$(KBASE)/svc/pic.h \
	$(KBASE)/svc/pit.h \
	$(KBASE)/svc/proctl.h \
	$(KBASE)/svc/resource.h \
	$(KBASE)/svc/sd.h \
	$(KBASE)/svc/secsys.h \
	$(KBASE)/svc/syscall.h \
	$(KBASE)/svc/sysconfig.h \
	$(KBASE)/svc/sysi86.h \
	$(KBASE)/svc/times.h \
	$(KBASE)/svc/trap.h \
	$(KBASE)/svc/uadmin.h \
	$(KBASE)/svc/ulimit.h \
	$(KBASE)/svc/utime.h \
	$(KBASE)/svc/utsname.h \
	$(KBASE)/svc/utssys.h \
	$(KBASE)/svc/systeminfo.h \
	$(KBASE)/svc/sysenvmt.h \
	$(KBASE)/svc/systm.h \
	$(KBASE)/svc/time.h \
	$(KBASE)/svc/timeb.h

vmHeader = \
	$(KBASE)/svc/reboot.h

headinstall: $(Header) $(vmHeader) $(FRC)
	[ -d $(INC)/sys ] || mkdir $(INC)/sys
	@for file in $(Header);\
	do\
		$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $$file; \
	done
	[ -d $(INC)/vm  ] || mkdir $(INC)/vm
	@for file in $(vmHeader);\
	do\
		$(INS) -f $(INC)/vm -m 644 -u $(OWN) -g $(GRP) $$file; \
	done

FRC: 
 
include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

