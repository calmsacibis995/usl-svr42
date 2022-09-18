#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)portmgmt:i386at/cmd/portmgmt/port_quick/q-add.sh	1.1.3.13"
#ident	"$Header: $"

# PURPOSE: Configure the software to a particular device type on RS232
#
#---------------------------------------------------------------------

/usr/bin/rm -f /usr/tmp/ttylist* /usr/tmp/ap* /usr/tmp/title*

TTY00=`/usr/bin/ls -l /dev/term/00s | /usr/bin/cut -c1-45`
TTY01=`/usr/bin/ls -l /dev/term/01s | /usr/bin/cut -c1-45`
NOTTY00="NOTTY00"
NOTTY00h="NOTTY00h"
NOTTY01="NOTTY01"
NOTTY01h="NOTTY01h"

# lists ports configured and available for adding devices

if [ "$1" = "COLLECT" -o "$1" = "REMOVE" ]
then
	CONSOLE=`/usr/bin/ls -l /dev/console | /usr/bin/cut -c1-45`
	if [ "$CONSOLE" = "$TTY00" ]
	then
		NOTTY00=00s
		NOTTY00h=00h
	fi
	if [ "$CONSOLE" = "$TTY01" ]
	then
		NOTTY01=01s
		NOTTY01h=01h
	fi

	/usr/bin/ls /dev/*vt00 /dev/term/???* /dev/tty???* | /usr/bin/grep -v "$NOTTY00" | /usr/bin/grep -v "$NOTTY00h" | /usr/bin/grep -v "$NOTTY01"| /usr/bin/grep -v "$NOTTY01h" >>/usr/tmp/ttylist.$VPID
fi

SPEED=$2
TYPE="$1"
PREFIX=tty
shift
if [ $# -ne 0 ]
then
	shift
fi
for i in $*
do

	if [ `echo $i | /usr/bin/grep "term"` ]
	then
		TTY=`echo $i | /usr/bin/cut -c11-14`
	elif [ `echo $i | /usr/bin/grep "tty"` ]
	then
		TTY=`echo $i | /usr/bin/cut -c9-12`
	else
		TTY=`echo $i | /usr/bin/cut -c6-14`
	fi
/usr/bin/mouseadmin -l | /usr/bin/grep tty00 >/dev/null 2>&1
if [ $? -eq 0 ]
then
	if [ $TTY = 00s -o $TTY = 00h -o $TTY = 00 ]
	then
		echo "Warning" > /usr/tmp/title.$VPID
		echo "Serial mouse on tty port. /dev/term/$TTY was not added. Select another port." >>/usr/tmp/ap.$VPID
	/usr/bin/ls /dev/*vt00 /dev/term/???* /dev/tty???* | /usr/bin/grep -v "$NOTTY00" | /usr/bin/grep -v "$NOTTY00h" | /usr/bin/grep -v "$NOTTY01"| /usr/bin/grep -v "$NOTTY01h" >>/usr/tmp/ttylist.$VPID
	echo 0
	exit 0
	fi
fi	
/usr/bin/mouseadmin -l | /usr/bin/grep tty01 >/dev/null 2>&1
if [ $? -eq 0 ]
then
	if [ $TTY = 01s -o $TTY = 01h -o $TTY = 01 ]
	then
		echo "Warning" > /usr/tmp/title.$VPID
		echo "Serial mouse on tty port. /dev/term/$TTY was not added. Select another port." >>/usr/tmp/ap.$VPID
	/usr/bin/ls /dev/*vt00 /dev/term/???* /dev/tty???* | /usr/bin/grep -v "$NOTTY00" | /usr/bin/grep -v "$NOTTY00h" | /usr/bin/grep -v "$NOTTY01"| /usr/bin/grep -v "$NOTTY01h" >>/usr/tmp/ttylist.$VPID
	echo 0
	exit 0
	fi
fi	
/usr/sbin/sacadm -l | /usr/bin/grep ttymon3 >/dev/null 2>&1
if [ $? = 1 ]
then
	/usr/sbin/sacadm -a -pttymon3 -t ttymon -c "/usr/lib/saf/ttymon" -v " `/usr/sbin/ttyadm -V ` "
fi
/usr/sbin/pmadm -r -p ttymon3 -s $TTY >/dev/null 2>&1
/usr/sbin/pmadm -a -p ttymon3 -s $TTY -S "login" -fu -v " `/usr/sbin/ttyadm -V ` " -m " `/usr/sbin/ttyadm -d $i -l $SPEED -s /usr/bin/shserv -m ldterm -p \"login: \" ` "
echo "Confirmation" > /usr/tmp/title.$VPID
echo "The port $i was setup.\n" >>/usr/tmp/ap.$VPID

done
echo 0
exit 0
