#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)sa:common/cmd/sa/sa.mk	1.6.2.10"
#ident  "$Header: $"

include $(CMDRULES)

OWN = bin
GRP = bin

# how to use this makefile
# to make sure all files  are up to date: $(MAKE) -f sa.mk all
#
# to force recompilation of all files: $(MAKE) -f sa.mk all FRC=FRC
#
# to test new executables before installing in 
# /usr/lib/sa: $(MAKE) -f sa.mk testbi
#
# The sadc and sadp modules must be able to read /dev/kmem,
# which standardly has restricted read permission.
# They must have set-group-ID mode
# and have the same group as /dev/kmem.
# The chmod and chgrp commmands below ensure this.
#
INSDIR = $(USRLIB)/sa
CRON   = $(VAR)/spool/cron
CRONDIR= $(CRON)/crontabs
CRONTAB= $(CRON)/crontabs/sys

ENTRY1= '0 * * * 0-6 $$TFADMIN /usr/lib/sa/sa1'
ENTRY2= '20,40 8-17 * * 1-5 $$TFADMIN /usr/lib/sa/sa1'
ENTRY3= '5 18 * * 1-5 $$TFADMIN /usr/lib/sa/sa2 -s 8:00 -e 18:01 -i 1200 -A'

MAINS = sadc sar sa1 sa2 perf timex sadp 

all: $(MAINS)

sadc:: sadc.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(LIBELF) $(SHLIBS)

sar:: sar.o sarmach.o
	$(CC) -o $@ $@.o sarmach.o $(LDFLAGS) $(LDLIBS) $(LIBELF) $(SHLIBS)

sa2:: sa2.sh
	cp sa2.sh sa2

sa1:: sa1.sh
	cp sa1.sh sa1
	
perf:: perf.sh
	cp perf.sh perf

timex:: timex.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

sadp:: sadp.o 
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(LIBELF) $(SHLIBS)

sar.o: sar.c \
	$(INC)/stdio.h \
	$(INC)/sys/param.h \
	$(INC)/ctype.h \
	$(INC)/time.h \
	$(INC)/priv.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/utsname.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/flock.h \
	$(INC)/sys/fs/rf_acct.h \
	sa.h \
	$(INC)/a.out.h

sadc.o: sadc.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/var.h \
	$(INC)/sys/iobuf.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/elog.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/file.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/vtoc.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/sdi_edt.h \
	$(INC)/sys/altsctr.h \
	$(INC)/sys/scsi.h \
	$(INC)/sys/sdi.h \
	$(INC)/sys/sd01.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/fs/rf_acct.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/flock.h \
	$(INC)/sys/tuneable.h \
	sa.h \
	$(INC)/sys/ksym.h

sadp.o: sadp.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/elog.h \
	$(INC)/sys/ksym.h \
	$(INC)/sys/iobuf.h \
	$(INC)/sys/vtoc.h \
	$(INC)/sys/altsctr.h \
	$(INC)/sys/sdi_edt.h \
	$(INC)/sys/sdi.h \
	$(INC)/sys/scsi.h \
	$(INC)/sys/sd01.h \
	$(INC)/time.h \
	$(INC)/sys/utsname.h \
	$(INC)/sys/var.h \
	$(INC)/ctype.h \
	$(INC)/stdio.h

sarmach.o: sarmach.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/fs/rf_acct.h \
	sa.h \
	$(INC)/a.out.h

timex.o: timex.c \
	$(INC)/sys/types.h \
	$(INC)/sys/times.h \
	$(INC)/sys/param.h \
	$(INC)/stdio.h \
	$(INC)/signal.h \
	$(INC)/errno.h \
	$(INC)/pwd.h

test: testai

testbi: #test for before installing
	sh runtest new $(ROOT)/usr/src/cmd/sa

testai: #test for after install
	sh runtest new

$(INSDIR):
	[ -d $@ ] || mkdir $@; \
		$(CH)chmod 775 $@; \
		$(CH)chown adm $@; \
		$(CH)chgrp bin $@;

$(CRON):
	[ -d $@ ] || mkdir -p $@; \
		$(CH)chmod 700 $@; \
		$(CH)chown root $@; \
		$(CH)chgrp sys $@;

$(CRONDIR): $(CRON)
	[ -d $@ ] || mkdir $@; \
		$(CH)chmod 755 $@; \
		$(CH)chown root $@; \
		$(CH)chgrp sys $@;

$(CRONTAB): $(CRONDIR)
	[ -f $@ ] || > $@;\
		$(CH)chmod 644 $@;\
		$(CH)chown root $@;\
		$(CH)chgrp sys $@;

install: all $(INSDIR) $(CRONTAB)
	if [ -f $(CRONTAB) ];\
	then \
		if grep "sa1" $(CRONTAB) >/dev/null 2>&1 ; then :;\
		else \
			echo $(ENTRY1) >> $(CRONTAB);\
			echo $(ENTRY2) >> $(CRONTAB);\
		fi;\
		if grep "sa2" $(CRONTAB) >/dev/null 2>&1 ; then :;\
		else\
			echo $(ENTRY3) >> $(CRONTAB);\
		fi;\
	fi;
	-rm -f $(USRBIN)/sar
	-rm -f $(USRBIN)/sadp
	-rm -f $(USRBIN)/sar
	-rm -f $(ETC)/rc2.d/S21perf
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) sar 
	$(INS) -f $(INSDIR)  -m 0555 -u $(OWN) -g $(GRP) sa2
	$(INS) -f $(INSDIR)  -m 0555 -u $(OWN) -g $(GRP) sa1
	$(INS) -f $(ETC)/init.d -m 0444 -u root -g sys perf
	$(INS) -f $(USRBIN)  -m 0555 -u $(OWN) -g sys timex
	$(INS) -f $(USRSBIN) -m 02555 -u $(OWN) -g sys sadp 
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) sar 
	$(INS) -o -f $(INSDIR) -m 02555 -u $(OWN) -g sys sadc
	ln $(ETC)/init.d/perf $(ETC)/rc2.d/S21perf
	$(SYMLINK) /usr/sbin/sar $(USRBIN)/sar
	$(SYMLINK) /usr/sbin/sadp $(USRBIN)/sadp

clean:
	-rm -f *.o

clobber: clean
	-rm -f $(MAINS)

lintit:
	$(LINT) $(LINTFLAGS) sar.c sarmach.c
	$(LINT) $(LINTFLAGS) sadc.c
