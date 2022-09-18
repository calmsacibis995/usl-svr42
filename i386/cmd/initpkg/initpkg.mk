#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)initpkg:i386/cmd/initpkg/initpkg.mk	1.19.22.8"
#ident "$Header: $"

include $(CMDRULES)

INSDIR = $(SBIN)
OWN = root
GRP = sys

LDLIBS = -lcmd

TOUCH=$(CH)/usr/bin/touch

SCRIPTS= bcheckrc brc inittab vfstab rstab mountall\
	 rc0 rc1 rc2 rc3 dinit\
	 rmount rmountall rumountall shutdown umountall mini_inittab \
	dumpsave configure netinfo ckroot
DIRECTORIES= init.d rc0.d rc1.d rc2.d rc3.d dinit.d
OTHERS= dumpcheck rc6
DIRS = \
	$(ETC) \
	$(ETC)/confnet.d \
	$(ETC)/init.d \
	$(ETC)/dfs \
	$(ETC)/rc0.d \
	$(ETC)/rc1.d \
	$(ETC)/rc2.d \
	$(ETC)/rc3.d \
	$(ETC)/dinit.d \
	$(ETC)/security

.MUTEX: install $(DIRECTORIES)

all:	scripts directories other

scripts: $(SCRIPTS)

directories: $(DIRECTORIES)

other: $(OTHERS)

clean:

clobber: clean
	rm -f $(SCRIPTS)

install: $(DIRS) all

$(DIRS):
	[ -d $@ ] || mkdir $@

brc bcheckrc::
	-rm -f $(ETC)/$@
	-rm -f $(USRSBIN)/$@
	cp $@.sh $@
	$(INS) -f $(INSDIR) -m 0744 -u $(OWN) -g $(GRP) $@
	$(INS) -f $(USRSBIN) -m 0744 -u $(OWN) -g $(GRP) $@
	$(TOUCH) 0101000070 $(INSDIR)/$@
	-$(SYMLINK) /sbin/$@ $(ETC)/$@
	
ckroot::
	cp $@.sh $@
	$(INS) -f $(INSDIR) -m 0744 -u $(OWN) -g $(GRP) $@
	$(TOUCH) 0101000070 $(INSDIR)/$@

configure::
	-rm -f $(ETC)/confnet.d/configure
	$(INS) -f $(ETC)/confnet.d -m 0755 -u $(OWN) -g $(GRP) configure

dumpsave::
	sh $@.sh $(ROOT)
	$(INS) -f $(INSDIR) -m 0744 -u root -g sys $@
	$(TOUCH) 0101000070 $(INSDIR)/$@

dumpcheck:	dumpcheck.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)
	$(INS) -f $(INSDIR) -m 0555 -u bin -g bin dumpcheck
	$(INS) -f $(ETC)/default -m 0644 -u bin -g bin dump.dfl
	-mv $(ETC)/default/dump.dfl $(ETC)/default/dump

vfstab::
	sh $@.sh  $(ROOT)
	$(INS) -f $(ETC) -m 0744 -u $(OWN) -g $(GRP) $@
	$(TOUCH) 0101000070 $(ETC)/$@

rstab::
	cp rstab.sh  dfstab
	$(INS) -f $(ETC)/dfs -m 0644 -u $(OWN) -g $(GRP) dfstab
	$(TOUCH) 0101000070 $(ETC)/dfs/dfstab

inittab::
	sh inittab.sh $(ROOT)
	$(INS) -f $(ETC)  -m 0664 -u $(OWN) -g $(GRP) inittab
	$(TOUCH) 0101000070 $(ETC)/inittab

mini_inittab::
	sh mini_itab.sh $(ROOT)
	$(INS) -f $(ETC)  -m 0664 -u $(OWN) -g $(GRP) mini_inittab
	$(TOUCH) 0101000070 $(ETC)/mini_inittab

mountall::
	-rm -f $(ETC)/mountall
	-rm -f $(USRSBIN)/mountall
	cp mountall.sh  ./mountall
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) mountall
	$(TOUCH) 0101000070 $(INSDIR)/mountall
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) mountall
	-$(SYMLINK) /sbin/mountall $(ETC)/mountall

netinfo::
	-rm -f $(USRSBIN)/netinfo
	$(INS) -f $(USRSBIN) -m 0755 -u $(OWN) -g $(GRP) netinfo

rc0 rc1 rc2 rc3 dinit::
	-rm -f $(ETC)/$@
	-rm -f $(USRSBIN)/$@
	cp $@.sh $@
	$(INS) -f $(INSDIR) -m 0744 -u $(OWN) -g $(GRP) $@
	$(INS) -f $(USRSBIN) -m 0744 -u $(OWN) -g $(GRP) $@
	$(TOUCH) 0101000070 $(INSDIR)/$@
	-$(SYMLINK) /sbin/$@ $(ETC)/$@

rc6:	rc0
	-$(SYMLINK) /sbin/rc0 /sbin/rc6
	-$(SYMLINK) /sbin/rc0 $(ETC)/rc6

rmount::
	-rm -f $(ETC)/rmount
	cp rmount.sh $@
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) rmount
	$(TOUCH) 0101000070 $(USRSBIN)/rmount
	-$(SYMLINK) /usr/sbin/rmount $(ETC)/rmount

rmountall rumountall::
	-rm -f $(ETC)/$@
	cp $@.sh $@
	$(INS) -f $(USRSBIN) -m 544 -u $(OWN) -g $(GRP) $@
	$(TOUCH) 0101000070 $(USRSBIN)/$@
	-$(SYMLINK) /usr/sbin/$@ $(ETC)/$@

shutdown::
	-rm -f $(ETC)/shutdown
	-rm -f $(USRSBIN)/shutdown
	cp shutdown.sh shutdown
	$(INS) -f $(INSDIR) -m 0755 -u $(OWN) -g $(GRP) shutdown
	$(INS) -f $(USRSBIN) -m 0755 -u $(OWN) -g $(GRP) shutdown
	$(TOUCH) 0101000070 $(INSDIR)/shutdown
	-$(SYMLINK) /sbin/shutdown $(ETC)/shutdown

umountall::
	-rm -f $(ETC)/umountall
	-rm -f $(USRSBIN)/umountall
	cp umountall.sh  umountall
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) umountall
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) umountall
	$(TOUCH) 0101000070 $(INSDIR)/umountall
	-$(SYMLINK) /sbin/umountall $(ETC)/umountall

init.d rc0.d rc1.d rc2.d rc3.d dinit.d::
	cd ./$@; \
	ROOT=$(ROOT) MACH=$(MACH) CH=$(CH) INS=$(INS) sh :mk.$@.sh 
