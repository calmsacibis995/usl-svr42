#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)lp:lib/bsd/Makefile	1.8.2.2"
#ident "$Header: Makefile 1.3 91/04/16 $"
#
# Makefile for bsd routines
#

include $(CMDRULES)

TOP	=	../..

include ../../common.mk

LOCALINC= -I$(LPINC)
#LOCALDEF= -DDEBUG

SRCS	= \
		displayq.c \
		escape.c \
		fatalmsg.c \
		findfld.c \
		gethostnm.c \
		global.c \
		log.c \
		parse.c \
		psfile.c \
		rmjob.c \
		rsendjob.c \
		sndrcv.c

LINTTXT =	$(SRCS)

OBJS	=	$(SRCS:.c=.o)

LIBNAME	=	lpbsd

include ../libcommon.mk
