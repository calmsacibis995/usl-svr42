#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)ksh:shlib/shlib.mk	1.2"
#ident "$Header: shlib.mk 1.3 91/04/29 $"

include $(CMDRULES)

# makefile for shlib generated by mkold and hand modified

KSHINC	 = ../include

LOCALINC = -I$(KSHINC)
LOCALDEF = -DKSHELL

SOURCES = adjust.c assign.c assnum.c cannon.c chkid.c convert.c findnod.c \
	gettree.c growaray.c gsort.c linknod.c namscan.c optget.c rjust.c \
	strdata.c streval.c strmatch.c tilde.c unassign.c utos.c valup.c

OBJECTS = $(SOURCES:.c=.o)


all: libsh.a

libsh.a: $(OBJECTS)
	$(AR) $(ARFLAGS) libsh.a $(OBJECTS)

adjust.o: adjust.c \
	$(KSHINC)/sh_config.h \
	$(INC)/sys/types.h \
	$(KSHINC)/name.h \
	$(KSHINC)/flags.h 

assign.o: assign.c \
	$(KSHINC)/sh_config.h \
	$(INC)/sys/types.h \
	$(KSHINC)/name.h \
	$(KSHINC)/flags.h \
	$(KSHINC)/national.h

assnum.o: assnum.c \
	$(KSHINC)/name.h \
	$(KSHINC)/sh_config.h \
	$(INC)/sys/types.h \
	$(KSHINC)/flags.h 

cannon.o: cannon.c \
	$(KSHINC)/io.h \
	$(KSHINC)/sh_config.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/unistd.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/sys/file.h

chkid.o: chkid.c \
	$(KSHINC)/sh_config.h \
	$(INC)/sys/types.h

convert.o: convert.c \
	$(KSHINC)/shtype.h \
	$(KSHINC)/sh_config.h \
	$(INC)/ctype.h

findnod.o: findnod.c \
	$(KSHINC)/sh_config.h \
	$(INC)/sys/types.h \
	$(KSHINC)/name.h \
	$(KSHINC)/flags.h \
	$(KSHINC)/shtype.h \
	$(INC)/ctype.h

gettree.o: gettree.c \
	$(KSHINC)/name.h \
	$(KSHINC)/sh_config.h \
	$(INC)/sys/types.h \
	$(KSHINC)/flags.h 

growaray.o: growaray.c \
	$(KSHINC)/name.h \
	$(KSHINC)/sh_config.h \
	$(INC)/sys/types.h \
	$(KSHINC)/flags.h 

gsort.o: gsort.c

linknod.o: linknod.c \
	$(KSHINC)/name.h \
	$(KSHINC)/sh_config.h \
	$(INC)/sys/types.h \
	$(KSHINC)/flags.h

namscan.o: namscan.c \
	$(KSHINC)/name.h \
	$(KSHINC)/sh_config.h \
	$(INC)/sys/types.h \
	$(KSHINC)/flags.h

optget.o: optget.c \
	$(KSHINC)/sh_config.h \
	$(INC)/sys/types.h

rjust.o: rjust.c \
	$(KSHINC)/shtype.h \
	$(KSHINC)/sh_config.h \
	$(INC)/ctype.h

strdata.o: strdata.c \
	$(KSHINC)/streval.h \
	$(KSHINC)/sh_config.h \
	$(INC)/sys/types.h

streval.o: streval.c \
	$(KSHINC)/shtype.h \
	$(INC)/ctype.h \
	$(KSHINC)/streval.h \
	$(KSHINC)/sh_config.h \
	$(INC)/sys/types.h

strmatch.o: strmatch.c \
	$(INC)/ctype.h \
	$(KSHINC)/national.h

tilde.o: tilde.c \
	$(KSHINC)/sh_config.h \
	$(INC)/sys/types.h \
	$(KSHINC)/defs.h \
	$(INC)/setjmp.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/unistd.h \
	$(INC)/sys/times.h \
	$(KSHINC)/name.h \
	$(KSHINC)/shnodes.h \
	$(KSHINC)/stak.h \
	$(KSHINC)/shtype.h \
	$(INC)/mnttab.h \
	$(INC)/sys/utsname.h \
	$(INC)/pwd.h

unassign.o: unassign.c \
	$(KSHINC)/name.h \
	$(KSHINC)/sh_config.h \
	$(INC)/sys/types.h \
	$(KSHINC)/flags.h

utos.o: utos.c \
	$(KSHINC)/sh_config.h \
	$(INC)/sys/types.h

valup.o: valup.c \
	$(KSHINC)/name.h \
	$(KSHINC)/sh_config.h \
	$(INC)/sys/types.h \
	$(KSHINC)/flags.h 

clean :
	rm -f $(OBJECTS)

clobber : clean
	rm -f libsh.a

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)