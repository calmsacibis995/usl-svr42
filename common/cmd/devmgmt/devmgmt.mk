#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)devmgmt:common/cmd/devmgmt/devmgmt.mk	1.7.6.2"
#ident "$Header: devmgmt.mk 1.3 91/06/27 $"

include $(CMDRULES)

SUBMAKES=devattr getdev getdgrp listdgrp devreserv devfree data putdev putdgrp getvol ddbconv

foo		: all

.DEFAULT	:	
		for submk in $(SUBMAKES) ; \
		do \
		    cd $$submk ; \
		    $(MAKE) -f $$submk.mk $(MAKEARGS) $@ ; \
		    cd .. ; \
		done
