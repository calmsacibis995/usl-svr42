/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_INLINE_H	/* wrapper symbol for kernel use */
#define _UTIL_INLINE_H	/* subject to change without notice */

#ident	"@(#)uts-x86:util/inline.h	1.11"
#ident	"$Header: $"

/*
 * This header file defines inline assembler versions of
 * several routines as a performance improvement measure.
 * Most of them also exist in the form of ordinary functions
 * defined somewhere in the kernel, since not all files are
 * or can be compiled with inline.h.
 */

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif	defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif	/* _KERNEL_HEADERS */

#if defined(__USLC__) && !defined(CXREF) && !defined(lint)

asm void
flushtlb()	
{
	movl	%cr3, %eax
	movl	%eax, %cr3
}
#pragma asm partial_optimization flushtlb

asm int
_cr0()
{
	movl	%cr0, %eax
}
#pragma asm partial_optimization _cr0

asm int
_cr2()
{
	movl	%cr2, %eax
}
#pragma asm partial_optimization _cr2

asm int
_cr3()
{
	movl	%cr3, %eax
	andl	$0x7FFFFFFF, %eax
}
#pragma asm partial_optimization _cr3

asm void
_wdr0(ulong x)
{
%reg	x;
	movl    x, %db0
%mem	x;
	movl	x, %eax
	movl    %eax, %db0
}
#pragma asm partial_optimization _wdr0

asm void
_wdr1(ulong x)
{
%reg	x;
	movl    x, %db1
%mem	x;
	movl	x, %eax
	movl    %eax, %db1
}
#pragma asm partial_optimization _wdr1

asm void
_wdr2(ulong x)
{
%reg	x;
	movl    x, %db2
%mem	x;
	movl	x, %eax
	movl    %eax, %db2
}
#pragma asm partial_optimization _wdr2

asm void
_wdr3(ulong x)
{
%reg	x;
	movl    x, %db3
%mem	x;
	movl	x, %eax
	movl    %eax, %db3
}
#pragma asm partial_optimization _wdr3

asm void
_wdr6(ulong x)
{
%reg	x;
	movl    x, %db6
%mem	x;
	movl	x, %eax
	movl    %eax, %db6
}
#pragma asm partial_optimization _wdr6

asm void
_wdr7(ulong x)
{
%reg	x;
	movl    x, %db7
%mem	x;
	movl	x, %eax
	movl    %eax, %db7
}
#pragma asm partial_optimization _wdr7

asm int
_dr0()
{
	movl	%dr0, %eax
}
#pragma asm partial_optimization _dr0

asm int
_dr1()
{
	movl	%dr1, %eax
}
#pragma asm partial_optimization _dr1

asm int
_dr2()
{
	movl	%dr2, %eax
}
#pragma asm partial_optimization _dr2

asm int
_dr3()
{
	movl	%dr3, %eax
}
#pragma asm partial_optimization _dr3

asm int
_dr6()
{
	movl	%dr6, %eax
}
#pragma asm partial_optimization _dr6

asm int
_dr7()
{
	movl	%dr7, %eax
}
#pragma asm partial_optimization _dr7

asm void
loadtr(ulong x)
{
%mem	x
	movl	x, %eax
	ltr	%ax
}
#pragma asm partial_optimization loadtr

asm caddr_t
_ebp()
{
	movl	%ebp, %eax
}
#pragma asm partial_optimization _ebp

asm caddr_t
_esp()
{
	movl	%esp, %eax
}
#pragma asm partial_optimization _esp

asm ulong
_ebx()
{
	movl	%ebx, %eax
}
#pragma asm partial_optimization _ebx

asm ulong
_edi()
{
	movl	%edi, %eax
}
#pragma asm partial_optimization _edi

asm ulong
_esi()
{
	movl	%esi, %eax
}
#pragma asm partial_optimization _esi

/*
 * The three "out" macros (outl, outw, and outb) must
 * take some care when moving port to %edx and val to %eax.
 * In particular, since port could be passed in %eax and
 * val in %edx, it is possible that a mov to either of these
 * registers could overwrite the argument value.  To handle this
 * problem, the asm is divided into four separate cases:
 *
 *  (1)	Both port and val in %treg.  In this case, we push port, mov
 * 	val into %eax, and then pop into %edx.
 *
 *  (2)	Port is in %treg, but val is not.  In this case, we mov port
 *	into %edx, then mov val into %eax.
 *
 *  (3)	Port is not in %treg, val is in %treg.  In this case, we
 *	mov val into %eax, then mov port into %edx.
 *
 *  (4)	Neither is in %treg.  Order doesn't matter, so we just handle it
 *	the same as case 3.
 */
asm void
outl(unsigned port, ulong val)
{
%treg	port, val;
	pushl	port
	movl	val, %eax
	popl	%edx
	outl	(%dx)
%treg	port; mem	val;
	movl	port, %edx
	movl	val, %eax
	outl	(%dx)
%mem	port, val;
	movl	val, %eax
	movl	port, %edx
	outl	(%dx)
}
#pragma asm partial_optimization outl

asm void
outw(unsigned port, ulong val)
{
%treg	port, val;
	pushl	port
	movl	val, %eax
	popl	%edx
	data16
	outl	(%dx)
%treg	port; mem	val;
	movl	port, %edx
	movl	val, %eax
	data16
	outl	(%dx)
%mem	port, val;
	movl	val, %eax
	movl	port, %edx
	data16
	outl	(%dx)
}
#pragma asm partial_optimization outw

asm void
outb(unsigned port, ulong val)
{
%treg	port, val;
	pushl	port
	movl	val, %eax
	popl	%edx
	outb	(%dx)
%treg	port; mem	val;
	movl	port, %edx
	movl	val, %eax
	outb	(%dx)
%mem	port, val;
	movl	val, %eax
	movl	port, %edx
	outb	(%dx)
}
#pragma asm partial_optimization outb

asm ulong
inl(unsigned port)
{
%mem	port;
	movl	port, %dx
	inl	(%dx)
}
#pragma asm partial_optimization inl

asm ulong
inw(unsigned port)
{
%mem	port;
	movl	port, %edx
	subl    %eax, %eax
	data16
	inl	(%dx)
}
#pragma asm partial_optimization inw

asm ulong
inb(unsigned port)
{
%mem	port;
	movl	port, %edx
	subl    %eax, %eax
	inb	(%dx)
}
#pragma asm partial_optimization inb

asm int
intr_disable()
{
	pushfl
	cli
	popl	%eax
}
#pragma asm partial_optimization intr_disable

asm void
intr_restore(int efl)
{
%mem	efl;
	movl	efl, %eax
	pushl	%eax
	popfl
}
#pragma asm partial_optimization intr_restore

/*
 * struct_zero zeroes out a specified block of memory.
 *
 * The zeroing is divided into two parts: the memory is first zeroed
 * a long at a time, then remaining bytes are zeroed a byte at a time.
 * Zeroing is done via the sstol and sstob instructions, with %eax
 * set to 0.
 *
 * struct_zero must take some care when moving addr to %edi and len
 * to %ecx.  In particular, since addr could be passed in %ecx and
 * len could be passed in %edi, it is possible that a mov of an argument
 * into the desired register would be overwriting the value of the other
 * argument.  To handle this, the asm is divided into four separate cases:
 *
 *  (1)	addr is in %treg and len is in %ureg.  In this case, we push
 *	addr, mov len to %ecx, and then pop the stack into %edi.
 *
 *  (2)	addr is in %treg, and len is not in %ureg.  In this case, we
 *	first mov addr into %edi, then mov len into %ecx.
 *
 *  (3) addr is not in %treg, len is in %ureg.  First mov len into %ecx,
 *	then mov addr into %edi.
 *
 *  (4) addr is not in %treg, len is not in %ureg.  Order doesn't
 *	matter, so this case is folded in with case (3).
 *
 * Note that this asm defines and uses the symbol WORDSIZE as the size
 * of a word in bytes.  It would be preferable to use NBPW, defined in
 * <util/param.h>, but that does not appear to be possible because it
 * is defined via "sizeof", and the compiler isn't too happy about it
 * being in an asm.
 *
 * Since %edi is a compiler-allocated register, its value must be
 * saved and restored by this asm.
 */
asm int
struct_zero(caddr_t addr, int len)
{
%treg	addr; %ureg	len;
	.set	WORDSIZE, 4
	pushl	%edi
	pushl	addr
	movl	len, %ecx
	popl	%edi
	xorl	%eax, %eax
	movl	%ecx, %edx
	andl	$WORDSIZE-1, %edx
	shrl	$2, %ecx
	rep
	sstol
	movl	%edx, %ecx
	rep
	sstob
	popl	%edi

%treg	addr; %mem	len;
	.set	WORDSIZE, 4
	pushl	%edi
	movl	addr, %edi
	movl	len, %ecx
	xorl	%eax, %eax
	movl	%ecx, %edx
	andl	$WORDSIZE-1, %edx
	shrl	$2, %ecx
	rep
	sstol
	movl	%edx, %ecx
	rep
	sstob
	popl	%edi


%mem	addr, len;
	.set	WORDSIZE, 4
	pushl	%edi
	movl	len, %ecx
	movl	addr, %edi
	xorl	%eax, %eax
	movl	%ecx, %edx
	andl	$WORDSIZE-1, %edx
	shrl	$2, %ecx
	rep
	sstol
	movl	%edx, %ecx
	rep
	sstob
	popl	%edi
}
#pragma asm partial_optimization struct_zero

#endif	/* __USLC__ && !defined(lint) */

#if defined(__STDC__)

extern void	flushtlb(void);
extern int	_cr0(void);
extern int	_cr2(void);
extern int	_cr3(void);
extern void	_wdr0(ulong);
extern void	_wdr1(ulong);
extern void	_wdr2(ulong);
extern void	_wdr3(ulong);
extern void	_wdr6(ulong);
extern void	_wdr7(ulong);
extern int	_dr0(void);
extern int	_dr1(void);
extern int	_dr2(void);
extern int	_dr3(void);
extern int	_dr6(void);
extern int	_dr7(void);
extern void	loadtr(ulong);
extern caddr_t	_ebp(void);
extern caddr_t	_esp(void);
extern ulong	_ebx(void);
extern ulong	_edi(void);
extern ulong	_esi(void);
extern void	outl(unsigned, ulong);
extern void	outw(unsigned, ulong);
extern void	outb(unsigned, ulong);
extern ulong	inl(unsigned);
extern ulong	inw(unsigned);
extern ulong	inb(unsigned);
extern int	intr_disable(void);
extern void	intr_restore(int);
extern int	struct_zero(caddr_t, int);

#else	/* __STDC__ */

extern void	flushtlb();	
extern int	_cr0();
extern int	_cr2();
extern int	_cr3();
extern void	_wdr0();
extern void	_wdr1();
extern void	_wdr2();
extern void	_wdr3();
extern void	_wdr6();
extern void	_wdr7();
extern int	_dr0();
extern int	_dr1();
extern int	_dr2();
extern int	_dr3();
extern int	_dr6();
extern int	_dr7();
extern void	loadtr();
extern caddr_t	_ebp();
extern caddr_t	_esp();
extern ulong	_ebx();
extern ulong	_edi();
extern ulong	_esi();
extern void	outl();
extern void	outw();
extern void	outb();
extern ulong	inl();
extern ulong	inw();
extern ulong	inb();
extern int	intr_disable();
extern void	intr_restore();
extern int	struct_zero();

#endif	/* __STDC__ */

#ifdef	KPERF  /* this is for kernel performance tool */
#if defined(__USLC__)
asm int
get_spl()
{
	movl	ipl, %eax
}
#else
extern void get_spl();
#endif
#endif	/* KPERF */

#endif	/* _UTIL_INLINE_H */
