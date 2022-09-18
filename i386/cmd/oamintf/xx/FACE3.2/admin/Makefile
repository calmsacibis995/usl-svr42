#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/admin/Makefile	1.3"
#ident	"$Header: $"

include $(CMDRULES)

SABIN1=$(ROOT)/$(MACH)/usr/sysadm/bin
SABIN2=$(USRSADM)/sysadm/bin
SABIN3=$(USRSADM)/sysadm/admsets

BINARY=menu_add
PRODUCTS=$(BINARY) .useradm base-adm
GRP=bin
OWN=bin


all: $(PRODUCTS)
	
$(BINARY):
	@echo "***** Making FACE admin files/cmds"
	$(CC) $(CFLAGS) $(DEFLIST) $(@).c -o $(@) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

.useradm:
	grep -v "^#ident	" useradm > .useradm

base-adm:
	grep -v "^#ident	" Base-adm > base-adm

install: all
	-[ -d $(SABIN1) ] || mkdir -p $(SABIN1)
	-[ -d $(SABIN2) ] || mkdir -p $(SABIN2)
	-[ -d $(SABIN3) ] || mkdir -p $(SABIN3)
	-[ -d $(ETC) ] || mkdir -p $(ETC)
	-[ -d $(USRBIN) ] || mkdir -p $(USRBIN)
	-[ -d $(USRSBIN) ] || mkdir -p $(USRSBIN)
	$(INS) -m 755 -g $(GRP) -u $(OWN) -f $(SABIN1) base-adm
	$(INS) -m 755 -g $(GRP) -u $(OWN) -f $(ETC) .useradm
	$(INS) -m 755 -g $(GRP) -u $(OWN) -f $(USRBIN) menu_add
	-rm -f $(SABIN2)/base-adm
	-ln $(SABIN1)/base-adm $(SABIN2)/base-adm
	-rm -f $(SABIN3)/base-adm
	-ln $(SABIN1)/base-adm $(SABIN3)/base-adm
	-rm -f $(USRBIN)/menu_del
	-ln $(USRBIN)/menu_add $(USRBIN)/menu_del
	-rm -f $(USRSBIN)/menu_add
	-ln $(USRBIN)/menu_add $(USRSBIN)/menu_add
	-rm -f $(USRSBIN)/menu_del
	-ln $(USRBIN)/menu_add $(USRSBIN)/menu_del

clean:
	@echo "***** Cleaning FACE admin files/cmds"
	rm -f *.o lint.out

clobber: clean
	@echo "***** Clobbering FACE admin files/cmds"
	rm -f $(PRODUCTS)

strip:
	$(STRIP) $(BINARY)

size:
	$(SIZE) $(BINARY)

lintit:
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(BINARY).c $(LINTLIBS) >> lint.out
