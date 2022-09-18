#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:util/mod/mod.mk	1.11"
#ident	"$Header: $"

include $(UTSRULES)

KBASE   = ../..
INSPERM = -m 644 -u $(OWN) -g $(GRP)
MOD	= $(CONF)/pack.d/mod/Driver.o
MODKSYM = $(CONF)/pack.d/modksym/Driver.o
FILES = \
	mod_obj.o \
	mod_objmd.o \
	modadm.o \
	modctl.o \
	mod_drv.o \
	mod_str.o \
	mod_fs.o \
	mod_misc.o \
	mod_intr.o 

CFILES = $(FILES:.o=.c) mod_ksym.c modinit.c


all:	ID $(MOD) $(MODKSYM)

$(MOD):	$(FILES)
	$(LD) -r -o $@ $(FILES)

$(MODKSYM):	mod_ksym.o modinit.o
	$(LD) -r -o $@ mod_ksym.o modinit.o
#
# Configuration Section
#
ID:
	cd mod.cf; $(IDINSTALL) -R$(CONF) -M mod 
	cd modksym.cf; $(IDINSTALL) -R$(CONF) -M modksym


clean:
	-rm -f $(FILES) mod_ksym.o modinit.o

clobber:	clean
	-$(IDINSTALL) -e -R$(CONF) -d mod
	-$(IDINSTALL) -e -R$(CONF) -d modksym

headinstall: \
	$(KBASE)/util/mod/ksym.h \
	$(KBASE)/util/mod/mod.h \
	$(KBASE)/util/mod/mod_intr.h \
	$(KBASE)/util/mod/mod_k.h \
	$(KBASE)/util/mod/mod_obj.h \
	$(KBASE)/util/mod/moddefs.h \
	$(KBASE)/util/mod/moddrv.h \
	$(KBASE)/util/mod/modfs.h \
	$(KBASE)/util/mod/module.h \
	$(FRC)
	[ -d $(INC)/sys ] || mkdir $(INC)/sys
	$(INS) -f $(INC)/sys $(INSPERM) $(KBASE)/util/mod/ksym.h
	$(INS) -f $(INC)/sys $(INSPERM) $(KBASE)/util/mod/mod.h
	$(INS) -f $(INC)/sys $(INSPERM) $(KBASE)/util/mod/mod_intr.h
	$(INS) -f $(INC)/sys $(INSPERM) $(KBASE)/util/mod/mod_k.h
	$(INS) -f $(INC)/sys $(INSPERM) $(KBASE)/util/mod/mod_obj.h
	$(INS) -f $(INC)/sys $(INSPERM) $(KBASE)/util/mod/moddefs.h
	$(INS) -f $(INC)/sys $(INSPERM) $(KBASE)/util/mod/moddrv.h
	$(INS) -f $(INC)/sys $(INSPERM) $(KBASE)/util/mod/modfs.h
	$(INS) -f $(INC)/sys $(INSPERM) $(KBASE)/util/mod/module.h


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

