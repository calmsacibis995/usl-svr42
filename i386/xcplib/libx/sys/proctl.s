/	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
/	UNIX System Laboratories, Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.


/	Copyright (c) 1987, 1988 Microsoft Corporation
/	  All Rights Reserved

/	This Module contains Proprietary Information of Microsoft
/	Corporation and should be treated as Confidential.

	.file	"proctl.s"

	.ident	"@(#)xcplibx:i386/xcplib/libx/sys/proctl.s	1.2"
	.ident  "$Header: proctl.s 1.1 91/07/04 $"


	.globl	proctl
	.globl	_cerror

proctl:
	MCOUNT			/ subroutine entry counter if profiling
	movl	$PROCTL,%eax
	lcall	SYSCALL
	jc	_cerror
	ret
