/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_FSID_H	/* wrapper symbol for kernel use */
#define _FS_FSID_H	/* subject to change without notice */

#ident	"@(#)uts-x86:fs/fsid.h	1.2"
#ident	"$Header: $"

/* Fstyp names for use in fsinfo structure. These names */
/* must be constant across releases and will be used by a */
/* user level routine to map fstyp to fstyp index as used */
/* ip->i_fstyp. This is necessary for programs like mount. */

#define S51K	"S51K"
#define PROC	"PROC"
#define DUFST	"DUFST"
#define NFS		"NFS"
#define S52K	"S52K"
#define XENIX	"XENIX"

#endif	/* _FS_FSID_H */
