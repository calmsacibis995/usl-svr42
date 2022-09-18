#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)initpkg:common/cmd/initpkg/dinit.d/:mk.dinit.d.sh	1.7"
#ident  "$Header: $"

STARTLST="23ttymap 70uucp 75cron 80cs"

INSDIR=${ROOT}/${MACH}/etc/dinit.d
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
