#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)idtools:i386/ktool/ktool.mk	1.6"
#ident	"$Header: $"
#
# makefile for ktool kernel build tools
#

include $(CMDRULES)

#
#
#
all:
	@for i in `ls`;\
	do\
		if [ -d $$i ]; then\
			cd  $$i;\
			echo "====== $(MAKE) -f $$i.mk all" ; \
			$(MAKE) -f $$i.mk all; \
			cd .. ; \
		fi;\
	done

#
#
#
install:
	@for i in `ls`;\
	do\
		if [ -d $$i ]; then\
			cd  $$i;\
			echo "====== $(MAKE) -f $$i.mk install" ; \
			$(MAKE) -f $$i.mk install; \
			cd .. ; \
		fi;\
	done

#
#
#
clean:  
	@for i in `ls`;\
	do\
		if [ -d $$i ]; then\
			cd  $$i;\
			echo "====== $(MAKE) -f $$i.mk clean" ; \
			$(MAKE) -f $$i.mk clean; \
			cd .. ; \
		fi;\
	done

#
#
#
clobber:
	@for i in `ls`;\
	do\
		if [ -d $$i ]; then\
			cd  $$i;\
			echo "====== $(MAKE) -f $$i.mk clobber" ; \
			$(MAKE) -f $$i.mk clobber; \
			cd .. ; \
		fi;\
	done

#
#
#
lintit:
	@for i in `ls`;\
	do\
		if [ -d $$i ]; then\
			cd  $$i;\
			echo "====== $(MAKE) -f $$i.mk lintit" ; \
			$(MAKE) -f $$i.mk lintit; \
			cd .. ; \
		fi;\
	done

#
#
#
fnames:
	@for i in `ls`;\
	do\
		if [ -d $$i ]; then\
			cd  $$i;\
			echo "====== $(MAKE) -f $$i.mk fnames" ; \
			$(MAKE) -f $$i.mk fnames; \
			cd .. ; \
		fi;\
	done


#
#
#
xenv:
	@for i in `ls`;\
	do\
		if [ -d $$i ]; then\
			cd  $$i;\
			echo "====== $(MAKE) -f $$i.mk xenv" ; \
			$(MAKE) -f $$i.mk xenv; \
			cd .. ; \
		fi;\
	done

xenv_install:
	@for i in `ls`;\
	do\
		if [ -d $$i ]; then\
			cd  $$i;\
			echo "====== $(MAKE) -f $$i.mk xenv_install" ; \
			$(MAKE) -f $$i.mk xenv_install; \
			cd .. ; \
		fi;\
	done

idtools.xenv_install:
	(cd idtools; $(MAKE) -f idtools.mk xenv_install)

unixsyms.xenv_install:
	(cd unixsyms; $(MAKE) -f unixsyms.mk xenv_install)
