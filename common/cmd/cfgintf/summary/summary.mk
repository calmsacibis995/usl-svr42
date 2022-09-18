#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cfgintf:common/cmd/cfgintf/summary/summary.mk	1.11.8.2"
#ident "$Header: summary.mk 2.0 91/07/11 $"

include $(CMDRULES)

OAMBASE		= $(USRSADM)/sysadm
BINDIR		= $(OAMBASE)/bin
DESTDIR		= $(OAMBASE)/menu/machinemgmt/configmgmt/summary
HELPSRCDIR 	= .
SHFILES		=
FMTFILES	= maxcol
DISPFILES	= Text.summary
HELPFILES	= Help
SRCFILE		= maxcol.c
OWN		= bin
GRP		= bin

all: $(SHFILES) $(HELPFILES) $(FMTFILES) $(DISPFILES) 

$(FMTFILES): $(SRCFILE)
	$(CC) $(CFLAGS) -o $(FMTFILES) $(SRCFILE) $(LDFLAGS) $(LDLIBS) \
		$(SHLIBS)

$(SRCFILE):

$(HELPFILES):

$(DISPFILES):

clean:
	rm -f *.o maxcol

clobber: clean

size:

strip:

lintit:

install: $(DESTDIR) all
	for i in $(DISPFILES) ;\
	do \
		$(INS) -m 640 -g $(GRP) -g $(OWN) -f $(DESTDIR) $$i ;\
	done
	for i in $(FMTFILES) ;\
		do \
		$(INS) -m 640 -g $(GRP) -g $(OWN) -f $(DESTDIR) $$i ;\
	done
	for i in $(HELPFILES) ;\
	do \
		$(INS) -m 640 -g $(GRP) -g $(OWN) -f $(DESTDIR) $(HELPSRCDIR)/$$i ;\
	done
#	for i in $(SHFILES) ;\
#	do \
#		$(INS) -m 750 -g $(GRP) -g $(OWN) -f $(DESTDIR) $$i ;\
#	done

$(DESTDIR):
	builddir() \
	{ \
		if [ ! -d $$1 ]; \
		then \
		    builddir `dirname $$1`; \
		    mkdir $$1; \
		fi \
	}; \
	builddir $(DESTDIR)
