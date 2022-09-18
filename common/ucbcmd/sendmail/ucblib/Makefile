#ident	"@(#)ucb:common/ucbcmd/sendmail/ucblib/Makefile	1.2"
#ident	"$Header: $"
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.




#		PROPRIETARY NOTICE (Combined)
#
#This source code is unpublished proprietary information
#constituting, or derived under license from AT&T's UNIX(r) System V.
#In addition, portions of such source code were derived from Berkeley
#4.3 BSD under license from the Regents of the University of
#California.
#
#
#
#		Copyright Notice 
#
#Notice of copyright on this source code product does not indicate 
#publication.
#
#	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
#	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#	          All rights reserved.

include $(CMDRULES)

LOCALINC = -I../include 

LDLIBS = -dy -lnsl -lsocket

INSDIR = $(ROOT)/$(MACH)/usr/ucblib

OWN = root

all:	binrmail binmail

binmail:	mail.o
	$(CC) mail.o -o binmail	 $(LDLIBS) $(LDFLAGS) $(PERFLIBS)

binrmail:	rmail.o
	$(CC) rmail.o -o binrmail $(LDFLAGS) $(PERFLIBS)

install:	binmail binrmail
	$(INS) -f $(INSDIR) -m 4551 -u $(OWN) binmail 
	$(INS) -f $(INSDIR) -m 4551 -u $(OWN) binrmail 

clean:	
	rm -f mail.o rmail.o
clobber:	clean
	rm -f binmail binrmail
