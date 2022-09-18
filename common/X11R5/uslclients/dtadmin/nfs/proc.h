/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/proc.h	1.3"
#endif

/*
 * Module:	dtadmin:nfs  Graphical Administration of Network File Sharing
 * File:	proc.h       header for AddInputProc client_data structure
 *
 */

typedef struct _inputProcData
{
    DmObjectPtr op;		/* used only by mount */
    Cardinal 	index;		/* icon index; used only by mount */
    FILE       *fp[2];
    XtInputId	readId;
    XtInputId 	exceptId;
    pid_t	pid;
} inputProcData;

#define BAD_PID ((pid_t) -1)
