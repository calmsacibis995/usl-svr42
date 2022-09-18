#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86at:io/hba/hba.mk	1.15"
#ident	"$Header: $"

include $(UTSRULES)

KBASE = ../..

LOCALDEF = -D_LTYPES -DSYSV -DSVR40 -D_SYSTEMENV=4

GRP = bin
OWN = bin
HINSPERM = -m 644 -u $(OWN) -g $(GRP)
INSPERM = -m 644 -u root -g sys

HEADERS = ad1542.h \
	  aha.h \
	  athd.h \
	  dcd.h \
	  dpt.h \
	  dpt_sdi.h \
	  gendev.h \
	  gendisk.h \
	  gentape.h \
	  had.h \
	  ict.h \
	  mcdma.h \
	  mcesdi.h \
	  mcis.h \
	  mcst.h \
	  wd7000.h

OBJFILES = \
	ad1542.o \
	athd.o \
	dpt.o \
	dpt_a.o \
	mcesdi.o \
	mcis.o \
	mcst.o \
	wd7000.o \
	dcd.o \
	ict.o \
	athd_stub.o \
	ict_stub.o \
	mcesdi_stub.o \
	mcst_stub.o

CFILES =  \
	ad1542.c \
	athd.c \
	dcdhlpr.c \
	dpt.c \
	gendev.c \
	mcesdi.c \
	mcis.c \
	mcst.c \
	wd7000.c \
	dcd.c \
	ict.c


SFILES =  \
	dpt_a.s \
	athd_stub.s \
	ict_stub.s \
	mcesdi_stub.s \
	mcst_stub.s

.s.o:
	$(AS) -m $<

DRVRFILES =\
	$(CONF)/pack.d/athd/Driver.o	\
	$(CONF)/pack.d/dcd/Driver.o	\
	$(CONF)/pack.d/adsc/Driver.o	\
	$(CONF)/pack.d/dpt/Driver.o	\
	$(CONF)/pack.d/ict/Driver.o	\
	$(CONF)/pack.d/wd7000/Driver.o	\
	$(CONF)/pack.d/mcst/Driver.o	\
	$(CONF)/pack.d/mcis/Driver.o	\
	$(CONF)/pack.d/mcesdi/Driver.o

MODSTUBS =\
	$(CONF)/pack.d/athd/Modstub.o    \
	$(CONF)/pack.d/mcst/Modstub.o    \
	$(CONF)/pack.d/ict/Modstub.o     \
	$(CONF)/pack.d/mcesdi/Modstub.o


all:	ID $(DRVRFILES) $(MODSTUBS)
	echo "**** HBA build completed ****" > /dev/null

ID:	
	(cd dcd.cf; \
	$(IDINSTALL) -R$(CONF) -M dcd; \
	$(INS) -f $(CONF)/pack.d/dcd $(INSPERM) space.gen )
	(cd athd.cf; \
	$(IDINSTALL) -R$(CONF) -M athd; \
	$(INS) -f $(CONF)/pack.d/athd $(INSPERM) disk.cfg )
	(cd adsc.cf; \
	$(IDINSTALL) -R$(CONF) -M adsc; \
	$(INS) -f $(CONF)/pack.d/adsc $(INSPERM) disk.cfg )
	(cd dpt.cf; \
	$(IDINSTALL) -R$(CONF) -M dpt; \
	$(INS) -f $(CONF)/pack.d/dpt $(INSPERM) disk.cfg )
	(cd ict.cf; \
	$(IDINSTALL) -R$(CONF) -M ict; \
	$(INS) -f $(CONF)/pack.d/ict $(INSPERM) disk.cfg )
	(cd wd7000.cf; \
	$(IDINSTALL) -R$(CONF) -M wd7000; \
	$(INS) -f $(CONF)/pack.d/wd7000 $(INSPERM) disk.cfg )
	(cd mcesdi.cf; \
	$(IDINSTALL) -R$(CONF) -M mcesdi; \
	$(INS) -f $(CONF)/pack.d/mcesdi $(INSPERM) disk.cfg )
	(cd mcis.cf; \
	$(IDINSTALL) -R$(CONF) -M mcis; \
	$(INS) -f $(CONF)/pack.d/mcis $(INSPERM) disk.cfg )
	(cd mcst.cf; \
	$(IDINSTALL) -R$(CONF) -M mcst; \
	$(INS) -f $(CONF)/pack.d/mcst $(INSPERM) disk.cfg )

clean:
	rm -f *.o

clobber: clean
	$(IDINSTALL) -R$(CONF) -e -d adsc
	$(IDINSTALL) -R$(CONF) -e -d dpt
	$(IDINSTALL) -R$(CONF) -e -d wd7000
	$(IDINSTALL) -R$(CONF) -e -d dcd
	$(IDINSTALL) -R$(CONF) -e -d ict
	$(IDINSTALL) -R$(CONF) -e -d athd
	$(IDINSTALL) -R$(CONF) -e -d mcesdi
	$(IDINSTALL) -R$(CONF) -e -d mcis
	$(IDINSTALL) -R$(CONF) -e -d mcst

$(CONF)/pack.d/adsc/Driver.o:	ad1542.o
	$(LD) -r -o $@ ad1542.o

$(CONF)/pack.d/dpt/Driver.o:	dpt.o dpt_a.o
	$(LD) -r -o $@ dpt.o dpt_a.o

$(CONF)/pack.d/wd7000/Driver.o:	wd7000.o
	$(LD) -r -o $@ wd7000.o

$(CONF)/pack.d/dcd/Driver.o:	dcd.o dcdhlpr.o gendev.o
	$(LD) -r -o $@ dcd.o dcdhlpr.o gendev.o


$(CONF)/pack.d/athd/Driver.o:	athd.o
	$(LD) -r -o $@ athd.o

$(CONF)/pack.d/ict/Driver.o:	ict.o
	$(LD) -r -o $@ ict.o

$(CONF)/pack.d/mcesdi/Driver.o:	mcesdi.o 
	$(LD) -r -o $@ mcesdi.o 

$(CONF)/pack.d/mcis/Driver.o:	mcis.o 
	$(LD) -r -o $@ mcis.o 

$(CONF)/pack.d/mcst/Driver.o:	mcst.o 
	$(LD) -r -o $@ mcst.o 

$(CONF)/pack.d/athd/Modstub.o:	athd_stub.o
	$(LD) -r -o $@ athd_stub.o

$(CONF)/pack.d/ict/Modstub.o:	ict_stub.o
	$(LD) -r -o $@ ict_stub.o

$(CONF)/pack.d/mcesdi/Modstub.o:	mcesdi_stub.o
	$(LD) -r -o $@ mcesdi_stub.o

$(CONF)/pack.d/mcst/Modstub.o:	mcst_stub.o
	$(LD) -r -o $@ mcst_stub.o

ad1542.o athd.o dcd.o dcdhlpr.o dpt.o gendev.o mcesdi.o mcst.o wd7000.o:
	$(CC) $(CFLAGS) $(DEFLIST) -DDDI_OFF -c $<

headinstall:	
	@for i in $(HEADERS); \
	do \
		$(INS) -f $(INC)/sys $(HINSPERM) $$i; \
	done

FRC: 
 
include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

