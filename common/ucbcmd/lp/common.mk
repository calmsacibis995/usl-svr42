#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/lp/common.mk	1.4"
#ident	"$Header: $"

#
# Common makefile definitions used by all makefiles
#


#####
#
# Each makefile defines $(TOP) to be a reference to this top-level
# directory (e.g. TOP=..).
#####


RM	=	$(RM) -f

USRLIBLP=	$(USRLIB)/lp

USRSHARELIB=	$(USRSHARE)/lib

LPBINDIR=	$(USRLIBLP)/bin

VARSPOOL=	$(VAR)/spool

ETCLP   =       $(ETC)/lp


#####
#
# Typical owner and group for LP things. These can be overridden
# in the individual makefiles.
#####
OWNER	=	lp
GROUP	=	lp
LEVEL	=	SYS_PUBLIC
SUPER	=	root

#####
#
# $(EMODES): Modes for executables
# $(SMODES): Modes for setuid executables
# $(DMODES): Modes for directories
#####
EMODES	=	0555
SMODES	=	04555
DMODES	=	0775


INCSYS  =       $(INC)/sys

TOP     =       ../../cmd/lp

LPINC	=	../$(TOP)/include
LPLIB	=	../$(TOP)/lib

LIBACC	=	$(LPLIB)/access/liblpacc.a
LIBCLS	=	$(LPLIB)/class/liblpcls.a
LIBFLT	=	$(LPLIB)/filters/liblpflt.a
LIBFRM	=	$(LPLIB)/forms/liblpfrm.a
LIBLP	=	$(LPLIB)/lp/liblp.a
LIBMSG	=	$(LPLIB)/msgs/liblpmsg.a
LIBOAM	=	$(LPLIB)/oam/liblpoam.a
LIBPRT	=	$(LPLIB)/printers/liblpprt.a
LIBREQ	=	$(LPLIB)/requests/liblpreq.a
LIBSEC	=	$(LPLIB)/secure/liblpsec.a
LIBSYS	=	$(LPLIB)/systems/liblpsys.a
LIBUSR	=	$(LPLIB)/users/liblpusr.a
#LIBNET	=       $(LPLIB)/lpNet/liblpNet.a
LIBBSD  =       $(LPLIB)/bsd/liblpbsd.a

LINTACC	=	$(LPLIB)/access/llib-llpacc.ln
LINTCLS	=	$(LPLIB)/class/llib-llpcls.ln
LINTFLT	=	$(LPLIB)/filters/llib-llpflt.ln
LINTFRM	=	$(LPLIB)/forms/llib-llpfrm.ln
LINTLP	=	$(LPLIB)/lp/llib-llp.ln
LINTMSG	=	$(LPLIB)/msgs/llib-llpmsg.ln
LINTOAM	=	$(LPLIB)/oam/llib-llpoam.ln
LINTPRT	=	$(LPLIB)/printers/llib-llpprt.ln
LINTREQ	=	$(LPLIB)/requests/llib-llpreq.ln
LINTSEC	=	$(LPLIB)/secure/llib-llpsec.ln
LINTSYS	=	$(LPLIB)/systems/llib-llpsys.ln
LINTUSR	=	$(LPLIB)/users/llib-llpusr.ln
