#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)r5util:scripts/x11mf.sh	1.1"
#!/bin/sh

# 
# generate a Makefile within the build tree
# 
# usage:  x11mf [treedir]
# 

if [ x$1 != x ]; then
	tree=$1
else
	tree=/x11
fi

dir=`pwd`
top=`(cd $tree; /bin/pwd)`
intree=no

case $dir in
	$top*)	intree=yes;;
esac

if [ $intree != yes ]; then
	echo "$0:  Must be underneath $tree"
	exit 1
fi

(cd ..; make SUBDIRS=`basename $dir` Makefiles)
