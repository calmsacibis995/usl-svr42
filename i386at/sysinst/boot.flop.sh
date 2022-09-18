#!/usr/bin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:boot.flop.sh	1.9.2.25"

CMD=$0
DESCRIPTION="Script to make the boot floppy images"

Usage(){
	echo "Usage: ${CMD}  -[k|K] | -c"
	echo "Usage: ${CMD} -h #for help info"
	test "$1" = "noargs" && {
		echo "Usage: No arguments specified."
	}
	test "$1" = "help" && {
		echo "At least one option required."
		echo
		echo ${CMD} - ${DESCRIPTION}
		echo
		test -f ${2:-Not-Set} && {
			grep '##USAGE##' $2 | 
			while read opt xxx descript
			do
				case ${opt} in
				-*)	
					opt=`echo $opt | sed 's/)//p'`
					echo "$opt	# $descript"
					;;
				esac
			done
		}
		echo
	}
	exit 1
}

Kflag=0
cflag=0
kflag=0
kernel=0
command=0

set -- `getopt ckKh? $*` || Usage

for i in $*
do
	case $i in
		-h)	##USAGE## This extended usage information
			Usage help `type ${CMD}|awk '{print $3}'`
			shift;;
		-c)	##USAGE## Build commands
			command=1;
			shift;;
		-k)	##USAGE## Build non kdb kernel
			kernel=1;
			kflag=1
			SYMS="-l /tmp/symlist"
			KDB=nokdb
			shift;;
		-K)	##USAGE## Build kdb kernel
			kernel=1;
			Kflag=1
			SYMS=""
			KDB=kdb
			shift;;
		--)	shift; break;;
	esac
done

if [ $Kflag -eq 1 -a $kflag -eq 1 ]
then
	Usage
fi
if [ $command -eq 0 -a $kernel -eq 0 ]
then
	Usage noargs
fi

#this script will create the mini kernel, build special boot flop
#only commands, and prep the executables included on the boot
#floppies. The shell script cutmedia must be used to create
#the boot floppies.

test -z "$ROOT" && {
	echo "ROOT not set"
	exit 1
}
test -z "$WORK" && {
	echo "WORK not set. For idbuild of mini-kernel,"
	echo "we're going to look in " $ROOT "/usr/src/\$WORK/uts"
	exit 1
}
test -z "$MACH" && {
	echo "MACH not set"
	exit 1
}
test -z "$PFX" && {
	echo "PFX not set"
	exit 1
}

#CPIODEBUG=v



MakeList()
{
	listfile=$1
	tmpdir=$2
	HERE=`pwd` #save where we're at
	mkdir $tmpdir
	cd $ROOT/$MACH

	for file in `grep -v "^#" $listfile`
	do
   	   if [ -f ${file}.dy ]
   	   then
		f_pstamp ${file}.dy
		echo ${file}.dy | cpio -pdum${CPIODEBUG} $tmpdir >/dev/null 2>&1
		mv $tmpdir/${file}.dy $tmpdir/${file}
   	   else
		[ -f $file ] || {
			echo cant find file $file
			exit 1
		}
		f_pstamp ${file}
		echo ${file} | cpio -pdum${CPIODEBUG} $tmpdir >/dev/null 2>&1
   	   fi
	done
	cd $HERE
}

test -f cmd/mini_kernel.sh || {
	echo "stop: cmd/mini_kernel.sh not found"
	echo "You should be in the directory $PROTO"
	exit 1
}

test -d $ROOT/.$MACH && {
	mv $ROOT/.$MACH $ROOT/.$$ || exit $?
	/usr/bin/nohup rm -rf $ROOT/.$$ > /dev/null &
}

#
# If we have a pstamp command use it, otherwise just strip the
# comment section using mcs(1)
#
NM=${PFX}nm
#
# PSTAMP_REL & PSTAMP_LOAD are used for pstamping only and are 
# not needed for builds outside of USL official Integration.
#
PSTAMP_REL=SVR4.2
PSTAMP_LOAD=${PSTAMP_LOAD}
export PSTAMP_REL PSTAMP_LOAD NM
type pstamp >/dev/null 2>&1 
if [ $? -eq 0 ]
then
	f_pstamp(){
	if [ -w $1 ]
	then
		pstamp -p unix -r ${PSTAMP_REL} -l ${PSTAMP_LOAD} -t i386 $1
	else
		echo "WARNING: Cannot write file $file, not pstamping"
	fi
	}
else
	f_pstamp(){ 
	if [ -w $1 ]
	then
		${PFX}mcs -d $1
	else
		echo "WARNING: Cannot write file $file, not mcs'ing"
	fi
	}
fi

if [ $kernel -eq 1 ]
then
echo "Creating a temporary kernel build tree in $ROOT/.$MACH .."
for localdir in mdevice.d sdevice.d cf.d bin
do
	mkdir -p $ROOT/.$MACH/etc/conf/$localdir
done


for dirs in $ROOT/$MACH/etc/conf/*
do
	dbase=`basename $dirs`
	test -d $ROOT/.$MACH/etc/conf/$dbase && continue
	ln -s $dirs $ROOT/.$MACH/etc/conf/$dbase
done

for bincmd in $ROOT/$MACH/etc/conf/bin/*
do
	bcmd=`basename $bincmd`
	ln -s $bincmd $ROOT/.$MACH/etc/conf/bin/$bcmd
done

find $ROOT/$MACH/etc/conf/sdevice.d -type f -print | 
	xargs -i cp {} $ROOT/.$MACH/etc/conf/sdevice.d &

find $ROOT/$MACH/etc/conf/mdevice.d -type f -print | 
	xargs -i cp {} $ROOT/.$MACH/etc/conf/mdevice.d &

cp $ROOT/$MACH/etc/conf/cf.d/* $ROOT/.$MACH/etc/conf/cf.d &

# Wait for the above copies to complete before proceding.
wait


(
cd  $ROOT/$MACH/etc/conf/modnew.d/
for i in bfs ufs sfs vxfs dcd adsc dpt wd7000 ict athd mcesdi mcst
do 
   ${NM} $i | grep UNDEF | sed -e 's/.*|//' > /tmp/${i}list
done
)

cat desktop/staticlist | sed '/#/D' | sed '/^$/D'  > /tmp/staticlist

for i in bfs ufs sfs vxfs dcd adsc dpt wd7000 ict athd mcesdi mcst static
do 
   cat /tmp/${i}list
done | sort -u > /tmp/symlist


echo "Reconfiguring system files in $ROOT/.$MACH/etc/conf/sdevice.d ..."
# Setup drivers for mini_unix
MACH=.$MACH sh cmd/mini_kernel.sh on ${KDB}
MACH=.$MACH $ROOT/.$MACH/etc/conf/bin/${PFX}idbuild ${SYMS} \
	-DRAMD_BOOT -I$ROOT/$MACH/usr/include -O $ROOT/$MACH/stand/unix ||
	exit $?

fi # if kernel=1

if [ $command -eq 1 ]
then

echo "\nBuilding local commands for Boot Floppies"
test -d desktop/ifiles || mkdir -p desktop/ifiles
(cd desktop/instcmd;	make -f instcmd.mk install)
(cd desktop;		make -f instscript.mk install)

# Strip out comment sections form local commands
find desktop/ifiles -type f -print | 
xargs file | 
grep ELF |
grep -v '\.o:'|
cut -d: -f1|
xargs -i ${PFX}mcs -d {}

fi # command = 1

if [ $kernel = 1 ]
then

SLBASE=$ROOT/$MACH export SLBASE
grep ROOT desktop/ramdfs.proto | 
awk '{print $NF}' | 
sed "s,\$ROOT\/\$MACH,$ROOT/$MACH,p" | 
while read file
do
	f_pstamp ${file}
done

#BUILD the ramd fs and stuff it in $ROOT/$MACH/stand/unix

sh conframdfs.sh

#Save a copy of the unstripped version for debugging.

cp $ROOT/$MACH/stand/unix $ROOT/$MACH/stand/unix.nostrip

echo "Stripping $ROOT/$MACH/stand/unix of all its worldly possesions ..."
${PFX}strip $ROOT/$MACH/stand/unix
echo "... and if thats not enough I now replace its .comment section."
SLBASE=$ROOT/$MACH f_pstamp $ROOT/$MACH/stand/unix

fi # kernel = 1

if [ $command -eq 1 ]
then

SLBASE=$ROOT/$MACH export SLBASE
grep ROOT desktop/proto.flop.* | 
awk '{print $NF}' | 
sed "s,\$ROOT\/\$MACH,$ROOT/$MACH,p" | 
while read file
do
	f_pstamp ${file}
done
HERE=`pwd`
export HERE
rm -rf tmp.3 tmp.4

MakeList $HERE/disk3.Z.list $HERE/tmp.3
MakeList $HERE/disk4.Z.list $HERE/tmp.4

mkdir -p $HERE/tmp.3/usr/lib/tape
ln $HERE/tmp.3/usr/bin/tapecntl $HERE/tmp.3/usr/lib/tape/tapecntl

cd ${HERE}/tmp.3
test -d etc/conf/modnew.d && mv etc/conf/modnew.d etc/conf/mod.d
find . -print | cpio -ocdu${CPIODEBUG} > ${HERE}/disk3.cpio 2>/dev/null

cd ${HERE}/tmp.4
test -d etc/conf/modnew.d && mv etc/conf/modnew.d etc/conf/mod.d
find . -print | cpio -ocdu${CPIODEBUG} > ${HERE}/disk4.cpio 2>/dev/null

cd $HERE #now we're back

for i in tmp.3 tmp.4 disk4.cpio.Z disk3.cpio.Z scripts.cpio.Z menus.cpio.Z
do
rm -rf $i & > /dev/null 2>&1
done

wait

(cd desktop/ifiles/hd;find . -type f -print|cpio -oc${CPIODEBUG} > $HERE/menus.cpio) &
(cd $HERE/desktop/hdscripts;find . -type f -follow -print|cpio -oLc${CPIODEBUG} > $HERE/scripts.cpio) & 

wait 

cd $HERE

compress disk4.cpio &
compress disk3.cpio &
compress scripts.cpio &
compress menus.cpio &

wait

fi # command = 1


echo "\n$0: Complete"
