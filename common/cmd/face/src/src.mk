#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)face:src/src.mk	1.4.4.3"
#ident "$Header: src.mk 1.3 91/04/18 $"

include $(CMDRULES)

DIRS =	proc xx filecab vinstall oam

all:
	@set -e;\
	for d in $(DIRS);\
	do\
		cd $$d;\
		echo Making $@ in $$d subsystem;\
		$(MAKE) -f $$d.mk $(MAKEARGS) $@;\
		cd ..;\
	done

install: all
	@set -e;\
	for d in $(DIRS);\
	do\
		cd $$d;\
		echo Making $@ in $$d subsystem;\
		$(MAKE) -f $$d.mk $(MAKEARGS) $@;\
		cd ..;\
	done


lintit clobber clean:
	@set -e;\
	for d in $(DIRS);\
	do\
		cd $$d;\
		echo "\nMaking $@ in $$d subsystem\n";\
		$(MAKE) -f $$d.mk $(MAKEARGS) $@;\
		cd ..;\
	done

