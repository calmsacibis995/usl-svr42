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

#ifndef _FS_CDFS_CDFS_H
#define _FS_CDFS_CDFS_H

#ident	"@(#)uts-comm:fs/cdfs/cdfs.h	1.8"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _FS_CDFS_CDFS_FS_H
#include <fs/cdfs/cdfs_fs.h>	/* REQUIRED */
#endif

#ifndef _FS_CDFS_CDFS_INODE_H
#include <fs/cdfs/cdfs_inode.h>	/* REQUIRED */
#endif

#ifndef _FS_CDFS_CDROM_H
#include <fs/cdfs/cdrom.h>	/* REQUIRED */
#endif

#ifndef _FS_CDFS_ISO9660_H
#include <fs/cdfs/iso9660.h>	/* REQUIRED */
#endif

#ifndef _FS_BUF_H
#include <fs/buf.h>		/* REQUIRED */
#endif

#ifndef _FS_FBUF_H
#include <fs/fbuf.h>		/* REQUIRED */
#endif

#ifndef _MEM_KMEM_H
#include <mem/kmem.h>		/* REQUIRED */
#endif

#ifndef _MEM_SEG_H
#include <mem/seg.h>		/* REQUIRED */
#endif

#ifndef _UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/fs/cdfs_fs.h>	/* REQUIRED */
#include <sys/fs/cdfs_inode.h>	/* REQUIRED */
#include <sys/cdrom.h>		/* REQUIRED */
#include <sys/fs/iso9660.h>	/* REQUIRED */
#include <sys/buf.h>		/* REQUIRED */
#include <sys/fbuf.h>		/* REQUIRED */
#include <sys/kmem.h>		/* REQUIRED */
#include <vm/seg.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#else

#include <sys/fs/cdfs_fs.h>
#include <sys/fs/cdfs_inode.h>
#include <sys/cdrom.h>
#include <sys/debug.h>
#include <sys/fs/iso9660.h>
#include <sys/buf.h>
#include <sys/fbuf.h>
#include <sys/kmem.h>
#include <vm/seg.h>

#endif /* _KERNEL_HEADERS */



/*
 * CDFS Global Kernel Constants
 */
extern const uchar_t			CDFS_ISO_STD_ID[];
extern const uchar_t			CDFS_HS_STD_ID[];
extern const uchar_t			CDFS_DOT[];
extern const uchar_t			CDFS_DOTDOT[];
extern const uchar_t			CDFS_POSIX_DOT[];
extern const uchar_t			CDFS_POSIX_DOTDOT[];
extern const struct cdfs_fid	CDFS_NULLFID;



/*
 * CDFS Global Kernel Variables.
 *
 * XXX - 'rootvp' appears only to be used by the VM code to handle
 * the case where UNIX is running off the mini-root.  This should
 * implemented differently.
 */
extern struct vnode			*rootvp;

extern struct vfsops		cdfs_vfsops;
extern struct vnodeops		cdfs_vnodeops;

extern caddr_t				cdfs_Mem;
extern ulong_t				cdfs_MemSz;
extern uint_t				cdfs_MemCnt;

extern caddr_t				cdfs_TmpBufPool;
extern uint_t				cdfs_TmpBufSz;

extern uint_t				cdfs_InodeCnt;
extern struct cdfs_inode	*cdfs_InodeCache;
extern struct cdfs_inode	*cdfs_InodeFree;

extern uint_t				cdfs_IhashCnt;
extern struct cdfs_inode	**cdfs_InodeHash;	/* Addr of Inode pntr array	*/

extern uint_t				cdfs_DrecCnt;
extern struct cdfs_drec		*cdfs_DrecCache;
extern struct cdfs_drec		*cdfs_DrecFree;

extern uid_t				cdfs_InitialUID;
extern gid_t				cdfs_InitialGID;
extern mode_t				cdfs_InitialFPerm;
extern mode_t				cdfs_InitialDPerm;
extern int					cdfs_InitialDirSearch;
extern uint_t				cdfs_InitialNmConv;


/*
 * Return code values for internal CDFS routines.
 * Note: Return codes > 0 are defined in errno.h
 */
#define RET_OK			 0					/* Success						*/
#define RET_ERR			-1					/* Internal error detected		*/
#define RET_EOF			-2					/* End-of-file was reached		*/
#define RET_TRUE		-3					/* Condition is TRUE			*/
#define RET_FALSE		-4					/* Condition is FALSE			*/  
#define RET_NOT_FOUND	-5					/* Item not found				*/
#define RET_SLEEP		-6					/* Process may have slept		*/



/*
 * "Buffer descriptors" that describe the information pertaining
 * to a temporary buffer.  These data structures, which are passed
 * between related routines, allow these routines to share and modify
 * the contents of a single buffer.
 *
 * These type of buffer interaction are necessary because a CDFS
 * file system stores the "Inode" information directly within the
 * directory itself.  In other file systems (e.g. s5 and ufs) the
 * directory contains an Inode ID # which is used to located the
 * Inode data in a separate area of the media.
 *
 * With a CDFS file system, all of the Inode information (except for
 * the XAR) are stored within the directory itself.  Therefore, once
 * an directory entry is located, the buffer that contains the contents
 * of a directory is used to collect the Inode information for that entry.
 */
enum cdfs_iotype {
	CDFS_FBUFIO,							/* 'struct fbuf' type I/O	*/
	CDFS_BUFIO								/* 'struct buf' type I/O	*/
};

struct cdfs_iobuf {
	enum cdfs_iotype	sb_type;	 		/* I/O Buffer type				*/
	daddr_t				sb_sect;			/* Sector # of buffer contents	*/
	ulong_t				sb_sectoff;			/* File offset of sector 		*/
	uchar_t				*sb_start;			/* Kernel addr of buf contents	*/
	uchar_t				*sb_end;			/* Ending kernel addr			*/
	uchar_t				*sb_ptr;			/* Buffer addr of roving pointer*/
	ulong_t				sb_offset;			/* File/Sect offset of roving pntr*/
	ulong_t				sb_reclen;			/* Length of current "object"	*/

	/*
	 * Fields used for implementing the temporary buffer scheme
	 * to transparently handle records that cross sector boundries.
	 */
	uchar_t				*sb_tmpbuf;			/* Temp buffer					*/
	uchar_t				*sb_split;			/* Temp buf addr of sect boundry*/
	daddr_t				sb_nextsect;		/* Sector # of next sector		*/
	ulong_t				sb_nextsectoff;		/* File offset of next sector 	*/
	uchar_t				*sb_nextstart;		/* Addr of next sector contents	*/
	uchar_t				*sb_nextend;		/* Ending addr of sect contents */
	uchar_t				*sb_nextptr;		/* Sect buf pntr of new data	*/
	ulong_t				sb_nextoffset;		/* File/Sect offset of roving pntr*/

	/*
	 * CDFS_FBUFIO Type: Used for 'fbread()' type I/O.
	 */
	struct vnode	*sb_vp;					/* Vnode of file/dir			*/
	struct fbuf		*sb_fbp;				/* I/O structure				*/

	/*
	 * CDFS_BUFIO Type: Used for 'bread()' type I/O.
	 */
	dev_t			sb_dev;					/* Device # of CDFS media		*/
	struct buf		*sb_bp;					/* Buffer I/O structure			*/
};

/*						
 * Perhaps a 'struct clear' call would be faster.
 */
#define	CDFS_SETUP_IOBUF(tbuf, type) \
	(tbuf)->sb_type = (type);	\
	(tbuf)->sb_sect = 0;		\
	(tbuf)->sb_sectoff = 0;		\
	(tbuf)->sb_start = NULL;	\
	(tbuf)->sb_end = NULL;		\
	(tbuf)->sb_ptr = NULL;		\
	(tbuf)->sb_offset = 0;		\
	(tbuf)->sb_reclen = 0;		\
								\
	(tbuf)->sb_tmpbuf = NULL;	\
	(tbuf)->sb_split = NULL;	\
	(tbuf)->sb_nextsect = 0;	\
	(tbuf)->sb_nextsectoff = 0;	\
	(tbuf)->sb_nextstart = NULL;\
	(tbuf)->sb_nextend = NULL;	\
	(tbuf)->sb_nextptr = NULL;	\
	(tbuf)->sb_nextoffset = 0;	\
								\
	(tbuf)->sb_vp = NULL;		\
	(tbuf)->sb_fbp = NULL;		\
	(tbuf)->sb_dev = NODEV;		\
	(tbuf)->sb_bp = NULL
	
#define CDFS_RELEASE_IOBUF(tbuf)	\
	if ((tbuf)->sb_bp != NULL) {	\
		brelse((tbuf)->sb_bp);		\
	}								\
	if ((tbuf)->sb_fbp != NULL) {	\
		fbrelse((tbuf)->sb_fbp, S_OTHER);	\
	}								\
	if ((tbuf)->sb_tmpbuf != NULL) {	\
		kmem_fast_free(&cdfs_TmpBufPool,(caddr_t)(tbuf)->sb_tmpbuf);	\
	}
	


/*
 * Setup the debug support structure.
 */
#ifndef CDFS_DEBUG

#define DB_CODE(x,y)

#else

#define	DB_CODE(x,y)	if (((x) & cdfs_dbflags) != 0) y

#define DB_NONE				0x00000000
#define DB_ENTER			0x00000001
#define DB_EXIT				0x00000002
#define DB_ALL				0xFFFFFFFF

extern STATIC u_int		cdfs_dbflags;		/* Flags to select desired DEBUG*/

extern void	(*cdfs_DebugPtr)();				/* CDFS Entry Point Debug Routine*/
extern void	cdfs_entry();					/* CDFS Entry Point debug stub	*/
#endif



/*
 * Prototypes for kernel functions that may not be externally referenced.
 */
#ifdef __STDC__
int			cdfs_GetInode (struct vfs *, struct cdfs_fid *,
				struct cdfs_iobuf *, struct cdfs_inode **);
int			cdfs_FindInode (struct vfs *, struct cdfs_fid *,
				struct cdfs_inode **);
int			cdfs_AllocInode (struct cdfs_inode **);
int			cdfs_BldInode (struct vfs *, struct cdfs_iobuf *,
				struct cdfs_inode *);
int			cdfs_AllocDrec (struct cdfs_drec **);
void		cdfs_CleanInode (struct cdfs_inode *);
int			cdfs_InitInode (struct cdfs_inode *);
int			cdfs_GetParent (struct vfs *, struct cdfs_inode *,
				struct cdfs_inode **, struct cred *);

int			cdfs_MergeDrec (struct cdfs_inode *, struct cdfs_drec *);
int			cdfs_GetXar (struct vfs *, struct cdfs_drec *, struct cdfs_xar *);
int			cdfs_MergeXar (struct cdfs_inode *, struct cdfs_xar *);

void		cdfs_iput (struct cdfs_inode *);
void		irele (struct cdfs_inode *);
int			cdfs_syncip (struct cdfs_inode *, int);
void		cdfs_idrop (struct cdfs_inode *);
void		cdfs_iinactive (struct cdfs_inode *);
void		cdfs_iupdat (struct cdfs_inode *, int);
int			cdfs_FlushInodes (struct vfs *);
int			cdfs_iaccess (struct vfs *, struct cdfs_inode *,
				mode_t, struct cred *);

void		cdfs_LockInode (struct cdfs_inode *);
void 		cdfs_UnlockInode (struct cdfs_inode *);
void		cdfs_IputFree(struct cdfs_inode *);
void		cdfs_IrmFree(struct cdfs_inode *);
void		cdfs_IputHash(struct cdfs_inode *);
void		cdfs_IrmHash(struct cdfs_inode *);
void		cdfs_DrecPut(struct cdfs_drec **, struct cdfs_drec *);
void		cdfs_DrecRm(struct cdfs_drec **, struct cdfs_drec *);

/*
 * CDFS Directory routines.
 */
int			cdfs_DirLookup (struct vfs *, struct cdfs_inode *, uchar_t *,
				struct cdfs_inode **, struct cred *);
int			cdfs_ReadDrec (struct vfs *, struct cdfs_iobuf *);
int			cdfs_CmpDrecName (struct vfs *, struct cdfs_iobuf *,
				uchar_t *, struct pathname *);
int			cdfs_HiddenDrec (struct vfs *, struct cdfs_iobuf *);
int			cdfs_SectNum (struct vfs *, struct cdfs_inode *, ulong_t,
				daddr_t *, uint_t *, uint_t *);
int			cdfs_GetIsoName (struct vfs *, struct cdfs_iobuf *,
				struct pathname *);

int			cdfs_bmap (struct vfs *, struct cdfs_inode *, ulong_t,
				daddr_t *, uint_t *, uint_t *);
int			blkatoff (struct cdfs_inode *, off_t, char **, struct fbuf **);
int			cdfs_dirempty (struct cdfs_inode *, ino_t, int *);
int			cdfs_dircheckpath (struct cdfs_inode *, struct cdfs_inode *);

/*
 * CDFS Support Routines:
 */
int			cdfs_getapage (struct vnode *, uint_t, uint_t *, struct page *[],
				uint_t, struct seg *, addr_t, enum seg_rw, struct cred *);
int			cdfs_GetSectSize (dev_t, uint_t *);
int			cdfs_ReadSect (struct vfs *, struct cdfs_iobuf *);
int			cdfs_FillBuf (struct vfs *, struct cdfs_iobuf *);
int			cdfs_ReadPvd (struct vfs *, struct cdfs_iobuf *, enum cdfs_type *);
int			cdfs_ConvertPvd (struct cdfs *, union media_pvd *, enum cdfs_type);
int			cdfs_ConvertDrec (struct cdfs_drec *, union media_drec *,
				enum cdfs_type);
int			cdfs_ConvertXar (struct cdfs_xar *, union media_xar *,
				enum cdfs_type);
int			cdfs_ConvertAdt (union media_adt *, timestruc_t *, enum cdfs_type);
int			cdfs_ConvertHdt (union media_hdt *, timestruc_t *, enum cdfs_type);
time_t		cdfs_ConvertDate (uint_t, uint_t, uint_t, uint_t, uint_t,
				uint_t, int);
int			cdfs_BldRootVnode (struct vfs *, struct cdfs_fid *,
				struct cdfs_iobuf *, struct vnode **, struct cred *);
struct vnode *cdfs_FindVnode (struct vfs *, struct cdfs_fid *);
int			cdfs_FlushVnodes (struct vfs *);
int			cdfs_atoi (uchar_t *, uint_t);
void		cdfs_pn_clear(struct pathname *);
int			cdfs_pn_set(struct pathname *, uchar_t *, uint_t);
int			cdfs_pn_append(struct pathname *, uchar_t *, uint_t);
int			cdfs_XcdrName (struct vfs *, uchar_t *, uint_t, struct pathname *);
dev_t		cdfs_GetDevNum(struct vfs *, struct cdfs_inode *);
uid_t		cdfs_GetUid(struct vfs *, struct cdfs_inode *);
gid_t		cdfs_GetGid(struct vfs *, struct cdfs_inode *);
mode_t		cdfs_GetPerms(struct vfs *, struct cdfs_inode *);

/*
 * RRIP procedures.
 */
int			cdfs_GetRripName (struct vfs *, struct cdfs_iobuf *,
				struct pathname *);
int			cdfs_LocSusp (struct vfs *, struct cdfs_iobuf *,
				struct susp_suf **, uint_t *);
int			cdfs_ReadSUF (struct vfs *, struct cdfs_iobuf *, uint_t, uint_t,
				struct susp_ce *);
int			cdfs_HiddenRrip (struct vfs *, struct cdfs_iobuf *);
int			cdfs_GetRrip (struct vfs *, struct cdfs_iobuf *,
				struct cdfs_rrip *);
int			cdfs_MergeRrip (struct vfs *, struct cdfs_inode *,
				struct cdfs_rrip *);
int			cdfs_AppendRripSL (struct vfs *, struct pathname *,
				struct rrip_sl *, boolean_t *);
int			cdfs_AppendRripNM (struct pathname *, struct rrip_nm *,
				boolean_t *);

/*
 * cdfs ioctl support routines.
 */
STATIC int	cdfs_GetIocArgs (const caddr_t, struct pathname *, uchar_t **,
				caddr_t *, struct vnode **, struct cred *);
STATIC int	cdfs_IocGetVnode (const struct pathname *, struct vnode **,
				struct cred *);
STATIC int	cdfs_ioc_Cleanup (struct vnode *, struct pathname *);
STATIC int	cdfs_ioc_GetXAR (const struct vnode *, caddr_t);
STATIC struct cdfs_drec *cdfs_FindDRec (const struct vnode *, const int);
STATIC int	cdfs_GetAndCopyXar (const struct vnode *,
				const struct cdfs_drec *, int, int, int,
				caddr_t);
STATIC int	cdfs_ioc_GetDREC (const struct vnode *, caddr_t);
STATIC int	cdfs_RetrieveDRec (const struct vnode *, const struct cdfs_drec *,
				struct cdfs_iobuf *);
STATIC int	cdfs_ioc_GetPTREC (const struct vnode *, caddr_t);
STATIC int	cdfs_GetNextPTRec (struct vfs *, struct cdfs_iobuf *, int *);
STATIC int	cdfs_CheckPTRec (const struct vnode *, const union media_ptrec *,
				const enum cdfs_type);
STATIC int	cdfs_ioc_GetSUF (const struct vnode *, caddr_t);
STATIC int	cdfs_ioc_GetType (const struct vnode *, caddr_t);
STATIC int	cdfs_ioc_DoDefs (const struct vnode *, caddr_t, struct cred *);
STATIC int	cdfs_ioc_DoIDMap (const struct vnode *, caddr_t, struct cred *);
STATIC int	cdfs_ioc_DoNmConv (const struct vnode *, caddr_t, struct cred *);
STATIC int	cdfs_ioc_SetDevMap (const struct vnode *, const uchar_t *,
				caddr_t, struct cred *);
STATIC int	cdfs_ioc_GetDevMap (const struct vnode *, uchar_t *, caddr_t);

#else /* __STDC__ */

void		cdfs_VnodeInit();
int			cdfs_GetInode();
int			cdfs_FindInode();
int			cdfs_AllocInode();
int			cdfs_BldInode();
int			cdfs_AllocDrec();
void		cdfs_CleanInode();
int			cdfs_InitInode();
int			cdfs_GetParent();

int			cdfs_MergeDrec();
int			cdfs_GetXar();
int			cdfs_MergeXar();

void		cdfs_iput();
void		irele();
int			cdfs_syncip();
void		cdfs_idrop();
void		cdfs_iinactive();
void		cdfs_iupdat();
int			cdfs_FlushInodes();
int			cdfs_iaccess();

void		cdfs_LockInode();
void 		cdfs_UnlockInode();
void		cdfs_IputFree();
void		cdfs_IrmFree();
void		cdfs_IputHash();
void		cdfs_IrmHash();
void		cdfs_DrecPut();
void		cdfs_DrecRm();

/*
 * CDFS Directory routines.
 */
int			cdfs_DirLookup();
int			cdfs_ReadDrec();
int			cdfs_CmpDrecName();
int			cdfs_HiddenDrec();
int			cdfs_SectNum();
int			cdfs_GetIsoName();

int			cdfs_bmap();
int			blkatoff();
int			cdfs_dirempty();
int			cdfs_dircheckpath();

/*
 * CDFS Support Routines:
 */
int			cdfs_getapage();
int			cdfs_GetSectSize();
int			cdfs_ReadSect();
int			cdfs_FillBuf();
int			cdfs_ReadPvd();
int			cdfs_ConvertPvd();
int			cdfs_ConvertDrec();
int			cdfs_ConvertXar();
time_t		cdfs_ConvertAdt();
time_t		cdfs_ConvertHdt();
time_t		cdfs_ConvertDate();
int			cdfs_BldRootVnode();
struct vnode *cdfs_FindVnode();
int			cdfs_FlushVnodes();
int			cdfs_atoi();
void		cdfs_pn_clear();
int			cdfs_pn_set();
int			cdfs_pn_append();
int			cdfs_XcdrName();
dev_t		cdfs_GetDevNum();
uid_t		cdfs_GetUid();
gid_t		cdfs_GetGid();
mode_t		cdfs_GetPerms();

/*
 * RRIP procedures.
 */
int			cdfs_GetRripName();
int			cdfs_LocSusp();
int			cdfs_ReadSUF();
int			cdfs_HiddenRrip();
int			cdfs_GetRrip();
int			cdfs_MergeRrip();
int			cdfs_AppendRripSL();
int			cdfs_AppendRripNM();

/*
 * cdfs ioctl support routines.
 */
STATIC int	cdfs_GetIocArgs();
STATIC int	cdfs_IocGetVnode();
STATIC int	cdfs_ioc_Cleanup();
STATIC int	cdfs_ioc_GetXAR();
STATIC struct cdfs_drec *cdfs_FindDRec();
STATIC int	cdfs_GetAndCopyXar();
STATIC int	cdfs_ioc_GetDREC();
STATIC int	cdfs_RetrieveDRec();
STATIC int	cdfs_ioc_GetPTREC();
STATIC int	cdfs_GetNextPTRec();
STATIC int	cdfs_CheckPTRec();
STATIC int	cdfs_ioc_GetSUF();
STATIC int	cdfs_ioc_GetType();
STATIC int	cdfs_ioc_DoDefs();
STATIC int	cdfs_ioc_DoIDMap();
STATIC int	cdfs_ioc_DoNmConv();
STATIC int	cdfs_ioc_SetDevMap();
STATIC int	cdfs_ioc_GetDevMap();

#endif /* __STDC__ */

#endif	/* _FS_CDFS_CDFS_H */
