/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)face:OBJECTS/Text.h	1.6.4.3"
#ident  "$Header: Text.h 1.7 91/10/15 $"

title=$ARG1
lifetime=shortterm

init="$RETVAL"

framemsg=$$uxface:321:"Press the CANCEL function key to cancel."

`$VMSYS/bin/nls_expand $VMSYS/HELP/$ARG2 | set -l HELPFILE;
test $RET = 0 && set -l RETVAL=true || set -l RETVAL=false;
regex -e -v "$RETVAL" 
	'^false$' '`message "$$uxface:354:No HELP text is available for this item."`'`

text="`readfile $HELPFILE`"
columns=`longline | set -l LL;
if [ "${LL}" -gt "${DISPLAYW}" ];
then
	echo ${DISPLAYW};
else
	echo ${LL};
fi`

name=""
button=1
action=nop

name=$$uxface:355:"CONTENTS"
button=8
action=OPEN MENU OBJECTS/Menu.h0.toc
