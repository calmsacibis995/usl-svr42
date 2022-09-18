#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)libiaf:common/lib/libiaf/libiaf.mk	1.1.2.3"
#ident "$Header: libiaf.mk 1.3 91/03/14 $"

include $(LIBRULES)

# 
# Identification and Authentication Facility Library
#

LDFLAGS=-G -dy -ztext -h /usr/lib/libiaf.so
LOCALDEF = $(PICFLAG)
ARFLAGS=r

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

	-rm -f libiaf.so
	$(CC) -o libiaf.so *.o $(LDFLAGS)
	-rm -f libiaf.a
	$(AR) $(ARFLAGS) libiaf.a *.o

install:  all
		$(INS) -f $(USRLIB) -m 0664 libiaf.so
		$(INS) -f $(USRLIB) -m 0664 libiaf.a

clean:
	-rm -f *.o
	@for i in `ls`;\
	do\
		if test -d $$i;then\
		case $$i in\
		*.*)\
			;;\
		*)\
		cd  $$i;\
		$(MAKE) -f $$i.mk clean $(MAKEARGS); \
		cd .. ;;\
		esac;\
		fi;\
	done

clobber:	clean
	-rm -f libiaf.so
	-rm -f libiaf.a
	@for i in `ls`;\
	do\
		if test -d $$i;then\
		case $$i in\
		*.*)\
			;;\
		*)\
		cd  $$i;\
		$(MAKE) -f $$i.mk clobber $(MAKEARGS); \
		cd .. ;;\
		esac;\
		fi;\
	done
