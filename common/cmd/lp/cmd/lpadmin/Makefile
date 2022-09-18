#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.



#ident	"@(#)lp:cmd/lpadmin/Makefile	1.21.5.2"
#ident "$Header: Makefile 1.8 91/07/30 $"
#
# Makefile for the lpadmin command
#

include $(CMDRULES)

TOP	=	../..

include ../../common.mk

#LOCALDEF= -DDEBUG $(NETWORKING)
LOCALDEF= $(NETWORKING)
LOCALINC = -I. -I$(LPINC)

HDRS	= \
		lpadmin.h

SRCS	= \
		chkopts.c \
		default.c \
		do_align.c \
		do_fault.c \
		do_mount.c \
		do_printer.c \
		do_pwheel.c \
		done.c \
		fromclass.c \
		ismodel.c \
		lpadmin.c \
		options.c \
		output.c \
		rmdest.c \
		send_message.c \
		signals.c \
		startup.c \
		usage.c

OBJS	=	$(SRCS:.c=.o)

LIBS	= \
		$(LIBACC) \
		$(LIBCLS) \
		$(LIBMSG) \
		$(LIBPRT) \
		$(LIBFRM) \
		$(LIBREQ) \
		$(LIBOAM) \
		$(LIBSYS) \
		$(LIBLP)

SYSLIBS	=	-lcurses

LINTS	= \
		$(LINTACC) \
		$(LINTCLS) \
		$(LINTMSG) \
		$(LINTPRT) \
		$(LINTFRM) \
		$(LINTREQ) \
		$(LINTOAM) \
		$(LINTSYS) \
		$(LINTLP)


CMDS	=	lpadmin  lpadmin.net

.MUTEX: lpadmin lpadmin.net

all:		$(CMDS)

install:
	$(INS) -m $(EMODES) -u $(OWNER) -g $(GROUP) -f $(USRSBIN) lpadmin
	$(INS) -m $(EMODES) -u $(OWNER) -g $(GROUP) -f $(USRSBIN) lpadmin.net
	rm -f $(USRLIB)/lpadmin $(USRLIB)/lpadmin.net
	$(SYMLINK) /usr/sbin/lpadmin $(USRLIB)/lpadmin
	$(SYMLINK) /usr/sbin/lpadmin.net $(USRLIB)/lpadmin.net

clean:
	$(RM) $(OBJS)

clobber:	clean
	$(RM) $(CMDS)

strip:
	$(STRIP) $(CMDS)

lpadmin:
	$(MAKE) DEBUG="$(DEBUG)" $(MAKEARGS) _lpadmin

_lpadmin:	$(OBJS) $(LIBS)
	$(CC) -o lpadmin $(OBJS) $(LDFLAGS) $(LIBS) $(SYSLIBS) $(SHLIBS)

lpadmin.net:	clean
	$(MAKE) DEBUG="$(DEBUG)" NETWORKING=-DNETWORKING $(MAKEARGS) _lpadmin.net

_lpadmin.net:	$(OBJS) $(LIBS)
	$(CC) -o lpadmin.net $(OBJS) $(LDFLAGS) $(LIBS) $(SYSLIBS) $(SHLIBS)

$(OBJS):	$(HDRS)

lintit:
	$(LINT) $(DEFLIST) $(SRCS) $(LINTS)
