#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:util/kdb/kdb.mk	1.6"
#ident	"$Header: $"

include $(UTSRULES)
KBASE = ../..
all:	subdirs

subdirs:
	@for d in *; \
	do \
		if [ -d $$d -a -f $$d/$$d.mk ]; then \
			(cd $$d; \
			echo "====== $(MAKE) -f $$d.mk all"; \
			$(MAKE) -f $$d.mk all $(MAKEARGS) ); \
		fi; \
	done

depend:: makedep
	@for d in *; \
	do \
		if [ -d $$d -a -f $$d/$$d.mk ]; then \
			(cd $$d; \
			echo "====== $(MAKE) -f $$d/$$d.mk depend"; \
			$(MAKE) -f $$d.mk depend MAKEFILE=$$d.mk $(MAKEARGS) ); \
		fi; \
	done

clean:
	@for d in *; \
	do \
		if [ -d $$d -a -f $$d/$$d.mk ]; then \
			(cd $$d; \
			echo "====== $(MAKE) -f $$d.mk clean"; \
			$(MAKE) -f $$d.mk clean $(MAKEARGS) ); \
		fi; \
	done

clobber:
	@for d in *; \
	do \
		if [ -d $$d -a -f $$d/$$d.mk ]; then \
			(cd $$d; \
			echo "====== $(MAKE) -f $$d.mk clobber"; \
			$(MAKE) -f $$d.mk clobber $(MAKEARGS) ) ; \
		fi; \
	done

headinstall:  \
	$(KBASE)/util/kdb/kdebugger.h \
	$(KBASE)/util/kdb/xdebug.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/util/kdb/kdebugger.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/util/kdb/xdebug.h
FRC: 
 
include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

