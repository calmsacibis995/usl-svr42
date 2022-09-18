#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:acc/audit/audit.mk	1.10"
#ident "$Header: audit.mk 1.1 91/03/21 $"
                                       
include $(UTSRULES)                    

KBASE    = ../..
AUDIT    = $(CONF)/pack.d/audit/Driver.o

FILES =\
	auditbuf.o \
	auditctl.o \
	auditdmp.o \
	auditevt.o \
	auditerr.o \
	auditent.o \
	auditflush.o \
	auditlog.o \
	auditrec.o

CFILES = $(FILES:.o=.c)

all:	ID $(AUDIT)

$(AUDIT): $(FILES)
	$(LD) -r -o $@ $(FILES)

#
# Configuration Section
#
ID:
	cd audit.cf; $(IDINSTALL) -R$(CONF) -M audit


#
# Header install Section
#
headinstall: \
	$(KBASE)/acc/audit/audit.h \
	$(KBASE)/acc/audit/auditrec.h \
	$(KBASE)/acc/audit/auditmdep.h \
	$(FRC)
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/acc/audit/audit.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/acc/audit/auditrec.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/acc/audit/auditmdep.h


clean:
	-rm -f $(FILES)

clobber:	clean
	-$(IDINSTALL) -e -R$(CONF) -d audit



FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

