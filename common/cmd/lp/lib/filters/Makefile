#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)lp:lib/filters/Makefile	1.12.2.2"
#ident "$Header: Makefile 1.2 91/04/12 $"
#
# Makefile for filtermgmt(lp) routines
#

include $(CMDRULES)

TOP	=	../..

include ../../common.mk

LOCALINC= -I$(LPINC)

SRCS	= \
		conv.c \
		delfilter.c \
		dumpfilters.c \
		freefilter.c \
		getfilter.c \
		filtertable.c \
		insfilter.c \
		loadfilters.c \
		putfilter.c \
		regex.c \
		search.c \
		trash.c

OBJS	=	$(SRCS:.c=.o)

LINTTXT =	$(SRCS)

LINTS	= \
		$(LINTLP)

LIBNAME	=	lpflt

include ../libcommon.mk
