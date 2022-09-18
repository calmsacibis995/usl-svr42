#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)listen:listen.mk	1.14.15.4"
#ident "$Header: listen.mk 1.2 91/06/12 $"

include $(CMDRULES)

#
# listen.mk: makefile for network listener 
#

OWN = root
GRP = sys

# if debug is needed then add -DDEBUGMODE to following line
LOCALDEF = 

LDLIBS = -lc -liaf
LINTFLAGS = -b -x

# change the next line to compile with -g
# DEV1 = -g

INSDIR = $(USRLIB)/saf


SOURCES = \
	listen.c \
	lsdata.c \
	lsdbf.c \
	lslog.c \
	nlsaddr.c \
	nstoa.c

NLPSSRC = \
	nlps_serv.c \
	lsdbf.c \
	lssmb.c \
	lsdata.c \
	lslog.c \
	nstoa.c

HEADER = \
 	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
 	$(INC)/signal.h $(INC)/sys/signal.h \
 	$(INC)/stdio.h \
 	$(INC)/varargs.h \
 	$(INC)/string.h \
 	$(INC)/errno.h $(INC)/sys/errno.h \
 	$(INC)/sys/utsname.h \
 	$(INC)/sys/tiuser.h \
 	$(INC)/sys/param.h \
 	$(INC)/sys/types.h \
 	$(INC)/sys/stat.h \
 	$(INC)/values.h \
 	$(INC)/ctype.h \
 	$(INC)/listen.h \
 	lsparam.h \
 	lsfiles.h \
 	lserror.h \
	lsdbf.h

LSOBJS = \
	listen.o \
	lslog.o \
	lsdbf.o \
	lsdata.o \
	nstoa.o \
	nlsaddr.o

NLPSOBJS = \
	nlps_serv.o \
	lsdbf.o \
	lssmb.o \
	nstoa.o \
	lslog.o \
	lsdata.o

all: listen nlps_server

# 
# SHAREDLIB version
# force the library order so libc is the first
# library
#

listen: $(LSOBJS)
	if [ x$(CCSTYPE) = xCOFF ] ; \
	then \
		 $(CC) -o $@ $(LSOBJS) $(LDFLAGS) $(LDLIBS) -lnsl_s $(SHLIBS) ; \
	else \
		 $(CC) -o $@ $(LSOBJS) $(LDFLAGS) $(LDLIBS) -lnsl $(SHLIBS) ; \
	fi

nlps_server: $(NLPSOBJS)
	if [ x$(CCSTYPE) = xCOFF ] ; \
	then \
		 $(CC) -o $@ $(NLPSOBJS) $(LDFLAGS) $(LDLIBS) -lnsl_s $(SHLIBS) ; \
	else \
		 $(CC) -o $@ $(NLPSOBJS) $(LDFLAGS) $(LDLIBS) -lnsl $(SHLIBS) ; \
	fi

listen.o: listen.c \
	$(HEADER) \
	$(INC)/unistd.h $(INC)/sys/unistd.h \
	$(INC)/memory.h \
	$(INC)/sys/mkdev.h \
	$(INC)/pwd.h \
	$(INC)/grp.h \
	$(INC)/sys/ipc.h \
	$(INC)/sys/poll.h \
	$(INC)/sys/stropts.h \
	$(INC)/sac.h \
	$(INC)/utmp.h \
	lsnlsmsg.h \
	lssmbmsg.h

lsdata.o: lsdata.c \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	lserror.h

lsdbf.o: lsdbf.c \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/ctype.h \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/stropts.h \
	$(INC)/listen.h \
	lsparam.h \
	lsfiles.h \
	lserror.h \
	lsdbf.h

lslog.o: lslog.c \
	$(HEADER) \
	$(INC)/time.h \
	$(INC)/sys/ipc.h

lsnames.o: lsnames.c \
	$(INC)/string.h \
	$(INC)/ctype.h \
	$(INC)/sys/utsname.h \
	lsparam.h \
	lserror.h

lssmb.o: lssmb.c \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/sys/param.h \
	$(INC)/sys/tiuser.h \
	lsparam.h \
	lssmbmsg.h \
	lsdbf.h

nlps_serv.o: nlps_serv.c \
	$(HEADER) \
	$(INC)/unistd.h $(INC)/sys/unistd.h \
	$(INC)/sys/mkdev.h \
	$(INC)/pwd.h \
	$(INC)/grp.h \
	$(INC)/sys/poll.h \
	$(INC)/sys/stropts.h \
	$(INC)/utmp.h \
	$(INC)/sac.h \
	lsnlsmsg.h \
	lssmbmsg.h

nlsaddr.o: nlsaddr.c \
	$(INC)/ctype.h \
	$(INC)/sys/tiuser.h \
	$(INC)/stdio.h

nstoa.o: nstoa.c \
	$(INC)/stdio.h \
	$(INC)/memory.h \
	$(INC)/ctype.h \
	nsaddr.h

install: all $(INSDIR)
	$(INS) -o -f $(INSDIR) -u $(OWN) -g $(GRP) listen
	$(INS) -f $(INSDIR) -u $(OWN) -g $(GRP) nlps_server

$(INSDIR):
	[ -d $@ ] || mkdir -p $@
	$(CH)chown $(OWN) $@
	$(CH)chgrp $(GRP) $@

clean:
	-rm -f *.o

clobber: clean
	-rm -f listen
	-rm -f nlps_server

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)
	$(LINT) $(LINTFLAGS) $(NLPSSRC)
