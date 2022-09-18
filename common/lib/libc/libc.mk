#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libc:libc.mk	1.27.7.21"
#
# makefile for libc
#
#
# The variable PROF is null by default, causing both the standard C library
# and a profiled library to be maintained.  If profiled object is not 
# desired, the reassignment PROF=@# should appear in the make command line.
#
# The variable IGN may be set to -i by the assignment IGN=-i in order to
# allow a make to complete even if there are compile errors in individual
# modules.
#
# See also the comments in the lower-level machine-dependent makefiles.
#

include $(LIBRULES)

PCFLAGS=
ABILIB=$(CCSLIB)/minabi
ABILIBP=$(CCSLIB)/minabi/libp
LIBP=$(CCSLIB)/libp
DONE=
PROF=
NONPROF=
PIC=
ABI=
LOCALDEF = -D$(CPU)
SGSBASE=../../cmd/sgs
CATROOT=./catalogs
ISANSI=TRUE
RTLD_DIR=./rtld

all:	all_objects
	$(MAKE) -f libc.mk ISANSI=$$ISANSI \
				archive_lib shared_lib compat_lib abi_lib

archive:	archive_objects
	$(MAKE) -f libc.mk ISANSI=$$ISANSI archive_lib

shared:		shared_objects
	$(MAKE) -f libc.mk ISANSI=$$ISANSI shared_lib

abi:	abi_objects
	$(MAKE) -f libc.mk ISANSI=$$ISANSI abi_lib

compat: compat_objects
	$(MAKE) -f libc.mk ISANSI=$$ISANSI compat_lib
	
all_objects:	rtld_obj
	- $(CC) -E tstansi.c; if [ $$? = 0 ] ; \
	then ISANSI="TRUE"; \
	else ISANSI="FALSE"; PIC="@#"; ABI="@#"; \
	fi ; \
	$(MAKE) -f libc.mk ISANSI=$$ISANSI specific

shared_objects:	rtld_obj
	- $(CC) -E tstansi.c; if [ $$? = 0 ] ; \
	then ISANSI="TRUE"; ABI="@#"; \
	else ISANSI="FALSE"; PIC="@#"; ABI="@#"; \
	fi ; \
	$(MAKE) -f libc.mk ISANSI=$$ISANSI PIC=$$PIC \
				ABI=$$ABI specific

abi_objects:	rtld_obj
	- $(CC) -E tstansi.c; if [ $$? = 0 ] ; \
	then ISANSI="TRUE"; PIC="@#"; \
	else ISANSI="FALSE"; PIC="@#"; ABI="@#"; \
	fi ; \
	$(MAKE) -f libc.mk ISANSI=$$ISANSI PIC=$$PIC \
				ABI=$$ABI specific

compat_objects:	
	- $(CC) -E tstansi.c; if [ $$? = 0 ] ; \
	then ISANSI="TRUE"; ABI="@#"; \
	else ISANSI="FALSE"; PIC="@#"; ABI="@#"; \
	fi ; \
	$(MAKE) -f libc.mk ISANSI=$$ISANSI PIC=$$PIC \
				ABI=$$ABI specific
archive_objects:
	- $(CC) -E tstansi.c; if [ $$? = 0 ] ; \
	then ISANSI="TRUE"; PIC="@#"; ABI="@#"; \
	else ISANSI="FALSE"; PIC="@#"; ABI="@#"; \
	fi ; \
	$(MAKE) -f libc.mk ISANSI=$$ISANSI PIC=$$PIC \
				ABI=$$ABI specific

specific:
	#
	# compile portable library modules
	cd port; $(MAKE) -f makefile 
	#
	# compile machine-dependent library modules
	cd $(CPU); $(MAKE) -f makefile ISANSI=$(ISANSI)

rtld_obj:
	# make the rtld objects
	cd $(RTLD_DIR); $(MAKE) -f rtld.mk

archive_lib:
	#
	# place portable modules in "object" directory, then overlay
	# 	the machine-dependent modules.
	-rm -rf object
	mkdir object
	find port $(CPU) -name '*.o' -print | \
	xargs sh -sc 'cp "$$@" object'
	$(PROF)find port $(CPU) -name '*.p' -print | xargs sh -sc 'cp "$$@" object'
	#
	# delete temporary libraries
	-rm -f lib.libc
	$(PROF)-rm -f libp.libc
	#
	# set aside run-time modules, which don't go in library archive!
	cd object; for i in *crt?.o values-Xt.o values-Xa.o values-Xc.o; do mv $$i ..; done
	#
	# build archive out of the remaining modules.
	cd object; $(MAKE) -f ../$(CPU)/makefile archive \
		PROF=$(PROF) MAC=$(MAC) ISANSI=$(ISANSI)
	-rm -rf object
	#
	$(DONE)

shared_lib:
	#
	# place portable modules in "object" directory, then overlay
	# 	the machine-dependent modules.
	-rm -rf object
	mkdir object
	find port $(CPU) -name '*.o' -print | \
	xargs sh -sc 'cp "$$@" object'
	find port $(CPU) -name '*.P' -print | \
	xargs sh -sc 'cp "$$@" object'
	cp $(RTLD_DIR)/$(CPU)/*.o object
	#
	# delete temporary libraries
	-rm -f libc.so
	#
	# set aside run-time modules, which don't go in library archive!
	cd object; for i in *crt?.o values-Xa.o values-Xc.o; do mv $$i ..; done; \
	cp values-Xt.o ..
	#
	# build archive out of the remaining modules.
	cd object; $(MAKE) -f ../$(CPU)/makefile shared \
		PROF=$(PROF) MAC=$(MAC) ISANSI=$(ISANSI)
#	-rm -rf object
	#
	$(DONE)

abi_lib:
	#
	# place portable modules in "object" directory, then overlay
	# 	the machine-dependent modules.
	-rm -rf object
	mkdir object
	find port $(CPU) -name '*.o' -print | \
	xargs sh -sc 'cp "$$@" object'
	find port $(CPU) -name '*.A' -print | \
	xargs sh -sc 'cp "$$@" object'
	cp $(RTLD_DIR)/$(CPU)/*.o object
	#
	# delete temporary libraries
	-rm -f libabi.so
	#
	# set aside run-time modules, which don't go in library archive!
	cd object; for i in *crt?.o values-Xt.o values-Xa.o values-Xc.o; do mv $$i ..; done
	#
	# build archive out of the remaining modules.
	cd object; $(MAKE) -f ../$(CPU)/makefile abi_lib \
		PROF=$(PROF) MAC=$(MAC) ISANSI=$(ISANSI)
	-rm -rf object
	#
	$(DONE)

compat_lib:
	#
	# place portable modules in "object" directory, then overlay
	# 	the machine-dependent modules.
	-rm -rf object
	mkdir object
	find port $(CPU) -name '*.o' -print | \
	xargs sh -sc 'cp "$$@" object'
	find port $(CPU) -name '*.P' -print | \
	xargs sh -sc 'cp "$$@" object'
	cp $(RTLD_DIR)/$(CPU)/align.o object
	#
	# delete temporary libraries
	-rm -f libc.so.1.1
	#
	# set aside run-time modules, which don't go in library archive!
	cd object; for i in *crt?.o values-Xa.o values-Xc.o; do mv $$i ..; done; \
	cp values-Xt.o ..
	# build archive out of the remaining modules.
	cd object; $(MAKE) -f ../$(CPU)/makefile compat_lib \
		PROF=$(PROF) MAC=$(MAC) ISANSI=$(ISANSI)
#	-rm -rf object
	#
	$(DONE)

move:	move_archive
	#
	# move the shared and abi libraries into the correct directories
	sh $(SGSBASE)/sgs.install 644 $(OWN) $(GRP) $(CCSLIB)/lib$(VARIANT)c.so libc.so ; \
	rm -f libc.so
	sh $(SGSBASE)/sgs.install 755 $(OWN) $(GRP) $(USRLIB)/lib$(VARIANT)c.so.1 libc.so.1 ; \
	rm -f libc.so.1
	sh $(SGSBASE)/sgs.install 755 $(OWN) $(GRP) $(USRLIB)/lib$(VARIANT)c.so.1.1 libc.so.1.1 ; \
	rm -f libc.so.1.1
	rm -f $(LIBP)/lib$(VARIANT)c.so
	ln $(CCSLIB)/lib$(VARIANT)c.so $(LIBP)/lib$(VARIANT)c.so

	if [ ! -d $(ABILIB) ]; then \
	mkdir $(ABILIB); \
	fi
	sh $(SGSBASE)/sgs.install 644 $(OWN) $(GRP) $(ABILIB)/lib$(VARIANT)c.so libabi.so ; \
	rm -f libabi.so
	sh $(SGSBASE)/sgs.install 755 $(OWN) $(GRP) $(USRLIB)/ld.so.1 ld.so.1 ; \
	rm -f ld.so.1
	if [ ! -d $(ABILIBP) ]; then \
	mkdir $(ABILIBP); \
	fi
	rm -f $(ABILIBP)/lib$(VARIANT)c.so
	ln $(ABILIB)/lib$(VARIANT)c.so $(ABILIBP)/lib$(VARIANT)c.so

move_archive:
	#
	# move the library or libraries into the correct directory
	for i in *crt?.o values-Xt.o values-Xa.o values-Xc.o;  \
	do sh $(SGSBASE)/sgs.install 644 $(OWN) $(GRP) $(CCSLIB)/$(SGS)$$i $$i; \
	rm -f $$i ; done
	sh $(SGSBASE)/sgs.install 644 $(OWN) $(GRP) $(CCSLIB)/lib$(VARIANT)c.a lib.libc ; \
	rm -f lib.libc
	$(PROF) if [ ! -d $(LIBP) ]; then \
	$(PROF) mkdir $(LIBP); \
	$(PROF) fi
	$(PROF)sh $(SGSBASE)/sgs.install 644 $(OWN) $(GRP) $(LIBP)/lib$(VARIANT)c.a libp.libc ; \
	rm -f libp.libc

.MUTEX:	all move msg_catalogs
install:	all move msg_catalogs

.MUTEX:	archive move_archive
install_archive:	archive move_archive

msg_catalogs:
	if [ ! -d $(USRLIB)/locale ]; then \
		mkdir $(USRLIB)/locale; \
	fi
	if [ ! -d $(USRLIB)/locale/C ]; then \
		mkdir $(USRLIB)/locale/C; \
	fi
	if [ ! -d $(USRLIB)/locale/C/MSGFILES ]; then \
		mkdir -p $(USRLIB)/locale/C/MSGFILES; \
	fi
	sh $(SGSBASE)/sgs.install 444 $(OWN) $(GRP) $(USRLIB)/locale/C/MSGFILES/syserr.str port/gen/syserr.str;
	sh $(SGSBASE)/sgs.install 444 $(OWN) $(GRP) $(USRLIB)/locale/C/MSGFILES/ar.str $(CATROOT)/ar.str
	sh $(SGSBASE)/sgs.install 444 $(OWN) $(GRP) $(USRLIB)/locale/C/MSGFILES/cplu.str $(CATROOT)/cplu.str
	sh $(SGSBASE)/sgs.install 444 $(OWN) $(GRP) $(USRLIB)/locale/C/MSGFILES/libc.str $(CATROOT)/libc.str

clean:
	#
	# remove intermediate files except object modules and temp library
	-rm -rf obj*
	cd port ;  $(MAKE) clean
	cd $(CPU) ;  $(MAKE) clean

clobber:
	#
	# remove intermediate files
	-rm -rf *.o lib*.libc obj*
	-rm -rf *.o libc.so libabi.so libc.so.1 ld.so.1
	cd port ;  $(MAKE) clobber
	if [ -d $(RTLD_DIR) ] ; then \
		cd $(RTLD_DIR); $(MAKE) -f rtld.mk clobber; fi
	cd $(CPU) ;  $(MAKE) clobber
