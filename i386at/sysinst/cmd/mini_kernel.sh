#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:cmd/mini_kernel.sh	1.3.6.19"
#ident	"$Header: $"

turnoff () {
	cd $ROOT/$MACH/etc/conf/sdevice.d
	for i in $*
	do
		if [ -f $i ]
		then
			mv $i .$i
		fi
	done
}

reqoff(){
	cd $ROOT/$MACH/etc/conf/mdevice.d
	for i in $*
	do
		if [ -f $i ]
		then
			mv $i .$i
		fi
	done
}

restore_sdevice() {
	cd $ROOT/$MACH/etc/conf
	rm -rf sdevice.d
	mv osdevice.d sdevice.d
}

backup_sdevice() {
	[ -d $ROOT/$MACH/etc/conf/osdevice.d ] || {
		mkdir -p $ROOT/$MACH/etc/conf/osdevice.d
	}
	cd $ROOT/$MACH/etc/conf/sdevice.d
	find . -print |
		cpio -pdmuv $ROOT/$MACH/etc/conf/osdevice.d 1>/dev/null 2>&1

# the following drivers are static for the mini kernel.
	for i in st01 s5 fd
	do
	ed $i << END > /dev/null 2>&1
/loadable/d
w
w
q
END
done
}

turnon () {
	cd $ROOT/$MACH/etc/conf/sdevice.d
	for i in $*
	do
		test -f .$i && mv .$i $i
		if [ -f $i ]
		then
		ed $i << END > /dev/null 2>&1
1,\$s/	N	/	Y	/
w
w
q
END
		fi
	done
}

if [ $# != 2 ]
then echo Usage: $0 on/off kdb/nokdb
     exit 2
fi
case "$1" in
on)  
     backup_sdevice
     turnon gvid kdvm nmi iaf namefs ramd

     turnoff async fc audit app tcp rfs nfs ticlts ticots ticotsor
     turnoff arp ktli llcloop krpc xnamfs xx klm udp cdfs
     turnoff log lp lpm mac modctl mpcntl
     turnoff osm isocket osocket osxt oxt sockmod sxt xt
     turnoff des devadp i386x icmp ip ipc sp sem shm msg
     turnoff spt mpt pc586 imx586 rt rtx rawip ie6 kbd kmacct
     turnoff xout clist clist_gd intmap bmse mse m320 ptm pts smse
     turnoff ptem prf clone connld fdfs intp pckt timod xque
     turnoff tirdwr weitek tpath cpyrt dac i286x pipemod wd
     turnoff dosx vx vc coff event events qt XENIX
     turnoff ntty gdebugger ee16 el16 i596 sysvendor raio rmc
     ## Move these required drivers off and move the mdevice
     ##   files out of the way.
     turnoff  sc01 sw01 
     reqoff   sc01 sw01

# XXX skip for P6
#     $ROOT/$MACH/etc/conf/bin/idtune -f MAXACL 10
#     $ROOT/$MACH/etc/conf/bin/idtune -f FLCKREC 100
#     $ROOT/$MACH/etc/conf/bin/idtune -f NPROC 100
#     $ROOT/$MACH/etc/conf/bin/idtune -f NCALL 100
#     $ROOT/$MACH/etc/conf/bin/idtune -f MAXUP 30
#     $ROOT/$MACH/etc/conf/bin/idtune -f NHBUF 32
#     $ROOT/$MACH/etc/conf/bin/idtune -f SPTMAP 50
#     $ROOT/$MACH/etc/conf/bin/idtune -f NAUTOP 10
#     $ROOT/$MACH/etc/conf/bin/idtune -f BUFHWM 25
#     $ROOT/$MACH/etc/conf/bin/idtune -f DMAABLEBUF 10
#     $ROOT/$MACH/etc/conf/bin/idtune -f NINODE 100
#     $ROOT/$MACH/etc/conf/bin/idtune -f SFSINODE 100
#     $ROOT/$MACH/etc/conf/bin/idtune -f NDQUOT 100

     #cd $ROOT/$MACH/usr/src/proto/i386/at386
     # If kernel has merge turned on, this will put this into mini mdevice
     #cp cmd/stune.mini $ROOT/$MACH/etc/conf/cf.d/stune
     ;;
off) 

     restore_sdevice
# XXX skip for P6
#     $ROOT/$MACH/etc/conf/bin/idtune -f -d MAXACL 
#     $ROOT/$MACH/etc/conf/bin/idtune -f -d FLCKREC
#     $ROOT/$MACH/etc/conf/bin/idtune -f -d NPROC 
#     $ROOT/$MACH/etc/conf/bin/idtune -f -d NCALL
#     $ROOT/$MACH/etc/conf/bin/idtune -f -d MAXUP
#     $ROOT/$MACH/etc/conf/bin/idtune -f -d NHBUF
#     $ROOT/$MACH/etc/conf/bin/idtune -f -d SPTMAP
#     $ROOT/$MACH/etc/conf/bin/idtune -f -d NAUTOP
#     $ROOT/$MACH/etc/conf/bin/idtune -f -d BUFHWM
#     $ROOT/$MACH/etc/conf/bin/idtune -f -d DMAABLEBUF
#     $ROOT/$MACH/etc/conf/bin/idtune -f -d NINODE
#     $ROOT/$MACH/etc/conf/bin/idtune -f -d SFSINODE
#     $ROOT/$MACH/etc/conf/bin/idtune -f -d NDQUOT

     ;;
*)   echo Usage: $0 on/off kdb/nokdb
     exit 2
     ;;
esac
case "$2" in
kdb)
	turnon kdb kdb-util
	;;
nokdb)
	turnoff kdb kdb-util
	;;
*)   echo Usage: $0 on/off kdb/nokdb
     exit 2
     ;;
esac
exit 0
