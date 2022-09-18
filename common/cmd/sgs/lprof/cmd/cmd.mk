#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lprof:cmd/cmd.mk	1.9.1.14"

include $(CMDRULES)

PROF_SAVE	=
XPROF_INCS	=
PLBBASE		= $(SGSBASE)/lprof/libprof
INCBASE		= ../hdr

LINK_MODE	=

PCFLAGS		= $(CFLAGS) -DTARGETPROFILER=1

INS		= $(SGSBASE)/sgs.install
INSDIR		= $(CCSBIN)
HFILES		= \
		env.h			\
		glob.h 			\
		coredefs.h 		\
		cor_errs.h		\
		$(CPUINC)/sgs.h	\
		$(PLBBASE)/$(CPU)/mach_type.h	\
		$(INCBASE)/retcode.h	\
		$(INCBASE)/funcdata.h	\
		$(INCBASE)/covfile.h	\
		$(INCBASE)/filedata.h	\
		$(PLBBASE)/common/symint.h	\
		$(PLBBASE)/common/symintHdr.h
INCDIRS		= \
		-I . \
		-I $(PLBBASE)/$(CPU) \
		-I $(PLBBASE)/common \
		-I $(INCBASE) \
		-I $(COMINC) \
		-I $(CPUINC) \
		$(XPROF_INCS)
SOURCES		= merge.c rept_utl.c src_list.c utility.c \
		lin_rept.c sum.c list_rept.c main.c
OBJECTS		= merge.o rept_utl.o src_list.o utility.o \
		lin_rept.o sum.o list_rept.o main.o
PRODUCTS	= ../lprof

PROFLIBD	= $(PLBBASE)
LIBSYMINT	= $(SGSBASE)/lprof/libprof/$(CPU)/libsymint.a
LIBPROF		= $(SGSBASE)/lprof/libprof/$(CPU)/libprof.a
LIBS		= $(LIBPROF) $(LIBELF)

all:	$(PRODUCTS)

$(PRODUCTS): $(PLBBASE)/$(CPU)/libprof.a $(OBJECTS)
	$(CC) -o $(PRODUCTS) $(PCFLAGS) $(OBJECTS) \
	$(LDFLAGS) $(LDLIBS) $(LIBS) $(LINK_MODE)

main.o:	$(HFILES) main.c
	$(CC) -c $(PCFLAGS) $(INCDIRS) main.c

lin_rept.o: $(HFILES) lin_rept.c
	$(CC) -c $(PCFLAGS) $(INCDIRS) lin_rept.c

sum.o: $(HFILES) sum.c
	$(CC) -c $(PCFLAGS) $(INCDIRS) sum.c

list_rept.o: $(HFILES) list_rept.c
	$(CC) -c $(PCFLAGS) $(INCDIRS) list_rept.c

merge.o: $(HFILES) merge.c
	$(CC) -c $(PCFLAGS) $(INCDIRS) merge.c

rept_utl.o: $(HFILES) rept_utl.c
	$(CC) -c $(PCFLAGS) $(INCDIRS) rept_utl.c

src_list.o: $(HFILES) src_list.c
	$(CC) -c $(PCFLAGS) $(INCDIRS) src_list.c

utility.o: $(HFILES) utility.c
	$(CC) -c $(PCFLAGS) $(INCDIRS) utility.c

install: all
	cp $(PRODUCTS) lprof.bak
	$(STRIP) $(PRODUCTS)
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(CCSBIN)/$(SGS)lprof $(PRODUCTS)
	mv lprof.bak $(PRODUCTS)

lintit:	$(SOURCES)
	$(LINT) $(LINTFLAGS) $(SOURCES)

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(PRODUCTS)

$(PLBBASE)/$(CPU)/libprof.a:
	cd $(PLBBASE)/$(CPU); $(MAKE) -f libprof.mk
