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

#ifndef _FS_CDFS_CDROM_H	/* wrapper symbol for kernel use */
#define _FS_CDFS_CDROM_H	/* subject to change without notice */

#ident	"@(#)uts-comm:fs/cdfs/cdrom.h	1.6"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef	_FS_CDFS_CDFS_INODE_H
#include <fs/cdfs/cdfs_inode.h>	/* REQUIRED */
#endif

#ifndef	_FS_CDFS_ISO9660_h
#include <fs/cdfs/iso9660.h>	/* REQUIRED */
#endif

#ifndef	_UTIL_PARAM_H
#include <util/param.h>		/* REQUIRED */
#endif

#ifndef	_UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/fs/cdfs_inode.h>	/* REQUIRED */
#include <sys/fs/iso9660.h>	/* REQUIRED */
#include <sys/param.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#else

#include <sys/param.h>
#include <sys/types.h>
#include <sys/fs/iso9660.h>
#include <sys/fs/cdfs_inode.h>

#endif /* _KERNEL_HEADERS */



#define CD_ISO9660		CDFS_ISO_9660		/* CD-ROM recorded to ISO-9660	*/

/*
 * ISO-9660 Primary Volume Descriptor (PVD).
 */
struct iso9660_pvd {
	uchar_t		voldestype;					/* Volume Descriptor Type		*/
	uchar_t		std_id[5];					/* Standard ISO-9660 ID String	*/
	uchar_t		voldesvers;					/* Volume Descriptor Version #	*/
	uchar_t		sys_id[32];					/* System ID String				*/
	uchar_t		vol_id[32];					/* Volume ID String				*/
	ulong_t		volspcsize;					/* Volume Space Size (Blks)		*/
	ushort_t	volsetsize;					/* Volume Set Size (Disc Count)	*/
	ushort_t	volseqno;					/* Volume Sequence Number		*/
	ushort_t	lblksize;					/* Logical Block Size (Bytes)	*/
	ulong_t		ptsize;						/* Path Table Size (Bytes)		*/
	ulong_t		locpt_l;					/* Type-L Path Table Loc (Blk #)*/
	ulong_t		locptopt_l;					/* optional Type-L Path Table	*/
	ulong_t		locpt_m;					/* Type-M Path Table Loc (Blk #)*/
	ulong_t		locptopt_m;					/* Optional Type-M Path Table	*/
	uchar_t		rootdir[34];				/* Directory Record for Root (/)*/
	uchar_t		volset_id[128];				/* Volume Set ID String			*/
	uchar_t		pub_id[128];				/* Publisher ID String			*/
	uchar_t		dtpre_id[128];				/* Data Preparer ID String		*/
	uchar_t		app_id[128];				/* Application ID String		*/
	uchar_t		cpfile_id[37];				/* Copyright File Name			*/
	uchar_t		abfile_id[37];				/* Abstract File Name			*/
	uchar_t		bgfile_id[37];				/* Bibliographic File Name		*/
	time_t		cre_time;					/* Volume Creation Date/Time	*/
	time_t		mod_time;					/* Volume Modification Date/Time*/
	time_t		exp_time;					/* Volume Expiration Date/Time	*/
	time_t		eff_time;					/* Volume Effective Date/Time	*/
	uchar_t		filestrver;					/* File Structure Version #		*/
	uchar_t		res1;						/* Reserved: Byte 883 			*/
	uchar_t		appuse[512];				/* Application Use Area			*/
	uchar_t		res2[653];					/* Reserved: Bytes 1396-2048	*/
};

#define CD_PVDLEN	ISO_VD_LEN				/* Size of Primary Volume Descr	*/



/*
 * ISO-9660 Extended Attribute Record (XAR).
 */ 
struct iso9660_xar {
	ushort_t	own_id;						/* Owner (User) ID				*/
	ushort_t	grp_id;						/* Group ID						*/
	ushort_t	permissions;				/* Permissions (See below)		*/
	time_t		cre_time;					/* File Creation Date/Time		*/
	time_t		mod_time;					/* File Modification Date/Time	*/
	time_t		exp_time;					/* File Expiration Date/Time	*/
	time_t		eff_time;					/* File Effective Date/Time		*/
	uchar_t		rec_form;					/* Record Format				*/
	uchar_t		rec_attr;					/* Record Attributes			*/
	ushort_t	rec_len;					/* Record Length				*/
	uchar_t		sys_id[32];					/* System Use Area ID String	*/
	uchar_t		sys_use[64];				/* System Use Area				*/
	uchar_t		xar_vers;					/* XAR Version					*/
	uchar_t		esc_len;					/* Len of Escape Sequences		*/
	uchar_t		resv[64];					/* Reserved: Bytes 183-246		*/
	ushort_t	appuse_len;					/* Len of Application Use Area	*/
	uchar_t		*app_use;					/* Pntr to Application Use Area	*/
	uchar_t		*esc_seq;					/* Pntr to Escape Sequence Area	*/
};

/*
 * XAR Permission Bits.
 * Note: Unused bits in bitfield are set to 'ONE'.
 */
#define CD_RSYS		ISO_XAR_RSYS			/* System Class Read Perm Bit	*/
#define	CD_XSYS		ISO_XAR_XSYS			/* System Class Execute Perm Bit*/
#define	CD_RUSR		ISO_XAR_RUSR			/* User Read Permission Bit		*/
#define	CD_XUSR		ISO_XAR_XUSR			/* User Execute Permission Bit	*/
#define	CD_RGRP		ISO_XAR_RGRP			/* Group Read Permission Bit	*/
#define	CD_XGRP		ISO_XAR_XGRP			/* Group Execute Permission Bit	*/
#define	CD_ROTH		ISO_XAR_ROTH			/* Other Read Permission Bit	*/
#define	CD_XOTH		ISO_XAR_XOTH			/* Other Execute Permission Bit	*/

#define	CD_XARFIXL	ISO_XAR_FIXEDLEN		/* Fixed-part of XAR Size (Bytes)*/



/*
 * ISO-9660 Directory Record.
 */
struct iso9660_drec	{
	uchar_t		drec_len;					/* Len of Directory Rec (Bytes)	*/
	uchar_t		xar_len;					/* Len of XAR (Blocks)			*/
	ulong_t		locext;						/* Location of Extent (Block #)	*/
	ulong_t		data_len;					/* File Section	Size (Bytes)	*/
	time_t		rec_time;					/* Recording Date/Time			*/
	uchar_t		file_flags;					/* File Flags					*/
	uchar_t		file_usize;					/* File Unit Size (Blocks)		*/
	uchar_t		ileav_gsize;				/* Interleave Gap Size (Blocks)	*/
	ushort_t 	volseqno;					/* Volume Sequence Num (Disc #)	*/
	uchar_t		fileid_len;					/* Len of File Name (ID String)	*/
	/*
	 * Note that leading zeros in the file_id field will be stripped if
	 * the actual file identifier is longer than 37 bytes.
	 */
	uchar_t		file_id[37];				/* File Name (ID String)		*/
	/*
	 * Note that sysuse_len is equal to:
	 *	drec_len minus fileid_len minus either 34 (if fileid_len is odd) or
	 *		35 (if fileid_len is even)
	 */
	uchar_t		sysuse_len;					/* Len of Sys Use Area (Bytes)	*/
	uchar_t		sys_use[218];				/* System Use Area				*/
};

/*
 * Directory Record File Flags.
 */
#define CD_EXIST	ISO_DREC_EXIST			/* Existence bit				*/
#define CD_DIR		ISO_DREC_DIR			/* Directory bit				*/
#define CD_ASSOFILE	ISO_DREC_ASSOC			/* Associated File bit			*/
#define CD_RECORD	ISO_DREC_RECORD			/* Record bit					*/
#define CD_PROTEC	ISO_DREC_PROTECT		/* Protection bit				*/
#define CD_MULTIEXT	ISO_DREC_MULTIEXT		/* Multi-Extent bit				*/

#define CD_MAXDRECL	ISO_DREC_MAXLEN			/* Max Length of Dir Record		*/



/*
 * ISO-9660 Path Table Record, with Directory ID limited in size.
 */
struct iso9660_ptrec {
	uchar_t		dirid_len;					/* Dir Identifier Length (bytes)*/
	uchar_t		xar_len;					/* XAR Length (logical blocks)	*/
	ulong_t		loc_ext;					/* Location of Extent			*/
	ushort_t	pdirno;						/* Parent Directory Number		*/
	uchar_t		dir_id[31];					/* Directory Identifier			*/
};

#define CD_MAXPTRECL	40					/* Max Path Tab Rec Len (bytes)	*/



/*
 * XCDR defaults structure.
 */
struct cd_defs {
	uid_t		def_uid;					/* Default User ID				*/
	gid_t		def_gid;					/* Default Group ID				*/
	mode_t		def_fperm;					/* Default File Permissions		*/
	mode_t		def_dperm;					/* Default Directory Permissions*/
	/*
	 * dirsperm is effectively an enum with its possible values listed below.
	 */
	int			dirsperm;					/* Directory search permission	*/
};

/*
 * Source of directory execute permission.
 */
#define CD_DIRXAR	0x0001					/* XAR exec perm sets exec		*/
#define CD_DIRRX	0x0002					/* XAR rd|exec perm sets exec	*/



/*
 * XCDR ID mapping structure.
 */
struct cd_idmap {
	ushort_t	from_id;					/* Owner or Group ID on CD-ROM	*/
	uid_t		to_uid;						/* Owner ID in XSI file hierarchy*/
	gid_t		to_gid;						/* Group ID in XSI file hierarchy*/
};

/*
 * These symbol definitions are optional in the XCDR and RRIP specifications.
 * Applications should only use these symbols in code conditionally compiled
 * on the existence of the symbols.
 *
 * The minimum acceptable value for all is 50.  An unused entry is flagged
 * with a null string for the fileid.
 */

/*
 * From XCDR:
 */
#define CD_MAXUMAP	100
#define CD_MAXGMAP	100
/*
 * From RRIP:
 */
#define CD_MAXDMAP	100



/*
 * File name conversion flags.
 */
#define		CD_NOCONV		0x0001			/* Don't convert name 			*/
#define		CD_LOWER		0x0010			/* Lower case, no "." if no ext	*/
#define		CD_NOVERSION	0x0100			/* No version number and no ";"	*/



/*
 * Structure for providing RRIP device mapping.
 *
 * An unused entry is flagged with NULL in the fileid field, and
 * NODEV in the to_num field.
 */
struct cd_devmap {
	struct cdfs_fid	fileid;
	uchar_t	path[MAXPATHLEN];
	dev_t	to_num;
};

/*
 * Structure for providing XCDR UID mapping.
 *
 * An unused entry is flagged with CDFS_UNUSED_MAP_ENTRY in each field.
 */
struct cd_uidmap {
	uid_t	from_uid;
	uid_t	to_uid;
};

/*
 * Structure for providing XCDR GID mapping.
 *
 * An usused entry is flagged with CDFS_UNUSED_MAP_ENTRY in each field.
 */
struct cd_gidmap {
	gid_t	from_gid;
	gid_t	to_gid;
};


/*
 * XCDR Operations.
 */
#define CD_SETDEFS		0x0001				/* Set default values			*/
#define CD_GETDEFS		0x0002				/* Get default values			*/
#define CD_SETUMAP 		0x0010				/* Set User ID mapping			*/
#define CD_SETGMAP		0x0020				/* Set Group ID mapping			*/
#define CD_GETUMAP		0x0040				/* Get User ID mapping			*/
#define CD_GETGMAP		0x0080				/* Get Group ID mapping			*/
#define CD_SETNMCONV	0x0100				/* Set file name conversion		*/
#define CD_GETNMCONV	0x0200				/* Get file name conversion		*/

/*
 * RRIP Operations.
 */
#define CD_SETDMAP		0x1000				/* Set device file mapping		*/
#define CD_UNSETDMAP	0x2000				/* Unset device file mapping	*/

#endif /* _FS_CDFS_CDROM_H */
