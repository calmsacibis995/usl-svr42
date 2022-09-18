#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:vpix/vpix.mk	1.9"
#ident	"$Header: vpix.mk 1.4 91/03/21 $"


include $(UTSRULES)

KBASE    = ..
LOCALDEF =
CNFFILE  = $(CONF)/pack.d/vx/Driver.o
DFILES	 = v86gptrap.o v86subr.o  v86.o

CFILES =  \
	v86subr.c \
	v86.c

all:	ID $(CNFFILE)

ID:	
	cd vpix.cf; $(IDINSTALL) -R$(CONF) -M vx

$(CNFFILE):	$(DFILES)
	$(LD) -r -o $@ $(DFILES)

headinstall: \
		$(KBASE)/vpix/v86.h \
		$(FRC)
		$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/vpix/v86.h

v86gptrap.o: v86gptrap.s $(KBASE)/util/symbols.m4
	$(M4) -B16384 $(LOCALDEF) $(GLOBALDEF) v86gptrap.s > v86.tmp.s
	$(AS) -o v86gptrap.o v86.tmp.s
	rm -f v86.tmp.s

$(KBASE)/util/symbols.m4:
	cd $(KBASE)/util ; $(MAKE) -f util.mk symbols.m4 $(MAKEARGS)

clean:
		-rm -f *.o

clobber: 	clean
	$(IDINSTALL) -e -R$(CONF) -d vx

FRC: 
 
include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

