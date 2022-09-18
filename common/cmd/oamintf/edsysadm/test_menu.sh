#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/edsysadm/test_menu.sh	1.4.5.10"
#ident	"@(#)oamintf:common/cmd/oamintf/edsysadm/test_menu.sh	1.4.5.10"
#ident  "$Header: test_menu.sh 2.0 91/07/12 $"
#################################################################################
#	Module Name: test_menu
#
#	Inputs:
#		$1 -> flag to indicate menu change/add
#			- chgmenu
#			- addmenu
#		$2 -> menu name
#		$3 -> description field of menu
#		$4 -> location of menu
#		$5 -> help file
#
#	Description:
#		This process allows for online testing of menus that
#		are added or modified through edsysadm.  It will show the
#		user how the menu will appear in the logical directory
#		structure.
#
#	UNIX commands:
#		basename, cat, cp, cpio, cut, echo, expr, find, grep
#		mkdir, rm, sed, sort
################################################################################

trap cleanup 1 2 3 15

# Assign temporary files for log files and menu file
MI_FILE=${TESTBASE}/$$.mi
LOG_FILE=${TESTBASE}/$$.log
LOG_2=${TESTBASE}/$$.log2
EXPR_LOG=${TESTBASE}/$$.expr
EXPR_2=${TESTBASE}/$$.exp2
RMMODTMP=${TESTBASE}/$$rmmodtmp
# Need to try ${VPID}
ITEMNAME=${TESTBASE}/${VPID}.arg1
MENUPATH=${TESTBASE}/${VPID}.arg2
MENUNAME=${TESTBASE}/${VPID}.arg3

# Assign arguments to descriptive variables
FLAG=$1
NAME=`echo $2 | tr -d '\"'`
DESC="$3"
LOCATION=`echo $4 | tr -d '\"'`
# Check if help file is absolute or relative path
if expr "$5" : '^[^/]' > /dev/null
then
	HELP=`pwd`/$5
else
	HELP=$5
fi
HELP=`echo ${HELP} | tr -d '\"'`
# Exit Codes
SUCCESS=0
COLLISION=1
MKMF_ERR=2
DUP_STRUCT=3
MOD_MENU=4
EXITCODE=$SUCCESS

# Tools Bin
EDBIN=$OAMBASE/edbin
INSTBIN=/usr/sadm/install/bin

#################################################################################
#
#	Module Name: cleanup
#
#	Description:
#		Remove temporary file, reset OAMBASE and exit.
#################################################################################
cleanup() {

# Remove the temp files created by mod_menus if created and error exits.
if [ -s $RMMODTMP ]
then
	for i in `cat $RMMODTMP`
	do
		rm -f $i
	done
fi

if [ $EXITCODE -ne 0 ]
then
	rm -rf $TMP_PATH
fi

rm -f $RMMODTMP \
      ${TESTBASE}/loc \
      $MI_FILE \
      $LOG_FILE \
      $LOG_2 \
      $EXPR_LOG \
      $EXPR_2 2>/dev/null

exit $EXITCODE
}

#################################################################################
#
#	Module Name: coll_detect
#
#	Description: Check for collision between new additions and
#		     the existing Interface menu definition
#
#	example: main:applications:ndevices
#	
#		pathname --> $OAMBASE/menu/applmgmt
#		part1    --> $OAMBASE
#		part2    --> menu/applmgmt
#		oambase  --> /usr/sadm/sysadm (or whatever $OAMBASE is)
#		PATHNAME --> /usr/sadm/sysadm/menu/applmgmt
#################################################################################
coll_detect() {

# get the physical location of menu
pathname=`$EDBIN/findmenu -o "$LOCATION:$NAME"`            
part1=`echo "$pathname" | sed "s/^\([^\/]*\)\/.*/\1/p"`   
part2=`echo "$pathname" | sed  "s/^[^\/]*\([\/.]*\)/\1/p"`
oambase=`eval echo $part1`
testbase=`eval echo $TESTBASE`
PATHNAME=$oambase$part2
TMP_PATH=$testbase$part2

#  Change to direct name.menu path when subdirectories structure used.
#  if grep "^${NAME}\^" $PATHNAME/${NAME}.menu

if [ "$FLAG" = "addmenu" ] || [ "$FLAG" = "addtask" ]
then
	if grep "^${NAME}\^" `eval echo $PATHNAME`/*.menu
	then
		EXITCODE=$COLLISION
		cleanup
	fi
fi
}

#################################################################################
#
#	Module Name: dup_struct
#
#	Description:
#		Duplicate menu structure in $TESTBASE
#################################################################################
dup_struct(){

if [ "$TMP_PATH" ] && [ ! -f "$TMP_PATH" ] && [ ! -d "$TMP_PATH" ]
then
	mkdir -m755 -p $TMP_PATH 2>/dev/null
fi
 
cp $OAMBASE/menu/main.menu $TESTBASE/menu/main.menu

cd $PATHNAME

find . -print | cpio -pdum $TMP_PATH 2>/dev/null
}

###########################################################
# main function
#	Set pkginst to _ONLINE
#	Collision Detection
#	Menu Info File Generation
#	Duplicate affected Menu Structure
#	Reset OAMBASE
#	Modify Menus
#	Commit Changes
#	Copy help file
#	Generate parent menu
###########################################################

# Set Pkginst Variable to _ONLINE
PKGINST=_ONLINE
export PKGINST

# Collision Detection
#if [ "$FLAG" = "addmenu" ]
#then
	coll_detect
#fi

# Menu Information File Generation - uses mkmf

$EDBIN/mkmf "online" "$MI_FILE" "$NAME" "$DESC" "$LOCATION" "$HELP" 2>/dev/null 2>&1 || {
		EXITCODE=$MKMF_ERR
		cleanup
		}

# Duplicate the menu structure in $TESTBASE
dup_struct

# Modify Menus - uses mod_menus 

$INSTBIN/mod_menus -t $MI_FILE $LOG_FILE $EXPR_LOG 2>/dev/null || {
	cat $LOG_FILE | grep -v "NEWDIR" | sort -d -u | 
	     cut -f1 -d" " >$RMMODTMP
	EXITCODE=$MOD_MENU
	cleanup
	}

# Commit Changes
EXITCODE=8

# Sort log file entries and remove duplicates
sort -d -u -o $LOG_FILE $LOG_FILE 2>/dev/null || {
	cat $LOG_FILE | grep -v "NEWDIR" | 
	     cut -f1 -d" " >$RMMODTMP
	cleanup
	}

# Remove "NEWDIR" entries from log file
grep -v "NEWDIR" $LOG_FILE > $LOG_2 2>/dev/null || {
	cat $LOG_2 | cut -f1 -d" " >$RMMODTMP
	cleanup
	}

# Move temp menu file to permanent menu file in log file
sed 's/^\(.*\)$/mv \1/' $LOG_2 | tr -d '\"' > $LOG_FILE 2>/dev/null || {
	cat $LOG_2 | cut -f1 -d" " >$RMMODTMP
	cleanup
	}

# Execute log file
#. $LOG_FILE 2>/dev/null || {
. $LOG_FILE || {
	cat $LOG_2 | cut -f1 -d" " >$RMMODTMP
	cleanup
	}

# Changes completed without error
EXITCODE=$SUCCESS

# Copy help file to temporary menu structure
if [ ! -f $TMP_PATH/$NAME/HELP ]  && [ ! -d $TMP_PATH/$NAME/HELP ]
then
	mkdir $TMP_PATH/$NAME/HELP
fi

cp $HELP $TMP_PATH/$NAME/HELP

# May want to try setting global variables??
# Create 3 temporary files that will hold Menu.testmenu arguments
echo $NAME > $ITEMNAME
echo $TMP_PATH > $MENUPATH
#ASSUME THERE IS ONLY 1 '*.menu' file; is this a correct assumption?? 
#echo `basename $PATHNAME/*.menu` > $MENUNAME
LOC=`echo $LOCATION | cut -d':' -f2`
echo `basename $PATHNAME/${LOC}.menu` > $MENUNAME

cleanup
