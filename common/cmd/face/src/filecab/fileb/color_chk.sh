#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)face:src/filecab/fileb/color_chk.sh	1.2.4.3"
#ident  "$Header: color_chk.sh 1.5 92/01/17 $"

color=`$VMSYS/bin/col2e $1`
background=$2
text=`$VMSYS/bin/col2e $3`

case $color in
	  black \
	| blue \
	| cyan \
	| green \
	| magenta \
	| red \
	| white \
	| yellow )
		if [ "$color" = "$background" ]
		then
			exit 2
		elif [ "$color" = "$text" ]
		then
			exit 3
		else
			exit 0
		fi;;
	*)
		exit 1;;
esac
