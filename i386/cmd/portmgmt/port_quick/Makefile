#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)portmgmt:i386/cmd/portmgmt/port_quick/Makefile	1.1.2.2"
#ident	"$Header: $"

include $(CMDRULES)

CFLAGS = -O -I$(INC) -Di386 -DSVR40

all:	layers

layers:
	$(CC) $(CFLAGS) -c layers.c
	$(CC) $(CFLAGS) -o layers layers.o $(LDLIBS) -los

