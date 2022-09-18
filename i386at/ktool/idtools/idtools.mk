#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)idtools:i386at/ktool/idtools/idtools.mk	1.28"
#ident	"$Header:"
#
# Makefile to build and install Driver Installation commands.
#

include $(CMDRULES)

HOSTINC = /usr/include
INSDIR = $(CONF)/bin
DFLDIR = $(ROOT)/$(MACH)/etc/default

DIRS = $(CONF) $(INSDIR) $(DFLDIR)

SHELLS = idbuild idreboot idtune idrebuild idcpunix
CMDS = idconfig idmkunix idinstall idspace idval idmknod \
	idmkinit idmkenv idcheck idmodreg idmodload
XCMDS =	$(PFX)idbuild $(PFX)idconfig $(PFX)idmkunix $(PFX)idinstall
XTARGS = xenv_idconfig xenv_idmkunix xenv_idinstall xenv_idbuild
XSHELLS = $(PFX)idbuild

SYS_DIR = ./sys
SYS_MOD = ./sys/mod.h
SYS_ELF = ./sys/elf.h
LIBELF = ./libelf.h

all:	$(CMDS) $(SHELLS) xenv

xenv:	$(XTARGS)

.MUTEX:	install xenv_install

install: all $(DIRS) xenv_install dfl
	(cd $(INSDIR); rm -f $(CMDS) $(SHELLS))
	for cmd in $(CMDS) $(SHELLS); \
	do \
		$(INS) -f $(INSDIR) $$cmd; \
	done

xenv_install: xenv $(DIRS)
	if [ "$(PFX)" ]; then \
		(cd $(INSDIR); rm -f $(XCMDS) $(XSHELLS)); \
		for cmd in $(XCMDS) $(XSHELLS); \
		do \
			$(INS) -f $(INSDIR) $$cmd; \
		done; \
	fi

dfl:	$(DIRS)
	rm -f $(DFLDIR)/idtools
	cp idtools.dfl $(DFLDIR)/idtools

$(DIRS):
	-mkdir -p $@

clean:
	-rm -f *.o $(XTARGS) $(LIBELF)
	-rm -rf sys

clobber: clean
	-rm -f $(CMDS) $(XCMDS)

idconfig: idconfig.c \
	getinst_targ.o \
	getmaj_targ.o \
	getfunc_targ.o \
	entry_targ.o \
	devconf_targ.o \
	mdep_targ.o \
	fdep_targ.o \
	util_targ.o \
	inst.h \
	defines.h \
	devconf.h \
	mdep.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/mod.h \
	$(INC)/errno.h \
	$(INC)/malloc.h \
	$(INC)/ctype.h \
	$(INC)/string.h
	$(CC) -I$(INC) $(CFLAGS) $(LDFLAGS) $(ROOTLIBS) \
		-o idconfig idconfig.c \
		getinst_targ.o getmaj_targ.o getfunc_targ.o entry_targ.o \
		devconf_targ.o mdep_targ.o fdep_targ.o util_targ.o -lelf

xenv_idconfig: idconfig.c \
	getinst_host.o \
	getmaj_host.o \
	getfunc_host.o \
	entry_host.o \
	devconf_host.o \
	mdep_host.o \
	fdep_host.o \
	util_host.o \
	inst.h \
	defines.h \
	devconf.h \
	mdep.h \
	$(HOSTINC)/sys/types.h \
	$(HOSTINC)/sys/stat.h \
	$(INC)/sys/mod.h \
	$(SYS_MOD) \
	$(HOSTINC)/ctype.h \
	$(HOSTINC)/string.h
	if [ "$(PFX)" ]; then \
		rm -f idconf_host.c; cp idconfig.c idconf_host.c; \
		$(HCC) -I. -I$(HOSTINC) -I$(INC) \
			-o $(PFX)idconfig idconf_host.c \
			getinst_host.o getmaj_host.o getfunc_host.o \
			entry_host.o devconf_host.o mdep_host.o fdep_host.o \
			util_host.o \
			$(TOOLS)/usr/ccs/lib/libelf$(PFX).a; \
		rm -f idconf_host.c; \
		touch xenv_idconfig; \
	fi

idmkunix: idmkunix.c \
	inst.h \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/ctype.h \
	$(INC)/fcntl.h \
	$(INC)/unistd.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h
	$(CC) -I$(INC) $(CFLAGS) $(LDFLAGS) $(ROOTLIBS) \
		-o idmkunix idmkunix.c

xenv_idmkunix: idmkunix.c \
	inst.h \
	$(HOSTINC)/stdio.h \
	$(HOSTINC)/string.h \
	$(HOSTINC)/ctype.h \
	$(HOSTINC)/fcntl.h \
	$(HOSTINC)/unistd.h \
	$(HOSTINC)/sys/types.h \
	$(HOSTINC)/sys/stat.h
	if [ "$(PFX)" ]; then \
		rm -f idmkunx_host.c; cp idmkunix.c idmkunx_host.c; \
		$(HCC) -I$(HOSTINC) -I$(INC) \
			-o $(PFX)idmkunix idmkunx_host.c; \
		rm -f idmkunx_host.c; \
		touch xenv_idmkunix; \
	fi

idinstall: idinstall.c \
	getinst_targ.o \
	getmaj_targ.o \
	entry_targ.o \
	defines.h \
	inst.h \
	devconf.h \
	mdep.h \
	$(INC)/ctype.h \
	$(INC)/string.h \
	$(INC)/errno.h \
	$(INC)/sys/errno.h \
	$(INC)/ftw.h \
	$(INC)/sys/stat.h
	$(CC) -I$(INC) $(CFLAGS) $(LDFLAGS) $(ROOTLIBS) \
		-o idinstall idinstall.c \
		getinst_targ.o getmaj_targ.o entry_targ.o

xenv_idinstall: idinstall.c \
	getinst_host.o \
	getmaj_host.o \
	entry_host.o \
	defines.h \
	inst.h \
	devconf.h \
	mdep.h \
	$(HOSTINC)/ctype.h \
	$(HOSTINC)/string.h \
	$(HOSTINC)/errno.h \
	$(HOSTINC)/sys/errno.h \
	$(HOSTINC)/sys/stat.h
	if [ "$(PFX)" ]; then \
		rm -f idinst_host.c; cp idinstall.c idinst_host.c; \
		$(HCC) -I$(HOSTINC) -I$(INC) -DCROSS \
			-o $(PFX)idinstall idinst_host.c \
			getinst_host.o getmaj_host.o entry_host.o ; \
		rm -f idinst_host.c; \
		touch xenv_idinstall; \
	fi

xenv_idbuild:	idbuild
	if [ "$(PFX)" ]; then \
		rm -rf $(PFX)idbuild; \
		cp idbuild $(PFX)idbuild; \
	fi

idcheck: idcheck.c \
	getinst_targ.o \
	getmaj_targ.o \
	entry_targ.o \
	util_targ.o \
	inst.h \
	defines.h \
	mdep.h \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h
	$(CC) -I$(INC) $(CFLAGS) $(LDFLAGS) $(ROOTLIBS) -o idcheck \
		idcheck.c getinst_targ.o getmaj_targ.o entry_targ.o util_targ.o

idmknod: idmknod.c \
	getinst_targ.o \
	getmaj_targ.o \
	devconf_targ.o \
	entry_targ.o \
	mdep_targ.o \
	util_targ.o \
	inst.h \
	devconf.h \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/ctype.h \
	$(INC)/sys/types.h \
	$(INC)/errno.h \
	$(INC)/mac.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/mkdev.h \
	$(INC)/sys/dirent.h \
	$(INC)/sys/stat.h
	$(CC) -I$(INC) $(CFLAGS) $(LDFLAGS) $(ROOTLIBS) \
		-o idmknod idmknod.c getinst_targ.o getmaj_targ.o \
		devconf_targ.o entry_targ.o mdep_targ.o util_targ.o

idmkinit: idmkinit.c \
	inst.h \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/errno.h \
	$(INC)/varargs.h \
	$(INC)/ctype.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/dirent.h
	$(CC) -I$(INC) $(CFLAGS) $(LDFLAGS) $(ROOTLIBS) \
		-o idmkinit idmkinit.c

idspace: idspace.c \
	$(INC)/stdio.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/types.h \
	$(INC)/sys/statvfs.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/mnttab.h \
	$(INC)/ustat.h
	$(CC) -I$(INC) $(CFLAGS) $(LDFLAGS) $(ROOTLIBS) \
		-o idspace idspace.c

idmkenv: idmkenv.c \
	inst.h \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/fcntl.h \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/mod.h \
	$(INC)/sys/dirent.h \
	$(INC)/pwd.h \
	$(INC)/grp.h \
	$(INC)/varargs.h
	$(CC) -I$(INC) $(CFLAGS) $(LDFLAGS) $(ROOTLIBS) \
		-o idmkenv idmkenv.c

idval: idval.c \
	$(INC)/stdio.h
	$(CC) -I$(INC) $(CFLAGS) $(LDFLAGS) $(ROOTLIBS) \
		-o idval idval.c

getinst_targ.o: getinst.c \
	inst.h \
	devconf.h \
	mdep.h \
	$(INC)/ctype.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/dirent.h \
	$(INC)/sys/dirent.h
	rm -f getinst_targ.c; cp getinst.c getinst_targ.c
	$(CC) -I$(INC) $(CFLAGS) -c getinst_targ.c
	rm -f getinst_targ.c

getinst_host.o: getinst.c \
	inst.h \
	mdep.h \
	$(HOSTINC)/ctype.h \
	$(HOSTINC)/sys/types.h \
	$(HOSTINC)/sys/stat.h \
	$(HOSTINC)/dirent.h \
	$(HOSTINC)/sys/dirent.h
	if [ "$(PFX)" ]; then \
		rm -f getinst_host.c; cp getinst.c getinst_host.c; \
		$(HCC) -I$(HOSTINC) -c getinst_host.c; \
		rm -f getinst_host.c; \
	fi

getmaj_targ.o: getmajors.c \
	inst.h \
	$(INC)/ctype.h
	rm -f getmaj_targ.c; cp getmajors.c getmaj_targ.c
	$(CC) -I$(INC) $(CFLAGS) -c getmaj_targ.c
	rm -f getmaj_targ.c

getmaj_host.o: getmajors.c \
	inst.h \
	$(HOSTINC)/ctype.h
	if [ "$(PFX)" ]; then \
		rm -f getmaj_host.c; cp getmajors.c getmaj_host.c; \
		$(HCC) -I$(HOSTINC) -c getmaj_host.c; \
		rm -f getmaj_host.c; \
	fi

devconf_targ.o: devconf.c \
	devconf.h \
	defines.h \
	inst.h
	rm -f devconf_targ.c; cp devconf.c devconf_targ.c
	$(CC) -I$(INC) $(CFLAGS) -c devconf_targ.c
	rm -f devconf_targ.c

devconf_host.o: devconf.c \
	devconf.h \
	defines.h \
	inst.h
	if [ "$(PFX)" ]; then \
		rm -f devconf_host.c; cp devconf.c devconf_host.c; \
		$(HCC) -I$(HOSTINC) -c devconf_host.c; \
		rm -f devconf_host.c; \
	fi

getfunc_targ.o: getfunc.c \
	defines.h \
	inst.h \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/errno.h \
	$(INC)/libelf.h
	rm -f getfunc_targ.c; cp getfunc.c getfunc_targ.c
	$(CC) -I$(INC) $(CFLAGS) -c getfunc_targ.c
	rm -f getfunc_targ.c

getfunc_host.o: getfunc.c \
	defines.h \
	inst.h \
	$(HOSTINC)/stdio.h \
	$(HOSTINC)/fcntl.h \
	$(HOSTINC)/errno.h \
	$(INC)/libelf.h \
	$(LIBELF)
	if [ "$(PFX)" ]; then \
		rm -f getfunc_host.c; cp getfunc.c getfunc_host.c; \
		$(HCC) -I. -I$(HOSTINC) -I$(INC) -c getfunc_host.c; \
		rm -f getfunc_host.c; \
	fi

entry_targ.o: entry.c \
	defines.h \
	devconf.h \
	inst.h \
	$(INC)/malloc.h
	rm -f entry_targ.c; cp entry.c entry_targ.c
	$(CC) -I$(INC) $(CFLAGS) -c entry_targ.c
	rm -f entry_targ.c

entry_host.o: entry.c \
	defines.h \
	devconf.h \
	inst.h \
	$(HOSTINC)/malloc.h
	if [ "$(PFX)" ]; then \
		rm -f entry_host.c; cp entry.c entry_host.c; \
		$(HCC) -I$(HOSTINC) -c entry_host.c; \
		rm -f entry_host.c; \
	fi

fdep_targ.o: fdep.c \
	defines.h \
	devconf.h \
	inst.h \
	$(INC)/sys/elf.h \
	$(INC)/stdio.h
	rm -f fdep_targ.c; cp fdep.c fdep_targ.c
	$(CC) -I$(INC) $(CFLAGS) -c fdep_targ.c
	rm -f fdep_targ.c

fdep_host.o: fdep.c \
	defines.h \
	devconf.h \
	inst.h \
	$(INC)/sys/elf.h \
	$(SYS_ELF) \
	$(HOSTINC)/stdio.h
	if [ "$(PFX)" ]; then \
		rm -f fdep_host.c; cp fdep.c fdep_host.c; \
		$(HCC) -I. -I$(HOSTINC) -I$(INC) -c fdep_host.c; \
		rm -f fdep_host.c; \
	fi

mdep_targ.o: mdep.c \
	defines.h \
	devconf.h \
	inst.h \
	mdep.h
	rm -f mdep_targ.c; cp mdep.c mdep_targ.c
	$(CC) -I$(INC) $(CFLAGS) -c mdep_targ.c
	rm -f mdep_targ.c

mdep_host.o: mdep.c \
	defines.h \
	devconf.h \
	inst.h \
	mdep.h
	if [ "$(PFX)" ]; then \
		rm -f mdep_host.c; cp mdep.c mdep_host.c; \
		$(HCC) -I$(HOSTINC) -c mdep_host.c; \
		rm -f mdep_host.c; \
	fi

util_targ.o: util.c \
	$(INC)/stdio.h \
	inst.h
	rm -f util_targ.c; cp util.c util_targ.c
	$(CC) -I$(INC) $(CFLAGS) -c util_targ.c
	rm -f util_targ.c

util_host.o: util.c \
	$(HOSTINC)/stdio.h \
	inst.h
	if [ "$(PFX)" ]; then \
		rm -f util_host.c; cp util.c util_host.c; \
		$(HCC) -I$(HOSTINC) -c util_host.c; \
		rm -f util_host.c; \
	fi

idmodreg: idmodreg.c \
	defines.h \
	inst.h \
	$(INC)/ctype.h \
	$(INC)/stdlib.h \
	$(INC)/fcntl.h \
	$(INC)/errno.h \
	$(INC)/varargs.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/mod.h \
	$(INC)/unistd.h
	$(CC) -I$(INC) $(CFLAGS) $(ROOTLIBS) -o idmodreg idmodreg.c

idmodload: idmodload.c \
	getinst_targ.o \
	getmaj_targ.o \
	devconf_targ.o \
	entry_targ.o \
	mdep_targ.o \
	util_targ.o \
	inst.h \
	defines.h \
	devconf.h \
	$(INC)/sys/mod.h
	$(CC) -I$(INC) $(CFLAGS) $(ROOTLIBS) -o idmodload \
	idmodload.c getinst_targ.o getmaj_targ.o devconf_targ.o \
	entry_targ.o mdep_targ.o util_targ.o

$(SYS_MOD): $(INC)/sys/mod.h
	[ -d $(SYS_DIR) ] || mkdir $(SYS_DIR)
	cp $(INC)/sys/mod.h sys/mod.h

$(SYS_ELF): $(INC)/sys/elf.h
	[ -d $(SYS_DIR) ] || mkdir $(SYS_DIR)
	cp $(INC)/sys/elf.h sys/elf.h
	
$(LIBELF): $(SYS_ELF) $(INC)/libelf.h
	cp $(INC)/libelf.h libelf.h
