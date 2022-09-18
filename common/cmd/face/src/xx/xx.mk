#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)face:src/xx/xx.mk	1.6.6.6"
#ident "$Header: xx.mk 1.5 91/06/11 $"

include $(CMDRULES)

LOCALINC= -I../filecab/inc
LDLIBS= ../proc/libproc.a

VMSYS = $(ROOT)/$(MACH)/home/vmsys
VMBIN = $(ROOT)/$(MACH)/home/vmsys/bin

CMDS =	suspend slash

all:	$(CMDS)

clean:
	rm -f *.o

clobber: clean
	rm -f facesuspend slash

suspend: suspend.o $(LDLIBS)
	$(CC) -o facesuspend suspend.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

suspend.o: suspend.c ../filecab/inc/wish.h

slash:  slash.o
	$(CC) -o $@ slash.o $(LDFLAGS) $(SHLIBS)

slash.o: slash.c
 
install: all $(VMSYS) $(VMBIN) dir
	:  VMBIN
		cp facesuspend $(VMBIN)/facesuspend
		cp initial.txt $(VMBIN)/initial
		cp slash $(VMBIN)/slash
		cp cmdfile.txt $(VMBIN)/cmdfile
		cp chksys.sh $(VMBIN)/chksys
		cp unix.sh $(VMBIN)/unix
		$(CH)chmod 755 $(VMBIN)/facesuspend
		$(CH)chmod 755 $(VMBIN)/slash
		$(CH)chmod 644 $(VMBIN)/initial
		$(CH)chmod 644 $(VMBIN)/cmdfile
		$(CH)chmod 755 $(VMBIN)/chksys
		$(CH)chmod 755 $(VMBIN)/unix
	:  OBJECTS
		cd ../../OBJECTS; find . -follow -print | cpio -pcvduL  $(VMSYS)/OBJECTS
	:  HELP
		cd ../../HELP; find . -follow -print | cpio -pcvduL $(VMSYS)/HELP

$(VMSYS):
	-mkdir -p $@ ; $(CH)chmod 755 $@

$(VMBIN):
	[ -d $@ ] || mkdir $@ ; $(CH)chmod 755 $@

dir:
	[ -d $(VMSYS)/OBJECTS ] || mkdir $(VMSYS)/OBJECTS 
	[ -d $(VMSYS)/HELP ] || mkdir $(VMSYS)/HELP 
	$(CH)find $(VMSYS)/OBJECTS $(VMSYS)/HELP -type d -print -exec chmod 755 {} \;
