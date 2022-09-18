#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/sun41.mk	1.3"
# "@(#)sun41.mk	1.7 'attmail mail(1) command'"
#
#	mail make file
#

# If VERS == SVR3 && SUN4.1, use the following...
USRLIB= /usr/lib
MBINDIR = /bin
CBINDIR = /usr/bin
MBOXDIR = /var/spool/mail
FILEDIR = /etc/mail
LFILEDIR = /etc/mail
SHRLIB  = /usr/share/lib
MPDIR = /usr/lib/mail
USRINC= /usr/include

REAL_MBOXDIR = /var/spool/mail
REAL_SHRLIB = /usr/share/lib
REAL_PATH = /usr/lib/mail/surrcmd:/usr/bin
REAL_SHELL = /bin/sh
REAL_VARSPOOLSMTPQ = /var/spool/smtpq

SYMLINK = ln -s
VERS = SVR3
VERS2 =
CC = $(ROOT)/usr/5bin/cc
LIBMAIL = libmail.a
LIBRE = libre.a
LOCALDEF= -D$(VERS) $(VERS2) -DUSE_STRFTIME -DUSE_GETDTABLESIZE
LOCALINC= -I.
LD_LIBS= $(LIBMAIL) $(LIBRE)
MAILLIBS = $(LD_LIBS)
TMPDIR = /usr/tmp
LINTFLAGS= $(LOCALDEF) $(LOCALINC)
CPPDEFS = $(LOCALDEF) $(LOCALINC)
CFLAGS = -O $(CPPDEFS)
SH_OPTCMD=USEGETOPTS
SH_PRTCMD=USEECHO

REFLAGS = -O
#REFLAGS = -g
LINT_C = lint-c
VAC_MSG_LOC = $(ROOT)/usr/share/lib/mail
INST_MSGFILES = yes
SMSRCMAKE=sun41.mk
LINT= $(PFX)lint
CMDRULES=/dev/null
RANLIB=ranlib

include comm.mk
