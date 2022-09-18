/	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
/	UNIX System Laboratories, Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)uts-x86:fs/s5fs/s5search.s	1.2"
	.ident	"$Header: $"
	.file	"fs/s5fs/s5search.s"

/	searchdir(buf, n, target) - search a directory for target
/	Return offset into directory of match, or if no match found
/       then return offset into directory of empty slot, or if none
/	found, then return -1

	.set	BUF, 8
	.set	BUFSIZ,	12
	.set	TARGET,	16
	.set	DIRENT,	16	/ size of a directory entry
	.set	DIRSIZ,	14	/ size of a file name

	.align	4
	.type	s5searchdir,@function
	.globl	s5searchdir
s5searchdir:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	movl	BUF(%ebp), %esi			/ pointer to directory
	movl	BUFSIZ(%ebp), %ebx		/ directory length in bytes
	movl	$0, %edx			/ pointer to empty slot
			/ get length of target string
	movl	TARGET(%ebp), %edi		/ address of target name
	movl	$DIRSIZ, %ecx
	movb	$0, %al
	repnz
	scab
	movl	$DIRSIZ, %eax
	subl	%ecx, %eax			/ %eax=length of target
.s_top:
	cmpl	$DIRENT, %ebx			/ length less than 16?
	jl	.sdone				/ done if less
	cmpw	$0, (%esi)			/ directory entry empty?
	je	.sempty				/ jump if true
	pushl	%esi				/ save start of entry
	addl	$2, %esi			/ address of file name
	movl	TARGET(%ebp), %edi		/ address of target name

	movl	%eax, %ecx			/ length of target name
	repz
	scmpb
	jcxz	.smatch				/ the names match
	popl	%esi				/ restore start of entry
.scont:
	addl	$DIRENT, %esi			/ increment directory pointer
	subl	$DIRENT, %ebx			/ decrement size
	jmp	.s_top				/ keep looking

	.align	4
.sempty:
	cmpl	$0, %edx			/ do we need an empty slot?
	jne	.scont				/ jump if no
	movl	%esi, %edx			/ save current offset
	jmp	.scont				/ and goto to next entry

	.align	4
.smatch:
	movb	-1(%esi), %cl
	cmpb	%cl, -1(%edi)
	je 	.srmatch				/ really a match
	popl	%esi				/ restore start of entry
	jmp	.scont				/ not really a match.
						/ Just a substring
	.align	4
.srmatch:
	popl	%esi				/ restore start of entry
	subl	BUF(%ebp), %esi		 	/ convert to offset
	movl	%esi, %eax			/ return offset
	jmp	.s_exit

	.align	4
.sdone:
	movl	$-1, %eax			/ save failure return
	cmpl	$0, %edx			/ empty slot found?
	je	.sfail				/ jump if false
	subl	BUF(%ebp), %edx			/ convert to offset
	movl	%edx, %eax			/ return empty slot
.sfail:
.s_exit:
	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret	

	.size	s5searchdir,.-s5searchdir
