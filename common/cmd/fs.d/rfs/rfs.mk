#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)rfs.cmds:rfs/rfs.mk	1.6.5.3"
#ident	"$Header: rfs.mk 1.2 91/04/11 $"
#
# makefile for rfs.cmds
#
# These are the rfs specific subcommands for the generic distributed file
# system administration commands
#

include $(CMDRULES)

COMMANDS=dfmounts dfshares mount share unshare
FRC =

.DEFAULT:
	@for i in $(COMMANDS);\
		do cd $$i;\
		$(MAKE) -f $$i.mk $(MAKEARGS) $@;\
		cd ..;\
	done;

FRC:
