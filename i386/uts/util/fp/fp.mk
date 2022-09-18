#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:util/fp/fp.mk	1.6"
#ident	"$Header: $"

include $(UTSRULES)

KBASE = ../..

INSDIR = $(ROOT)/$(MACH)/sbin

OBJS =	dcode.o \
	arith.o \
	divmul.o \
	lipsq.o \
	reg.o \
	remsc.o \
	round.o \
	status.o \
	store.o \
	subadd.o \
	trans.o

all:	emulator.rel1
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) emulator.rel1 $(INSDIR) 744 bin bin
	-rm -f $(INSDIR)/emulator
	-rm -f $(ROOT)/$(MACH)/etc/emulator
	-rm -f $(ROOT)/$(MACH)/etc/emulator.rel1
	-ln $(INSDIR)/emulator.rel1 $(INSDIR)/emulator
	-ln $(INSDIR)/emulator $(ROOT)/$(MACH)/etc/emulator
	-ln $(INSDIR)/emulator.rel1 $(ROOT)/$(MACH)/etc/emulator.rel1

emulator.rel1: $(OBJS) 
	$(LD) -Mmapfile -dn -s -e e80387 -o $@ $(OBJS)

clean:
	-rm -f $(OBJS) fpsymbols.m4

clobber: clean
	-rm emulator.rel1

$(OBJS):	fpsymbols.m4  e80387.m4

.s.o:
	$(AS) $(ASFLAGS) -o $@ -- $(LOCALDEF) $(GLOBALDEF) $<

fpsymbols.m4: \
	fpsymbols.c \
	$(KBASE)/util/fp/sizes.h \
	$(KBASE)/proc/proc.h \
	$(KBASE)/proc/user.h
	$(CC) $(CFLAGS) $(DEFLIST) -S fpsymbols.c && \
	awk -f $(KBASE)/util/syms.awk < fpsymbols.s | \
	sed -e '1,$$s;__SYMBOL__;;' > fpsymbols.m4 && \
	rm -f fpsymbols.s

FRC: 
 
include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

