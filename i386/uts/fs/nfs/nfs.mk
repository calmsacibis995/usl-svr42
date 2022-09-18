#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:fs/nfs/nfs.mk	1.5"
#ident "$Header: nfs.mk 1.1 91/03/21 $"

include $(UTSRULES)

KBASE    = ../..
LOCALDEF = -UDEBUG -DSYSV
FS	 = $(CONF)/pack.d/nfs/Driver.o

FILES = \
	nfs_aux.o\
	nfs_client.o\
	nfs_cnvt.o\
	nfs_common.o\
	nfs_export.o\
	nfs_server.o\
	nfs_subr.o\
	nfs_vfsops.o\
	nfs_vnops.o\
	nfs_xdr.o\
	nfssys.o

CFILES = $(FILES:.o=.c)



all:	ID $(FS)

$(FS):	$(FILES)
	$(LD) -r -o $@ $(FILES)

#
# Configuration Section
#
ID:
	cd nfs.cf; $(IDINSTALL) -R$(CONF) -M nfs


clean:
	-rm -f $(FILES)

clobber:	clean
	-$(IDINSTALL) -e -R$(CONF) -d nfs

headinstall: \
	$(KBASE)/fs/nfs/export.h \
	$(KBASE)/fs/nfs/mount.h \
	$(KBASE)/fs/nfs/nfs.h \
	$(KBASE)/fs/nfs/nfs_clnt.h \
	$(KBASE)/fs/nfs/nfssys.h \
	$(KBASE)/fs/nfs/rnode.h \
	$(FRC)
	[ -d $(INC)/nfs ] || mkdir $(INC)/nfs
	$(INS) -f $(INC)/nfs -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/nfs/export.h
	$(INS) -f $(INC)/nfs -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/nfs/mount.h
	$(INS) -f $(INC)/nfs -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/nfs/nfs.h
	$(INS) -f $(INC)/nfs -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/nfs/nfs_clnt.h
	$(INS) -f $(INC)/nfs -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/nfs/nfssys.h
	$(INS) -f $(INC)/nfs -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/nfs/rnode.h


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

