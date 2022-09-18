#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/rcmds.d/Makefile	1.3"
#ident	"$Header: $"

include $(CMDRULES)

OAMBASE = $(ROOT)/$(MACH)/usr/sadm/sysadm
BINDIR = $(OAMBASE)/bin
LOCALINC = -I ..
OWN = bin
GRP = bin

SHELLS = add_cron filechk filemsg owrchk runbackf runbacku runstore runstore2
CFILES = Ckdev chg_cron del_cron display format getchg getdel prtask setquote \
         ts_valid slash

all:	$(CFILES) $(SHELLS)

$(CFILES):
	$(CC) $(CFLAGS) $(DEFLIST) $(@).c -o $(@) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

$(SHELLS):
	-cp $(@).sh $(@)

install:	all
	-mkdir -p $(BINDIR)

	-for i in $(CFILES); \
	do \
		$(INS) -f $(BINDIR) -m 0755 -u $(OWN) -g $(GRP) $$i ;\
	done

	-for i in $(SHELLS); \
	do \
		$(INS) -f $(BINDIR) -m 0755 -u $(OWN) -g $(GRP) $$i ;\
	done

clean:
	-rm -f *.o $(SHELLS) $(CFILES)

clobber: clean
	@echo "No targets to clobber!"

size:
	$(SIZE) $(CFILES)

strip:
	$(STRIP) $(CFILES)

lintit:
	for file in $(CFILES); \
	do \
		$(LINT) $$file.c ;\
	done
