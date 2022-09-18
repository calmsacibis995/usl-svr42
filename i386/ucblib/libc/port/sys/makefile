#ident	"@(#)ucb:i386/ucblib/libc/port/sys/makefile	1.5"
#ident	"$Header: $"
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#
# makefile for ucblibc/port/gen
#
#

include $(LIBRULES)

PROF=
NONPROF=
INC1=$(ROOT)/$(MACH)/usr/ucbinclude
INCSYS=$(INC)/sys
INCSYS1=$(INC1)/sys
SDEFLIST=

OBJECTS =  gettimeofday.o signal.o \
	wait3.o getrusage.o reboot.o utimes.o \
	gethostname.o killpg.o \
	gethostid.o getdtblsize.o setreid.o \
	getpagesize.o mctl.o


SOURCES =  gettimeofday.c signal.c \
	wait3.c getrusage.c reboot.c utimes.c \
	gethostname.c killpg.c \
	gethostid.c getdtblsize.c setreid.c \
	getpagesize.c mctl.c

ALL:		 $(OBJECTS)

all:	ALL

setreid.o:	$(INCSYS)/types.h

getrusage.o:	 $(INCSYS)/errno.h \
		 $(INCSYS)/param.h \
		 $(INCSYS)/select.h \
		 $(INCSYS)/time.h \
		 $(INCSYS)/times.h \
		 $(INCSYS)/types.h \
		 $(INC)/time.h \
		 $(INCSYS1)/resource.h 
		$(CC) $(CFLAGS) -I$(INC1) -I$(INC) -c getrusage.c

gettimeofday.o:	 $(INC)/errno.h \
		 $(INC)/stdio.h \
		 $(INCSYS)/errno.h \
		 $(INCSYS)/hrtcntl.h \
		 $(INCSYS)/select.h \
		 $(INCSYS)/time.h \
		 $(INCSYS)/types.h \
		 $(INC)/time.h 

reboot.o:	 $(INCSYS)/errno.h 


signal.o:	 $(INC)/errno.h \
		 $(INC1)/signal.h	\
		 $(INCSYS)/errno.h \
		 $(INCSYS)/select.h \
		 $(INCSYS)/siginfo.h \
		 $(INCSYS)/signal.h \
		 $(INCSYS)/types.h \
		 $(INCSYS)/ucontext.h 
		$(CC) $(CFLAGS) -I$(INC1) -I$(INC) -c signal.c

utimes.o:	 $(INCSYS)/select.h \
		 $(INCSYS)/time.h \
		 $(INCSYS)/utime.h \
		 $(INC)/time.h 

wait3.o:	 $(INC)/errno.h \
		 $(INCSYS)/errno.h \
		 $(INCSYS)/param.h \
		 $(INCSYS)/procset.h \
		 $(INCSYS1)/resource.h \
		 $(INCSYS)/select.h \
		 $(INCSYS)/siginfo.h \
		 $(INCSYS)/time.h \
		 $(INCSYS)/times.h \
		 $(INCSYS)/types.h \
		 $(INCSYS)/wait.h \
		 $(INC)/time.h 
		$(CC) $(CFLAGS) -I$(INC1) -I$(INC) -c wait3.c

gethostname.o:	$(INCSYS)/types.h \
		$(INCSYS)/systeminfo.h 

killpg.o:	$(INCSYS)/types.h 

getdtblsize.o:	 $(INCSYS)/resource.h

gethostid.o:	$(INCSYS)/types.h \
		$(INCSYS)/systeminfo.h 

getpagesize.o:	$(INC)/unistd.h

mctl.o:		$(INCSYS)/types.h \
		$(INCSYS)/mman.h

GLOBALINCS = $(INC)/errno.h \
	$(INC1)/signal.h \
	$(INC)/stdio.h \
	$(INCSYS)/errno.h \
	$(INCSYS)/hrtcntl.h \
	$(INCSYS)/param.h \
	$(INCSYS)/procset.h \
	$(INCSYS1)/resource.h \
	$(INCSYS)/select.h \
	$(INCSYS)/siginfo.h \
	$(INCSYS)/signal.h \
	$(INCSYS)/time.h \
	$(INCSYS)/times.h \
	$(INCSYS)/types.h \
	$(INCSYS)/mman.h \
	$(INCSYS)/ucontext.h \
	$(INCSYS)/wait.h \
	$(INC)/time.h \
	$(INC)/ctype.h \
	$(INCSYS)/utime.h \
	$(INCSYS)/systeminfo \
	$(INC)/unistd.h

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS)
