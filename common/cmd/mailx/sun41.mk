#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mailx:sun41.mk	1.6"
# "@(#)sun41.mk	1.8 'attmail mail(1) command'"
#
# mailx -- a modified version of a University of California at Berkeley
#	mail program
#
# for standard Unix
#

# If system == SVR3 && SUN4.1, use the following...
CPPDEFS = -DpreSVr4 -I$(HDR) -DNEED_SIGRETRO -DVAR_SPOOL_MAIL \
	  -DHAVE_GID_T -DHAVE_UID_T -DHAVE_MODE_T -DHAVE_PID_T
CFLAGS  = -O $(CPPDEFS)
#CFLAGS  = -g $(CPPDEFS)
USRLIB= /usr/lib
MBINDIR=/usr/bin
USRBIN= /usr/bin
ETC= /etc
VAR= /var
USRSHARE= /usr/share
LD_FLAGS = -s
LINT= lint

VERS	 = SVR3
DESTLIB  = $(ROOT)/usr/lib/mailx
LDESTLIB = $(ROOT)/usr/lib/mailx
LD_LIBS = -lmail
SYMLINK = ln -s
RCDIR = $(ROOT)/etc/mail
LRCDIR = $(ROOT)/etc/mail
CC = $(ROOT)/usr/5bin/cc

include comm.mk
