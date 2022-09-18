#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libns:common/lib/libns/libns.mk	1.10.12.3"
#ident	"$Header: $"


# This makefile makes libns.a, which is the library for
# the name server library.
# NOTE: this library is not for general use.  It is put
# 	in /usr/lib ONLY for the convenience of the
#	commands that use it.
#

include $(LIBRULES)
LIBDIR		= .
LIBNAME		= libns.a
LLIB		= ns
LINTLIB		= llib-l$(LLIB).ln
LOG		= -DLOGGING -DLOGMALLOC
PROFILE		=
DEBUG		=
LOCALDEF	= $(LOG) $(DEBUG) $(PROFILE)
LINTFLAGS	= -uax -DLOGGING

FILES = \
	ind_data.o \
	nsblock.o \
	nsports.o \
	nsrports.o \
	rtoken.o \
	stoa.o \
	astoa.o \
	ns_comm.o \
	nslog.o \
	canon.o \
	logmalloc.o \
	ns_getaddr.o \
	ns_findp.o \
	ns_getblock.o \
	ns_initaddr.o \
	ns_verify.o \
	ns_error.o \
	ns_errlist.o \
	ns_dfinfo.o \
	ns_info.o \
	ns_sendpass.o \
	attconnect.o \
	rfrequest.o \
	negotiate.o \
	getoken.o \
	netname.o \
	uidmap.o \
	rfs_up.o \
	ns_syntax.o \
	swtab.o \
	rfrcv.o \
	rfslck.o

CFILES	= $(FILES:.o=.c)
LNFILES = $(FILES:.o=.ln)


.SUFFIXES: .ln

.c.ln :
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(CFLAGS) -c $*.c

all:	$(LIBNAME) 

debug:
	$(MAKE) -f libns.mk LIBNAME=libnsdb.a DEBUG="-g -DDEBUG" all

lintit: $(LNFILES)
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(LNFILES)

$(LINTLIB) lintlib : 			$(LNFILES)
	$(LINT) $(LINTFLAGS) $(DEFLIST)	$(LNFILES) -o $(LINTLIB) 



install: all
	$(INS) -f $(USRLIB) $(LIBNAME)

uninstall:
	-rm $(USRLIB)/$(LIBNAME)

.PRECIOUS:	$(LIBNAME)

$(LIBNAME):	$(FILES)
	$(AR) $(ARFLAGS) $(LIBNAME) $(FILES)

clean:
	-rm -f *.o *.ln $(LINTLIB)

clobber: clean
	-rm -f $(LIBNAME)

#### dependencies now follow

nsports.o: \
	stdns.h \
	nsports.h \
	nsdb.h \
	$(INC)/nsaddr.h \
	nslog.h

nsrports.o: \
	stdns.h \
	nsports.h \
	nsdb.h \
	$(INC)/nsaddr.h \
	nslog.h \
	$(INC)/pn.h

rtoken.o: \
	stdns.h \
	nsdb.h \
	$(INC)/sys/types.h \
	$(INC)/sys/nserve.h \
	$(INC)/sys/rf_sys.h

ind_data.o: \
	stdns.h \
	nslog.h

nsblock.o: \
	nslog.h \
	nsdb.h \
	stdns.h \
	$(INC)/nserve.h

ns_comm.o: \
	$(INC)/nserve.h \
	$(INC)/nsaddr.h \
	nslog.h \
	stdns.h \
	nsports.h \
	$(INC)/sys/types.h \
	$(INC)/sys/nserve.h \
	$(INC)/sys/rf_sys.h

nslog.o: \
	nslog.h

astoa.o: \
	$(INC)/nsaddr.h

stoa.o: \
	$(INC)/nsaddr.h

ns_getaddr.o: \
	$(INC)/nserve.h \
	$(INC)/nsaddr.h

ns_findp.o: \
	$(INC)/nserve.h \
	$(INC)/nsaddr.h

ns_getblock.o: \
	$(INC)/nserve.h

ns_initaddr.o: \
	$(INC)/nserve.h

ns_verify.o: \
	$(INC)/nserve.h

ns_sendpass.o: \
	$(INC)/nserve.h

attconnect.o: \
	$(INC)/pn.h

rfrequest.o: \
	$(INC)/pn.h

negotiate.o: \
	$(INC)/pn.h \
	$(INC)/sys/types.h \
	$(INC)/sys/nserve.h\
	$(INC)/sys/rf_sys.h

getoken.o: \
	$(INC)/sys/nserve.h \
	$(INC)/sys/rf_cirmgr.h

netname.o: \
	$(INC)/string.h \
	$(INC)/errno.h \
	$(INC)/sys/nserve.h\
	$(INC)/sys/utsname.h \
	$(INC)/sys/types.h\
	$(INC)/sys/rf_sys.h

swtab.o: \
	$(INC)/sys/nserve.h \
	$(INC)/sys/rf_cirmgr.h\
	$(INC)/sys/param.h \
	$(INC)/pn.h

uidmap.o: \
	idload.h \
	$(INC)/sys/types.h \
	$(INC)/sys/rf_sys.h\
	$(INC)/errno.h \
	$(INC)/nserve.h \
	$(INC)/sys/param.h

rfs_up.o: \
	$(INC)/nserve.h \
	$(INC)/sys/types.h \
	$(INC)/sys/nserve.h \
	$(INC)/sys/list.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/rf_messg.h \
	$(INC)/sys/rf_comm.h \
	$(INC)/errno.h \
	$(INC)/sys/rf_sys.h \
	$(INC)/stdio.h nslog.h

ns_syntax.o: \
	$(INC)/nserve.h

rfrcv.o: \
	$(INC)/tiuser.h
