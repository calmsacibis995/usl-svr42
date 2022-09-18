/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _BOOT_LIBFM_H	/* wrapper symbol for kernel use */
#define _BOOT_LIBFM_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:boot/libfm.h	1.4"
#ident  "$Header: $"

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
 /* "(c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990" */

#ifndef _UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif	/* _UTIL_TYPES_H */

#ifndef _PROC_OBJ_ELF_H
#include <proc/obj/elf.h>
#endif	/* _PROC_OBJ_ELF_H */

#include <filehdr.h>
#include <aouthdr.h>
#include <scnhdr.h>

#define BKINAME		('B' + ('K' << 8) + ('I' << 16))
#define BKIVERSION	2		

enum bfstyp { s5, BFS, UNKNOWN };
typedef enum bfstyp bfstyp_t;

#define DFL_BFS	BFS

struct	coffhdrs {
	struct	filehdr	mainhdr;
	struct	aouthdr	secondhdr;
	};

typedef union {
	struct coffhdrs	coff;
	Elf32_Ehdr	elf;
	} BFHDR;

enum lfsect { TLOAD, DLOAD, NOLOAD, BKI, BLOAD };
typedef enum lfsect lfsect_t;

/*  common program header for ELF or COFF 				*/
struct bootproghdr {
	lfsect_t p_type;	/* type of section 			*/
	ulong p_vaddr;		/* virtual address to load section 	*/
	ulong p_memsz;		/* memory size of section 		*/
	ulong p_filsz;		/* file size of section 		*/
	off_t p_offset;		/* offset in file 			*/
};

enum lfhdr { COFF, ELF, NONE};
typedef enum lfhdr lfhdr_t;

				/* max boot program headers		*/
#define NBPH	5		/* text, data, bss and bki		*/
struct bftbl {
	lfhdr_t	t_type;		/* type of file 			*/
	int 	t_nsect;	/* number of sections or segments 	*/
	ulong 	t_entry;	/* entry point virtual 			*/
	ulong	t_offset;	/* file offset to program headers	*/
	int	t_nbph;		/* number of boot program headers	*/
	struct	bootproghdr t_bph[NBPH]; /* boot program header		*/
};

/*	loadable program names						*/
#define	SIP	0
#define MIP	1
#define KERNEL	2

#define	NPDATA	10
/*	loadable program control block					*/
struct lpcb {
	int 	lp_type;	/* type of file 			*/
	ulong	lp_flag;	/* status flag				*/
	ulong	lp_entry;	/* entry point - physical		*/
	paddr_t	lp_memsrt;	/* starting memory address - physical	*/
	paddr_t	lp_memend;	/* ending memory address - physical	*/
	char	*lp_path;	/* program path name			*/
	struct	bftbl lp_bftbl; /* boot file table			*/
	char	*lp_pptr;	/* program pointer			*/
	ulong	lp_pdata[NPDATA];/* program data area			*/
};
#define	LP_BFTBL	(lpcbp->lp_bftbl)	

/*	command flag	- SIP						*/
#define	SIP_INIT	0x1	/* collect system and BIOS parameters	*/
#define SIP_KPREP	0x2	/* prepare system for kernel loading	*/
#define SIP_KSTART	0x3	/* startup kernel			*/

/*	command flag	- MIP						*/
#define MIP_INIT	0x1	/* identify machine & startup machine	*/
#define MIP_END		0x2	/* complete final machine startup	*/

/*	global buffer cache 						*/
struct gcache {
	daddr_t	gc_bno;
	int	gc_cnt;
};

#endif	/* _BOOT_LIBFM_H */
