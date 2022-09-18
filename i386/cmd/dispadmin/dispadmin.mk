#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)dispadmin:i386/cmd/dispadmin/dispadmin.mk	1.5.5.6"
#ident "$Header: $"

include $(CMDRULES)
INSDIR = $(USRSBIN)
OWN = bin
GRP = bin
CLASSDIR = $(USRLIB)/class
DIRS = 	\
	$(CLASSDIR) \
	$(CLASSDIR)/RT \
	$(CLASSDIR)/TS \
	$(CLASSDIR)/FC \
	$(CLASSDIR)/VC

LDLIBS = -lgen
CFLAGS = -O -I$(INC)

all: dispadmin classes

dispadmin: dispadmin.o subr.o
	$(CC) dispadmin.o subr.o -o $@ $(LDFLAGS) $(LDLIBS) $(SHLIBS)

dispadmin.o: dispadmin.c \
	$(INC)/stdio.h\
	$(INC)/string.h\
	$(INC)/unistd.h\
	$(INC)/sys/types.h\
	$(INC)/sys/priocntl.h

classes: RTdispadmin TSdispadmin VCdispadmin FCdispadmin

rtdispadmin.o: rtdispadmin.c\
	$(INC)/stdio.h\
	$(INC)/string.h\
	$(INC)/sys/types.h\
	$(INC)/sys/priocntl.h\
	$(INC)/sys/rtpriocntl.h\
	$(INC)/sys/param.h\
	$(INC)/sys/hrtcntl.h\
	$(INC)/sys/rt.h

RTdispadmin: rtdispadmin.o subr.o
	$(CC) rtdispadmin.o subr.o -o $@ $(LDFLAGS) $(LDLIBS) $(SHLIBS)

tsdispadmin: tsdispadmin.c\
	$(INC)/stdio.h\
	$(INC)/string.h\
	$(INC)/sys/types.h\
	$(INC)/sys/priocntl.h\
	$(INC)/sys/tspriocntl.h\
	$(INC)/sys/param.h\
	$(INC)/sys/hrtcntl.h\
	$(INC)/errno.h

TSdispadmin: tsdispadmin.o subr.o
	$(CC) tsdispadmin.o subr.o -o $@ $(LDFLAGS) $(LDLIBS) $(SHLIBS)


fcdispadmin: fcdispadmin.c\
	$(INC)/stdio.h\
	$(INC)/string.h\
	$(INC)/sys/types.h\
	$(INC)/sys/priocntl.h\
	$(INC)/sys/fcpriocntl.h\
	$(INC)/sys/param.h\
	$(INC)/sys/hrtcntl.h\
	$(INC)/errno.h

FCdispadmin: fcdispadmin.o subr.o
	$(CC) fcdispadmin.o subr.o -o $@ $(LDFLAGS) $(LDLIBS) $(SHLIBS)


VCdispadmin: vcdispadmin.o subr.o\
	$(INC)/stdio.h	\
	$(INC)/string.h	\
	$(INC)/unistd.h	\
	$(INC)/sys/unistd.h	\
	$(INC)/sys/types.h	\
	$(INC)/sys/select.h	\
	$(INC)/sys/types.h	\
	$(INC)/sys/priocntl.h	\
	$(INC)/sys/vcpriocntl.h	\
	$(INC)/sys/param.h	\
	$(INC)/sys/types.h	\
	$(INC)/sys/fs/s5param.h	\
	$(INC)/sys/hrtcntl.h	\
	$(INC)/sys/vc.h
	$(CC) $(CFLAGS) vcdispadmin.o subr.o -o VCdispadmin $(LDFLAGS) $(LDLIBS) $(SHLIBS)

subr.o: subr.c\
	$(INC)/stdio.h\
	$(INC)/priv.h\
	$(INC)/sys/types.h\
	$(INC)/sys/hrtcntl.h

$(DIRS):
	-mkdir -p $@

install: all $(DIRS)
	$(INS) -f $(INSDIR) -u $(OWN) -g $(GRP) -m 555 dispadmin
	$(INS) -f $(CLASSDIR)/RT -u $(OWN) -g $(GRP) -m 555 RTdispadmin
	$(INS) -f $(CLASSDIR)/TS -u $(OWN) -g $(GRP) -m 555 TSdispadmin
	$(INS) -f $(CLASSDIR)/VC -u $(OWN) -g $(GRP) -m 555 VCdispadmin
	$(INS) -f $(CLASSDIR)/FC -u $(OWN) -g $(GRP) -m 555 FCdispadmin

clean:
	rm -f *.o

clobber: clean
	rm -f dispadmin RTdispadmin TSdispadmin FCdispadmin VCdispadmin
