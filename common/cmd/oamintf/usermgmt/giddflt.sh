#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/usermgmt/giddflt.sh	1.2.7.2"
#ident  "$Header: giddflt.sh 2.0 91/07/12 $"

################################################################################
#	Command Name: giddflt
#
# 	This functions is used for finding the next available Group ID 
################################################################################

# Sort 3rd field in /etc/group then cut 3rd field
# from last line.

defuid=`/usr/bin/sort -u -t: +2n /etc/group | /usr/bin/cut -d: -f3`

# assign minid the default reseve id

# getusrdefs has been replaced by defadm which does not support
# extracting the maximum reserved id information.  Since 
# getusrdefs assigned 99 to this if the default file did not
# contain this information (as is now the case), we'll hard 
# code it here.
# minid=`/usr/sadm/sysadm/bin/getusrdefs -r | /usr/bin/cut -d= -f2`
minid=99

# eliminate all userid's less than minid, then find first available
# userid number greater than minid.

for n in `echo ${defuid} | /usr/bin/sed -e 's% \([1-9][0-9][0-9][0-9]*\)%_\1%;s%.*_%%'`
do
if [ $minid -ne $n ]
then
	break
else
	minid=`expr $minid + 1`
fi
done
echo $minid
