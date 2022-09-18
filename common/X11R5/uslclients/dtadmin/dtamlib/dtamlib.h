/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma	ident	"@(#)dtadmin:dtamlib/dtamlib.h	1.12"
#endif

#include <stdio.h>
#include <libDtI/DtI.h>
#include "owner.h"

/*
 *	old diagnose.h, translated to use values from DtI.h
 */
#define	UNDIAGNOSED	DTAM_UNDIAGNOSED
#define	S5_FILES	DTAM_S5_FILES
#define	UFS_FILES	DTAM_UFS_FILES
#define	FS_TYPE		DTAM_FS_TYPE		
#define	BACKUP		DTAM_BACKUP	
#define	CPIO		DTAM_CPIO	
#define	CPIO_BINARY	DTAM_CPIO_BINARY
#define	CPIO_ODH_C	DTAM_CPIO_ODH_C
#define	TAR		DTAM_TAR	
#define	DOS_DISK	DTAM_DOS_DISK
#define	UNFORMATTED	DTAM_UNFORMATTED
#define	NO_DISK		DTAM_NO_DISK	
#define	UNREADABLE	DTAM_UNREADABLE
#define	BAD_ARGUMENT	DTAM_BAD_ARGUMENT
#define	BAD_DEVICE	DTAM_BAD_DEVICE
#define	DEV_BUSY	DTAM_DEV_BUSY
#define	UNKNOWN		DTAM_UNKNOWN	

#define	UNMOUNTED	DTAM_UNMOUNTED
#define	PACKAGE		DTAM_PACKAGE	
#define	MOUNTED		DTAM_MOUNTED	
#define MIS_MOUNT	DTAM_MIS_MOUNT
#define	CANT_MOUNT	DTAM_CANT_MOUNT
#define	CANT_OPEN	DTAM_CANT_OPEN
#define	NOT_OWNER	DTAM_NOT_OWNER
#define	NO_ROOM		DTAM_NO_ROOM	

/*
 *	old devtab.h, translated to use values from DtI.h
 */
#define	FIRST	DTAM_FIRST
#define	NEXT	DTAM_NEXT

extern	char	ALIAS[];
extern	char	DTALIAS[];
extern	char	DESC[];
extern	char	DTDESC[];
extern	char	BDEVICE[];
extern	char	CDEVICE[];
extern	char	CTAPE1[];
extern	char	DISKETTE[];

#define	getdev		DtamGetDev
#define	devattr		DtamDevAttr
#define	devalias	DtamDevAlias
#define	devdesc		DtamDevDesc
#define	MapAlias	DtamMapAlias
#define	CheckMedia	DtamCheckMedia

/*
 *	the rest remains honestly within dtamlib
 */
extern	Boolean	_DtamIsOwner(
			char	*	/* name of a cmd in adminuser entry */
		);

extern	void	_DtamNoteTmpFile(
			char *		/* name of a temporary file to */
		);			/* be deleted at program exit. */

extern	void	_DtamUnlink();

extern	char *	_DtamGetline(
			FILE*		/* stdout or stderr */,
			Boolean		/* lineflag */
		);

extern	Boolean _DtamSetbuf(
			FILE *		/* [stdin], stdout or stderr */,
			int		/* timeout (default 100 msec) */,
			Boolean		/* flag -- write through */
		);

extern	Boolean	_DtamBackground(
			char *		/* cmd -- string to be executed */
		);

extern	void	_DtamWMProtocols(
			Widget		/* toplevel application shell */
		);

extern	int	_Dtam_p3open(
			char *		/* cmd -- string to be executed */,
			FILE **		/* pair of fd's for the pipe	*/,
			int		/* flag to push ptem module	*/
		);

extern	int	_Dtam_p3close(
			FILE **		/* pair of fd's for the pipe	*/,
			int		/* signal to be sent to child	*/
		);

extern	int	_Dtam_p3kill(
			FILE **		/* pair of fd's for the pipe	*/,
			int		/* signal to be sent to child	*/
		);
