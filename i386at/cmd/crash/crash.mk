#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)crash:i386at/cmd/crash/crash.mk	1.1.1.9"
#ident "$Header: crash.mk 1.1 91/07/23 $"

#
# Tool Section
#
include $(CMDRULES)

#
# Define Section
#
INSPERM  = -m 0555 -u $(OWN) -g $(GRP)
LOCALDEF = -D$(MACH) -D_KMEMUSER -DDBO
LDLIBS   = -lelf -lia -lcmd -lc
FRC 	 =

OFILES= abuf.o \
	base.o \
	buf.o \
	callout.o \
	class.o \
	dis.o \
	disp.o \
	fpriv.o \
	init.o \
	inode.o \
	kma.o \
	lck.o \
	lidcache.o \
	main.o \
	map.o \
	misc.o \
	stat.o \
	page.o \
	prnode.o \
	proc.o \
	pty.o \
	resource.o \
	rfs.o \
	rt.o \
	search.o \
	sfs_inode.o \
	size.o \
	snode.o \
	stream.o \
	symtab.o \
	ts.o \
	tty.o \
	u.o \
	util.o \
	var.o \
	vfs.o \
	vfssw.o \
	vtop.o \
	vxfs_inode.o \
	i386.o

KMAOBJ = \
	kmacct.o \
	kmasym.o


all: 		crash ldsysdump memsize

crash:		$(OFILES)
		$(CC) -o $@ $(OFILES) $(LDFLAGS) $(LDLIBS) $(NOSHLIBS)

ldsysdump: 	ldsysdump.sh
		cp ldsysdump.sh ldsysdump

memsize: 	memsize.o
		$(CC) $(LDFLAGS) -o $@ memsize.o $(ROOTLIBS)

memsize.dy: 	memsize.o
		$(CC) $(LDFLAGS) -o $@ memsize.o 

kmacct:		$(KMAOBJ)
		$(CC) $(DEFLIST) $(LDFLAGS) -o $@ $(KMAOBJ) $(LDLIBS) $(SHLIBS)


install: 	ins_crash ins_ldsysdump ins_memsize

ins_crash: 	crash
		-rm -f $(ETC)/crash
		$(INS) -f $(USRSBIN) -m 0555 -u bin -g bin crash
		-$(SYMLINK) /usr/sbin/crash $(ETC)/etc/crash

ins_ldsysdump: 	ldsysdump
		-rm -f $(ETC)/ldsysdump
		$(INS) -f $(USRSBIN) -m 0555 -u bin -g bin ldsysdump
		-$(SYMLINK) /usr/sbin/ldsysdump $(ETC)/ldsysdump

ins_memsize:	memsize memsize.dy
		$(INS) -f $(SBIN) -m 0555 -u bin -g bin memsize
		$(INS) -f $(SBIN) -m 0555 -u bin -g bin memsize.dy

clean:
		-rm -f *.o

clobber: 	clean
		-rm -f crash
		-rm -f ldsysdump
		-rm -f memsize
		-rm -f memsize.dy

lint: 		$(CFILES) $(HFILES) 
		lint $(CPPFLAGS) -uh $(CFILES) 

#FRC:

async.o:	async.c

