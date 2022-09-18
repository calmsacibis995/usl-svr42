#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:fs/fdfs/fdfs.mk	1.4"
#ident "$Header: fdfs.mk 1.2 91/03/20 $"

include $(UTSRULES)

KBASE    = ../..
FS	 = $(CONF)/pack.d/fdfs/Driver.o

FILES    = fdops.o

CFILES = $(FILES:.o=.c)



all:	ID $(FS)

$(FS):	$(FILES)
	$(LD) -r -o $@ $(FILES)

#
# Configuration Section
#
ID:
	cd fdfs.cf; $(IDINSTALL) -R$(CONF) -M fdfs


clean:
	-rm -f $(FILES)

clobber:	clean
	-$(IDINSTALL) -e -R$(CONF) -d fdfs

lint:
	lint -u -x $(DEFLIST) $(CFLAGS) \
		fdops.c

headinstall:


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

