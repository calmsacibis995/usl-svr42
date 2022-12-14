#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)libnsl:common/lib/libnsl/des/des.mk	1.8.9.5"
#ident "$Header: des.mk 1.4 91/06/27 $"

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
#	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#          All rights reserved.
# 
#

include $(LIBRULES)

LOCALDEF = $(PICFLAG)
OBJS= des_crypt.o des_soft.o
INTLOBJS= intl_crypt.o intl_soft.o
SRCS = $(OBJS:.o=.c)
INTLSRCS = $(INTLOBJS:.o=.c)

all:
	if [ -s des_crypt.c -a	-s des_soft.c ] ;\
	then \
		$(MAKE) -f des.mk usa ;\
	fi
	$(MAKE) -f des.mk intl

usa: $(OBJS)
	cp des_crypt.o ../des_crypt.o_d
	cp des_soft.o ../des_soft.o_d

intl: $(INTLOBJS)
	cp intl_crypt.o ../des_crypt.o_i
	cp intl_soft.o ../des_soft.o_i

des_crypt.o intl_crypt.o: $(INC)/sys/types.h \
	$(INC)/rpc/des_crypt.h \
	$(INC)/des/des.h
 
des_soft.o intl_soft.o: $(INC)/sys/types.h \
	$(INC)/des/softdes.h \
	$(INC)/des/desdata.h \
	$(INC)/des/des.h 
	$(CC) -S $(CFLAGS) $(DEFLIST) $*.c
	/bin/echo "/^[ 	]*\.data[ 	]*$$/s/data/text/\nw"|ed - $*.s
	$(AS) $(ASFLAGS) -o $@ $*.s
	-rm $*.s

lintit:	
	$(LINT) $(LINTFLAGS) $(SRCS) $(INTLSRCS)

clean:
	rm -f $(OBJS) $(INTLOBJS)

clobber: clean

strip:	$(OBJS) $(INTLOBJS)
	$(STRIP) $(OBJS) $(INTLOBJS)

size:	$(OBJS) $(INTLOBJS)
	$(SIZE) $(OBJS) $(INTLOBJS)
