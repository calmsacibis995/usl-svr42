#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lprof:libprof/i386/libprof.mk	1.2"
#
#	makefile for libprof.a and for libsymint.a
#

include $(CMDRULES)

PROF_RUNLD=ld
PROF_PCRTI=
PROF_PCRTN=
PROF_SAVE	=
XPROF_INCS	=
PLBBASE		= ../../libprof
COMDIR		= $(PLBBASE)/common
CPUDIR		= $(PLBBASE)/$(CPU)
INCBASE		= ../../hdr
SGSBASE		= ../../..

# TARGETPROFILER:   1 => lprof   2 => prof
CFLAGS_1	= $(CFLAGS) -DTARGETPROFILER=1
CFLAGS_2	= $(CFLAGS) -DTARGETPROFILER=2

INS		= $(SGSBASE)/sgs.install
INSDIR		= $(CCSBIN)

HFILES_A	= $(COMDIR)/lst_str.h $(COMDIR)/cov_errs.h \
		$(INCBASE)/covfile.h $(INCBASE)/retcode.h $(INCBASE)/filedata.h 
HFILES_B	= mach_type.h $(COMDIR)/symint.h \
		$(COMDIR)/symintHdr.h $(COMDIR)/debug.h $(COMDIR)/profopt.h
HFILES		= $(HFILES_A) $(HFILES_B)

INCDIRS		= \
		-I . \
		-I $(COMDIR) \
		-I $(INCBASE) \
		-I $(SGSBASE)/inc/common \
		-I $(CPUINC) \
		$(XPROF_INCS)

PROD_1		= libprof.a
PROD_2		= libsymint.a
PROD_1SO	= libprof.so
PROD_2SO	= libsymint.so
PRODUCTS	= $(PROD_1) $(PROD_2)

SRCS_1		= $(COMDIR)/dump.c $(COMDIR)/comops.c $(COMDIR)/cov_join.c \
		$(COMDIR)/exist.c $(COMDIR)/new.c $(COMDIR)/soqueue.c
SRCS_2		= $(COMDIR)/symintClose.c $(COMDIR)/symintErr.c \
		$(COMDIR)/symintLoad.c $(COMDIR)/symintOpen.c \
		$(COMDIR)/symintUtil.c
SOURCES		= $(SRCS_1) $(SRCS_2)

IOBJS_0		= symintClose.o symintErr.o symintLoad.o \
		symintOpen.o symintUtil.o
IOBJS_1		= symintClose1.o symintErr1.o symintLoad1.o \
		symintOpen1.o symintUtil1.o
IOBJS_2		= symintClose2.o symintErr2.o symintLoad2.o \
		symintOpen2.o symintUtil2.o
OBJS_1		= $(IOBJS_1) dump.o comops.o cov_join.o exist.o new.o soqueue.o
OBJS_2		= $(IOBJS_2)
OBJECTS		= $(OBJS_1) $(OBJS_2) $(IOBJS_0)

#PLBBASE		= $(SGSBASE)/lprof/libprof
PROFLIBD	= $(PLBBASE)
LIBSYMINT	= $(SGSBASE)/lprof/libprof/libsymint.a
LDFLAGS		=
LIBS		= $(PLBBASE)/libprof.a $(LIBELF)

all: $(PRODUCTS)
	if test ! "$(PROF_SAVE)"; then rm -f *.o; fi

$(PROD_1): $(OBJS_1)
	$(AR) $(ARFLAGS) $(PROD_1) `$(LORDER) $(OBJS_1) | tsort`

$(PROD_2): $(OBJS_2)
	$(AR) $(ARFLAGS) $(PROD_2) `$(LORDER) $(OBJS_2) | tsort`

$(PROD_1SO): $(PROD_1) $(OBJS_1)
	if echo $(CFLAGS) | grep ql > /dev/null; then \
		$(PROF_RUNLD) -dy -G -Bsymbolic $(OBJS_1) \
			$(PROF_PCRTI) $(PROF_PCRTN) \
			-lelf -lc -lm -Qy -o $(PROD_1SO); \
	else \
		$(PROF_RUNLD) -dy -G -Bsymbolic $(OBJS_1) \
			-lelf -lc -lm -Qy -o $(PROD_1SO); \
	fi

$(PROD_2SO): $(PROD_1) $(OBJS_2)
	if echo $(CFLAGS) | grep ql > /dev/null; then \
		$(PROF_RUNLD) -dy -G -Bsymbolic $(OBJS_2) \
			$(PROF_PCRTI) $(PROF_PCRTN) \
			-lelf -lc -lm -Qy -o $(PROD_2SO); \
	else \
		$(PROF_RUNLD) -dy -G -Bsymbolic $(OBJS_2) \
			-lelf -lc -lm -Qy -o $(PROD_2SO); \
	fi

comops.o:	$(HFILES) $(COMDIR)/comops.c
	$(CC) -c $(CFLAGS_1) $(INCDIRS) $(COMDIR)/comops.c
cov_join.o:	$(HFILES) $(COMDIR)/cov_join.c
	$(CC) -c $(CFLAGS_1) $(INCDIRS) $(COMDIR)/cov_join.c
dump.o:		$(HFILES) $(COMDIR)/dump.c
	$(CC) -c $(CFLAGS_1) $(INCDIRS) $(COMDIR)/dump.c
exist.o:	$(HFILES) $(COMDIR)/exist.c
	$(CC) -c $(CFLAGS_1) $(INCDIRS) $(COMDIR)/exist.c
new.o:		$(HFILES) $(COMDIR)/new.c
	$(CC) -c $(CFLAGS_1) $(INCDIRS) $(COMDIR)/new.c
soqueue.o:	$(HFILES) $(COMDIR)/soqueue.c
	$(CC) -c $(CFLAGS_1) $(INCDIRS) $(COMDIR)/soqueue.c

symintClose1.o symintClose2.o: symintClose.o
symintClose.o: $(HFILES_B) $(COMDIR)/symintClose.c
	$(CC) -c $(CFLAGS_1) $(INCDIRS) $(COMDIR)/symintClose.c;	mv symintClose.o symintClose1.o
	$(CC) -c $(CFLAGS_2) $(INCDIRS) $(COMDIR)/symintClose.c;	ln symintClose.o symintClose2.o

symintErr1.o symintErr2.o: symintErr.o
symintErr.o: $(HFILES_B) $(COMDIR)/symintErr.c
	$(CC) -c $(CFLAGS_1) $(INCDIRS) $(COMDIR)/symintErr.c;	mv symintErr.o symintErr1.o
	$(CC) -c $(CFLAGS_2) $(INCDIRS) $(COMDIR)/symintErr.c;	ln symintErr.o symintErr2.o

symintLoad1.o symintLoad2.o: symintLoad.o
symintLoad.o: $(HFILES_B) $(COMDIR)/symintLoad.c
	$(CC) -c $(CFLAGS_1) $(INCDIRS) $(COMDIR)/symintLoad.c;	mv symintLoad.o symintLoad1.o
	$(CC) -c $(CFLAGS_2) $(INCDIRS) $(COMDIR)/symintLoad.c;	ln symintLoad.o symintLoad2.o

symintOpen1.o symintOpen2.o: symintOpen.o
symintOpen.o: $(HFILES_B) $(COMDIR)/symintOpen.c
	$(CC) -c $(CFLAGS_1) $(INCDIRS) $(COMDIR)/symintOpen.c;	mv symintOpen.o symintOpen1.o
	$(CC) -c $(CFLAGS_2) $(INCDIRS) $(COMDIR)/symintOpen.c;	ln symintOpen.o symintOpen2.o

symintUtil1.o symintUtil2.o: symintUtil.o
symintUtil.o: $(HFILES_B) $(COMDIR)/symintUtil.c
	$(CC) -c $(CFLAGS_1) $(INCDIRS) $(COMDIR)/symintUtil.c;	mv symintUtil.o symintUtil1.o
	$(CC) -c $(CFLAGS_2) $(INCDIRS) $(COMDIR)/symintUtil.c;	ln symintUtil.o symintUtil2.o


install: all
	/bin/sh $(INS) 644 $(OWN) $(GRP) $(CCSLIB)/libprof.a libprof.a

lintit:	$(SOURCES)
	$(LINT) $(LINTFLAGS) $(SOURCES)

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(PRODUCTS)
	rm -f $(PROD_1SO) $(PROD_2SO)

