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

#ifndef _FS_CDFS_CDFS_INODE_H
#define _FS_CDFS_CDFS_INODE_H

#ident	"@(#)uts-comm:fs/cdfs/cdfs_inode.h	1.11"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef	_FS_CDFS_CDFS_SUSP_H
#include <fs/cdfs/cdfs_susp.h>	/* REQUIRED */
#endif

#ifndef	_FS_CDFS_ISO9660_h
#include <fs/cdfs/iso9660.h>	/* REQUIRED */
#endif

#ifndef	_FS_PATHNAME_H
#include <fs/pathname.h>	/* REQUIRED */
#endif

#ifndef	_FS_STATVFS_H
#include <fs/statvfs.h>		/* REQUIRED */
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

#include <sys/fs/cdfs_susp.h>	/* REQUIRED */
#include <sys/fs/iso9660.h>	/* REQUIRED */
#include <sys/pathname.h>	/* REQUIRED */
#include <sys/statvfs.h>	/* REQUIRED */
#include <sys/vfs.h>		/* REQUIRED */
#include <sys/vnode.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#else

#include <sys/types.h>
#include <sys/pathname.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/fs/cdfs_susp.h>
#include <sys/fs/iso9660.h>

#endif /* _KERNEL_HEADERS */


/*
 * CDFS File ID structure that uniquely identifies a specific
 * file/dir within a specific CDFS file-system.  This data
 * identifies the location of the first Directory Record
 * of the file/dir in question.
 *
 * WARNING:
 * The size of 'cdfs_fid' CAN NOT be greater than MAXFIDSZ (vfs.h)
 * and (if using NFS) NFS_FHMAXDATA (nfs.h).  Refer to
 * cdfs_fid() (cdfs_vnops.c) and makefh() (nfs_export.c) for details.
 */
struct cdfs_fid {
	daddr_t		fid_SectNum;				/* Log. Sect of 1st Dir Rec		*/
	uint_t		fid_Offset;					/* Sect offset of 1st Dir Rec	*/
};



/*
 * Macro to compare two File ID structures.
 * Note: 'fid_Offset' is most likely to be different.
 */
#define CDFS_CMPFID(fid1, fid2) ( \
	((((fid1)->fid_Offset) == ((fid2)->fid_Offset)) && \
	(((fid1)->fid_SectNum) == ((fid2)->fid_SectNum))) \
	? B_TRUE : B_FALSE \
)



struct cdfs_drec {
	struct cdfs_drec *drec_NextDR;			/* Pointer to next Dir Rec		*/
	struct cdfs_drec *drec_PrevDR;			/* Pointer to previous Dir Rec	*/
	uint_t			drec_Loc;				/* Loc of media DREC (L-Sec #)	*/
	uint_t			drec_Offset;			/* # bytes from L-sec start		*/
	uint_t			drec_Len;				/* Len of media Dir Rec (Bytes)	*/
	uint_t			drec_XarLen;			/* Len of media XAR (Log Blk)	*/
	daddr_t			drec_ExtLoc;			/* Location of Extent (L-Blk #)	*/
	uint_t			drec_DataLen;			/* Len of File Sec data			*/
	timestruc_t		drec_Date;				/* Recording date and time		*/
	uint_t			drec_Flags;				/* Flags - See below			*/
	uint_t			drec_UnitSz;			/* File Unit Size				*/
	uint_t			drec_Interleave;		/* Interleave Gap Size			*/
	uint_t			drec_VolSeqNum;			/* Volume Sequence Number		*/
	uint_t			drec_FileIDLen;			/* Len of File ID String		*/
	uint_t		 	drec_FileIDOff;			/* Dir Rec offset of File ID	*/	
	uint_t		 	drec_SysUseOff;			/* Dir Rec offset of Sys Use Area*/
	uint_t		 	drec_SysUseSz;			/* Size of Sys Use Area			*/
};

/*
 * Bit definition of 'drec_Flags' field.
 */
#define	CDFS_DREC_EXIST		0x01			/* Hide the file's existence	*/
#define CDFS_DREC_DIR		0x02			/* Entry is a Directory			*/
#define CDFS_DREC_ASSOC		0x04			/* Entry is an Associated File	*/
#define CDFS_DREC_REC		0x08			/* File data has "Record" format*/
#define CDFS_DREC_PROTECT	0x10			/* File has valid permission data*/
#define CDFS_DREC_RESERVE	0x60			/* Reserved						*/
#define CDFS_DREC_MULTI		0x80			/* Additional Dir Records follow*/

/*
 * Define the various Record Formats for the data in a file.
 */
enum cdfs_recfmt {
	CDFS_RECFMT_NONE	= 0,				/* No record format				*/
	CDFS_RECFMT_FIXED	= 1,				/* Fixed length records			*/
	CDFS_RECFMT_VAR1	= 2,				/* Variable length records: Type 1*/
	CDFS_RECFMT_VAR2	= 3					/* Variable length records: Type 2*/
};

/*
 * Define the various Record Attributes for the Records of a file.
 * A record attribute defines the control characters that preceed
 * the actual record data.
 */
enum cdfs_recattr {
	CDFS_RECATTR_CRLF	= 0,				/* Begins with a CR and LF chars*/
	CDFS_RECATTR_1539	= 1,				/* 1st char conforms to ISO-1539*/
	CDFS_RECATTR_NONE	= 2					/* No leading control characters*/
};



/*
 * Common XAR structure.
 */
struct cdfs_xar {
	uid_t				xar_UserID;			/* User ID						*/
	gid_t				xar_GroupID;		/* Group ID						*/
	uint_t				xar_Perms;			/* Mode/Perm flags - See below	*/
	timestruc_t			xar_CreateDate;		/* File creation date/time		*/
	timestruc_t			xar_ModDate;		/* File modification date/time	*/
	timestruc_t			xar_ExpireDate;		/* File expiration date/time	*/
	timestruc_t			xar_EffectDate;		/* File Effective date/time		*/
	enum cdfs_recfmt	xar_RecFmt;			/* File record format			*/
	enum cdfs_recattr	xar_RecAttr;		/* File record attribute		*/
	uint_t				xar_RecLen;			/* File record length (Bytes)	*/
	uchar_t				*xar_SysID;			/* System ID string				*/
	uchar_t				*xar_SysUse;		/* System Use Area				*/
	uint_t				xar_EscSeqLen;		/* Len of Escape Sequences		*/ 
	uint_t				xar_ApplUseLen;		/* Len of Application Use Area	*/	
	uchar_t				*xar_EscSeq;		/* Escape Sequences				*/
	uchar_t				*xar_ApplUse;		/* Application Use Area			*/
};

/*
 * Bit definitions of Inode Permission field.
 * Note: All reserved bits are set to one.
 */
#define	CDFS_XAR_SYSUSER		0x0001		/* System-group Read bit		*/
#define CDFS_XAR_SYSGROUP		0x0004		/* System-group Execute bit		*/
#define CDFS_XAR_OWNREAD		0x0010		/* File-owner Read bit			*/
#define CDFS_XAR_OWNEXEC		0x0040		/* File-owner Execute bit		*/
#define CDFS_XAR_GROUPREAD		0x0100		/* File-group Read bit			*/
#define CDFS_XAR_GROUPEXEC		0x0400		/* File-group Execute bit		*/
#define CDFS_XAR_OTHERREAD		0x1000		/* Other Read bit				*/
#define CDFS_XAR_OTHEREXEC		0x4000		/* Other Execute bit			*/
#define CDFS_XAR_NONESET		0xAAAA		/* All other bits set to 1		*/		



/*
 * CDFS Inode structure contains the relevent information
 * of an individual file or directory.
 */
struct cdfs_inode {
	struct cdfs_inode	*i_FreeFwd;			/* Free list forward link		*/
	struct cdfs_inode	*i_FreeBack;		/* Free list backward link		*/
	struct cdfs_inode	*i_HashFwd;			/* Hash list forward link		*/
	struct cdfs_inode	*i_HashBack;		/* Hash list backward link		*/
	uint_t				i_Flags;			/* Inode flags - See CDFS struct*/
	struct cdfs_fid		i_Fid;				/* File ID info					*/
	struct cdfs_fid		i_ParentFid;		/* Parent's File ID info		*/
	uid_t				i_UserID;			/* User ID						*/
	gid_t				i_GroupID;			/* Group ID						*/
	uint_t				i_Mode;				/* File type, Mode, and Perms	*/	
	uint_t				i_Size;				/* Total # of bytes in file		*/
	uint_t				i_LinkCnt;			/* # of links to file			*/
	dev_t				i_DevNum;			/* Device # of BLK/CHR file type*/
	ulong_t				i_LockOwner;		/* Process # of owner of lock	*/
	short				i_Count;			/* # of inode locks by lock owner*/
	uint_t				i_DRcount;			/* # of Directory Records		*/
	struct vfs			*i_vfs;				/* File sys associated with inode*/
	daddr_t				i_NextByte;			/* Next read-ahead offset (Byte)*/
	int					i_mapsz;			/* kmem_alloc'ed size			*/
	long				i_mapcnt;			/* mappings to file pages		*/
	struct cdfs_drec	*i_DirRec;			/* 1st link-list Dir Rec of file*/
	struct cdfs_xar		*i_Xar;				/* XAR info from last Dir Rec	*/
	struct cdfs_rrip	*i_Rrip;			/* RRIP info from last Dir Rec	*/
	struct vnode		*i_Vnode;			/* Vnode associated with Inode	*/
	timestruc_t			i_AccessDate;		/* File Access date/time		*/
	timestruc_t			i_ModDate;			/* File Modification date/time	*/
	timestruc_t			i_CreateDate;		/* File Creation date/time		*/
	timestruc_t			i_ExpireDate;		/* File Expiration date/time	*/
	timestruc_t			i_EffectDate;		/* File Effective date/time		*/
	timestruc_t			i_AttrDate;			/* File Attribute Change date/time*/
	timestruc_t			i_BackupDate;		/* File Backup date/time		*/
	struct pathname		i_SymLink;			/* Symbolic Link pathname		*/
	off_t				i_DirOffset;		/* Dir offset of last ref'd entry*/
	ulong				i_VerCode;			/* Version code attribute		*/
	daddr_t				i_ReadAhead;		/* File offset of read-ahead I/O*/
	/*
	 * The following fields cause storage to be allocated for the
	 * corresponding data structures.  Since each inode will usually
	 * need each of these structures, this is a simple mechanism for
	 * getting the needed storage.  Reference to these structures should
	 * be done via the corresponding pointers allocated above.  Thus,
	 * if the storage is to be dynamically allocated, very little
	 * code needs to change.
	 */
	struct cdfs_drec	i_DirRecStorage;	/* Static storage for i_DirRec	*/
	struct cdfs_xar		i_XarStorage;		/* Static storage for i_Xar		*/
	struct cdfs_rrip	i_RripStorage;		/* Static storage for i_Rrip	*/
	struct vnode		i_VnodeStorage;		/* Static storage for i_Vnode	*/
};

/*
 * Bit definitions of Inode Flag field.
 */
#define	ILOCKED				0x00000001		/* Inode structure is locked	*/
#define	IUPD				0x00000002		/* File has been modified		*/
#define	IACC				0x00000004		/* Inode access time needs update*/
#define	IMOD				0x00000008		/* Inode has been modified		*/
#define	IWANT				0x00000010		/* Process waiting on Inode lock*/
#define	ISYNC				0x00000020		/* Do allocations synchronously	*/
#define	ICHG				0x00000040		/* Inode has been changed		*/
#define	ILWAIT				0x00000080		/* Process waiting on file lock */
#define	IREF				0x00000100		/* Inode is being referenced	*/
#define	INOACC				0x00000200		/* No access time update:getpage()*/
#define	IMODTIME			0x00000400		/* Mod time already set			*/
#define	IINACTIVE			0x00000800		/* Inode iinactive in progress	*/
#define	IRWLOCKED			0x00001000		/* Inode's Read/Write Lock set	*/

#define CDFS_INODE_HIDDEN	0x00010000		/* Hide file from user			*/
#define CDFS_INODE_ASSOC	0x00020000		/* Associated file type			*/

#define CDFS_INODE_PERM_OK	0x00100000		/* Inode's permission is valid	*/
#define CDFS_INODE_UID_OK	0x00200000		/* Inode's User ID is valid		*/
#define CDFS_INODE_GID_OK	0x00400000		/* Inode's Group ID is valid	*/
#define CDFS_INODE_DEV_OK	0x00800000		/* Inode's Device Num is valid	*/

#define CDFS_INODE_RRIP_REL	0x10000000		/* Inode data relocated via RRIP*/

/*
 * Bit definitions of Inode Mode field.
 * - File type definitions.
 * - Special mode flags.
 * - Permission flags: Can be used for each perms set (User, Group, Other)
 *   by right-shifting the appropriate amount:
 *   (User = Bits 6-8), (Group = Bits 3-5), (Other = Bits 0-2)
 */
#define	IFMT		0170000					/* File type mask				*/
#define	IFIFO		0010000					/* Named pipe (fifo) type file	*/
#define	IFCHR		0020000					/* Character special 			*/
#define	IFDIR		0040000					/* Directory					*/
#define	IFBLK		0060000					/* Block special				*/
#define	IFREG		0100000					/* Regular						*/
#define	IFLNK		0120000					/* Symbolic link				*/
#define	IFSOCK		0140000					/* Socket						*/

#define	ISUID		0004000					/* Set User ID on execution		*/
#define	ISGID		0002000					/* set Group ID on execution	*/
#define	ISVTX		0001000					/* Save swapped text after use	*/

#define	IREAD		0000400					/* Read, Write, Execute perms	*/
#define	IWRITE		0000200
#define	IEXEC		0000100

#define IUSER_SHIFT		0
#define IGROUP_SHIFT	3
#define IOTHER_SHIFT	6

#define IREAD_USER		(IREAD >> IUSER_SHIFT)
#define IWRITE_USER		(IWRITE >> IUSER_SHIFT)
#define IEXEC_USER		(IEXEC >> IUSER_SHIFT)
#define IREAD_GROUP		(IREAD >> IGROUP_SHIFT)
#define IWRITE_GROUP	(IWRITE >> IGROUP_SHIFT)
#define IEXEC_GROUP		(IEXEC >> IGROUP_SHIFT)
#define IREAD_OTHER		(IREAD >> IOTHER_SHIFT)
#define IWRITE_OTHER	(IWRITE >> IOTHER_SHIFT)
#define IEXEC_OTHER		(IEXEC >> IOTHER_SHIFT)



/*
 * Define the Inode Hashing Function.  If the size of the
 * Inode Table is an even power-of-two, a fast hashing function
 * function can be used.   Otherwise, we have to use a slow one.
 */
#define	CDFS_INOHASH(fid) ( \
	((cdfs_IhashCnt & (cdfs_IhashCnt-1)) == 0) \
	? \
		(((uint_t)((fid)->fid_SectNum) + \
		(uint_t)((fid)->fid_Offset)) & (cdfs_IhashCnt-1)) \
	: \
		(((uint_t)((fid)->fid_SectNum) + \
		(uint_t)((fid)->fid_Offset)) % (cdfs_IhashCnt-1)) \
)

#define ITOV(ip)	((ip)->i_Vnode)
#define VTOI(vp)	((struct cdfs_inode *)(vp)->v_data)

/*
 * XXX - The current definition of an Inode number "wastes" many
 * potential values of an ino_t type.  Instead of using a
 * sector/offset of the Dir Rec, it may be better (i.e. a more
 * efficient use of the value range of an ino_t) to use the
 * sector # of the first Dir Rec in the directory (i.e. the DOT
 * entry) and an occurence # of the desired Dir Rec.  However,
 * keeping track of the "Dir Rec count" will require additional code.
 */
#define	CDFS_INUM(vfs, sect, offset)	\
	((ino_t)(((sect) << CDFS_SECTSHFT(vfs)) + (offset)))

#endif	/* _FS_CDFS_CDFS_INODE_H */
