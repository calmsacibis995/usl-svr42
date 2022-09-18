/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _SVC_BRGMGR_H	/* wrapper symbol for kernel use */
#define _SVC_BRGMGR_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:svc/brgmgr.h	1.4"
#ident	"$Header: $"

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#define BCBmagic	0xBAD1BAD2
#define BCBversion	0x10		/* version 1.0			*/

struct brg_ctl_blk {
		uint	bcb_magic;	/* identification number	*/
		paddr_t	bcb_kvbase;	/* constant KVBASE		*/
		paddr_t	bcb_kvsbase;	/* constant KVSBASE		*/
		paddr_t	bcb_kpt0;	/* address location of kpt0	*/
		paddr_t	bcb_kptn;	/* address location of kptn	*/
		paddr_t	bcb_stext;	/* address location of stext	*/
		paddr_t	bcb_sdata;	/* address location of sdata	*/
		paddr_t	bcb_sbss;	/* address location of sbss	*/
		paddr_t	bcb_df_stack;	/* address location of df_stack	*/
		uint 	bcb_dfstksiz;	/* constant DFSTKSIZ		*/
		paddr_t	bcb_gdtp;	/* address location of gdt	*/
		int	bcb_gdt_cnt;	/* number of gdt entries	*/
		paddr_t	bcb_idtp;	/* address location of idt	*/
		int	bcb_idt_cnt;	/* number of idt entries	*/
		paddr_t	bcb_scallp;	/* addr location of scall_dscr	*/
		paddr_t	bcb_sigretp;	/* addr location of sigret_dscr	*/
		paddr_t	bcb_idt2p;	/* address location of idt and	*/
		int	bcb_idt2_cnt;	/* its count - VPIX		*/

		uint	bcb_version;	/* version number		*/
		uint	bcb_ctl_flg;	/* control flag			*/
		paddr_t	bcb_stat_flg;	/* status flag			*/
		uint	bcb_jtsssel;	/* constant JTSSSEL		*/
		uint	bcb_ktsssel;	/* constant KTSSSEL		*/
		paddr_t	bcb_bootinfo;	/* addr of struct bootinfo	*/
		paddr_t	bcb_sysenvmt;	/* addr of struct sysenvmt	*/
		paddr_t	bcb_bps;	/* addr of boot parameter string*/
		paddr_t	bcb_uprt;	/* entry point for uprt start 	*/
		paddr_t bcb_kstart;	/* entry point for kernel start */
		long	bcb_pad[10];	/* reserved 			*/
};


#endif	/* _SVC_BRGMGR_H */
