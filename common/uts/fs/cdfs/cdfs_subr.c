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

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ident	"@(#)uts-comm:fs/cdfs/cdfs_subr.c	1.7"
#ident	"$Header: $"

#include <fs/buf.h>
#include <fs/cdfs/cdfs_fs.h>
#include <fs/cdfs/cdfs_inode.h>
#include <fs/cdfs/cdfs_susp.h>
#include <fs/cdfs/cdrom.h>
#include <fs/cdfs/iso9660.h>
#include <fs/pathname.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <proc/proc.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#if ((defined CDFS_DEBUG)  && (!defined DEBUG))
#define		DEBUG	YES
#include	<util/debug.h>
#undef		DEBUG
#else
#include	<util/debug.h>
#endif
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include "cdfs.h"



/*
 * External references not defined by any kernel header file.
 */
extern time_t	c_correct;



uint_t	cdfs_DaysOfMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

enum	cdfs_MonthsOfYear {
	JAN=1, FEB=2, MAR=3, APR=4, MAY=5, JUN=6,
	JUL=7, AUG=8, SEP=9, OCT=10, NOV=11, DEC=12
};


/*
 * Get the Logical sector size for the device (media).
 */
int
cdfs_GetSectSize(Dev, SecSize)
dev_t	Dev;								/* Device # of file system media*/	
u_int	*SecSize;							/* Current Logical Sector Size	*/
{
	struct buf			*SecBufh;			/* Buf header with sector data	*/
	union media_vd		*VolD;	 			/* Volume Descriptor template	*/
	int					RetVal;				/* Return value of function		*/

	for (*SecSize=ISO_MIN_LSEC_SZ; *SecSize <= MAXBSIZE; *SecSize*=2) {
		/*
		 * Based on the current Logical Sector Size, read in the
		 * sector containing the 1st Volume Descriptor in the
		 * Volume Descriptor list.
		 */
		SecBufh = bread(Dev, ISO_VD_LOC, *SecSize);
		if ((SecBufh == NULL) ||
			((SecBufh->b_flags & B_ERROR) != 0)) {
			/*
			 * Error reading sector so try next Logical Sector Size.
			 */
			if (SecBufh != NULL) {
				brelse(SecBufh);
			}
			RetVal = EIO;
			continue;
		}

		/*
		 * If the current Logical Sector Size is currect,
		 * a valid ISO-9660 or High-Sierra ID string
		 * will exist at the appropriate offset within the buffer.
		 *
		 * XXX - Additional check should be added to verify that
		 * this is really a valid CDFS Volume Descriptor.
		 */
		VolD = (union media_vd *) SecBufh->b_un.b_addr;

		if ((strncmp((caddr_t)&VolD->Iso.vd_StdID[0], (caddr_t)CDFS_ISO_STD_ID,
				sizeof(VolD->Iso.vd_StdID)) == 0) &&
			(CDFS_ISO_STD_ID[sizeof(VolD->Iso.vd_StdID)] == '\0')) {
			brelse(SecBufh);
			RetVal = RET_OK;
			break;

		} else
		if ((strncmp((caddr_t)&VolD->Hs.vd_StdID[0], (caddr_t)CDFS_HS_STD_ID,
				sizeof(VolD->Hs.vd_StdID)) == 0) &&
			(CDFS_HS_STD_ID[sizeof(VolD->Hs.vd_StdID)] == '\0')) {
			brelse(SecBufh);
			RetVal = RET_OK;
			break;
		}

		/*
		 * Logical sector does not contain a known Vol Descr, so
		 * try the next Logical Sector Size.
		 */
		brelse(SecBufh);
		RetVal = RET_NOT_FOUND;
	}

	return(RetVal);
}




/*
 * Read PVD sector.
 * Note: This routine can easily be modified to read
 * any VD type requested by the caller.
 */ 
int
cdfs_ReadPvd(vfs, pvd_buf, fstype)
struct vfs			*vfs;
struct cdfs_iobuf	*pvd_buf;
enum cdfs_type		*fstype;
{
	union media_vd		*VolD;				/* Volume Descriptor template	*/
	int					retval;

	/*
	 * Make sure the caller has not messes up.
	 */
	if (pvd_buf->sb_type != CDFS_BUFIO) {
		cmn_err(CE_WARN, "cdfs_ReadPvd(): Invalid IOBUF type (0x%x).",
			pvd_buf->sb_type);
		cmn_err(CE_CONT, "Expecting a type 0x%x IOBUF.\n\n", CDFS_BUFIO);
		return(RET_ERR);
	}

	for (;;) {
		/*
		 * Read data from the next Logical Sector.
		 */
		if ((pvd_buf->sb_start == NULL) ||
			(pvd_buf->sb_ptr < pvd_buf->sb_start) ||
			(pvd_buf->sb_ptr + ISO_VD_LEN > pvd_buf->sb_end)) {
			/*
			 * If we're gotten here, and we're pointing to the
			 * beginning of a sector that has already been read in,
			 * then there must not be enough data for a Vol. Descr.
			 * Since this shouldn't happen, we return an error.
			 */
			if ((pvd_buf->sb_start != NULL) &&
				(pvd_buf->sb_ptr == pvd_buf->sb_start)) {
				cmn_err(CE_WARN,
					"cdfs_ReadPvd(): Invalid sector size:");
				cmn_err(CE_CONT,
					"Sector=%d, size=%d, Volume Descriptor size=%d\n\n",
					pvd_buf->sb_sect,
					pvd_buf->sb_end - pvd_buf->sb_start);
				return(EINVAL);
			}

			retval = cdfs_ReadSect(vfs, pvd_buf);
			if (retval != RET_OK) {
				return(retval);
			}

			/*
			 * Validate that the new buffer contains enough
			 * data to hold a Volume Descriptor.
			 * Note: Infinate loops are prevented by the above checks.
			 */
			continue;
		}

		/*
		 * See if this is an ISO-9660 Volume Descriptor by checking
		 * for the ISO_9660 Standard ID string.
		 */
		VolD = (union media_vd *) pvd_buf->sb_ptr;

		if ((strncmp((caddr_t)&VolD->Iso.vd_StdID[0], (caddr_t)CDFS_ISO_STD_ID,
				sizeof(VolD->Iso.vd_StdID)) == 0) &&
			(CDFS_ISO_STD_ID[sizeof(VolD->Iso.vd_StdID)] == '\0')) {
			
			*fstype = CDFS_ISO_9660;

			/*
			 * Determine Vol. Descr. type and responde accordingly.
			 * - If this is a PVD then we're done.
			 * - If this is a Terminator VD then we've failed.
			 */
			if (VolD->Iso.vd_Type == ISO_PVD_TYPE) {
				retval = RET_OK;
				break;

			} else if (VolD->Iso.vd_Type == ISO_TERM_TYPE) {
				retval = RET_NOT_FOUND;
				break;
			}
				
		/*
		 * Check for HIGH Sierra Vol Descr.
		 */
		} else
		if ((strncmp((caddr_t)&VolD->Hs.vd_StdID[0], (caddr_t)CDFS_HS_STD_ID,
				sizeof(VolD->Hs.vd_StdID)) == 0) &&
			(CDFS_HS_STD_ID[sizeof(VolD->Hs.vd_StdID)] == '\0')) {
				
			*fstype = CDFS_HIGH_SIERRA;

			/*
			 * Determine Vol. Descr. type and responde accordingly.
			 * - If this is a PVD then we're done.
			 * - If this is a Terminator VD then we've failed.
			 */
			if (VolD->Hs.vd_Type == HS_PVD_TYPE) {
				retval = RET_OK;
				break;

			} else if (VolD->Hs.vd_Type == HS_TERM_TYPE) {
				retval = RET_NOT_FOUND;
				break;
			}

		} else {
			/*
			 * Unrecognized Volume Descriptor format.
			 */
			retval = RET_NOT_FOUND;
			break;
		}

		/*
		 * This recognized VD is not a valid PVD or a Terminater VD,
		 * so skip it and continue with the next one.
		 */
		pvd_buf->sb_sect++;
		pvd_buf->sb_ptr = NULL;
		pvd_buf->sb_offset = 0;
	}
	return(retval);
}



/*
 * Convert a media-based PVD structure to the generic CDFS PVD structure.
 */
int
cdfs_ConvertPvd(Cdfs, Pvd, FsType)
struct cdfs		*Cdfs;
union media_pvd	*Pvd;
enum cdfs_type	FsType;
{
	switch (FsType) {
		case CDFS_ISO_9660: {
			Cdfs->cdfs_VolVer =		Pvd->Iso.pvd_Ver;
			Cdfs->cdfs_FileVer =	Pvd->Iso.pvd_FileVer;
			Cdfs->cdfs_VolSetSz =	Pvd->Iso.pvd_VolSetSz;
			Cdfs->cdfs_VolSeqNum =	Pvd->Iso.pvd_VolSeqNum;
			Cdfs->cdfs_VolSpaceSz =	Pvd->Iso.pvd_VolSpcSz;
			Cdfs->cdfs_LogBlkSz =	Pvd->Iso.pvd_LogBlkSz;
			Cdfs->cdfs_PathTabSz =	Pvd->Iso.pvd_PathTabSz;
			Cdfs->cdfs_PathTabLoc = Pvd->Iso.pvd_PathTabLoc;

			(void)cdfs_ConvertAdt(
				(union media_adt *)&Pvd->Iso.pvd_CreateDate,
				&Cdfs->cdfs_CreateDate, CDFS_ISO_9660);

			(void)cdfs_ConvertAdt(
				(union media_adt *)&Pvd->Iso.pvd_ModDate,
				&Cdfs->cdfs_ModDate, CDFS_ISO_9660);

			(void)cdfs_ConvertAdt(
				(union media_adt *)&Pvd->Iso.pvd_EffectDate,
				&Cdfs->cdfs_EffectDate, CDFS_ISO_9660);

			(void)cdfs_ConvertAdt(
				(union media_adt *)&Pvd->Iso.pvd_ExpireDate,
				&Cdfs->cdfs_ExpireDate, CDFS_ISO_9660);

			(void)strncpy((caddr_t)&Cdfs->cdfs_VolID[0],
				(caddr_t)&Pvd->Iso.pvd_VolID[0],
				sizeof(Cdfs->cdfs_VolID));

			Cdfs->cdfs_RootDirOff = CDFS_STRUCTOFF(Pure9660_pvd, pvd_RootDir);
			Cdfs->cdfs_RootDirSz = sizeof(Pvd->Iso.pvd_RootDir);
			break;
		}
		case CDFS_HIGH_SIERRA: {
			Cdfs->cdfs_VolVer =		Pvd->Hs.pvd_Ver;
			Cdfs->cdfs_FileVer =	Pvd->Hs.pvd_FileVer;
			Cdfs->cdfs_VolSetSz =	Pvd->Hs.pvd_VolSetSz;
			Cdfs->cdfs_VolSeqNum =	Pvd->Hs.pvd_VolSeqNum;
			Cdfs->cdfs_VolSpaceSz =	Pvd->Hs.pvd_VolSpcSz;
			Cdfs->cdfs_LogBlkSz =	Pvd->Hs.pvd_LogBlkSz;
			Cdfs->cdfs_PathTabSz =	Pvd->Hs.pvd_PathTabSz;
			Cdfs->cdfs_PathTabLoc = Pvd->Hs.pvd_PathTabLoc;

			(void)cdfs_ConvertAdt(
				(union media_adt *)&Pvd->Hs.pvd_CreateDate,
				&Cdfs->cdfs_CreateDate, CDFS_HIGH_SIERRA);

			(void)cdfs_ConvertAdt(
				(union media_adt *)&Pvd->Hs.pvd_ModDate,
				&Cdfs->cdfs_ModDate, CDFS_HIGH_SIERRA);

			(void)cdfs_ConvertAdt(
				(union media_adt *)&Pvd->Hs.pvd_EffectDate,
				&Cdfs->cdfs_EffectDate, CDFS_HIGH_SIERRA);

			(void)cdfs_ConvertAdt(
				(union media_adt *)&Pvd->Hs.pvd_ExpireDate,
				&Cdfs->cdfs_ExpireDate, CDFS_HIGH_SIERRA);

			(void)strncpy((caddr_t)&Cdfs->cdfs_VolID[0],
				(caddr_t)&Pvd->Hs.pvd_VolID[0],
				sizeof(Cdfs->cdfs_VolID));

			Cdfs->cdfs_RootDirOff = CDFS_STRUCTOFF(HiSierra_pvd, pvd_RootDir);
			Cdfs->cdfs_RootDirSz = sizeof(Pvd->Hs.pvd_RootDir);
			break;
		}
		default: {
			/*
			 * Invalid CDFS file-system type.
			 */
			cmn_err(CE_WARN,
				"cdfs_ConvertPvd(): Invalid CDFS FS-type (0x%x).\n");
			return(RET_ERR);
		}
	}
	return(RET_OK);
}



/*
 * Convert a media-based Directory Record structure to the generic
 * CDFS Directory Record structure.
 */
int
cdfs_ConvertDrec(CdfsDrec, MediaDrec, FsType)
struct cdfs_drec	*CdfsDrec;
union media_drec	*MediaDrec;
enum cdfs_type		FsType;
{
	/*
	 * Use the appropriate Directory Record template.
	 */
	switch (FsType) {
		case CDFS_ISO_9660: {
			CdfsDrec->drec_Len = MediaDrec->Iso.drec_Size;
			CdfsDrec->drec_XarLen = MediaDrec->Iso.drec_XarSize;
			CdfsDrec->drec_ExtLoc = MediaDrec->Iso.drec_ExtentLoc;
			CdfsDrec->drec_DataLen = MediaDrec->Iso.drec_DataSz;

			(void)cdfs_ConvertHdt(
				(union media_hdt *)&MediaDrec->Iso.drec_RecordDate,
				&CdfsDrec->drec_Date, CDFS_ISO_9660);

			CdfsDrec->drec_Flags = MediaDrec->Iso.drec_Flags;
			CdfsDrec->drec_UnitSz = MediaDrec->Iso.drec_UnitSz;
			CdfsDrec->drec_Interleave = MediaDrec->Iso.drec_Interleave;
			CdfsDrec->drec_VolSeqNum = MediaDrec->Iso.drec_VolSeqNum;
			CdfsDrec->drec_FileIDLen = MediaDrec->Iso.drec_FileIDSz;
			CdfsDrec->drec_FileIDOff =
				CDFS_STRUCTOFF(Pure9660_drec, drec_VarData);
			break;
		}
		case CDFS_HIGH_SIERRA: {
			CdfsDrec->drec_Len = MediaDrec->Hs.drec_Size;
			CdfsDrec->drec_XarLen = MediaDrec->Hs.drec_XarSize;
			CdfsDrec->drec_ExtLoc = MediaDrec->Iso.drec_ExtentLoc;
			CdfsDrec->drec_DataLen = MediaDrec->Hs.drec_DataSz;

			(void)cdfs_ConvertHdt(
				(union media_hdt *)&MediaDrec->Hs.drec_RecordDate,
				&CdfsDrec->drec_Date, CDFS_HIGH_SIERRA);

			CdfsDrec->drec_Flags = MediaDrec->Hs.drec_Flags;
			CdfsDrec->drec_UnitSz = MediaDrec->Hs.drec_InterleaveSz;
			CdfsDrec->drec_Interleave = MediaDrec->Hs.drec_InterleaveSkip;
			CdfsDrec->drec_VolSeqNum = MediaDrec->Hs.drec_VolSeqNum;
			CdfsDrec->drec_FileIDLen = MediaDrec->Hs.drec_FileIDSz;
			CdfsDrec->drec_FileIDOff =
				CDFS_STRUCTOFF(HiSierra_drec, drec_VarData);
			break;
		}
		default: {
			/*
			 * Invalid CDFS file-system type.
			 */
			cmn_err(CE_WARN,
				"cdfs_ConvertDrec(): Invalid CDFS FS-type (0x%x).\n");
			return(RET_ERR);
		}
	}

	/*
	 * Calc other useful values not dependant on File System Type.
	 * - Offset and size of System Use Area.
	 */
	CdfsDrec->drec_SysUseOff =
		CdfsDrec->drec_FileIDOff +
		CdfsDrec->drec_FileIDLen +
		(((CdfsDrec->drec_FileIDLen & 0x01) == 0) ? 1 : 0);

	if (CdfsDrec->drec_SysUseOff >= CdfsDrec->drec_Len) {
		CdfsDrec->drec_SysUseOff = 0;
		CdfsDrec->drec_SysUseSz = 0;
	} else {
		CdfsDrec->drec_SysUseSz =
			CdfsDrec->drec_Len - CdfsDrec->drec_SysUseOff;
	}

	return(RET_OK);
}




/*
 * Convert a media-based XAR structure to the generic CDFS XAR structure.
 */
int
cdfs_ConvertXar(CdfsXar, MediaXar, FsType)
struct cdfs_xar		*CdfsXar;
union media_xar		*MediaXar;
enum cdfs_type		FsType;
{
	/*
	 * Use the appropriate Directory Record template.
	 */
	switch (FsType) {
		case CDFS_ISO_9660: {
			CdfsXar->xar_UserID = MediaXar->Iso.xar_User;
			CdfsXar->xar_GroupID = MediaXar->Iso.xar_Group;
			CdfsXar->xar_Perms =
				(MediaXar->Iso.xar_Perms1 << 8) | MediaXar->Iso.xar_Perms2;

			(void)cdfs_ConvertAdt(
				(union media_adt *)&MediaXar->Iso.xar_CreateDate,
				&CdfsXar->xar_CreateDate, CDFS_ISO_9660);

			(void)cdfs_ConvertAdt(
				(union media_adt *)&MediaXar->Iso.xar_ModDate,
				&CdfsXar->xar_ModDate, CDFS_ISO_9660);

			(void)cdfs_ConvertAdt(
				(union media_adt *)&MediaXar->Iso.xar_EffectDate,
				&CdfsXar->xar_EffectDate, CDFS_ISO_9660);

			(void)cdfs_ConvertAdt(
				(union media_adt *)&MediaXar->Iso.xar_ExpireDate,
				&CdfsXar->xar_ExpireDate, CDFS_ISO_9660);

			CdfsXar->xar_RecFmt = MediaXar->Iso.xar_RecFmt;
			CdfsXar->xar_RecAttr = MediaXar->Iso.xar_RecAttr;
			CdfsXar->xar_RecLen = MediaXar->Iso.xar_RecLen;
			CdfsXar->xar_SysID = NULL;
			CdfsXar->xar_SysUse = NULL;
			CdfsXar->xar_EscSeqLen = MediaXar->Iso.xar_EscSeqLen;
			CdfsXar->xar_ApplUseLen = MediaXar->Iso.xar_ApplUseLen;
			CdfsXar->xar_ApplUse = NULL;
			break;
		}
		case CDFS_HIGH_SIERRA: {
			CdfsXar->xar_UserID = MediaXar->Hs.xar_User;
			CdfsXar->xar_GroupID = MediaXar->Hs.xar_Group;
			CdfsXar->xar_Perms =
				(MediaXar->Hs.xar_Perms1 << 8) | MediaXar->Hs.xar_Perms2;

			(void)cdfs_ConvertAdt(
				(union media_adt *)&MediaXar->Hs.xar_CreateDate,
				&CdfsXar->xar_CreateDate, CDFS_HIGH_SIERRA);

			(void)cdfs_ConvertAdt(
				(union media_adt *)&MediaXar->Hs.xar_ModDate,
				&CdfsXar->xar_ModDate, CDFS_HIGH_SIERRA);

			(void)cdfs_ConvertAdt(
				(union media_adt *)&MediaXar->Hs.xar_EffectDate,
				&CdfsXar->xar_EffectDate, CDFS_HIGH_SIERRA);

			(void)cdfs_ConvertAdt(
				(union media_adt *)&MediaXar->Hs.xar_ExpireDate,
				&CdfsXar->xar_ExpireDate, CDFS_HIGH_SIERRA);

			CdfsXar->xar_RecFmt = MediaXar->Hs.xar_RecFmt;
			CdfsXar->xar_RecAttr = MediaXar->Hs.xar_RecAttr;
			CdfsXar->xar_RecLen = MediaXar->Hs.xar_RecLen;
			CdfsXar->xar_SysID = NULL;
			CdfsXar->xar_SysUse = NULL;
			CdfsXar->xar_EscSeqLen = 0;
			CdfsXar->xar_ApplUseLen = MediaXar->Hs.xar_ApplUseLen;
			CdfsXar->xar_ApplUse = NULL;
			break;
		}
		default: {
			/*
			 * Invalid CDFS file-system type.
			 */
			cmn_err(CE_WARN,
				"cdfs_ConvertXar(): Invalid CDFS FS-type (0x%x).\n");
			return(RET_ERR);
		}
	}
	return(RET_OK);
}




/*
 * Convert a media-based "Long format" Time structure
 * to an single time value.
 */
int
cdfs_ConvertAdt(DatePtr, Time, FsType)
union media_adt	*DatePtr;
timestruc_t		*Time;
enum cdfs_type	FsType;
{
	Time->tv_sec = 0;
	Time->tv_nsec = 0;

	if (DatePtr == NULL) {
		return(RET_ERR);
	}

	/*
	 * Use the appropriate date template to get the various date fields.
	 */
	switch (FsType) {
		case CDFS_ISO_9660: {
			Time->tv_sec = cdfs_ConvertDate(
				cdfs_atoi(&DatePtr->Iso.adt_Year[0],
					sizeof(DatePtr->Iso.adt_Year)),

				cdfs_atoi(&DatePtr->Iso.adt_Month[0],
					sizeof(DatePtr->Iso.adt_Month)),

				cdfs_atoi(&DatePtr->Iso.adt_Day[0],
					sizeof(DatePtr->Iso.adt_Day)),

				cdfs_atoi(&DatePtr->Iso.adt_Hour[0],
					sizeof(DatePtr->Iso.adt_Hour)), 

				cdfs_atoi(&DatePtr->Iso.adt_Minute[0],
					sizeof(DatePtr->Iso.adt_Minute)), 

				cdfs_atoi(&DatePtr->Iso.adt_Second[0],
					sizeof(DatePtr->Iso.adt_Second)),

				(DatePtr->Iso.adt_GmtOffset * 15 * 60)
			); 

			Time->tv_nsec = cdfs_atoi(&DatePtr->Iso.adt_Hundredths[0],
					sizeof(DatePtr->Iso.adt_Hundredths)) * (10*1000*1000);
			break;
		}
		case CDFS_HIGH_SIERRA: {
			Time->tv_sec = cdfs_ConvertDate(
				cdfs_atoi(&DatePtr->Hs.adt_Year[0],
					sizeof(DatePtr->Hs.adt_Year)),

				cdfs_atoi(&DatePtr->Hs.adt_Month[0],
					sizeof(DatePtr->Hs.adt_Month)),

				cdfs_atoi(&DatePtr->Hs.adt_Day[0],
					sizeof(DatePtr->Hs.adt_Day)),

				cdfs_atoi(&DatePtr->Hs.adt_Hour[0],
					sizeof(DatePtr->Hs.adt_Hour)), 

				cdfs_atoi(&DatePtr->Hs.adt_Minute[0],
					sizeof(DatePtr->Hs.adt_Minute)), 

				cdfs_atoi(&DatePtr->Hs.adt_Second[0],
					sizeof(DatePtr->Hs.adt_Second)),

				(-1 * (c_correct))
			);

			Time->tv_nsec = cdfs_atoi(&DatePtr->Hs.adt_Hundredths[0],
					sizeof(DatePtr->Hs.adt_Hundredths)) * (10*1000*1000);

			break;
		}

		default: {
			/*
			 * Invalid CDFS file-system type.
			 */
			cmn_err(CE_WARN,
				"cdfs_ConvertAdt(): Invalid CDFS FS-type (0x%x).\n");
			return(RET_ERR);
		}
	}
	return(RET_OK);
}



/*
 * Convert a media-based "Short Format" Time structure
 * to an single time value.
 */
int
cdfs_ConvertHdt(DatePtr, Time, FsType)
union media_hdt	*DatePtr;
timestruc_t		*Time;
enum cdfs_type	FsType;
{
	Time->tv_sec = 0;
	Time->tv_nsec = 0;

	if (DatePtr == NULL) {
		return(RET_ERR);
	}

	/*
	 * Use the appropriate date template.
	 */
	switch (FsType) {
		case CDFS_ISO_9660: {
			Time->tv_sec = cdfs_ConvertDate(
				(DatePtr->Iso.hdt_Year + 1900), DatePtr->Iso.hdt_Month,
				DatePtr->Iso.hdt_Day, DatePtr->Iso.hdt_Hour,
				DatePtr->Iso.hdt_Minute, DatePtr->Iso.hdt_Second,
				(DatePtr->Iso.hdt_GmtOffset * 15 * 60) 
			);
			Time->tv_nsec = 0;
			break;
		}
		case CDFS_HIGH_SIERRA: {
			Time->tv_sec = cdfs_ConvertDate(
				DatePtr->Hs.hdt_Year + 1900, DatePtr->Hs.hdt_Month,
				DatePtr->Hs.hdt_Day, DatePtr->Hs.hdt_Hour,
				DatePtr->Hs.hdt_Minute, DatePtr->Hs.hdt_Second,
				(-1 * (c_correct))
			);
			Time->tv_nsec = 0;
			break;
		}
		default: {
			/*
			 * Invalid CDFS file-system type.
			 */
			cmn_err(CE_WARN,
				"cdfs_ConvertHdt(): Invalid CDFS FS-type (0x%x).\n");
			return(RET_ERR);
		}
	}
	return(RET_OK);
}



/*
 * Convert the individual time/date fields into a single time value
 * that represents the elapsed time since 12:00am Jan 1, 1970 GMT.
 */
time_t
cdfs_ConvertDate(Year, Month, Day, Hour, Min, Sec, GmtOff)
u_int	Year;								/* # of whole years (0-9999)	*/
u_int	Month;								/* Month of year (1-12)			*/
u_int	Day;								/* Day of Month (1-31)			*/
u_int	Hour;								/* Hour (0-23)					*/
u_int	Min;								/* Minute (0-59)				*/
u_int	Sec;								/* Second (0-59)				*/
int	GmtOff;									/* Offset from GMT: Seconds		*/
{
	time_t	Date;
	u_int	Cnt;							/* Loop Counter					*/

	/*
	 * Validate parameters.
	 * - The limits of 'Year' are based on the max # of
	 *   years (expressed in seconds) that can be stored in
	 *   a 'time_t' data type without causing an overflow.
	 *
	 * Note: We choose to validate only the year and month
	 * values because if we are too picky, the user will see a
	 * date of '0' even though the other data is only slightly
	 * invalid.  For example, with a recorded date of 'June 31',
	 * it is less confusing to the user to see a date of 'July 1'
	 * rather than Dec 31, 1969 (a typical result when 0 is returned).
	 * The most common violation is with the GMT offset value.
	 */
	 if ((Year < (1970-CDFS_MAX_YEARS)) ||
		(Year > (1970+CDFS_MAX_YEARS)) ||
		(Month == 0) || (Month > 12)) {
		return(0);
	}

	/*
	 * Combine the individual date fields into a single date value
	 * which represents the # of seconds since 12:00am Jan 1, 1970.
	 * - Convert elapsed WHOLE years into elapesed days.
	 * - Account for leap-years as follows:
	 *   - Add 1 day for each WHOLE Leap Year after 1970.
	 *   - Add 1 day if this is a leap-year and the day is AFTER 2/29.
	 * - Convert elapsed (whole) months to days..
	 * - Added in the day of the month.
	 * - Convert elapsed days to hours and translate to "local time".
	 * - Convert to elapsed minutes.
	 * - Convert to elapsed seconds.
	 */
	Date = ((Year - 1970) * 365);

	Date = Date + (Year-1)/4 - (1970/4);
	if ((Year%4 == 0) && (Month >= MAR)) { 
		Date++;
	}

	for (Cnt=1; Cnt < Month; Cnt++) {
		Date += cdfs_DaysOfMonth[Cnt-1];
	}
	Date = Date + (Day-1);

	Date = (Date * 24) + Hour;
	Date = (Date * 60) + Min;
	Date = (Date * 60) + Sec - GmtOff;

	return(Date);
}



int
cdfs_atoi(str, len)
uchar_t		*str;
uint_t		len;
{
	uint_t		val;
	uint_t		i;
	uint_t		digit;
	
	val = 0;
	for (i=0; i < len; i++) {
		digit = str[i];
		if ((digit < '0') || (digit > '9')) {
			break;
		}
		val = 10 * val + (digit - '0');
	}
		
	return(val);
}



/*
 * Initialize a pathname structure to store a NULL pathname.
 */
void
cdfs_pn_clear(pnp)
struct pathname *pnp;
{
	pnp->pn_buf[0] = '\0';
	pnp->pn_path = &pnp->pn_buf[0];
	pnp->pn_pathlen = 0;
	return;
}

	

/*
 * Initialize a pathname structure to the given string.
 * Similar to pn_set() except that this routine accepts
 * a length parameter instead of requiring a NULL terminated
 * string.
 */
int
cdfs_pn_set(pnp, str, len)
struct pathname *pnp;
uchar_t			*str;
uint_t			len;
{
	uint_t		count;

	count = MIN(len, MAXPATHLEN);
	(void) strncpy(&pnp->pn_buf[0], (caddr_t)str, count);
	pnp->pn_path = &pnp->pn_buf[0];
	pnp->pn_pathlen = count;
	return(RET_OK);
}




/*
 * Append the given string to the specified pathname buffer.
 */
int
cdfs_pn_append(pnp, str, len)
struct pathname *pnp;
uchar_t			*str;
uint_t			len;
{
	uint_t		count;

	count = MIN(len, (MAXPATHLEN - pnp->pn_pathlen));
	(void) strncpy(&pnp->pn_buf[pnp->pn_pathlen], (caddr_t)str, count);
	pnp->pn_pathlen += count;
	return(RET_OK);
}



/*
 * Resolve any XCDR pathname mappings that are enabled.
 */
int
cdfs_XcdrName (myvfs, source, str_len, dest_struct)

	struct vfs		*myvfs;					/* Ptr to fs data				*/
	uchar_t			*source;				/* String to manipulate			*/
	uint_t			str_len;				/* Length of source				*/
	struct pathname	*dest_struct;			/* Path struct to fill			*/
{
	struct cdfs		*myfs;					/* Ptr to fs private data		*/
	uint_t			count;					/* Loop counter					*/
	uchar_t			*tmp_src;				/* Ptr to location in source	*/
	uchar_t			*tmp_dst;				/* Ptr to location in dest		*/
	boolean_t		StripSepOne = B_FALSE;	/* Are we stripping separator 1?*/
	boolean_t		Same = B_FALSE;			/* Are source and dest the same?*/

	if (str_len > MAXPATHLEN) {
		return (ENAMETOOLONG);
	}

	myfs = CDFS_FS (myvfs);
	tmp_src = source;
	tmp_dst = (uchar_t *) dest_struct->pn_path;
	dest_struct->pn_pathlen = str_len;

	if (tmp_src == tmp_dst) {
		Same = B_TRUE;
	} else {
		Same = B_FALSE;
	}

	if (myfs->cdfs_NameConv != CD_NOCONV) {
		if ((myfs->cdfs_NameConv & (CD_NOVERSION | CD_LOWER)) != 0) {
			for (count = 0; count < str_len; (count++, tmp_src++, tmp_dst++)) {
				/*
				 * Convert pathname to lower case.  Strip out the
				 * separator 1, if there is no file name extension.
				 */
				if ((myfs->cdfs_NameConv & CD_LOWER) != 0) {
					if ((*tmp_src == ISO_FILEID_SEPARATOR1) &&
								(*(tmp_src + 1) == ISO_FILEID_SEPARATOR2)) {
						tmp_dst--;
						StripSepOne = B_TRUE;
						continue;
					} else {
						*tmp_dst = CDFS_TOLOWER ((int) *tmp_src);
					}
				} else {
					if (!Same) {
						*tmp_dst = *tmp_src;
					}
				}

				/*
				 * Remove separator and version number from end of pathname.
				 */
				if ((myfs->cdfs_NameConv & CD_NOVERSION) != 0) {
					if (*tmp_src == ISO_FILEID_SEPARATOR2) {
						dest_struct->pn_pathlen = count;
						break;
					}
				}
			}

			/*
			 * Adjust the pathlength, if necessary.
			 */
			if (StripSepOne) {
				dest_struct->pn_pathlen--;
			}

			return (RET_OK);
		}
	}

	/*
	 * Just copy the contents across, but only if they're pointing at
	 * different locations.
	 */
	if (!Same) {
		for (count = 0; count < str_len; count++) {
			*tmp_dst++ = *tmp_src++;
		}
	}

	return (RET_OK);
}



/*
 * Get the effective device number of an Inode.
 */
dev_t
cdfs_GetDevNum(vfs, ip)
struct vfs			*vfs;					/* Vfs pointer					*/
struct cdfs_inode	*ip;					/* Inode 						*/
{
	int					i;					/* Loop counter					*/
	struct cd_devmap	*devmap;			/* Device map entry				*/

	/*
	 * If this is not a Device-node file, then return an
	 * invalid dev number. 
	 */
	if (((ip->i_Mode & IFMT) != IFBLK) &&
		((ip->i_Mode & IFMT) != IFCHR)) {
		return(NODEV);
	}

	ASSERT((ip->i_Flags & CDFS_INODE_DEV_OK) != 0);
	
	/*
	 * Scan the Device mappings for the Inode's device number.
	 */
	devmap = &(CDFS_FS(vfs)->cdfs_DevMap[0]);
	for (i=0; i < (CDFS_FS (vfs))->cdfs_DevMapCnt; i++) {
		if (CDFS_CMPFID(&(devmap->fileid), &ip->i_Fid) == B_TRUE) {
			return(devmap->to_num);
		}
		devmap++;
	}

	/*
	 * The device mapping was not found.  Return the unmapped value.
	 */
	return(ip->i_DevNum);
}



/*
 * Get the effective UID of an Inode.
 */
uid_t
cdfs_GetUid(vfs, ip)
struct vfs			*vfs;					/* Vfs pointer					*/
struct cdfs_inode	*ip;					/* Inode pointer				*/
{
	int					i;					/* Loop counter					*/
	struct cd_uidmap	*uidmap; 			/* User ID map entry			*/

	/*
	 * If Inode has no valid UID, return the default.
	 */
	 if ((ip->i_Flags & CDFS_INODE_UID_OK) == 0) {
		return(CDFS_FS(vfs)->cdfs_Dflts.def_uid);
	}

	/*
	 * Check the current UID mappings for the Inode's UID.
	 */
	uidmap = &(CDFS_FS(vfs)->cdfs_UidMap[0]);
	for (i=0; i < (CDFS_FS (vfs))->cdfs_UidMapCnt; i++) {
		if (uidmap->from_uid == ip->i_UserID) {
			return(uidmap->to_uid);
		}
		uidmap++;
	}

	/*
	 * The UID mapping was not found.  Return the unmapped value.
	 */
	return(ip->i_UserID);
}

	
/*
 * Get the effective GID of an Inode.
 */
gid_t
cdfs_GetGid(vfs, ip)
struct vfs			*vfs;					/* Vfs pointer					*/
struct cdfs_inode	*ip;					/* Inode pointer				*/
{
	int					i;					/* Loop counter					*/
	struct cd_gidmap	*gidmap;			/* Group ID map entry			*/

	/*
	 * If Inode has no valid UID, return the default.
	 */
	if ((ip->i_Flags & CDFS_INODE_GID_OK) == 0) {
		return(CDFS_FS(vfs)->cdfs_Dflts.def_gid);
	}

	/*
	 * Check the current Group ID mappings for the Inode's
	 * nominal Group ID.
	 */
	gidmap = &(CDFS_FS(vfs)->cdfs_GidMap[0]);
	for (i=0; i < (CDFS_FS (vfs))->cdfs_GidMapCnt; i++) {
		if (gidmap->from_gid == ip->i_GroupID) {
			return(gidmap->to_gid);
		}
		gidmap++;
	}

	/*
	 * The GID mapping was not found.  Return the unmapped value.
	 */
	return(ip->i_GroupID);
}




/*
 * Get the effective permissions of an Inode.
 */
mode_t
cdfs_GetPerms(vfs, ip)
struct vfs			*vfs;					/* Vfs pointer					*/
struct cdfs_inode	*ip;					/* Inode pointer				*/
{
	mode_t	perms;							/* Effective permission of Inode*/

	/*
	 * If Inode has no valid UID, return the default.
	 */
	if ((ip->i_Mode & IFMT) != IFDIR) {
		if ((ip->i_Flags & CDFS_INODE_PERM_OK) == 0) {
			perms = (ip->i_Mode & IFMT) |
				(CDFS_FS(vfs)->cdfs_Dflts.def_fperm) & ~IFMT;
		} else {
			perms = ip->i_Mode;
		}
		return(perms);
	}

	/*
	 * Get the permissions for a directory.
	 */
	if ((ip->i_Flags & CDFS_INODE_PERM_OK) == 0) {
		perms = (ip->i_Mode & IFMT) |
			(CDFS_FS(vfs)->cdfs_Dflts.def_dperm) & ~IFMT;
	} else {
		perms = ip->i_Mode;
		switch (CDFS_FS(vfs)->cdfs_Dflts.dirsperm) {
			case CD_DIRRX: {
				/*
				 * READ perms imply POSIX EXEC as well.
				 */
				perms |= (
					(((perms & IREAD_USER)  == 0) ? 0 : IEXEC_USER) |
					(((perms & IREAD_GROUP) == 0) ? 0 : IEXEC_GROUP) |
					(((perms & IREAD_OTHER) == 0) ? 0 : IEXEC_OTHER)); 
				break;
			}
			case CD_DIRXAR: {
				break;
			}
			default: {
				cmn_err(CE_WARN,
					"cdfs_GetPerms(): Invalid directory search mode: %x\n",
					CDFS_FS(vfs)->cdfs_Dflts.dirsperm);
				break;
			}
		}
	}
	
	return(perms); 
}
