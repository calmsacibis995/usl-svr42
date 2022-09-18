#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)librpcsvc:common/lib/librpcsvc/librpcsvc.mk	1.7.10.2"
#ident "$Header: librpcsvc.mk 1.4 91/06/27 $"

#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#	PROPRIETARY NOTICE (Combined)
#
# This source code is unpublished proprietary information
# constituting, or derived under license from AT&T's UNIX(r) System V.
# In addition, portions of such source code were derived from Berkeley
# 4.3 BSD under license from the Regents of the University of
# California.
#
#
#
#	Copyright Notice 
#
# Notice of copyright on this source code product does not indicate 
#  publication.
#
#	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
#	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
#	(c) 1990,1991  UNIX System Laboratories, Inc.
#          All rights reserved.
# 
#

include $(LIBRULES)

HDRS = rusers.h rwall.h spray.h
DESTINCLUDE = $(INC)/rpcsvc
LIBNAME = librpcsvc.a
ARFLAGS=cr

OBJS= rusersxdr.o rwallxdr.o sprayxdr.o \
      klm_prot.o nlm_prot.o sm_inter.o
SRCS = $(OBJS:%.o=%.c)

all: $(LIBNAME)

$(LIBNAME): $(OBJS)
	$(AR) $(ARFLAGS) $(LIBNAME) `$(LORDER) $(OBJS) | $(TSORT) `

install: $(LIBNAME)
	$(INS) -f $(USRLIB) $(LIBNAME)

installhdrs: $(HDRS)
	$(INS) -f  $(DESTINCLUDE) $(HDRS)

lintit:
	$(LINT) $(LINTFLAGS) $(SRCS)  

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(LIBNAME)
