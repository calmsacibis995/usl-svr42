#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:proc/obj/obj.mk	1.9"
#ident	"$Header: $"

include $(UTSRULES)

KBASE = ../..
ELF = $(CONF)/pack.d/elf/Driver.o
COFF = $(CONF)/pack.d/coff/Driver.o
INTP = $(CONF)/pack.d/intp/Driver.o
I286X = $(CONF)/pack.d/i286x/Driver.o
DOSX = $(CONF)/pack.d/dosx/Driver.o
XOUT = $(CONF)/pack.d/xout/Driver.o

CFILES = coff.c \
	elf.c \
	i286x.c \
	intp.c \
	dosx.c \
	xout.c

all:	ID $(COFF) $(ELF) $(INTP) $(I286X) $(DOSX) $(XOUT)

ID:
	cd coff.cf; $(IDINSTALL) -R$(CONF) -M coff
	cd elf.cf; $(IDINSTALL) -R$(CONF) -M elf
	cd intp.cf; $(IDINSTALL) -R$(CONF) -M intp
	cd i286x.cf; $(IDINSTALL) -R$(CONF) -M i286x
	cd dosx.cf; $(IDINSTALL) -R$(CONF) -M dosx
	cd xout.cf; $(IDINSTALL) -R$(CONF) -M xout

$(COFF): coff.o
	$(LD) -r -o $(COFF) coff.o

$(ELF): elf.o
	$(LD) -r -o $(ELF) elf.o

$(INTP): intp.o
	$(LD) -r -o $(INTP) intp.o

$(I286X): i286x.o
	$(LD) -r -o $(I286X) i286x.o

$(DOSX): dosx.o
	$(LD) -r -o $(DOSX) dosx.o

$(XOUT): xout.o
	$(LD) -r -o $(XOUT) xout.o
clean:
	-rm -f *.o

clobber:	clean
	-$(IDINSTALL) -e -R$(CONF) -d coff
	-$(IDINSTALL) -e -R$(CONF) -d elf
	-$(IDINSTALL) -e -R$(CONF) -d intp
	-$(IDINSTALL) -e -R$(CONF) -d i286x
	-$(IDINSTALL) -e -R$(CONF) -d dosx
	-$(IDINSTALL) -e -R$(CONF) -d xout

headinstall: \
	$(KBASE)/proc/obj/elf.h \
	$(KBASE)/proc/obj/elf_386.h \
	$(KBASE)/proc/obj/elftypes.h \
	$(KBASE)/proc/obj/x.out.h \
	$(FRC)
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/obj/elf.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/obj/elf_386.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/obj/elftypes.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/obj/x.out.h


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

