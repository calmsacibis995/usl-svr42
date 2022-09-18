#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/lp/lpq/Makefile	1.3"
#ident	"$Header: $"

include $(CMDRULES)


#TOP	=	../../..

include ../common.mk


TAGS	=	ctags

USRUCB	=	$(ROOT)/$(MACH)/usr/ucb

LOCALDEF = $(DEBUG) -DTERMCAP

LOCALINC = -I$(LPINC)

SRCS	= \
		lpq.c

OBJS	=	$(SRCS:.c=.o)


CMD	=	lpq


all:		$(CMD)

install:	all
	$(INS) -m $(SMODES) -u $(OWNER) -g $(GROUP) -f $(USRUCB) $(CMD)

clean:
	$(RM) $(OBJS)

clobber:	clean
	$(RM) $(CMD)

strip:
	$(STRIP) $(CMD)

lpq:		$(OBJS) \
			$(LIBBSD) \
			$(LIBREQ) \
			$(LIBMSG) \
			$(LIBOAM) \
			$(LIBPRT) \
			$(LIBSYS) \
			$(LIBLP)
	$(CC) -o lpq $(OBJS) $(LDFLAGS) $(PERFLIBS) \
			$(LIBBSD) \
			$(LIBREQ) \
			$(LIBMSG) \
			$(LIBPRT) \
			$(LIBSYS) \
			$(LIBLP) \
			$(LIBOAM) \
				-lgen -lcurses

stamp:

lint:
	$(LINT) $(CFLAGS) $(SRCS) \
			$(LINTREQ) \
			$(LINTMSG) \
			$(LINTOAM) \
			$(LINTPRT) \
			$(LINTSYS) \
			$(LINTLP)  \
			$(LINTOAM)

ctags:
	$(TAGS) $(SRCS)
