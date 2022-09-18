/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/delete.c	1.12"
#endif

/*
 * Module:	dtadmin:nfs   Graphical Administration of Network File Sharing
 * File:	delete.c  delete table entries
 */

#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <errno.h>
#include <sys/mnttab.h>
#include <sys/vfstab.h>

#include <X11/Intrinsic.h>
#include <DtI.h>
#include <X11/StringDefs.h>

#include <Gizmo/Gizmos.h>
#include <Gizmo/BaseWGizmo.h>
#include <Gizmo/MenuGizmo.h>

#include "nfs.h"
#include "text.h"

static Boolean vfstabDelete();

extern NFSWindow *MainWindow, *nfsw;
extern void DelObjectFromContainer();
extern void FreeObjectList();
extern int  UnMountIt();
extern Boolean isMounted();
extern void    alignIcons();

extern void
DeleteRemoteCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DeleteFlags flags = (DeleteFlags) call_data;
    struct vfstab * vp;
    Dimension       width;
    DmObjectPtr     op;
    DmItemPtr	    ip;
    int		    nitems;
    int		    i;
    ObjectListPtr   prev, slp = nfsw-> remoteSelectList;

    DEBUG0(">>DeleteRemoteCB entry\n");

    RETURN_IF_NULL(slp, TXT_SelectFolderDel);
    XtVaGetValues(nfsw->iconbox, XtNnumItems, &nitems, 0);

    for (prev = NULL; slp; prev = slp, slp = slp-> next)
    {	 
	DEBUG0(">>slp loop\n");
	if ((op = slp-> op) == NULL)
	{
	    SetMessage(MainWindow, TXT_BadOp, Base);
	    continue;
	}
	if ((vp = (struct vfstab *)op-> objectdata) == NULL ||
	    vp-> vfs_special == NULL || vp-> vfs_mountp == NULL)
	{
	    SetMessage(MainWindow, TXT_BadVp, Base);
	    continue;
	}

	if ((flags == ReDoIt_Confirm || flags == ReDoIt_Silent) &&
	    isMounted(vp-> vfs_mountp, vp-> vfs_special)) 
	{
	    if (UnMountIt(vp, op) != 0)	/* unmount failed */
		continue;
	}
	/* FIX: save a copy of VFSTAB to restore if DTVFSTAB update */
	/* fails */

	if (vfstabDelete(VFSTAB, vp) == FAILURE)
	    continue;
	if (vfstabDelete(DTVFSTAB,   vp) == FAILURE)
	    continue;

	for (i=0, ip = nfsw-> itp; i < nitems; i++, ip++)
	{
	    DEBUG0(">>itp loop\n");
	    if(ITEM_MANAGED(ip) == False)
		continue;
	    if((DmObjectPtr)OBJECT_PTR(ip) == op)
	    {			/* found it */
		DEBUG1(">>found item number %d\n", i);
		OlVaFlatSetValues(nfsw-> iconbox, (int)(ip - nfsw-> itp),
				  XtNmanaged, False,
				  XtNselect, False,
				  0
				  );
		ip-> select = False;
		DelObjectFromContainer(op, nfsRemote);
		DeleteFromObjectList(slp, prev);
		if (flags == ReDoIt_Confirm)
		    SetMessage(MainWindow, TXT_DeleteSucceeded, Base);
		break;
	    }
	}
    }
    unselectAll();
    XtVaGetValues(GetBaseWindowScroller(nfsw-> baseWindow), XtNwidth,
                  &width, 0);
    alignIcons(width);
    DEBUG0(">>DeleteRemoteCB exit\n");
    return;
}

/* TMPDIR must be in the same filesystem as VFSTAB and DTVFSTAB */
#define TMPDIR "/etc/dfs"
#define PREFIX ".dtr"

static Boolean 
vfstabDelete(char * vfstable, struct vfstab *vp)
{
    struct vfstab next;
    FILE 	*vfp, *tfp;
    char *      tname, *errorText;
    int         status;

    DEBUG1("--vfstabDelete %s\n", vfstable);
    /* FIX: do we need to lock the file? */
    RETURN_VALUE_IF_NULL((vfp = fopen(vfstable, "r")),
			 TXT_vfstabOpenError, FAILURE);
    if ((tname = tempnam(TMPDIR, PREFIX)) == NULL ||
	(tfp   = fopen(tname, "w"))       == NULL)
    {
	fclose(vfp);
	if (tname) free(tname);
	SetMessage(MainWindow, TXT_RemoteDeleteFailed, Base);
	errorText = GetGizmoText(TXT_tmpfileOpenError);
	fprintf(stderr, "%s\n", errorText);
	return FAILURE;
    }
    DEBUG1("--tempname = %s\n", tname);

    /* copy all entries except the one we're deleting into the tmpfile */
    while ((status = getvfsent(vfp, &next)) != -1)
    {
	if ( status != 0)
	{
	    errorText = GetGizmoText(TXT_Bad_vfstabEntry);
	    (void)fprintf(stderr, "%s %d\n", errorText, status);
	    continue;
	}
	/* FIX: compare label if dtvfstab */
	if ((strcmp(next.vfs_fstype,  vp->vfs_fstype)  != 0)	||
	    (strcmp(next.vfs_special, vp->vfs_special) != 0)	||
	    (strcmp(next.vfs_mountp,  vp->vfs_mountp)  != 0)	||
	    (strcmp(next.vfs_automnt, vp->vfs_automnt) != 0)	||
	    (strcmp(next.vfs_mntopts, vp->vfs_mntopts) != 0))
	{
	    putvfsent(tfp, &next);
	}
	else
	    DEBUG1("++Matched %s\n", next.vfs_special);  
    }
    /* replace the old table with the new. */
    fclose(vfp);
    fclose(tfp);
    if (rename(tname, vfstable) != 0)
    {
	SetMessage(MainWindow, TXT_RemoteDeleteFailed, Base);
	errorText = GetGizmoText(TXT_RenameError);
	fprintf(stderr, errorText, tname, vfstable, errno);
	return FAILURE;
    }
    DEBUG0("--Renamed file\n");
    if (tname)
	free(tname);
    return SUCCESS;
}
