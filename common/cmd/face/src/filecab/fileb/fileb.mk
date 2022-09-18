#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)face:src/filecab/fileb/fileb.mk	1.10.3.5"
#ident "$Header: fileb.mk 1.6 92/01/17 $"

include $(CMDRULES)

LOCALINC= -I../inc
LDLIBS= -lgen

USR=$(ROOT)/$(MACH)/home
VBIN=$(USR)/vmsys/bin
VLIB=$(USR)/vmsys/lib
OABIN = $(USR)/oasys/bin
HEADER1=../inc

DIRS= $(USR) $(USR)/vmsys $(USR)/oasys

LCMDS = services

OCMDS =	termtest identify setenv

VCMDS = face dir_move dir_copy dir_creat chkperm creaserve listserv delserve \
ichexec chexec chkterm basename mnucheck modserv color_chk frame_chk \
nls_expand pathfind col2i col2e

CMDS = $(VCMDS) $(OCMDS)

all:	$(CMDS)

face: face.o
	$(CC) -o $(@) $(@).o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

face.o:	face.c 

chkperm: chkperm.o
	$(CC) -o $(@) $(@).o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

chkperm.o: chkperm.c $(HEADER1)/wish.h

creaserve: creaserve.o
	$(CC) -o $(@) $(@).o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

creaserve.o: creaserve.c $(HEADER1)/wish.h

delserve: delserve.o
	$(CC) -o $(@) $(@).o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

delserve.o: delserve.c $(HEADER1)/wish.h

listserv: listserv.o
	$(CC) -o $(@) $(@).o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

listserv.o: listserv.c $(HEADER1)/wish.h

modserv: modserv.o
	$(CC) -o $(@) $(@).o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

modserv.o: modserv.c $(HEADER1)/wish.h

mnucheck: mnucheck.o
	$(CC) -o $(@) $(@).o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

mnucheck.o: mnucheck.c $(HEADER1)/wish.h


termtest:	termtest.c $(HEADER1)/wish.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

nls_expand: nls_expand.o
	$(CC) -o $@ nls_expand.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

nls_expand.o: nls_expand.c

pathfind: pathfind.o
	$(CC) -o $@ pathfind.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

pathfind.o: pathfind.c

col2e: col2e.o
	$(CC) -o $@ col2e.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

col2e.o: col2e.c

col2i: col2i.o
	$(CC) -o $@ col2i.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

col2i.o: col2i.c

###### Standard Makefile Targets ######

clean: 
	rm -f *.o

clobber:	clean
	rm -f $(VCMDS) $(OCMDS)

lintit:

install: all dirs $(VBIN) $(OABIN) $(VLIB)
	@set +e;\
	> $(VLIB)/.facerc
	$(CH)chmod 600 $(VLIB)/.facerc
	for f in $(LCMDS);\
	do\
		$(INS) -f $(VLIB) -m 0644 $$f ;\
	done;\
	for f in $(VCMDS);\
	do\
		$(INS) -f $(VBIN) -m 0755 $$f ;\
	done;\
	for f in $(OCMDS);\
	do\
		$(INS) -f $(OABIN) -m 0755 $$f ;\
	done;\
	$(CH)chmod 6755 $(VBIN)/chkperm

dirs:	$(DIRS)

$(DIRS):
	-mkdir $@ 
	$(CH)chmod 755 $@

$(VBIN) $(OABIN) $(VLIB):
	-mkdir $@ 
	$(CH)chmod 755 $@

