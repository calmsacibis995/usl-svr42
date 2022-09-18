#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lp.admin:lp.admin.mk	1.14.3.4"
#ident "$Header: lp.admin.mk 2.0 91/07/12 $"

#
#	Makefile for lp.admin: OAM interface for lp
#

include $(CMDRULES)

OWN=root
GRP=sys

PKGMI=$(VAR)/sadm/pkg/lp/save/intf_install
INTFBASE=$(USRSADM)/sysadm/add-ons/lp

DIRS=\
	$(PKGMI) \
	$(INTFBASE)/printers/classes \
	$(INTFBASE)/printers/filters \
	$(INTFBASE)/printers/forms \
	$(INTFBASE)/printers/operations \
	$(INTFBASE)/printers/printers/add \
	$(INTFBASE)/printers/printers/modify \
	$(INTFBASE)/printers/printers/terminfo \
	$(INTFBASE)/printers/priorities \
	$(INTFBASE)/printers/reports \
	$(INTFBASE)/printers/requests \
	$(INTFBASE)/printers/systems

.MUTEX: $(DIRS)

all:

install: all $(DIRS)
	#
	$(INS) -f $(PKGMI) -m 0644 -u $(OWN) -g $(GRP) lp.mi
	#
	find printers -type f -follow -print | \
	sed 's,^\(.*\)/,\1 ,p' | \
	while read dir file ;\
	do \
		$(INS) -f $(INTFBASE)/$$dir -m 0644 -u $(OWN) -g $(GRP) $$dir/$$file || exit 1 ;\
	done
	$(CH)-chmod 755 $(INTFBASE)/printers/printers/add/*.sh
	$(CH)-chmod 755 $(INTFBASE)/printers/*/*.sh

$(DIRS):
	[ -d $@ ] || mkdir -p $@ ;\
		$(CH)chmod 0755 $@ ;\
		$(CH)chgrp $(GRP) $@ ;\
		$(CH)chown $(OWN) $@

clean:

clobber: clean

lintit:
