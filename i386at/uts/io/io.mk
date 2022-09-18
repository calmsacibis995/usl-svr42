#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86at:io/io.mk	1.19"
#ident	"$Header: $"

include $(UTSRULES)

KBASE     = ..
IO        = $(CONF)/pack.d/io/Driver.o
FILES = \
	iosubr.o \
	ddi.o \
	ddi386at.o \
	physdsk.o \
	predki.o \
	strcalls.o \
	stream.o \
	streamio.o \
	strsubr.o

CFILES = $(FILES:.o=.c)

all:	ID $(IO) dir

$(IO): $(FILES)
	$(LD) -r -o $@ $(FILES)

ID:
	cd io.cf; $(IDINSTALL) -R$(CONF) -M io

dir:
	@for i in *;\
	do\
		if [ -d $$i -a $$i != io.cf ];then\
			cd  $$i;\
			echo "====== $(MAKE) -f $$i.mk" ; \
			$(MAKE) -f $$i.mk $(MAKEARGS) ; \
			cd .. ; \
		fi;\
	done

depend:: makedep
	@for i in *;\
	do\
		if [ -d $$i -a $$i != io.cf ];then\
			cd  $$i;\
			echo "====== $(MAKE) -f $$i.mk depend" ; \
			$(MAKE) -f $$i.mk depend MAKEFILE=$$i.mk $(MAKEARGS) ; \
			cd .. ; \
		fi;\
	done

clean:
	-rm -f $(FILES)
	@for i in *; \
	do \
		if [ -d $$i -a $$i != io.cf ]; then \
			cd $$i; \
			$(MAKE) -f $$i.mk clean $(MAKEARGS); \
			cd ..; \
		fi; \
	done

clobber:
	-rm -f $(FILES)
	@for i in *; \
	do \
		if [ -d $$i -a $$i != io.cf ]; then \
			cd $$i; \
			$(MAKE) -f $$i.mk clobber $(MAKEARGS) ; \
			cd ..; \
		fi; \
	done
	-$(IDINSTALL) -R$(CONF) -e -d io

headinstall:  \
	$(KBASE)/io/ascii.h \
	$(KBASE)/io/asyncio.h \
	$(KBASE)/io/asyncsys.h \
	$(KBASE)/io/conf.h \
	$(KBASE)/io/ddi.h \
	$(KBASE)/io/ddi_i386at.h \
	$(KBASE)/io/dma.h \
	$(KBASE)/io/elog.h \
	$(KBASE)/io/i8237A.h \
	$(KBASE)/io/iobuf.h \
	$(KBASE)/io/ioctl.h \
	$(KBASE)/io/mkdev.h \
	$(KBASE)/io/open.h \
	$(KBASE)/io/poll.h \
	$(KBASE)/io/stermio.h \
	$(KBASE)/io/stream.h \
	$(KBASE)/io/strlog.h \
	$(KBASE)/io/strmdep.h \
	$(KBASE)/io/stropts.h \
	$(KBASE)/io/strstat.h \
	$(KBASE)/io/strsubr.h \
	$(KBASE)/io/strtty.h \
	$(KBASE)/io/syslog.h \
	$(KBASE)/io/termio.h \
	$(KBASE)/io/termios.h \
	$(KBASE)/io/termiox.h \
	$(KBASE)/io/ttold.h \
	$(KBASE)/io/tty.h \
	$(KBASE)/io/ttychars.h \
	$(KBASE)/io/ttydev.h \
	$(KBASE)/io/uio.h \
	$(KBASE)/io/vtoc.h \
	$(FRC)
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/ascii.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/asyncio.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/asyncsys.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/conf.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/ddi.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/ddi_i386at.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/dma.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/elog.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/i8237A.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/iobuf.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/ioctl.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/mkdev.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/mouse.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/open.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/poll.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/stermio.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/stream.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/strlog.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/strmdep.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/stropts.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/strstat.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/strsubr.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/strtty.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/syslog.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/termio.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/termios.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/termiox.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/ttold.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/tty.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/ttychars.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/ttydev.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/uio.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/vtoc.h
	@for i in *; \
	do \
		if [ -d $$i -a $$i != io.cf ]; then \
			cd $$i; \
			$(MAKE) -f $$i.mk $(MAKEARGS) headinstall; \
			cd ..; \
		fi; \
	done


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

