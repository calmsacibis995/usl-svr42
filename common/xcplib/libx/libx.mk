#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)xcplibx:common/xcplib/libx/libx.mk	1.2.4.2"
#ident  "$Header: libx.mk 1.2 91/07/04 $"

include $(LIBRULES)

#	Copyright (c) 1987, 1988 Microsoft Corporation
#	  All Rights Reserved
#	This Module contains Proprietary Information of Microsoft
#	Corporation and should be treated as Confidential.	  
#
# makefile for libx
#
#
# The variable PROF is null by default, causing both the standard XENIX library
# and a profiled library to be maintained.  If profiled object is not 
# desired, the reassignment PROF=@# should appear in the make command line.
#
# The variable IGN may be set to -i by the assignment IGN=-i in order to
# allow a make to complete even if there are compile errors in individual
# modules.
#
# See also the comments in the lower-level machine-dependent makefiles.
#

OWN = 
GRP = 

VARIANT=
LOCALDEF = -DMERGE
LIBP=$(USRLIB)/libp
DONE=
PROF=
NONPROF=

all:
	#
	# compile portable library modules
	cd port; $(MAKE) -f makefile $(MAKEARGS)
	#
	# compile machine-dependent library modules
	cd sys; $(MAKE) -f makefile $(MAKEARGS)
	#
	# place portable modules in "object" directory, then overlay
	# the machine-dependent modules.
	-rm -rf object
	mkdir object
	-cp port/*.o object
#	cp port/[a-l]*.o object
#	cp port/[!a-l]*.o object
	-$(PROF)cp port/*.p object
#	$(PROF)cp port/[a-l]*.p object
#	$(PROF)cp port/[!a-l]*.p object
	cp sys/*.o object
#	cp sys/[a-l]*.o object
#	cp sys/[!a-l]*.o object
	$(PROF)cp sys/*.p object
#	$(PROF)cp sys/[a-l]*.p object
#	$(PROF)cp sys/[!a-l]*.p object
	#
	# delete temporary libraries
	-rm -f lib.libx
	$(PROF)-rm -f libp.libx
	#
	# build archive out of the remaining modules.
	#
	# Note that "archive" is invoked with libx/object as current directory.
	#
	# figure out the correct ordering for all the archive modules 
	$(LORDER) object/*.o | $(PFX)tsort >objlist
	#
	# build the archive with the modules in correct order.
	$(AR) $(ARFLAGS) lib.libx `cat objlist` 
	$(PROF)#
	$(PROF)# build the profiled library archive, first renaming the
	$(PROF)# .p (profiled object) modules to .o
	$(PROF)for i in object/*.p ; do mv $$i object/`basename $$i .p`.o ; done
	$(PROF)$(AR) $(ARFLAGS) libp.libx `cat objlist` ; \
	rm -rf object
	#
	$(DONE)

install: all
	#
	# move the library or libraries into the correct directory
	cp lib.libx $(USRLIB)/lib$(VARIANT)x.a ; rm -f lib.libx
	$(PROF)if [ ! -d $(LIBP) ]; then \
	$(PROF) mkdir $(LIBP); \
	$(PROF)fi
	$(PROF)cp libp.libx $(LIBP)/lib$(VARIANT)x.a ; rm -f libp.libx

clean:
	#
	# remove intermediate files except object modules and temp library
	cd port ; $(MAKE) -f makefile clean
	cd sys  ; $(MAKE) -f makefile clean

clobber: clean
	#
	# remove intermediate files
	-rm -rf lib*.libx lib*.contents obj*
	cd port ; $(MAKE) -f makefile clobber
	cd sys  ; $(MAKE) -f makefile clobber

lintit:
	cd port ; $(MAKE) -f makefile lintit
	cd sys  ; $(MAKE) -f makefile lintit
