#ident	"@(#)ucb:i386/ucblib/libc/i386/sys/_times.s	1.2"
#ident	"$Header: $"
/	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
/	UNIX System Laboratories, Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.


.ident		"@(#)ucb:i386/ucblib/libc/i386/sys/_times.s	1.2"


	.file	"times.s"

	.set	TIMES, 43

	.text

	.globl	_cerror

times:
	movl	$TIMES,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
