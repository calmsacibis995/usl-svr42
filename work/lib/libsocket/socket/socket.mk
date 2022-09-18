#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)libsocket:common/lib/libsocket/socket/socket.mk	1.4.9.4"
#ident "$Header: socket.mk 1.5 91/06/27 $"

#
# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# 		PROPRIETARY NOTICE (Combined)
# 
# This source code is unpublished proprietary information
# constituting, or derived under license from AT&T's UNIX(r) System V.
# In addition, portions of such source code were derived from Berkeley
# 4.3 BSD under license from the Regents of the University of
# California.
# 
# 
# 
# 		Copyright Notice 
# 
# Notice of copyright on this source code product does not indicate 
# publication.
# 
# 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
# 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T
#	(c) 1990,1991  UNIX System Laboratories, Inc.
# 	          All rights reserved.
#  

include $(LIBRULES)

LOCALDEF=-DSYSV $(PICFLAG)

OBJS=		accept.o bind.o callselect.o connect.o socket.o socketpair.o \
		shutdown.o getsockopt.o setsockopt.o listen.o \
		receive.o send.o _conn_util.o _utility.o \
		getsocknm.o getpeernm.o setsocknm.o setpeernm.o s_ioctl.o

all:		$(OBJS)
		cp $(OBJS) ../


install:	all

clean:
		-rm -f *.o

clobber:	clean

#
# To do -- header dependencies
#
