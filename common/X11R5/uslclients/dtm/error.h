/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:error.h	1.10"

/******************************file*header********************************

    Description:
	Define file operations error codes.
*/

#ifndef _error_h
#define _error_h
			     /* 0 means no error */
#define ERR_IsAFile		1
#define ERR_Rm			2
#define ERR_Read		3
#define ERR_Write		4
#define ERR_NotAFile		5
#define ERR_Mkdir		6
#define ERR_Stat		7
#define ERR_OpenSrc		8
#define ERR_OpenDst		9
#define ERR_Link		10
#define ERR_Rename		11
#define ERR_ForeignLink		12
#define ERR_NoDirHardlink	13
#define ERR_MoveToSelf		14
#define ERR_CopyToSelf		15
#define ERR_LinkToSelf		16
#define ERR_MoveToDesc		17
#define ERR_CopyToDesc		18
#define ERR_ReadLink		19
#define ERR_OpenDir		20
#define ERR_CopySpecial		21
#define ERR_TargetFS		22
#define ERR_NoSrc		23
#define ERR_MoveRoot		24
#define ERR_MoveDesktop		25
#define ERR_MoveWastebasket	26
#define ERR_OpenErr		27
#define ERR_CreateEntry		28

#endif /* _error_h */

