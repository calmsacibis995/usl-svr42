#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)initpkg:i386at/cmd/initpkg/mini_itab.sh	1.2.7.2"
#ident "$Header: mini_inittab.sh 1.4 91/06/28 $"

echo "ap::sysinit:/sbin/autopush -f /etc/ap/chan.ap
bt::sysinit:/sbin/sh -c 'echo Booting System Maintenance Mode' </dev/console >/dev/console 2>&1
fs::sysinit:/sbin/bcheckrc </dev/console >/dev/console 2>&1
ck::sysinit:/sbin/setclk </dev/console >/dev/console 2>&1
xdc::sysinit:/sbin/sh -c 'if [ -x /etc/rc.d/es_setup ] ; then /etc/rc.d/es_setup ; fi' >/dev/console 2>&1
ia::sysinit:/sbin/creatiadb </dev/console >/dev/console 2>&1
is:s:initdefault:
s0:0:wait:/sbin/rc0 off >/dev/console 2>&1 </dev/console
s1:1234:wait:/sbin/rc1 >/dev/console 2>&1 </dev/console
s6:6:wait:/sbin/rc6 reboot >/dev/console 2>&1 < /dev/console
ec:1234:wait:/sbin/sh -c 'echo Changing states in System Maintenance Mode;echo Multi-user operations will not be started' </dev/console >/dev/console 2>&1
co:1234:respawn:/usr/lib/saf/ttymon -g -d /dev/systty -p \"Console Login: \" -m ldterm -l console" \
>mini_inittab
