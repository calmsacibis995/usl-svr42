#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)devintf:i386at/cmd/devintf/devintf.mk	1.3.10.2"
#ident "$Header: devintf.mk 2.0 91/07/11 $"

include $(CMDRULES)

SUBMAKES=devices groups mkdtab

foo	: all

.DEFAULT	:	
		for submk in $(SUBMAKES) ; \
		do \
			cd $$submk ; \
			$(MAKE) -f $$submk.mk $(MAKEARGS) $@ ; \
			cd .. ; \
		done
