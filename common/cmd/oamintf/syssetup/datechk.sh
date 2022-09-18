#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/syssetup/datechk.sh	1.1.4.3"
#ident  "$Header: datechk.sh 2.0 91/07/12 $"
################################################################################
#	Module Name: datechk
#	Calling Sequence: This script is invoked through the Form.set
#			  form in "syssetup:datetime:set" menu.
#	Functional Description:	
#	Inputs: $1 is MM, $2 is Day and $3 is Year (ccxx).
#	Outputs: Exit code should always be 0.
#	Functions Called: 
###############################################################################

if test $# -ne 3 
then
	exit 1
fi

fmtstr="%D"

# assign month
case "${1}" in
	'Jan' | 'January')	mm="01";;
	'Feb' | 'February')	mm="02";;
	'Mar' | 'March')	mm="03";;
	'Apr' | 'April')	mm="04";;
	'May' | 'May')		mm="05";;
	'Jun' | 'June')		mm="06";;
	'Jul' | 'July')		mm="07";;
	'Aug' | 'August')	mm="08";;
	'Sep' | 'September')	mm="09";;
	'Oct' | 'October')	mm="10";;
	'Nov' | 'November')	mm="11";;
	'Dec' | 'December')	mm="12";;
	'*')			mm="";;
esac

# assign day
case "${2}" in
	'1')	day="01";;
	'2')	day="02";;
	'3')	day="03";;
	'4')	day="04";;
	'5')	day="05";;
	'6')	day="06";;
	'7')	day="07";;
	'8')	day="08";;
	'9')	day="09";;
	*)	day="$2";;
esac

dd=$day

# assign year
if [ `echo $3 | /usr/bin/wc -c` -ge 4 ]
then
	year=$3
	yy=$3
	fmtstr="%m/%d/%Y"
#	yy=`echo "$3" | /usr/bin/cut -c3,4`
else 
	yy=$3
fi


# Check February valid days including leap year
A=`expr $year % 4`
B=`expr $year % 100`
C=`expr $year % 400`
if [ $A = 0 -a $B != 0 -o $C = 0 ]
then
	if [ $day -ge 30 ]
	then
		exit 1
	fi
else
	if [ $day -ge 29 ]
	then
		exit 1
	fi
fi


# validate full date
valdate -f$fmtstr $mm/$dd/$yy
