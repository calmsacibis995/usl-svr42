#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)cs:cs.mk	1.5.1.2"
#ident "$Header: cs.mk 1.2 91/03/25 $"

# 
# Creates CS daemon and reportscheme service
#

include $(CMDRULES)

all: 
	@for i in `ls`;\
	do\
		if test -d $$i;then\
		case $$i in\
		*.*)\
			;;\
		*)\
		cd  $$i;\
		echo "===== $(MAKE) -f $$i.mk all";\
		$(MAKE) -f $$i.mk $(MAKEARGS); \
		cd .. ;;\
		esac;\
		fi;\
	done

install:  all 
	@for i in `ls`;\
	do\
		if test -d $$i;then\
		case $$i in\
		*.*)\
			;;\
		*)\
		cd  $$i;\
		$(MAKE) -f $$i.mk $(MAKEARGS) install; \
		cd .. ;;\
		esac;\
		fi;\
	done

clean:
	@for i in `ls`;\
	do\
		if test -d $$i;then\
		case $$i in\
		*.*)\
			;;\
		*)\
		cd  $$i;\
		$(MAKE) -f $$i.mk $(MAKEARGS) clean; \
		cd .. ;;\
		esac;\
		fi;\
	done

clobber:	clean
	@for i in `ls`;\
	do\
		if test -d $$i;then\
		case $$i in\
		*.*)\
			;;\
		*)\
		cd  $$i;\
		$(MAKE) -f $$i.mk $(MAKEARGS) clobber; \
		cd .. ;;\
		esac;\
		fi;\
	done
