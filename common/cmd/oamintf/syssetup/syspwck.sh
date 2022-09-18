#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/syssetup/syspwck.sh	1.1.7.2"
#ident  "$Header: syspwck.sh 2.0 91/07/12 $"

syslogs=`/usr/bin/sed -n '/^[^:]*:[^:]*:.\{1,2\}:/s/:.*//p' /etc/passwd | /usr/bin/sort -u`

# Remove output files from previous call to this
# script within the same fmli session.
/usr/bin/rm -f /tmp/pswderr.${VPID} /tmp/syslgs.${VPID}

# get system logins which do not have a password
	for sys in $syslogs
	do
		if $TFADMIN passwd -s $sys 2>/tmp/pswderr.${VPID} | /usr/bin/grep "^$sys  *LK" >/dev/null
		then
			echo $sys >> /tmp/syslgs.${VPID}
		fi
	done


if [ -s /tmp/syslgs.${VPID} ] && [ ! -s /tmp/pswderr.${VPID} ]
then 
	exit 0
else if [ ! -f /tmp/syslgs.${VPID} ] && [ ! -s /tmp/pswderr.${VPID} ]
     then
	    exit 2
     else
	    exit 1
     fi
fi


