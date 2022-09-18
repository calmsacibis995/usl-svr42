#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/usermgmt/Makefile	1.4.15.2"
#ident "$Header: Makefile 2.0 91/07/12 $"

include $(CMDRULES)

LDLIBS = -lia

MAINS=\
	count getmaxuid lsgrp

O_SHFILES=\
	grpchk pswdchk pswddef listuser getdefl lvlget \
	addgrp chk_logins chk_sgrp getval giddflt checkage \
	uniq_gid uniq_uid uniq_gnam uniq_lnam chk_pgrp modgrp

O_DFILES=\
	user.menu \
	Form.defaults Text.dfltnok Text.dfltok \
	Form.password Menu.passwd Text.pswdok Text.pswdnok \
	Form.list Form.lsusr Text.lsgrp Text.lsusr Text.nolsusr \
	Form.add Form.addgrp Form.addusr Form.addpswd Text.agrpnok Text.agrpok \
	Form.audit Form.mac1 Form.modaud1 Form.modaud2 Form.modmac1 \
	Form.macadd Form.macrem Form.macrep \
	Form.modmac2 Text.badprim Text.badsup Menu.mlogins Menu.pgrp Menu.sgrp \
	Form.modgrp1 Form.modgrp2 Form.modify Form.modusr1 Form.modusr2 \
	Text.mgrpnok Text.mgrpok Text.musrnok Text.musrok \
	Text.rgrpnok Menu.logins Menu.mlogins Menu.pgrp Menu.sgrp \
	Form.remove Form.rmgrp Form.rmusr1 Form.rmusr2 Text.rmgrp \
	Text.rgrpok Text.rusrnok Text.rusrok Menu.logins Menu.pgrp \
	Text.ausrnok Text.apswdok Text.apswdnok Menu.zlogins \
	Menu.evts Menu.lvls Menu.mevts Menu.mlvls Text.rctabnok Text.rctabok

HELPFILES=\
	Help add.Help defaults.Help list.Help modify.Help password.Help \
	remove.Help

HELPDEST=\
	usermgmt/Help add/Help defaults/Help list/Help modify/Help \
	password/Help remove/Help

.MUTEX:	all install

all: $(O_SHFILES) $(MAINS)

lsgrp:  lsgrp.o $(INC)/stdio.h $(INC)/pwd.h $(INC)/grp.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

getmaxuid: getmaxuid.o $(INC)/sys/param.h $(INC)/limits.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

count: count.o $(INC)/stdio.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

clean:
	rm -f lint.out *.o

clobber: clean
	rm -f $(O_SHFILES) $(MAINS)

lintit:
	rm -f lint.out
	for file in $(MAINS) ;\
	do \
		$(LINT) $(LINTFLAGS) $(DEFLIST) $$file.c $(LINTLIBS) >> lint.out ;\
	done

size strip: all
	:

install: all xinstall

PROTO=Prototype
OBJECTS=$(HELPFILES) $(MAINS) $(O_SHFILES) $(O_DFILES)

OBJDEST=$(HELPDEST)
include ../Install
