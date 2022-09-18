/	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
/	Copyright (c) 1988, 1989, 1990 AT&T
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
/	UNIX System Laboratories, Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)vga16:vga16/vgabytefl.s	1.2"

/
/	Copyright (c) 1991 USL
/	All Rights Reserved 
/ 
/ 	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
/ 	The copyright notice above does not evidence any 
/ 	actual or intended publication of such source code.
/ 
/ 	Copyrighted as an unpublished work.
/ 	(c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
/ 	All rights reserved.
/

	.file	"vgabyteflip.s"

#include "vtdefs.h"

/*
/*	vga_slsbbmin(psrc, pdst, cnt, pad)	-- copy bytes from psrc to pdst
/*						flipping the bits as we go.
/*
/*	Input:
/*		BYTE	*psrc		-- points to source bitmap sl
/*		BYTE	*pdst		-- points to source bitmap sl
/*		int	cnt		-- number of bytes to copy
/*		int	pad		-- amount to pad before copying
/*/		
/vga_byteflip(psrc, pdst, cnt, pad)
/register BYTE	*psrc;
/register BYTE	*pdst;
/register int	cnt;
/int	pad;
/{
/	psrc += pad;
/	while (--cnt >= 0)
/		*pdst++ = vga_bitflip[*psrc++];
/}

	.text
	.align	4
	.globl	vga_bitflip
	.globl	vga_byteflip
vga_byteflip:
	MCOUNT			/ for profiling
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	movl	16(%esp),%esi
	movl	20(%esp),%edi
	movl	24(%esp),%ecx
	addl	28(%esp),%edi
	leal	vga_bitflip,%ebx
.L108:
	lodsb
	xlat
	stosb
	.byte	0xE0, 0xFB	/ loopne	.L108
	popl	%ebx
	popl	%esi
	popl	%edi
	ret	
