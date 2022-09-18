#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)shl:shl.mk	1.5.9.2"
#ident "$Header: shl.mk 1.2 91/06/04 $"

include $(CMDRULES)


INSDIR=	$(USRBIN)
LDLIBS=	-ll

OBJECTS= defs.o yacc.o newmain.o mpx.o layer.o misc.o stat.o signal.o \
	 system.o aux.o
SOURCES= $(OBJECTS:.o=.c)

all:		$(OBJECTS)
		$(CC) -o shl $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

yacc.o	:	yacc.c lex.c

yacc.c	:	yacc.y

lex.c	:	lex.l
		$(LEX) lex.l
# fixing ECHO redefined - termios.h defines ECHO
		echo "# undef ECHO" >lex.c
		cat lex.yy.c >>lex.c
		rm lex.yy.c


aux.o defs.o newmain.o mpx.o layer.o misc.o stat.o signal.o system.o	: defs.h


install:	all
		$(INS) -o -f $(INSDIR) -m 4755 -u root -g bin shl

lintit:
		$(LINT) $(LINTFLAGS) $(SOURCES)

clobber:	clean
		rm -f shl
		rm -f $(INSDIR)/OLDshl

clean:		
		rm -f lex.c yacc.c *.o

aux.o: aux.c \
	defs.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stream.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/strtty.h \
	$(INC)/sys/nsxt.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/stdio.h \
	$(INC)/pfmt.h \
	$(INC)/string.h
defs.o: defs.c \
	defs.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stream.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/strtty.h \
	$(INC)/sys/nsxt.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/stdio.h \
	$(INC)/pfmt.h \
	$(INC)/string.h
layer.o: layer.c \
	defs.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stream.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/strtty.h \
	$(INC)/sys/nsxt.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/stdio.h \
	$(INC)/pfmt.h \
	$(INC)/string.h
misc.o: misc.c \
	defs.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stream.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/strtty.h \
	$(INC)/sys/nsxt.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/stdio.h \
	$(INC)/pfmt.h \
	$(INC)/string.h \
	$(INC)/unistd.h $(INC)/sys/unistd.h \
	$(INC)/utmp.h
mpx.o: mpx.c \
	defs.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stream.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/strtty.h \
	$(INC)/sys/nsxt.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/stdio.h \
	$(INC)/pfmt.h \
	$(INC)/string.h \
	$(INC)/unistd.h \
	$(INC)/sys/stropts.h
newmain.o: newmain.c \
	defs.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stream.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/strtty.h \
	$(INC)/sys/nsxt.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/stdio.h \
	$(INC)/pfmt.h \
	$(INC)/string.h \
	$(INC)/utmp.h \
	$(INC)/unistd.h $(INC)/sys/unistd.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/stropts.h \
	$(INC)/locale.h
signal.o: signal.c \
	defs.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stream.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/strtty.h \
	$(INC)/sys/nsxt.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/stdio.h \
	$(INC)/pfmt.h \
	$(INC)/string.h \
	$(INC)/unistd.h $(INC)/sys/unistd.h
stat.o: stat.c \
	defs.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stream.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/strtty.h \
	$(INC)/sys/nsxt.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/stdio.h \
	$(INC)/pfmt.h \
	$(INC)/string.h
system.o: system.c \
	defs.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stream.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/strtty.h \
	$(INC)/sys/nsxt.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/stdio.h \
	$(INC)/pfmt.h \
	$(INC)/string.h \
	$(INC)/unistd.h $(INC)/sys/unistd.h

