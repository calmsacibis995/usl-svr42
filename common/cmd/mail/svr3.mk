#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/svr3.mk	1.3"
# "@(#)svr3.mk	1.9 'attmail mail(1) command'"
#
#	mail make file
#

# If VERS == SVR3 && !SUN4.1, use the following...
USRLIB= /usr/lib
MBINDIR = /bin
CBINDIR = /usr/bin
MBOXDIR = /usr/mail
FILEDIR = /etc/mail
LFILEDIR = /etc/mail
SHRLIB  = /usr/lib
MPDIR = /usr/lib/mail
USRINC= /usr/include

REAL_MBOXDIR = /usr/mail
REAL_SHRLIB = /usr/lib
REAL_PATH = /usr/lib/mail/surrcmd:/bin:/usr/bin
REAL_SHELL = /bin/sh
REAL_VARSPOOLSMTPQ = /usr/spool/smtpq

SYMLINK = :
VERS = SVR3
VERS2 =
LIBMAIL = libmail.a
LIBRE = libre.a
LOCALDEF= -D$(VERS) $(VERS2)
LOCALINC= -I.
LD_LIBS= $(LIBMAIL) $(LIBRE)
MAILLIBS = $(LD_LIBS) -lmalloc
TMPDIR = /usr/tmp
LINTFLAGS= $(LOCALDEF) $(LOCALINC)
CPPDEFS = $(LOCALDEF) $(LOCALINC)
#CFLAGS = -O $(CPPDEFS)
CFLAGS = -g $(CPPDEFS)
# LD_FLAGS= -s
LD_FLAGS=
SH_OPTCMD=USEGETOPT
SH_PRTCMD=USEECHO

REFLAGS = -O
#REFLAGS = -g
LINT_C = cc
VAC_MSG_LOC = $(ROOT)/usr/lib/mail
INST_MSGFILES = no
SMSRCMAKE=svr3.mk
LINT= $(PFX)lint
CMDRULES=/dev/null
RANLIB=:

include comm.mk
