#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)bkrs:common/cmd/bkrs/bkrs.mk	1.1.10.3"
#ident "$Header: bkrs.mk 1.3 91/06/03 $"

include $(CMDRULES)
LOCAL=hdrs
SADMDIR=$(ROOT)/$(MACH)/usr/sadm
SADMVAR=$(VAR)/sadm

BKLIB=bklib
RSLIB=rslib
LIBBR=libbrmeth
IOLIB=libadmIO

PRODUCTS = backup basic bkdaemon bkexcept bkhistory bkintf bkoper bkreg bkstatus \
	   intftools meth rbasic rcmds restore rsintf rsnotify rsoper rsstatus tmeth

.MUTEX:	$(IOLIB)

.MUTEX:	$(IOLIB)

all: $(PRODUCTS)

tmeth $(IOLIB) $(LIBBR) $(BKLIB) $(RSLIB) $(PRODUCTS): 
	cd $(@).d; $(MAKE) $(MAKEARGS) all; cd ..

install: all
	[ -d $(ETC)/bkup ] || mkdir $(ETC)/bkup
	[ -d $(ETC)/bkup/method ] || mkdir $(ETC)/bkup/method
	[ -d $(SBIN) ] || mkdir $(SBIN)
	[ -d $(USRLIB)/getdate ] || mkdir $(USRLIB)/getdate
	[ -d $(USRSBIN) ] || mkdir $(USRSBIN)
	[ -d $(SADMDIR) ] || mkdir $(SADMDIR)
	[ -d $(SADMDIR)/bkup ] || mkdir $(SADMDIR)/bkup
	[ -d $(SADMDIR)/bkup/bin ] || mkdir $(SADMDIR)/bkup/bin
	[ -d $(SADMDIR)/sysadm ] || mkdir $(SADMDIR)/sysadm
	[ -d $(SADMDIR)/sysadm/add-ons ] || mkdir $(SADMDIR)/sysadm/add-ons
	[ -d $(SADMDIR)/sysadm/add-ons/bkrs ] || mkdir $(SADMDIR)/sysadm/add-ons/bkrs
	[ -d $(VAR) ] || mkdir $(VAR)
	[ -d $(SADMVAR) ] || mkdir $(SADMVAR)
	[ -d $(SADMVAR)/bkup ] || mkdir $(SADMVAR)/bkup
	[ -d $(SADMVAR)/bkup/logs ] || mkdir $(SADMVAR)/bkup/logs
	[ -d $(SADMVAR)/bkup/toc ] || mkdir $(SADMVAR)/bkup/toc
	[ -d $(USRLIB)/libadmIO ] || mkdir $(USRLIB)/libadmIO 
	for f in $(PRODUCTS) $(BKLIB) $(LIBBR) $(RSLIB) $(IOLIB) ; \
	do \
		cd $$f.d; \
		$(MAKE) $(MAKEARGS) $(@) ; \
		cd ..; \
	done

strip lintit clobber clean touch: 
	for f in $(IOLIB) $(BKLIB) $(RSLIB) $(LIBBR) $(PRODUCTS) ; \
	do \
		cd $$f.d; \
		$(MAKE) $(MAKEARGS) $(@) ; \
		cd ..; \
	done

tmeth $(PRODUCTS): $(IOLIB) $(BKLIB) $(LIBBR)
restore rsoper: $(RSLIB)
