#ident	"@(#)ucb:common/ucbcmd/sendmail/sendmail.mk	1.3"
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

#
#	Makefile for sendmail base directory
#

include $(CMDRULES)

INSDIR1 = $(ROOT)/$(MACH)/usr/ucb

INSDIR = $(ROOT)/$(MACH)/usr/ucblib

ALL = src/sendmail 

DIRS = $(ROOT)/$(MACH)/usr/ucblib/mqueue

SENDMAIL=$(ROOT)/$(MACH)/usr/ucblib/sendmail

all: $(MAKEFILES)
	cd lib; $(MAKE)  all
	cd src; $(MAKE)  all
	cd aux; $(MAKE)  all
	cd cf;  $(MAKE)  all
	cd ucblib;  $(MAKE)  all

clean:
	cd lib;	$(MAKE)  clean
	cd src;	$(MAKE)  clean
	cd aux;	$(MAKE)  clean
	cd cf;  $(MAKE)  clean
	cd ucblib;  $(MAKE)  clean
clobber:
	cd lib;	$(MAKE)  clobber
	cd src;	$(MAKE)  clobber
	cd aux;	$(MAKE)  clobber
	cd cf;  $(MAKE)  clobber
	cd ucblib;  $(MAKE)  clobber

install: all $(ALL) $(DIRS)
	cd src; $(MAKE)  install #DESTDIR=${DESTDIR} install
	cd ucblib;  $(MAKE)  install
	$(INS) -f $(INSDIR) -m 444 lib/sendmail.hf 
#	cp  -m 660	/dev/null	$(SENDMAIL).fc
	$(INS) -f $(INSDIR) -m 444 cf/subsidiary.cf 
	$(INS) -f $(INSDIR) -m 444 cf/main.cf 
	-ln cf/subsidiary.cf cf/sendmail.cf
	$(INS) -f $(INSDIR) -m 444 cf/sendmail.cf 
	$(SYMLINK) $(INSDIR)/sendmail.cf $(ROOT)/$(MACH)/etc/sendmail.cf
	-rm -f cf/sendmail.cf
	$(INS) -f $(INSDIR) lib/aliases
	$(INS) -f $(INSDIR1) aux/vacation
	$(INS) -f $(INSDIR1) aux/mconnect
	$(INS) -f $(INSDIR1) aux/mailstats
	-rm -f $(INSDIR1)/newaliases
	-rm -f $(INSDIR1)/mailq
	$(SYMLINK) $(INSDIR)/sendmail $(INSDIR1)/newaliases
	$(SYMLINK) $(INSDIR)/sendmail $(INSDIR1)/mailq
	-mv $(INSDIR)/mailsurr $(INSDIR)/mailsurr.`/bin/sh -c "echo $$$$"`	
	$(INS) -f $(INSDIR) cf/mailsurr

$(DIRS):
	-mkdir $(DIRS)
	$(SYMLINK) $(DIRS) $(ROOT)/$(MACH)/var/spool/mqueue
