#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/plot/plot.sh	1.2"
#ident	"$Header: $"
#!/sbin/sh
#! /bin/sh

#	Copyright (c) 1988, Sun Microsystems, Inc.
#	All Rights Reserved.

PATH=/bin:/usr/bin:/usr/ucb
case $1 in
-T*)	t=$1
	shift ;;
*)	t=-T$TERM
esac
case $t in
-T450)			exec t450 $*;;
-T300)			exec t300 $*;;
-T300S|-T300s)		exec t300s $*;;
-Tver)			exec lpr -Pversatec -g $*;;
-Tvar)			exec lpr -Pvarian -g $*;;
-Ttek|-T4014|-T)	exec tek $* ;;
-T4013)			exec t4013 $* ;;
-Tbitgraph|-Tbg)	exec bgplot $*;;
-Tgigi|-Tvt125)		exec gigiplot $*;;
-Taed)			exec aedplot $*;;
-Thp7221|-Thp7|-Th7)	exec hp7221plot $*;;
-Thp|-T2648|-T2648a|-Thp2648|-Thp2648a|h8)
			exec hpplot $*;;
-Tip|-Timagen|-Tim)	exec implot $*;;
-Tdumb|un|unknown)	exec dumbplot $*;;
-Tato)			exec atoplot $*;;
*)  			exec crtplot $*;;
esac
