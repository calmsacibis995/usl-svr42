#ident	"@(#)ucb:i386/ucblib/libc/libc.mk	1.4"
#ident	"$Header: $"
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.



#
# makefile for libc
#
#

include $(LIBRULES)

INSDIR=$(ROOT)/$(MACH)/usr/ucblib
DONE=
PROF=
NONPROF=
OWN=bin
GRP=bin

all: specific

specific:
	#
	# compile portable library modules
	cd port; $(MAKE) $(MAKEARGS)
	cd i386; $(MAKE) $(MAKEARGS)
	#
	# place portable modules in "object" directory, then overlay
	# 	the machine-dependent modules.
	-rm -rf object
	mkdir object
	cp port/*/*.o object
	cp i386/*/*.o object
	#
	# delete temporary libraries
	-rm -f lib.libucb
	#
	# build archive out of the remaining modules.
	cd object; $(MAKE)  -f ../i386/makefile archive \
		AR=$(AR)  LORDER=$(LORDER) PROF=$(PROF) MAC=$(MAC)
	-rm -rf object
	#
	$(DONE)

install: all
	#
	# move the library or libraries into the correct directory
	mv lib.libucb lib$(VARIANT)ucb.a ; \
	$(INS) -m 644 -u $(OWN) -g $(GRP) -f $(INSDIR) lib$(VARIANT)ucb.a ; \
	rm -f lib$(VARIANT)ucb.a

clean:
	#
	# remove intermediate files except object modules and temp library
	-rm -rf lib*.contents obj*
	cd port ;  $(MAKE) clean
	cd i386; $(MAKE) clean

clobber:
	#
	# remove intermediate files
	-rm -rf *.o lib*.libucb lib*.contents obj*
	cd port; $(MAKE) clobber
	cd i386; $(MAKE) clobber
