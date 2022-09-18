#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libc-port:sys/makefile	1.5.9.1"
#
# makefile for libc/port/sys
#
#

include $(LIBRULES)

.SUFFIXES: .p .P .A
PROF=
NONPROF=
PIC=
ABI=
ROOT=
INCLIBC=../../inc
DEFLIST=
SDEFLIST=
INCLIST=-I$(INCLIBC)

OBJECTS=\
execl.o       execle.o      execv.o	hrtsys.o   lockf.o    \
msgsys.o      nfssys.o      semsys.o    shmsys.o  

POBJECTS=$(OBJECTS:.o=.p)

PIC_OBJECTS=\
execl.P       execle.P      execv.P     hrtsys.P   lockf.P    \
msgsys.P      semsys.P      shmsys.P

ABI_OBJECTS=$(PIC_OBJECTS:.P=.A)

nonprof: $(OBJECTS)

prof: $(POBJECTS)

pic: $(PIC_OBJECTS)

abi: $(ABI_OBJECTS)

msgsys.o msgsys.p semsys.o semsys.p \
	shmsys.o shmsys.p: $(INC)/sys/ipc.h
msgsys.o msgsys.p: $(INC)/sys/msg.h
semsys.o semsys.p: $(INC)/sys/sem.h
shmsys.o shmsys.p: $(INC)/sys/shm.h
msgsys.o msgsys.p semsys.o semsys.p \
	shmsys.o shmsys.p: $(INC)/sys/types.h
lockf.o lockf.p: $(INC)/errno.h 
lockf.o lockf.p: $(INC)/fcntl.h 
lockf.o lockf.p: $(INC)/unistd.h

.c.o .c.p .c.P .c.A:
	$(NONPROF)@echo $*.c:
	$(NONPROF)$(CC) $(DEFLIST) $(SDEFLIST) $(INCLIST) $(CFLAGS) -c $*.c
	$(PROF)@echo $*.c:
	$(PROF)$(CC) $(DEFLIST) $(INCLIST) $(CFLAGS) -c -p $*.c  && mv $(*F).o $*.p
	$(PIC)@echo $*.c:
	$(PIC)$(CC) $(DEFLIST) $(INCLIST) $(CFLAGS) -c -DDSHLIB -Kpic $*.c  && mv $(*F).o $*.P
	$(ABI)@echo $*.c:
	$(ABI)$(CC) $(DEFLIST) $(INCLIST) $(CFLAGS) -c -DABI -DDSHLIB -Kpic $*.c  && mv $(*F).o $*.A
