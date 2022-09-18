#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/mail.mk	1.47.1.11"
# "@(#)ma.mk	2.81 'attmail mail(1) command'"
#
#	mail make file
#

# If SVR4DT, use the following...

# first, in case $CMDRULES is empty, we define
# the things that $CMDRULES is supposed to supply.
CFLAGS= -O $(LOCALDEF) $(LOCALINC)
USRLIB= /usr/lib
MBINDIR=/usr/bin
USRBIN= /usr/bin
ETC= /etc
VAR= /var
USRSHARE= /usr/share

include $(CMDRULES)

OWN = 
GRP = 

MBINDIR = $(USRBIN)
CBINDIR = $(USRBIN)
MBOXDIR = $(VAR)/mail
FILEDIR = $(ETC)/mail
LFILEDIR = /etc/mail
SHRLIB  = $(USRSHARE)/lib
MPDIR = $(USRLIB)/mail
USRINC = $(ROOT)/$(MACH)/usr/include

REAL_MBOXDIR = /var/mail
REAL_SHRLIB = /usr/share/lib
REAL_PATH = /usr/lib/mail/surrcmd:/usr/bin
REAL_SHELL = /sbin/sh
REAL_VARSPOOLSMTPQ = /var/spool/smtpq

SYMLINK = ln -s
VERS = SVR4
VERS2 = -DSVR4_1
LIBMAIL = libmail.a
LIBRE = libre.a
LOCALDEF= -D$(VERS) $(VERS2)
LOCALINC= -I.
LD_LIBS= $(LIBMAIL) $(LIBRE)
MAILLIBS = $(LD_LIBS) -lw -lmalloc
TMPDIR = /usr/tmp
LINTFLAGS= $(LOCALDEF) $(LOCALINC)
SH_OPTCMD=USEGETOPTS
SH_PRTCMD=USEPFMT

REFLAGS = -O -v
#REFLAGS = -g -v
LINT_C = lint-c
VAC_MSG_LOC = $(USRSHARE)/lib/mail
INST_MSGFILES = yes
SMSRCMAKE=src.mk
LINT= $(PFX)lint -s
RANLIB=:

include comm.mk
