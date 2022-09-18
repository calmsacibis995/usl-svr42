#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)lp:cmd/lpsched/lpNet/bsdChild/Makefile	1.8.3.2"
#ident "$Header: Makefile 1.4 91/07/04 $"

include $(CMDRULES)

LPROOT	=	../../../..
LPNETROOT=	..
LPLIB	=	$(LPROOT)/lib
LPINC	=	$(LPROOT)/include
INCSYS	=	$(INC)/sys
LPNETINC=	$(LPNETROOT)/include

ENV	=	

LOCALINC= -I$(LPNETINC) -I$(LPINC)
#LOCALDEF= -DDEBUG

LFLAGS	=	-I$(LPNETINC) -I$(LPINC) -I$(INC)

LIB	=	bsdChild.a
TARGETS	=	$(LIB)

SRCS	=	bsdChild.c	\
		cancel.c	\
		getstatus.c	\
		netutil.c	\
		printjob.c	\
		printreq.c	\
		recvjob.c

OBJS	=	$(SRCS:.c=.o)

LINTLB  =	../../../llib-llpcmd.ln ../../../../lib/llib-llplib.ln

$(LIB):		$(OBJS)
		$(AR) $(ARFLAGS) $(LIB) *.o

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
		$(LINT) -abhmuvxy -o lpbsd $(LFLAGS) $(SRCS)

product:
		@for t in $(TARGETS);	\
		do			\
			echo $${t};	\
		done

.PRECIOUS:	$(TARGETS)

