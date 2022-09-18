/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _BOOT_MIP_H	/* wrapper symbol for kernel use */
#define _BOOT_MIP_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:boot/mip.h	1.5"
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

struct machconfig {
	char		*sigaddr;	/* Machine signature location 	*/
	unsigned char	siglen;		/* Signature length 		*/
	unsigned char	sigid[10];	/* Signature to match 		*/
	unsigned char	old_mt;		/* OLD Machine type 		*/
	unsigned char	machine;	/* Machine type 		*/
	unsigned long	m_flag;		/* status flag			*/
	int		(*m_entry)();	/* machine entry point		*/
	};
#define M_FLG_SRGE	1	/* sig scattered in a range of memory	*/

#define M_ID_AT386	0
#define M_ID_MC386	1

#define SYS_MODEL() 	*(char *)0xFFFFE
#define MODEL_AT	(unsigned char)0xFC
#define MODEL_MC	(unsigned char)0xF8

#define MIP_init	(lpcbp->lp_pdata[0])/* machine init entry point	*/
#define MIP_end		(lpcbp->lp_pdata[1])/* machine final startup ent*/

#endif	/* _BOOT_MIP_H */
