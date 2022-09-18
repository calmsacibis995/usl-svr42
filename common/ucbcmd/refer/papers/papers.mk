#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/refer/papers/papers.mk	1.1"
#ident	"$Header: $"
#     Portions Copyright(c) 1988, Sun Microsystems, Inc.
#     All Rights Reserved


#     Makefile for refer/papers

include $(CMDRULES)

DESTDIR = $(ROOT)/$(MACH)/usr/ucblib/reftools

INSDIR = $(DESTDIR)/papers

PAPERS= Rbstjissue runinv Rv7man

all: $(PAPERS)

$(INSDIR):
	-mkdir $@
	$(CH)-chmod 755 $@
	$(CH)-chgrp bin $@
	$(CH)-chown bin $@

install: all $(INSDIR)
	for i in $(PAPERS); do \
		($(INS) -f $(INSDIR) -m 644 $$i); done
	cd $(INSDIR); chmod 755 runinv;
