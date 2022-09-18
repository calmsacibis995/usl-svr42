/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _BOOT_SIP_H	/* wrapper symbol for kernel use */
#define _BOOT_SIP_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:boot/sip.h	1.4"
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

#define SIP_fdesc_p	(lpcbp->lp_pdata[0])/* addr ptr to boot flat desc */
#define SIP_kentry	(lpcbp->lp_pdata[1])/* kernel entry point	  */
#define SIP_lpcb_p	(lpcbp->lp_pdata[2])/* addr ptr to lpcb		  */

#endif	/* _BOOT_SIP_H */
