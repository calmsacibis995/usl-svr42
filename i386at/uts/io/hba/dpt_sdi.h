/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _IO_HBA_DPT_SDI_H	/* wrapper symbol for kernel use */
#define _IO_HBA_DPT_SDI_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:io/hba/dpt_sdi.h	1.5"
#ident  "$Header: dpt_sdi.h 1.1 $"

/*      Copyright (c) 1991  Intel Corporation     */
/*	All Rights Reserved	*/

/*      INTEL CORPORATION PROPRIETARY INFORMATION                          */
/*                                                                         */
/*	This software is supplied under the terms of a license agreement   */
/*	or nondisclosure agreement with Intel Corporation and may not be   */
/*	copied or disclosed except in accordance with the terms of that    */
/*	agreement.							   */
/*                                                                         */

/*
**	sdi function definitions
*/

extern void dptintr();
extern struct hbadata *dptgetblk();
extern long dptfreeblk();
extern long dpticmd();
extern int dptinit();
extern void dptgetinfo();
extern long dptsend();
extern void dptxlat();
extern int dptopen();
extern int dptclose();
extern int dptioctl();

#endif /* _IO_HBA_DPT_SDI_H */
