/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:d_cache.c	1.36"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <X11/Intrinsic.h>
#include <memutil.h>
#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

extern void Dm__SetDfltFileClass(DmObjectPtr op, struct stat *buf,
						 struct stat *lbuf);
extern void DmInitObj(DmObjectPtr, struct stat *, DtAttrs);
static void Dm__FlushDir(DmContainerPtr cp);

/****************************procedure*header*****************************
    Dm__StampContainer-
*/
void
Dm__StampContainer(DmContainerRec * container)
{
    struct stat buf;

    container->time_stamp =
	(stat((const char *)container->path, &buf) == 0) ? buf.st_ctime : 0;
}

void
DmSetFileClass(op)
DmObjectPtr op;
{
	char *p;
	struct stat buf;
	struct stat lbuf;

	p = DmObjPath(op);
	(void)lstat(p, &lbuf);

	/* Set default file class based on stat(2) info */
	if (stat(p, &buf) == -1)
		Dm__SetDfltFileClass(op, NULL, &lbuf);
	else {
		Dm__SetDfltFileClass(op, &buf, &lbuf);

		/* Set file class based on user's file class database */
		Dm__SetFileClass(op);
	}
}

DmContainerPtr
Dm__NewContainer(path)
char *path;
{
	register DmContainerPtr cp;

	if (!path) 
		return(NULL);

	if ((cp = (DmContainerPtr)CALLOC(1, sizeof(DmContainerRec))) == NULL) {
		Dm__VaPrintMsg(TXT_MEM_ERR);
		return(NULL);
	}

	cp->path = strdup(path);
	cp->count = 1;
	return(cp);
}

/*
 * Free container structure.
 * Note: the object list is not freed!
 */
void
Dm__FreeContainer(cp)
DmContainerPtr cp;
{
	if (cp->path)
		free(cp->path);

	if (cp->plist.count)
		DtFreePropertyList(&(cp->plist));

	if (cp->data)
		free(cp->data);

	free(cp);
}

void
Dm__FreeObject(op)
DmObjectPtr op;
{
	XtFree(op->name);
	XtFree(op->objectdata);
	if (op->fcp && (op->fcp->attrs & DM_B_FREE)) {
		DmReleasePixmap(DESKTOP_SCREEN(Desktop), op->fcp->glyph);
		DmReleasePixmap(DESKTOP_SCREEN(Desktop), op->fcp->cursor);
		free(op->fcp);
    	}

	DtFreePropertyList(&(op->plist));
	free(op);
}

/****************************procedure*header*****************************
    Dm__NewObject-
*/
DmObjectPtr
Dm__NewObject(DmContainerPtr cp, char * name)
{
    DmObjectPtr op;

    if ( (op = (DmObjectPtr)CALLOC(1, sizeof(DmObjectRec))) == NULL)
	return(NULL);

    if (name)
	op->name = strdup(name);

    /* ADD_TO_END on behalf of DmOpenDir... sorry */
    Dm__AddToObjList(cp, op, DM_B_ADD_TO_END);

    return(op);
}					/* end of Dm__NewObject */
 
/****************************procedure*header*****************************
    DmInitObj- for an object, initialize the file info and type it.
	If caller has stat_buf, use it; otherwise, stat it here.
*/
void
DmInitObj(DmObjectPtr obj, struct stat * caller_buf, DtAttrs options)
{
    char *		path = DmObjPath(obj);
    struct stat		local_buf;
    struct stat		lstat_buf;
    struct stat *	stat_buf;

    /* Stat the file if the caller has not.  Since each file is classified
       BEFORE the ".dtinfo" file is read, no instance properties should
       affect the file class that a file is in.
       
       If stat returns an error consider it a "hidden" file.  May be
       link which points to file that doesn't exist.
    */
    if (caller_buf == NULL)
    {
	if ( stat(path, &local_buf) == -1 )
	{
	    obj->attrs |= DM_B_HIDDEN;
	    return;
	}
	stat_buf = &local_buf;

    } else
    {
	stat_buf = caller_buf;
    }

    if (options & DM_B_INIT_FILEINFO)
    {
	DmFileInfoPtr f_info = (DmFileInfoPtr)MALLOC(sizeof(DmFileInfo));

	if (f_info != NULL)
	{
	    f_info->mode	= stat_buf->st_mode;
	    f_info->nlink	= stat_buf->st_nlink;
	    f_info->uid		= stat_buf->st_uid;
	    f_info->gid		= stat_buf->st_gid;
	    f_info->size	= stat_buf->st_size;
	    f_info->mtime	= stat_buf->st_mtime;
	    obj->objectdata	= (void *)f_info;
	}

	/* Provide symbolic link info as part of file info.  Requesting
	   SET_TYPE will also do this so don't do it twice.
	*/
	if (!(options & DM_B_SET_TYPE) && (lstat(path, &lstat_buf) == 0) &&
	    ((lstat_buf.st_mode & S_IFMT) == S_IFLNK))
	    obj->attrs |= DM_B_SYMLINK;
    }

    if (options & DM_B_SET_TYPE)
    {
	(void)lstat(path, &lstat_buf);

	/* set default file class based on stat(2) info */
	Dm__SetDfltFileClass(obj, stat_buf, &lstat_buf);

	/* Set file class based on user's file class database */
	Dm__SetFileClass(obj);
    }
}					/* end of DmInitObj */

/****************************procedure*header*****************************
    Dm__CreateObj-
*/
DmObjectPtr
Dm__CreateObj(DmContainerPtr cp, char * name, DtAttrs options)
{
    DmObjectPtr obj;

    if (( (obj = Dm__NewObject(cp, name)) != NULL ) &&
	(options & (DM_B_INIT_FILEINFO | DM_B_SET_TYPE)))
    {
	DmInitObj(obj, NULL, options);
    }

    return (obj);
}					/* end of Dm__CreateObj */

/****************************procedure*header*****************************
    Dm__ReadDir-
*/
int
Dm__ReadDir(DmContainerPtr cp, DtAttrs options)
{
    DIR *		dirptr;
    struct dirent *	direntptr;
    Boolean		dtinfo_found = False;
    int			status = 0;		/* success */

    errno = 0;

    if ( (dirptr = opendir(cp->path)) == NULL )
    {
	Dm__VaPrintMsg(TXT_OPENDIR, errno, cp->path);
	return(-1);
    }
	
    if (options & DM_B_SET_TYPE)
	cp->attrs |= DM_B_INITED;

    while ( (direntptr = readdir(dirptr)) != NULL )
    {
	register char * name = direntptr->d_name;

	/* Check for "dot" files: if ".dtinfo" file found, remember this and
	   continue.  Never show "." or "..".  Show other dot files conditionally.
	*/
	if (name[0] == '.')
	{
	    if (!strcmp(name, DM_INFO_FILENAME))
	    {
		dtinfo_found = True;
		continue;
	    }

	    if (IS_DOT_DOT(name) || !SHOW_HIDDEN_FILES(Desktop))
		continue;
	}

	if ( Dm__CreateObj(cp, name, options) == NULL )
	{
	    status = -1;
	    break;
	}
    }

    /*
     * Must ignore return code from closedir(), because some file system,
     * like /dev/fd, may not support the close() system call, even though
     * everything else works.
     */
    (void)closedir(dirptr);

    if ((status == -1) || !(options & DM_B_READ_DTINFO) || !dtinfo_found ||
	(DmReadDtInfo(cp, DmMakePath(cp->path, DM_INFO_FILENAME),
		      INTERSECT) == -1))
	cp->attrs |= DM_B_NO_INFO;

    return(status);
}				/* end of Dm__ReadDir */

DmContainerPtr
DmOpenDir(char * path, DtAttrs options)
{
    DmContainerPtr	cp = NULL;
    char *		real_path = realpath(path, Dm__buffer);
    int			len;

    errno = 0;
    if ((real_path = realpath(path, Dm__buffer)) == NULL)
    {
	fprintf(stderr,
		"realpath(3C) error on '%s' (offending file=%s, errno=%d)\n",
		path, Dm__buffer, errno);
	return(NULL);
    }

    len = strlen(real_path) + 1;

    if ((cp = DtGetData(NULL, DM_CACHE_FOLDER, (void *)real_path, len)) != NULL)
    {
	if (!(cp->attrs & DM_B_INITED) && (options & DM_B_SET_TYPE))
	{
	    DmObjectPtr op;

	    for (op = cp->op; op; op = op->next)
		DmSetFileClass(op);
	}

	cp->count++;		/* bump usage count */

	/* Do a quick check to see if search permission on path is allowed. */
    } else if (!access(real_path, X_OK| F_OK) &&
	       ((cp = Dm__NewContainer(real_path)) != NULL))
    {
	Dm__ReadDir(cp, DM_B_READ_DTINFO | options);
	DtPutData(NULL, DM_CACHE_FOLDER, cp->path, len, cp);

	if (options & DM_B_TIME_STAMP)
	    Dm__StampContainer(cp);
    }

    return(cp);
}				/* End of DmOpenDir */

int
DmCloseDir(path, options)
char *path;
DtAttrs options;
{
	DmContainerPtr cp;

	if (cp = DtGetData(NULL, DM_CACHE_FOLDER, (void *)path, strlen(path)+1))
		return(DmCloseContainer(cp, options));
	return(1);
}

int
DmCloseContainer(cp, options)
DmContainerPtr cp;
DtAttrs options;
{
	if (!(options & DM_B_NO_FLUSH))
		/* remember the fact that someone want to flush the data */
		cp->attrs |= DM_B_FLUSH_DATA;

	if (--(cp->count) == 0) {
		register DmObjectPtr op;
		DmObjectPtr save;

		DtDelData(NULL, DM_CACHE_FOLDER, (void *)(cp->path),
			 strlen(cp->path)+1);
		if (cp->attrs & DM_B_FLUSH_DATA)
			DmFlushContainer(cp);
		for (op=cp->op; op;) {
			save = op->next;
			Dm__FreeObject(op);
			op = save;
		}
		Dm__FreeContainer(cp);
	}
	return(0);
}

int
DmFlushContainer(cp)
DmContainerPtr cp;
{

	static char *wb_dtinfo = NULL;

	if (cp == DESKTOP_HELP_DESK(Desktop)->cp)
		DmWriteDtInfo(cp, DESKTOP_HELP_DESK(Desktop)->cp->path, 0);
	else if (cp == DESKTOP_WB_WIN(Desktop)->cp) {
		if (wb_dtinfo == NULL) {
			char buf[PATH_MAX];

			sprintf(buf, "%s/.Wastebasket",
				(char *)DmGetDTProperty(WBDIR, NULL));
			wb_dtinfo = strdup(buf);
		}
		DmWriteDtInfo(cp, wb_dtinfo, 0);
	} else
		DmWriteDtInfo(cp, DmMakePath(cp->path, DM_INFO_FILENAME), 0);
	return(0);
}

int
DmFlushDir(path)
char *path;
{
	DmContainerPtr cp;

	if (cp = DtGetData(NULL, DM_CACHE_FOLDER, (void *)path, strlen(path)+1))
		return(DmFlushContainer(cp));
	return(1);
}

void
DmDelObjectFromContainer(cp, target_op)
DmContainerPtr cp;
DmObjectPtr target_op;
{
	register DmObjectPtr op = cp->op;

	if (op == target_op) {
		cp->op = op->next;
		cp->num_objs--;
	}
	else
		for (; op->next; op=op->next)
			if (op->next == target_op) {
				op->next = target_op->next;
				cp->num_objs--;
				return;
			}
}

DmObjectPtr
DmGetObjectInContainer(cp, name)
DmContainerPtr cp;
char *name;
{
	register DmObjectPtr op;

	for (op=cp->op; op; op=op->next)
		if (!strcmp(op->name, name))
			return(op);
	return(NULL);
}

DmObjectPtr
DmDupObject(op)
DmObjectPtr op;
{
	DmObjectPtr new_op;

	if (new_op = (DmObjectPtr)MALLOC(sizeof(DmObjectRec))) {
		*new_op = *op;
		new_op->next		= NULL;
		new_op->objectdata	= NULL; /* not copied */
		if (op->name)
			new_op->name = strdup(op->name);
		(void)DtCopyPropertyList(&(new_op->plist), &(op->plist));
	}

	return(new_op);
}
