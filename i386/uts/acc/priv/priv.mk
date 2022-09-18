#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:acc/priv/priv.mk	1.10"
#ident	"$Header: $"
#ident "$Header: priv.mk 1.1 91/03/21 $"

include $(UTSRULES)

KBASE    = ../..
SFILE    = 
FILES    = 

all:	dir

ID:	dir

dir:
	@for i in `ls | grep -v '\.cf$'`; \
	do\
		if [ -d $$i ];then\
			cd  $$i;\
			echo $(MAKE) -f $$i.mk ; \
			$(MAKE) -f $$i.mk $(MAKEARGS)  ; \
			cd .. ; \
		fi;\
	done

depend:: makedep
	@for i in `ls | grep -v '\.cf$'`; \
	do\
		if [ -d $$i ];then\
			cd  $$i;\
			echo "====== $(MAKE) -f $$i.mk depend"; \
			$(MAKE) -f $$i.mk depend MAKEFILE=$$i.mk $(MAKEARGS)  ; \
			cd .. ; \
		fi;\
	done

clean:
	@for i in `ls | grep -v '\.cf$'`; \
	do \
		if [ -d $$i ];then\
			cd $$i; \
			$(MAKE) -f $$i.mk clean $(MAKEARGS) ; \
			cd ..; \
		fi; \
	done

clobber:	clean
	@for i in `ls | grep -v '\.cf$'`; \
	do \
		if [ -d $$i ];then\
			cd $$i; \
			$(MAKE) -f $$i.mk clobber $(MAKEARGS) ; \
			cd ..; \
		fi; \
	done

headinstall: \
	$(KBASE)/acc/priv/privilege.h \
	$(FRC)
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/acc/priv/privilege.h
	@for i in `ls | grep -v '\.cf$'`; \
	do \
		if [ -d $$i ]; then \
			cd $$i; \
			$(MAKE) -f $$i.mk headinstall $(MAKEARGS) ; \
			cd ..; \
		fi; \
	done

FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

