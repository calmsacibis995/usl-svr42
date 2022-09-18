/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:f_sync.c	1.31"

/******************************file*header********************************

    Description:
	This file contains the source code for the "sync" timer
*/
						/* #includes go here	*/
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>
#include <Xol/array.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

typedef struct _FileInfo {
    Boolean	found;
    DmObjectPtr	obj;
} FileInfo;

typedef _OlArrayStruct(FileInfo, _FileArray) FileArray;
typedef _OlArrayStruct(DmFolderWinRec *, _StaleList) StaleList;

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Public  Procedures 
*/
					/* private procedures		*/
static int	CmpNames(const void *, const void *);
static FileArray*GetFiles(char * path);
static Boolean	SyncProc(XtPointer client_data);
static void	SyncFolder(DmFolderWindow folder);

					/* public procedures		*/
void		Dm__RmFromStaleList(DmFolderWindow);
void		Dm__SyncFolder(DmFolderWindow folder);
void		Dm__SyncTimer(XtPointer client_data, XtIntervalId * timer_id);

/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
    CmpNames-
*/
static int
CmpNames(const void * f1, const void * f2)
{
    FileInfo * file1 = (FileInfo *)f1;
    FileInfo * file2 = (FileInfo *)f2;

    return(strcmp((const char *)file1->obj->name,
		  (const char *)file2->obj->name));

}					/* End of CmpNames */

/****************************procedure*header*****************************
    GetFiles- this is equivalent to DmOpenDir except that a "dummy" container
	is used.
*/
static FileArray *
GetFiles(char * path)
{
    static FileArray *	files;		/* Freed when dtm exits */
    DmContainerRec	container;
    DmObjectPtr		obj;
    FileInfo		new_file;

    container.next	= NULL;
    container.path	= path;
    container.count	= 0;		/* probably don't care */
    container.op	= NULL;
    container.num_objs	= 0;
    container.attrs	= 0;

    /* Use Dm__ReadDir to get a list of objs */
    (void)Dm__ReadDir(&container, DM_B_SET_TYPE | DM_B_INIT_FILEINFO);

    /* Initialize (re-use) files buffer.  Freed when dtm exits */
    if (files == NULL)
	_OlArrayAllocate(_FileArray, files,
			 container.num_objs, _OlArrayDefaultStep);
    else
	_OlArraySize(files) = 0;

    /* Now run thru all objs and create FileInfo element for each */
    new_file.found = False;
    for (obj = container.op; obj != NULL; obj = obj->next)
    {
	new_file.obj = obj;
	_OlArrayAppend(files, new_file);
    }

    return(files);
}					/* End of GetFiles */

/****************************procedure*header*****************************
    SyncFolder- update "stale" folder pointed to by 'folder'
*/
static void
SyncFolder(DmFolderWindow folder)
{
    char *	path = DM_WIN_PATH(folder);
    FileArray *	files = GetFiles(path);
    FileInfo *	file;
    DmObjectPtr obj;
    int		result;
    Dimension	pane_width = 0;
    int		sync_ed = 0;
#define FINFO(obj)	( (DmFileInfoPtr)((obj)->objectdata) )

    /* Stamp container now (as early as possible) */
    Dm__StampContainer(folder->cp);

    /* Sort files by name */
    qsort(files->array, (size_t)_OlArraySize(files),
	  (size_t)_OlArrayElementSize(files), CmpNames);

    /* Get the width of the view once outside of the loop */
    XtVaGetValues(folder->box, XtNwidth, &pane_width, NULL);

    /* First, look for deletions.  If there's an object in the folder which
       is not in the file list, delete it.  Otherwise, mark the file in the
       list as found.
    */
    for (obj = folder->cp->op; obj != NULL; obj = obj->next)
    {
	Boolean		found = False;
	DmItemPtr	item;

        for (file = files->array;
             file < files->array + _OlArraySize(files); file++)
        {
            if (file->found ||          /* already accounted for */
                ((result = strcmp(file->obj->name, obj->name)) < 0))
                continue;

            /* File found or file won't be found (too far down the list) */
            if (result == 0)
                file->found = found = True;

            break;
        }

        if (found)	/* If found, make sure item has latest "stats" */
	{
	    if (FINFO(obj) && FINFO(file->obj)) {
	    	if (FINFO(obj)->mtime < FINFO(file->obj)->mtime)
	    	{
			*FINFO(obj) = *FINFO(file->obj);
			sync_ed = -1;		/* ie, modification */
	    	}
	    }
	    else {
		if (!(FINFO(obj)) && FINFO(file->obj)) {
			struct stat		lstat_buf;
			char *		path = DmObjPath(obj);
			DmFileInfoPtr f_info = 
				(DmFileInfoPtr)MALLOC(sizeof(DmFileInfo));
			obj->objectdata  = (void *)f_info;
			*FINFO(obj) = *FINFO(file->obj);
			obj->ftype  = file->obj->ftype;
			obj->fcp    = file->obj->fcp;

			obj->attrs = file->obj->attrs;
	    		(void)DmAddObjToFolder(folder, obj, 
				UNSPECIFIED_POS, UNSPECIFIED_POS, pane_width);

			sync_ed	    = -1;
		
		}
	    }
	} else		/* else, remove item or object */
	{
	    sync_ed = 1;		/* ie., addition or delete */

            if ((item = DmObjectToItem((DmWinPtr)folder, obj)) != NULL)
                DmRmItemFromFolder(folder, item);
            else 
                DmDelObjectFromContainer(folder->cp, obj);
	}
    }

    /* Now look for files in file array that were not found in the items list
       and add them to the folder.
    */

    for (file = files->array; file < files->array + _OlArraySize(files); file++)
	if (file->found)
	{
	    Dm__FreeObject(file->obj);
	} else
	{
	    sync_ed = 1;		/* ie., addition or delete */

	    (void)DmAddObjToFolder(folder, file->obj, UNSPECIFIED_POS,
				   UNSPECIFIED_POS, pane_width);
	}

    if (sync_ed)		/* ie, addition, deletion, or mod */
    {
	/* If not ICONIC, keep new item(s) in sorted order */
	if (folder->view_type != DM_ICONIC)
	    DmSortItems(folder, folder->sort_type, 0, 0, pane_width);

	if (sync_ed == 1){	/* ie, addition or deletion */
	    DmDisplayStatus((DmWinPtr)folder);
	}
    }

#undef FINFO
}					/* end of SyncFolder */

/****************************procedure*header*****************************
    SyncProc- called by Dm__SyncTimer to update a "stale" folder.  The sync
    timer builds a list of indexes to stale folders and calls registers this
    work proc.  This work proc updates the "current" stale folders and, if
    there are more stale folders to update, returns "False" so that we're
    called again for the next stale folder.  If there are no more stale
    folders to update, the sync timer is re-activated and this work proc is
    not re-registered (return True).
*/
static Boolean
SyncProc(XtPointer client_data)
{
    DmDesktopPtr	desktop = (DmDesktopPtr)client_data;
    u_short		current = STALE_INDEX(desktop);
    StaleList *	stale_folders = (StaleList *)STALE_FOLDERS(desktop);

    SyncFolder(_OlArrayElement(stale_folders, current));

    if (current == _OlArraySize(stale_folders) - 1)
    {
	Dm__AddSyncTimer(desktop);
	return(True);

    } else
    {
	STALE_INDEX(desktop)++;
	return(False);
    }
}				/* End of SyncProc */

/***************************public*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
    Dm__RmFromStaleList-
*/
void
Dm__RmFromStaleList(DmFolderWindow folder)
{
    StaleList *	folders = (StaleList *)STALE_FOLDERS(Desktop);
    int		indx = _OlArrayFind(folders, folder);

    if (!_OL_ARRAY_IS_EMPTY(folders) && (indx != _OL_NULL_ARRAY_INDEX))
	_OlArrayDelete(folders, indx);
}					/* end of Dm__RmFromStaleList */

/****************************procedure*header*****************************
    Dm__SyncFolder- update "stale" folder pointed to by 'folder'

	This is the "public" routine which first checks to see if the folder
	is really stale.  If so, call SyncFolder which does the real work.
*/
void
Dm__SyncFolder(DmFolderWindow folder)
{
    struct stat		buf;

    if (stat((const char *)DM_WIN_PATH(folder), &buf) != 0)
    {
	Dm__VaPrintMsg(TXT_STAT, errno, DM_WIN_PATH(folder));
	return;
    }
	
    if (folder->cp->time_stamp != buf.st_ctime)
	SyncFolder(folder);
}

/****************************procedure*header*****************************
    Dm__SyncTimer- this timer is called to keep "Desktop" in sync with
	changes on disk which occur outside of the desktop.

	This timer merely builds a list of open folder which need updating.
	If any "stale" folders are found, it registers a "background" Xt work
	proc to do the actual updates.  If none are found, the timer resets
	itself to try again later.

	The timer interval is user-settable.

	If there is a "file-op" work proc registered (ie., there is Desktop
	file activity in progress), we return right away (after resetting the
	timer).
*/
void
Dm__SyncTimer(XtPointer client_data, XtIntervalId * timer_id)
{
    DmDesktopPtr	desktop = (DmDesktopPtr)client_data;
    DmFolderWindow	folders;
    DmFolderWindow	folder;
    StaleList *		stale_folders;
    Boolean		found;
    struct stat		buf;

    /* Return now if no folders (exiting) */
    if ( (folders = DESKTOP_FOLDERS(desktop)) == NULL)
	return;

    /* Point to list of open folders and list of stale folders.  Init size of
       stale folder list to '0': each time the timer expires, we start a new
       list of folders that might need updates.
    */
    stale_folders		= (StaleList *)STALE_FOLDERS(desktop);
    _OlArraySize(stale_folders)	= 0;
    found			= False;

    for (folder = folders; folder != NULL; )
    {
	if (folder->attrs & DM_B_FILE_OP_BUSY) {
	    folder = folder->next;
	    continue;		/* busy with file operation */
	}

	if (stat((const char *)DM_WIN_PATH(folder), &buf) != 0) {
	    DmFolderWindow save_folder = folder->next;

	    Dm__VaPrintMsg(TXT_STAT, errno, DM_WIN_PATH(folder));
	    DmCloseFolderWindow(folder);

	    /*
	     * Can't do folder = folder->next here, because folder is
	     * already free in DmCloseFolderWindow.
	     */
	    folder = save_folder;
	    continue;
	}
	
	if (folder->cp->time_stamp != buf.st_ctime)
	{
	    _OlArrayAppend(stale_folders, folder);
	    found = True;
	}

	folder = folder->next;
    }

    /* If there are stale folders, register work proc to update them.
       Otherwise, re-activate this timer to check again later.
    */
    if (found)
    {
	STALE_INDEX(desktop) = 0;
	XtAddWorkProc(SyncProc, (XtPointer)desktop);

    } else
    {
	Dm__AddSyncTimer(desktop);
    }
}				/* End of Dm__SyncTimer */
