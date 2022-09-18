#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)libadm:common/lib/libadm/libadm.mk	1.2.6.2"
#ident "$Header: libadm.mk 1.9 91/08/02 $"

include $(LIBRULES)

.c.a:;

LIBADM=libadm.a
LINTLIBADM=llib-ladm.ln
OBJECTS=$(SOURCES:.c=.o)

SOURCES= \
	pkginfo.c pkgnmchk.c pkgparam.c \
	getinput.c ckint.c ckitem.c \
	ckpath.c ckrange.c ckstr.c \
	ckyorn.c puterror.c puthelp.c \
	puttext.c ckkeywd.c getvol.c \
	devattr.c putprmpt.c ckgid.c \
	ckdate.c cktime.c ckuid.c \
	dgrpent.c getdev.c \
	devtab.c data.c getdgrp.c \
	listdev.c listdgrp.c regexp.c \
	devreserv.c putdev.c putdgrp.c \
	ddb_dsf.c ddb_gen.c \
	ddb_lib.c ddb_sec.c \
	devalloc.c devdealloc.c \
	space.c

LOCALINC=-I.
LINTFLAGS=$(LOCALINC) -I$(INC) -u -o adm

PKGINFO_FILES=\
	$(LIBADM)(pkginfo.o) $(LIBADM)(pkgnmchk.o) $(LIBADM)(pkgparam.o) 

VALTOOL_FILES=\
	$(LIBADM)(getinput.o) $(LIBADM)(ckint.o) $(LIBADM)(ckitem.o) \
	$(LIBADM)(ckpath.o) $(LIBADM)(ckrange.o) $(LIBADM)(ckstr.o) \
	$(LIBADM)(ckyorn.o) $(LIBADM)(puterror.o) $(LIBADM)(puthelp.o) \
	$(LIBADM)(puttext.o) $(LIBADM)(ckkeywd.o) $(LIBADM)(getvol.o) \
	$(LIBADM)(putprmpt.o) $(LIBADM)(ckgid.o) \
	$(LIBADM)(ckdate.o) $(LIBADM)(cktime.o) $(LIBADM)(ckuid.o) \
	$(LIBADM)(space.o)

DEVMGMT_FILES=\
	$(LIBADM)(dgrpent.o) $(LIBADM)(getdev.o) $(LIBADM)(devattr.o) \
	$(LIBADM)(devtab.o) $(LIBADM)(data.o) $(LIBADM)(getdgrp.o) \
	$(LIBADM)(listdev.o) $(LIBADM)(listdgrp.o) $(LIBADM)(regexp.o) \
	$(LIBADM)(devreserv.o) $(LIBADM)(putdev.o) $(LIBADM)(putdgrp.o) \
	$(LIBADM)(ddb_dsf.o) $(LIBADM)(ddb_gen.o) \
	$(LIBADM)(ddb_lib.o) $(LIBADM)(ddb_sec.o)

DEVALLOC_FILES=\
	$(LIBADM)(devalloc.o) $(LIBADM)(devdealloc.o)

all: $(LIBADM)

$(DEVMGMT_FILES):	$(INC)/devmgmt.h devtab.h
$(DEVALLOC_FILES):	$(INC)/devmgmt.h $(INC)/mac.h
$(PKGINFO_FILES):	$(INC)/pkginfo.h $(INC)/pkgstrct.h
$(VALTOOL_FILES):	$(INC)/valtools.h

.PRECIOUS: $(LIBADM)

# $(LIBADM): $(DEVMGMT_FILES) $(DEVALLOC_FILES) $(PKGINFO_FILES) $(VALTOOL_FILES)

$(LIBADM): $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $?

clean:
	rm -f lint.out $(LINTLIBADM) *.o

clobber: clean
	rm -f $(LIBADM)

strip:

install: all 
	$(INS) -f $(USRLIB) $(LIBADM)
	@if [ -f $(LINTLIBADM) ] ;\
	then \
		$(INS) -f $(USRLIB) $(LINTLIBADM) ;\
	fi

lintit:
	lint $(LINTFLAGS) $(SOURCES) > lint.out 2>&1
