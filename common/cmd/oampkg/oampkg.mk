#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)oampkg:common/cmd/oampkg/oampkg.mk	1.5.5.4"
#ident "$Header: oampkg.mk 1.3 91/04/18 $"

include $(CMDRULES)

DIRS=\
	libinst pkgadd pkgaudit pkginstall pkgrm pkgremove \
	pkginfo pkgproto pkgchk pkgmk pkgscripts \
	installf pkgtrans setsizecvt

all clobber install clean strip lintit:
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) $(MAKEARGS) $@" ;\
		if cd $$i ;\
		then \
			$(MAKE) $(MAKEARGS) $@ ;\
		 	cd .. ;\
		fi ;\
	done

