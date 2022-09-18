#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:net/rpc/rpc.mk	1.7"
#ident 	"$Header: $"

#
#	rpc.mk 1.4 89/01/03 SMI
#
#  		PROPRIETARY NOTICE (Combined)
#  
#  This source code is unpublished proprietary information
#  constituting, or derived under license from AT&T's Unix(r) System V.
#  In addition, portions of such source code were derived from Berkeley
#  4.3 BSD under license from the Regents of the University of
#  California.
#  
#  
#  
#  		Copyright Notice 
#  
#  Notice of copyright on this source code product does not indicate 
#  publication.
#  
#  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
#  	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#  	          All rights reserved.
#
#	Kernel RPC
#
include $(UTSRULES)

KBASE    = ../..
LOCALDEF = -DSYSV
INSPERM  = -u $(OWN) -g $(GRP)
KRPCOBJ = clnt_clts.o clnt_gen.o svc_gen.o svc_clts.o \
	  xdr_mblk.o xdr_mem.o svc.o  auth_kern.o rpc_prot.o \
	  rpc_calmsg.o xdr.o svc_auth.o authu_prot.o \
	  svcauthdes.o svc_authu.o xdr_array.o key_call.o \
	  key_prot.o clnt_perr.o svcauthesv.o  auth_esv.o authesvprt.o \
	  auth_des.o authdesprt.o authdesubr.o rpc_subr.o token.o

CFILES = $(KRPCOBJ:.o=.c)


KRPC = $(CONF)/pack.d/krpc/Driver.o


all:	ID $(KRPC)

ID:
	cd krpc.cf; $(IDINSTALL) -R$(CONF) -M krpc

$(KRPC):	krpc.o 
	$(LD) -r -o $@ krpc.o
lint:
	lint $(CFLAGS) -Dlint *.c

krpc.o:	$(KRPCOBJ)
	$(LD) -r -o krpc.o $(KRPCOBJ)

clean:
	-rm -f *.o

clobber:	clean
	$(IDINSTALL) -e -R$(CONF) -d krpc

headinstall: \
	$(KBASE)/net/rpc/auth.h \
	$(KBASE)/net/rpc/auth_esv.h \
	$(KBASE)/net/rpc/auth_des.h \
	$(KBASE)/net/rpc/auth_sys.h \
	$(KBASE)/net/rpc/auth_unix.h \
	$(KBASE)/net/rpc/clnt.h \
	$(KBASE)/net/rpc/clnt_soc.h \
	$(KBASE)/net/rpc/des_crypt.h \
	$(KBASE)/net/rpc/key_prot.h \
	$(KBASE)/net/rpc/nettype.h \
	$(KBASE)/net/rpc/pmap_clnt.h \
	$(KBASE)/net/rpc/pmap_prot.h \
	$(KBASE)/net/rpc/pmap_rmt.h \
	$(KBASE)/net/rpc/raw.h \
	$(KBASE)/net/rpc/rpc.h \
	$(KBASE)/net/rpc/rpc_com.h \
	$(KBASE)/net/rpc/rpc_msg.h \
	$(KBASE)/net/rpc/rpcb_clnt.h \
	$(KBASE)/net/rpc/rpcb_prot.h \
	$(KBASE)/net/rpc/rpcent.h \
	$(KBASE)/net/rpc/svc.h \
	$(KBASE)/net/rpc/svc_auth.h \
	$(KBASE)/net/rpc/svc_soc.h \
	$(KBASE)/net/rpc/types.h \
	$(KBASE)/net/rpc/token.h \
	$(KBASE)/net/rpc/xdr.h \
	$(FRC)
	$(INS) -f $(INC)/rpc -m 644 $(INSPERM)  $(KBASE)/net/rpc/auth.h
	$(INS) -f $(INC)/rpc -m 644 $(INSPERM)  $(KBASE)/net/rpc/auth_esv.h
	$(INS) -f $(INC)/rpc -m 644 $(INSPERM)  $(KBASE)/net/rpc/auth_des.h
	$(INS) -f $(INC)/rpc -m 644 $(INSPERM)  $(KBASE)/net/rpc/auth_sys.h
	$(INS) -f $(INC)/rpc -m 644 $(INSPERM)  $(KBASE)/net/rpc/auth_unix.h
	$(INS) -f $(INC)/rpc -m 644 $(INSPERM)  $(KBASE)/net/rpc/clnt.h
	$(INS) -f $(INC)/rpc -m 644 $(INSPERM)  $(KBASE)/net/rpc/clnt_soc.h
	$(INS) -f $(INC)/rpc -m 644 $(INSPERM)  $(KBASE)/net/rpc/des_crypt.h
	$(INS) -f $(INC)/rpc -m 644 $(INSPERM)  $(KBASE)/net/rpc/key_prot.h
	$(INS) -f $(INC)/rpc -m 644 $(INSPERM)  $(KBASE)/net/rpc/nettype.h
	$(INS) -f $(INC)/rpc -m 644 $(INSPERM)  $(KBASE)/net/rpc/pmap_clnt.h
	$(INS) -f $(INC)/rpc -m 644 $(INSPERM)  $(KBASE)/net/rpc/pmap_prot.h
	$(INS) -f $(INC)/rpc -m 644 $(INSPERM)  $(KBASE)/net/rpc/pmap_rmt.h
	$(INS) -f $(INC)/rpc -m 644 $(INSPERM)  $(KBASE)/net/rpc/raw.h
	$(INS) -f $(INC)/rpc -m 644 $(INSPERM)  $(KBASE)/net/rpc/rpc.h
	$(INS) -f $(INC)/rpc -m 644 $(INSPERM)  $(KBASE)/net/rpc/rpc_com.h
	$(INS) -f $(INC)/rpc -m 644 $(INSPERM)  $(KBASE)/net/rpc/rpc_msg.h
	$(INS) -f $(INC)/rpc -m 644 $(INSPERM)  $(KBASE)/net/rpc/rpcb_clnt.h
	$(INS) -f $(INC)/rpc -m 644 $(INSPERM)  $(KBASE)/net/rpc/rpcb_prot.h
	$(INS) -f $(INC)/rpc -m 644 $(INSPERM)  $(KBASE)/net/rpc/rpcent.h
	$(INS) -f $(INC)/rpc -m 644 $(INSPERM)  $(KBASE)/net/rpc/svc.h
	$(INS) -f $(INC)/rpc -m 644 $(INSPERM)  $(KBASE)/net/rpc/svc_auth.h
	$(INS) -f $(INC)/rpc -m 644 $(INSPERM)  $(KBASE)/net/rpc/svc_soc.h
	$(INS) -f $(INC)/rpc -m 644 $(INSPERM)  $(KBASE)/net/rpc/types.h
	$(INS) -f $(INC)/rpc -m 644 $(INSPERM)  $(KBASE)/net/rpc/token.h
	$(INS) -f $(INC)/rpc -m 644 $(INSPERM)  $(KBASE)/net/rpc/xdr.h


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

