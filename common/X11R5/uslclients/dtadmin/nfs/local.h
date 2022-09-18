/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/local.h	1.5"
#endif

/*
 * Module:	dtadmin:nfs  Graphical Administration of Network File Sharing
 * File:	local.h      header for local resources
 *
 */

#define SHARETAB    "/etc/dfs/sharetab"
#define SHARECMD    "/usr/sbin/share -F nfs"
#define SHARECMDLEN 22
#define UNSHARECMD    "/usr/sbin/unshare -F nfs"
#define QUOTEWHITE  "\"\' \t\n"
#define QUOTE	    "\"\'"

extern void 		writedfs();
extern void 		free_dfstab();
extern struct share    *sharedup();
extern Boolean 		sharecmp();

typedef struct _dfstab
{
    struct share *sharep;
    Boolean autoShare;
} dfstab;

typedef enum _dfstabEntryType { NFS, RFS, Mystery, NoMore} dfstabEntryType;
