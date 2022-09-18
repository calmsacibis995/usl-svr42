#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.



#ident	"@(#)uts-x86:net/loopback/loopback.mk	1.4"
#ident	"$Header: $"
#ident "$Header: proc.mk 1.5 91/03/28 $"

include $(UTSRULES)

KBASE = ../..
all:
	@for i in ticlts ticots ticotsord;\
	do \
		$(MAKE) -f $$i.mk all $(MAKEARGS); \
	done

depend:: makedep
	@for i in ticlts ticots ticotsord;\
	do \
		echo "====== $(MAKE) -f $$i.mk depend";\
		$(MAKE) -f $$i.mk depend MAKEFILE=$$i.mk $(MAKEARGS); \
	done

clean:
	-rm -f *.o

clobber:	clean
	@for i in ticlts ticots ticotsord;\
	do \
		$(MAKE) -f $$i.mk clobber $(MAKEARGS) ; \
	done

headinstall:
	$(MAKE) -f ticlts.mk headinstall $(MAKEARGS)
	$(MAKE) -f ticots.mk headinstall $(MAKEARGS)
	$(MAKE) -f ticotsord.mk headinstall $(MAKEARGS)

FRC: 
 
include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

