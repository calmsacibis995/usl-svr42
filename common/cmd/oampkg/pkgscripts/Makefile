#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)oampkg:common/cmd/oampkg/pkgscripts/Makefile	1.4.8.5"
#ident "$Header: $"

include $(CMDRULES)

SRCBASE=../
LINTLIBS=$(USRLIB)/llib-lpkg.ln
OBJECTS=$(O_CFILES) $(O_SHFILES) $(O_DFFILES)

LDLIBS= -lpkg -ladm -lcmd

O_SHFILES=
O_CFILES=cmdexec
O_DFFILES=default r.build i.build i.sed i.awk r.sed r.awk check

all: .check $(O_CFILES) $(O_SHFILES) $(O_DFFILES)
	@:

.check:	$(INC)/pkginfo.h
	rm -f $(O_CFILES)
	touch .check

.SUFFIXES:	.df

.df:
	cp $< $*
	chmod 644 $*

.c:
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)
	chmod 775 $@

clean:
	rm -f .check

clobber: clean
	rm -f $(O_CFILES) $(O_SHFILES) 

strip:
	$(STRIP) $(O_CFILES)

lintit:
	rm -f lint.out
	for file in $(O_CFILES) ;\
	do \
		$(LINT) $(DEFLIST) $$file.c $(LINTLIBS) >>lint.out ;\
	done

include $(SRCBASE)Install
