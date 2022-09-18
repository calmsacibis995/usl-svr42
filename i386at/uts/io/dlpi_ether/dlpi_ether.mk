#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86at:io/dlpi_ether/dlpi_ether.mk	1.9"
#ident	"$Header: $"

include $(UTSRULES)

LOCALDEF = -DDL_STRLOG 

KBASE =		../..
IE6 =		$(CONF)/pack.d/ie6/Driver.o
IMX586 =	$(CONF)/pack.d/imx586/Driver.o
EE16 =		$(CONF)/pack.d/ee16/Driver.o
EL16 =		$(CONF)/pack.d/el16/Driver.o
I596 =		$(CONF)/pack.d/i596/Driver.o
WD =		$(CONF)/pack.d/wd/Driver.o
IE6FILES = \
	dlpi_ie6.o \
	ie6hrdw.o
IMX586FILES = \
	dlpi_imx586.o \
	imx586hrdw.o \
	imx586bcopy.o
EE16FILES = \
	dlpi_ee16.o \
	ee16hrdw.o \
	ee16init.o
EL16FILES = \
	dlpi_el16.o \
	el16hrdw.o \
	el16init.o
I596FILES = \
	dlpi_i596.o \
	i596hrdw.o \
	i596init.o
WDFILES = \
	dlpi_wd.o \
	wdhrdw.o \
	wdbcopy.o

CFILES =  \
	ie6hrdw.c \
	imx586hrdw.c \
	ee16hrdw.c \
	ee16init.c \
	el16hrdw.c \
	el16init.c \
	i596hrdw.c \
	i596init.c \
	wdhrdw.c

SFILES =  \
	imx586bcopy.s \
	wdbcopy.s

#all:	ID $(IE6) $(IMX586) $(EE16) $(EL16) $(WD) $(I596)
all:	ID $(IE6) $(IMX586) $(EE16) $(EL16) $(WD)

$(IE6):	$(IE6FILES)
	$(LD) -r -o $@ $(IE6FILES)

$(IMX586):	$(IMX586FILES)
	$(LD) -r -o $@ $(IMX586FILES)

$(EE16):	$(EE16FILES)
	$(LD) -r -o $@ $(EE16FILES)

$(EL16):	$(EL16FILES)
	$(LD) -r -o $@ $(EL16FILES)

$(I596):	$(I596FILES)
	$(LD) -r -o $@ $(I596FILES)

$(WD):	$(WDFILES)
	$(LD) -r -o $@ $(WDFILES)

#
# Configuration Section
#
ID:
	cd ie6.cf; $(IDINSTALL) -R$(CONF) -M ie6
	cd imx586.cf; $(IDINSTALL) -R$(CONF) -M imx586
	cd ee16.cf; $(IDINSTALL) -R$(CONF) -M ee16
	cd el16.cf; $(IDINSTALL) -R$(CONF) -M el16
#	cd i596.cf; $(IDINSTALL) -R$(CONF) -M i596
	cd wd.cf; $(IDINSTALL) -R$(CONF) -M wd

#
# Header Install Section
#
headinstall: \
	$(KBASE)/io/dlpi_ether/dlpi_ether.h \
	$(KBASE)/io/dlpi_ether/dlpi_ie6.h \
	$(KBASE)/io/dlpi_ether/ie6.h \
	$(KBASE)/io/dlpi_ether/dlpi_imx586.h \
	$(KBASE)/io/dlpi_ether/imx586.h \
	$(KBASE)/io/dlpi_ether/dlpi_ee16.h \
	$(KBASE)/io/dlpi_ether/ee16.h \
	$(KBASE)/io/dlpi_ether/dlpi_el16.h \
	$(KBASE)/io/dlpi_ether/el16.h \
	$(KBASE)/io/dlpi_ether/dlpi_i596.h \
	$(KBASE)/io/dlpi_ether/i596.h \
	$(KBASE)/io/dlpi_ether/dlpi_wd.h \
	$(KBASE)/io/dlpi_ether/wd.h \
	$(FRC)
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/dlpi_ether/dlpi_ether.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/dlpi_ether/dlpi_ie6.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/dlpi_ether/ie6.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/dlpi_ether/dlpi_imx586.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/dlpi_ether/imx586.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/dlpi_ether/dlpi_ee16.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/dlpi_ether/ee16.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/dlpi_ether/dlpi_el16.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/dlpi_ether/el16.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/dlpi_ether/dlpi_i596.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/dlpi_ether/i596.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/dlpi_ether/dlpi_wd.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/dlpi_ether/wd.h

clean:
	-rm -f *.o

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d ie6 
	-$(IDINSTALL) -R$(CONF) -e -d imx586
	-$(IDINSTALL) -R$(CONF) -e -d ee16
	-$(IDINSTALL) -R$(CONF) -e -d el16
	-$(IDINSTALL) -R$(CONF) -e -d i596
	-$(IDINSTALL) -R$(CONF) -e -d wd

#
# Special Header dependencies
#

dlpi_ie6.o: FRC
	$(CC) $(CFLAGS) $(DEFLIST) -DIE6 -c dlpi_ether.c && mv dlpi_ether.o dlpi_ie6.o

dlpi_imx586.o: FRC
	$(CC) $(CFLAGS) $(DEFLIST) -DIMX586 -c dlpi_ether.c && mv dlpi_ether.o dlpi_imx586.o

dlpi_ee16.o: FRC
	$(CC) $(CFLAGS) $(DEFLIST) -DEE16 -c dlpi_ether.c && mv dlpi_ether.o dlpi_ee16.o

dlpi_el16.o: FRC
	$(CC) $(CFLAGS) $(DEFLIST) -DEL16 -c dlpi_ether.c && mv dlpi_ether.o dlpi_el16.o

dlpi_i596.o: FRC
	$(CC) $(CFLAGS) $(DEFLIST) -DI596 -c dlpi_ether.c && mv dlpi_ether.o dlpi_i596.o

dlpi_wd.o: FRC
	$(CC) $(CFLAGS) $(DEFLIST) -DWD -c dlpi_ether.c && mv dlpi_ether.o dlpi_wd.o


FRC: 
 
include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

