#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)devintf:common/cmd/devintf/devices/reserve/free/free.mk	1.8.7.2"
#ident "$Header: free.mk 2.0 91/07/11 $"

include $(CMDRULES)

INSDIR		= $(USRSADM)/sysadm/menu/devices/devices/reserve/free
OWN=bin
GRP=bin

DESTDIR		= $(INSDIR)
HELPSRCDIR	= .

SHFILES		= getkey
FMTFILES	= 
DISPFILES	= Form.free Text.c_free
HELPFILES	= Help

all: $(SHFILES) $(HELPFILES) $(FMTFILES) $(DISPFILES) 

$(SHFILES):

# $(FMTFILES):

$(HELPFILES):

$(DISPFILES):

clean:

clobber: clean

size:

strip:

lintit:

install: $(DESTDIR) all
	for i in $(DISPFILES) ;\
	do \
		$(INS) -f $(DESTDIR) -m 0640 -u $(OWN) -g $(GRP) $$i ;\
	done
#	for i in $(FMTFILES) ;\
#	do \
#		$(INS) -m 640 -g bin -u bin -f $(DESTDIR) $$i ;\
#	done
	for i in $(HELPFILES) ;\
	do \
		$(INS) -f $(DESTDIR) -m 0640 -u $(OWN) -g $(GRP) $(HELPSRCDIR)/$$i ;\
	done
	for i in $(SHFILES) ;\
	do \
		$(INS) -f $(DESTDIR) -m 0750 -u $(OWN) -g $(GRP) $$i ;\
	done

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
