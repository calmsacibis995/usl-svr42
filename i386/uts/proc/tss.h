/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_TSS_H	/* wrapper symbol for kernel use */
#define _PROC_TSS_H	/* subject to change without notice */

#ident	"@(#)uts-x86:proc/tss.h	1.6"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/* Flags Register */

typedef struct flags {
	uint	fl_cf	:  1,		/* carry/borrow */
			:  1,		/* reserved */
		fl_pf	:  1,		/* parity */
			:  1,		/* reserved */
		fl_af	:  1,		/* carry/borrow */
			:  1,		/* reserved */
		fl_zf	:  1,		/* zero */
		fl_sf	:  1,		/* sign */
		fl_tf	:  1,		/* trace */
		fl_if	:  1,		/* interrupt enable */
		fl_df	:  1,		/* direction */
		fl_of	:  1,		/* overflow */
		fl_iopl :  2,		/* I/O privilege level */
		fl_nt	:  1,		/* nested task */
			:  1,		/* reserved */
		fl_rf	:  1,		/* resume flag */
		fl_vm	:  1,		/* virtual 86 mode */
		fl_ac	:  1,		/* alignment check */
		fl_vif	:  1,		/* virtual interrupt flag */
		fl_vip	:  1,		/* virtual interrupt pending */
		fl_id	:  1,		/* indentification */
		fl_res	: 10;		/* reserved */
} flags_t;

#define	PS_C		0x00000001		/* carry bit			*/
#define	PS_P		0x00000004		/* parity bit			*/
#define	PS_AC		0x00000010		/* auxiliary carry bit		*/
#define	PS_Z		0x00000040		/* zero bit			*/
#define	PS_N		0x00000080		/* negative bit			*/
#define	PS_T		0x00000100		/* trace enable bit		*/
#define	PS_IE		0x00000200		/* interrupt enable bit		*/
#define	PS_D		0x00000400		/* direction bit		*/
#define	PS_V		0x00000800		/* overflow bit			*/
#define	PS_IOPL		0x00003000		/* I/O privilege level		*/
#define	PS_NT		0x00004000		/* nested task flag		*/
#define	PS_RF		0x00010000		/* Resume Flag			*/
#define	PS_VM		0x00020000		/* Virtual 86 mode flag		*/
#define	PS_ACK		0x00040000		/* Alignment Check flag		*/
#define	PS_VIF		0x00080000		/* Virtual Interrupt Flag	*/
#define	PS_VIP		0x00100000		/* Virtual Interrupt Pending	*/
#define	PS_ID		0x00200000		/* IDentification flag		*/


/*
 * System bits in the flags register.
 * We do not allow the user to change these.
 */
#define PS_SYSMASK	(PS_T | PS_IE | PS_IOPL | PS_NT | PS_RF | PS_VM)

/*
 * Maximum I/O address that will be in TSS bitmap.
 */
#define	MAXTSSIOADDR	0x3ff

/*
 * 386 TSS definition.
 */

struct tss386 {
	unsigned long t_link;
	unsigned long t_esp0;
	unsigned long t_ss0;
	unsigned long t_esp1;
	unsigned long t_ss1;
	unsigned long t_esp2;
	unsigned long t_ss2;
	paddr_t	      t_cr3;
	unsigned long t_eip;
	unsigned long t_eflags;
	unsigned long t_eax;
	unsigned long t_ecx;
	unsigned long t_edx;
	unsigned long t_ebx;
	unsigned long t_esp;
	unsigned long t_ebp;
	unsigned long t_esi;
	unsigned long t_edi;
	unsigned long t_es;
	unsigned long t_cs;
	unsigned long t_ss;
	unsigned long t_ds;
	unsigned long t_fs;
	unsigned long t_gs;
	unsigned long t_ldt;
	unsigned long t_bitmapbase;
};

#define		PS_USER		3
#define		PS_KERNEL	0

#endif	/* _PROC_TSS_H */
