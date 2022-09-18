#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libw:libw.mk	1.2.8.3"
#ident "$Header: libw.mk 1.5 91/06/27 $"
#
# makefile for libw
#
#
# The variable PROF is null by default, causing both the standard libw
# and a profiled library to be maintained.  If profiled object is not 
# desired, the reassignment PROF=@# should appear in the make command line.
#

include $(LIBRULES)

VARIANT=
DONE=
PROF=
NONPROF=
OWN=bin
GRP=bin
USRLIBP=$(USRLIB)/libp

.MUTEX: libw libw16
.MUTEX:	libw move install16

all:
	$(MAKE) -f libw.mk $(MAKEARGS) libw

libw:
	$(MAKE) -f libw.mk $(MAKEARGS) clean
	$(MAKE) -f libw.mk $(MAKEARGS) nonshared PROF=$(PROF)

nonshared:
	#
	# compile portable library modules
	cd port; $(MAKE) -f makefile $(MAKEARGS)
	#
	# place portable modules in "object" directory
	-rm -rf object
	mkdir object
	cp port/*/*.o object
	$(PROF)cp port/*/*.p object
	#
	# delete temporary libraries
	-rm -f lib.libw
	$(PROF)-rm -f libp.libw
	#
	# build archive out of the remaining modules.
	cd object; $(MAKE) -f ../libw.mk $(MAKEARGS) archive
	-rm -rf object
	#
	$(DONE)

archive:
	#
	# Note that "archive" is invoked with libw/object as current directory.
	#
	# figure out the correct ordering for all the archive modules
	$(LORDER) *.o | $(TSORT) >objlist
	#
	# build the archive with the modules in correct order.
	$(AR) q ../lib.libw `cat objlist`
	$(PROF)#
	$(PROF)# build the profiled library archive, first renaming the
	$(PROF)#	.p (profiled object) modules to .o
	$(PROF)for i in *.p ; do mv $$i `basename $$i .p`.o ; done
	$(PROF)if [ "$(PROF)" != "@#" ]; \
	$(PROF)then \
	$(PROF)$(AR) q ../libp.libw `cat objlist`; \
	$(PROF)fi

move:
	#
	# move the library or libraries into the correct directory
	$(PROF)mv lib.libw libw.a
	$(PROF)$(INS) -f $(USRLIB) -m 0644 -u $(OWN) -g $(GRP) libw.a
	$(PROF)mv libw.a lib.libw
	$(PROF)if [ ! -d $(USRLIBP) ]; then \
	$(PROF)	mkdir $(USRLIBP); \
	$(PROF)fi
	$(PROF)mv libp.libw libw.a
	$(PROF)$(INS) -f $(USRLIBP) -m 0644 -u $(OWN) -g $(GRP) libw.a
	$(PROF)mv libw.a libp.libw
	$(PROF)$(INS) -f $(ROOT)/$(MACH)/usr/include -m 444 -u $(OWN) -g $(GRP) ./inc/libw.h
	$(PROF)$(INS) -f $(ROOT)/$(MACH)/usr/include -m 444 -u $(OWN) -g $(GRP) ./inc/getwidth.h
	$(PROF)$(INS) -f $(ROOT)/$(MACH)/usr/include -m 444 -u $(OWN) -g $(GRP) ./inc/wctype.h
	$(PROF)$(INS) -f $(ROOT)/$(MACH)/usr/include -m 444 -u $(OWN) -g $(GRP) ./inc/widec.h

install:
	$(MAKE) -f libw.mk $(MAKEARGS) libw
	$(MAKE) -f libw.mk $(MAKEARGS) move

clean:
	#
	# remove intermediate files except object modules and temp library
	-rm -rf obj*
	cd port ;  $(MAKE) -f makefile $(MAKEARGS) clobber

clobber:
	#
	# remove intermediate files
	-rm -rf *.o lib*.libw* obj* *.a
	cd port ;  $(MAKE) -f makefile $(MAKEARGS) clobber
	#done
	#
