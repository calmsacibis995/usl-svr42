#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)vtlmgr:vtlmgr.mk	1.3.3.5"

include	$(CMDRULES)

MAINS	= vtlmgr newvt vtgetty license
OBJECTS	= vtlmgr.o newvt.o vtgetty.o license.o
SOURCES	= vtlmgr.c newvt.c vtgetty.c licence.c

LIMITED = -DLIMITED
CONS 	= -DCONSOLE='"/dev/console"' -DSECURITY $(LIMITED)
LDLIBS	= -lcmd -lgen -lcrypt_i

all: $(MAINS)

install: all
	$(INS) -f $(USRBIN) -m 2555  -u bin  -g tty vtlmgr
	$(INS) -f $(USRBIN) -m 555   -u bin  -g bin newvt
	$(INS) -f $(USRSBIN)   -m 544   -u root -g bin vtgetty
	$(INS) -f $(USRBIN) -m 555   -u bin  -g bin license

vtlmgr:	vtlmgr.o
	$(CC) -o vtlmgr vtlmgr.o $(LDFLAGS)

license:	license.o
	$(CC) -o license license.o $(LDFLAGS) $(LDLIBS)

vtlmgr.o: $(INC)/fcntl.h \
	$(INC)/signal.h \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/sys/at_ansi.h \
	$(INC)/sys/kd.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/vt.h

newvt: newvt.o
	$(CC) -o newvt newvt.o $(LDFLAGS) 

newvt.o: $(INC)/fcntl.h \
	$(INC)/signal.h \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/ctype.h \
	$(INC)/sys/at_ansi.h \
	$(INC)/sys/kd.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/vt.h

vtgetty: vtgetty.o
	$(CC) -o vtgetty vtgetty.o $(LDFLAGS)

vtgetty.o: $(INC)/fcntl.h \
	$(INC)/signal.h \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/sys/at_ansi.h \
	$(INC)/sys/kd.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/vt.h

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)
