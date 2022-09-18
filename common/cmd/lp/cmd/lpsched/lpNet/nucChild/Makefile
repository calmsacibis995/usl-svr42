#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)lp:cmd/lpsched/lpNet/nucChild/Makefile	1.1"

include $(CMDRULES)

LPROOT	=	../../../..
LPNETROOT=	..
LPLIB	=	$(LPROOT)/lib
LPINC	=	$(LPROOT)/include
INCSYS	=	$(INC)/sys
INCDIR	=	../include

ENV	=

LOCALINC= -I$(INCDIR) -I$(LPINC)
#LOCALDEF= -DDEBUG

LFLAGS	=	-I$(INCDIR) -I$(LPINC) -I$(INC)

LIB	=	nucChild.a
TARGETS	=	$(LIB)

SRCS	=	nucChild.c
OBJS	=	$(SRCS:.c=.o)

LINTLB  =	../../../llib-llpcmd.ln ../../../../lib/llib-llplib.ln

all:		$(LIB)
		@for t in $(TARGETS);			\
		do					\
			echo "$${t} is-up-to-date.";	\
		done

install:	all

clean:
		rm -f *.o

clobber:	clean
		rm -f $(LIB)

lintit:
		$(LINT) $(LFLAGS) $(SRCS) $(LINTLB)

lintlib:
		$(LINT) -abhmuvxy -o lpnucc $(LFLAGS) $(SRCS)

product:
		@for t in $(TARGETS);	\
		do			\
			echo $${t};	\
		done


$(LIB):		$(OBJS)
		$(AR) $(ARFLAGS) $(LIB) *.o

nucChild.o:	$(INCDIR)/lpNet.h


.PRECIOUS:	$(TARGETS)

