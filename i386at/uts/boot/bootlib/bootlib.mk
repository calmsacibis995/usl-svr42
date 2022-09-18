#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#	Copyrighted as an unpublished work.
#	(c) Copyright 1989 INTERACTIVE Systems Corporation
#	All rights reserved.

#	RESTRICTED RIGHTS

#	These programs are supplied under a license.  They may be used,
#	disclosed, and/or copied only as permitted under such license
#	agreement.  Any copy must contain the above copyright notice and
#	this restricted rights notice.  Use, copying, and/or disclosure
#	of the programs is strictly prohibited unless otherwise provided
#	in the license agreement.

#ident	"@(#)uts-x86at:boot/bootlib/bootlib.mk	1.4"
#ident  "$Header: $"

include $(UTSRULES)

#
#	We do not wish to use all the capabilities of
#	the optimizer.  So, we do not use CFLAGS on
#	the compile lines.  Instead, we set our own
#	first-pass options in BOOTOPTIM and use it.

BOOTOPTIM = -W0,-Lb

KBASE = ../..

LIBOBJS = s5fs.o getfhdr.o blfile.o bfs.o

all: bootlib.o 
	
bootlib.o: $(LIBOBJS)
	${LD} -r -o bootlib.o $(LIBOBJS)

#	Special Dependencies

s5fs.o: FRC
	${CC} ${BOOTOPTIM} ${DEFLIST} -c s5fs.c

getfhdr.o: FRC
	${CC} ${BOOTOPTIM} ${DEFLIST} -c getfhdr.c

blfile.o: FRC
	${CC} ${BOOTOPTIM} ${DEFLIST} -c blfile.c

#bfsfilesys.o: FRC
#	${CC} ${BOOTOPTIM} ${DEFLIST} -c bfsfilesys.c

bfs.o: FRC
	${CC} ${BOOTOPTIM} ${DEFLIST} -c bfs.c

install: all 

clean:
	-/bin/rm -f $(LIBOBJS) 

clobber: clean
	-/bin/rm -f *.o 
FRC: 
 
include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

