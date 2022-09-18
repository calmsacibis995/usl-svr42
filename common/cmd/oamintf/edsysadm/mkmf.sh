#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/edsysadm/mkmf.sh	1.5.5.5"
#ident  "$Header: mkmf.sh 2.0 91/07/12 $"

################################################################################
#	Module Name: mkmf
#
#	Input: 
#		$1 -> flag to indicate menu/task and change/add
#			- chgmenu
#			- chgtask
#			- addmenu
#			- addtask
#			- online
#		$2 -> name of package description file 
#		      tmp file name for online flag
#		$3 -> name (changed name)
#		$4 -> description 
#		$5 -> location (changed location)
#		$6 -> help file
#		$7 -> action (Forms only)
#		$8 -> location:name (original "location:name")
#
#	Output:
#          	task -> location:name^Description of Menu Item^Help file^Action
#          	menu -> location:name^Description of Menu Item^Help file^
#
#	Processing:
#		1) Check flag and determine output destination.  Possible
#		   destinations are:
#			- *.mi file from package info file
#			- create new *.mi file
#			- online output goes to a temp file
#			- *.mi file in current directory
#		2) For change menu/task, delete original entry.
#
#		3) Create entry for menu information file
#
#		4) Check for duplicate entries.
#
#		5) Add menu information file to prototype file.
#
# 	UNIX commands:
#		grep, cut, mv, ls, wc, date, echo, pwd
#
################################################################################

# Exit values
SUCCESS=0
INVALID=1
DUP_ENTRY=2
TOO_MANY=4

# Path name where menu information file goes
MIPATH=/var/sadm/pkg/intf_install/\$PKGINST

# try to create the MIPATH directory, if necessary (added 04/09/92)
prepath=""
f_num=2
T_MIPATH=`eval echo \`expr $MIPATH\``
while :
do
	dir=`echo $T_MIPATH | cut -d'/' -f1-${f_num}`
	if [ "$dir" = "/" ] || [ "$dir" = "$prevpath" ]
	then
		break
	fi

	if [ ! -d "$dir" ]
	then
		mkdir $dir 2>/dev/null
		chmod 755 $dir 2>/dev/null
		chgrp sys $dir 2>/dev/null
		chown root $dir 2>/dev/null
		prevpath=$dir
	fi
	f_num=`expr $f_num + 1`
done

# Temporary menu information buffer
TMP_MI=$$mi

# Menu/task logical location
LOC="$5:$3"

# Assign arguments to descriptive variables
FLAG="$1"
MI_FILE="$2"
NAME="$3"
DESCRIP="`echo $4 | tr '\005' '\040'`"
LOCATION="$5"
HELP="$6"
ACTION="$7"
ORIGLOC="$8"

# Tools bin
EDBIN=$OAMBASE/edbin

##########################################################################
# The following code checks flag options and determines the destination
# of the menu information entries.
##########################################################################

if [ "$FLAG" = "chgmenu" ] || [ "$FLAG" = "chgtask" ]
then
	# delete original entry from menu information file
	grep -v "^$ORIGLOC^" $MI_FILE > ${TESTBASE}/$$mifile
	mv ${TESTBASE}/$$mifile  $MI_FILE
		
elif [ "$FLAG" = "addmenu" ] || [ "$FLAG" = "addtask" ]
then
	# must be addmenu or addtask
	# find number of menu information file exist in current directory
	filecnt=`ls *.mi 2> /dev/null | wc -w`

	# Does a menu information file exist??
	if [ $filecnt -eq 0 ]
	then
		#   Set variable to hour, minute, second, day-of-year
		#   and year to guarantee uniqueness of the .mi file

		MI_FILE="`date +%\H%\M%\S'%j%y'`.mi"

	# Check for multiple menu information files
	elif [ $filecnt -gt 1 ]
	then
		exit $TOO_MANY
	
	# Only 1 menu information file
	else
		MI_FILE=`ls *.mi`

	fi
fi

########################   END FLAG OPTION CHECK    #################################

# Check for duplicate entries

#if [ -r *.mi ]
if [ -r $MI_FILE ]
then
	grep "^$LOC^" $MI_FILE > /dev/null && exit $DUP_ENTRY
fi

####################################################################
#   Append entry to menu information file
#   EXAMPLE ENTRY:
#          location:name^Description of Menu Item^Help file^Action
#   Only task items have action files, for menu items the action
#
#   file does not exist
####################################################################
#

# REMOVED THE HELP FILE FOR NOW - 8/8/88
#echo ""$LOCATION:$NAME"^"$DESCRIP"^"$HELP"^"$ACTION"" >> $MI_FILE
echo ""$LOCATION:$NAME"^"$DESCRIP"^"$ACTION"" >> $MI_FILE


####################################################################################
# Add menu infomation file (*.mi) to prototype file
# EXAMPLE:
#	 1              2    3               4               5           6
#   mkpf prototype file flag "pkgdesc file" "location:name" "help file" "comma separated file list"
####################################################################################

if [ -w "$MI_FILE" ]
then
	grep $MIPATH prototype >/dev/null 2>&1
	if [ $? -ne 0 ]
	then

	  $EDBIN/mkpf prototype "mifile" "" "$MIPATH" "" "$OAMBASE/$PKGINST/save/intf_install/$MI_FILE" >/dev/null 2>&1 || exit
	fi
fi

exit $SUCCESS
