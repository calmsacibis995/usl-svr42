#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:net/tcpip/tcp.mk	1.6"
#ident 	"$Header: $"
#
#  		PROPRIETARY NOTICE (Combined)
#  
#  This source code is unpublished proprietary information
#  constituting, or derived under license from AT&T's Unix(r) System V.
#  In addition, portions of such source code were derived from Berkeley
#  4.3 BSD under license from the Regents of the University of
#  California.
#  
#  
#  
#  		Copyright Notice 
#  
#  Notice of copyright on this source code product does not indicate 
#  publication.
#  
#  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
#  	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#  	          All rights reserved.
#

include $(UTSRULES)

KBASE    = ../..
LOCALDEF = -DSYSV -DSLIP
INSPERM  = -m 644 -u $(OWN) -g $(GRP)
TCP = $(CONF)/pack.d/tcp/Driver.o
OBJ = tcp_debug.o tcp_input.o tcp_main.o tcp_output.o tcp_state.o \
	tcp_subr.o tcp_timer.o

CFILES = $(OBJ:.o=.c)


CLEAN = tcp.o $(OBJ)


all : ID $(TCP)

ID:
	cd tcp.cf; $(IDINSTALL) -R$(CONF) -M tcp


$(TCP): $(OBJ)
	$(LD) -r -o $@ $(OBJ)

clean:
		-rm -f $(CLEAN)

clobber:	clean
	$(IDINSTALL) -e -R$(CONF) -d tcp

headinstall: \
	$(KBASE)/net/tcpip/nihdr.h \
	$(KBASE)/net/tcpip/route.h \
	$(KBASE)/net/tcpip/strioc.h \
	$(KBASE)/net/tcpip/symredef.h \
	$(KBASE)/net/tcpip/tcp.h \
	$(KBASE)/net/tcpip/tcp_debug.h \
	$(KBASE)/net/tcpip/tcp_fsm.h \
	$(KBASE)/net/tcpip/tcp_seq.h \
	$(KBASE)/net/tcpip/tcp_timer.h \
	$(KBASE)/net/tcpip/tcp_var.h \
	$(KBASE)/net/tcpip/tcpip.h \
	$(FRC)
	$(INS) -f $(INC)/netinet -m 644 $(INSPERM)  $(KBASE)/net/tcpip/nihdr.h
	$(INS) -f $(INC)/net     -m 644 $(INSPERM)  $(KBASE)/net/tcpip/route.h
	$(INS) -f $(INC)/net     -m 644 $(INSPERM)  $(KBASE)/net/tcpip/strioc.h
	$(INS) -f $(INC)/netinet -m 644 $(INSPERM)  $(KBASE)/net/tcpip/symredef.h
	$(INS) -f $(INC)/netinet -m 644 $(INSPERM)  $(KBASE)/net/tcpip/tcp.h
	$(INS) -f $(INC)/netinet -m 644 $(INSPERM)  $(KBASE)/net/tcpip/tcp_debug.h
	$(INS) -f $(INC)/netinet -m 644 $(INSPERM)  $(KBASE)/net/tcpip/tcp_fsm.h
	$(INS) -f $(INC)/netinet -m 644 $(INSPERM)  $(KBASE)/net/tcpip/tcp_seq.h
	$(INS) -f $(INC)/netinet -m 644 $(INSPERM)  $(KBASE)/net/tcpip/tcp_timer.h
	$(INS) -f $(INC)/netinet -m 644 $(INSPERM)  $(KBASE)/net/tcpip/tcp_var.h
	$(INS) -f $(INC)/netinet -m 644 $(INSPERM)  $(KBASE)/net/tcpip/tcpip.h


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

