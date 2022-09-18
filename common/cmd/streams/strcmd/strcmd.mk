#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)cmd-streams:strcmd/strcmd.mk	1.5.6.3"
#ident "$Header: strcmd.mk 1.2 91/03/20 $"

include $(CMDRULES)

OWN = root
GRP = root

MAINS = strchg strconf
SOURCES = strchg.c strconf.c

all: $(MAINS)

strchg: strchg.c \
	$(INC)/stdio.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/sad.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/types.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

strconf: strconf.c \
	$(INC)/stdio.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stropts.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	for n in $(MAINS) ; do \
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) $$n ; \
	done
	
clean:
	rm -f *.o

clobber: clean
	-rm -f $(MAINS)

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)
