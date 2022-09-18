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

#ifndef _FS_CDFS_CDFS_FS_H
#define _FS_CDFS_CDFS_FS_H

#ident	"@(#)uts-comm:fs/cdfs/cdfs_fs.h	1.9"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef	_FS_CDFS_CDFS_INODE_H
#include <fs/cdfs/cdfs_inode.h>	/* REQUIRED */
#endif

#ifndef	_FS_CDFS_CDROM_H
#include <fs/cdfs/cdrom.h>	/* REQUIRED */
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

#include <sys/fs/cdfs_inode.h>	/* REQUIRED */
#include <sys/cdrom.h>		/* REQUIRED */
#include <sys/fs/iso9660.h>	/* REQUIRED */
#include <sys/pathname.h>	/* REQUIRED */
#include <sys/statvfs.h>	/* REQUIRED */
#include <sys/vfs.h>		/* REQUIRED */
#include <sys/vnode.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#else

#include <sys/types.h>
#include <sys/pathname.h>
#include <sys/statvfs.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/fs/iso9660.h>
#include <sys/fs/cdfs_inode.h>
#include <sys/cdrom.h>

#endif /* _KERNEL_HEADERS */



/*
 * CDFS specific mount arguments passed to the VFS layer from
 * the CDFS specific 'mount(1M)' command.
 */
struct cdfs_mntargs {
	uint_t	mnt_Flags;						/* Flags - See below			*/
	uint_t	mnt_LogSecSz;					/* User-defined Log Sector Size	*/
};




/*
 * Miscellaneous definitions
 */
#define CDFS_ID		"cdfs"					/* CDFS file-system ID string	*/
#define	CDFS_MAX_LSEC_SZ	(16 * 1024)		/* Max Logical Sector Size		*/
#define	CDFS_MAX_NAME_LEN	32				/* Max File Name Length			*/
#define	CDFS_UNUSED_MAP_ENTRY	0xdead		/* Unused ID map entry			*/

#define	CDFS_MAX_YEARS	(68) 

#define CDFS_STRUCTOFF(str, fld) \
	((uint_t)(&(((struct str *)0)->fld)))




/*
 * CDFS file-system layouts handled by CDFS.
 */
enum cdfs_type {
	CDFS_ISO_9660,							/* ISO-9660 format 				*/
	CDFS_HIGH_SIERRA,						/* High-Sierra format			*/
 	CDFS_UNDEF_FS_TYPE						/* Undefined file system format	*/
};



/*
 * Pointed to by the VFS stucture of each CDFS instance.
 * Contains the CDFS-specific data (mainly from the PVD)
 * pertaining to that file-system instance.  Similar to
 * the "super-block" information.
 */
struct cdfs {
	uint_t			cdfs_Flags;				/* State flags for this FS		*/
	struct pathname cdfs_MntPnt;			/* Pathname of mount-point		*/
	struct pathname cdfs_DevNode;			/* Pathname of device node		*/
	struct vnode	*cdfs_DevVnode;			/* 'specfs' vnode for the device*/	
	struct cdfs_inode	*cdfs_RootInode;	/* Inode of CDFS root directory	*/
	struct cdfs_fid	cdfs_RootFid;			/* FID of Root Inode			*/
	enum cdfs_type	cdfs_Type;				/* File system type (9660/Hi-S)	*/
	daddr_t			cdfs_PvdLoc;			/* PVD location (Log Sector #)	*/
	uint_t			cdfs_LogSecSz;			/* Logical sector size (Bytes)	*/
	uint_t			cdfs_LogSecMask;		/* Convert bytes to beg of Sect	*/
	uint_t			cdfs_LogSecShift;		/* Convert bytes to Log Sect Num*/
	uint_t			cdfs_LogBlkMask;		/* Convert bytes to beg of Blk	*/
	uint_t			cdfs_LogBlkShift;		/* Convert bytes to Log Blk Num	*/

	/*
	 * Relevant PVD Information
	 */
	uint_t			cdfs_LogBlkSz;			/* Logical block size (Bytes)	*/
	uint_t			cdfs_VolVer;			/* Version # of Vol Descr struct*/
	uint_t			cdfs_FileVer;			/* Version # of Dir Rec/Path Tbl */
	uint_t			cdfs_VolSetSz;			/* Volume Set size (# of discs)	*/
	uint_t	 		cdfs_VolSeqNum;			/* Volume Sequence # (Disc #)	*/
	uint_t			cdfs_VolSpaceSz;		/* Volume Space	Size (Log Blocks)*/
	uint_t			cdfs_PathTabSz; 		/* Path Table size (Bytes)		*/
	daddr_t			cdfs_PathTabLoc;		/* Path Table loc. (Log Block #)*/
	timestruc_t		cdfs_CreateDate;		/* Volume Creation date/time	*/
	timestruc_t		cdfs_ModDate;			/* Volume Modification date/time*/
	timestruc_t		cdfs_ExpireDate;		/* Volume Expiration date/time	*/
	timestruc_t		cdfs_EffectDate;		/* Volume Effective date/time	*/
	uchar_t			cdfs_VolID[32];			/* Volume ID string				*/
	uint_t			cdfs_RootDirOff;		/* PVD offset of Root Dir Rec	*/
	uint_t			cdfs_RootDirSz;			/* Size (bytes) of Root Dir Rec */

	/*
	 * XCDR specific fields.
	 */
	struct cd_defs		cdfs_Dflts;			/* Default IDs, perms and modes	*/
	uint_t				cdfs_NameConv;		/* XCDR name conversion mode	*/
	uint_t				cdfs_UidMapCnt;		/* Num of valid UID mappings	*/
	uint_t				cdfs_GidMapCnt;		/* Num of valid GID mappings	*/
	struct cd_uidmap	cdfs_UidMap[CD_MAXUMAP];/* User ID map array		*/
	struct cd_gidmap	cdfs_GidMap[CD_MAXGMAP];/* Group ID map array		*/

	/*
	 * SUSP specific fields.
	 */
	uint_t			cdfs_SuspSkip;			/* Value for finding SUFs in SUA*/

	/*
	 * RRIP specific field(s).
	 */
	uint_t				cdfs_DevMapCnt;		/* Num of valid Device mappings	*/
	struct cd_devmap cdfs_DevMap[CD_MAXDMAP];/* Device Node (Number) Map	*/
};



/*
 * CDFS Flags bit definitions.
 */
#define CDFS_ALL_FLAGS		0x00000733		/* Bit-mask for all valid flags	*/
#define	CDFS_USER_BLKSZ		0x00000001		/* User specified Log Block size*/
#define	CDFS_BUILDING_ROOT	0x00000002		/* Currently building Root Inode*/
#define	CDFS_SUSP			0x00000010		/* User wants SUSP extensions	*/
#define	CDFS_SUSP_PRESENT	0x00000020		/* SUSP extensions were found	*/
#define	CDFS_RRIP			0x00000100		/* User wants RRIP extensions	*/
#define	CDFS_RRIP_PRESENT	0x00000200		/* RRIP extensions were found	*/
#define CDFS_RRIP_ACTIVE	0x00000400		/* RRIP/SUSP ext. will be parsed*/



/*
 * Define some useful macos for the CDFS structure.
 */
#define	CDFS_DEV(vfs)	(vfs->vfs_dev)
#define	CDFS_BLKSZ(vfs)	(vfs->vfs_bsize)
#define	CDFS_FS(vfs)	((struct cdfs *)(vfs->vfs_data))

#define	CDFS_TYPE(vfs)		(CDFS_FS(vfs)->cdfs_Type)
#define	CDFS_FLAGS(vfs)		(CDFS_FS(vfs)->cdfs_Flags)
#define	CDFS_BLKSHFT(vfs)	(CDFS_FS(vfs)->cdfs_LogBlkShift)
#define	CDFS_BLKMASK(vfs)	(CDFS_FS(vfs)->cdfs_LogBlkMask)
#define	CDFS_SECTSZ(vfs)	(CDFS_FS(vfs)->cdfs_LogSecSz)
#define	CDFS_SECTSHFT(vfs)	(CDFS_FS(vfs)->cdfs_LogSecShift)
#define	CDFS_SECTMASK(vfs)	(CDFS_FS(vfs)->cdfs_LogSecMask)

#define CDFS_ROOTFID(vfs)	 (CDFS_FS(vfs)->cdfs_RootFid)
#define CDFS_ROOT_INODE(vfs) (CDFS_FS(vfs)->cdfs_RootInode)
#define	CDFS_ROOT(vfs)		(ITOV(CDFS_ROOT_INODE(vfs)))
#define	CDFS_SUSPOFF(vfs)	(CDFS_FS(vfs)->cdfs_SuspSkip)

#define CDFS_BLKSECT_SHFT(vfs)	(CDFS_SECTSHFT(vfs) - CDFS_BLKSHFT(vfs))




/*
 * Define the "Put" and "Remove" operations for a generic
 * circular double-linked list.
 */

/*
 * Put an 'item' onto the circular doublely-linked list as specified
 * by the 'head' of the list.  The field identifiers 'next' and 'prev'
 * are the two link fields of the structure.
 *
 * - If the List is empty, adding the Item is trivial.
 * - Otherwise, add the item to the tail.
 *
 * Note: By default the Inode is placed at the tail of the free list.
 * Since the free list is a circular doublely-linked list, the
 * caller can have the Inode placed at the head, reassigning the
 * head-pointer to the newly freed Inode, after it has been added.
 *
 * Note: 'next' and 'prev' are structure field identifiers and
 * can not be enclosed by ().
 */
#define CDFS_LIST_PUT(head, item, next, prev) \
	if ((*(head)) == NULL) { \
		(*(head)) = (item); \
		(item)->next = (item); \
		(item)->prev = (item); \
	} else { \
		(item)->next = (*(head)); \
		(item)->prev = (*(head))->prev; \
		((*(head))->prev)->next = (item); \
		(*(head))->prev = (item); \
	}




/*
 * Remove an 'item' from the circular doublely-linked list as specified
 * by the 'head' of the list.  The field identifiers 'next' and 'prev'
 * are the two link fields of the structure.
 *
 * - If the "head" item is being removed, then the Head pointer
 *	 must be adjusted.  If this is the last item on the list, the
 *	 Head is set to NULL.  Otherwise, its adjusted to the next item.
 * - The item is remove by adjusting the 'next' and 'prev' pointers
 *	 of the "previous" and "next" item, respectively.  This algorithm
 *	 also works with the special cases described above.
 *
 * Note: 'next' and 'prev' are structure field identifiers and
 * can not be enclosed by ().
 */
#define CDFS_LIST_RM(head, item, next, prev) \
	if ((*(head)) == (item)) { \
		if ((item)->next == (item)) { \
			(*(head)) = NULL; \
		} else { \
			(*(head)) = (item)->next; \
		} \
	} \
	((item)->prev)->next = (item)->next; \
	((item)->next)->prev = (item)->prev; \
	(item)->next = NULL; \
	(item)->prev = NULL

/*
 * Convert the given character to lower case in US-ASCII, only
 * if it's upper case.
 * The result is the converted character.
 */
#define CDFS_TOLOWER(c) \
	((((c) >= 65) && ((c) < 91)) ? ((c) + 32) : (c))



/*
 * These unions define the various forms of each data structure
 * that can be read from the media.
 */
union media_adt {							/* ASCII-base date/time 		*/
	struct Pure9660_adt	Iso;
	struct HiSierra_adt	Hs;
};

union media_hdt {							/* Hex-based date/time			*/ 
	struct Pure9660_hdt		Iso;
	struct HiSierra_hdt		Hs;
};

union media_vd {							/* General Volume Descriptor	*/
	struct Pure9660_vd		Iso;
	struct HiSierra_vd		Hs;
};

union media_boot {							/* Boot-Record Volume Descriptor*/
	struct Pure9660_boot	Iso;
	struct HiSierra_boot	Hs;
};

union media_term {							/* Terminating Volume Descriptor*/
	struct Pure9660_term	Iso;
	struct HiSierra_term	Hs;
};

union media_part {							/* Volume Partition Descriptor	*/
	struct Pure9660_part	Iso;
	struct HiSierra_part	Hs;
};

union media_pvd {							/* Primary Volume Descriptor	*/
	struct Pure9660_pvd		Iso;
	struct HiSierra_pvd		Hs;
};

union media_svd {							/* Secondary Volume Descriptor	*/
	struct Pure9660_svd		Iso;
	struct HiSierra_svd		Hs;
};

union media_drec {							/* Directory Record				*/
	struct Pure9660_drec	Iso;
	struct HiSierra_drec	Hs;
};

union media_xar {							/* Extended Attribute Record	*/
	struct Pure9660_xar		Iso;
	struct HiSierra_xar		Hs;
};

union media_ptrec {							/* Path Table Record			*/
	struct Pure9660_ptrec	Iso;
	struct HiSierra_ptrec	Hs;
};




/*
 * Function prototypes
 */
#ifdef __STDC__

#ifdef _KERNEL

void	cdfsinit (struct vfssw *);
int		cdfs_mount (struct vfs *, struct vnode *, struct mounta *,
			struct cred *);
int		cdfs_mountroot (struct vfs *, enum whymountroot);
int		cdfs_mountfs (struct vfs *, dev_t, struct cdfs_mntargs *,
			struct cred *);
int		cdfs_unmount (struct vfs *, struct cred *);
int		cdfs_unmountfs (struct vfs *, struct cred *);
int		cdfs_root (struct vfs *, struct vnode **);
int		cdfs_statvfs (struct vfs *, struct statvfs *);
int		cdfs_sync (struct vfs *, int, struct cred *);
int		cdfs_vget (struct vfs *, struct vnode **, struct fid *);
int		cdfs_swapvp (struct vfs *, struct vnode **, char *);
int		cdfs_ioctl (struct vnode *, int, char *, int, struct cred *, int *);

#else

/* Prototypes for API library functions */
int cd_pvd (char *, struct iso9660_pvd *);
int cd_cpvd (char *, char *);
int cd_xar (char *, int, struct iso9660_xar *, int, int);
int cd_cxar (char *, int, char *, int);
int cd_drec (char *, int, struct iso9660_drec *);
int cd_cdrec (char *, int, char *);
int cd_ptrec (char *, struct iso9660_ptrec *);
int cd_cptrec (char *, char *);
int cd_type (char *);
int cd_defs (char *, int, struct cd_defs *);
int cd_idmap (char *, int, struct cd_idmap *, int *);
int cd_nmconv (char *, int, int *);
int cd_suf (char *, int, char [2], int, char *, int);
int cd_setdevmap (char *, int, int *, int *);
int cd_getdevmap (char *, int, int, int *, int *);

/* Prototypes for non-API library functions */
int cdfs_GetSectSize (int, uint_t *);
int cdfs_ReadPvd (int, char *, uint_t, uint_t *, enum cdfs_type *);
void cdfs_DispPvd (char *, enum cdfs_type);
time_t cd_hs_adtToDate (struct HiSierra_adt *);
time_t cd_iso_adtToDate (struct Pure9660_adt *);
time_t cd_hs_hdtToDate (struct HiSierra_hdt *);
time_t cd_iso_hdtToDate (struct Pure9660_hdt *);
time_t cd_ConvertDate (ushort_t, ushort_t, ushort_t, ushort_t, ushort_t,
			ushort_t, short);
uint_t cd_NumLeapYears (uint_t);

#endif

#else	/* __STDC__ */

#ifdef _KERNEL

void	cdfsinit();
int		cdfs_mount();
int		cdfs_mountroot();
int		cdfs_mountfs();
int		cdfs_unmount();
int		cdfs_unmountfs();
int		cdfs_root();
int		cdfs_statvfs();
int		cdfs_sync();
int		cdfs_vget();
int		cdfs_swapvp();
int		cdfs_ioctl();

#else

/* Prototypes for API library functions */
int cd_pvd();
int cd_cpvd();
int cd_xar();
int cd_cxar();
int cd_drec();
int cd_cdrec();
int cd_ptrec();
int cd_cptrec();
int cd_type();
int cd_defs();
int cd_idmap();
int cd_nmconv();
int cd_suf();
int cd_setdevmap();
int cd_getdevmap();

/* Prototypes for non-API library functions */
int cdfs_GetSectSize ();
int cdfs_ReadPvd ();
void cdfs_DispPvd ();
time_t cd_hs_adtToDate ();
time_t cd_iso_adtToDate ();
time_t cd_hs_hdtToDate ();
time_t cd_iso_hdtToDate ();
time_t cd_ConvertDate ();
uint_t cd_NumLeapYears ();

#endif

#endif	/* __STDC__ */

#endif	/* _FS_CDFS_CDFS_FS_H */
