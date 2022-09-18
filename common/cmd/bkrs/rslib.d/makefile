#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)bkrs:common/cmd/bkrs/rslib.d/makefile	1.9.5.2"
#ident "$Header: makefile 1.3 91/05/23 $"

include $(CMDRULES)

SRC1= rsspawn.c rsstrat.c rstm.c rstree.c queue.c rsfile.c gethdr.c
OBJ=$(SRC1:.c=.o)

BKRSDIR=$(USROAM)/bkrs
SADMDIR=$(USRSADM)
BKBIN=$(SADMDIR)/bkup/bin
OAMBASE=$(SADMDIR)/sysadm
ETCBKUP=$(ETC)/bkup
LINTFLAGS=-u $(DEFLIST)

BKLIB=../bklib.d/bklib.a
LIBBR=../libbrmeth.d/libbrmeth.a
IOLIB=../libadmIO.d/libadmIO.a
LOCAL=hdrs
GENERAL=../hdrs
FLAGS=
LDLIBS=$(LDLIBPATH) -lTL -lgen
LOCALINC=-I. -I $(LOCAL) -I $(GENERAL)
ARFLAGS=cr
OWN=root
GRP=sys

PRODUCT=rslib.a
TABLES=rsmethod.tab rsstrat.tab

all: $(PRODUCT)

rslib.a: $(OBJ)
	$(AR) $(ARFLAGS) $(@) `$(LORDER) $(OBJ) | tsort`

touch: 
	touch $(SRC1)

clean:
	rm -f $(OBJ)

clobber: clean
	rm -f $(PRODUCT)

lintit:
	$(LINT) $(LINTFLAGS) $(SRC1)

strip: $(PRODUCT)

install: $(PRODUCT) $(TABLES)
	$(INS) -f $(ETCBKUP) -u $(OWN) -g $(GRP) -m 0644 rsmethod.tab
	$(INS) -f $(ETCBKUP) -u $(OWN) -g $(GRP) -m 0644 rsstrat.tab

rstest: $(PRODUCT) $(BKLIB) $(LIBBR) $(IOLIB) rstest.o
	cc $(LDFLAGS) -o $(@) rstest.o $(PRODUCT) $(BKLIB) $(LIBBR) $(IOLIB) \
		$(LDLIBS) $(SHLIBS)

rsspawn.o: $(INC)/stdio.h
rsspawn.o: $(INC)/signal.h
rsspawn.o: $(INC)/sys/types.h
rsspawn.o: $(INC)/errno.h
rsspawn.o: $(INC)/bkrs.h
rsspawn.o: $(GENERAL)/backup.h
rsspawn.o: $(GENERAL)/restore.h
rsspawn.o: $(GENERAL)/rsstatus.h
rsspawn.o: $(GENERAL)/bkerrors.h
rsstrat.o: $(INC)/fcntl.h
rsstrat.o: $(INC)/time.h
rsstrat.o: $(INC)/errno.h
rsstrat.o: $(INC)/table.h
rsstrat.o: $(GENERAL)/bktypes.h
rsstrat.o: $(GENERAL)/bkerrors.h
rsstrat.o: $(GENERAL)/restore.h
rsstrat.o: $(GENERAL)/rsmethod.h
rsstrat.o: $(GENERAL)/rsstrat.h
rsstrat.o: $(LOCAL)/rstm.h
rstest.o: $(INC)/time.h
rstest.o: $(INC)/fcntl.h
rstest.o: $(INC)/stdio.h
rstest.o: $(INC)/ctype.h
rstest.o: $(INC)/table.h
rstest.o: $(INC)/errno.h
rstest.o: $(INC)/bkrs.h
rstest.o: $(GENERAL)/restore.h
rstest.o: $(GENERAL)/bkhist.h
rstm.o: $(INC)/fcntl.h
rstm.o: $(GENERAL)/brtoc.h
rstm.o: $(INC)/sys/types.h
rstm.o: $(INC)/sys/stat.h
rstm.o: $(INC)/time.h
rstm.o: $(INC)/table.h
rstm.o: $(GENERAL)/bktypes.h
rstm.o: $(GENERAL)/bkhist.h
rstm.o: $(GENERAL)/restore.h
rstm.o: $(GENERAL)/rsstrat.h
rstm.o: $(GENERAL)/rsmethod.h
rstm.o: $(LOCAL)/rstm.h
rstree.o: $(INC)/search.h
rstree.o: $(GENERAL)/bktypes.h
rstree.o: $(GENERAL)/bkerrors.h
rstree.o: $(LOCAL)/rstm.h
rsspawn.o: $(INC)/stdio.h
rsspawn.o: $(INC)/sys/types.h
rsspawn.o: $(INC)/errno.h
rsspawn.o: $(INC)/bkrs.h
rsspawn.o: $(GENERAL)/backup.h
rsspawn.o: $(GENERAL)/bkmsgs.h
rsspawn.o: $(GENERAL)/restore.h
rsspawn.o: $(GENERAL)/bkerrors.h
queue.o: $(INC)/sys/types.h
queue.o: $(INC)/time.h
queue.o: $(GENERAL)/backup.h
queue.o: $(GENERAL)/bkmsgs.h
rsfile.o: $(INC)/sys/types.h
rsfile.o: $(INC)/sys/stat.h
rsfile.o: $(INC)/stdio.h
rsfile.o: $(INC)/sys/mnttab.h
rsfile.o: $(INC)/errno.h
gethdr.o: $(INC)/time.h
gethdr.o: $(INC)/string.h
gethdr.o: $(INC)/pwd.h
gethdr.o: $(GENERAL)/bktypes.h
gethdr.o: $(GENERAL)/brarc.h
gethdr.o: $(INC)/bkrs.h
gethdr.o: $(GENERAL)/libadmIO.h
