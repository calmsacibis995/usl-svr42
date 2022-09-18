#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ttymon:i386at/cmd/ttymon/ttymon.mk	1.17.13.4"
#ident "$Header: ttymon.mk 1.1 91/05/27 $"

include $(CMDRULES)

#
# ttymon.mk: makefile for ttymon and stty
#

OWN = root
GRP = sys

# If machine name and /etc/issue file need to be printed 
# before the service prompt is printed, then add -DSYS_NAME to LOCALDEF
LOCALDEF = -DSYS_NAME

# If debug is needed then add -DDEBUG to following line
# If trace is needed then add -DTRACE to following line
LDFLAGS = $(LLDFLAGS)
LDLIBS = -ladm -liaf -lcmd 

ELFNSL = -lnsl
COFFNSL = -lnsl_s

# change the next two lines to compile with -g
# CFLAGS = -g
LLDFLAGS = -s

TTYMONSRC= \
	ttymon.c \
	tmglobal.c \
	tmhandler.c \
	tmpmtab.c \
	tmttydefs.c \
	tmparse.c \
	tmsig.c \
	tmsac.c \
	tmchild.c \
	tmautobaud.c \
	tmterm.c \
	tmutmp.c \
	tmpeek.c \
	tmlog.c \
	tmmach.c \
	tmutil.c \
	tmvt.c \
	tmexpress.c \
	sttytable.c \
	sttyparse.c

TTYADMSRC= \
	ttyadm.c \
	tmutil.c \
	admutil.c 

STTYDEFSSRC= \
	sttydefs.c \
	admutil.c \
	tmttydefs.c \
	tmparse.c \
	sttytable.c \
	sttyparse.c

HDR = \
	ttymon.h \
	tmstruct.h \
	tmextern.h \
	stty.h 

TTYMONOBJ= $(TTYMONSRC:.c=.o)

TTYADMOBJ= $(TTYADMSRC:.c=.o)

STTYDEFSOBJ= $(STTYDEFSSRC:.c=.o)

PRODUCTS = stty ttymon ttyadm sttydefs defsak

all: $(PRODUCTS)
	@echo "           $(MAKE) all done!"

stty: 
	$(MAKE) -f stty.mk $(MAKEARGS)

ttymon: $(TTYMONOBJ)
	if [ x$(CCSTYPE) = xCOFF ] ; \
	then \
	 	$(CC) -o $@ $(TTYMONOBJ) $(LDFLAGS) $(LDLIBS) $(SHLIBS) $(COFFNSL)  ; \
	else \
	 	$(CC) -o $@ $(TTYMONOBJ) $(LDFLAGS) $(LDLIBS) $(SHLIBS) $(ELFNSL)  ; \
	fi

ttyadm: $(TTYADMOBJ)
	$(CC) -o $@ $(TTYADMOBJ) $(LDFLAGS) $(LDLIBS) $(SHLIBS)  

sttydefs: $(STTYDEFSOBJ)
	if [ x$(CCSTYPE) = xCOFF ] ; \
	then \
	 	$(CC) -o $@ $(STTYDEFSOBJ) $(LDFLAGS) $(LDLIBS) $(SHLIBS) $(COFFNSL)  ; \
	else \
	 	$(CC) -o $@ $(STTYDEFSOBJ) $(LDFLAGS) $(LDLIBS) $(SHLIBS) $(ELFNSL)  ; \
	fi

# defsak is conditionally made if its makefile exists.  Its source and makefile
# are only available with the B2 enhanced security package
defsak:
	@if [ -s defsak.mk ] ; \
	then \
	 	echo "\n\tmake -f defsak.mk" ; \
		$(MAKE) -f defsak.mk $(MAKEARGS) ; \
	fi

lintit:
	$(LINT) $(LINTFLAGS) $(TTYMONSRC)
	$(LINT) $(LINTFLAGS) $(TTYADMSRC)
	$(LINT) $(LINTFLAGS) $(STTYDEFSSRC)

ttymon.o: ttymon.c \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/fcntl.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/poll.h \
	$(INC)/string.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/resource.h \
	$(INC)/limits.h \
	$(INC)/pwd.h \
	$(INC)/grp.h \
	$(INC)/mac.h \
	$(INC)/priv.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/sac.h \
	ttymon.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tp.h \
	tmstruct.h \
	tmextern.h

tmglobal.o: tmglobal.c \
	$(INC)/stdio.h \
	$(INC)/poll.h \
	$(INC)/signal.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/resource.h \
	$(INC)/sac.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tp.h \
	tmstruct.h \
	ttymon.h

tmhandler.o: tmhandler.c \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/poll.h \
	$(INC)/termio.h $(INC)/sys/termio.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/wait.h \
	$(INC)/priv.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/termios.h \
	$(INC)/pfmt.h \
	ttymon.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tp.h \
	tmstruct.h \
	tmextern.h \
	$(INC)/sac.h

tmpmtab.o: tmpmtab.c \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/sys/types.h \
	$(INC)/ctype.h \
	$(INC)/string.h \
	$(INC)/pwd.h \
	$(INC)/grp.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/pfmt.h \
	$(INC)/priv.h \
	ttymon.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tp.h \
	tmstruct.h \
	tmextern.h

tmttydefs.o: tmttydefs.c \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/string.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/termio.h $(INC)/sys/termio.h \
	$(INC)/sys/stermio.h \
	$(INC)/sys/termiox.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/stream.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/pfmt.h \
	ttymon.h \
	$(INC)/sys/tp.h \
	tmstruct.h \
	stty.h

tmparse.o: tmparse.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h

tmsig.o: tmsig.c \
	$(INC)/stdio.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	tmextern.h

tmsac.o: tmsac.c \
	$(INC)/stdlib.h \
	$(INC)/stdio.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/fcntl.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/string.h \
	$(INC)/unistd.h \
	$(INC)/pfmt.h \
	ttymon.h \
	$(INC)/sac.h

tmchild.o: tmchild.c \
	$(INC)/stdlib.h \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/sys/types.h \
	$(INC)/termio.h $(INC)/sys/termio.h \
	$(INC)/string.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/poll.h \
	$(INC)/unistd.h \
	$(INC)/iaf.h \
	$(INC)/priv.h \
	$(INC)/pfmt.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/resource.h \
	$(INC)/sac.h \
	ttymon.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tp.h \
	tmstruct.h \
	tmextern.h \
	$(INC)/sys/utsname.h

tmautobaud.o: tmautobaud.c \
	$(INC)/stdio.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/fcntl.h \
	$(INC)/termio.h $(INC)/sys/termio.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stropts.h \
	$(INC)/priv.h \
	$(INC)/pfmt.h

tmterm.o: tmterm.c \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/stdio.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/termio.h $(INC)/sys/termio.h \
	$(INC)/sys/stermio.h \
	$(INC)/sys/termiox.h \
	$(INC)/string.h \
	$(INC)/ctype.h \
	$(INC)/priv.h \
	$(INC)/pfmt.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/signal.h \
	ttymon.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tp.h \
	tmstruct.h

tmvt.o: tmvt.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/string.h \
	$(INC)/signal.h \
	$(INC)/fcntl.h \
	$(INC)/sys/kd.h \
	$(INC)/sys/vt.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/stat.h \
	$(INC)/errno.h \
	$(INC)/pfmt.h \
	$(INC)/sys/sysmsg.h

tmutmp.o: tmutmp.c \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/sys/types.h \
	$(INC)/string.h \
	$(INC)/memory.h \
	$(INC)/utmp.h \
	$(INC)/priv.h \
	$(INC)/pfmt.h \
	$(INC)/sac.h

tmpeek.o: tmpeek.c \
	$(INC)/stdio.h \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/ctype.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/termio.h \
	$(INC)/poll.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/pfmt.h \
	$(INC)/priv.h \
	ttymon.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tp.h \
	tmstruct.h \
	tmextern.h

tmlog.o: tmlog.c \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/string.h \
	$(INC)/sys/types.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/priv.h \
	$(INC)/pfmt.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tp.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/sysmsg.h \
	ttymon.h \
	tmstruct.h \
	tmextern.h

tmmach.o: tmmach.c \
	$(INC)/stdio.h \
	ttymon.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/sysmsg.h \
	$(INC)/sys/utsname.h

tmutil.o: tmutil.c \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/stdio.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/sys/types.h \
	$(INC)/ctype.h \
	$(INC)/string.h \
	$(INC)/sys/stat.h \
	$(INC)/fcntl.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/sad.h \
	$(INC)/mac.h \
	$(INC)/priv.h \
	$(INC)/pfmt.h \
	ttymon.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tp.h \
	tmstruct.h

tmexpress.o: tmexpress.c \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/unistd.h \
	$(INC)/fcntl.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/ctype.h \
	$(INC)/string.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/sys/stat.h \
	$(INC)/utmp.h \
	$(INC)/stropts.h \
	$(INC)/poll.h \
	$(INC)/pfmt.h \
	ttymon.h \
	tmextern.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tp.h \
	tmstruct.h \
	$(INC)/mac.h \
	$(INC)/priv.h

ttyadm.o: ttyadm.c \
	$(INC)/stdio.h \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/sys/types.h \
	$(INC)/ctype.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tp.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/priv.h \
	tmstruct.h \
	ttymon.h

admutil.o: admutil.c \
	$(INC)/stdio.h \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/sys/types.h \
	$(INC)/ctype.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tp.h \
	$(INC)/pfmt.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	tmstruct.h \
	ttymon.h

sttydefs.o: sttydefs.c \
	$(INC)/stdio.h \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/sys/types.h \
	$(INC)/ctype.h \
	$(INC)/string.h \
	$(INC)/termio.h $(INC)/sys/termio.h \
	$(INC)/sys/stat.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tp.h \
	$(INC)/sys/mac.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	tmstruct.h \
	ttymon.h

install: all
	 $(INS) -o -f $(USRLIB)/saf -m 0544 -u $(OWN) -g $(GRP) ttymon
	-mv $(ETC)/getty $(USRSBIN)/OLDgetty 
	-$(SYMLINK) /usr/lib/saf/ttymon $(ETC)/getty
	 $(INS) -f $(USRSBIN) -m 0755 -u $(OWN) -g $(GRP) sttydefs
	 $(INS) -f $(USRSBIN) -m 0755 -u $(OWN) -g $(GRP) ttyadm
	@if [ -s defsak ] ; \
	then \
	 	echo "\n\tmake -f defsak.mk install" ; \
	 	$(MAKE) -f defsak.mk $(MAKEARGS) install ; \
	fi
	$(MAKE) -f stty.mk $(MAKEARGS) install
	@echo "========== $(INS) done!"

clean:
	-rm -f *.o
	
clobber: clean
	-rm -f $(PRODUCTS)
