/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _STDARG_H
#define _STDARG_H

#ident	"@(#)sgs-head:stdarg.h	1.10"

#if defined(__STDC__)

#ifndef _VA_LIST
#if #machine(i860)
#	define _VA_LIST	struct _va_list
	struct _va_list {
		unsigned _ireg_used;
		unsigned _freg_used;
		long *_reg_base;
		long *_mem_ptr;
	};
#else
#	define _VA_LIST	void *
#endif	/* machine(i860)	*/
#endif	/* _VA_LIST	*/

typedef _VA_LIST	va_list;

#if #machine(i860)
#ifndef __HIGHC__
extern void  __i860_builtin_va_start(va_list *);
extern void *__builtin_va_arg();
#define va_start(ap,parm) __i860_builtin_va_start(&ap)
#define va_arg(ap,type) (*(type *)__builtin_va_arg(&ap, (type *)0))
#else	/* __HIGHC__ version */
#define va_start(ap,parm)				\
	{extern char _ADDRESS_OF_MEMOFLO_AREA[];	\
	extern long _ADDRESS_OF_INT_END_AREA[];		\
	extern int  _PARMBYTES_USEDI##parm[],		\
		    _PARMBYTES_USEDF##parm[],		\
		    _PARMBYTES_USEDM##parm[];		\
	ap._ireg_used = (int)_PARMBYTES_USEDI##parm>>2,	\
	ap._freg_used = (int)_PARMBYTES_USEDF##parm>>2,	\
	ap._mem_ptr = (void *)&parm,			\
	ap._mem_ptr = _ADDRESS_OF_MEMOFLO_AREA +		\
		(int)_PARMBYTES_USEDM##parm;		\
	ap._reg_base = (int)_ADDRESS_OF_INT_END_AREA-80;	\
	}
#define va_arg(ap,type) \
	(*(type*) _va_arg(&ap,sizeof(type),_INFO(type,0),_INFO(type,2)))
extern void *_va_arg(va_list *ap, unsigned len, unsigned align, unsigned class);
#endif

#else	/* i860 */

#if #machine(sparc)
#define va_start(list, name) (void) (list = (va_list) &__builtin_va_alist)
#else
#if __STDC__ != 0				/* -Xc compilation */
#define va_start(list, name) (void) (list = \
	(void *)((char *)&name + ((sizeof(name)+(sizeof(int)-1)) & ~(sizeof(int)-1))))
#else
#define va_start(list, name) (void) (list = (void *)((char *)&...))
#endif 	/* != 0 */
#endif	/* sparc */

#if #machine(m88k)
#define va_align(list,mode) (char *)(((int)list + sizeof(mode) - 1) & (~(sizeof(mode)-1)))
#define va_arg(list, mode) ((mode *)(list = va_align(list, mode) + sizeof(mode)))[-1]
#elif #machine(sparc)
#define va_arg(list, mode) ((mode *)__builtin_va_arg_incr((mode *)list))[0]
#else
#define va_arg(list, mode) ((mode *)(list = (char *)list + sizeof(mode)))[-1]
#endif

#endif	/* i860 */

extern void va_end(va_list);

#define va_end(list) (void)0

#else	/* not __STDC__ */

#include <varargs.h>

#endif	/* __STDC__ */

#endif 	/* _STDARG_H */
