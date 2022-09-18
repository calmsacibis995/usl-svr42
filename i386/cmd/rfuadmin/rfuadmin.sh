#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)rfuadmin:rfuadmin.sh	1.12.14.3"
#ident  "$Header: rfuadmin.sh 1.3 91/07/01 $"

COPYRIGHT="
#ident	\"@(#)rfuadmin:rfuadmin.sh	1.12.14.3\"
#ident  \"$Header: rfuadmin.sh 1.3 91/07/01 $\"
"

echo "
${COPYRIGHT}

# executed by rfudaemon on request from another system.
# this is a filter that allows other systems to execute
# selected commands. This example is used by the fumount
# command when a remote system is going down.
# System administrators may expand the case list if it is
# desired to add to the remote command list.


if [ -d /var/adm/net/servers/rfs ]
then	LOG=/var/adm/net/servers/rfs/rfuadmin.log
else	LOG=/var/adm/rfuadmin.log
fi

echo \`date\` \$* >>\$LOG
case \"\$1\" in
'fuwarn' | 'disconnect' | 'fumount')
	TYPE=\$1
	RESRC=\$2
	GRACE=\${3:-0} #by default, zero
	;;
'error')
	echo \$* >>\$LOG
	echo \$* >/dev/console 2>&1
	exit 1
	;;
*)
	echo \"unexpected command \\\"\$*\\\"\"	>>\$LOG
	echo \"unexpected command for \$0 \\\"\$*\\\"\"	>/dev/console 2>&1
	exit 1
	;;
esac

# RESRC is of the form domain.resource  If bad format, then no match below.
DofR=\`	echo \$RESRC | sed -e 's/\\..*//'\`	# domain of the resource
Rbase=\`	echo \$RESRC | sed -e 's/.*\\.//'\`	# resource name only
Dom=\`/usr/sbin/dname\`				# this domain

(
        mldmode >/dev/null 2>&1
        exit \$?
)
if [ \"\$?\" = \"0\" ]
then	Mopt=\"-v -z\"	#secure system
else	Mopt=\"-v\"	#non-secure
fi

match=\`/sbin/mount \$Mopt  |
	while read	SPECIAL on MOUNTPT type ftype MODE \\
			on dow mon dom time year LEVEL ignored

				# Note:	LEVEL field assigned \"\" on
				#	non-secure systems
	do
		if [ \"\${SPECIAL}\" = \"\${RESRC}\" ]
		then
			# if the full name is in the mount table, it is unique.
			echo \$MOUNTPT \$SPECIAL \$MODE \$LEVEL
			exit 0
		else
			# otherwise,
			# if the domain of this machine is the same
			# as the that of the resource, it may be
			# mounted without the domain name specified.
			# Must be careful here, because if the resource
			# is 'XXX', we may find 'other_domain.XXX'
			# as well as 'XXX' in the mount table.

			if [ \"\$DofR\" = \"\$Dom\" ]
			then
				if [ \"\${SPECIAL}\" = \"\${Rbase}\" ]
				then
					echo \$MOUNTPT \$SPECIAL \$MODE \$LEVEL
					exit 0
				fi
			fi
		fi
	done
\`
if [ \"\$match\" = \"\" ]
then	exit 0		# it's not mounted
else	set \$match
fi


Mp=\${1}		# mountpoint
Rsc=\${2}	# 'domain.resource' or 'resource' 
Mode=\${3}	# 'read/write/remote' or 'read'
Level=\${4}	# macceiling level (on secure systems)

msg_fuwarn=\"\$Mp will be disconnected from the system in \$GRACE seconds.\"
msg_discon=\"\$Mp has been disconnected from the system.\"
msg_fumoun=\"\$Mp is being disconnected from the system NOW!\"

case \${TYPE} in
'fuwarn') #The fumount(1M) warning from a host
	if [ \"\$GRACE\" != \"0\" ]
	then
		echo \"\$msg_fuwarn\" >> \$LOG
		echo \"\$msg_fuwarn\" | /usr/sbin/wall 
	fi
	exit 0
	;;
'disconnect' | 'fumount')
	if [ \"\$TYPE\" = \"disconnect\" ]
	then	msg=\"\$msg_discon\"
	else	msg=\"\$msg_fumoun\"
	fi
	echo \"\$msg\" >> \$LOG
	echo \"\$msg\" | /usr/sbin/wall 
	/usr/sbin/fuser -k \$Rsc >>\$LOG 2>&1
	# wait for the signalled processes to die
	sleep 5
	echo	/sbin/umount -d \$Rsc >>\$LOG
		/sbin/umount -d \$Rsc

	# for automatic remount, ...
	sleep 10
	case \"\$Mode\" in
	*write*)ro=\"\"	;;
	*)	ro=\"r\"	;;
	esac
	if [ \"\$Level\" = \"\" ]
	then	Lopt=\"\"
	else	Lopt=\"-l \$Level\"
	fi
	echo	sh /usr/sbin/rmount -d\$ro \$Lopt \$Rsc \$Mp >>\$LOG
		sh /usr/sbin/rmount -d\$ro \$Lopt \$Rsc \$Mp &
	exit 0
	;;
esac" >rfuadmin
