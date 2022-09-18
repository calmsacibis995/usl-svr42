#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/refer/refer.mk	1.1"
#ident	"$Header: $"
#     Portions Copyright(c) 1988, Sun Microsystems, Inc.
#     All Rights Reserved


#     Makefile for refer

include $(CMDRULES)

OWN = bin

GRP = bin

INSDIR=$(ROOT)/$(MACH)/usr/ucb

INSLIB=$(ROOT)/$(MACH)/usr/ucblib

CMDS = mkey inv hunt refer addbib lookbib sortbib roffbib indxbib

DIR=$(INSLIB)/reftools

DIR1=$(INSLIB)/doctools

all:	$(CMDS)

$(DIR):
	-mkdir $@
	$(CH)-chmod 755 $@
	$(CH)-chgrp bin $@
	$(CH)-chown bin $@

$(DIR1):
	-mkdir $(INSLIB)/doctools
	$(CH)-chmod 755 $(INSLIB)/doctools
	$(CH)-chgrp bin $(INSLIB)/doctools
	$(CH)-chown bin $(INSLIB)/doctools
	-mkdir $(INSLIB)/doctools/tmac
	$(CH)-chmod 755 $(INSLIB)/doctools/tmac
	$(CH)-chgrp bin $(INSLIB)/doctools/tmac
	$(CH)-chown bin $(INSLIB)/doctools/tmac

install: all $(DIR) $(DIR1)
	$(INS) -f $(INSLIB)/reftools -u $(OWN) -g $(GRP) -m 00555 mkey
	$(INS) -f $(INSLIB)/reftools -u $(OWN) -g $(GRP) -m 00555 inv
	$(INS) -f $(INSLIB)/reftools -u $(OWN) -g $(GRP) -m 00555 hunt
	$(INS) -f $(INSDIR) -u $(OWN) -g $(GRP) -m 00555 refer
	$(INS) -f $(INSDIR) -u $(OWN) -g $(GRP) -m 00555 addbib
	$(INS) -f $(INSDIR) -u $(OWN) -g $(GRP) -m 00555 sortbib
	$(INS) -f $(INSDIR) -u $(OWN) -g $(GRP) -m 00555 roffbib
	$(INS) -f $(INSDIR) -u $(OWN) -g $(GRP) -m 00555 indxbib
	$(INS) -f $(INSDIR) -u $(OWN) -g $(GRP) -m 00555 lookbib
	$(INS) -f $(INSLIB)/doctools/tmac -u $(OWN) -g $(GRP) -m 00644 tmac.bib
	cd papers; $(MAKE) -f papers.mk install

mkey:	mkey1.o mkey2.o mkey3.o deliv2.o
	$(CC) mkey?.o deliv2.o -o mkey $(LDFLAGS) $(PERFLIBS)

inv:	inv1.o inv2.o inv3.o inv5.o inv6.o deliv2.o
	$(CC) inv?.o deliv2.o -o inv $(LDFLAGS) $(PERFLIBS)

hunt:	hunt1.o hunt2.o hunt3.o hunt5.o hunt6.o hunt7.o glue5.o refer3.o hunt9.o shell.o deliv2.o hunt8.o glue4.o tick.o
	$(CC) hunt?.o refer3.o glue5.o glue4.o shell.o deliv2.o tick.o -o hunt $(LDFLAGS) $(PERFLIBS)


refer: glue1.o refer1.o refer2.o refer4.o refer5.o refer6.o mkey3.o refer7.o refer8.o hunt2.o hunt3.o deliv2.o hunt5.o hunt6.o hunt8.o glue3.o hunt7.o hunt9.o glue2.o glue4.o glue5.o refer0.o shell.o
	$(CC) glue?.o refer[01245678].o hunt[2356789].o mkey3.o shell.o deliv2.o -o refer $(LDFLAGS) $(PERFLIBS)

addbib: addbib.o
	$(CC) addbib.o -o addbib $(LDFLAGS) $(PERFLIBS)

lookbib: lookbib.o
	$(CC) lookbib.o -o lookbib $(LDFLAGS) $(PERFLIBS)

sortbib: sortbib.o
	$(CC) sortbib.o -o sortbib $(LDFLAGS) $(PERFLIBS)

indxbib: indxbib.sh
	cp indxbib.sh indxbib

roffbib: roffbib.sh
	cp roffbib.sh roffbib

clean:
	rm -f *.o 

clobber:	clean
	rm -f $(CMDS)
