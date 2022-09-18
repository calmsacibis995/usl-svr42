#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/lp/lp.mk	1.5"
#ident	"$Header: $"

include $(CMDRULES)

include ./common.mk


DEBUG	=


CMDS	= \
		lpc \
		lpq \
		lpr \
		lprm \
		lptest

UCBTOP	=	../../ucbcmd/lp

install:	allLp install2 strip
all:	allLp stripLp all2

allLp:
	cd $(TOP); \
	$(MAKE) -f lp.mk DEBUG="$(DEBUG)" all; 

stripLp:
	cd $(TOP); \
	$(MAKE) -f lp.mk DEBUG="$(DEBUG)" strip

all2:
	cd $(UCBTOP); \
	for d in $(CMDS); \
	do \
		cd $$d; \
		$(MAKE) DEBUG="$(DEBUG)" all; \
		cd ..; \
	done
install2:
	cd $(UCBTOP); \
	for d in $(CMDS); \
	do \
		cd $$d; \
		$(MAKE) DEBUG="$(DEBUG)" install; \
		cd ..; \
	done

clean clobber strip:
	cd $(TOP); \
	$(MAKE) -f lp.mk DEBUG="$(DEBUG)" $@; \
	cd $(UCBTOP); \
	for d in $(CMDS); \
	do \
		cd $$d; \
		if [ -n "$(STRIP)" ]; \
		then \
			$(MAKE) STRIP=$(STRIP) DEBUG="$(DEBUG)" $@; \
		else \
			$(MAKE) STRIP=strip DEBUG="$(DEBUG)" $@; \
		fi;\
		cd ..; \
	done

