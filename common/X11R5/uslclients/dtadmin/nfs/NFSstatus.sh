#!/bin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)dtadmin:nfs/NFSstatus.sh	1.2"

# NFSstatus.sh  report the status of NFS Network File System

# exit codes:
#
#  0 client-server + boot server + pc server mode: all daemons running.
# 32 running in client-server + boot server mode:  pcnfsd not running.
# 33 running in client-server + pc server mode:    bootparamd not running.
# 34 running in client-server mode:     pcnfsd and bootparamd not running.
# 35 running in server mode only:       nfsd not running either.
# 36 running in client mode only:       rpcbind, lockd, biod and statd running.
# 37 NFS corrupt state -- NFS should be stopped and restarted -- 
#                                       lockd and/or statd not running.
# 38 RPC corrupt state -- RPC and NFS should be stopped and restarted --
#                                       rpcbind not running. 
# 39 NFS not running:     none of the daemons other than rpcbind running.

set `/usr/bin/ps -eu 0 | /usr/bin/grep 'd$'| /usr/bin/cut -f2 -d: | \
     /usr/bin/cut -f2 -d' ' | /usr/bin/sort -u`

unset numDaemons rpc bio lock nfs mount stat boot pc
numDaemons=0

for i in $* 
do
	case $i in

	rpcbind)	rpc=1;   numDaemons=`expr $numDaemons + 1`;;
	biod)		bio=1;   numDaemons=`expr $numDaemons + 1`;;
	lockd)		lock=1;  numDaemons=`expr $numDaemons + 1`;;
	nfsd)		nfs=1;   numDaemons=`expr $numDaemons + 1`;;
	mountd)		mount=1; numDaemons=`expr $numDaemons + 1`;;
	statd)		stat=1;  numDaemons=`expr $numDaemons + 1`;;
	bootparmd)	boot=1;  numDaemons=`expr $numDaemons + 1`;;
	pcnfsd)		pc=1;    numDaemons=`expr $numDaemons + 1`;;
	*)		;;
	esac
done


if [ "$numDaemons" = "8" ]      ; then exit 0; fi
if [ "$numDaemons" = "$rpc" ]   ; then exit 39; fi
if [ -z "$rpc" ]                ; then exit 38; fi
if [ -z "$lock" -o -z "$stat" ] ; then exit 37; fi
if [ "$numDaemons" = "6" -a -z "$boot" -a -z "$pc" ] ; then exit 34; fi
if [ "$numDaemons" = "7" ]
then
	if [ -z "$boot" ]
           then exit 33
        else
	    if [ -z "$pc" ]
               then exit 32
            fi
        fi
fi
if [ -z "$nfs" ] 
   then exit 35
else 
   exit 36  #only choice left
fi
