#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)nserve:nserve.mk	1.11.21.2"
#ident  "$Header: nserve.mk 1.6 91/07/03 $"

include $(CMDRULES)


OWN = bin
GRP = bin

INSDIR = $(USRLIB)/rfs
LDLIBS = -lcrypt_i -lns
LINTFLAGS = -pua
LLIB   = $(CCSLIB)/llib-lns.ln
MAINS  = TPnserve nserve

SOURCES = TPnserve.c nsrec.c nsfunc.c nsdb.c
OBJECTS = $(SOURCES:.c=.o)

all:	$(MAINS)

TPnserve: $(OBJECTS)
	if [ x$(CCSTYPE) = xCOFF ] ; \
	then \
		$(CC) -o $@ $(OBJECTS) $(LDFLAGS) $(LDLIBS) -lnsl_s $(SHLIBS); \
	else \
		$(CC) -o $@ $(OBJECTS) $(LDFLAGS) $(LDLIBS) -lnsl $(SHLIBS); \
	fi

nserve: nserve.o
	if [ x$(CCSTYPE) = xCOFF ] ; \
	then \
		$(CC) -o $@ nserve.o $(LDFLAGS) $(LDLIBS) -lnsl_s $(SHLIBS); \
	else \
		$(CC) -o $@ nserve.o $(LDFLAGS) $(LDLIBS) -lnsl $(SHLIBS); \
	fi

TPnserve.o: TPnserve.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/string.h \
	$(INC)/signal.h \
	$(INC)/sys/utsname.h \
	$(INC)/errno.h \
	$(INC)/fcntl.h \
	nsdb.h \
	nslog.h \
	$(INC)/sys/types.h \
	$(INC)/nsaddr.h \
	stdns.h \
	$(INC)/nserve.h \
	nsports.h \
	nsrec.h \
	$(INC)/unistd.h \
	$(INC)/sys/rf_sys.h

nsdb.o: nsdb.c \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/ctype.h \
	nslog.h \
	$(INC)/sys/types.h \
	nsdb.h \
	stdns.h \
	$(INC)/nserve.h \
	nsrec.h \
	$(INC)/nsaddr.h

nserve.o: nserve.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/tiuser.h \
	$(INC)/signal.h \
	$(INC)/errno.h \
	$(INC)/fcntl.h \
	$(INC)/nserve.h \
	$(INC)/nsaddr.h \
	nsdb.h \
	stdns.h \
	nslog.h \
	$(INC)/sys/types.h \
	$(INC)/unistd.h

nsfunc.o: nsfunc.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/unistd.h \
	$(INC)/string.h \
	$(INC)/sys/utsname.h \
	nslog.h \
	$(INC)/sys/types.h \
	nsdb.h \
	stdns.h \
	$(INC)/nserve.h \
	$(INC)/nsaddr.h

nsrec.o: nsrec.c \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/signal.h \
	$(INC)/sys/utsname.h \
	$(INC)/sys/types.h \
	$(INC)/errno.h \
	$(INC)/fcntl.h \
	nsdb.h \
	nslog.h \
	$(INC)/sys/types.h \
	$(INC)/sys/tiuser.h \
	$(INC)/nsaddr.h \
	stdns.h \
	$(INC)/nserve.h \
	nsports.h \
	nsrec.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/rf_cirmgr.h \
	$(INC)/pn.h

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR); \
		$(CH)chmod 775 $(INSDIR); \
		$(CH)chgrp sys $(INSDIR); \
		$(CH)chown root $(INSDIR); \
	[ -d $(USRNSERVE) ] || mkdir -p $(USRNSERVE)
	-rm -f $(USRNSERVE)/TPnserve
	-rm -f $(USRNSERVE)/nserve
	$(INS) -f $(INSDIR) -m 0550 -u $(OWN) -g $(GRP) nserve
	$(INS) -f $(INSDIR) -m 0550 -u $(OWN) -g $(GRP) TPnserve
	-$(SYMLINK) /usr/lib/rfs/TPnserve $(USRNSERVE)/TPnserve
	-$(SYMLINK) /usr/lib/rfs/nserve $(USRNSERVE)/nserve

clean:
	-rm -f $(OBJECTS)

clobber: clean
	-rm -f $(MAINS)

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES) $(LLIB)
	$(LINT) $(LINTFLAGS) nserve.c $(LLIB)

#	optional targets

debug:
	$(MAKE) -f nserve.mk LOCALDEF="-g -DLOGGING -DLOGMALLOC" all

dashg:
	if [ x$(CCSTYPE) = xCOFF ] ; \
	then \
		$(MAKE) -f nserve.mk LDLIBS="-lnsl_s -lnsdb -lcrypt" LOCALDEF="-DLOGGING -DLOGMALLOC" CFLAGS="-g" all; \
	else \
		$(MAKE) -f nserve.mk LDLIBS="-Bstatic -lnsl -Bdynamic -lnsdb -lcrypt" LOCALDEF="-DLOGGING -DLOGMALLOC" CFLAGS="-g" all; \
	fi

