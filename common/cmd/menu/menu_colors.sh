#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)menu.cmd:menu_colors.sh	1.1"
#ident	"$Header: $"

CMD_NAME=menu_colors

#
#  Usage statement. Mostly not necessary, as we assume menu_colors()
#  will be used primarily by well-designed scripts, but its good for
#  debugging anyway.
#
usage(){
echo "Usage: ${CMD_NAME} regular  (sets menu colors to REGULAR screen colors)"
echo "   or: ${CMD_NAME} warn     (sets menu colors to WARNING screen colors)"
echo "   or: ${CMD_NAME} error    (sets menu colors to ERROR screen colors)"
return 1
}

#
#  Color definitions - Right out of curses.h
#
COLOR_BLACK=0
COLOR_RED=1
COLOR_GREEN=2
COLOR_YELLOW=3
COLOR_BLUE=4
COLOR_MAGENTA=5
COLOR_CYAN=6
COLOR_WHITE=7
A_NORMAL=0
A_BLINK=524288
A_STANDOUT=65536
A_REVERSE=262144

#
#  This is the function that will be available to the shell that .'s
#  this script in.  Sets the menu colors in the environment to be correct
#  for any of: 'regular', 'warn', and 'error'.
#

menu_colors() {
	#
	#  Check for valid number of args.  Validity check is by default
	#  at the end of this routine.
	#
	if [ "$1" = "" ]
	then
		usage
	fi


	#
	#  Defaults.  Set these values to yError erganization's favorites!
	#
	REG_FG=${COLOR_WHITE}		#  Regular screen foreground color
	REG_BG=${COLOR_BLUE}		#  Regular screen background color
	REG_ATTR=${A_NORMAL}		#  Regular screen mono attribute

	WARN_FG=${COLOR_WHITE}		#  Warning screen foreground color
	WARN_BG=${COLOR_MAGENTA}	#  Warning screen background color
	WARN_ATTR=${A_STANDOUT}		#  Warning screen mono attribute

	HELP_FG=${COLOR_BLACK}		#  Help screen foreground color
	HELP_BG=${COLOR_CYAN}		#  Help screen background color
	HELP_ATTR=${A_NORMAL}		#  Help screen mono attribute

	ERROR_FG=${COLOR_WHITE}		#  Error screen foreground color
	ERROR_BG=${COLOR_RED}		#  Error screen background color
	ERROR_ATTR=${A_STANDOUT}	#  Error screen mono attribute
	#
	#  Case for setting up the attributes for a REGULAR screen
	#
	if [ "$1" = "regular" -o "$1" = "REGULAR" -o "$1" = "Regular" ]
	then
		REG_FG=${REG_FG}
		REG_BG=${REG_BG}
		REG_ATTR=${REG_ATTR}
		export REG_FG REG_BG ERROR_FG ERROR_BG HELP_FG HELP_BG
		export REG_ATTR WARN_ATTR HELP_ATTR ERROR_ATTR
		return 0
	fi

	#
	#  Case for setting up the attributes for an WARNING screen
	#
	if [ "$1" = "warn" -o "$1" = "WARN" -o "$1" = "Warn" ]
	then
		REG_FG=${WARN_FG}
		REG_BG=${WARN_BG}
		REG_ATTR=${WARN_ATTR}
		export REG_FG REG_BG ERROR_FG ERROR_BG HELP_FG HELP_BG
		export REG_ATTR WARN_ATTR HELP_ATTR ERROR_ATTR
		return 0
	fi

	#
	#  Case for setting up the attributes for an ERROR screen
	#
	if [ "$1" = "error" -o "$1" = "ERROR" -o "$1" = "Error" ]
	then
		REG_FG=${ERROR_FG}
		REG_BG=${ERROR_BG}
		REG_ATTR=${ERROR_ATTR}
		export REG_FG REG_BG ERROR_FG ERROR_BG HELP_FG HELP_BG
		export REG_ATTR WARN_ATTR HELP_ATTR ERROR_ATTR
		return 0
	fi

	#
	#  If we got this far, we had a bad argument.
	#
	usage
}
