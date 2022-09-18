#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:util/kdb/kdb/kdb.mk	1.5"
#ident	"$Header: $"

include $(UTSRULES)

KBASE = ../../..

OFILE = $(CONF)/pack.d/kdb/Driver.o

FILES = db.o dbcon.o dbintrp.o dblex.o dbshow.o dbshow2.o

CFILES = $(FILES:.o=.c)



all:	ID $(OFILE)

$(OFILE):       $(FILES) $(FRC)
	$(LD) -r -o $@ $(FILES)

ID:
	cd kdb.cf; $(IDINSTALL) -M -R$(CONF) kdb


clean:
	rm -f *.o

clobber:	clean
	$(IDINSTALL) -e -R$(CONF) -d kdb



FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

