#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/bkintf.d/Makefile	1.1.4.4"
#ident "$Header: Makefile 1.2 91/03/20 $"

include $(CMDRULES)

LOCAL=hdrs

PRODUCTS=backups

all: $(PRODUCTS)

$(PRODUCTS):
	cd $(@); $(MAKE) $(MAKEARGS) all; cd ..

install: all
	for f in $(PRODUCTS) ; \
	do \
		cd $$f ; \
		$(MAKE) $(MAKEARGS) $(@) ; \
		cd ..; \
	done

size strip lintit clobber clean:
	for f in $(PRODUCTS) ; \
	do \
		cd $$f ; \
		$(MAKE) $(MAKEARGS) $(@) ; \
		cd ..; \
	done
