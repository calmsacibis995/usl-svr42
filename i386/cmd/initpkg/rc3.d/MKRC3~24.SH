#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)initpkg:i386/cmd/initpkg/rc3.d/:mk.rc3.d.sh	1.7.8.4"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/initpkg/rc3.d/:mk.rc3.d.sh,v 1.1 91/02/28 17:37:45 ccs Exp $"

STARTLST="21rfs"
STOPLST= 

INSDIR=${ROOT}/${MACH}/etc/rc3.d
if u3b2 || i386
then
	if [ ! -d ${INSDIR} ] 
	then 
		mkdir ${INSDIR} 
		eval ${CH}chmod 755 ${INSDIR}
		eval ${CH}chgrp sys ${INSDIR}
		eval ${CH}chown root ${INSDIR}
	fi 
	for f in ${STARTLST}
	do 
		name=`echo $f | sed -e 's/^..//'`
		rm -f ${INSDIR}/S$f
		ln ${ROOT}/${MACH}/etc/init.d/${name} ${INSDIR}/S$f
	done
fi
