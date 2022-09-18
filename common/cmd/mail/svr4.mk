#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/svr4.mk	1.3"
# "@(#)svr4.mk	1.8 'attmail mail(1) command'"
#
#	mail make file
#

# If SVR4, use the following...
USRLIB= /usr/lib
MBINDIR = /usr/bin
CBINDIR = /usr/bin
MBOXDIR = /var/mail
FILEDIR = /etc/mail
LFILEDIR = /etc/mail
SHRLIB  = /usr/share/lib
MPDIR = /usr/lib/mail
USRINC= /usr/include

REAL_MBOXDIR = /var/mail
REAL_SHRLIB = /usr/share/lib
REAL_PATH = /usr/lib/mail/surrcmd:/usr/bin
REAL_SHELL = /sbin/sh
REAL_VARSPOOLSMTPQ = /var/spool/smtpq

SYMLINK = ln -s
VERS = SVR4
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
CFLAGS = -O $(CPPDEFS)
SH_OPTCMD=USEGETOPTS
SH_PRTCMD=USEECHO

REFLAGS = -O -v
#REFLAGS = -g -v
LINT_C = lint-c
VAC_MSG_LOC = $(ROOT)/usr/share/lib/mail
INST_MSGFILES = yes
SMSRCMAKE=svr4.mk
LINT= $(PFX)lint
CMDRULES=/dev/null
RANLIB=:

include comm.mk
