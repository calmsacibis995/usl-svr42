#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:desktop/hdscripts/chk_up_over.sh	1.3.1.29"
#ident	"$Header: $"

#
# This script does the preliminary work during an upgrade or overlay
# installation, before the base system is installed on the hard disk.
#

# All these directories exist before this script is executed.

CONF=/etc/conf
ETCINST=/etc/inst
UPINSTALL=$ETCINST/up
PKGINST=/usr/sbin/pkginst
UPGRADE_STORE=/var/sadm/upgrade

PKGRM=/usr/sbin/pkgrm

. $ETCINST/scripts/common.sh
. $PKGINST/updebug

# Should I move this below NEWINSTALL ??
# At this time, I expect these values to be set !!
# Two files check /tmp/upgrade.sh: fnd/request and base/request.

rm -f /tmp/upgrade.sh
echo PKGINSTALL_TYPE=$INSTALL_TYPE >/tmp/upgrade.sh
echo INSTALL_MODE=$INSTALL_MODE >>/tmp/upgrade.sh

#
# If the installation type is destructive then there's nothing to do.
#

[ "$INSTALL_TYPE" = "NEWINSTALL" ] && exit 0

[ "$UPDEBUG" = "YES" ] && set -x

#
# This is required for either UPGRADE or OVERLAY.  Before this change,
# everything worked fine for OVERLAY, but failed for UPGRADE, because
# I mv /etc/conf /etc/conf.v4 for an UPGRADE.
#
# This solution will work for both OVERLAY and UPGRADE, and NOT require
# two different solutions in the package request scripts.
#

for i in bmse ptm mpt
do
	[ -f /etc/conf/sdevice.d/$i ] &&
		cp /etc/conf/sdevice.d/$i $UPINSTALL/$i
done

[ "$INSTALL_TYPE" = "OVERLAY" ] && {

	# To prevent a possible attribute verification failure

	chown root /dev/console >>$UPERR 2>&1

	#
	# We need to use the correct IRQ and IOADDR's in the driver we're
	# about to overlay, so we'll save the sdevice files here and merge
	# the info in during the Base System Package postinstall script.
	#
	# If we're upgrading a Version 4 box, the driver is qt, NOT ict
	# and we save that info farther down in this script.
	#

	[ -f $CONF/sdevice.d/ict ] && cp $CONF/sdevice.d/ict $UPINSTALL/ict.sdev

	[ "$UPDEBUG" = "YES" ] && goany

	exit 0
}

# Everything else in the script is upgrade specific

menu_colors regular
menu -r -f $HD_MENUS/chkupover.1 -o /dev/null

#
# Prepare for requirement on pg 14, the actual warning message will be
# presented to the user in the Base System Package request script.
#

rm -f /tmp/kdb.obs >/dev/null 2>&1
cd /var/sadm/pkg

[ -d kdb ] && echo "      Kernel Debugger" >/tmp/kdb.obs
[ -d kdb-util ] && echo "      Kernel Debugger Utilities" >>/tmp/kdb.obs

#
# Save volatile files: base.LIST has list of volatiles from base system,
# lp.LIST has list that will require merging during pkgadd of Printer
# Support Package.
#

# may require sync'ing with p loads
# I should fgrep all files in *.LIST to verify I don't blow
# something away I shouldn't (e.g. /etc/acct/holidays).

cd /

cat $UPINSTALL/patch/base.LIST $UPINSTALL/patch/lp.LIST |
	grep -v '^[ 	]*#' |
	grep -v '^[ 	]*$' |
	awk '{print $1}' |
	cpio -pdmu $UPGRADE_STORE >>$UPERR 2>&1

[ "$UPDEBUG" = "YES" ] && goany

#
# pkgrm obsolete packages after we do some pre-pkgrm stuff
#

# qt: save IRQ and I/O addrs

[ -f $CONF/sdevice.d/qt ] && cp $CONF/sdevice.d/qt $UPINSTALL/qt.sdev

#
# qt: removing the qt package will blow away the tape nodes we
#     require to load from tape, so we will save them for now.
#

mv /dev/rmt /dev/rmt.save

#
# lp: pkgrm does NOT delete non-empty directories, so we don't
#     need to save user added files in various lp directories.
#
#     But I do want to save the pkginfo file.  Then the generic
#     upnover tools will correctly identify that there are volatile
#     files that will require merging.
#

[ -f /var/sadm/pkg/lp/pkginfo ] && cp /var/sadm/pkg/lp/pkginfo /tmp/lp.$$

#
# mouse: pkgrm of mouse below, will also remove /usr/lib/mousetab.
#        We're using the contents of this file to set default answers
#        to mouse questions in the base package request script.
#

[ -f /usr/lib/mousetab ] && mv /usr/lib/mousetab /tmp/mse.$$

#
# I want to use pkgrm to remove as many obsolete packages as I can,
# BUT, I don't want to idbuild those with drivers, so I'm going to
# blow idbuild away and replace it.
#

cd $CONF/bin
rm -f idbuild
echo exit 0 >idbuild
chmod 755 idbuild

[ "$UPDEBUG" = "YES" ] && goany

# This is required because 'face' depends on 'ed'

sed s/rdepend=ask/rdepend=nocheck/ /var/sadm/install/admin/default >/tmp/def.$$

for i in kdb kdb-util mouse qt ed lp rfs
do
	[ -d /var/sadm/pkg/$i ] && $PKGRM -n -a /tmp/def.$$ $i >>$UPERR 2>&1
done

rm -f /tmp/def.$$

[ "$UPDEBUG" = "YES" ] && goany

#
# I need to remove the license package also, however it does something
# it shouldn't, it requires input (an ENTER) in the postremove script
# so it hangs forever waiting for it.  Luckily, all the license package
# does is tweek a variable in pack.d/kernel/space.c.  Since my little
# TRICK below will blow the space.c away, all I need to do is rm -rf
# the license packaging directory.
#

[ -d /var/sadm/pkg/license ] && rm -rf /var/sadm/pkg/license

# Restore things I saved above

mv /dev/rmt.save /dev/rmt

[ -f /tmp/mse.$$ ] && mv /tmp/mse.$$ /usr/lib/mousetab

mkdir /var/sadm/pkg/lp 2>>$UPERR		# was removed by pkgrm

[ -f /tmp/lp.$$ ] && mv /tmp/lp.$$ /var/sadm/pkg/lp/pkginfo

#
# Rename log files in base.v4.log:
#

cd /

cat $UPINSTALL/base.v4.log |
	grep -v '^[ 	]*#' |
	grep -v '^[ 	]*$' |
	awk '{print $1}' >/tmp/log.$$

[ "$UPDEBUG" = "YES" ] && goany

while read logfile
do
	[ -f $logfile ] && mv $logfile $logfile.v4

done </tmp/log.$$

rm -f /tmp/log.$$

[ "$UPDEBUG" = "YES" ] && goany

#
# I need to create a file I'll use at the end of this script, and
# I need to create it before I lose 'cat', which is soon to happen.
#

cat >/tmp/blow.$$ <<EOF
ptem
pckt
ptm
pts
sockmod
timod
tirdwr
ticlts
ticots
ticotsor
XENIX
osocket
devadp
sp
spt
mpt
log
sc01
sd01
st01
sw01
scsi
EOF

#
# Delete a few files that are not listed in the contents file
# and therefore are not dealt with in my TRICK below.
#

cat $UPINSTALL/straglers.v4 | 
	grep -v '^[ 	]*#' |
	grep -v '^[ 	]*$' |
	awk '{print $1}' |
	xargs rm -f >>$UPERR 2>&1

rm -rf /osm >>$UPERR 2>&1

#
# Delete files moved to Advanced Commands Package.  Delete files from
# v4 that are now obsolete.  Delete entries from contents file for
# those files we just deleted.  Leave contents file in a state where
# we can pkgadd the base and not require any sync'ing.
#
# And do it in a tricky, difficult to understand way.
# 

# NOTE: this will blow away LOTS from /etc/conf/*,  I need to make
# sure I save anything I'll need first (see conf.base.v4)

# NOTE: this may blow away terminfo entry required by menu tool ?

# Now I need to save those files that the boot floppies just installed.
# boot.inst will require sync'ing after each p-load

mkdir /tmp/boot.$$

cat $UPINSTALL/boot.inst |
	grep -v '^[ 	]*#' |
	grep -v '^[ 	]*$' |
	awk '{print $1}' |
	cpio -pdlmu /tmp/boot.$$ >>$UPERR 2>&1

cd /var/sadm/
mkdir -p pkg/BLAST/install
echo CLASSES=none >pkg/BLAST/pkginfo
cd install

[ "$UPDEBUG" = "YES" ] && goany

ed contents <<- EOF >>$UPERR 2>&1
	?^/var/sadm/install/contents
	d
	?^/shlib d
	s?shlib d?shlib=/usr/lib s?
	1,\$s/ foundation/ BLAST/
	w
	q
	EOF

$PKGRM -n BLAST >>$UPERR 2>&1

#
# Now I'll restore the files that were installed by the boot floppies.
# Note: find and cpio are included in the list, hence the relative paths.
# cd is a shell builtin.
#

cd /tmp/boot.$$
usr/bin/find . -print | usr/bin/cpio -pdlmu / >>$UPERR 2>&1

# One last bit-o-cleanup

cd /var/sadm/pkg
rm -rf BLAST dfm sysenv sys
rm -rf /tmp/boot.$$

[ "$UPDEBUG" = "YES" ] && goany

#
# Save Version 4 /etc/conf tree for later "Reconfiguration of Drivers"
# stage.
#

# Q: What happens when nsu pkg tries to pkgrm and it's drivers are NOT
# in the new /etc/conf tree ??  Does nsu pkgrm ?? or just pkgadd over ??

cd /etc
rm -rf conf/cf.d/unix conf/cf.d/sdevice conf/bin conf/mod.d >>$UPERR 2>&1
mv conf conf.v4

#
# We've been authorized by System Engineering to make a simplifying
# assumption. "If they've got nsu or acp currently installed, they MUST
# upgrade it."  The justification for making this assumption is that
# both those packages are part of the Foundation Set and CANNOT be
# purchased separately from the Base System Package.
#
# Without this assumption, several of the drivers will not compile
# and/or link correctly.
#
# Actually we're not going to force them to upgrade, but we will NOT
# reconfigure those drivers back into the kernel during the "Reconfig of
# Drivers" stage.  We warned the user that we would NOT reconfigure the
# following drivers, so we're going to delete all relevant files.
#
# The drivers that are part of the base system package, were deleted from
# the /etc/conf tree during by my little 'pkgrm BLAST' trick (except 'log'
# which was not listed in the contents file for some reason).
#
# We need to delete all relevant files because if we have encounter
# trouble reconfigureing their old drivers, we need to cpio /etc/conf.v4
# to $UPGRADE_STORE
#

cd /etc/conf.v4

while read MOD
do
	[ -d pack.d/$MOD ] && rm -rf `find * -name $MOD -print`

done </tmp/blow.$$

rm -f /tmp/blow.$$

#
# At this point, the new boot code has been placed on the hard disk
# and this script runs in non-interruptable mode, so I'm going to
# removed any Version 4 kernels that may be in /stand, since they
# will not boot any longer.
#

rm -f /stand/unix*

# One last special case, otherwise kernel will NOT build if nfs
# is NOT installed.

[ ! -f /etc/conf.v4/pack.d/nfs/Driver.o ] && rm -f /etc/conf.v4/sfsys.d/nfs

[ "$UPDEBUG" = "YES" ] && goany

exit 0
