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

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1991,1992  Intel Corporation
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ident	"@(#)uts-comm:fs/cdfs/cdfs_susp.c	1.8"
#ident	"$Header: $"

#include <fs/buf.h>
#include <fs/cdfs/cdfs_fs.h>
#include <fs/cdfs/cdfs_inode.h>
#include <fs/cdfs/cdfs_susp.h>
#include <fs/cdfs/iso9660.h>
#include <fs/dnlc.h>
#include <fs/fbuf.h>
#include <fs/mode.h>
#include <fs/mount.h>
#include <fs/stat.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/proc.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/utsname.h>
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
 * Get the RRIP Alternate Name associated with the
 * specified Directory Record. 
 */
int
cdfs_GetRripName(vfs, drec_buf, pname)
struct vfs			*vfs;					/* File system's VFS structure	*/
struct cdfs_iobuf	*drec_buf;				/* Dir Rec buffer structure		*/
struct pathname		*pname;					/* Pathname struct to put name	*/
{
	struct susp_suf		*susp;				/* Addr of in-core SUSP area	*/
	uint_t				susplen;			/* Length of SUSP area (Bytes)	*/
	struct cdfs_iobuf	suf_buf;			/* SUF buffer structure			*/

	struct rrip_nm		*nm_suf;			/* Template for Alt Name SUF	*/
	boolean_t			nm_cont;			/* 'NM' continuation flag		*/		
	struct susp_ce		ce_suf; 			/* Continuation SUF				*/
	int					RetVal;				/* Return value of called procs	*/

	cdfs_pn_clear(pname);

	/*
	 * Locate the starting address (and size) of the
	 * SUSP area of the current Dir Rec.
	 */
	RetVal = cdfs_LocSusp(vfs, drec_buf, &susp, &susplen);
	if (RetVal != RET_OK) {
		return(RetVal);
	}

	if (susplen == 0) {
		return(RET_OK);
	}

	/*
	 * Initialize the SUF buffer structure based on the
	 * location of the SUSP area of the Dir Rec.
	 *
	 * Note: The Dir Rec buffer structure can not be used here
	 * for two reasons:
	 * 1) The location of the Dir Rec must not be lost. 
	 * 2) Addition media I/O (as would be necessary if a 'CE' suf
	 *	 were found) requires a 'struct buf' instead of a 'struct fbuf'.
	 */
	CDFS_SETUP_IOBUF(&suf_buf, CDFS_BUFIO);
	suf_buf.sb_dev = CDFS_DEV(vfs);
	suf_buf.sb_sect = drec_buf->sb_sect;
	suf_buf.sb_sectoff = drec_buf->sb_sectoff;
	suf_buf.sb_start = (uchar_t *)susp;
	suf_buf.sb_end = (uchar_t *)susp + susplen;
	suf_buf.sb_offset = drec_buf->sb_offset +
		((uchar_t *)susp - drec_buf->sb_ptr);
	suf_buf.sb_ptr = (uchar_t *)susp;

	/*
	 * Locate and concatenate successive 'NM' SUFs util no more exist.
	 */
	nm_cont = B_FALSE;
	bzero((caddr_t)&ce_suf, sizeof(ce_suf));
	for (;;) {
		RetVal = cdfs_ReadSUF(vfs, &suf_buf, CDFS_SUFID_NM, 1, &ce_suf); 
		if (RetVal != RET_OK) {
			break;
		}

		nm_suf = (struct rrip_nm *)suf_buf.sb_ptr;

		RetVal = cdfs_AppendRripNM(pname, nm_suf, &nm_cont); 
		if (RetVal != RET_OK) {
			break;
		}

		if (nm_cont == B_FALSE) {
			break;
		}
		
		suf_buf.sb_ptr += suf_buf.sb_reclen;
		suf_buf.sb_offset += suf_buf.sb_reclen;
	}

	CDFS_RELEASE_IOBUF(&suf_buf);
	
	return(RetVal);
}



/*
 * Determine if the specified Dir Rec should be hidden from the user.
 */
int
cdfs_HiddenRrip(vfs, drec_buf)
struct vfs			*vfs;					/* File system's VFS structure	*/
struct cdfs_iobuf	*drec_buf;				/* Dir Rec buffer structure		*/
{
	struct susp_suf		*susp;				/* Addr of in-core SUSP area	*/
	uint_t				susplen;			/* Length of SUSP area (Bytes)	*/
	struct cdfs_iobuf	suf_buf;			/* SUF buffer structure			*/

	struct susp_ce		ce_suf;
	int					RetVal;

	/*
	 * Locate the starting address (and size) of the
	 * SUSP area of the current Dir Rec.
	 */
	RetVal = cdfs_LocSusp(vfs, drec_buf, &susp, &susplen);
	if (RetVal != RET_OK) {
		return(RetVal);
	}

	if (susplen == 0) {
		return(RET_FALSE);
	}

	/*
	 * Initialize the SUF buffer structure based on the
	 * location of the SUSP area of the Dir Rec.
	 *
	 * Note: The Dir Rec buffer structure can not be used here
	 * for two reasons:
	 * 1) The location of the Dir Rec must not be lost. 
	 * 2) Addition media I/O (as would be necessary if a 'CE' suf
	 *	 were found) requires a 'struct buf' instead of a 'struct fbuf'.
	 */
	CDFS_SETUP_IOBUF(&suf_buf, CDFS_BUFIO);
	suf_buf.sb_dev = CDFS_DEV(vfs);
	suf_buf.sb_sect = drec_buf->sb_sect;
	suf_buf.sb_sectoff = drec_buf->sb_sectoff;
	suf_buf.sb_start = (uchar_t *)susp;
	suf_buf.sb_end = (uchar_t *)susp + susplen;
	suf_buf.sb_offset = drec_buf->sb_offset +
		((uchar_t *)susp - drec_buf->sb_ptr);
	suf_buf.sb_ptr = (uchar_t *)susp;
	suf_buf.sb_reclen = susp->suf_Len;

	/*
	 * Determine if an 'RE' (Relocation) SUF is present.
	 */
	bzero((caddr_t)&ce_suf, sizeof(ce_suf));

	RetVal = cdfs_ReadSUF(vfs, &suf_buf, CDFS_SUFID_RE, 1, &ce_suf); 
	switch (RetVal) {
		case RET_OK: {
			RetVal = RET_TRUE;
			break;
		}
		case RET_EOF:
		case RET_NOT_FOUND: {
			RetVal = RET_FALSE;
			break;
		}
		default: {
			break;
		}
	}
		
	CDFS_RELEASE_IOBUF(&suf_buf);
	
	return(RetVal);
}



/*
 * Locate the starting address and size of the SUSP area within
 * the specified Directory Record.
 */
int
cdfs_LocSusp(vfs, drec_buf, susp, susplen)
struct vfs			*vfs;					/* File system's VFS structure	*/
struct cdfs_iobuf	*drec_buf;				/* Dir Rec buffer structure		*/
struct susp_suf		**susp;					/* Ret addr for SUSP pointer	*/
uint_t				*susplen;				/* Ret addr for SUSP length		*/
{
	union media_drec	*drec;				/* Dir Rec template				*/

	drec = (union media_drec *) drec_buf->sb_ptr;

	/*
	 * Compute the location of the SUSP area:
	 * - Locate the beginning of the File ID string.
	 * - Add the length of the File ID string.
	 * - Add 1 byte of padding if the File ID string is an even # of bytes.
	 * - Add the System Use Area offset of SUSP area.
	 * - Compute the SUSP size by subtracting the Dir Rec offset of
	 *	 the SUSP area from the total # of bytes in the Dir Rec.
	 */
	switch (CDFS_TYPE(vfs)) {
		case CDFS_ISO_9660: {
			*susp = (struct susp_suf *)
				((uchar_t *)&drec->Iso.drec_VarData +
				drec->Iso.drec_FileIDSz +
				((drec->Iso.drec_FileIDSz % 2 == 0) ? 1 : 0));
			*susplen = (uint_t)
				(drec->Iso.drec_Size -
				((uchar_t *)*susp - (uchar_t *)drec));
			break;
		}
		case CDFS_HIGH_SIERRA: {
			*susp = (struct susp_suf *)
				((uchar_t *)&drec->Hs.drec_VarData +
				drec->Hs.drec_FileIDSz +
				((drec->Hs.drec_FileIDSz % 2 == 0) ? 1 : 0));
			*susplen = (uint_t)
				(drec->Hs.drec_Size -
				((uchar_t *)*susp - (uchar_t *)drec));
			break;
		}
		default: {
			cmn_err(CE_WARN,
				"cdfs_LocSusp(): Unknown CDFS type: 0x%x\n",
				CDFS_TYPE(vfs));
			*susp = NULL;
			*susplen = 0;
			return(RET_ERR);
		}
	}

	/*
	 * Adjust for the SUSP offset ('SP') for all Dir Recs
	 * except the Root Dir Rec.
	 * Note: When building the Root Inode, the Root's FID is invalid.
	 */
	if ((drec_buf->sb_offset != CDFS_ROOTFID(vfs).fid_Offset) ||
		(drec_buf->sb_sect != CDFS_ROOTFID(vfs).fid_SectNum)) {
		if ((CDFS_FLAGS(vfs) & CDFS_BUILDING_ROOT) == 0) {
			*susp += CDFS_SUSPOFF(vfs);
			*susplen -= CDFS_SUSPOFF(vfs);
		}
	}
	return(RET_OK);
}




	
/*
 * Find the specified SUF by search the remaining SUFs
 * (System Use Fields) within the remaining SUSP area(s).
 * SUSP 'CE' and 'ST' fields are transparently handled for
 * each SUSP area.  Note: The IOBUF MUST be a CDFS_BUFIO type.
 */
int
cdfs_ReadSUF(vfs, suf_buf, sufid_wanted, count, ce_suf)
struct vfs			*vfs;					/* File system's VFS structure	*/
struct cdfs_iobuf	*suf_buf;				/* SUF buffer structure			*/
uint_t				sufid_wanted;			/* Desired SUF id # 			*/
uint_t				count;					/* Relative occurence count		*/
struct susp_ce		*ce_suf;				/* Caller's 'CE' SUF storage	*/
{
	union media_suf	*suf;					/* Pointer to current SUF		*/
	uint_t			sufid;					/* SUF ID of current SUF		*/

	ulong_t			reclen_min;				/* Min Rec len needed in buffer	*/
	ulong_t			nextsect; 				/* Sect of next SUSP area		*/
	ulong_t			nextsectoff;			/* Sect offset of next SUSP	area*/
	uint_t			rrmask;					/* RRIP 'RR' mask for desired SUF*/
	int				RetVal;					/* Return value of called procs	*/

	/*
	 * Verify caller is pass-in a CDFS_BUFIO type IOBUF.
	 * This is required because the Continuation Area's ('CE') are
	 * sector-based and,strictly speeking, are not part of a Vnode.
	 */
	if (suf_buf->sb_type != CDFS_BUFIO) {
		cmn_err(CE_WARN, "cdfs_ReadSUF(): Invalid IOBUF type (0x%x).",
			suf_buf->sb_type);
		cmn_err(CE_CONT, "Expecting a type 0x%x IOBUF.\n\n", CDFS_BUFIO);
		return(RET_ERR);
	}

	/*
	 * Set up the RRIP 'RR' mask corresponding to the desired SUF.
	 * If while searching, we find a RRIP 'RR' SUF, it will indicate
	 * whether or not the desired field is recoreded.
	 * If the caller is not looking for a specific SUF,
	 * (i.e CDFS_SUFID_NULL) then the 'RR' SUF is not useful.
	 */
	rrmask = 0;
	if (sufid_wanted != CDFS_SUFID_NULL) {
		switch (sufid_wanted) {
			case CDFS_SUFID_PX: rrmask = RRIP_RR_PX; break;
			case CDFS_SUFID_PN: rrmask = RRIP_RR_PN; break;
			case CDFS_SUFID_SL: rrmask = RRIP_RR_SL; break;
			case CDFS_SUFID_NM: rrmask = RRIP_RR_NM; break;
			case CDFS_SUFID_CL: rrmask = RRIP_RR_CL; break;
			case CDFS_SUFID_PL: rrmask = RRIP_RR_PL; break;
			case CDFS_SUFID_RE: rrmask = RRIP_RR_RE; break;
			case CDFS_SUFID_TF: rrmask = RRIP_RR_TF; break;
			default: rrmask = 0; break;
		}
	}

	/*
	 * Search each remaining SUSP area.
	 */
	for (;;) {
		/*
		 * Verify that the next SUF is within the current buffer.
		 */ 
		if ((suf_buf->sb_start == NULL) ||
			(suf_buf->sb_ptr < suf_buf->sb_start) ||
			(suf_buf->sb_ptr + sizeof(struct susp_suf) > suf_buf->sb_end)) {
			/*
			 * If the current buffer has valid data in it,
			 * then, since the data is not longer needed,
			 * we need to start using the data defined by
			 * the 'CE' SUF previously encountered. 
			 */
			if (suf_buf->sb_start != NULL) {

				if ((ce_suf->ce_Sig1 != 'C') ||
					(ce_suf->ce_Sig2 != 'E')) {
					return(RET_NOT_FOUND);
				}

				suf_buf->sb_sect =
					ce_suf->ce_Loc >> CDFS_BLKSECT_SHFT(vfs);

				suf_buf->sb_offset =
					((ce_suf->ce_Loc << CDFS_BLKSHFT(vfs)) &
					CDFS_SECTMASK(vfs)) + ce_suf->ce_Offset;
			}

			RetVal = cdfs_ReadSect(vfs, suf_buf);
			if (RetVal != RET_OK) {
				return(RetVal);
			}

			/*
			 * Adjust the end of the buffer to match the end
			 * of the 'CE' area. 
			 */
			if ((ce_suf->ce_Sig1 == 'C') &&
				(ce_suf->ce_Sig2 == 'E')) {

				if (suf_buf->sb_ptr + ce_suf->ce_AreaSz <= suf_buf->sb_end) {
					suf_buf->sb_end = suf_buf->sb_ptr + ce_suf->ce_AreaSz;
				} else {
					cmn_err(CE_WARN,
						"cdfs_ReadSUF(): Continuation Area ('CE') too large:");
					cmn_err(CE_CONT,
						"Sect=%d, Sect size=%d, CE offset=%d, CE len=%d\n",
						ce_suf->ce_Loc, CDFS_SECTSZ(vfs), ce_suf->ce_Offset,
						ce_suf->ce_AreaSz);
					cmn_err(CE_CONT, "CE Area truncated at sector boundry.\n\n");
				}
				bzero((caddr_t)ce_suf, sizeof(*ce_suf));
			}

			/*
			 * We need to verify that the new buffer contains
			 * enough data for at least a minimal-size SUF.
			 * Note - Infinate loops won't occur since there
			 * will be at most 1 'CE' area to process.
			 */
			continue;
		}

		/*
		 * Verify that the length of the SUF is valid and
		 * that there's enough data in the buffer for the
		 * entire SUF.
		 * Note: Since we've gotten here, there are at least
		 * enough bytes in the buffer for a minimal size SUF.
		 */
		suf = (union media_suf *)suf_buf->sb_ptr;
		sufid = CDFS_SUFID(suf->gen.suf_Sig1, suf->gen.suf_Sig2);
		suf_buf->sb_reclen = suf->gen.suf_Len;

		if ((suf_buf->sb_reclen < sizeof(struct susp_suf)) ||
			(suf_buf->sb_ptr + suf_buf->sb_reclen > suf_buf->sb_end)) {
			/*
			 * If we haven't seen the 'SP' SUF, then quietly,
			 * return an error.  No SUSP-based extensions exist.
			 */
			if ((CDFS_FLAGS(vfs) & CDFS_SUSP_PRESENT) == 0) {
				return(RET_NOT_FOUND);
			}

			cmn_err(CE_WARN,
				"cdfs_ReadSUF(): Invalid System Use Field (SUF) found:");
			cmn_err(CE_CONT,
				"Sect %d,  Sect offset %d,  SUF len %d\n",
				suf_buf->sb_sect,
				(suf_buf->sb_offset - suf_buf->sb_sectoff),
				suf_buf->sb_reclen);

			if (suf_buf->sb_reclen < sizeof(struct susp_suf)) {
				cmn_err(CE_CONT,
					"Invalid SUF length - Min length is %d bytes\n\n",
					sizeof(struct susp_suf));
			} else {
				cmn_err(CE_CONT,
				"SUF exceeds sector boundry - Sec size = %d bytes.\n\n",
				suf_buf->sb_end - suf_buf->sb_start);
			}

			return(RET_ERR);
		}
				
		/*
		 * If we haven't encountered the 'SP' SUF, then this had
		 * better be the Root Inode and this "first" SUF had
		 * better be the 'SP' SUF.  Otherwise, there are no
		 * SUSP-complient extensions present. 
		 */
		if ((CDFS_FLAGS(vfs) & CDFS_SUSP_PRESENT) == 0) {
			if (((CDFS_FLAGS(vfs) & CDFS_BUILDING_ROOT) == 0) ||
				(sufid != CDFS_SUFID_SP) ||
				(suf->gen.suf_Len != sizeof(suf->sp)) ||
				(suf->sp.sp_ID1 != SUSP_SP_ID1) ||
				(suf->sp.sp_ID2 != SUSP_SP_ID2)) {
				return(RET_NOT_FOUND);
			}

			CDFS_FLAGS(vfs) |= CDFS_SUSP_PRESENT;
			CDFS_SUSPOFF(vfs) = suf->sp.sp_Offset;
		}

		/*
		 * Some of the SUSP SUF's require special handling:
		 * - 'CE': SUSP Continuation Area SUF.
		 * - 'ST': SUSP Terminating SUF.
		 */
		switch (sufid) {
			case CDFS_SUFID_CE: {
				if ((ce_suf->ce_Sig1 != 'C') ||
					(ce_suf->ce_Sig2 != 'E')) {
					*ce_suf = suf->ce;
				} else {
					cmn_err(CE_WARN,
						"cdfs_ReadSUF(): Multiple 'CE' System Use Fields.");
					cmn_err(CE_CONT, "Sector %d   Sect offset %d\n",
						suf_buf->sb_sect,
						suf_buf->sb_offset - suf_buf->sb_sectoff);
					cmn_err(CE_CONT, "'CE' System Use Field Ignored.\n\n");
				}
				break;
			}
			case CDFS_SUFID_ST: {
				/*
				 * There should be no more SUF's following the
				 * Terminator SUF so set the record length to
				 * consume the remainder of the SUSP area.
				 */
				suf_buf->sb_reclen = suf_buf->sb_end - suf_buf->sb_ptr;
				break;
			}
		}

		/* 
		 * See if this is the SUF we're looking for.
		 */
		if ((sufid_wanted == CDFS_SUFID_NULL) ||
			(sufid == sufid_wanted)) {
			count--;
			if (count == 0) {
				return(RET_OK);
			}
		}

		/*
		 * If this is a RRIP 'RR" SUF, it may tell us if the
		 * SUF we're looking for is recorded in this SUSP area.
		 */
		if (sufid == CDFS_SUFID_RR) {
			if ((rrmask != 0) && (suf->rr.rr_Flags & rrmask) == 0) {
				return(RET_NOT_FOUND);
			}
		}

		/*
		 * Calc the location of the next SUF.
		 */
		suf_buf->sb_ptr += suf_buf->sb_reclen;
		suf_buf->sb_offset += suf_buf->sb_reclen;
	}
}




/*
 * Get the Rrip information associated with a Dir Rec (File Section)
 * and merge its data into the Inode structure.
 */
int
cdfs_GetRrip(vfs, drec_buf, rrip)
struct vfs			*vfs;
struct cdfs_iobuf	*drec_buf;
struct cdfs_rrip	*rrip;
{
	struct cdfs_iobuf	suf_buf;
	struct susp_suf		*susp;
	uint_t				susplen;

	union media_suf		*suf;
	uint_t				sufid;
	struct susp_ce		ce_suf;

	struct pathname		*symlink;
	boolean_t			sl_pending;
	boolean_t			sl_comp_cont;

	struct pathname		*name;
	boolean_t			nm_pending;

	uint_t				tf_flags;
	uint_t				tf_bitnum;
	timestruc_t			*tf_var;
	uchar_t				*tf_data;

	int					RetVal;

	RetVal = cdfs_LocSusp(vfs, drec_buf, &susp, &susplen);
	if (RetVal != RET_OK) {
		return(RetVal);
	}

	if (susplen == 0) {
		rrip->rrip_Flags = 0;
		return(RET_OK);
	}

	sl_pending = B_FALSE;
	symlink = &rrip->rrip_SymLink;
	symlink->pn_buf = NULL;
	symlink->pn_path = NULL;
	symlink->pn_pathlen = 0;

	nm_pending = B_FALSE;
	name = &rrip->rrip_AltName;
	name->pn_buf = NULL;
	name->pn_path = NULL;
	name->pn_pathlen = 0;

	CDFS_SETUP_IOBUF(&suf_buf, CDFS_BUFIO);
	suf_buf.sb_dev = CDFS_DEV(vfs);
	suf_buf.sb_sect = drec_buf->sb_sect;
	suf_buf.sb_sectoff = drec_buf->sb_sectoff;
	suf_buf.sb_start = (uchar_t *)susp;
	suf_buf.sb_end = (uchar_t *)susp + susplen;
	suf_buf.sb_offset = drec_buf->sb_offset +
		((uchar_t *)susp - drec_buf->sb_ptr);
	suf_buf.sb_ptr = (uchar_t *)susp;

	bzero((caddr_t)&ce_suf, sizeof(ce_suf));
	rrip->rrip_Flags = 0;
	for (;;) {
		RetVal = cdfs_ReadSUF(vfs, &suf_buf, CDFS_SUFID_NULL, 1, &ce_suf);
		if (RetVal != RET_OK) {
			/*
			 * Since cdfs_ReadSUF() eventually returns a value other than
			 * RET_OK, we check for the return values that are expected.
			 */
			if ((RetVal == RET_EOF) ||
				(RetVal == RET_NOT_FOUND)) {
				RetVal = RET_OK;
			}
			break;
		}

		suf = (union media_suf *)suf_buf.sb_ptr;
		sufid = CDFS_SUFID(suf->gen.suf_Sig1, suf->gen.suf_Sig2);

		/*
		 * XXX - The version # of each SUF should be checked
		 * before processing the SUF's data.
		 */
		switch (sufid) {
			case CDFS_SUFID_PX: {
				rrip->rrip_Flags |= RRIP_RR_PX;
				rrip->rrip_Mode = suf->px.px_Mode;
				rrip->rrip_LinkCnt = suf->px.px_LinkCnt;
				rrip->rrip_UserID = suf->px.px_UserID;
				rrip->rrip_GroupID = suf->px.px_GroupID;
				break;
			}
			case CDFS_SUFID_PN: {
				rrip->rrip_Flags |= RRIP_RR_PN;
				rrip->rrip_DevNum_Hi = suf->pn.pn_DevHigh;
				rrip->rrip_DevNum_Lo = suf->pn.pn_DevLow;
				break;
			}
			case CDFS_SUFID_SL: {
				rrip->rrip_Flags |= RRIP_RR_SL;
				if (sl_pending == B_FALSE) {
					if (symlink->pn_buf != (char *)NULL) {
						break;
					} 
					pn_alloc(symlink);
					sl_comp_cont = B_FALSE;
				}
				RetVal = cdfs_AppendRripSL(vfs, symlink,
					(struct rrip_sl *)suf, &sl_comp_cont);
				if (RetVal != RET_OK) {
					break;
				}
				sl_pending = ((suf->sl.sl_Flags & RRIP_SL_CONTINUE) != 0) ?
					B_TRUE : B_FALSE;
				break;
			}
			case CDFS_SUFID_NM: {
				rrip->rrip_Flags |= RRIP_RR_NM;
				if (nm_pending == B_FALSE) {
					if (name->pn_buf != (char *)NULL) {
						break;
					} 
					pn_alloc(name);
				}
				RetVal = cdfs_AppendRripNM(name, (struct rrip_nm *)suf,
					&nm_pending);
				if (RetVal != RET_OK) {
					break;
				}
				nm_pending = ((suf->nm.nm_Flags & RRIP_NM_CONTINUE) != 0) ?
					B_TRUE : B_FALSE;
				break;
			}
			case CDFS_SUFID_CL: {
				rrip->rrip_Flags |= RRIP_RR_CL;
				rrip->rrip_ChildLink = suf->cl.cl_Loc;
				break;
			}
			case CDFS_SUFID_PL: {
				rrip->rrip_Flags |= RRIP_RR_PL;
				rrip->rrip_ParentLink = suf->pl.pl_Loc;
				break;
			}
			case CDFS_SUFID_RE: {
				rrip->rrip_Flags |= RRIP_RR_RE;
				break;
			}
			case CDFS_SUFID_TF: {
				rrip->rrip_Flags |= RRIP_RR_TF;
				tf_data = &suf->tf.tf_TimeStamps;
				tf_bitnum = 0;
				tf_flags = suf->tf.tf_Flags & ~RRIP_TF_LONG;
				while (tf_flags != 0) {
					if ((tf_flags & 0x01) != 0) {
						switch (0x01 << tf_bitnum) {
						case RRIP_TF_CREATE: tf_var = &rrip->rrip_CreateDate;
							break;
						case RRIP_TF_MODIFY: tf_var = &rrip->rrip_ModDate;
							break;
						case RRIP_TF_ACCESS: tf_var = &rrip->rrip_AccessDate;
							break;
						case RRIP_TF_ATTR: tf_var = &rrip->rrip_AttrDate;
							break;
						case RRIP_TF_BACKUP: tf_var = &rrip->rrip_BackupDate;
							break;
						case RRIP_TF_EXPIRE: tf_var = &rrip->rrip_ExpireDate;
							break;
						case RRIP_TF_EFFECT: tf_var = &rrip->rrip_EffectDate;
							break;
						default:
							cmn_err(CE_NOTE,
								"cdfs_RripTF(): Invalid RRIP TF bit: 0x%x.\n",
								(0x01 < tf_bitnum));
							tf_bitnum++;
							tf_flags >>= 1;
							continue;
						}
						if ((suf->tf.tf_Flags & RRIP_TF_LONG) != 0) {
							(void)cdfs_ConvertAdt(
								(union media_adt *)tf_data,
								tf_var, CDFS_ISO_9660);
							tf_data += sizeof(struct Pure9660_adt);
						} else {
							(void)cdfs_ConvertHdt(
								(union media_hdt *)tf_data,
								tf_var, CDFS_ISO_9660);
							tf_data += sizeof(struct Pure9660_hdt);
						}
					}
					tf_bitnum++;
					tf_flags >>= 1;
				}
				break;
			}
			case CDFS_SUFID_ER: {
				/*
				 * We ignore all 'ER' SUF's except we're building
				 * the Root Inode and the we haven't yet found the
				 * RRIP 'ER' SUF.
				 */
				if (((CDFS_FLAGS(vfs) & CDFS_BUILDING_ROOT) != 0) && 
					((CDFS_FLAGS(vfs) & CDFS_RRIP_PRESENT) == 0)) {
					if ((suf->er.er_Ver == SUSP_ER_VER) &&
						(suf->er.er_ExtVer == RRIP_ER_VER) &&
						(suf->er.er_IdLen == RRIP_ER_ID_LEN) &&
						(strncmp((caddr_t)&suf->er.er_VarData,
							RRIP_ER_ID_STRING, RRIP_ER_ID_LEN) == 0)) {
						CDFS_FLAGS(vfs) |= CDFS_RRIP_PRESENT;
					}
				}
			}
			default: {
				/*
				 * Ignore all other SUFs.
				 */
				break;
			}
		}
		
		/*
		 * If an error occurd then bail out.
		 */
		if (RetVal != RET_OK) {
			break;
		}

		suf_buf.sb_ptr += suf_buf.sb_reclen;
		suf_buf.sb_offset += suf_buf.sb_reclen;
	}

	/*
	 * We're done collecting the RRIP data so release the I/O buffer.
	 */
	CDFS_RELEASE_IOBUF(&suf_buf);

	return(RetVal);
}



/*
 * Append the SL pathname components to the pathname structure.
 */
int
cdfs_AppendRripSL(vfs,symlink, sl_suf, contflg)
struct vfs		*vfs;
struct pathname *symlink;
struct rrip_sl	*sl_suf;
boolean_t		*contflg;
{
	struct rrip_slcr	*slcr;
	uchar_t				*bufend;

	uchar_t				*hostname;
	struct pathname		*mntpnt;

	slcr = (struct rrip_slcr *)&sl_suf->sl_CompRec; 
	bufend = (uchar_t *)sl_suf + sl_suf->sl_Len; 
	while ((uchar_t *)slcr < bufend) {
		/*
		 * If this is not a continuation of the previous component
		 * add the pathname seperator (slash: "/") between components.
		 * However, don't add a seperator at the beginning.
		 */
		if ((*contflg == B_FALSE) && (symlink->pn_pathlen != 0)) {
			cdfs_pn_append(symlink, (uchar_t *)"/", 1);
		}

		/*
		 * Detect and handle special-case pathname components.
		 * - Current directory 
		 * - Parent directory 
		 * - Root directory 
		 * - Volume Root directory
		 * - Host Name
		 */
		if ((slcr->slcr_Flags & RRIP_SLCR_CURRENT) != 0) {
			cdfs_pn_append(symlink, (uchar_t *)".", 1);	

		} else if ((slcr->slcr_Flags & RRIP_SLCR_PARENT) != 0) {
			cdfs_pn_append(symlink, (uchar_t *)"..", 2);

		} else if ((slcr->slcr_Flags & RRIP_SLCR_ROOT) != 0) {
			cdfs_pn_append(symlink, (uchar_t *)"/", 1);

		} else if ((slcr->slcr_Flags & RRIP_SLCR_VOLROOT) != 0) {
			mntpnt = &CDFS_FS(vfs)->cdfs_MntPnt;
			cdfs_pn_append(symlink, (uchar_t *)mntpnt->pn_buf,	
				mntpnt->pn_pathlen);

		} else if ((slcr->slcr_Flags & RRIP_SLCR_HOST) != 0) {
			hostname = (uchar_t *)&utsname.sysname;
			cdfs_pn_append(symlink, hostname, (uint_t)strlen(hostname));

		} else {
			cdfs_pn_append(symlink, &slcr->slcr_Comp, slcr->slcr_Len);
		}

		if ((slcr->slcr_Flags & RRIP_SLCR_CONTINUE) != 0) {
			*contflg = B_TRUE;
		} else {
			*contflg = B_FALSE;
		}
		
		slcr = (struct rrip_slcr *)((uchar_t *)slcr + RRIP_SLCR_SIZE(slcr));
	}
	return(RET_OK);
}






/*
 * Append the Alternate Name ("NM") pathname components to the
 * pathname structure.
 */
int
cdfs_AppendRripNM(altname, nm_suf, contflg)
struct pathname *altname;
struct rrip_nm	*nm_suf;
boolean_t		*contflg;
{
	uchar_t 	*hostname;

	/*
	 * If this is not a continuation of the previous component,
	 * add the pathname seperator (slash: "/") between components.
	 * However, don't add a seperator at the beginning.
	 */
	if ((*contflg == B_FALSE) && (altname->pn_pathlen != 0)) {
		cdfs_pn_append(altname, (uchar_t *)"/", 1);
	}

	/*
	 * Detect and handle special-case pathname components.
	 * - Current directory 
	 * - Parent directory 
	 * - Host Name
	 */
	if ((nm_suf->nm_Flags & RRIP_NM_CURRENT) != 0) {
		cdfs_pn_append(altname, (uchar_t *)".", 1);	

	} else if ((nm_suf->nm_Flags & RRIP_NM_PARENT) != 0) {
		cdfs_pn_append(altname, (uchar_t *)"..", 2);

	} else if ((nm_suf->nm_Flags & RRIP_NM_HOST) != 0) {
		hostname = (uchar_t *)&utsname.sysname;
		cdfs_pn_append(altname, hostname, strlen(hostname));

	} else {
		cdfs_pn_append(altname, &nm_suf->nm_Name,
			nm_suf->nm_Len - CDFS_STRUCTOFF(rrip_nm, nm_Name));
	}

	if ((nm_suf->nm_Flags & RRIP_NM_CONTINUE) != 0) {
		*contflg = B_TRUE;
	} else {
		*contflg = B_FALSE;
	}
	return(RET_OK);
}



/*
 * Merge the RRIP information into the Inode structure.
 */
int
cdfs_MergeRrip(vfs, ip, rrip)
struct vfs			*vfs;
struct cdfs_inode	*ip;
struct cdfs_rrip	*rrip;
{
	struct cdfs_fid	fid;

	/*
	 * Update the appropriate Inode fields based on the RRIP data.
	 * - Update mode/perms/link count/user ID/group ID from 'PX' SUF.
	 * - Update Device Node's major/minor # from 'PN' SUF.
	 * - Update Symbolic Link name from 'SL' SUF.
	 * - Update Child link location from 'CL' SUF.
	 * - Update Parent link location from 'PL' SUF.
	 *
	 * XXX - Some checks could be added to validate the contents
	 * of each field before merging it into the Inode.
	 */
	ip->i_Rrip = rrip;
	if (rrip->rrip_Flags == 0) {
		return(RET_OK);
	}

	if ((rrip->rrip_Flags & RRIP_RR_PX) != 0) {
		/*
		 * XXX - Strickly speeking, the assignment of i_Mode should
		 * be a bit-for-bit assignment as is coded in cdfs_MergeXar().
		 * However, since the CDFS and RRIP mode bits will almost
		 * certainly never changed, a simple assignment should suffice.
		 */
		ip->i_Mode = rrip->rrip_Mode;
		if ((ip->i_Mode & IFMT) == IFSOCK) {
			ip->i_Mode = (ip->i_Mode & ~IFMT) | IFIFO;
		}

		ip->i_LinkCnt = rrip->rrip_LinkCnt;
		ip->i_UserID = rrip->rrip_UserID;
		ip->i_GroupID = rrip->rrip_GroupID;
		ip->i_Flags |=
			(CDFS_INODE_PERM_OK | CDFS_INODE_UID_OK | CDFS_INODE_GID_OK);
	}

	if ((rrip->rrip_Flags & RRIP_RR_PN) != 0) {
		/*
		 * Since a 'dev_t' is only 32-bits, we only save the
		 * low-order 32-bits of the RRIP 'PN' device number.
		 */
		ip->i_DevNum = rrip->rrip_DevNum_Lo;
		ip->i_Flags |= CDFS_INODE_DEV_OK;
	}

	if ((rrip->rrip_Flags & RRIP_RR_SL) != 0) {
		ip->i_SymLink = rrip->rrip_SymLink;
		ip->i_Size = rrip->rrip_SymLink.pn_pathlen;
	}

	if ((rrip->rrip_Flags & RRIP_RR_TF) != 0) {
		if ((rrip->rrip_AccessDate.tv_sec != 0) ||
			(rrip->rrip_AccessDate.tv_nsec != 0)) {
			ip->i_AccessDate = rrip->rrip_AccessDate;
		}
		if ((rrip->rrip_CreateDate.tv_sec != 0) ||
			(rrip->rrip_CreateDate.tv_nsec != 0)) {
			ip->i_CreateDate = rrip->rrip_CreateDate;
		}
		if ((rrip->rrip_ModDate.tv_sec != 0) ||
			(rrip->rrip_ModDate.tv_nsec != 0)) {
			ip->i_ModDate = rrip->rrip_ModDate;
		}
		if ((rrip->rrip_EffectDate.tv_sec != 0) ||
			(rrip->rrip_EffectDate.tv_nsec != 0)) {
			ip->i_EffectDate = rrip->rrip_EffectDate;
		}
		if ((rrip->rrip_ExpireDate.tv_sec != 0) ||
			(rrip->rrip_ExpireDate.tv_nsec != 0)) {
			ip->i_ExpireDate = rrip->rrip_ExpireDate;
		}
		if ((rrip->rrip_AttrDate.tv_sec != 0) ||
			(rrip->rrip_AttrDate.tv_nsec != 0)) {
			ip->i_AttrDate = rrip->rrip_AttrDate;
		}
		if ((rrip->rrip_BackupDate.tv_sec != 0) ||
			(rrip->rrip_BackupDate.tv_nsec != 0)) {
			ip->i_BackupDate = rrip->rrip_BackupDate;
		}
	}

	if ((rrip->rrip_Flags & RRIP_RR_CL) != 0) {
		ip->i_Flags |= CDFS_INODE_RRIP_REL;
		(ip->i_DirRec)->drec_ExtLoc = rrip->rrip_ChildLink;
	}

	if ((rrip->rrip_Flags & RRIP_RR_PL) != 0) {
		ip->i_Flags |= CDFS_INODE_RRIP_REL;
		(ip->i_DirRec)->drec_ExtLoc = rrip->rrip_ParentLink;
	}
	
	if ((rrip->rrip_Flags & RRIP_RR_RE) != 0) {
		ip->i_Flags |= CDFS_INODE_HIDDEN;
	}

	return(RET_OK);
}
