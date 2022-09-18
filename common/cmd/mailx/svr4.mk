#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mailx:svr4.mk	1.5"
# "@(#)svr4.mk	1.7 'attmail mail(1) command'"
#
# mailx -- a modified version of a University of California at Berkeley
#	mail program
#
# for standard Unix
#

# If system == SVR4, use the following...
CPPDEFS = -DUSE_TERMIOS -I$(HDR) -I$(CRX)/usr/include -I$(ROOT)/usr/include
#CFLAGS  = -O -v -Xa $(CPPDEFS)
CFLAGS  = -g -v -Xa $(CPPDEFS)
USRLIB= /usr/lib
MBINDIR=/usr/bin
USRBIN= /usr/bin
ETC= /etc
VAR= /var
USRSHARE= /usr/share
#LD_FLAGS = -s -dn $(LDFLAGS) $(PERFLIBS)
LD_FLAGS = $(LDFLAGS) $(PERFLIBS) -dn
LINT= lint -s

VERS	 = SVR4
DESTLIB = $(ROOT)/usr/share/lib/mailx/C
LDESTLIB = $(DESTLIB)
LD_LIBS = -L$(ROOT)/usr/lib -lmail -lw $(LDLIBS)
SYMLINK = ln -s
RCDIR = $(ROOT)/etc/mail
LRCDIR = /etc/mail

include comm.mk
