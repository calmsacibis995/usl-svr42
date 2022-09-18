#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)cs:cs/cs.mk	1.14.2.5"
#ident  "$Header: cs.mk 1.4 91/07/01 $"

include $(CMDRULES)

#
# Makefile for user level connection server daemon

OWN = root
GRP = sys

DESTSBIN = $(USRSBIN)
HDRS	= dk.h sysexits.h uucp.h parms.h sysfiles.h uucp.h
LDLIBS = -lcmd -lnsl -liaf -ladm 

LOG  = $(VAR)/adm/log
AUTH = $(ETC)/cs
DIRS = $(LOG) $(ETC)/iaf $(AUTH)
VARFILES= ./log_files/cs.log ./log_files/cs.debug 
ETCFILES= ./log_files/serve.allow ./log_files/serve.alias
AUTHFILES= ./log_files/auth

OBJECTS = dial.o getscheme.o global.o log.o sendreq.o cs.o \
	callers.o conn.o \
	dkbreak.o dkdial.o dkerr.o dkminor.o dtnamer.o \
	getargs.o interface.o line.o \
	stoa.o strecpy.o sysfiles.o ulockf.o uucpdefs.o devopen.o

SOURCES = $(OBJECTS:.o=.c)

cs: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

all: cs

dial.o: $(HDRS) callers.o conn.o \
	dkbreak.o dkdial.o dkerr.o dkminor.o dtnamer.o \
	getargs.o interface.o line.o \
	stoa.o strecpy.o sysfiles.o ulockf.o uucpdefs.o devopen.o

lintit:
	$(LINT) $(LINTFLAGS) -I $(INC) $(SOURCES)

install: $(DIRS) all
	$(INS) -f $(DESTSBIN) -m 0555 -u $(OWN) -g $(GRP) cs
	for i in $(VARFILES) ; \
	do \
	 	$(INS) -f $(LOG) -m 0666 -u $(OWN) -g $(GRP) $$i ; \
	done ; \
	for i in $(ETCFILES) ; \
	do \
	 	$(INS) -f $(ETC)/iaf -m 0666 -u $(OWN) -g $(GRP) $$i ; \
	done
	for i in $(AUTHFILES) ; \
	do \
	 	$(INS) -f $(AUTH) -m 0666 -u $(OWN) -g $(GRP) $$i ; \
	done

$(DIRS):
	[ -d $@ ] || mkdir -p $@

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f cs
