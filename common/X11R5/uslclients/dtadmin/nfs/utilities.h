/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/utilities.h	1.4"
#endif

/*
 * Module:      dtadmin: nfs  Graphical Administration of Network File Sharing
 * File:        utilities.h: utility prototypes.
 *
 */

extern Boolean	AcceptFocus();
extern XtArgVal	GetValue();
extern void	SetValue();
extern struct vfstab * Copy_vfstab();
extern void	Free_vfstab();
extern void	NoMemoryExit();
extern void	FreeObjectList();
extern void	setInitialValue();
extern void	setPreviousValue();
extern void	setChoicePreviousValue();
extern void	free_dfstab();
extern int	direq();
extern Boolean	sharecmp();
extern void	UpdateList();
extern void	MoveFocus();
extern void	SetMessage();
extern void	DeleteFromObjectList();
