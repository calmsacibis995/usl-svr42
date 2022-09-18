#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)devintf:common/cmd/devintf/groups/mbrship/list/list.mk	1.5.8.2"
#ident "$Header: list.mk 2.0 91/07/11 $"

include $(CMDRULES)

INSDIR		= $(USRSADM)/sysadm/menu/devices/groups/mbrship/list
OWN=bin
GRP=bin

DESTDIR		= $(INSDIR)
HELPSRCDIR 	= .

SHFILES		= 
DISPFILES	= Form.list Text.list Text.none
HELPFILES	= Help

all: $(SHFILES) $(HELPFILES) $(DISPFILES) 

# $(SHFILES):

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
	for i in $(HELPFILES) ;\
	do \
		$(INS) -f $(DESTDIR) -m 0640 -u $(OWN) -g $(GRP) $(HELPSRCDIR)/$$i ;\
	done
#	for i in $(SHFILES) ;\
#	do \
#		$(INS) -m 750 -g bin -u bin -f $(DESTDIR) $$i ;\
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
