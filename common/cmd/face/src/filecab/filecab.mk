#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)face:src/filecab/filecab.mk	1.5.4.4"
#ident "$Header: filecab.mk 1.3 91/04/18 $"

include $(CMDRULES)

LOCALINC= -I../inc
LDLIBS= -lgen

USR=$(ROOT)/$(MACH)/home
DIRS =	fileb
VHOME=$(USR)/vmsys
OHOME=$(USR)/oasys
ROOTDIRS=$(USR) $(VHOME) $(OHOME)
STANDARD=$(VHOME)/standard
VDIRS= $(VHOME)/standard  $(VHOME)/standard/WASTEBASKET $(VHOME)/standard/pref
ODIRS= $(OHOME)/info $(OHOME)/info/OH $(OHOME)/info/OH/externals $(OHOME)/tmp
TERRLOG = $(OHOME)/tmp/TERRLOG
SFILES= .faceprofile
WFILES=WASTEBASKET/.pref 
PFILES=pref/.environ pref/.variables pref/.colorpref
OAFILES=allobjs detect.tab
INFILES=pathalias

all:
	@set -e;\
	for d in $(DIRS);\
	do\
		cd $$d;\
		echo Making $@ in filecab/$$d subsystem;\
		$(MAKE) -f $$d.mk $(MAKEARGS) $@;\
		cd ..;\
	done

install:	all rootdirs vdirs odirs
		@set -e;\
		for d in $(DIRS);\
		do\
			cd $$d;\
			echo Making $@ in filecab/$$d subsystem;\
			$(MAKE) -f $$d.mk $(MAKEARGS) $@;\
			cd ..;\
		done

		for i in $(SFILES);\
		do\
			$(INS) -f $(STANDARD) -m 0664 standard/$$i;\
		done

		for i in $(WFILES);\
		do\
			$(INS) -f $(STANDARD)/WASTEBASKET -m 0664 standard/$$i;\
		done

		for i in $(PFILES);\
		do\
			$(INS) -f $(STANDARD)/pref -m 0664 standard/$$i;\
		done

		for i in $(INFILES);\
		do\
			$(INS) -f $(VHOME) -m 0664 $$i;\
		done

		for i in $(OAFILES);\
		do\
			$(INS) -f $(OHOME)/info/OH/externals -m 0664 oasys/info/OH/externals/$$i;\
		done


clean lintit:
	@set -e;\
	for d in $(DIRS);\
	do\
		cd $$d;\
		echo "\nMaking $@ in $$d subsystem\n";\
		$(MAKE) -f $$d.mk $(MAKEARGS) $@;\
		cd ..;\
	done

clobber: clean
	@set -e;\
	for d in $(DIRS);\
	do\
		cd $$d;\
		echo "\nMaking $@ in $$d subsystem\n";\
		$(MAKE) -f $$d.mk $(MAKEARGS) $@;\
		cd ..;\
	done

rootdirs: $(ROOTDIRS)

vdirs: 	$(VDIRS)

odirs:  $(ODIRS) $(TERRLOG)

$(ROOTDIRS) $(VDIRS) $(ODIRS):
		-mkdir $@
		$(CH)chmod 775 $@

$(TERRLOG):
	> $@ ;\
	$(CH)chmod 622 $@
