#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)lp:cmd/lpdata/Makefile	1.9.3.2"
#ident "$Header: Makefile 1.4 91/07/04 $"
#
# Makefile for the lpdata command
#


include $(CMDRULES)

TOP	=	../..

include ../../common.mk

LOCALINC= -I. -I$(LPINC)
#LOCALDEF=-DDEBUG

SRCS	= \
		lpdata.c

OBJS	=	$(SRCS:.c=.o)

LIBS	= \
		$(LIBMSG) \
		$(LIBFRM) \
		$(LIBREQ) \
		$(LIBPRT) \
		$(LIBCLS) \
		$(LIBACC) \
		$(LIBFLT) \
		$(LIBUSR) \
		$(LIBLP)  \
		$(LIBOAM)

LINTLB  =	../llib-llpcmd.ln $(TOP)/lib/llib-llplib.ln

SYSLIBS	=	-lgen

LINTS	= \
		$(LINTMSG) \
		$(LINTFRM) \
		$(LINTREQ) \
		$(LINTPRT) \
		$(LINTCLS) \
		$(LINTACC) \
		$(LINTFLT) \
		$(LINTUSR) \
		$(LINTLP)  \
		$(LINTOAM)


CMD	=	lpdata


all:		$(CMD)

install:
	$(INS) -m $(EMODES) -u $(OWNER) -g $(GROUP) -f $(USRLIBLP) $(CMD)

clean:
	$(RM) $(OBJS)

clobber:	clean
	$(RM) $(CMD)

strip:
	$(STRIP) $(CMD)

$(CMD):		$(OBJS) $(LIBS)
	$(CC) -o $(CMD) $(OBJS) $(LDFLAGS) $(LIBS) $(SYSLIBS) $(SHLIBS)

lintit:
	$(LINT) $(DEFLIST) $(SRCS) $(LINTLB)
 
lintlib:
	$(LINT) -abhmuvxy -o lpdat $(DEFLIST) $(SRCS)
