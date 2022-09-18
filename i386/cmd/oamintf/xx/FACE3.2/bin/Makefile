#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/bin/Makefile	1.2"
#ident	"$Header: $"

# Note: In 3.2 FACE, some shells install as .sh and others do not!

include $(CMDRULES)

FACEBIN=$(ROOT)/$(MACH)/usr/vmsys/bin
BINARY=getchg getdel getword slash
SHELLS=doexec getname
PRODUCTS=$(BINARY) $(SHELLS)
GRP=bin
OWN=bin

all: $(PRODUCTS)
	
$(BINARY):
	@echo "***** Making VMSYS/bin"
	$(CC) $(CFLAGS) $(DEFLIST) $(@).c -o $(@) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

$(SHELLS):
	-for f in $(SHELLS) ; \
	do \
		cp $$f.sh $$f ; \
	done

install: all
	-[ -d $(FACEBIN) ] || mkdir -p $(FACEBIN)

	-for f in $(PRODUCTS) ; \
	do \
		$(INS) -m 755 -g $(GRP) -u $(OWN) -f $(FACEBIN) $$f ; \
	done

clean:
	@echo "***** Cleaning VMSYS/bin"
	rm -f *.o lint.out

clobber: clean
	@echo "***** Clobbering VMSYS/bin"
	rm -f $(PRODUCTS)

size:
	-for f in $(BINARY) ; \
	do \
		$(SIZE) $$f ; \
	done

strip:
	-for f in $(BINARY) ; \
	do \
		$(STRIP) $$f ; \
	done

lintit:
	-for f in $(BINARY) ; \
	do \
		$(LINT) $(LINTFLAGS) $(DEFLIST) $$f.c $(LINTLIBS) >> lint.out ; \
	done
