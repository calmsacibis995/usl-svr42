#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1991, 1992  Intel Corporation
#	All Rights Reserved
#
#	INTEL CORPORATION CONFIDENTIAL INFORMATION
#
#	This software is supplied to USL under the terms of a license 
#	agreement with Intel Corporation and may not be copied nor 
#	disclosed except in accordance with the terms of that agreement.

#ident	"@(#)cdfs.cmds:cdfs/cddrec/cddrec.mk	1.5"
#ident	"$Header: $"

# Tabstops: 4


include		$(CMDRULES)

INSDIR		= $(USRLIB)/fs/cdfs
OWN			= bin
GRP			= bin

LDLIBS		= -lcdfs -lgen


all:	cddrec

cddrec:	cddrec.o
	$(CC) $(LDFLAGS) -o $@ $@.o $(LDLIBS)

install:	all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) cddrec

headinstall:

lintit:
	$(LINT) $(LINTFLAGS) $(DEFLIST) cddrec.c

clean:
	-rm -f *.o
	-rm -f *.ln

clobber:	clean
	rm -f cddrec

cddrec.o:	cddrec.c \
	cddrec.h \
	$(INC)/errno.h \
	$(INC)/libgen.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/sys/cdrom.h \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/fs/cdfs_fs.h \
	$(INC)/sys/fs/cdfs_ioctl.h

