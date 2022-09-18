#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lorder:i386/makefile	1.1.1.7"
#
#	i386 LORDER MAKEFILE
#
include $(CMDRULES)
#
SED=/bin/sed
SEDFLAGS=-e
SED_CMD=$(SED) $(SEDFLAGS)
#
#	Internal Directories
#
COMLORDER= $(SGSBASE)/lorder/common
#
all:		lorder
#
lorder:		$(COMLORDER)/lorder
		-rm -f lorder
		$(SED_CMD) "s/SGS/$(SGS)/" < $(COMLORDER)/lorder > lorder
#
install:	$(CCSBIN)/$(SGS)lorder
#
$(CCSBIN)/$(SGS)lorder:	lorder
		sh $(SGSBASE)/sgs.install 755 $(OWN) $(GRP) $(CCSBIN)/$(SGS)lorder lorder
#
save:		$(CCSBIN)/$(SGS)lorder
		-rm -f $(CCSBIN)/$(SGS)lorder.back
		cp $(CCSBIN)/$(SGS)lorder $(CCSBIN)/$(SGS)lorder.back
#
uninstall:	$(CCSBIN)/$(SGS)lorder.back
		-rm -f $(CCSBIN)/$(SGS)lorder
		cp $(CCSBIN)/$(SGS)lorder.back $(CCSBIN)/$(SGS)lorder
#
clobber:		
		-rm -f lorder
