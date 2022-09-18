#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)r5util:scripts/lninst.sh	1.1"
#!/bin/sh

#
# This accepts bsd-style install arguments and simply makes symbolic links.
#

flags=""
dst=""
src=""
dostrip=""

while [ x$1 != x ]; do
    case $1 in 
	-c) shift
	    continue;;

	-[mog]) flags="$flags $1 $2 "
	    shift
	    shift
	    continue;;

	-s) dostrip="strip"
	    shift
	    continue;;

	*)  if [ x$src = x ] 
	    then
		src=$1
	    else
		dst=$1
	    fi
	    shift
	    continue;;
    esac
done

if [ x$src = x ] 
then
	echo "syminst:  no input file specified"
	exit 1
fi

if [ x$dst = x ] 
then
	echo "syminst:  no destination specified"
	exit 1
fi

if [ -d $dst ]; then
    rm -f $dst/`basename $src`
else
    rm -f $dst
fi

ln -s `pwd`/$src $dst
