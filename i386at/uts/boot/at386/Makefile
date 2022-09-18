#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


# Copyrighted as an unpublished work.
# (c) Copyright 1989 INTERACTIVE Systems Corporation
# All rights reserved.

# RESTRICTED RIGHTS

# These programs are supplied under a license.  They may be used,
# disclosed, and/or copied only as permitted under such license
# agreement.  Any copy must contain the above copyright notice and
# this restricted rights notice.  Use, copying, and/or disclosure
# of the programs is strictly prohibited unless otherwise provided
# in the license agreement.

#ident	"@(#)uts-x86at:boot/at386/Makefile	1.9"
#ident	"$Header: $"

include $(UTSRULES)

KBASE = ../..
INSDIR = $(ROOT)/$(MACH)/etc

ASFLAGS = -m

HDBOOTST = -DHDTST
BOOTLIB = ../bootlib/bootlib.o

OBJS =  boot.o ix_util.o util.o \
        printf.o gets.o ix_cutil.o  \
	load.o string.o 

CFILES =  \
	boot.c \
	printf.c \
	gets.c \
	ix_cutil.c \
	load.c \
	string.c


SFILES =  \
	ix_util.s \
	util.s
#
#	The following .c.o rule is being defined because we
#	do not with to use all the capabilities of the optimizer
#	Notice we do not include CFLAGS, but set our own
#	first-pass options.  This .c.o rule overides the one
#	in the uts rulefile.

.c.o:
	$(CC) -W0,-Lb $(DEFLIST) -c $<

# 	fpboot and fdboot make the floppy boot while wpboot and wdboot
#	make the hard disk boot.

.MUTEX: fdboot wdboot sips mips

all: fdboot wdboot sips mips 

depend:: makedep
	-@for i in mip sip;\
	do\
		if [ -d $$i ]; then\
			cd  $$i;\
			echo "====== $(MAKE) -f $$i.mk depend" ; \
			$(MAKE) -f $$i.mk depend MAKEFILE=$$i.mk $(MAKEARGS) ; \
			cd .. ; \
		fi;\
	done
	@cd  tool ; \
	echo "====== $(MAKE) -f tools.mk depend" ; \
	$(MAKE) -f tools.mk depend MAKEFILE=tools.mk $(MAKEARGS) 

tools:	
	cd tool; $(MAKE) -f tools.mk $(MAKEARGS)

sips: 
	cd sip; $(MAKE) -f sip.mk $(MAKEARGS)

mips: 
	cd mip; $(MAKE) -f mip.mk $(MAKEARGS)

fpboot: fpriboot.o piftmp tools
	if [ x${CCSTYPE} = xELF ] ; \
	then \
		${LD} -Mpiftmp -dn -o fpboot.obj fpriboot.o ; \
	else \
		${LD} -N -o fpboot.obj piftmp fpriboot.o ; \
	fi 
	tool/tdxtract fpboot.obj fpboot

wpboot: wpriboot.o piftmp tools
	if [ x${CCSTYPE} = xELF ] ; \
	then \
		${LD} -Mpiftmp -dn -o wpboot.obj wpriboot.o ; \
	else \
		${LD} -N -o wpboot.obj piftmp wpriboot.o ; \
	fi 
	tool/tdxtract wpboot.obj wpboot

hdpboot: hdpriboot.o piftmp tools
	if [ x${CCSTYPE} = xELF ] ; \
	then \
		${LD} -Mpiftmp -dn -o hdpboot.obj hdpriboot.o ; \
	else \
		${LD} -N -o hdpboot.obj piftmp hdpriboot.o ; \
	fi 
	tool/tdxtract hdpboot.obj hdpboot

fdboot: fbootcntl fsboot fpboot 
	cat fpboot fbootcntl fsboot >fdboot

fsboot: siftmp prot.o ftmpdisk.o tools $(OBJS)
	if [ x${CCSTYPE} = xELF ] ; \
	then \
		${LD} -Msiftmp -dn -o fsboot.obj prot.o ftmpdisk.o $(OBJS); \
	else \
		${LD} -N -o fsboot.obj siftmp prot.o ftmpdisk.o $(OBJS); \
	fi 
	tool/progconf fsboot.obj fsbt_pconf.h
	tool/tdxtract fsboot.obj fsboot

wdboot: wbootcntl wsboot wpboot 
	cat wpboot wbootcntl wsboot >wdboot

wsboot: siftmp prot.o wtmpdisk.o tools $(OBJS)
	if [ x${CCSTYPE} = xELF ] ; \
	then \
		${LD} -Msiftmp -dn -o wsboot.obj prot.o wtmpdisk.o $(OBJS) ; \
	else \
		${LD} -N -o wsboot.obj siftmp prot.o wtmpdisk.o $(OBJS) ; \
	fi 
	tool/progconf wsboot.obj wsbt_pconf.h
	tool/tdxtract wsboot.obj wsboot

#	hdboot is used only for testing hard disk boot without the 
#	need to install the boot program onto the hard disk
#	It is a mix of floppy primary and fixed disk secondary
#	boot programs. It can be loaded onto a floppy and used
#	to start up the kernel on the hard disk.

hdboot: wbootcntl hdsboot hdpboot
	cat hdpboot wbootcntl hdsboot > hdboot
	$(INS) -f $(INSDIR) -m 644 -u bin -g bin hdboot

hdsboot: siftmp prot.o hdtmpdisk.o tools $(OBJS)
	if [ x${CCSTYPE} = xELF ] ; \
	then \
		${LD} -Msiftmp -dn -o hdsboot.obj prot.o hdtmpdisk.o $(OBJS) ; \
	else \
		${LD} -N -o hdsboot.obj siftmp prot.o hdtmpdisk.o $(OBJS) ; \
	fi 
	tool/progconf hdsboot.obj hdsbt_pconf.h
	tool/tdxtract hdsboot.obj hdsboot

fbootcntl: bootcntl.c tools
	$(CC) -W0,-Lb $(DEFLIST) -c bootcntl.c
	if [ x${CCSTYPE} = xELF ] ; \
	then \
		${LD} -Mbcifile -dn -o bootcntl.obj bootcntl.o; \
	else \
		${LD} -o bootcntl.obj bootcntl.o; \
	fi 
	tool/tdxtract bootcntl.obj fbootcntl

wbootcntl: bootcntl.c tools
	$(CC) -W0,-Lb $(DEFLIST) -DWINI -c bootcntl.c
	if [ x${CCSTYPE} = xELF ] ; \
	then \
		${LD} -Mbcifile -dn -o bootcntl.obj bootcntl.o; \
	else \
		${LD} -o bootcntl.obj bootcntl.o; \
	fi 
	tool/tdxtract bootcntl.obj wbootcntl

piftmp:		pribtifile
	${M4} -D${CCSTYPE} pribtifile >piftmp

siftmp:		secbtifile
	${M4} -D${CCSTYPE} secbtifile >siftmp

fpriboot.o:     priboot.s fpriboot.m4
	${AS} $(ASFLAGS) priboot.s
	-/bin/mv priboot.o fpriboot.o

fpriboot.m4: fsboot
	-/bin/rm -f priboot.m4
	$(CC) -W0,-Lb $(DEFLIST) -S priboot_sym.c
	$(AWK) -f syms.awk < priboot_sym.s | \
	$(SED) -e '1,$$s;__SYMBOL__;;' >priboot.m4

wpriboot.o:     priboot.s wpriboot.m4
	${AS} $(ASFLAGS) -- -DWINI priboot.s
	-/bin/mv priboot.o wpriboot.o

wpriboot.m4: wsboot
	-/bin/rm -f priboot.m4
	$(CC) -W0,-Lb $(DEFLIST) -DWINI -S priboot_sym.c
	$(AWK) -f syms.awk < priboot_sym.s | \
	$(SED) -e '1,$$s;__SYMBOL__;;' >priboot.m4

hdpriboot.o:     priboot.s hdpriboot.m4
	${AS} $(ASFLAGS) -- $(HDBOOTST) priboot.s
	-/bin/mv priboot.o hdpriboot.o

hdpriboot.m4: hdsboot
	-/bin/rm -f priboot.m4
	$(CC) -W0,-Lb $(DEFLIST) $(HDBOOTST) -S priboot_sym.c
	$(AWK) -f syms.awk < priboot_sym.s | \
	$(SED) -e '1,$$s;__SYMBOL__;;' >priboot.m4

ftmpdisk.o:	fdisk.o $(BOOTLIB)
	${LD} -r -o ftmpdisk.o fdisk.o $(BOOTLIB)

wtmpdisk.o:	wdisk.o ix_alts.o $(BOOTLIB) 
	${LD} -r -o wtmpdisk.o wdisk.o ix_alts.o $(BOOTLIB) 

hdtmpdisk.o:	hddisk.o ix_alts.o $(BOOTLIB) 
	${LD} -r -o hdtmpdisk.o hddisk.o ix_alts.o $(BOOTLIB) 

prot.o: prot.s prot.m4
	${AS} $(ASFLAGS) prot.s

wdisk.o: disk.c				
	${CC} -W0,-Lb ${DEFLIST} -DWINI -c disk.c
	mv disk.o wdisk.o

hddisk.o: disk.c				
	${CC} -W0,-Lb -DWINI $(HDBOOTST) ${DEFLIST} -c disk.c
	mv disk.o hddisk.o

fdisk.o: disk.c	
	${CC} -W0,-Lb ${DEFLIST} -c disk.c
	mv disk.o fdisk.o
prot.m4:
	$(CC) -W0,-Lb $(DEFLIST) -S prot_sym.c
	$(AWK) -f syms.awk < prot_sym.s | \
	$(SED) -e '1,$$s;__SYMBOL__;;' >prot.m4

install: all
	$(INS) -f $(INSDIR) -m 644 -u bin -g bin fdboot
	$(INS) -f $(INSDIR) -m 644 -u bin -g bin wdboot
	mv $(INSDIR)/fdboot $(INSDIR)/.fboot
	mv $(INSDIR)/wdboot $(INSDIR)/.wboot
	ln $(INSDIR)/.fboot $(INSDIR)/fboot
	ln $(INSDIR)/.wboot $(INSDIR)/boot
	cd sip; $(MAKE) -f sip.mk install $(MAKEARGS)
	cd mip; $(MAKE) -f mip.mk install $(MAKEARGS)

clean:
	-/bin/rm -f *.o *.fd *.hd *.i *iftmp *iftmp.c *.obj *.m4 *_sym.s
	cd sip; $(MAKE) -f sip.mk clean $(MAKEARGS)
	cd mip; $(MAKE) -f mip.mk clean $(MAKEARGS)
	cd tool; $(MAKE) -f tools.mk clean $(MAKEARGS)

clobber: clean
	-/bin/rm -f ??boot ???boot ?bootcntl ?sbt_pconf.h
	cd sip; $(MAKE) -f sip.mk clobber $(MAKEARGS)
	cd mip; $(MAKE) -f mip.mk clobber $(MAKEARGS)
	cd tool; $(MAKE) -f tools.mk clobber $(MAKEARGS)
	-/bin/rm -f ${INSDIR}/.fboot
	-/bin/rm -f ${INSDIR}/.wboot
	-/bin/rm -f ${INSDIR}/fboot
	-/bin/rm -f ${INSDIR}/boot
	-/bin/rm -f ${INSDIR}/hdboot

FRC: 
 
include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

