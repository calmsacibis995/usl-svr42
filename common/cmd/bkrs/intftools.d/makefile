#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/intftools.d/makefile	1.17.7.2"
#ident "$Header: makefile 1.3 91/05/23 $"

include $(CMDRULES)

BKLIB=../bklib.d/bklib.a
LIBBR=../libbrmeth.d/libbrmeth.a
IOLIB=../libadmIO.d/libadmIO.a

SRC1=tag_exists.c validdays.c validweeks.c bkregvals.c bkregmsgs.c get_rotvals.c rot_msgs.c rsid.c bkid.c validmons.c cron_parse.c cpmsgs.c validdate.c brfindtab.c

SADMDIR=$(USRSADM)
BKBIN=$(SADMDIR)/bkup/bin
OAMBASE=$(SADMDIR)/sysadm
ETCBKUP=$(ETC)/bkup

LOCAL=hdrs
GENERAL=../hdrs
LOCALINC=-I $(LOCAL) -I $(GENERAL)
LDLIBS=$(LDLIBPATH) -lTL -lgen
LINTFLAGS=$(DEFLIST)
OWN=bin
GRP=bin

PSHELLS1=add bkhfull bkhsel bkhtofile bkmadd bkmsg bkregdisp bksadd bkscddisp bkschg bkssel bkup change chgscd chgmsg delscd bkregpath
PSHELLS2=errdays errnewtag erroldtag errweeks full getrpd newtag oldtag rest rsop rsschg rsstat
PSHELLS3=summary valbkids valdays valmeth valmonths valpaths valrpath valrsids valusers valweeks
CCODE=tag_exists validdays validweeks bkregvals get_rotvals rsid bkid validmons cron_parse validdate brfindtab
PRODUCT=$(PSHELLS1) $(PSHELLS2) $(PSHELLS3) $(CCODE)

all: $(PRODUCT)

tag_exists: $(BKLIB) tag_exists.o
	$(CC) $(LDFLAGS) -o $(@) tag_exists.o $(BKLIB) $(LDLIBS) $(SHLIBS)

validdays: $(BKLIB) validdays.o
	$(CC) $(LDFLAGS) -o $(@) validdays.o $(BKLIB) $(LDLIBS) $(SHLIBS)

validdate: $(BKLIB) validdate.o
	$(CC) $(LDFLAGS) -o $(@) validdate.o $(BKLIB) $(LDLIBS) $(SHLIBS)

validweeks: $(BKLIB) validweeks.o
	$(CC) $(LDFLAGS) -o $(@) validweeks.o $(BKLIB) $(LDLIBS) $(SHLIBS)

validmons: $(BKLIB) validmons.o
	$(CC) $(LDFLAGS) -o $(@) validmons.o $(BKLIB) $(LDLIBS) $(SHLIBS)

bkregvals: $(BKLIB) bkregvals.o bkregmsgs.o
	$(CC) $(LDFLAGS) -o $(@) bkregvals.o bkregmsgs.o $(BKLIB) $(LDLIBS) \
		$(SHLIBS)

cron_parse: $(BKLIB) cron_parse.o cpmsgs.o $(BKLIB) $(LIBBR)
	$(CC) $(LDFLAGS) -o $(@) cron_parse.o cpmsgs.o $(BKLIB) $(LIBBR) \
		$(LDLIBS) $(SHLIBS)

get_rotvals: $(BKLIB) get_rotvals.o rot_msgs.o
	$(CC) $(LDFLAGS) -o $(@) get_rotvals.o rot_msgs.o $(BKLIB) $(LDLIBS) \
		$(SHLIBS)

rsid: $(BKLIB) rsid.o
	$(CC) $(LDFLAGS) -o $(@) rsid.o $(BKLIB) $(LDLIBS) $(SHLIBS)

bkid: $(BKLIB) bkid.o
	$(CC) $(LDFLAGS) -o $(@) bkid.o $(BKLIB) $(LDLIBS) $(SHLIBS)

brfindtab: $(BKLIB) $(IOLIB) $(LIBBR) brfindtab.o
	$(CC) $(LDFLAGS) -o $(@) brfindtab.o $(BKLIB) $(LIBBR) $(IOLIB) \
		$(LDLIBS) $(SHLIBS)

.sh:
	cp $*.sh $*; chmod 755 $(@)
touch: 
	touch $*.c

clean:
	rm -f *.o

clobber: clean
	rm -f $(PRODUCT)

lintit:
	$(LINT) $(LINTFLAGS) $(SRC1)

strip:	$(CCODE)
	$(STRIP) $(CCODE)

install: $(PRODUCT)
	for f in $(PRODUCT) ; \
	do \
		$(INS) -f $(BKBIN) -u $(OWN) -g $(GRP) -m 0755 $$f; \
	done

bkid.o: $(INC)/stdio.h

bkregvals.o: $(INC)/stdio.h
bkregvals.o: $(INC)/fcntl.h
bkregvals.o: $(INC)/string.h
bkregvals.o: $(INC)/table.h
bkregvals.o: $(GENERAL)/bkreg.h
bkregvals.o: $(LOCAL)/bkregerrs.h

cron_parse.o: $(INC)/stdio.h
cron_parse.o: $(INC)/fcntl.h
cron_parse.o: $(INC)/string.h
cron_parse.o: $(GENERAL)/bktypes.h
cron_parse.o: $(LOCAL)/cperrs.h

get_rotvals.o: $(INC)/stdio.h
get_rotvals.o: $(INC)/fcntl.h
get_rotvals.o: $(INC)/errno.h
get_rotvals.o: $(INC)/sys/types.h
get_rotvals.o: $(INC)/sys/stat.h
get_rotvals.o: $(INC)/table.h
get_rotvals.o: $(GENERAL)/bkreg.h
get_rotvals.o: $(LOCAL)/rot_errs.h
get_rotvals.o: $(GENERAL)/bkerrors.h

rconvdate.o: $(INC)/string.h
rconvdate.o: $(INC)/sys/types.h
rconvdate.o: $(INC)/stdio.h

rsid.o: $(INC)/stdio.h

tag_exists.o: $(INC)/string.h
tag_exists.o: $(INC)/sys/types.h
tag_exists.o: $(INC)/fcntl.h
tag_exists.o: $(INC)/table.h
tag_exists.o: $(GENERAL)/bkreg.h

validdate.o: $(INC)/string.h
validdate.o: $(INC)/sys/types.h
validdate.o: $(INC)/stdio.h
