#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.



#ident	"@(#)adm:i386/cmd/.adm/.adm.mk	1.1.2.9"
#ident	"$Header: $"

include $(CMDRULES)

CRONTABS = $(VAR)/spool/cron/crontabs
LIBCRON = $(ETC)/cron.d
INSDIR = $(ETC)
INSDIR1 = $(SBIN)
INSDIR2 = $(USRSBIN)
TOUCH = touch

CRON_ENT= adm root sys

CRON_LIB= .proto at.allow at.deny cron.allow cron.deny queuedefs

ETC_SCRIPTS= bupsched checklist cleanup datemsk dfspace \
	_sactab _sysconfig ttydefs ttysrch group ioctl.syscon issue motd \
	passwd profile stdprofile shadow TIMEZONE cshrc ttytype gettydefs

OTHER= spellhist ascii adduser deluser

DIRS= \
	$(VAR)/spool \
	$(VAR)/spool/cron \
	$(VAR)/spool/cron/crontabs \
	$(VAR)/spool/cron/atjobs \
	$(ETC)/cron.d \
	$(ETC)/rc0.d \
	$(ETC)/rc1.d \
	$(ETC)/rc2.d \
	$(ETC)/rc3.d \
	$(INSDIR)/init.d \
	$(ROOT)/$(MACH)/usr/share \
	$(ROOT)/$(MACH)/usr/share/lib \
	$(VAR)/news \
	$(ETC)/saf \
	$(VAR)/saf \
	$(USRLIB)/spell \
	$(USRSBIN) \
	$(SBIN)

all:

crontab: $(CRON_ENT)

cronlib: $(CRON_LIB)

etc_scripts: $(ETC_SCRIPTS)

other: $(OTHER)

clean:

clobber: clean

install: dirs etc_scripts crontab cronlib other

dirs: $(DIRS)

$(DIRS):
	-mkdir -p $@

adm::
	cp adm $(CRONTABS)/adm
	$(CH)chmod 644 $(CRONTABS)/adm
	$(CH)chgrp sys $(CRONTABS)/adm
	$(TOUCH) 0101000070 $(CRONTABS)/adm
	$(CH)chown root $(CRONTABS)/adm

root::
	cp root $(CRONTABS)/root
	$(CH)chmod 644 $(CRONTABS)/root
	$(CH)chgrp sys $(CRONTABS)/root
	$(TOUCH) 0101000070 $(CRONTABS)/root
	$(CH)chown root $(CRONTABS)/root

sys::
	cp sys $(CRONTABS)/sys
	$(CH)chmod 644 $(CRONTABS)/sys
	$(CH)chgrp sys $(CRONTABS)/sys
	$(TOUCH) 0101000070 $(CRONTABS)/sys
	$(CH)chown root $(CRONTABS)/sys

.proto::
	cp .proto $(LIBCRON)/.proto
	$(CH)chmod 744 $(LIBCRON)/.proto
	$(CH)chgrp sys $(LIBCRON)/.proto
	$(TOUCH) 0101000070 $(LIBCRON)/.proto
	$(CH)chown root $(LIBCRON)/.proto

ascii::
	cp ascii $(ROOT)/$(MACH)/usr/share/lib/ascii
	$(CH)chmod 644 $(ROOT)/$(MACH)/usr/share/lib/ascii
	$(CH)chgrp bin $(ROOT)/$(MACH)/usr/share/lib/ascii
	$(CH)chown bin $(ROOT)/$(MACH)/usr/share/lib/ascii

at.allow::
	cp at.allow $(LIBCRON)/at.allow
	$(CH)chmod 644 $(LIBCRON)/at.allow
	$(CH)chgrp sys $(LIBCRON)/at.allow
	$(TOUCH) 0101000070 $(LIBCRON)/at.allow
	$(CH)chown root $(LIBCRON)/at.allow

at.deny::
	if [ ! -f $@ ] ; then touch $@; fi
	cp at.deny $(LIBCRON)/at.deny
	$(CH)chmod 644 $(LIBCRON)/at.deny
	$(CH)chgrp sys $(LIBCRON)/at.deny
	$(TOUCH) 0101000070 $(LIBCRON)/at.deny
	$(CH)chown root $(LIBCRON)/at.deny

cron.allow::
	cp cron.allow $(LIBCRON)/cron.allow
	$(CH)chmod 644 $(LIBCRON)/cron.allow
	$(CH)chgrp sys $(LIBCRON)/cron.allow
	$(TOUCH) 0101000070 $(LIBCRON)/cron.allow
	$(CH)chown root $(LIBCRON)/cron.allow

cron.deny::
	if [ ! -f $@ ] ; then touch $@; fi
	cp cron.deny $(LIBCRON)/cron.deny
	$(CH)chmod 644 $(LIBCRON)/cron.deny
	$(CH)chgrp sys $(LIBCRON)/cron.deny
	$(TOUCH) 0101000070 $(LIBCRON)/cron.deny
	$(CH)chown root $(LIBCRON)/cron.deny

queuedefs::
	cp queuedefs $(LIBCRON)/queuedefs
	$(CH)chmod 644 $(LIBCRON)/queuedefs
	$(CH)chgrp sys $(LIBCRON)/queuedefs
	$(TOUCH) 0101000070 $(LIBCRON)/queuedefs
	$(CH)chown root $(LIBCRON)/queuedefs

bupsched::
	$(INS) -f $(ETC) -m 0644 -u root -g sys bupsched

checklist::
	cp checklist $(INSDIR)/checklist
	$(CH)chmod 664 $(INSDIR)/checklist
	$(CH)chgrp sys $(INSDIR)/checklist
	$(TOUCH) 0101000070 $(INSDIR)/checklist
	$(CH)chown root $(INSDIR)/checklist

cleanup::
	cp cleanup $(INSDIR1)/cleanup 
	cp cleanup $(INSDIR2)/cleanup
	$(CH)chmod 744 $(INSDIR1)/cleanup $(INSDIR2)/cleanup
	$(CH)chgrp sys $(INSDIR1)/cleanup $(INSDIR2)/cleanup
	$(CH)chown root $(INSDIR1)/cleanup $(INSDIR2)/cleanup

datemsk::
	$(INS) -f $(ETC) -m 0444 -u root -g sys datemsk
	
cshrc::
	cp cshrc $(INSDIR)/cshrc
	$(CH)chmod 644 $(INSDIR)/cshrc
	$(CH)chgrp sys $(INSDIR)/cshrc
	$(CH)chown root $(INSDIR)/cshrc

dfspace::
	cp dfspace $(INSDIR1)/dfspace 
	cp dfspace $(INSDIR2)/dfspace
	$(CH)chmod 755 $(INSDIR1)/dfspace $(INSDIR2)/dfspace
	$(CH)chgrp sys $(INSDIR1)/dfspace $(INSDIR2)/dfspace
	$(CH)chown root $(INSDIR1)/dfspace $(INSDIR2)/dfspace

_sactab::
	cp _sactab $(ETC)/saf
	$(CH)chmod 444 $(ETC)/saf/_sactab
	$(CH)chgrp other $(ETC)/saf/_sactab
	$(TOUCH) 0101000070 $(ETC)/saf/_sactab
	$(CH)chown root $(ETC)/saf/_sactab
	
_sysconfig::
	cp _sysconfig $(ETC)/saf
	$(CH)chmod 444 $(ETC)/saf/_sysconfig
	$(CH)chgrp other $(ETC)/saf/_sysconfig
	$(TOUCH) 0101000070 $(ETC)/saf/_sysconfig
	$(CH)chown root $(ETC)/saf/_sysconfig
	
ttydefs::
	cp ttydefs $(INSDIR)/ttydefs
	$(CH)chmod 664 $(INSDIR)/ttydefs
	$(CH)chgrp sys $(INSDIR)/ttydefs
	$(TOUCH) 0101000070 $(INSDIR)/ttydefs
	$(CH)chown root $(INSDIR)/ttydefs

ttysrch::
	$(INS) -f $(ETC) -m 0644 -u root -g sys ttysrch
	
group::
	cp group $(INSDIR)/group
	$(CH)chmod 644 $(INSDIR)/group
	$(CH)chgrp sys $(INSDIR)/group
	$(TOUCH) 0101000070 $(INSDIR)/group
	$(CH)chown root $(INSDIR)/group

ioctl.syscon::
	cp ioctl.syscon $(INSDIR)/ioctl.syscon
	$(CH)chmod 644 $(INSDIR)/ioctl.syscon
	$(CH)chgrp sys $(INSDIR)/ioctl.syscon
	$(TOUCH) 0101000070 $(INSDIR)/ioctl.syscon
	$(CH)chown root $(INSDIR)/ioctl.syscon

issue::
	    cp issue $(INSDIR)/issue
	    $(CH)chmod 744 $(INSDIR)/issue
	    $(CH)chgrp sys $(INSDIR)/issue
	    $(CH)chown root $(INSDIR)/issue

motd::
	sed 1d motd > $(INSDIR)/motd
	$(CH)chmod 644 $(INSDIR)/motd
	$(CH)chgrp sys $(INSDIR)/motd
	$(CH)chown root $(INSDIR)/motd

passwd::
	cp passwd $(INSDIR)/passwd
	$(CH)chmod 444 $(INSDIR)/passwd
	$(CH)chgrp sys $(INSDIR)/passwd
	$(TOUCH) 0101000070 $(INSDIR)/passwd
	$(CH)chown root $(INSDIR)/passwd

shadow::
	cp shadow $(INSDIR)/shadow
	$(CH)chmod 444 $(INSDIR)/shadow
	$(CH)chgrp sys $(INSDIR)/shadow
	$(TOUCH) 0101000070 $(INSDIR)/shadow
	$(CH)chown root $(INSDIR)/shadow

profile::
	    cp profile $(INSDIR)/profile;\
	    $(CH)chmod 644 $(INSDIR)/profile;\
	    $(CH)chgrp sys $(INSDIR)/profile;\
	    $(CH)chown root $(INSDIR)/profile

ttytype::
	cp ttytype $(INSDIR)/ttytype
	$(CH)chmod 644 $(INSDIR)/ttytype
	$(CH)chgrp sys $(INSDIR)/ttytype
	$(CH)chown root $(INSDIR)/ttytype

gettydefs::
	cp gettydefs $(INSDIR)/gettydefs
	$(CH)chmod 644 $(INSDIR)/gettydefs
	$(CH)chgrp sys $(INSDIR)/gettydefs
	$(CH)chown root $(INSDIR)/gettydefs

stdprofile::
	cp stdprofile $(INSDIR)/stdprofile;
	$(CH)chmod 644 $(INSDIR)/stdprofile
	$(CH)chgrp sys $(INSDIR)/stdprofile
	$(CH)chown root $(INSDIR)/stdprofile

TIMEZONE::
	    cp TIMEZONE $(INSDIR)/TIMEZONE
	    $(CH)chmod 744 $(INSDIR)/TIMEZONE
	    $(CH)chgrp sys $(INSDIR)/TIMEZONE
	    $(CH)chown root $(INSDIR)/TIMEZONE

spellhist::
	if [ ! -f $@ ] ; then touch $@; fi
	cp $@ $(USRLIB)/spell/$@
	$(CH)chmod 666 $(USRLIB)/spell/$@
	$(CH)chgrp sys $(USRLIB)/spell/$@
	$(CH)chown root $(USRLIB)/spell/$@

adduser:
	cp $@.sh        $(SBIN)/$@
	$(CH)chmod 755  $(SBIN)/$@
	$(CH)chgrp sys  $(SBIN)/$@
	$(CH)chown root $(SBIN)/$@

deluser:
	cp $@.sh        $(SBIN)/$@
	$(CH)chmod 755  $(SBIN)/$@
	$(CH)chgrp sys  $(SBIN)/$@
	$(CH)chown root $(SBIN)/$@
