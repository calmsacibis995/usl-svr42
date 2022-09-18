#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucblib/libtermcap/libtermcap.mk	1.3"
#ident	"$Header: $"
#		PROPRIETARY NOTICE (Combined)
#
#This source code is unpublished proprietary information
#constituting, or derived under license from AT&T's UNIX(r) System V.
#In addition, portions of such source code were derived from Berkeley
#4.3 BSD under license from the Regents of the University of
#California.
#
#
#
#		Copyright Notice 
#
#Notice of copyright on this source code product does not indicate 
#publication.
#
#	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
#	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#	          All rights reserved.


#
# makefile for libtermcap.a
#
#

include $(LIBRULES)

CFLAGS= -O -DM_N -DCM_GT -DCM_B -DCM_D
PROF=
NONPROF=
INC1=$(ROOT)/$(MACH)/usr/ucbinclude
INCSYS=$(INC)/sys
INCSYS1=$(INC1)/sys
SDEFLIST=
OWN=bin
GRP=bin
INSDIR=$(ROOT)/$(MACH)/usr/ucblib
ARFLAGS = q

OBJECTS =  termcap.o tgoto.o tputs.o

SOURCES =  termcap.c tgoto.c tputs.c

ALL:	libtermcap.a

all:	ALL

termcap.o: $(INC)/ctype.h $(INCSYS1)/ioctl.h
		$(CC)  $(CFLAGS) -I. -I$(INC1) -c termcap.c
 
tgoto.o: 

tputs.o:	$(INC1)/sgtty.h $(INC)/ctype.h
		$(CC) $(CFLAGS) -I$(INC) -c tputs.c

GLOBALINCS = $(INC)/ctype.h \
	$(INC1)/sgtty.h \
	$(INCSYS1)/ioctl.h 

libtermcap.a: $(OBJECTS)
	$(AR) $(ARFLAGS) libtermcap.a `$(LORDER) $(OBJECTS) | $(TSORT)`

install: ALL
	$(INS) -f $(INSDIR) -m 644 -u $(OWN) -g $(GRP) libtermcap.a
	rm -f $(INSDIR)/libtermlib.a
	ln $(INSDIR)/libtermcap.a $(INSDIR)/libtermlib.a

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) libtermcap.a
