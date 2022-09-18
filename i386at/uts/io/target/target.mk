#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86at:io/target/target.mk	1.13"
#ident  "$Header: $"

include $(UTSRULES)

KBASE = ../..

LOCALDEF = -D_LTYPES -DSYSV -DSVR40 -D_SYSTEMENV=4

GRP = bin
OWN = bin
HINSPERM = -m 644 -u $(OWN) -g $(GRP)
INSPERM	= -m 644 -u root -g sys

HEADERS = altsctr.h \
	  alttbl.h \
	  badsec.h \
	  cd_ioctl.h \
	  dynstructs.h \
	  fdisk.h \
	  mirror.h \
	  sc01.h \
	  scl.h \
	  scsi.h \
	  scsicomm.h \
	  sd01.h \
	  sd01_ioctl.h \
	  sdi.h \
	  sdi_edt.h \
	  st01.h \
	  sw01.h \
	  tape.h \
	  tokens.h \
	  worm.h

OBJFILES= sc01.o sdi.o sd01.o st01.o sw01.o

CFILES = $(OBJFILES:.o=.c) conf.c


DRVRFILES =\
	$(CONF)/pack.d/sc01/Driver.o	\
	$(CONF)/pack.d/sd01/Driver.o	\
	$(CONF)/pack.d/st01/Driver.o	\
	$(CONF)/pack.d/sw01/Driver.o	\
	$(CONF)/pack.d/sdi/Driver.o

all:	ID $(DRVRFILES)
	echo "**** Target build completed ****" > /dev/null

ID:	
	cd sc01.cf; $(IDINSTALL) -R$(CONF) -M sc01
	cd sd01.cf; $(IDINSTALL) -R$(CONF) -M sd01
	cd st01.cf; $(IDINSTALL) -R$(CONF) -M st01
	cd sw01.cf; $(IDINSTALL) -R$(CONF) -M sw01
	(cd sdi.cf; \
	$(IDINSTALL) -R$(CONF) -M sdi; \
	$(INS) -f $(CONF)/pack.d/sdi $(INSPERM) space.gen )
	
clean:
	rm -f *.o

clobber: clean
	$(IDINSTALL) -R$(CONF) -e -d sc01
	$(IDINSTALL) -R$(CONF) -e -d sd01
	$(IDINSTALL) -R$(CONF) -e -d st01
	$(IDINSTALL) -R$(CONF) -e -d sw01
	$(IDINSTALL) -R$(CONF) -e -d sdi

$(CONF)/pack.d/sc01/Driver.o:	sc01.o
	$(LD) -r -o $@ sc01.o

$(CONF)/pack.d/sdi/Driver.o: sdi.o  conf.o dynstructs.o
	$(LD) -r -o $@ sdi.o  conf.o dynstructs.o

$(CONF)/pack.d/sd01/Driver.o:	sd01.o
	$(LD) -r -o $@ sd01.o

$(CONF)/pack.d/st01/Driver.o:	st01.o
	$(LD) -r -o $@ st01.o

$(CONF)/pack.d/sw01/Driver.o:	sw01.o
	$(LD) -r -o $@ sw01.o

dynstructs.o sd01.o sdi.o sto1.o:
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

