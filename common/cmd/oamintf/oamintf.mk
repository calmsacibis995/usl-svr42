#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/oamintf.mk	1.10.7.2"
#ident "$Header: oamintf.mk 2.0 91/07/12 $"

include $(CMDRULES)

DOFIRST=libintf

all install clobber clean size strip lintit:
	@# \
	# Create our own dependency processing - do the  \
	# things in $(DOFIRST) first. \
	# NOTE: the $(DOFIRST) items get done in *reverse* \
	# order \
	# \
	# First create an 'ed' script to move the DOFIRST \
	# things to the top \
	for f in `ls -d $(DOFIRST)` ; \
	do \
		echo "/$$f/m0" ; \
	done > .ed ;\
	echo "w" >>.ed ;\
	echo "q" >>.ed ;\
	# \
	# create a temp file of all makefiles in this subtree \
	# \
	ls */Makefile */makefile */*.mk 2>/dev/null >.tmp || true ;\
	# \
	# use the 'ed' script to move the DOFIRST things to the top \
	# \
	ed -s .tmp < .ed ;\
	# \
	# now, do the rule \
	# NOTE: could use -ef to pass env. var. down if need to \
	for i in `cat .tmp` ;\
	do \
		( \
		echo "cd `dirname $$i` && $(MAKE) -f `basename $$i` $(MAKEARGS) $@" ;\
		cd `dirname $$i` && $(MAKE) -f `basename $$i` $(MAKEARGS) $@ ;\
		) \
	done ;\
	rm -f .ed .tmp
