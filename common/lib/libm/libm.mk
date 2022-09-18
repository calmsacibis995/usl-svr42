#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libm:libm.mk	1.17.8.7"
#
# makefile for libm
#
# By default, both profiled and non-profiled libraries are built.
# To build only the non-profiled version, set ARCHIVE=nonprofarch
# To build only the profiled version, set ARCHIVE=profarch
#
# If using a non-ANSI compiler, only double precision functions are
# built.
#
include $(LIBRULES)

SGSBASE=../../../cmd/sgs
INS=$(SGSBASE)/sgs.install
INSDIR=
HFILES=
SOURCES=
OBJECTS=
PRODUCTS=
ARCHIVE=archive
DEFLIST=
INSPDIR=
CCS=ALL

#
# By default, both INLINE and FPE versions of the 3b2 libm
# are built.  This behaviour can be altered by setting
# the parameter CCS to INLINE for only the inline version, or to
# anything but ALL or INLINE for the FPE version
#

all:
	cd $(CPU); $(MAKE)  $(ARCHIVE)

nonansi:
	cd $(CPU); $(MAKE) nonansi_arch ANSIDEF=

move:
	cd $(CPU); $(MAKE) move INSDIR="$(CCSLIB)" INSPDIR="$(CCSLIB)/libp"

nonansi_move:
	cd $(CPU); $(MAKE) move INSDIR="$(CCSLIB)" INSPDIR="$(CCSLIB)/libp"

install: 
	set +e; $(CC) -E tstansi.c; if [ $$? = 0 ] ; \
	then $(MAKE) -f libm.mk all move ;\
	else $(MAKE) -f libm.mk nonansi nonansi_move ;\
	fi ; 

clean:
	cd $(CPU); $(MAKE) clean
clobber:
	cd $(CPU); $(MAKE) clobber

lintit:
	cd port ; $(MAKE) lintit
