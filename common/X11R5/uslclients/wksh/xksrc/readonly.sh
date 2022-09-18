#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)wksh:xksrc/readonly.sh	1.1"

: "Make readonly data into text for sharing"
error=0 CC=${CC-cc}
flags=
while test '' != "$1"
do	case $1 in
	-*) flags="$flags $1";;
	*)  break;;
	esac
	shift
done
for i in "$@"
do	x=`basename "$i" .c`
	${CC} ${CCFLAGS} $flags -S "$i"
	sed -e 's/^\([ 	]*\.*\)data/\1text/
		s/\([^:][ 	]*\.*\)zero[ 	][ 	]*/\1set	.,.+/
		s/\([^:][ 	]*\.*\)space[ 	][ 	]*4$/\1byte 0,0,0,0/
		s/\([^:][ 	]*\.*\)space[ 	][ 	]*3$/\1byte 0,0,0/
		s/\([^:][ 	]*\.*\)space[ 	][ 	]*2$/\1byte 0,0/
		s/\([^:][ 	]*\.*\)space[ 	][ 	]*1$/\1byte 0/' "$x.s" > data$$.s
	mv data$$.s "$x.s"
	if	${CC} -c "$x.s"
	then	rm -f "$x.s"
	else	error=1
		${CC} ${flags} -c "$i"
	fi
done
exit $error
