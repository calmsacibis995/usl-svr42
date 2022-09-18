#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)rfsetup:cmd/rfsetup/rfsetup.mk	1.9.10.3"
#ident  "$Header: rfsetup.mk 1.3 91/06/28 $"

include $(CMDRULES)


OWN = root
GRP = adm

ELFNSL	= -lnsl
COFFNSL	= -lnsl_s
LDLIBS	= -lns -lcrypt_i
INSDIR	= $(USRNET)/servers/rfs
DIRS	= $(USRNET) $(USRNET)/servers $(USRNET)/servers/rfs

LINTFLAGS= -I$(INC) -ux

all: rfsetup

rfsetup: rfsetup.o
	if [ x$(CCSTYPE) = xCOFF ] ; \
	then \
		$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(COFFNSL) $(SHLIBS) ; \
	else \
		$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(ELFNSL) -dy $(SHLIBS) ;\
	fi

rfsetup.o: rfsetup.c \
	$(INC)/fcntl.h \
	$(INC)/string.h \
	$(INC)/stdio.h \
	$(INC)/errno.h \
	$(INC)/sys/stropts.h \
	$(INC)/nserve.h \
	$(INC)/sys/rf_cirmgr.h \
	$(INC)/sys/types.h \
	$(INC)/sys/rf_sys.h \
	$(INC)/pn.h \
	$(INC)/sys/hetero.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/stat.h

install: all $(DIRS)
	 $(INS) -f $(INSDIR) -m 4550 -u $(OWN) -g $(GRP) rfsetup 

$(DIRS):
	[ -d $@ ] || mkdir -p $@

clean:
	-rm -f *.o

clobber: clean
	-rm -f rfsetup

lintit:
	$(LINT) $(LINTFLAGS) rfsetup.c
