#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mailx:mailx.mk	1.21.5.9"
# "@(#)mx.mk	1.22 'attmail mail(1) command'"
#
# mailx -- a modified version of a University of California at Berkeley
#	mail program
#
# for standard Unix
#

# first, in case $CMDRULES is empty, we define
# the things that $CMDRULES is supposed to supply.
#CFLAGS= -g $(LOCALDEF) $(LOCALINC)
CFLAGS= -O $(LOCALDEF) $(LOCALINC)
USRLIB= /usr/lib
MBINDIR=/usr/bin
USRBIN= /usr/bin
ETC= /etc
VAR= /var
USRSHARE= /usr/share
#LD_FLAGS =
LD_FLAGS = -s
LINT= lint -s

include $(CMDRULES)

# If system == SVR4ES, use the following...
VERS	 = SVR4ES
DESTLIB = $(USRSHARE)/lib/mailx/C
LDESTLIB = /usr/share/lib/mailx/C
LOCALINC=-Ihdr -I$(ROOT)/$(MACH)/usr/include
LOCALDEF=-Xa -D$(VERS)
LD_LIBS = -L$(USRLIB) -lmail -lw $(PERFLIBS)
RCDIR=$(ETC)/mail
LRCDIR=/etc/mail
SYMLINK = ln -s

include comm.mk
