#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:acc/acc.mk	1.9"
#ident "$Header: acc.mk 1.1 91/03/21 $"

include $(UTSRULES)

all:
	-@for i in `ls`;\
	do\
		if [ -d $$i ]; then\
			test ! -d $$i/$$i.cf && {\
				ls -F $$i | grep / >/dev/null 2>&1;\
				test $$? -ne 0 && continue;\
			};\
			cd  $$i;\
			## if there is no source just build the ID target;\
			unset target;\
			test -z "`ls *.c 2>/dev/null`" && target=ID;\
			echo "====== $(MAKE) -f $$i.mk $$target" ; \
			$(MAKE) -f $$i.mk $$target $(MAKEARGS) ; \
			cd .. ; \
		fi;\
	done

depend:: makedep
	-@for i in `ls`;\
	do\
		if [ -d $$i ]; then\
			test ! -d $$i/$$i.cf && {\
				ls -F $$i | grep / >/dev/null 2>&1;\
				test $$? -ne 0 && continue;\
			};\
			cd  $$i;\
			echo "====== $(MAKE) -f $$i.mk depend" ; \
			$(MAKE) -f $$i.mk depend MAKEFILE=$$i.mk $(MAKEARGS) ; \
			cd .. ; \
		fi;\
	done

clean:
	@for i in `ls`; \
	do \
		if [ -d $$i ]; then \
			cd $$i; \
			echo "====== $(MAKE) -f $$i.mk clean" ; \
			$(MAKE) -f $$i.mk clean $(MAKEARGS) ; \
			cd ..; \
		fi; \
	done

clobber:	clean
	@for i in `ls`; \
	do \
		if [ -d $$i ]; then \
			cd $$i; \
			echo "====== $(MAKE) -f $$i.mk clobber" ; \
			$(MAKE) -f $$i.mk clobber $(MAKEARGS) ; \
			cd ..; \
		fi; \
	done

headinstall:
	@for i in `ls`; \
	do \
            	if [ -d $$i ]; then \
                        cd $$i; \
			echo "====== $(MAKE) -f $$i.mk headinstall" ; \
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

