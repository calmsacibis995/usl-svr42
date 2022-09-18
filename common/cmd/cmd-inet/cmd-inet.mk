#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/cmd-inet.mk	1.10.10.1"
#ident	"$Header: $"
# 
# 
# 		Copyright Notice 
# 
# Notice of copyright on this source code product does not indicate 
# publication.
# 
# 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
#

include $(CMDRULES)

LOCALDEF=	-DSYSV -DSTRNET -DBSD_COMP

YFLAGS = -d

DIRS=		etc usr.bin usr.sbin

all:
	@for i in $(DIRS);\
	do\
		cd $$i;\
		/bin/echo "\n===== $(MAKE) -f $$i.mk all";\
		$(MAKE) -f $$i.mk all $(MAKEARGS);\
		cd ..;\
	done;\
	wait

install:
	@for i in $(DIRS);\
	do\
		cd $$i;\
		/bin/echo "\n===== $(MAKE) -f $$i.mk install";\
		$(MAKE) -f $$i.mk install $(MAKEARGS);\
		cd ..;\
	done;\
	wait

clean lintit:
	@for i in $(DIRS);\
	do\
		cd $$i;\
		/bin/echo "\n===== $(MAKE) -f $$i.mk $@";\
		$(MAKE) -f $$i.mk $@;\
		cd ..;\
	done;\
	wait

clobber: clean
	@for i in $(DIRS);\
	do\
		cd $$i;\
		/bin/echo "\n===== $(MAKE) -f $$i.mk clobber";\
		$(MAKE) -f $$i.mk clobber;\
		cd ..;\
	done;\
	wait
