#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)iconv:iconv.mk	1.4.10.4"
#ident	"$Header: $"


include $(CMDRULES)

LOCALINC= -I.

ICONV=$(USRLIB)/iconv
LICONV=/usr/lib/iconv
OWN = bin
GRP = bin

IOBJS=	iconv.o gettab.o process.o
KOBJS=	main.o gram.o lexan.o output.o reach.o sort.o sym.o tree.o
LOC_KOBJS= lmain.o lgram.o llexan.o loutput.o lreach.o lsort.o lsym.o ltree.o \
	loc_msg.o

#MAINS = iconv kbdcomp local_kbdcomp
MAINS = iconv kbdcomp

CODESETS=\
	codesets/646da.8859.p codesets/646de.8859.p codesets/646en.8859.p \
	codesets/646es.8859.p codesets/646fr.8859.p codesets/646it.8859.p \
	codesets/646sv.8859.p codesets/8859.646.p codesets/8859.646da.p \
	codesets/8859.646de.p codesets/8859.646en.p codesets/8859.646es.p \
	codesets/8859.646fr.p codesets/8859.646it.p codesets/8859.646sv.p \
	codesets/8859-1.dk.p codesets/Case.p codesets/Cmacs.p codesets/Deutsche.p \
	codesets/Dvorak.p codesets/PFkeytest.p codesets/lnktst.p

.MUTEX: gram.c

all:	$(MAINS)

iconv:	$(IOBJS)
	$(CC) -o $@ $(IOBJS) $(LDLIBS) $(SHLIBS) $(LDFLAGS)

kbdcomp: $(KOBJS)
	$(CC) -o $@ $(KOBJS) $(LDLIBS) $(SHLIBS) $(LDFLAGS)

local_kbdcomp: $(LOC_KOBJS)
	$(HCC) -o $@ $(LOC_KOBJS) $(LDLIBS) $(SHLIBS) $(LDFLAGS)

install : $(MAINS)
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) iconv
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) kbdcomp
	- [ -d $(ICONV)/codesets ] || mkdir -p $(ICONV)/codesets ; \
		$(CH)chmod 755 $(ICONV)/codesets ; \
		$(CH)chown $(OWN) $(ICONV)/codesets ; \
		$(CH)chgrp $(GRP) $(ICONV)/codesets
	$(INS) -f  $(ICONV) -m 0444 -u $(OWN) -g $(GRP) codesets/iconv_data
	for i in  $(CODESETS) ;\
	do \
		$(CH)./local_kbdcomp -o `basename $$i .p` $$i ; \
		$(CH)$(INS) -f $(ICONV) -m 0444 -u $(OWN) -g $(GRP) `basename $$i .p` ; \
		$(INS) -f $(ICONV)/codesets -m 0444 -u $(OWN) -g $(GRP) $$i; \
	done

	
$(IOBJS): ./iconv.h ./symtab.h ./kbd.h

$(LOC_KOBJS) $(KOBJS):	./symtab.h ./kbd.h

.PRECIOUS:	gram.y

gram.c:	gram.y ./symtab.h ./kbd.h
	$(YACC) -vd gram.y
	mv y.tab.c gram.c

.c.o:
	$(CC) $(CFLAGS) $(DEFLIST) -c $*.c

clean:
	rm -f *.o *.t y.tab.h y.output gram.c local_kbdcomp
	
clobber: clean
	rm -f *.o *.t iconv kbdcomp
	$(CH)for i in  $(CODESETS) ;\
	$(CH)do \
		$(CH) rm -f `basename $$i .p` ; \
	$(CH)done

lintit:
	$(LINT) $(LINTFLAGS) $(IOBJS:.o=.c)
	$(LINT) $(LINTFLAGS) $(KOBJS:.o=.c)

lmain.o:	main.c
	cp main.c lmain.c
	$(HCC) -Dconst="" -O $(DEFLIST) -c lmain.c
	rm -f lmain.c

lgram.o:	gram.c
	cp gram.c lgram.c
	$(HCC) -Dconst="" -O $(DEFLIST) -c lgram.c
	rm -f lgram.c

llexan.o:	lexan.c
	cp lexan.c llexan.c
	$(HCC) -Dconst="" -O $(DEFLIST) -c llexan.c
	rm -f llexan.c

loutput.o:	output.c
	cp output.c loutput.c
	$(HCC) -Dconst="" -O $(DEFLIST) -c loutput.c
	rm -f loutput.c

lreach.o:	reach.c
	cp reach.c lreach.c
	$(HCC) -Dconst="" -O $(DEFLIST) -c lreach.c
	rm -f lreach.c

lsort.o:	sort.c
	cp sort.c lsort.c
	$(HCC) -Dconst="" -O $(DEFLIST) -c lsort.c
	rm -f lsort.c

lsym.o:		sym.c
	cp sym.c lsym.c
	$(HCC) -Dconst="" -O $(DEFLIST) -c lsym.c
	rm -f lsym.c

ltree.o:	tree.c
	cp tree.c ltree.c
	$(HCC) -Dconst="" -O $(DEFLIST) -c ltree.c
	rm -f ltree.c

loc_msg.o:	loc_msg.c
	$(HCC) -Dconst="" -O $(DEFLIST) -c loc_msg.c
