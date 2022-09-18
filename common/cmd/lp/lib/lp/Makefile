#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)lp:lib/lp/Makefile	1.25.2.2"
#ident "$Header: Makefile 1.2 91/04/12 $"

#
# Makefile for LP support routines
#

include $(CMDRULES)

TOP	=	../..

include ../../common.mk

LOCALINC= -I$(LPINC)

SRCS	= \
		Syscalls.c \
		addlist.c \
		addstring.c \
		appendlist.c \
		audit.c \
		alerts.c \
		charset.c \
		cs_strcmp.c \
		cs_strncmp.c \
		dellist.c \
		dashos.c \
		debug.c \
		dirs.c \
		duplist.c \
		files.c \
		freelist.c \
		getlist.c \
		getname.c \
		getpaths.c \
		getspooldir.c \
		isterminfo.c \
		joinlist.c \
		lenlist.c \
		lp_errno.c\
		makepath.c \
		makestr.c \
		mergelist.c \
		next.c \
		printlist.c \
		sdn.c \
		sprintlist.c \
		searchlist.c \
		sendmail.c \
		set_charset.c \
		set_pitch.c \
		set_size.c \
		sop.c \
		strip.c \
		syntax.c \
		tidbit.c \
		tinames.c \
		wherelist.c \
		which.c	\
		GetProcLevel.c \
		SetProcLevel.c \
		SetFileLevel.c \
		TruncLevel.c \
		WrapLevel.c

LINTTXT =	$(SRCS) ../oam/llib-llpoam.ln

OBJS	=	$(SRCS:.c=.o)

LINTS	=

LIBNAME	=	lp

include ../libcommon.mk
