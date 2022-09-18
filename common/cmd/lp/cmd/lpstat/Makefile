#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)lp:cmd/lpstat/Makefile	1.20.4.2"
#ident "$Header: Makefile 1.4 91/07/04 $"
#
# Makefile for the lpstat command
#

include $(CMDRULES)

TOP	=	../..

include ../../common.mk

LOCALINC= -I. -I$(LPINC)

#LOCALDEF= -DDEBUG

HDRS	= \
		lpstat.h

SRCS	= \
		accept.c \
		add_mounted.c \
		charset.c \
		class.c \
		device.c \
		done.c \
		form.c \
		lpstat.c \
		output.c \
		parse.c \
		printer.c \
		request.c \
		send_message.c

OBJS	=	$(SRCS:.c=.o)

LINTLB  =	../llib-llpcmd.ln ../../lib/llib-llplib.ln

LIBS	= \
		$(LIBACC) \
		$(LIBCLS) \
		$(LIBMSG) \
		$(LIBPRT) \
		$(LIBFRM) \
		$(LIBLP)  \
		$(LIBOAM)

SYSLIBS =	-lcurses

CMD	=	lpstat


all:		$(CMD)

install:	all
	$(INS) -m $(EMODES) -u $(OWNER) -g $(GROUP) -f $(USRBIN) $(CMD)

clean:
	$(RM) $(OBJS)

clobber:	clean
	$(RM) $(CMD)

strip:
	$(STRIP) $(CMD)

$(CMD):		$(OBJS) $(LIBS)
	$(CC) -o $(CMD) $(OBJS) $(LDFLAGS) $(LIBS) $(SYSLIBS) $(SHLIBS)

$(OBJS):	$(HDRS)

lintit:
	$(LINT) $(DEFLIST) $(SRCS) $(LINTLB)

lintlib:
	$(LINT) -abhmuvxy -o lpstt $(DEFLIST) $(SRCS)

