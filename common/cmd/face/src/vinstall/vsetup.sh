#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)face:src/vinstall/vsetup.sh	1.7.5.5"
#ident  "$Header: vsetup.sh 1.7 91/11/14 $"
ferror()
{
	echo "$*" ; exit 1
}
set -a

LOGINID=${1}
service=`echo ${2}|cut -c1`
autoface=`echo ${3}|cut -c1`
shell_esc=`echo ${4}|cut -c1`

LOGDIR=`sed -n -e "/^${LOGINID}:/s/^.*:\([^:][^:]*\):[^:]*$/\1/p" < /etc/passwd`
if [ ! -d "${LOGDIR}" ]
then
	gettxt uxface:2 "${LOGINID}'s home directory doesn't exist"
	echo
	exit 1
fi

VMSYS=`sed -n -e '/^vmsys:/s/^.*:\([^:][^:]*\):[^:]*$/\1/p' < /etc/passwd`
if [ ! -d "${VMSYS}" ]
then
	gettxt uxface:31 "The value for VMSYS is not set."
	echo
	exit 1
fi

UHOME=`grep -s "^$LOGINID:" /etc/passwd | cut -f6 -d:`
if [ -z "${UHOME}" ]
then
	gettxt uxface:44 "\n${LOGNID}'s home directory has not been retrieved correctly."
	echo
	exit 1
fi

GRPNAME=`grep -s "^$LOGINID:" /etc/passwd 2> /dev/null | cut -f4 -d: `
if [ -z "${GRPNAME}" ]
then 
	gettxt uxface:43 "\n$LOGINID's group is not in /etc/group.\n"
	exit 1
fi

cp $VMSYS/standard/.faceprofile ${UHOME} || ferror `gettxt uxface:6 "Can't access $LOGINID's home directory"`
chmod 644 ${UHOME}/.faceprofile || ferror `gettxt uxface:6 "Can't access $LOGINID's home directory"`
chown  ${LOGINID} ${UHOME}/.faceprofile || ferror `gettxt uxface:6 "Can't access $LOGINID's home directory"`
chgrp ${GRPNAME} ${UHOME}/.faceprofile || ferror `gettxt uxface:6 "Can't access $LOGINID's home directory"`

( mldmode > /dev/null 2>&1 )
if [ "$?" = "0" ]
then
	MY_TFADMIN="$TFADMIN"
else
	MY_TFADMIN=""
fi

if [ -n "${MY_TFADMIN}" ];
then
	L_NAME=`/usr/bin/logins -oh -l ${LOGINID} | cut -d: -f6-7 `;

	if [ -z ${L_NAME} ];
	then
		L_NAME="user_login";
	fi ;

	if [ -f  /sbin/chlvl ] ;
	then
	/sbin/chlvl ${L_NAME} ${UHOME}/.faceprofile || ferror `gettxt uxface:6 "Can't access $LOGINID's home directory"` ;
	fi;
fi;

cd ${UHOME}     
for dir in WASTEBASKET pref tmp bin
do
	if [ ! -d ${dir} ]
	then
		mkdir ${dir} || ferror `gettxt uxface:8 "Can't create $dir in  $LOGINID's home directory"`
		gettxt uxface:51 "\t${dir} directory has been created for ${LOGINID}"
		echo
		chgrp ${GRPNAME} ${dir}
		chown ${LOGINID} ${dir}
		chmod 755 ${dir}

		if [ -n "${MY_TFADMIN}" -a -f /sbin/chlvl ] ;
		then
			/sbin/chlvl ${L_NAME} ${dir} ;
		fi;
	else
		gettxt uxface:50 "\t${dir} directory already exists"
		echo
	fi
done

if [ ! -f ${UHOME}/pref/services ]
then
	echo '#3B2-4I1' > ${UHOME}/pref/services
	chmod 644 ${UHOME}/pref/services
	chown  ${LOGINID} ${UHOME}/pref/services
	chgrp ${GRPNAME} ${UHOME}/pref/services

	if [ -n "${MY_TFADMIN}" -a -f /sbin/chlvl ] ;
	then
		/sbin/chlvl ${L_NAME} ${UHOME}/pref/services ;
	fi;
fi

if [ ! -f ${UHOME}/.profile ]
then
	if [ -f /etc/stdprofile ]
	then
		cp /etc/stdprofile ${UHOME}/.profile
	else
		touch ${UHOME}/.profile
	fi
	chmod 644 ${UHOME}/.profile
	chown  ${LOGINID} ${UHOME}/.profile
	chgrp ${GRPNAME} ${UHOME}/.profile

	if [ -n "${MY_TFADMIN}" -a -f /sbin/chlvl ] ;
	then
		/sbin/chlvl ${L_NAME} ${UHOME}/.profile ;
	fi;
fi

$VMSYS/bin/chkperm -${autoface} invoke -u ${LOGINID} 2>&1 || ferror `gettxt uxface:41 "You must be super-user to set the FACE permissions for $LOGINID."`

$VMSYS/bin/chkperm -${service} admin -u ${LOGINID} 2>&1 || ferror `gettxt uxface:41 "You must be super-user to set the FACE permissions for $LOGINID."`

$VMSYS/bin/chkperm -${shell_esc} unix -u ${LOGINID} 2>&1 || ferror `gettxt uxface:41 "You must be super-user to set the FACE permissions for $LOGINID."`

#if grep '^\. \$HOME/\.faceprofile$' ${UHOME}/.profile > /dev/null
if cat ${UHOME}/.profile | grep '^\. \$HOME/\.faceprofile$' > /dev/null
then
	exit 0
else
	echo '. $HOME/.faceprofile' >> ${UHOME}/.profile
fi

exit 0
