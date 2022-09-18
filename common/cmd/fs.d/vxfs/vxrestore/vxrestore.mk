#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

# @(#)usr/src/common/cmd/fs.d/vxfs/vxrestore/vxrestore.mk	1.5 18 May 1992 18:27:37 - 
#ident	"@(#)vxfs.cmds:common/cmd/fs.d/vxfs/vxrestore/vxrestore.mk	1.4"

# Copyright (c) 1991, 1992 VERITAS Software Corporation.  ALL RIGHTS RESERVED.
# UNPUBLISHED -- RIGHTS RESERVED UNDER THE COPYRIGHT
# LAWS OF THE UNITED STATES.  USE OF A COPYRIGHT NOTICE
# IS PRECAUTIONARY ONLY AND DOES NOT IMPLY PUBLICATION
# OR DISCLOSURE.
# 
# THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
# TRADE SECRETS OF VERITAS SOFTWARE.  USE, DISCLOSURE,
# OR REPRODUCTION IS PROHIBITED WITHOUT THE PRIOR
# EXPRESS WRITTEN PERMISSION OF VERITAS SOFTWARE.
# 
#               RESTRICTED RIGHTS LEGEND
# USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT IS
# SUBJECT TO RESTRICTIONS AS SET FORTH IN SUBPARAGRAPH
# (C) (1) (ii) OF THE RIGHTS IN TECHNICAL DATA AND
# COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013.
#               VERITAS SOFTWARE
# 4800 GREAT AMERICA PARKWAY, SUITE 420, SANTA CLARA, CA 95054

include $(CMDRULES)
LIBDIR	= ../lib
LOCALINC=-I$(LIBDIR)
INSDIR1 = $(USRLIB)/fs/vxfs
INSDIR2 = $(USRSBIN)
OWN = bin
GRP = bin
OBJS= dirs.o interact.o main.o restore.o symtab.o tape.o utilities.o

PROBEFILE = dirs.c
MAKEFILE = vxrestore.mk
BINARIES = vxrestore

all:
	@if [ -f $(PROBEFILE) ]; then \
		find $(BINARIES) \( ! -type f -o -links +1 \) \
		    -exec echo rm -f {} \; -exec rm -f {} \; 2> /dev/null ;\
		$(MAKE) -f $(MAKEFILE) binaries $(MAKEARGS) ;\
	else \
		for fl in $(BINARIES); do \
			if [ ! -f $$fl ]; then \
				echo "ERROR: $$fl is missing" 1>&2 ;\
				false ;\
				break ;\
			fi \
		done \
	fi

install: all
	[ -d $(INSDIR1) ] || mkdir -p $(INSDIR1)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) vxrestore
	-rm -f $(INSDIR2)/vxrestore
	ln $(INSDIR1)/vxrestore $(INSDIR2)/vxrestore

clean:
	-rm -f $(OBJS)

clobber: clean
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

binaries: $(BINARIES)


vxrestore: $(OBJS) $(LIBDIR)/libvxfs.a
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBDIR)/libvxfs.a $(LDLIBS) $(SHLIBS)

dirs.o:	dirs.c \
	restore.h \
	dumprest.h \
	$(INC)/sys/file.h

interact.o:	interact.c \
	restore.h \
	dumprest.h \
	$(INC)/setjmp.h

main.o:	main.c \
	restore.h \
	dumprest.h \
	$(INC)/signal.h

restore.o:	restore.c \
	restore.h

symtab.o:	symtab.c \
	restore.h \
	$(INC)/sys/stat.h

tape.o:	tape.c \
	restore.h \
	dumprest.h \
	$(INC)/sys/ioctl.h \
	$(INC)/sys/fs/vx_ioctl.h \
	$(INC)/sys/file.h \
	$(INC)/setjmp.h \
	$(INC)/sys/statvfs.h \
	$(INC)/sys/stat.h

utilities.o:	utilities.c \
	restore.h
