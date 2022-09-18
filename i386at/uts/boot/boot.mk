#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.



#	Copyright (c) 1987, 1988 Microsoft Corporation
#	  All Rights Reserved

#	This Module contains Proprietary Information of Microsoft
#	Corporation and should be treated as Confidential.

#ident	"@(#)uts-x86at:boot/boot.mk	1.3"
#ident  "$Header: $"

include $(UTSRULES)

HDTSTON = 0

all install:
	cd bootlib; $(MAKE) -f bootlib.mk $@ $(MAKEARGS)
	cd at386; $(MAKE) $@ $(MAKEARGS)
	if [ ${HDTSTON} = "1" ] ; then \
		$(MAKE) hdboot $(MAKEARGS) ; \
	fi
	echo "**** Boot installation completed" > /dev/null

depend:: makedep
	@cd  bootlib;\
	echo "====== $(MAKE) -f bootlib.mk depend" ; \
	$(MAKE) -f bootlib.mk depend MAKEFILE=bootlib.mk $(MAKEARGS) 
	@echo "====== $(MAKE) -f at386/Makefile depend" ; \
	cd at386; $(MAKE) depend MAKEFILE=Makefile $(MAKEARGS) 

clean clobber :
	cd bootlib; $(MAKE) -f bootlib.mk $@ $(MAKEARGS)
	cd at386; $(MAKE) $@ $(MAKEARGS)

FRC: 
 
include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

