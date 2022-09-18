/	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
/	UNIX System Laboratories, Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)uts-x86at:svc/syms.s	1.2"
	.ident	"$Header: $"

	.text
	.globl	stext
	.align	0x1000
	.set	stext, .

	.data
	.globl	sdata
	.align	0x1000
	.set 	sdata, .

	.bss
	.globl	sbss
	.align	0x1000
	.set 	sbss, .

	.section	BKI, "a"
	.align	4
	.value		0x02


	.globl	df_stack
	.set	df_stack, 0xD0000000

	.globl	bootinfo
	.globl	kspt0
	.globl	kpd0

	.set	Page0,0xD0000000
	.set	bootinfo, Page0 + 0x600
	.set 	kspt0	, Page0 + 0x1000
	.set	kpd0	, Page0 + 0x2000

	.globl	syssegs
	.set	syssegs, 0xd1000000

	.globl	piosegs
	.set	piosegs, 0xd2000000

	.globl	kvsegmap
	.set	kvsegmap, 0xd2400000

	.globl	kvsegu
	.set	kvsegu, 0xe0400000

	.globl	userstack
	.set	userstack,0x7ffffffc

	.globl	u
	.set	u,0xe0000000
