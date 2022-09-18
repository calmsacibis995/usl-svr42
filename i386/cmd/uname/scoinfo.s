/	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
/	UNIX System Laboratories, Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

/	@(#) scoinfo.s 1.1 90/03/12 
/
/	Copyright (C) The Santa Cruz Operation, 1990.
/	This Module contains Proprietary Information of
/	The Santa Cruz Operation, and should be treated as Confidential.
/
	.file	"scoinfo.s"

.ident	"@(#)uname:i386/cmd/uname/scoinfo.s	1.1"

	.set	SCOINFO,12840

	.globl	scoinfo
	.globl	errno

scoinfo:
	movl	$SCOINFO,%eax
	lcall	$0x7,$0
	jc	.cerror
	xorl	%eax,%eax
	ret

.cerror:
	movl	%eax,errno
	movl	$-1,%eax
	ret
