#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86at:mem/mem.mk	1.15"
#ident	"$Header: $"

include $(UTSRULES)

KBASE = ..
FILES = \
	copy.o \
	getcpages.o \
	memprobe.o \
	move.o \
	rdma.o \
	sched.o \
	seg_dev.o \
	seg_dummy.o \
	seg_kmem.o \
	seg_map.o \
	seg_objs.o \
	seg_u.o \
	seg_vn.o \
	seg_vpix.o \
	ucopy.o \
	vm_anon.o \
	vm_as.o \
	vm_hat.o \
	vm_machdep.o \
	vm_meter.o \
	vm_page.o \
	vm_pageout.o \
	vm_pvn.o \
	vm_rm.o \
	vm_seg.o \
	vm_subr.o \
	vm_swap.o \
	vm_vpage.o \
	vm_mapfile.o \
	vcopy.o

CFILES =  \
	getcpages.c  \
	kma.c \
	kmacct.c \
	memprobe.c  \
	move.c  \
	rdma.c  \
	sched.c  \
	seg_dev.c  \
	seg_dummy.c  \
	seg_kmem.c  \
	seg_map.c  \
	seg_objs.c  \
	seg_u.c  \
	seg_vn.c  \
	seg_vpix.c  \
	ucopy.c  \
	vm_anon.c  \
	vm_as.c  \
	vm_hat.c  \
	vm_machdep.c  \
	vm_meter.c  \
	vm_page.c  \
	vm_pageout.c  \
	vm_pvn.c  \
	vm_rm.c  \
	vm_seg.c  \
	vm_subr.c  \
	vm_swap.c  \
	vm_vpage.c  \
	vm_mapfile.c 


SFILES =  \
	copy.s  \
	vcopy.s

.s.o:
	$(AS) $<

all:	ID $(CONF)/pack.d/mem/Driver.o \
	$(CONF)/pack.d/kma/Driver.o $(CONF)/pack.d/kmacct/Driver.o

$(CONF)/pack.d/mem/Driver.o: $(FILES)
	$(LD) -r -o $@ $(FILES)

$(CONF)/pack.d/kma/Driver.o: kma.o
	$(LD) -r -o $@ kma.o

$(CONF)/pack.d/kmacct/Driver.o: kmacct.o
	$(LD) -r -o $@ kmacct.o

ID:
	cd mem.cf;    $(IDINSTALL) -R$(CONF) -M mem
	cd kma.cf;    $(IDINSTALL) -R$(CONF) -M kma
	cd kmacct.cf; $(IDINSTALL) -R$(CONF) -M kmacct

clean:
	-rm -f *.o

clobber: clean
	-$(IDINSTALL) -e -R$(CONF) -d mem
	-$(IDINSTALL) -e -R$(CONF) -d kma
	-$(IDINSTALL) -e -R$(CONF) -d kmacct

headinstall: \
	$(KBASE)/mem/anon.h \
	$(KBASE)/mem/as.h \
	$(KBASE)/mem/bootconf.h \
	$(KBASE)/mem/faultcatch.h \
	$(KBASE)/mem/faultcode.h \
	$(KBASE)/mem/hat.h \
	$(KBASE)/mem/immu.h \
	$(KBASE)/mem/kernel.h \
	$(KBASE)/mem/kmacct.h \
	$(KBASE)/mem/kmem.h \
	$(KBASE)/mem/page.h \
	$(KBASE)/mem/pte.h \
	$(KBASE)/mem/pvn.h \
	$(KBASE)/mem/rm.h \
	$(KBASE)/mem/seg.h \
	$(KBASE)/mem/seg_dev.h \
	$(KBASE)/mem/seg_dummy.h \
	$(KBASE)/mem/seg_kmem.h \
	$(KBASE)/mem/seg_map.h \
	$(KBASE)/mem/seg_objs.h \
	$(KBASE)/mem/seg_u.h \
	$(KBASE)/mem/seg_vn.h \
	$(KBASE)/mem/seg_vpix.h \
	$(KBASE)/mem/swap.h \
	$(KBASE)/mem/trace.h \
	$(KBASE)/mem/tuneable.h \
	$(KBASE)/mem/vm.h \
	$(KBASE)/mem/vm_hat.h \
	$(KBASE)/mem/vmlog.h \
	$(KBASE)/mem/vmmac.h \
	$(KBASE)/mem/vmmeter.h \
	$(KBASE)/mem/vmparam.h \
	$(KBASE)/mem/vmsystm.h \
	$(KBASE)/mem/vpage.h \
	$(FRC)
	$(INS) -f $(INC)/vm -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/anon.h
	$(INS) -f $(INC)/vm -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/as.h
	$(INS) -f $(INC)/vm -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/bootconf.h
	$(INS) -f $(INC)/vm -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/faultcatch.h
	$(INS) -f $(INC)/vm -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/faultcode.h
	$(INS) -f $(INC)/vm -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/hat.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/immu.h
	$(INS) -f $(INC)/vm -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/kernel.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/kmacct.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/kmem.h
	$(INS) -f $(INC)/vm -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/page.h
	$(INS) -f $(INC)/vm -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/pte.h
	$(INS) -f $(INC)/vm -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/pvn.h
	$(INS) -f $(INC)/vm -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/rm.h
	$(INS) -f $(INC)/vm -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/seg.h
	$(INS) -f $(INC)/vm -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/seg_dev.h
	$(INS) -f $(INC)/vm -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/seg_dummy.h
	$(INS) -f $(INC)/vm -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/seg_kmem.h
	$(INS) -f $(INC)/vm -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/seg_map.h
	$(INS) -f $(INC)/vm -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/seg_objs.h
	$(INS) -f $(INC)/vm -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/seg_u.h
	$(INS) -f $(INC)/vm -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/seg_vn.h
	$(INS) -f $(INC)/vm -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/seg_vpix.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/swap.h
	$(INS) -f $(INC)/vm -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/trace.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/tuneable.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/vm.h
	$(INS) -f $(INC)/vm -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/vm_hat.h
	$(INS) -f $(INC)/vm -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/vmlog.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/vmmac.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/vmmeter.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/vmparam.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/vmsystm.h
	$(INS) -f $(INC)/vm -m 644 -u $(OWN) -g $(GRP) $(KBASE)/mem/vpage.h


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

