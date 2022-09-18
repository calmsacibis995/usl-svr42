#ident	"@(#)ucb:common/ucbcmd/troff/troff.d/tmac.d/tmac.mk	1.4"
#ident	"$Header: $"
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.




#		PROPRIETARY NOTICE (Combined)
#
#This source code is unpublished proprietary information
#constituting, or derived under license from AT&T's UNIX(r) System V.
#In addition, portions of such source code were derived from Berkeley
#4.3 BSD under license from the Regents of the University of
#California.
#
#
#
#		Copyright Notice 
#
#Notice of copyright on this source code product does not indicate 
#publication.
#
#	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
#	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#	          All rights reserved.

include $(CMDRULES)

INSDIR= $(ROOT)/$(MACH)/usr/ucblib

OWN = bin

GRP = bin

# Files which live in the current directory and are copied to the destination.
#
FILES=	acm.me an bib chars.me deltext.me e eqn.me \
	exp.tbl float.me footnote.me \
	index.me local.me m mmn mmt ms.acc ms.cov ms.eqn ms.ref \
	ms.tbl ms.ths ms.toc null.me refer.me s sh.me tbl.me \
	thesis.me v

all:	${FILES}

install:
	if [ ! -d $(INSDIR)/doctools/tmac ]; then \
		mkdir $(INSDIR)/doctools/tmac; \
		$(CH)chmod 755 $(INSDIR)/doctools/tmac; \
		$(CH)chown $(OWN) $(INSDIR)/doctools/tmac; \
		$(CH)chgrp $(GRP) $(INSDIR)/doctools/tmac; \
	fi
	for i in ${FILES}; do \
   	    ($(INS) -f $(INSDIR)/doctools/tmac -u $(OWN) -g $(GRP) -m 644 $$i); done
