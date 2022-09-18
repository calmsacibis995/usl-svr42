#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lib:.lib.mk	1.2"

include $(LIBRULES)

LIBLIST=*

install:
	for i in $(LIBLIST); \
	do \
	   if [ -d $$i ]; \
	   then \
		case "$$i" \
		{ \
		libc		|\
		libdl		|\
		libl		|\
		libm		|\
		libmalloc	|\
		liby)	echo $$i; cd $$i; $(MAKE) -f $$i.mk install LIBRULES=$(LIBRULES); cd ..;; \
		*)	echo $$i; cd $$i; $(MAKE) -f $$i.mk install LIBRULES=$(OSRULES); cd ..;; \
		} \
	   fi \
	done
	if [ "$(CPU)" = "u3b2" ] ; \
	then \
		cd libc; $(MAKE) -f libc.mk clobber; \
		$(MAKE) -f libc.mk MAC=$(MAC) CFLAGS="-O -K mau" archive; \
	fi

clean:
	for i in $(LIBLIST); \
	do \
	   if [ -d $$i ]; \
	   then \
	   	echo $$i; cd $$i; $(MAKE) -f $$i.mk clean; cd ..; \
	   fi \
	done

clobber:
	for i in $(LIBLIST); \
	do \
	   if [ -d $$i ]; \
	   then \
	   	echo $$i; cd $$i; $(MAKE) -f $$i.mk clobber; cd ..; \
	   fi \
	done

lintit:
	for i in $(LIBLIST); \
	do \
	   if [ -d $$i ]; \
	   then \
	   	echo $$i; cd $$i; $(MAKE) -f $$i.mk lintit; cd ..; \
	   fi \
	done
