#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)lp:lib/forms/Makefile	1.13.3.2"
#ident "$Header: Makefile 1.4 91/07/04 $"
#
# Makefile for formsmgmt(lp) routines
#

include $(CMDRULES)

TOP	=	../..

include ../../common.mk

LOCALINC= -I$(LPINC)

SRCS	= \
		delform.c \
		f_head.c \
		freeform.c \
		getform.c \
		putform.c \
		rdform.c \
		wrform.c

LINTTXT =	$(SRCS)

OBJS	=	$(SRCS:.c=.o)

LINTS	= \
		$(LINTLP)

LIBNAME	=	lpfrm

include ../libcommon.mk
