#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)lp:cmd/lpsched/lpNet/Makefile	1.16.3.3"
#ident  "$Header: Makefile 1.5 91/06/27 $"

include $(CMDRULES)

LPROOT	=	../../..
LPNETROOT=	.
LPLIB	=	$(LPROOT)/lib
LPINC	=	$(LPROOT)/include
INCSYS	=	$(INC)/sys

USRLIBLP=	$(USRLIB)/lp
ETCLP	=	$(ETC)/lp

OWNER	=	lp
GROUP	=	tty
MODES	=	0555

NETLIBS =	parent/parent.a	\
		svChild/svChild.a	\
		bsdChild/bsdChild.a	\
		nucChild/nucChild.a

LPNETLIB=	$(LPLIB)/lpNet/liblpNet.a
LIBBSD	=	$(LPLIB)/bsd/liblpbsd.a

LPLIBS	=	$(LPNETLIB)			\
		$(LIBBSD)			\
		$(LPLIB)/requests/liblpreq.a	\
		$(LPLIB)/msgs/liblpmsg.a	\
		$(LPLIB)/secure/liblpsec.a	\
		$(LPLIB)/printers/liblpprt.a	\
		$(LPLIB)/systems/liblpsys.a	\
		$(LPLIB)/lp/liblp.a             \
		$(LPLIB)/oam/liblpoam.a

DEBUG=
#LOCALDEF=$(DEBUG)
GLIB	=
SYSLIBS	=	-lnls -lnsl -liaf -lgen -lcurses $(GLIB)
LIBS	=	$(NETLIBS) $(LPLIBS) $(SYSLIBS)

SRCDIRS	=	parent svChild bsdChild nucChild
TARGETS	=	lpNet

MAKECMD	=	$(MAKE) $(MAKEARGS)

.PRECIOUS:	$(TARGETS) $(NETLIBS)


all:		$(TARGETS)
		@for t in $(TARGETS);			\
		do					\
			echo "$${t} is-up-to-date.";	\
		done

install:
		$(INS) -m $(MODES) -u $(OWNER) -g $(GROUP) \
			-f $(USRLIBLP) lpNet
		$(INS) -m 644 -u $(OWNER) -g $(GROUP) \
			-f $(ETCLP) Systems

strip:
		$(STRIP) lpNet

clean:
		@for d in $(SRCDIRS); \
		do \
			cd $${d}; \
			$(MAKECMD) $@; \
			cd ..; \
		done

clobber:
		@for d in $(SRCDIRS); \
		do \
			cd $${d}; \
			$(MAKECMD) $@; \
			cd ..; \
		done
		rm -f $(TARGETS)

product:
		@for t in $(TARGETS);	\
		do			\
			echo "$${t}";	\
		done

lintit:
		@for d in $(SRCDIRS); \
		do \
			cd $${d}; \
			$(MAKECMD) $@; \
			cd ..; \
		done

lintlib:
		@for d in $(SRCDIRS); \
		do \
			cd $${d}; \
			$(MAKECMD) $@; \
			cd ..; \
		done

lpNet:		$(NETLIBS)
		$(CC) $(LDFLAGS) $(LIBS) $(SHLIBS) -o lpNet

#parent/parent.a:	FRC
parent/parent.a:
	cd parent; $(MAKECMD) all

#svChild/svChild.a:	FRC
svChild/svChild.a:
	cd svChild; $(MAKECMD) all

#bsdChild/bsdChild.a:	FRC
bsdChild/bsdChild.a:
	cd bsdChild; $(MAKECMD) all

#nucChild/nucChild.a:	FRC
nucChild/nucChild.a:
	cd nucChild; $(MAKECMD) all

FRC:
