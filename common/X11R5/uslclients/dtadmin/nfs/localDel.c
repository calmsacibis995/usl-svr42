/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/localDel.c	1.10"
#endif

/*
 * Module:	dtadmin:nfs   Graphical Administration of Network File Sharing
 * File:	localdelete.c  delete dfstab table entries
 */

#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <errno.h>

#include <X11/Intrinsic.h>
#include <DtI.h>
#include <X11/StringDefs.h>

#include <Gizmo/Gizmos.h>
#include <Gizmo/BaseWGizmo.h>
#include <Gizmo/MenuGizmo.h>

#include "nfs.h"
#include "text.h"
#include "sharetab.h"
#include "local.h"

static Boolean dfstabDelete();

extern NFSWindow *MainWindow, *nfsw;
extern void       DelObjectFromContainer();
extern dfstabEntryType  Get_dfstab_entry();
extern void       FreeObjectList();
extern int        UnShareIt();
extern Boolean    isShared();
extern void       alignIcons();

extern void
DeleteLocalCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DeleteFlags flags = (DeleteFlags) call_data;
    dfstab       * dfsp;
    struct share * sharep;
    Dimension      width;
    DmObjectPtr    op;
    DmItemPtr	   ip;
    int		   nitems;
    int		   i;
    ObjectListPtr  prev, slp = nfsw-> localSelectList;

    DEBUG0(">>DeleteLocal entry\n");

    RETURN_IF_NULL(slp, TXT_SelectFolderDel);
    XtVaGetValues(nfsw->iconbox, XtNnumItems, &nitems, 0);
    XtVaGetValues(GetBaseWindowScroller(nfsw-> baseWindow), XtNwidth,
                  &width, 0);

    for (prev = NULL; slp; prev = slp, slp = slp-> next)
    {	 
	DEBUG0(">>slp loop\n");
	if ((op = slp-> op) == NULL)
	{
	    SetMessage(MainWindow, TXT_BadOp, Base);
	    continue;
	}
	if ((dfsp = (dfstab *)op-> objectdata) == NULL ||
	    (sharep = dfsp-> sharep) == NULL || sharep-> sh_path == NULL)
	{
	    SetMessage(MainWindow, TXT_BadDfs, Base);
	    continue;
	}

	if ((flags == ReDoIt_Confirm || flags == ReDoIt_Silent) &&
	    isShared(sharep)) 
	{
	    if (UnShareIt(sharep, op) != 0)	/* unshare failed */
		continue;
	}

	if (dfstabDelete(dfsp) == FAILURE)
	    continue;

	for (i=0, ip = nfsw-> itp; i < nitems; i++, ip++)
	{
	    DEBUG0(">>itp loop\n");
	    if(ITEM_MANAGED(ip) == False)
		continue;
	    if((DmObjectPtr)OBJECT_PTR(ip) == op)
	    {			/* found it */
		DEBUG1(">>found item number %d\n", i);
		OlVaFlatSetValues(nfsw-> iconbox, i,
				  XtNmanaged, False,
				  XtNselect, False,
				  0
				  );
		DelObjectFromContainer(op, nfsLocal);
                DeleteFromObjectList(slp, prev);
		if (flags == ReDoIt_Confirm) 
		    SetMessage(MainWindow, TXT_DeleteSucceeded, Base);
		break;
	    }
	}
    }
    unselectAll();
    alignIcons(width);
    DEBUG0(">>DeleteLocalCB exit\n");
    return;
}

/* TMPDIR must be in the same filesystem as VFSTAB and DTVFSTAB */
#define TMPDIR "/etc/dfs"
#define PREFIX ".dtl"

static Boolean 
dfstabDelete(dfstab *dfsp)
{
    dfstab       *next;
    FILE         *dfp,    *tfp;
    struct share *sharep = dfsp-> sharep;
    struct share *nsharep;
    char         *tname,  *errorText;
    char          buf[BUFSIZ];
    dfstabEntryType type;

    /* FIX: do we need to lock the file? */
    RETURN_VALUE_IF_NULL((dfp = fopen(DFSTAB, "r")),
			 TXT_dfstabOpenError, FAILURE);
    if ((tname = tempnam(TMPDIR, PREFIX)) == NULL ||
	(tfp   = fopen(tname, "w"))       == NULL)
    {
	fclose(dfp);
	if (tname) free(tname);
	SetMessage(MainWindow, TXT_LocalDeleteFailed, Base);
	errorText = GetGizmoText(TXT_tmpfileOpenError);
	fprintf(stderr, "%s\n", errorText);
	return FAILURE;
    }
    DEBUG1("--tempname = %s\n", tname);

    /* copy all entries except the one we're deleting into the tmpfile */
    while ((type = Get_dfstab_entry(dfp, &next, buf)) != NoMore)
    {
	if (type != NFS)
	{
	    fputs(buf, tfp);
	    continue;
	}
	nsharep = next-> sharep;
	if ((dfsp-> autoShare != next-> autoShare)		||
	    (strcmp(nsharep-> sh_path,  sharep-> sh_path))	||
	    (strcmp(nsharep-> sh_res,   sharep-> sh_res))	||
	    (strcmp(nsharep-> sh_opts,  sharep-> sh_opts))	||
	    (strcmp(nsharep-> sh_descr, sharep-> sh_descr)))
	    
	{
 	    fputs(buf, tfp);
	}
	else
	    DEBUG1("++Matched %s\n", nsharep-> sh_res);  
	free_dfstab(next);
    }
    /* replace the old table with the new. */
    fclose(dfp);
    fclose(tfp);
    if (rename(tname, DFSTAB) != 0)
    {
	SetMessage(MainWindow, TXT_LocalDeleteFailed, Base);
	errorText = GetGizmoText(TXT_RenameError);
	fprintf(stderr, errorText, tname, DFSTAB, errno);
	return FAILURE;
    }
    DEBUG0("--Renamed file\n");
    if (tname)
	free(tname);
    return SUCCESS;
}
