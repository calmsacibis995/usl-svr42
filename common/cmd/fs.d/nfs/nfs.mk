#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)nfs.cmds:nfs/nfs.mk	1.9.9.3"
#ident	"$Header: nfs.mk 1.2 91/04/11 $"
#
# makefile for nfs.cmds
#
# These are the nfs specific subcommands for the generic distributed file
# system administration commands, along with many other nfs-specific
# administrative commands
#

include $(CMDRULES)

COMMANDS=automount biod bootpd dfmounts dfshares exportfs mount mountd nfsd share showmount umount unshare statd lockd nfsstat pcnfsd nfsping

.DEFAULT:
	@for i in $(COMMANDS);\
		do cd $$i;\
		echo "====> $(MAKE) -f $$i.mk $@" ;\
		$(MAKE) -f $$i.mk $(MAKEARGS) $@;\
		cd ..;\
	done;
