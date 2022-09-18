#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)rtld:i386/makefile	1.9.2.2"

# makefile for i386 version of the run-time linker

include $(LIBRULES)

SGSBASE = ../../../../cmd/sgs
SGSINC=$(SGSBASE)/inc
MODE= -KPIC
COMDIR=../common
CPUDIR = ../$(CPU)
GLOBALINC =
LOCALDEF = -D$(CPU) -DWEITEK
LOCALINC = -I$(COMDIR) -I$(CPUDIR) -I$(SGSINC)/common -I$(SGSINC)/$(CPU) -I$(INC)
PROPTS= -dxerox -omode=l -omode=h
AWK=awk
ECHO=echo

HFILES= $(COMDIR)/rtld.h $(COMDIR)/dllib.h $(COMDIR)/externs.h \
	$(CPUDIR)/machdep.h $(COMDIR)/rtinc.h

SOURCES= $(COMDIR)/dlfcns.c $(COMDIR)/globals.c $(COMDIR)/map.c \
	 $(COMDIR)/rtfcns.c $(COMDIR)/rtld.c $(COMDIR)/paths.c  \
	 $(CPUDIR)/reloc.c $(CPUDIR)/rtsetup.c $(CPUDIR)/binder.c 

AS_SOURCES= $(CPUDIR)/rtbinder.s $(CPUDIR)/rtboot.s $(CPUDIR)/rtmemcpy.s \
	 $(CPUDIR)/clrpage.s $(CPUDIR)/align.s $(CPUDIR)/rtstrings.s

# rtboot.o, which contains the startup code, must be linked first
OBJECTS= align.o rtboot.o binder.o dlfcns.o globals.o map.o paths.o reloc.o \
	rtfcns.o rtld.o rtsetup.o rtbinder.o clrpage.o rtabiaddr.o rtsetaddr.o rtmemcpy.o \
	rtstrings.o

PRODUCTS= $(OBJECTS)


all:	$(OBJECTS) 

dlfcns.o:	$(COMDIR)/dlfcns.c $(HFILES)
	$(CC) -c $(CFLAGS) $(MODE) $(DEFLIST) $(COMDIR)/dlfcns.c

globals.o:	$(COMDIR)/globals.c $(HFILES)
	$(CC) -c $(CFLAGS) $(MODE) $(DEFLIST) $(COMDIR)/globals.c

map.o:	$(COMDIR)/map.c $(HFILES)
	$(CC) -c $(CFLAGS) $(MODE) $(DEFLIST) $(COMDIR)/map.c

rtfcns.o:	$(COMDIR)/rtfcns.c $(HFILES)
	$(CC) -c $(CFLAGS) $(MODE) $(DEFLIST) $(COMDIR)/rtfcns.c

rtld.o:	$(COMDIR)/rtld.c $(HFILES)
	$(CC) -c $(CFLAGS) $(MODE) $(DEFLIST) $(COMDIR)/rtld.c

paths.o:	$(COMDIR)/paths.c $(HFILES)
	$(CC) -c $(CFLAGS) $(MODE) $(DEFLIST) $(COMDIR)/paths.c

binder.o:	$(CPUDIR)/binder.c $(HFILES)
	$(CC) -c $(CFLAGS) $(MODE) $(DEFLIST) $(CPUDIR)/binder.c

rtsetup.o:	$(CPUDIR)/rtsetup.c $(HFILES)
	$(CC) -c $(CFLAGS) $(MODE) $(DEFLIST) $(CPUDIR)/rtsetup.c

reloc.o:	$(CPUDIR)/reloc.c $(HFILES)
	$(CC) -c $(CFLAGS) $(MODE) $(DEFLIST) $(CPUDIR)/reloc.c

rtboot.o:	$(CPUDIR)/rtboot.s $(HFILES)
	$(CC) -c $(CFLAGS) $(MODE) $(CPUDIR)/rtboot.s

rtbinder.o:	$(CPUDIR)/rtbinder.s $(HFILES)
	$(CC) -Wa,-m -c $(CFLAGS) $(MODE) $(CPUDIR)/rtbinder.s

clrpage.o:	$(CPUDIR)/clrpage.s
	$(CC) -c $(CFLAGS) $(MODE) $(CPUDIR)/clrpage.s

rtmemcpy.o:	$(CPUDIR)/rtmemcpy.s
	$(CC) -c $(CFLAGS) $(MODE) $(CPUDIR)/rtmemcpy.s

rtstrings.o:	$(CPUDIR)/rtstrings.s
	$(CC) -c $(CFLAGS) $(MODE) $(CPUDIR)/rtstrings.s

rtsetaddr.o:	$(CPUDIR)/genset.awk $(CPUDIR)/genset.in $(HFILES)
	$(ECHO) "@FULL" | $(AWK) -f $(CPUDIR)/genset.awk - $(CPUDIR)/genset.in \
		> $(CPUDIR)/rtsetaddr.s
	$(CC) -c $(CFLAGS) $(MODE) $(CPUDIR)/rtsetaddr.s
	
rtabiaddr.o:	$(CPUDIR)/genset.awk $(CPUDIR)/abigenset.in $(HFILES)
	$(ECHO) "@MIN" | $(AWK) -f $(CPUDIR)/genset.awk - $(CPUDIR)/abigenset.in \
		> $(CPUDIR)/rtabiaddr.s
	$(CC) -c $(CFLAGS) $(MODE) $(CPUDIR)/rtabiaddr.s

lintit:	$(SOURCES) $(HFILES)
	$(LINT) $(LINTFLAGS) $(LOCALINC) $(DEFLIST) $(SOURCES)

clean:
	-rm -f $(OBJECTS)

clobber:	clean
	-rm -f rtsetaddr.s rtabiaddr.s

print:	$(SOURCES) $(AS_SOURCES) $(HFILES)
	pr -n $(SOURCES) $(AS_SOURCES) $(HFILES) | lp $(PROPTS) 
