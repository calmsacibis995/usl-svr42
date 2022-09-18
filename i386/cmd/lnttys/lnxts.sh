#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lnttys:lnxts.sh	1.1.3.1"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/lnttys/lnxts.sh,v 1.1 91/02/28 17:44:22 ccs Exp $"
# install links to /dev sub-directories

cd /dev/xt
for i in *
do
	rm -f /dev/xt$i
	ln /dev/xt/$i /dev/xt$i
done
