/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROF_H
#define _PROF_H

#ident	"@(#)sgs-head:prof.h	1.10.4.2"

#ifndef MARK
#define MARK(K)	{}
#else
#undef MARK

#if defined(__STDC__)

#if #machine(i386)
#define MARK(K)	{\
		asm("	.data");\
		asm("	.align 4");\
		asm("."#K".:");\
		asm("	.long 0");\
		asm("	.text");\
		asm("M."#K":");\
		asm("	movl	$."#K".,%edx");\
		asm("	call _mcount");\
		}
#elif #machine(m68k)
#define MARK(K)	{\
                asm("   data");\
                asm("   align 4");\
                asm("K:");\
                asm("   long    0");\
                asm("   text");\
                asm("   pea	K");\
                asm("   jsr     mcount");\
                asm("   addq.l	4,%sp");\
                }
#elif #machine(m88k)
#define MARK(K)	{\
                asm("   data");\
                asm("   align 4");\
                asm("K:");\
                asm("   byte    0,0,0,0");\
                asm("   text");\
                asm("   subu    r31,r31,4");\
                asm("   st      r2,r31,0");\
                asm("   or.u    r2,r0,hi16(L)");\
                asm("   bsr.n   mcount");\
                asm("   or      r2,r2,lo16(L)");\
                asm("   ld	r2,r31,0");\
                asm("   addu    r31,r31,4");\
                }
#elif #machine(sparc)			
#define MARK(K) {\
		asm("	.reserve	."#K"., 4, \"data\", 4");\
		asm("M."#K":");\
		asm("	sethi	%hi(."#K".), %o0");\
		asm("	call	.mcount");\
		asm("	or	%o0, %lo(."#K".), %o0");\
		}				
#else
#define MARK(K)	{\
		asm("	.data");\
		asm("	.align	4");\
		asm("."#K".:");\
		asm("	.word	0");\
		asm("	.text");\
		asm("M."#K":");\
		asm("	movw	&."#K".,%r0");\
		asm("	jsb	_mcount");\
		}
#endif

#else
#if M32
#define MARK(K)	{\
		asm("	.data");\
		asm("	.align	4");\
		asm(".K.:");\
		asm("	.word	0");\
		asm("	.text");\
		asm("M.K:");\
		asm("	movw	&.K.,%r0");\
		asm("	jsb	_mcount");\
		}
#endif
#ifdef i386
#define MARK(K)	{\
		asm("	.data");\
		asm("	.align 4");\
		asm(".K.:");\
		asm("	.long 0");\
		asm("	.text");\
		asm("M.K:");\
		asm("	movl	$.K.,%edx");\
		asm("	call _mcount");\
		}
#endif
#ifdef m68k
#define MARK(K)	{\
                asm("   data");\
                asm("   align 4");\
                asm("K:");\
                asm("   long    0");\
                asm("   text");\
                asm("   pea	K");\
                asm("   jsr     mcount");\
                asm("   addq.l	4,%sp");\
                }
#endif
#ifdef m88k
#define MARK(K)	{\
                asm("   data");\
                asm("   align 4");\
                asm("K:");\
                asm("   byte    0,0,0,0");\
                asm("   text");\
                asm("   subu    r31,r31,4");\
                asm("   st      r2,r31,0");\
                asm("   or.u    r2,r0,hi16(L)");\
                asm("   bsr.n   mcount");\
                asm("   or      r2,r2,lo16(L)");\
                asm("   ld	r2,r31,0");\
                asm("   addu    r31,r31,4");\
                }
#endif
#ifdef sparc		
#define MARK(K) {\
		asm("	.reserve	.K., 4, \"data\", 4");\
		asm("M.K:");\
		asm("	sethi	%hi(.K.), %o0");\
		asm("	call	.mcount");\
		asm("	or	%o0, %lo(.K.), %o0");\
		}		
#endif			

#endif	/* __STDC__ */

#endif  /* MARK */

#endif /* _PROF_H */
