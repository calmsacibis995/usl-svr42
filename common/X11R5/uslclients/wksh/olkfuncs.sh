#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1990, 1991 AT&T and UNIX System Laboratories, Inc.
#	All Rights Reserved
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	and UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)wksh:olkfuncs.sh	1.4"

#
# Addbuttons - add simple buttons with callbacks to another widget
#
# usage: addbuttons $PARENT Label1 Callback1 Label2 Callback2 ...
#
# For example: addbuttons $MENU_MP Open do_Open Save do_Save Quit "exit"

addbuttons() {
	typeset W nowidget="" TMP=""

	if [ x"$1" = "x-w" ]
	then nowidget=false
	     shift
	fi
	W=$1
	shift
	while [ $# -gt 1 ]
	do
		if [ "$nowidget" = false ]
		then 
			cmw TMP "$1" flatButtons "$W" \
				itemFields:{label} \
				items:{"$2"}
			sv $TMP selectProc:"$3"
			eval "$1=$TMP"
			shift 3
		else 
			cmw TMP TMP flatButtons "$W" \
				itemFields:{label} \
				items:{"$1"}
			sv $TMP selectProc:"$2"
			shift 2
		fi
	done
}


# Usage: addfields [-m] $HANDLE [ varname [mnemonic] label verify length ... ]
#
# The mnemonic is only allowed with the -m option.
#
# Adds single line captioned textField widgets to $HANDLE

addfields() {
	typeset mne=false TMP=""

	if [ "$1" = "-m" ]
	then mne=true
	     shift
	fi
	W="$1"

	shift
	while [ $# -gt 3 ]
	do
		if [ "$mne" = true ]
		then
			cmw "TMP" caption caption "$W" label:"$3" \
				mnemonic:"$2" $CAPARGS
			eval "$1_CAP=$TMP"
			cmw "$1" "$1" textField "$TMP" \
				verification:"$4" charsVisible:$5 \
				$TFARGS
			if [ "$TFOPTS" ]
			then
				TextFieldOp $TFOPTS `eval echo '$'$1`
			fi
			shift 5
		else
			cmw "TMP" caption caption "$W" label:"$2" $CAPARGS
			eval "$1_CAP=$TMP"
			cmw "$1" "$1" textField "$TMP" \
				verification:"$3" charsVisible:$4 \
				$TFARGS
			if [ "$TFOPTS" ]
			then
				TextFieldOp $TFOPTS `eval echo '$'$1`
			fi
			shift 4
		fi
	done
}

#
# Create a popupWindow shell conveniently
#
# usage: mkpopup VAR [ ARGS ... ]
#

mkpopup() {
	typeset VAR=$1
	shift
	cps "$VAR" "$VAR" popupWindowShell $TOPLEVEL $*
}

#
# Create a toplevel popup shell conveniently
#
# usage: mktoplevel VAR [ ARGS ... ]
#

mktoplevel() {
	typeset VAR=$1
	shift
	cps "$VAR" "$VAR" topLevelShell $TOPLEVEL $*
}

under() { # Widget [offset]
	echo "yRefWidget:$1 yAddHeight:true yOffset:${2:-0}"
}

rightof() { # Widget [offset]
	echo "xRefWidget:$1 xAddWidth:true xOffset:${2:-0}"
}

floatright() {	# no args
	echo xAttachRight:true xVaryOffset:true
}

floatbottom() {	# no args
	echo yAttachBottom:true yVaryOffset:true
}

spanwidth() {	# no args
	echo xAttachRight:true xVaryOffset:false
}

spanheight() {	# no args
	echo yAttachBottom:true yVaryOffset:false 
}

#
# Create a notice that warns about something, with an OK button
#

warn() {
	if [ "$_WARNWIDGET" = "" ]
	then cps _WARNWIDGET Warning noticeShell $TOPLEVEL
	     addbuttons $_WARNWIDGET_CA OK :
	fi
	sv $_WARNWIDGET_TA string:"$1"
	XtPopup $_WARNWIDGET GrabExclusive
}

#
# Create a notice that errors about something, with an OK button,
# then exits when the ok is hit.
#

fatal() {
	cps _FATALWIDGET Fatal noticeShell $TOPLEVEL
	addbuttons $_FATALWIDGET_CA OK "exit 1"
	sv $_FATALWIDGET_TA string:"$1"
	XtPopup $_FATALWIDGET GrabExclusive
}

#
# Create a notice that asks for confirmation.
#
# -y "YES-STRING"   sets a string other than "YES" for confirm button
# -n "NO-STRING"    sets a string other than "NO" for deny button
#
# $1 = string indicating the nature of the decision
# $2 = a function to call if the user says "YES"
# $3 = a function to call if the user says "NO"
#

confirm() {
	typeset tmp YES="YES" NO="NO"

	if [ x"$1" = "x-y" ]
	then YES=$2; shift 2
	fi
	if [ x"$1" = "x-n" ]
	then NO=$2; shift 2
	fi
	if [ "$_CONFWIDGET" = "" ]
	then cps _CONFWIDGET Confirm noticeShell $TOPLEVEL
	     sv $_CONFWIDGET_CA layoutType:fixedrows
	     addbuttons -w $_CONFWIDGET_CA \
		_CONFWIDGET_YES YES : \
		_CONFWIDGET_NO  NO  :
	fi
	sv $_CONFWIDGET_TA string:"$1"
	sv $_CONFWIDGET_YES selectProc:"$2" label:"$YES"
	sv $_CONFWIDGET_NO selectProc:"$3" label:"$NO"
	XtPopup $_CONFWIDGET GrabExclusive
}

#
# The following functions allow for simple creation of a set of
# text fields that prompt for a date and validate it.
#

set -A MONTHS 0 31 28 31 30 31 30 31 31 30 31 30 31

blankdate() {
	case "$*" in
	''|' '|'  ') return 0 ;;
	*) return 1 ;;
	esac
}

verify_month() {
	typeset -Z2 string

	gv $CB_WIDGET string:string
	sv $CB_WIDGET string:"$string"
	if blankdate "$string"
	then 
		if [ "$CALL_DATA_REASON" = return ]
		then
			focmv $1 nextfield
		fi
		return 0
	fi
	if ((string<1 || string>12))
	then warn "Bad month: should be between 01 and 12"
	     focmv $CB_WIDGET immediate
	fi
}

set -A MONTHS 0 31 28 31 30 31 30 31 31 30 31 30 31

verify_day() {
	typeset -Z2 string
	typeset month year max

	gv $CB_WIDGET string:string
	sv $CB_WIDGET string:"$string"
	if [ ! "$string" -o "$string" = " " -o "$string" = "  " ]
	then return 0
	fi
	monwid=$1
	yearwid=$2
	gv $monwid string:month
	gv $yearwid string:year
	if [ ! "$month" ]
	then
		max=31
	else
		max=${MONTHS[$month]}
		if [ ! "$year" ]
		then
			if ((month==2))
			then let max=max+1
			fi
		elif ((month==2 && (year%4) == 0))
		then let max=max+1
		fi
	fi
	if ((string < 1 || string > max))
	then warn "Bad day: should be between 01 and $max"
	     focmv $CB_WIDGET immediate
	fi
}

verify_year() {
	typeset -Z2 string

	gv $CB_WIDGET string:string
	sv $CB_WIDGET string:"$string"
	# should really re-verify day and month here
}

#
# usage: adddatefields $PARENT [VAR LABEL ...]
#
# Creates a captioned set of textfields (caption given by LABEL).
# Three textfields are created, named $VAR_MM, $VAR_DD, and $VAR_YY
# that hold the day, month, and year of the date.
#
# The variable $CAPARGS is used to provide additional options on the
# caption widget.  The variable $TFOPTS can be used to provide
# TextFieldOp options on the text fields.  By default they validate
# for legal numbers.

adddatefields() {
	typeset WID=$1 datetmp t1 t2 t3

	shift
	while [ "$#" -gt 1 ]
	do
		cmw datetmp caption caption $WID label:"$2" $CAPARGS

		eval "$1_CAP=$datetmp"
		cmw ca ca controlArea $datetmp hPad:0 hSpace:0 vPad:0
		cmw $1_MM $1_MM textField $ca charsVisible:2 maximumSize:2
		cmw datetmp st staticText $ca string:"-"
		cmw $1_DD $1_DD textField $ca charsVisible:2 maximumSize:2
		cmw datetmp st staticText $ca string:"-"
		cmw $1_YY $1_YY textField $ca charsVisible:2 maximumSize:2
		eval "t1=\$$1_MM; t2=\$$1_DD; t3=\$$1_YY"
		TextFieldOp $TFOPTS -t -v '[0-9]' $t1 $t2 $t3
		acb $t1 verification "verify_month $t3"
		acb $t2 verification "verify_day $t1 $t3"
		acb $t3 verification "verify_year"

		shift 2
	done
}

#
# Addrows: add single row controlAreas to a parent.
#
# usage: addrows $PARENT [VAR ...]
#
# This is mainly useful to motif/open look compatibility.

addrows() { # $1 = parent widget handle, rest of args are variable names to set
	typeset W=$1 i tmp

	shift
	for i
	do
		cmw "$i" row controlArea "$W" layoutType:fixedrows
	done
}

#
# Addcols: add single column controlAreas to a parent.
#
# usage: addcols $PARENT [VAR ...]
#
# This is mainly useful to motif/open look compatibility.

addcols() { # $1 = parent widget handle, rest of args are variable names to set
	typeset W=$1 i tmp

	shift
	for i
	do
		cmw "$i" col controlArea "$W" layoutType:fixedcols
	done
}
