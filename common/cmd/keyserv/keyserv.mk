#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)keyserv:keyserv.mk	1.27.10.2"
#ident  "$Header: keyserv.mk 1.3 91/06/27 $"

include $(CMDRULES)

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
#       (c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc                     
#       (c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.                      
#       (c) 1990,1991  UNIX System Laboratories, Inc.
#          All rights reserved.
#

LOCALDEF = -DYP
DESTBIN = $(USRBIN)
LDLIBS = -lrpcsvc -lnsl
SBINS = keyserv newkey  
BINS = keylogout keylogin domainname chkey 
KEYSERV_OBJS = setkey.o detach.o key_generic.o
LIBMPOBJS= pow.o gcd.o msqrt.o mdiv.o mout.o mult.o madd.o util.o
CHANGE_OBJS = generic.o update.o
OBJECTS = $(KEYSERV_OBJS) $(CHANGE_OBJS) $(SBINS:=.o) $(BINS:=.o)
SOURCES = $(OBJECTS:.o=.c)


all: $(BINS) $(SBINS)

keyserv: $(KEYSERV_OBJS) $(LIBMPOBJS) keyserv.o
	$(CC) -o $@ $(KEYSERV_OBJS) $(LIBMPOBJS) keyserv.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

keylogout: keylogout.o 
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

keylogin: keylogin.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

chkey: $(CHANGE_OBJS) $(LIBMPOBJS) chkey.o
	$(CC) -o $@ $(CHANGE_OBJS) $(LIBMPOBJS) chkey.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

newkey:$(CHANGE_OBJS) $(LIBMPOBJS) newkey.o
	$(CC) -o $@ $(CHANGE_OBJS) $(LIBMPOBJS) newkey.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

generic:$(LIBMPOBJS) generic.o
	$(CC) -o $@ $(LIBMPOBJS) generic.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

update:$(LIBMPOBJS) update.o
	$(CC) -o $@ $(LIBMPOBJS) update.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

domainname: domainname.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

chkey.o: chkey.c \
	$(INC)/stdio.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpc/key_prot.h \
	$(INC)/rpcsvc/ypclnt.h \
	$(INC)/pwd.h \
	$(INC)/string.h

detach.o: detach.c \
	$(INC)/sys/termios.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h

domainname.o: domainname.c \
	$(INC)/stdio.h

gcd.o: gcd.c \
	mp.h

generic.o: generic.c \
	$(INC)/stdio.h \
	$(INC)/rpc/rpc.h \
	$(INC)/sys/file.h \
	mp.h \
	$(INC)/rpc/key_prot.h

init_tr.o: init_tr.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/unistd.h $(INC)/sys/unistd.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpc/rpcb_prot.h \
	$(INC)/netconfig.h \
	$(INC)/netdir.h \
	$(INC)/sys/wait.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/syslog.h

key_generic.o: key_generic.c \
	$(INC)/stdio.h \
	$(INC)/rpc/rpc.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/sys/syslog.h \
	$(INC)/rpc/nettype.h \
	$(INC)/netconfig.h \
	$(INC)/netdir.h

keylogin.o: keylogin.c \
	$(INC)/stdio.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpc/key_prot.h

keylogout.o: keylogout.c \
	$(INC)/stdio.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpc/key_prot.h

keyserv.o: keyserv.c \
	$(INC)/stdio.h \
	$(INC)/rpc/rpc.h \
	$(INC)/sys/param.h \
	$(INC)/sys/file.h \
	$(INC)/pwd.h \
	$(INC)/rpc/des_crypt.h \
	$(INC)/rpc/key_prot.h

madd.o: madd.c \
	mp.h

mdiv.o: mdiv.c \
	mp.h \
	$(INC)/stdio.h

mout.o: mout.c \
	$(INC)/stdio.h \
	mp.h

msqrt.o: msqrt.c \
	mp.h

mult.o: mult.c \
	mp.h

newkey.o: newkey.c \
	$(INC)/stdio.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpc/key_prot.h \
	$(INC)/rpcsvc/ypclnt.h \
	$(INC)/sys/wait.h \
	$(INC)/netdb.h \
	$(INC)/pwd.h \
	$(INC)/string.h \
	$(INC)/sys/resource.h \
	$(INC)/netconfig.h \
	$(INC)/netdir.h

pow.o: pow.c \
	mp.h

setkey.o: setkey.c \
	$(INC)/stdio.h \
	mp.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpc/key_prot.h \
	$(INC)/rpc/des_crypt.h \
	$(INC)/sys/errno.h

update.o: update.c \
	$(INC)/stdio.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpc/key_prot.h \
	$(INC)/rpcsvc/ypclnt.h \
	$(INC)/sys/wait.h \
	$(INC)/netdb.h \
	$(INC)/pwd.h \
	$(INC)/string.h \
	$(INC)/sys/resource.h

util.o: util.c \
	$(INC)/stdio.h \
	mp.h

install: $(BINS) $(SBINS)
	$(INS) -f $(USRSBIN) -m 0555 -u root -g sys keyserv
	$(INS) -f $(USRSBIN) -m 0555 -u root -g sys newkey
	$(INS) -f $(USRBIN) -m 0555 -u bin -g bin chkey
	$(INS) -f $(USRBIN) -m 0555 -u bin -g bin domainname
	$(INS) -f $(USRBIN) -m 0555 -u bin -g bin keylogin
	$(INS) -f $(USRBIN) -m 0555 -u bin -g bin keylogout

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)

clean:
	rm -f *.o

clobber: clean
	rm -f $(SBINS) $(BINS)
