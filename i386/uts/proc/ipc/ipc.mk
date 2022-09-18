#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:proc/ipc/ipc.mk	1.5"
#ident	"$Header: $"

include $(UTSRULES)

KBASE = ../..
IPC = $(CONF)/pack.d/ipc/Driver.o
MSG = $(CONF)/pack.d/msg/Driver.o
SEM = $(CONF)/pack.d/sem/Driver.o
SHM = $(CONF)/pack.d/shm/Driver.o

FILE = ipc.o msg.o sem.o shm.o

CFILES = $(FILE:.o=.c)


all:	ID $(IPC) $(MSG) $(SEM) $(SHM)


ID:
	cd ipc.cf; $(IDINSTALL) -R$(CONF) -M ipc
	cd msg.cf; $(IDINSTALL) -R$(CONF) -M msg
	cd sem.cf; $(IDINSTALL) -R$(CONF) -M sem
	cd shm.cf; $(IDINSTALL) -R$(CONF) -M shm

$(IPC): ipc.o
	$(LD) -r -o $(IPC) ipc.o

$(MSG): msg.o
	$(LD) -r -o $(MSG) msg.o

$(SEM): sem.o
	$(LD) -r -o $(SEM) sem.o

$(SHM): shm.o
	$(LD) -r -o $(SHM) shm.o

clean:
	-rm -f *.o

clobber:	clean
	-rm -f $(IPC) $(MSG) $(SEM) $(SHM)
	-$(IDINSTALL) -e -R$(CONF) -d ipc
	-$(IDINSTALL) -e -R$(CONF) -d msg
	-$(IDINSTALL) -e -R$(CONF) -d sem
	-$(IDINSTALL) -e -R$(CONF) -d shm

headinstall: \
	$(KBASE)/proc/ipc/ipc.h \
	$(KBASE)/proc/ipc/ipcsec.h \
	$(KBASE)/proc/ipc/msg.h \
	$(KBASE)/proc/ipc/sem.h \
	$(KBASE)/proc/ipc/shm.h \
	$(FRC)
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/ipc/ipc.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/ipc/ipcsec.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/ipc/msg.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/ipc/sem.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/proc/ipc/shm.h


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

