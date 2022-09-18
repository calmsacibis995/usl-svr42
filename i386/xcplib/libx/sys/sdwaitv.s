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

	.file	"sdwaitv.s"

	.ident	"@(#)xcplibx:i386/xcplib/libx/sys/sdwaitv.s	1.2"
	.ident  "$Header: sdwaitv.s 1.1 91/07/04 $"


	.globl	sdwaitv
	.globl	_cerror

sdwaitv:
	MCOUNT			/ subroutine entry counter if profiling
	movl	$SDWAITV,%eax
	lcall	SYSCALL
	jc	_cerror
	ret
