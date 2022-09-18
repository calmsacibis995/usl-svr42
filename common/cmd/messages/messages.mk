#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)messages:messages.mk	1.3.3.3"
#ident  "$Header: messages.mk 1.4 91/07/02 $"

include $(CMDRULES)

SUBMAKES = uxaudit uxawk uxcore uxcore.abi uxdfm uxed.abi uxmesg uxue \
		uxue.abi uxsysadm uxnsu uxcdfs

foo : all

.DEFAULT : 
	for submk in $(SUBMAKES) ; \
	do \
	 	cd $$submk ; \
	 	$(MAKE) -f msgs.mk $@ ; \
	 	cd .. ; \
	done

