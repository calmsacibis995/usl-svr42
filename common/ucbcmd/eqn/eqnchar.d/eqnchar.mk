#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/eqn/eqnchar.d/eqnchar.mk	1.1"
#ident	"$Header: $"
#     Portions Copyright(c) 1988, Sun Microsystems, Inc.
#     All Rights Reserved


include $(CMDRULES)

INSDIR= $(ROOT)/$(MACH)/usr/ucblib

OWN = bin

GRP = bin

# Files which live in the current directory are copied to the destination.
#
FILES=	eqnchar 

all:	${FILES}

install:
	-mkdir $(INSDIR)/pub
	$(CH)-chmod 755 $(INSDIR)/pub
	$(CH)-chgrp $(GRP) $(INSDIR)/pub
	$(CH)-chown $(OWN) $(INSDIR)/pub
	$(INS) -f $(INSDIR)/pub -u $(OWN) -g $(GRP) -m 644 $(FILES)

clean:
