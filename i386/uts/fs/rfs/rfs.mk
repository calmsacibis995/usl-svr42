#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:fs/rfs/rfs.mk	1.7"
#ident	"$Header: $"

include $(UTSRULES)

KBASE    = ../..
FS	 = $(CONF)/pack.d/rfs/Driver.o

FILES = \
	rf_auth.o \
	rf_cirmgr.o \
	rf_comm.o \
	rf_rsrc.o \
	rf_admin.o \
	rf_canon.o \
	rf_sys.o \
	rfsr_subr.o \
	rfsr_ops.o \
	rf_serve.o \
	du.o \
	rf_vfsops.o \
	rf_getsz.o \
	rf_cache.o \
	rfcl_subr.o \
	rf_vnops.o \
	rf_name.o

CFILES = $(FILES:.o=.c)

MAKEFILE = rfs.mk
LINTFLAGS= -unx

.SUFFIXES: .ln

.c.ln :
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(CFLAGS) -c $*.c

all:	ID $(FS)

$(FS):	$(FILES)
	$(LD) -r -o $@ $(FILES)

lint:
	$(MAKE) -ef $(MAKEFILE) -$(MAKEFLAGS) \
		 LINTFLAGS="$(LINTFLAGS)" O=ln lintit


lintit: $(FILES)
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(FILES)

#
# Configuration Section
#
ID:
	cd rfs.cf; $(IDINSTALL) -R$(CONF) -M rfs

clean:	cleanX cleanlint

cleanX:
	-rm -f $(FILES)

cleanlint :
	$(MAKE) -ef $(MAKEFILE) -$(MAKEFLAGS) O=ln cleanX

clobber:	clean
	-$(IDINSTALL) -e -R$(CONF) -d rfs


headinstall: \
	$(KBASE)/fs/rfs/hetero.h \
	$(KBASE)/fs/rfs/idtab.h \
	$(KBASE)/fs/rfs/nserve.h \
	$(KBASE)/fs/rfs/rf_acct.h \
	$(KBASE)/fs/rfs/rf_adv.h \
	$(KBASE)/fs/rfs/rf_cirmgr.h \
	$(KBASE)/fs/rfs/rf_comm.h \
	$(KBASE)/fs/rfs/rf_debug.h \
	$(KBASE)/fs/rfs/rf_messg.h \
	$(KBASE)/fs/rfs/rf_sys.h \
	$(KBASE)/fs/rfs/rf_vfs.h \
	$(FRC)
	[ -d $(INC)/sys ] || mkdir $(INC)/sys
	[ -d $(INC)/sys/fs ] || mkdir $(INC)/sys/fs
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/rfs/hetero.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/rfs/idtab.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/rfs/nserve.h
	$(INS) -f $(INC)/sys/fs -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/rfs/rf_acct.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/rfs/rf_adv.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/rfs/rf_cirmgr.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/rfs/rf_comm.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/rfs/rf_debug.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/rfs/rf_messg.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/rfs/rf_sys.h
	$(INS) -f $(INC)/sys/fs -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/rfs/rf_vfs.h
	$(INS) -f $(INC)/sys/fs -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/rfs/rf_vfs.h

FRC: 
 
include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

