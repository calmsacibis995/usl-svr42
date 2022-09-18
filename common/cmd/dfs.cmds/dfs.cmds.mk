#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)dfs.cmds:dfs.cmds.mk	1.5.6.2"
#ident "$Header: dfs.cmds.mk 1.2 91/04/05 $"
#
# makefile for dfs.cmds
#
# These are the generic distributed file system administration commands
#

include $(CMDRULES)

COMMANDS=general dfshares share shareall unshareall lidload
GENERAL=unshare
FRC =

all:
	@for i in $(COMMANDS);\
		do cd $$i;\
		echo $$i;\
		make -f $$i.mk $(MAKEARGS);\
		cd ..;\
	done;

install:
	for i in $(COMMANDS);\
		do cd $$i;\
		echo $$i;\
		make install -f $$i.mk $(MAKEARGS);\
		cd ..;\
	done;
	for i in $(GENERAL);\
		do \
			rm -f $(USRSBIN)/$$i;\
			ln $(USRSBIN)/general $(USRSBIN)/$$i;\
		done
	rm -f $(USRSBIN)/dfmounts
	ln $(USRSBIN)/dfshares $(USRSBIN)/dfmounts

clean:
	for i in $(COMMANDS);\
	do\
		cd $$i;\
		echo $$i;\
		make -f $$i.mk clean $(MAKEARGS);\
		cd .. ;\
	done

clobber: clean
	for i in $(COMMANDS);\
		do cd $$i;\
		echo $$i;\
		make -f $$i.mk clobber $(MAKEARGS);\
		cd .. ;\
	done

FRC:
