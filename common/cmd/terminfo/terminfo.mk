#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)terminfo:common/cmd/terminfo/terminfo.mk	1.10.4.2"
#ident	"$Header: $"

include $(CMDRULES)

#
#	terminfo makefile
#

TERMDIR =	$(USRSHARE)/lib/terminfo
TABDIR  =	$(USRSHARE)/lib/tabset
TERMCAPDIR =	$(USRSHARE)/lib


PARTS =		header *.ti trailer

all: ckdir terminfo.src
	TERMINFO=$(TERMDIR) 2>&1 $(TIC) -v terminfo.src > errs
	@touch install
	@echo
	@sh ./ckout
	@echo
	@echo
	@echo
	cp tabset/* $(TABDIR)
	cp *.ti $(TERMDIR)/ti
	cp termcap $(TERMCAPDIR)

install: all

terminfo.src: $(PARTS)
	@cat $(PARTS) > terminfo.src

clean:
	rm -f terminfo.src install errs nohup.out

clobber: clean

ckdir:
	@echo
	@echo "The terminfo database will be built in $(TERMDIR)."
	@echo "Checking for the existence of $(TERMDIR):"
	@echo
	[ -d $(TERMDIR) ] || mkdir -p $(TERMDIR)
	$(CH)chown bin $(TERMDIR);
	$(CH)chgrp bin $(TERMDIR);
	$(CH)chmod 775 $(TERMDIR);
	@echo
	@echo
	@echo "The terminfo database will reference the tabset file in $(TABDIR)."
	@echo "Checking for the existence of $(TABDIR):"
	@echo
	[ -d $(TABDIR) ] || mkdir -p $(TABDIR)
	$(CH)chown bin $(TABDIR);
	$(CH)chgrp bin $(TABDIR);
	$(CH)chmod 775 $(TABDIR);
	@echo
	@echo
	@echo "The terminfo source files will be installed in $(TERMDIR)/ti."
	@echo "Checking for the existence of $(TERMDIR)/ti:"
	@echo
	[ -d $(TERMDIR)/ti ] || mkdir -p $(TERMDIR)/ti
	$(CH)chown root $(TERMDIR)/ti;
	$(CH)chgrp root $(TERMDIR)/ti;
	$(CH)chmod 775 $(TERMDIR)/ti;
	@echo
