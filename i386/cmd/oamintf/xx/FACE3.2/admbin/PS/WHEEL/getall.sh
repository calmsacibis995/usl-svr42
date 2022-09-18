#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.



#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/admbin/PS/WHEEL/getall.sh	1.1.1.2"
#ident	"$Header: $"

> /usr/tmp/wheel.$VPID
text=`/usr/lib/lpadmin -S $1 -A list `

if [ "`echo "$text" | grep mail`" != "" ]
	then 
		echo "fault=mail" > /usr/tmp/wheel.$VPID
		login=`echo $text | /usr/bin/cut -d' ' -f7`
	     	echo "login=$login" >> /usr/tmp/wheel.$VPID
elif [ "`echo "$text" | /usr/bin/grep "alert\ with"`" != "" ]
	then  
		found=`echo "$text" | /usr/bin/cut -d' ' -f7 | /usr/bin/sed 's/"//g' `
		echo "fault=$found" > /usr/tmp/wheel.$VPID
		login=`echo "$text" | /usr/bin/cut -d' ' -f9 | /usr/bin/sed 's/"//g' `
	     	echo "login=$login" >> /usr/tmp/wheel.$VPID
fi



if [ -f /usr/spool/lp/admins/lp/pwheels/$1/alert.vars ]
	then
	value=`/usr/bin/ed - /usr/spool/lp/admins/lp/pwheels/$1/alert.vars <<-eof
		2
		q
	eof`
fi

if [ "$value" = 0 ]
	then echo "freq=once" >> /usr/tmp/wheel.$VPID
else
	echo "freq=$value" >> /usr/tmp/wheel.$VPID
fi



if [  -f /usr/spool/lp/admins/lp/pwheels/$1/alert.vars ]
	then
	value=`/usr/bin/ed - /usr/spool/lp/admins/lp/pwheels/$1/alert.vars <<-eof
		1
		q
	eof`
fi

echo "requests=$value" >> /usr/tmp/wheel.$VPID
