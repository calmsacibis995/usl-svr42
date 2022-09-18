#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)initpkg:i386/cmd/initpkg/ckroot.sh	1.8"
#ident "$Header: $"


# This file has those commands necessary to check the root file
# system.


rrootfs=/dev/rroot


# Try to get the file system type for root.  If this fails,
# it will be set to null.  In that case, we hope that vfstab
# is around so that fsck will use it for the type.
set `/sbin/df -n /` > /dev/null
if [ "$3" = "" ]
then
	fstypa=""
else
	fstyp=$3
	fstypa="-F $3"
fi

remount=no

donext()
{
	case $1 in
	  0|40)	# remount the root file system only if in compatibility
		if [ ! -z "$2" ]
		then
			remount=yes
		fi
		;;
	  101)  # undconditionally remount root file system
		remount=yes
		;;

	  39)	# couldn't fix root - enter a shell
		echo "  *** ROOT COULD NOT BE REPAIRED - Entering shell"
		/sbin/sh
		# repair work has hopefully completed so reboot
		echo "  *** SYSTEM WILL REBOOT AUTOMATICALLY ***"
		uadmin 2 1
		;;

	  *)	# fsck determined reboot is necessary
		echo "ckroot: warning, return value $?"
		echo "  *** SYSTEM WILL REBOOT ***"
		uadmin 1 0
		;;
	
	esac
}

prt_msg=0

/sbin/fsck $fstypa -m ${rrootfs}  >/dev/null 2>&1

retval=$?
if [ $retval -eq 101 ]
then
	remount="yes"
elif [ $retval -ne 0 ]
then
	echo "Please wait while the system is examined.  This may take a few minutes.\n"
	prt_msg=1
	/sbin/fsck $fstypa -y ${rrootfs} > /dev/null 2>&1

	donext $? "compat"
fi

if [ ! -z "$fstyp" -a -x /etc/fs/$fstyp/ckroot ]
then
	/etc/fs/$fstyp/ckroot
	donext $?
fi

if [ $remount = "yes" ]
then
	/sbin/uadmin 4 0 
	if [ $? -ne 0 ]
	then
		echo "*** REMOUNT OF ROOT FAILED ***"
		echo "*** SYSTEM WILL REBOOT AUTOMATICALLY ***"
		/sbin/uadmin 2 1
	fi
	#echo "  *** ROOT REMOUNTED ***"
fi

if [ $prt_msg -eq 1 ]
then
	echo >/etc/.fscklog	# fsck message has been printed
else
	/sbin/rm /etc/.fscklog >/dev/null 2>&1
fi
