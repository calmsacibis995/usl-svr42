/	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
/	UNIX System Laboratories, Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.


	.ident	"@(#)uts-x86:util/string.s	1.6"
	.ident	"$Header: $"
	.file	"util/strings.s"

/ String functions copied from the C library.
/
/ Fast assembler language version of the following C-program for
/			strcmp
/ which represents the "standard" for the C-library.

/	/*
/	 * Compare strings:  s1>s2: >0  s1==s2: 0  s1<s2: <0
/	 */
/
/	int
/	strcmp(s1, s2)
/	register char *s1, *s2;
/	{
/
/		if(s1 == s2)
/			return(0);
/		while(*s1 == *s2++)
/			if(*s1++ == '\0')
/				return(0);
/		return(*s1 - *--s2);
/	}


	.type	strcmp,@function
	.globl	strcmp
	.align	4

strcmp:
	pushl	%esi
	movl	8(%esp),%esi	/ %esi = pointer to string 1
	movl	12(%esp),%edx	/ %edx = pointer to string 2
	cmpl	%esi,%edx	/ s1 == s2 ?
	je	.equal		/ they are equal
	.align	4
.loop:				/ iterate for cache performance
	movl	(%esi),%eax	/ pick up 4-bytes from first string
	movl	(%edx),%ecx	/ pick up 4-bytes from second string
	cmpl	%ecx,%eax	/ see if they are equal
	jne	.notequal	/ if not, find out why and where
	subl	$0x01010101,%eax	/ see if we hit end of the string
	notl	%ecx
	andl	$0x80808080,%ecx
	andl	%ecx,%eax
	jnz	.equal		/ there was a 0 in the 4-bytes
	movl	4(%esi),%eax	/ pick up 4-bytes from first string
	movl	4(%edx),%ecx	/ pick up 4-bytes from second string
	cmpl	%ecx,%eax	/ see if they are equal
	jne	.notequal	/ if not, find out why and where
	subl	$0x01010101,%eax	/ see if we hit end of the string
	notl	%ecx
	andl	$0x80808080,%ecx
	andl	%ecx,%eax
	jnz	.equal		/ there was a 0 in the 4-bytes
	movl	8(%esi),%eax	/ pick up 4-bytes from first string
	movl	8(%edx),%ecx	/ pick up 4-bytes from second string
	cmpl	%ecx,%eax	/ see if they are equal
	jne	.notequal	/ if not, find out why and where
	subl	$0x01010101,%eax	/ see if we hit end of the string
	notl	%ecx
	andl	$0x80808080,%ecx
	andl	%ecx,%eax
	jnz	.equal		/ there was a 0 in the 4-bytes
	movl	12(%esi),%eax	/ pick up 4-bytes from first string
	movl	12(%edx),%ecx	/ pick up 4-bytes from second string
	cmpl	%ecx,%eax	/ see if they are equal
	jne	.notequal	/ if not, find out why and where
	addl	$16,%esi	/ increment first pointer
	addl	$16,%edx	/ increment second pointer
	subl	$0x01010101,%eax	/ see if we hit end of the string
	notl	%ecx
	andl	$0x80808080,%ecx
	andl	%ecx,%eax
	jz	.loop		/ not yet at end of string, try again
	.align	4
.equal:				/ strings are equal, return 0
	popl	%esi
	xorl	%eax,%eax
	ret
	.align	4
.notequal:			/ two words are not the same, find out why
	cmpb	%cl,%al		/ see if individual bytes are the same
	jne	.set_sign	/ if not the same, go set sign
	andb	%al,%al		/ see if we hit the end of string
	je	.equal		/ yes, they are equal
	cmpb	%ch,%ah		/ check next byte...
	jne	.set_sign
	andb	%ah,%ah
	je	.equal
	shrl	$16,%eax
	shrl	$16,%ecx
	cmpb	%cl,%al
	jne	.set_sign
	andb	%al,%al
	je	.equal
	cmpb	%ch,%ah		/ last byte guaranteed not equal
.set_sign:			/ set the sign of the result for unequal
	popl	%esi
	sbbl	%eax,%eax
	orl	$1,%eax
	ret
	.size	strcmp,.-strcmp


/ Fast assembler language version of the following C-program
/			strlen
/ which represents the "standard" for the C-library.
/
/ Given string s, return length (not including the terminating null).

/	strlen(s)
/	register char	*s;
/	{
/		register n;
/	
/		n = 0;
/		while (*s++)
/			n++;
/		return(n);
/	}


	.type	strlen,@function
	.globl	strlen
	.align	4

strlen:
	pushl	%edi		/ save register variables

	movl	8(%esp),%edi	/ string address
	xorl	%eax,%eax	/ %al = 0
	movl	$-1,%ecx	/ Start count backward from -1.
	repnz ; scab
	incl	%ecx		/ Chip pre-decrements.
	movl	%ecx,%eax	/ %eax = return values
	notl	%eax		/ Twos complement arith. rule.

	popl	%edi		/ restore register variables
	ret
	.size	strlen,.-strlen

/ Fast assembler language version of the following C-program
/			strcpy
/ which represents the "standard" for the C-library.
/
/ Copy string s2 to s1.  s1 must be large enough. Return s1.
/
/	char	*
/	strcpy(s1, s2)
/	register char	*s1, *s2;
/	{
/		register char	*os1;
/	
/		os1 = s1;
/		while (*s1++ = *s2++)
/			;
/		return(os1);
/	}


	.type	strcpy,@function
	.globl	strcpy
	.align	4

strcpy:
	pushl	%edi		/ save register variables
	movl	%esi,%edx

	movl	12(%esp),%edi	/ %edi = source string address
	xorl	%eax,%eax	/ %al = 0 (search for 0)
	movl	$-1,%ecx	/ length to look: lots
	repnz ; scab

	notl	%ecx		/ %ecx = length to move
	movl	12(%esp),%esi	/ %esi = source string address
	movl	8(%esp),%edi	/ %edi = destination string address
	movl	%ecx,%eax	/ %eax = length to move
	shrl	$2,%ecx		/ %ecx = words to move
	rep ; smovl

	movl	%eax,%ecx	/ %ecx = length to move
	andl	$3,%ecx		/ %ecx = leftover bytes to move
	rep ; smovb

	movl	8(%esp),%eax	/ %eax = returned dest string addr
	movl	%edx,%esi	/ restore register variables
	popl	%edi
	ret
	.size	strcpy,.-strcpy

/
/ Fast assembler version of `strcat'.
/

	.type	strcat,@function
	.globl	strcat
	.align	4

strcat:
	pushl	%esi
	pushl	%edi

	movl	16(%esp), %esi	/ get source address
	movl	%esi, %edi	/ save for later
	xorl	%eax, %eax	/ search \0 char
	movl	$-1, %ecx	/ in many chars
	repnz ;	scab

	notl	%ecx		/ number to copy
	movl	%ecx, %edx	/ save for copy
	
	movl	12(%esp),%edi	/ get destination end address
	movl	$-1, %ecx	/ search for many
	repnz ; scab

	decl	%edi		/ backup 1 byte

	movl	%edx, %ecx	/ bytes to copy
	shrl	$2, %ecx	/ double to copy
	rep ;	smovl

	movl	%edx, %ecx	/ bytes to copy
	andl	$3, %ecx	/ mod 4
	rep ;	smovb

	movl	12(%esp), %eax

	popl	%edi
	popl	%esi
	ret
	.size	strcat,.-strcat

/
/ Fast assembler version of `strncat'.
/

	.type	strncat,@function
	.globl	strncat
	.align	4

strncat:
	pushl	%esi
	pushl	%edi

	movl	20(%esp), %ecx	/ max number to copy
	jcxz	.strncat_ret

	movl	%ecx, %edx
	movl	16(%esp), %esi	/ get source address
	movl	%esi, %edi	/ save for later
	xorl	%eax, %eax	/ search \0 char
	repnz ;	scab

	jne	.strncat_no_null
	incl	%ecx
.strncat_no_null:

	subl	%ecx, %edx	/ bytes to copy

	movl	12(%esp),%edi	/ get destination end address
	movl	$-1, %ecx	/ search for many
	repnz ; scab

	decl	%edi		/ backup 1 byte

	movl	%edx, %ecx	/ copy bytes - most suffixes are not very long;
	rep ;	smovb		/   no worth setup for longword moves

	sstob			/ null byte terminator

.strncat_ret:
	movl	12(%esp), %eax

	popl	%edi
	popl	%esi
	ret
	.size	strncat,.-strncat

/ Fast assembler version of `strchr':
/ return the pointer in sp at which the character c appears;
/ NULL if not found.
/	
/	char *
/	strchr(sp, c)
/	register char *sp;
/	register int c;
/	{
/		register char ch = (char)c;
/		do {
/			if(*sp == ch)
/				return(sp);
/		} while(*sp++);
/		return(NULL);
/	}

	.type	strchr,@function
	.globl	strchr
	.align	4

strchr:
	movl	4(%esp),%eax	/ %eax = string address
	movb	8(%esp),%dh	/ %dh = byte sought
.strchr_loop:
	movb	(%eax),%dl	/ %dl = byte of string
	cmpb	%dh,%dl		/ find it?
	je	.strchr_found	/ yes
	testb	%dl,%dl		/ is it null?
	je	.strchr_notfound

	movb	1(%eax),%dl	/ %dl = byte of string
	cmpb	%dh,%dl		/ find it?
	je	.strchr_found1	/ yes
	testb	%dl,%dl		/ is it null?
	je	.strchr_notfound

	movb	2(%eax),%dl	/ %dl = byte of string
	cmpb	%dh,%dl		/ find it?
	je	.strchr_found2	/ yes
	testb	%dl,%dl		/ is it null?
	je	.strchr_notfound

	movb	3(%eax),%dl	/ %dl = byte of string
	cmpb	%dh,%dl		/ find it?
	je	.strchr_found3	/ yes
	addl	$4,%eax
	testb	%dl,%dl		/ is it null?
	jne	.strchr_loop

.strchr_notfound:
	xorl	%eax,%eax	/ %eax = NULL
	ret

.strchr_found3:
	incl	%eax
.strchr_found2:
	incl	%eax
.strchr_found1:
	incl	%eax
.strchr_found:
	ret
	.size	strchr,.-strchr
