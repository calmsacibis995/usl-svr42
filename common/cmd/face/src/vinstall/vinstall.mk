#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#     copyright       "%c%"

#ident	"@(#)face:src/vinstall/vinstall.mk	1.4.4.4"
#ident "$Header: vinstall.mk 2.0 91/07/12 $"
#
# vinstall.mk -- the makefile for the install subsystem of FACE
#

include $(CMDRULES)

USR =	$(ROOT)/$(MACH)/home

LCMDS = vsetup vmodify vdelete addmenu delmenu

VBIN = $(USR)/vmsys/bin

BIN = $(USR)/vmsys $(USR)/vmsys/bin $(USR)/oasys $(USR)/oasys/bin

TMP = $(USR)/oasys/tmp

TERRLOG = $(TMP)/TERRLOG

all:  	$(LCMDS)

install: 	all  $(USR) $(BIN) $(TMP) $(TERRLOG)
		@set +e;\
		for i in $(LCMDS);\
		do\
			$(INS) -f $(VBIN) $$i;\
		done

clean:
	rm -f $(LCMDS)

clobber:	clean

$(USR) $(BIN):
	-mkdir $@ ;\
	$(CH)chmod 755 $@

$(TMP):
	-mkdir $@ ;\
	$(CH)chmod 777 $@

$(TERRLOG):
	> $@ ;\
	$(CH)chmod 622 $@

