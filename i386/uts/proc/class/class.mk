#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:proc/class/class.mk	1.12"
#ident	"$Header: $"

include $(UTSRULES)

KBASE = ../..
RT = $(CONF)/pack.d/rt/Driver.o
TS = $(CONF)/pack.d/ts/Driver.o
VC = $(CONF)/pack.d/vc/Driver.o
FC = $(CONF)/pack.d/fc/Driver.o

FILE = ts.o rt.o vc.o fc.o

CFILES = $(FILE:.o=.c)

all:	ID ../sysclass.o $(RT) $(TS) $(VC) $(FC)

ID:
	cd ts.cf; $(IDINSTALL) -R$(CONF) -M ts
	cd rt.cf; $(IDINSTALL) -R$(CONF) -M rt
	cd vc.cf; $(IDINSTALL) -R$(CONF) -M vc
	cd fc.cf; $(IDINSTALL) -R$(CONF) -M fc

$(TS): ts.o
	$(LD) -r -o $(TS) ts.o

$(RT): rt.o
	$(LD) -r -o $(RT) rt.o

$(VC): vc.o
	$(LD) -r -o $(VC) vc.o

$(FC): fc.o
	$(LD) -r -o $(FC) fc.o

clean:
	-rm -f rt.o ts.o vc.o fc.o

clobber:	clean
	-rm -f ../sysclass.o $(RT) $(TS) $(VC) $(FC)
	-$(IDINSTALL) -e -R$(CONF) -d ts
	-$(IDINSTALL) -e -R$(CONF) -d rt
	-$(IDINSTALL) -e -R$(CONF) -d vc
	-$(IDINSTALL) -e -R$(CONF) -d fc

headinstall: \
	$(KBASE)/proc/class/rt.h \
	$(KBASE)/proc/class/rtpriocntl.h \
	$(KBASE)/proc/class/ts.h \
	$(KBASE)/proc/class/tspriocntl.h \
	$(KBASE)/proc/class/vc.h \
	$(KBASE)/proc/class/vcpriocntl.h \
	$(KBASE)/proc/class/fc.h \
	$(KBASE)/proc/class/fcpriocntl.h \
	$(FRC)
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/class/rt.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/class/rtpriocntl.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/class/ts.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/class/tspriocntl.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/class/vc.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/class/vcpriocntl.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/class/fc.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/class/fcpriocntl.h

#
# Special dependencies
#
../sysclass.o: FRC
	$(CC) $(DEFLIST) $(CFLAGS) -c sysclass.c
	mv sysclass.o ../sysclass.o 	# to be linked to proc.o later

FRC: 
 
include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

