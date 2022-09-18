#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)valtools:valtools.mk	1.1.9.3"
#ident "$Header: valtools.mk 1.5 91/06/06 $"

include $(CMDRULES)

LDLIBS = -lpkg -ladm 

OWN = bin
GRP = bin

#LLIBADM=$(USRLIB)/llib-ladm.ln
#LLIBPKG=$(USRLIB)/llib-lpkg.ln
#LINTLIBS=$(LLIBADM) $(LLIBPKG)

SOURCES = ckint.c ckitem.c ckpath.c ckrange.c ckstr.c ckyorn.c ckkeywd.c \
	puttext.c ckdate.c cktime.c ckuid.c ckgid.c

FILES = ckint ckitem ckpath ckrange ckstr ckyorn ckkeywd puttext ckdate \
	cktime ckuid ckgid

LOCALHEADS = $(INC)/stdio.h $(INC)/string.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	usage.h

all: $(FILES)

ckint: ckint.c $(LOCALHEADS)
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

ckitem: ckitem.c \
	$(LOCALHEADS) \
	$(INC)/ctype.h \
	$(INC)/valtools.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

ckpath: ckpath.c \
	$(LOCALHEADS) \
	$(INC)/valtools.h 
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

ckrange: ckrange.c \
	$(LOCALHEADS) \
	$(INC)/limits.h \
	$(INC)/values.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

ckstr: ckstr.c $(LOCALHEADS)
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

ckyorn: ckyorn.c $(LOCALHEADS)
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

ckkeywd: ckkeywd.c $(LOCALHEADS)
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

puttext: puttext.c \
	$(LOCALHEADS) \
	$(INC)/ctype.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

ckdate: ckdate.c $(LOCALHEADS)
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

cktime: cktime.c $(LOCALHEADS)
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

ckuid: ckuid.c $(LOCALHEADS)
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

ckgid: ckgid.c $(LOCALHEADS)
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f $(FILES)

lintit:
	rm -f lint.out
	for file in $(SOURCES) ;\
	do \
	echo '## lint output for '$$file' ##' >>lint.out ;\
	$(LINT) $(LINTFLAGS) $$file $(LINTLIBS) >>lint.out ;\
	done

install: all
	[ -d $(USRSADM)/bin ] || mkdir -p $(USRSADM)/bin
	$(CH)chmod 0755 $(USRSADM)
	$(CH)chgrp $(GRP) $(USRSADM)
	$(CH)chown $(OWN) $(USRSADM)
	$(INS) -f $(USRSADM)/bin -m 0555 -u $(OWN) -g $(GRP) puttext
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) ckint
	-rm -f $(USRSADM)/bin/valint
	-rm -f $(USRSADM)/bin/helpint
	-rm -f $(USRSADM)/bin/errint
	ln $(USRBIN)/ckint $(USRSADM)/bin/valint
	ln $(USRBIN)/ckint $(USRSADM)/bin/helpint
	ln $(USRBIN)/ckint $(USRSADM)/bin/errint
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) ckitem
	-rm -f $(USRSADM)/bin/helpitem
	-rm -f $(USRSADM)/bin/erritem
	ln $(USRBIN)/ckitem $(USRSADM)/bin/helpitem
	ln $(USRBIN)/ckitem $(USRSADM)/bin/erritem
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) ckpath
	-rm -f $(USRSADM)/bin/valpath
	-rm -f $(USRSADM)/bin/helppath
	-rm -f $(USRSADM)/bin/errpath
	ln $(USRBIN)/ckpath $(USRSADM)/bin/valpath
	ln $(USRBIN)/ckpath $(USRSADM)/bin/helppath
	ln $(USRBIN)/ckpath $(USRSADM)/bin/errpath
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) ckrange
	-rm -f $(USRSADM)/bin/valrange
	-rm -f $(USRSADM)/bin/helprange
	-rm -f $(USRSADM)/bin/errange
	ln $(USRBIN)/ckrange $(USRSADM)/bin/valrange
	ln $(USRBIN)/ckrange $(USRSADM)/bin/helprange
	ln $(USRBIN)/ckrange $(USRSADM)/bin/errange
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) ckstr
	-rm -f $(USRSADM)/bin/valstr
	-rm -f $(USRSADM)/bin/helpstr
	-rm -f $(USRSADM)/bin/errstr
	ln $(USRBIN)/ckstr $(USRSADM)/bin/valstr
	ln $(USRBIN)/ckstr $(USRSADM)/bin/helpstr
	ln $(USRBIN)/ckstr $(USRSADM)/bin/errstr
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) ckyorn
	-rm -f $(USRSADM)/bin/valyorn
	-rm -f $(USRSADM)/bin/helpyorn
	-rm -f $(USRSADM)/bin/erryorn
	ln $(USRBIN)/ckyorn $(USRSADM)/bin/valyorn
	ln $(USRBIN)/ckyorn $(USRSADM)/bin/helpyorn
	ln $(USRBIN)/ckyorn $(USRSADM)/bin/erryorn
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) ckkeywd
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) cktime
	-rm -f $(USRSADM)/bin/valtime
	-rm -f $(USRSADM)/bin/helptime
	-rm -f $(USRSADM)/bin/errtime
	ln $(USRBIN)/cktime $(USRSADM)/bin/valtime
	ln $(USRBIN)/cktime $(USRSADM)/bin/helptime
	ln $(USRBIN)/cktime $(USRSADM)/bin/errtime
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) ckdate
	-rm -f $(USRSADM)/bin/valdate
	-rm -f $(USRSADM)/bin/helpdate
	-rm -f $(USRSADM)/bin/errdate
	ln $(USRBIN)/ckdate $(USRSADM)/bin/valdate
	ln $(USRBIN)/ckdate $(USRSADM)/bin/helpdate
	ln $(USRBIN)/ckdate $(USRSADM)/bin/errdate
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) ckuid
	-rm -f $(USRBIN)/dispuid
	ln $(USRBIN)/ckuid $(USRBIN)/dispuid
	-rm -f $(USRSADM)/bin/valuid
	-rm -f $(USRSADM)/bin/helpuid
	-rm -f $(USRSADM)/bin/erruid
	-rm -f $(USRSADM)/bin/dispuid
	ln $(USRBIN)/ckuid $(USRSADM)/bin/valuid
	ln $(USRBIN)/ckuid $(USRSADM)/bin/helpuid
	ln $(USRBIN)/ckuid $(USRSADM)/bin/erruid
	ln $(USRBIN)/ckuid $(USRSADM)/bin/dispuid
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) ckgid
	-rm -f $(USRBIN)/dispgid
	ln $(USRBIN)/ckgid $(USRBIN)/dispgid
	-rm -f $(USRSADM)/bin/valgid
	-rm -f $(USRSADM)/bin/helpgid
	-rm -f $(USRSADM)/bin/errgid
	-rm -f $(USRSADM)/bin/dispgid
	ln $(USRBIN)/ckgid $(USRSADM)/bin/valgid
	ln $(USRBIN)/ckgid $(USRSADM)/bin/helpgid
	ln $(USRBIN)/ckgid $(USRSADM)/bin/errgid
	ln $(USRBIN)/ckgid $(USRSADM)/bin/dispgid
