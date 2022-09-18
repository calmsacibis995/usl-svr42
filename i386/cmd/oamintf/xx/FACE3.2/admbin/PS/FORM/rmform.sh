#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.



#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/admbin/PS/FORM/rmform.sh	1.1.1.2"
#ident	"$Header: $"
allforms=`echo $1 | /usr/bin/sed 's/,/ /g'`
if [ "$allforms" = "all" ]
	then 
	for form in `/usr/bin/ls /usr/spool/lp/admins/lp/forms`
	do
		/usr/lib/lpforms -f $form -x
		/usr/bin/rm -rf /usr/vmsys/OBJECTS/PS/FORM/alnames
	done
else
	for form in $allforms
	do
		/usr/lib/lpforms -f $form -x
		if [ "`/usr/bin/grep \"^$form\" /usr/vmsys/OBJECTS/PS/FORM/alnames`" 2>/dev/null != "" ] 
			then
			/usr/bin/ed - /usr/vmsys/OBJECTS/PS/FORM/alnames <<-eof 2>/dev/null
			/$form/d
			w
			q
			eof
		fi
	done

fi
