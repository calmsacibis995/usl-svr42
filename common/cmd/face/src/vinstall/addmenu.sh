#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)face:src/vinstall/addmenu.sh	1.3.4.4"
#ident  "$Header: addmenu.sh 1.5 91/10/16 $"

trap 'rm -f $VMSYS/OBJECTS/.Lserve ; exit 0' 1 2 15

error ()
{
	echo "$*"; rm -f ${VMSYS}/OBJECTS/.Lserve; exit 1
}
# set VMSYS so that Menu.programs file can be updated if installed.
VMSYS=`sed -n -e '/^vmsys:/s/^.*:\([^:][^:]*\):[^:]*$/\1/p' /etc/passwd`
export VMSYS

if [ ! -d "${VMSYS}" ]
then
	error `gettxt uxface:9 "Can't find home directory for vmsys"`
fi

if [ ! -f ${VMSYS}/OBJECTS/.Lserve ]
then
	>${VMSYS}/OBJECTS/.Lserve
else
	error `gettxt uxface:12 "Can't update ${VMSYS}/lib/services file because it is LOCKED!!!"`
fi

echo "\`echo 'name=\"${1}\"';echo 'action=OPEN ${2}'\`" >> $VMSYS/lib/services || error `gettxt uxface:7 "Can't access $VMSYS/lib/services"`
sort $VMSYS/lib/services > /tmp/f.sv.$$
cp /tmp/f.sv.$$ $VMSYS/lib/services || error `gettxt uxface:7 "Can't access $VMSYS/lib/services"`
rm -f /tmp/f.sv.$$

rm -f ${VMSYS}/OBJECTS/.Lserve
exit 0
