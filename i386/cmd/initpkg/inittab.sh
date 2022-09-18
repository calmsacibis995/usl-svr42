#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)initpkg:i386/cmd/initpkg/inittab.sh	1.18.29.16"
#ident "$Header:"

#NOTE:  This inittab source is used during integration to create the original
#	/etc/inittab.  The source under uts-x86at:kernel.cf/Init should be kept 
#	in sync with this source.
#	The platform-specific source under uts-x86at:kernel.cf/Init is placed 
#	under /etc/conf/init.d/kernel during installation and is used to 
#	restore /etc/inittab after a kernel rebuild using the ID tools.

echo "cr::sysinit:/sbin/ckroot > /dev/sysmsg 2>&1
ck::sysinit:/sbin/setclk  >/dev/sysmsg 2>&1
mm::sysinit:/etc/conf/bin/idmodreg >/dev/sysmsg 2>&1
ldmd::sysinit:/etc/conf/bin/idmodload >/dev/sysmsg 2>&1
ap::sysinit:/sbin/autopush -f /etc/ap/chan.ap
ak::sysinit:/sbin/wsinit 1>/etc/wsinit.err 2>&1
bchk::sysinit:/sbin/bcheckrc </dev/console >/dev/sysmsg 2>&1
bu::sysinit:/etc/conf/bin/idrebuild reboot </dev/console >/dev/sysmsg 2>&1
me::sysinit:/etc/conf/bin/idmkenv >/dev/sysmsg 2>&1
xdc::sysinit:/sbin/sh -c 'if [ -x /etc/rc.d/es_setup ] ; then /etc/rc.d/es_setup ; fi' >/dev/console 2>&1
ia::sysinit:/sbin/creatiadb </dev/console >/dev/sysmsg 2>&1
is:2:initdefault:
bd:56:wait:/etc/conf/bin/idrebuild </dev/console >/dev/sysmsg 2>&1
r0:0:wait:/sbin/rc0 off 1> /dev/sysmsg 2>&1 </dev/console
r1:1:wait:/sbin/rc1  1> /dev/sysmsg 2>&1 </dev/console
r2:23:wait:/sbin/rc2 1> /dev/sysmsg 2>&1 </dev/console
r3:3:wait:/sbin/rc3  1> /dev/sysmsg 2>&1 </dev/console
r5:5:wait:/sbin/rc0 firm 1> /dev/sysmsg 2>&1 </dev/console
r6:6:wait:/sbin/rc0 reboot 1> /dev/sysmsg 2>&1 </dev/console
li:23:wait:/usr/bin/ln /dev/systty /dev/syscon >/dev/null 2>&1
sc:234:respawn:/usr/lib/saf/sac -t 300
co:12345:respawn:/usr/lib/saf/ttymon -g -v -p \"Console Login: \" -d /dev/console -l console
d2:23:wait:/sbin/dinit 1> /dev/sysmsg 2>&1 </dev/console
" >inittab
