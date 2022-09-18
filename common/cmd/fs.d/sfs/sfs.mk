#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)sfs.cmds:sfs/sfs.mk	1.1.1.2"
#ident "$Header: sfs.mk 1.2 91/04/11 $"
#  /usr/src/cmd/lib/fs/sfs is the directory of all sfs specific commands
#  whose executable reside in $(INSDIR1) and $(INSDIR2).

include $(CMDRULES)

#
#  This is to build all the sfs commands
#
.DEFAULT:
	@for i in *;\
	do\
	    if [ -d $$i -a -f $$i/$$i.mk ]; \
		then \
		cd  $$i;\
		$(MAKE) -f $$i.mk $(MAKEARGS) $@ ; \
		cd .. ; \
	    fi;\
	done
