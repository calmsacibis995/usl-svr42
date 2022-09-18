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

#ident	"@(#)uts-x86:fs/cdfs/cdfs.cf/Space.c	1.5"
#ident	"$Header: $"

#include <sys/cdrom.h>
#include <sys/param.h>
#include <sys/types.h>



/*
 * Initial settings for administrator-settable file/directory attributes.
 */
uid_t				cdfs_InitialUID = UID_NOBODY;
gid_t				cdfs_InitialGID = GID_NOBODY;
mode_t				cdfs_InitialFPerm = 0444;
mode_t				cdfs_InitialDPerm = 0555;
int					cdfs_InitialDirSearch = CD_DIRXAR;
uint_t				cdfs_InitialNmConv = CD_LOWER | CD_NOVERSION;




/********************************************************************
 *
 * CDFS Kernel Parameters:
 *
 ********************************************************************/

/*
 * CDFS Temporary Buffer Size (Bytes):
 *
 * These buffers are used as a:
 * - Formating buffer when converting one or more Directory
 *	 Records to the generic directory entry format 'struct dirent'.
 *
 * - Continuous-address storage area for temporarily storing
 *	 both parts of a "record" that cross "logically adjacent" sectors.
 *
 * Guidelines:
 * - Should be a whole factor or whole multiple of a memory page.
 * - Should be at least as big as one disc sector (usually 2K).
 */
uint_t	cdfs_TmpBufSz = PAGESIZE;



/*
 * CDFS Incore Inode Cache and Hash-Table Sizes (Count):
 * 
 * The Inode Cache is used to store the "active" inodes
 * of all mounted CDFS file systems.
 *
 * The Inode Hash-Table is used to Hash the Inodes currently
 * in the Inode Cache.  This technique an specific Inode
 * to be located quickly.
 *
 * Guidelines:
 * Inode Cache Size:
 * - Should be relatively small - A CDFS Inode is about 500 bytes.
 * 
 * Inode Hash-Table Size:
 * - Should be an whole power-of-two (Helps performance).
 * - Should be a whole factor of the Inode Cache Size.
 */
uint_t	cdfs_InodeCnt = 128;
uint_t	cdfs_IhashCnt = 64;



/*
 * CDFS Incore Directory Record Cache Size (Count):
 *
 * The Dir Rec cache temporarily stores the information of
 * one or more Dir Recs for each multi-extent file whose
 * Inode is currently stored in the Inode Cache.  In a
 * sense, the Dir Rec cache is an extention to the Inode
 * Cache in order handle multi-extent files.
 *
 * Guidelines:
 * - Should be less than the Inode Cache: Multi-extent file are rare.
 */
uint_t	cdfs_DrecCnt = 16;
