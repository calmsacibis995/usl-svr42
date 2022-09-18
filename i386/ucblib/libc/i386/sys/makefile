#ident	"@(#)ucb:i386/ucblib/libc/i386/sys/makefile	1.2"
#ident	"$Header: $"
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#
#
# makefile for ucblib/libc/i386/sys
#

include $(LIBRULES)

CFLAGS=-c

OBJECTS= setjmp.o syscall.o _times.o

SOURCES= setjmp.s syscall.s _times.s

.s.o:
	$(CC) $(CFLAGS) $*.s

ALL:	$(OBJECTS)

all:	ALL

clean:
	rm -rf *.o

clobber:
	rm -rf *.o
