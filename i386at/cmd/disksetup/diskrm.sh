#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)disksetup:i386at/cmd/disksetup/diskrm.sh	1.1"

# This script is used to remove disk drives from the system and its
# correpsonding nodes from /etc/vfstab

trap 'trap "" 1 2 3 9 15;
	set +e;
	cd /;
	echo "You have canceled the diskrm program."
	rm -rf /var/spool/locks/.DISKRM.LOCK
exit 2' 1 2 3 15

if [ -f /var/spool/locks/.DISKRM.LOCK ]
then
	echo "The diskrm program is currently being run and cannot be run"
	echo "concurrently. Please retry this at a later time."
	exit 1
else
	>/var/spool/locks/.DISKRM.LOCK
fi

if [ -n "$1" ]
then
	if [ "$1" = "c0t0d0" ] || [ "$1" = "/dev/dsk/0s" ]
	then
		echo "Disk 0 not removable"
		rm -rf /var/spool/locks/.DISKRM.LOCK
		exit 1
	fi
	drive=$1
	driv_arg=$1
else
	drive="NODEV"
fi
case "$drive" in
c?t?d? | /dev/dsk/[1-9]s) 

	echo "You have invoked the diskrm utility. The purpose of this utility"
	echo "is the removal of secondary disk drives from the system and to" 
	echo "update the /etc/vfstab file. Do you wish to continue?"
	echo "(Type y for yes or n for no followed by ENTER): "
	read cont
	if  [ "$cont" != "y" ] && [ "$cont" != "Y" ] 
	then
		rm -rf /var/spool/locks/.DISKRM.LOCK
		exit 0
	fi
	/usr/bin/grep "$drive" /etc/vfstab >/dev/null 2>&1
	ret=`echo $?`
	if [ "$ret" = 0 ]
	then
		>/tmp/$$vfstab
		IFS="
"
		for i
		in `cat /etc/vfstab`
		do
			echo $i|grep $drive >/dev/null 2>&1
			ret=`echo $?`
			if [ "$ret" = 0 ]
			then
				echo "\nDo you want to delete the following entry?\n"
				echo "$i"
				echo "\n(Type y for yes or n for no and press <ENTER>):"
				read cont
				if  [ "$cont" = "n" ] || [ "$cont" = "N" ] ||
				    [ "$cont" = "" ] 
				then
					echo "$i" >>/tmp/$$vfstab
				else
					CHANGE=yes
				fi
			else
				echo $i >>/tmp/$$vfstab
			fi
		done
		IFS="	"
		if [ "$CHANGE" = "yes" ]
		then
			echo "saving /etc/vfstab to /etc/Ovfstab"
			cp /etc/vfstab /etc/Ovfstab
			echo "creating a new /etc/vfstab"
			cp /tmp/$$vfstab /etc/vfstab
		fi
		rm /tmp/$$vfstab
	fi
	;;
*)
   echo "usage: $0  cCtTdD | /dev/dsk/?s (where ? is drive number) \n";
   rm -rf /var/spool/locks/.DISKRM.LOCK
   exit 1;;
esac
rm -rf /var/spool/locks/.DISKRM.LOCK
if [ "$CHANGE" = "yes" ]
then
	echo "Diskrm for" disk $driv_arg "DONE at" `date`
else
	echo "No entries for disk $driv_arg exist in /etc/vfstab"
fi
exit 0
