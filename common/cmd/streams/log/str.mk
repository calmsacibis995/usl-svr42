#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)cmd-streams:log/str.mk	1.5.6.2"
#ident "$Header: str.mk 1.2 91/03/20 $"

include $(CMDRULES)

OWN = root
GRP = sys

SOURCES = strace.c strerr.c strclean.c
MAINS = strace strerr strclean

all: $(MAINS)

strace: strace.c \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/time.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/strlog.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

strerr: strerr.c \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/time.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/strlog.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

strclean: strclean.c \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/ftw.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/strlog.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	-rm -f $(USRBIN)/strace
	-rm -f $(USRBIN)/strerr
	-rm -f $(USRBIN)/strclean
	$(INS) -f $(USRSBIN) -m 0100 -u $(OWN) -g $(GRP) strace
	$(INS) -f $(USRSBIN) -m 0100 -u $(OWN) -g $(GRP) strerr
	$(INS) -f $(USRSBIN) -m 0100 -u $(OWN) -g $(GRP) strclean
	-$(SYMLINK) /usr/sbin/strace $(USRBIN)/strace
	-$(SYMLINK) /usr/sbin/strerr $(USRBIN)/strerr
	-$(SYMLINK) /usr/sbin/strclean $(USRBIN)/strclean
	
clean:
	rm -f *.o
	
clobber: clean
	-rm -f $(MAINS)

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)
