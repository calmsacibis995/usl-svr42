#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:desktop/fdscripts/custom_fs.sh	1.1.1.17"
#	Portions Copyright (C) 1990, 1991 Intel Corporation.
# 	Portions Copyright (C) 1990 Interactive Systems Corporation.
# 	All Rights Reserved

# This script loads the function FileSystem_Setup, which is called
# out of hdprepare.sh. This is only done for custom disk preparation.
# All otherfunctions here are sub-functions of FileSystem_Setup.

# Display list of required slices for disk 0 if $1 is REQ.
# Otherwise pull out optional slices, ask if the user wants to
# create them. For all slices, ask the type of file system.  

elements() {
IFS="${TAB}"
SECOND_DISK=$2
OPTION=$1
SZ=$3
while read ele name ro disk1 disk2 slice sz wflg min_sz vfst
do
   SIZE_CHECK=No
   [ "$ele" != "/" ] && {
	ans=`expr $ele : '^#'`
   	if [ $ans -ge 1 -o "$ele" = "" ];then 
      		continue
   	fi
   }

   if [ "${OPTION}" = "REQ" ];then
      if [ "$ro" = "R" ];then
         echo "0\t$name\t$ele"
         continue
      fi
   fi
   if [ "$ro" != "${OPTION}" ];then 
       continue
   fi

   if [ "${OPTION}" != "R" ];then
      ttyflushin < /dev/tty
      echo "\nCreate a slice for ${ele} (y or n)? \c"
      read ans < /dev/tty
      [ "${wflg}" = "W" ] && SIZE_CHECK=Yes
   else
      ans=Y
   fi
   while [ 1 ]
   do
      if [ "$ans" = "y" -o "$ans" = "Y" ];then
         if [ "$vfst" != "" ];then
             while [ 1 ]
             do
               IFS=", "
               set $vfst
               if [ "$1" = "$vfst" ];then 
                  ans="$1"
                  break
               fi
	       ttyflushin < /dev/tty
               echo "\nPlease select the File System Type for ${name} (${ele})"
               echo "from the following list:"
               echo "\n${vfst}\t\tdefault($1): \c"
               read ans < /dev/tty
	       [ "$ans" = "" ] && ans=$1
               case $ans in
                  bfs|s52k) break;;
                  ufs) ans="ufs4k";break;;
                  sfs) ans="sfs4k";break;;
                  vxfs) ans="vxfs1k";break;;
                  s5) ans="s51k";break;;
                   *) echo "Please enter one of $vfst" ;;
               esac
             done
	     IFS="${TAB}"
         else 
	    # /dev/swap and /dev/dump don't have fstyp
            ans="-"
         fi
         if [ "${SECOND_DISK}" = "Yes" -a "${disk2}" = "Yes" ];then
            while [ 1 ]
            do
	       ttyflushin < /dev/tty
               echo "Please select the drive upon which you wish to install \c"
               echo "${ele} (0 or 1)? \c"
               read drive < /dev/tty
               if [ "${drive}" = "" ];then 
                  drive=0
               fi
               if expr $drive : '^[0-1]$' > /dev/null;then 
                   break
               else 
                  echo "Please select 0 or 1"
               fi
            done
         else
            drive=0
         fi
         echo "${drive}\t${name}\t${ans}\t\t${ele}" >> /tmp/hd.lay
	 case $ans in
	 s51k|s52k) ans=s5;;
	 ufs4k) ans=ufs;;
	 vxfs1k) ans=vxfs;;
	 sfs4k) ans=sfs;;
	 esac
         if [ "${ele}" = "/" ];then
            echo "rootfstype=${ans}" > /tmp/ROOTFS
            echo "${ans}" >  ${ROOTFSTYPE}
         fi
         break
      elif [ "$ans" = "n" -o "$ans" = "N" ];then   
	 # Add this slice's minimum size requirement/weight to
	 # that of root. 
         if [ "${SIZE_CHECK}" = "Yes" ]
	 then
		read tsize < $SIZEFILE
	 	tsize=`expr $tsize + $sz`
	 	echo ${tsize} > $SIZEFILE
         	read tsize < $MINSZFILE
	 	tsize=`expr $tsize + $min_sz`
	 	echo ${tsize} > $MINSZFILE
	 fi
         break
      else
	 ttyflushin < /dev/tty
         echo "Please answer \"y\" or \"n\": \c"
         read ans < /dev/tty
      fi
   done
done < /etc/disk.ele${SZ}
IFS="$OFS"
}

# the user did not wish to create additional slices
# adjust /tmp/hd.0.lay file so that minimum size for root
# includes sizes for other slices. Modify the default
# size for swap in case we needed a second disk to 
# put us over the minimum 200 Mb combined size that requires
# a 10-Mb stand.

onlyroot () {

DISK1SZ=$1

MINROOTSIZE=45
MINSTANDSIZE=5
STANDSIZE=${MINSTANDSIZE}
if [ "${DISK1SZ}" -gt 150 ]
then
	STANDSIZE=10
fi

# if the 1st disk is less in size than the recommended
# minimum size for the hard disk, adjust the size of the
# root file system.
if [ "${DISK1SZ}" -lt ${RECMIN_HARDDISK} ]
then
	ADJUSTMENT=`expr ${RECMIN_HARDDISK} - ${DISK1SZ}`
	MINROOTSIZE=`expr ${MINROOTSIZE} - ${ADJUSTMENT}`
fi

# copy hd.0.lay -- backed up version is input to create new
# version of hd.0.lay
mv /tmp/hd.0.lay /tmp/hd.0.tmp

IFS="${TAB}"
while read slice ele fsty blksz sz min_sz_ro
do
	
	if [ "$ele" = "/" ];then
		ro=`expr $min_sz_ro : '.*\([RO]\)$'`
		min_sz=${MINROOTSIZE}
		min_sz_ro=${min_sz}${ro}
	fi
	if [ "$ele" = "/stand" ];then
		ro=`expr $min_sz_ro : '.*\([RO]\)$'`
		min_sz=${MINSTANDSIZE}
		min_sz_ro=${min_sz}${ro}
		sz=${STANDSIZE}M
	fi
	echo "${slice}	${ele}	${fsty}	${blksz}	${sz}	${min_sz_ro}" >> /tmp/hd.0.lay
done < /tmp/hd.0.tmp
rm -f /tmp/hd.0.tmp
IFS="$OFS"
}



# adjust the size of swap from the recommended size of 2 times memory
# (SWAPMULT) to 13% of the hard disk if that is smaller.
# (If the partition size is below RECMIN_HARDDISK then use
# RECMIN_HARDDISK as the partition size.

detune () {
MEM=$1
DISK=$2
SWAPMULT=$3

# partsize with no options writes size of unix partition
DISKSIZE=`partsize /dev/rdsk/c0t${DISK}d0s0`
[ ${DISKSIZE} -lt ${RECMIN_HARDDISK} ] && {
	DISKSIZE=${RECMIN_HARDDISK}
}

DISKMIN=`expr ${DISKSIZE} \* 13`
DISKMIN=`expr ${DISKMIN} + 99`	# Round up
DISKMIN=`expr ${DISKMIN} / 100`

MEMMIN=`expr ${SWAPMULT} \* ${MEM}`
if [ ${MEMMIN} -gt ${DISKMIN} ]
then
	MEMMIN=${DISKMIN};
fi

mv /tmp/hd.${DISK}.lay /tmp/hd.${DISK}.tmp

IFS="${TAB}"
while read slice ele fsty blksz sz min_sz_ro
do
	
	if [ "$ele" = "/dev/swap" ];then
		ro=`expr $min_sz_ro : '.*\([RO]\)$'`
		min_sz=${MEMMIN}
		min_sz_ro=${min_sz}${ro}
		sz=${min_sz}M
	fi
	echo "${slice}	${ele}	${fsty}	${blksz}	${sz}	${min_sz_ro}" >> /tmp/hd.${DISK}.lay
done < /tmp/hd.${DISK}.tmp
rm -f /tmp/hd.${DISK}.tmp
IFS="$OFS"
}

#
# Create /tmp/hd.[01].lay file for the disk. Add weights/min sizes
# for slices not selected to those of root file system.
# The hd.lay files are in format suitable for passing to disksetup.
# 
setup()
{
DISK=$1
SECOND=$2
SZ=$3
ONEMB=1048576
MEM=`memsize`
IFS="${TAB}"


if [ `expr $MEM % $ONEMB` -ne 0 ];then
	MEM=`expr $MEM / $ONEMB + 1`
else
	MEM=`expr $MEM / $ONEMB`
fi
> /tmp/hd.${DISK}.lay
while read drive name fsty ele
do
	if [ ${drive} != ${DISK} ];then 
		continue
	fi
	IFS="${TAB}"
	while read tele name ro disk1 disk2 slice sz wflg min_sz vl
	do
	if [ "$ele" = "$tele" ];then
		if [ "$ele" = "/dev/swap"  -o "$ele" = "/dev/dump" ];then
			if [ `expr $sz : '.*m$'` -eq 0 ];then
				min_sz=`expr $sz : '\(.*\)[MW]$'`
			else
				MULT=`expr $sz : '\(.*\)m$'`
				if [ "$ele" = "/dev/swap" ];then
					SWAPMULT=$MULT
				fi
				min_sz=`expr $MEM \* $MULT`
				sz=${min_sz}M
			fi
		fi
		if [ "$ele" = "/" ]; then
			read tsz < $SIZEFILE
			read tmin_sz < $MINSZFILE
			sz=`expr $sz + $tsz`
			min_sz=`expr $min_sz + $tmin_sz`
		fi
               case $fsty in
                     bfs) blksz="512";;
		    s51k) fsty="s5";blksz="1024";;
		    s52k) fsty="s5";blksz="2048";;
                   ufs4k) fsty="ufs";blksz="4096";;
                   sfs4k) fsty="sfs";blksz="4096";;
                  vxfs1k) fsty="vxfs";blksz="1024";;
                       *) blksz="-";;
               esac

	       # create hd.[01].lay for input to disksetup
	[ "${wflg}" = "N" ] && {
		 wflg=""
		export wflg
	}
		echo "${slice}	${ele}	${fsty}	${blksz}	${sz}${wflg}	${min_sz}${ro}" >> /tmp/hd.${DISK}.lay
		break;
	fi
	done < /etc/disk.ele${SZ}
done < /tmp/hd.lay

echo ". \${SCRIPTS}/common.sh" >> /tmp/hdscripts.sh
if [ -s /tmp/hd.${DISK}.lay ];then
	if [ ${DISK} = 0 ];then
    detune $MEM $DISK $SWAPMULT
                echo "disksetup -m ${MEMSIZE} -x /tmp/disksetup.sh -d /tmp/hd.${DISK}.lay -IB -b /etc/boot /dev/rdsk/c0t${DISK}d0s0" >> /tmp/hdscripts.sh
		echo "[ \"\$?\" != 0 ] && error_out unexpdisk" >> /tmp/hdscripts.sh
    else
		echo "echo \"\n\nDone preparing first disk, now preparing second disk...\"" >> /tmp/hdscripts.sh
		echo disksetup -m ${MEMSIZE} -x /tmp/disksetup.sh -d /tmp/hd.${DISK}.lay -I /dev/rdsk/c0t${DISK}d0s0 >> /tmp/hdscripts.sh
		echo "[ \"\$?\" != 0 ] && error_out unexpdisk2" >> /tmp/hdscripts.sh
	fi
else
	echo "\nYou have not allocated any slices/file systems on your second drive,"
    echo "you may run \"diskadd\" after completing installation if you wish to"
    echo "install your second drive."
fi
# don't remove it now because hdscripts wil need it
# rm -f /tmp/hd.${DISK}.lay
IFS="$OFS"
}

FileSystem_Setup () {
   TAB="	"
   SIZEFILE=/tmp/size.$$
   MINSZFILE=/tmp/minsize.$$
   MEMSIZE="`memsize`"
   P_SZ=`partsize /dev/rdsk/c0t0d0s0`
   SZ0=$P_SZ
   SZ1=0
   [ "${SECOND_DISK}" = "Yes" ] && {
      SZ1=`partsize /dev/rdsk/c0t1d0s0`
      P_SZ=`eval expr $SZ0 + $SZ1`
   }
   SMALL=""

   # if first disk is less than 100Mb in size and there is no
   # second disk, use disk.elesm as the file from which to
   # offer file system choices
   if [ ${SZ0} -lt 100 -a "${SZ1}" = "0" ]
   then
      SMALL=sm
   fi

   # if combined sizes of first and second disk exceeds 200Mb in size 
   # use disk.elebig as the file from which to offer file system choices
   [ $P_SZ -ge 200 ] && { SMALL=big; }

   echo "The following hard disk elements are required and"
   echo "must reside on your primary (disk 0) hard disk:\n"
   echo "Drive\t      Name      \tFile System/Slice"
   echo "-----\t----------------\t-----------------"
   elements REQ ${SECOND_DISK} ${SMALL}

   while [ 1 ]
   do
      > /tmp/hdscripts.sh
      > /tmp/hd.lay
      echo "0" > ${SIZEFILE}
      echo "0" > ${MINSZFILE}
      elements R ${SECOND_DISK} ${SMALL}

      ttyflushin 
      echo "\nDo you wish to create any optional"
      echo "disk slices or filesystems (y or n)? \c"
      while read ans
      do
      if [ "$ans" = "y" -o "$ans" = "Y" ];then
	 # elements call changes the value of "ans", so save/restore it
         save_ans=${ans}
         elements O ${SECOND_DISK} ${SMALL}
         ans=${save_ans}

      elif [ "$ans" != "n" -a "$ans" != "N" ];then 
         ttyflushin 
         echo "Please answer \"y\" or \"n\": \c"
           continue
      fi
      ONLYROOT=NO
      if [ "$ans" = "n" -o "$ans" = "N" ]
      then
	 # adjust size of stand based on size of first disk
	 # only, and make sure minimum for root reflects
	 # single root file system.
	 ONLYROOT=YES
      fi
      # Also force a check of root file system size if 
      # the disk.elesm file was used.  This is because
      # the size of the UNIX partition may be below 60Mb
      # (case of unformatted 60Mb partition) and thus the size
      # of root requires adjusting. We check SMALL to see if it
      # is "sm".
      [ "${SMALL}" = "sm" ] && ONLYROOT=YES
      break
      done
      ttyflushin 
      echo "\nThe Hard disk layout you have selected is:\n"
      echo "Drive\tName            \tType\t\tFile System/Slice"
      echo "-----\t----------------\t----\t\t-----------------"
      cp /tmp/hd.lay /dev/console
      echo "\nIs this correct (y or n)? \c"
      while read ans
      do
         if [ "$ans" = "y" -o "$ans" = "Y" -o "$ans" = "n" -o "$ans" = "N" ];then 
            break
         else 
            ttyflushin 
            echo "Please answer \"y\" or \"n\": \c"
         fi
      done
      if [ "$ans" = "y" -o "$ans" = "Y" ];then 
   	rm -f /tmp/disksetup.sh
   	setup 0 ${SECOND_DISK} ${SMALL}
	if [ "${ONLYROOT}" = "YES" ]
	then
		# adjust /tmp/hd.0.lay
		onlyroot ${SZ0}
	fi
   	if [ "${SECOND_DISK}" = "Yes" ];then
        	setup 1 ${SECOND_DISK} ${SMALL}
        fi
   	sync
   	echo "\n"
   	# verify that hd.0.lay will fit on first disk; save 1Mb for alternates
   	echo "1" > ${MINSZFILE}
   	IFS="${TAB}"
   	while read slice name fstyp bsiz size min_siz
   	do
		tminsz=`expr $min_siz : '\(.*\)[mMWRO]$'`
		read MINSZ < ${MINSZFILE}
		MINSZ=`expr ${MINSZ} + ${tminsz}`
		echo ${MINSZ} > ${MINSZFILE}
   	done < /tmp/hd.0.lay
   	read MINSZ < ${MINSZFILE}
        # Allow 1Mb for alternates, so if minimum sizes of slices sum
        # to the size of the disk, make 'em do it again!
   	if [ "${MINSZ}" -ge "${SZ0}" ]
   	then
	   echo "Your allocation of file systems on drive 0 will not fit."
	   echo "Please try again."
	   continue
        fi	
	# if SZ1 is 0, no second hard disk to check
   	if [ "${SZ1}" = "0" ]; then
	  break
	fi

	# otherwise, make sure hd.1.lay fits on hard drive 1; save 1Mb for alternates
   	echo "1" > ${MINSZFILE}
   	IFS="${TAB}"
   	while read slice name fstyp bsiz size min_siz
   	do
		tminsz=`expr $min_siz : '\(.*\)[mMWRO]$'`
		read MINSZ < ${MINSZFILE}
		MINSZ=`expr ${MINSZ} + ${tminsz}`
		echo ${MINSZ} > ${MINSZFILE}
   	done < /tmp/hd.1.lay
   	read MINSZ < ${MINSZFILE}
   	if [ "${MINSZ}" -ge "${SZ1}" ]
   	then
	   echo "Your allocation of file systems on drive 1 will not fit."
	   echo "Please try again."
	   continue
	fi	
	break
      fi
   done
}
