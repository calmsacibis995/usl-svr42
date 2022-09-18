/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1991, 1992  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION CONFIDENTIAL INFORMATION	*/

/*	This software is supplied to USL under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ifndef _FS_CDFS_CDFS_SUSP_H
#define _FS_CDFS_CDFS_SUSP_H

#ident	"@(#)uts-comm:fs/cdfs/cdfs_susp.h	1.9"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef	_FS_CDFS_ISO9660_h
#include <fs/cdfs/iso9660.h>	/* REQUIRED */
#endif

#ifndef	_FS_PATHNAME_H
#include <fs/pathname.h>	/* REQUIRED */
#endif

#ifndef	_FS_VFS_H
#include <fs/vfs.h>		/* REQUIRED */
#endif

#ifndef	_FS_VNODE_H
#include <fs/vnode.h>		/* REQUIRED */
#endif

#ifndef	_UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/fs/iso9660.h>	/* REQUIRED */
#include <sys/pathname.h>	/* REQUIRED */
#include <sys/vfs.h>		/* REQUIRED */
#include <sys/vnode.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#else

#include <sys/types.h>
#include <sys/pathname.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/fs/iso9660.h>

#endif /* _KERNEL_HEADERS */



/*
 * Define a SUF structure.
 */
union media_suf {
	struct susp_suf	gen;
	struct susp_sp	sp;
	struct susp_ce	ce;
	struct susp_pd	pd;
	struct susp_st	st;
	struct susp_er	er;
	struct rrip_px	px;
	struct rrip_pn	pn;
	struct rrip_sl	sl;
	struct rrip_nm	nm;
	struct rrip_cl	cl;
	struct rrip_pl	pl;
	struct rrip_re	re;
	struct rrip_tf	tf;
	struct rrip_rr	rr;
};

#define CDFS_SUFID(x,y)	((uint_t)(((uchar_t)(x) << 8) | (uchar_t)(y)))

#define CDFS_SUFID_NULL	CDFS_SUFID('\0','\0')
#define CDFS_SUFID_SP	CDFS_SUFID(SUSP_SP_SIG1,SUSP_SP_SIG2)
#define CDFS_SUFID_CE	CDFS_SUFID(SUSP_CE_SIG1,SUSP_CE_SIG2)
#define CDFS_SUFID_PD	CDFS_SUFID(SUSP_PD_SIG1,SUSP_PD_SIG2)
#define CDFS_SUFID_ST	CDFS_SUFID(SUSP_ST_SIG1,SUSP_ST_SIG2)
#define CDFS_SUFID_ER	CDFS_SUFID(SUSP_ER_SIG1,SUSP_ER_SIG2)

#define CDFS_SUFID_PX	CDFS_SUFID(RRIP_PX_SIG1,RRIP_PX_SIG2)
#define CDFS_SUFID_PN	CDFS_SUFID(RRIP_PN_SIG1,RRIP_PN_SIG2)
#define CDFS_SUFID_SL	CDFS_SUFID(RRIP_SL_SIG1,RRIP_SL_SIG2)
#define CDFS_SUFID_NM	CDFS_SUFID(RRIP_NM_SIG1,RRIP_NM_SIG2)
#define CDFS_SUFID_CL	CDFS_SUFID(RRIP_CL_SIG1,RRIP_CL_SIG2)
#define CDFS_SUFID_PL	CDFS_SUFID(RRIP_PL_SIG1,RRIP_PL_SIG2)
#define CDFS_SUFID_RE	CDFS_SUFID(RRIP_RE_SIG1,RRIP_RE_SIG2)
#define CDFS_SUFID_TF	CDFS_SUFID(RRIP_TF_SIG1,RRIP_TF_SIG2)
#define CDFS_SUFID_RR	CDFS_SUFID(RRIP_RR_SIG1,RRIP_RR_SIG2)



/*
 * RRIP information from last Dir Rec of file.
 */
struct cdfs_rrip {
	uint_t				rrip_Flags;			/* Valid RRIP fields			*/
	uint_t				rrip_Mode;			/* Mode/permission fields		*/
	uint_t				rrip_LinkCnt;		/* # of links for this file		*/	
	uid_t				rrip_UserID;		/* User ID						*/
	gid_t				rrip_GroupID;		/* Group ID						*/
	uint_t				rrip_DevNum_Hi;		/* High 32-bits of Dev Num		*/
	uint_t				rrip_DevNum_Lo;		/* Low 32-bits of Dev Num		*/
	struct pathname		rrip_SymLink;		/* Symbolic link pathname		*/
	struct pathname		rrip_AltName;		/* Alternate file name			*/
	daddr_t				rrip_ChildLink;		/* New loc of relocated directory*/ 
	daddr_t				rrip_ParentLink;	/* Loc of original parent dir	*/
	timestruc_t			rrip_CreateDate;	/* Date/time of file creation	*/
	timestruc_t			rrip_ModDate;		/* Date/time of last modification*/
	timestruc_t			rrip_ExpireDate;	/* Date/time of data expiration	*/
	timestruc_t			rrip_EffectDate;	/* Data/time of data effective	*/
	timestruc_t			rrip_AccessDate;	/* Date/time of last accessed	*/
	timestruc_t			rrip_AttrDate;		/* Date/time attributes changed */
	timestruc_t			rrip_BackupDate;	/* Date/time of last backup		*/
};

#endif	/* _FS_CDFS_CDFS_SUSP_H */
