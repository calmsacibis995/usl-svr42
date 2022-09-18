/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:f_task.c	1.101"

/******************************file*header********************************

    Description:
	This file contains the source code for processing file-related "tasks".
*/
						/* #includes go here	*/
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <utime.h>
#include <sys/stat.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>

#include <Gizmos.h>
#include <MenuGizmo.h>
#include <ModalGizmo.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "error.h"
#include "extern.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Public  Procedures 
*/
					/* private procedures		*/
static void	BeginFileOp(DmTaskInfoListPtr tip);
static int	BuildSubtasksForDir(DmTaskInfoListPtr, DmFileOpType,
		    		char *, char *, DmTaskListPtr **q);
static Boolean	ExecuteTask(XtPointer);
static int	Dm__FileType(char * filename, struct stat *st);
static void	FreeSrcList(DmTaskInfoListPtr tip);
static void	FreeTasks(DmTaskListPtr current);
static DmTaskInfoListPtr GetTaskInfoItem(void);
static int	DmMakeDirectory(char * src, char * dst);
static void	MakeTask(DmFileOpType, char *, char *, DmTaskListPtr **);
static void	RegFileOpWorkProc(DmTaskInfoListPtr tip);
static DmTaskInfoListPtr ReturnError(DmTaskInfoListPtr, int, char * name);
static Boolean	ScheduleNextTask(void);

static int	CopyBlock(char *, char *, int *, int *, mode_t);
static int	DmCopyFile(char * src, char * dst, int * rfd, int * wfd);
static int	DmDeleteFile(char * src);
static int	DmLinkFile(char * src, char * dst);
static int	MakeLinkInstead(char * src, char * dst);
static int	DmMoveFile(char * src, char * dst);
static int	DmRmDir(char * src);
static int	DmSymLinkFile(char * src, char * dst);

					/* public procedures		*/
DmTaskInfoListPtr DmDoFileOp(DmFileOpInfoPtr, DmClientProc, XtPointer);
void		DmFreeTaskInfo(DmTaskInfoListPtr tip);
void		Dm__Overwrite(DmTaskInfoListPtr tip, Boolean overwrite);
int		DmUndoFileOp(DmTaskInfoListPtr tip);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

/* For subtasks of type DM_ENDOVER or DM_BEGIN, non-NULL source and targets
   are source indexes NOT pointers so don't free them.
*/
#define FreeTaskContents(task) \
    if (((task)->type != DM_ENDOVER) && ((task)->type != DM_BEGIN)) \
	XtFree((task)->source), XtFree((task)->target)

/* Operations in target table.
   These are defined in the order of "execution" (that is, OVER_WRITE must be
   considered, built (if necessary) and executed before MSRC_MKDIR, etc).
   This is done for documenting purposes only; the bit values are not
   important.

   OVER_WRITE: unconditionally generate OVERWRITE tasks and associated tasks
   MSRC_MKDIR: check for multiple srcs and make dir (target_path) if so.
   ADJUST_TARGET: target_path is dir (not necessarily existing dir) so src
	basenames must be appended to target_path to get real 'target'.
	(ADJUST_TARGET is also generated on-the-fly (outside of table))
*/
#define OVER_WRITE	(1 << 0)
#define MSRC_MKDIR	(1 << 1)
#define ADJUST_TARGET	(1 << 2)

static const unsigned short target_table[] = {
		 MSRC_MKDIR,			/* NO_FILE    */
				ADJUST_TARGET,	/* IS_DIR     */
    OVER_WRITE | MSRC_MKDIR,			/* IS_FILE    */
    OVER_WRITE | MSRC_MKDIR,			/* IS_SPECIAL */
};

/* Operations in opcode table
   The op defines are grouped to give a hint about order of "execution" (this
   is done for documenting purposes only; the bit values are not important):
	The 2nd group are the basic file operations.
	The 1st group occur before these.
	The 3rd group deal with recursively descending into a dir.

   CHK_FS: check for moving across file systems
   CHK_CP: check for invalid copies (ie, can't copy special files)
   CHK_SAME: check for same src & target
   CHK_DESC: check if target is same OR descendant of src
   CP_DEL_OP: copy-then-delete; for instance, to simulate moving across fs
   RECUR_*: recursively operate on src (ie, when src is dir)
*/
#define CHK_FS		(1 << 0)
#define CHK_CP		(1 << 1)
#define CHK_SAME	(1 << 2)
#define CHK_DESC	(1 << 3)

#define CP_OP		(1 << 4)
#define MV_OP		(1 << 5)
#define LN_OP		(1 << 6)	/* Symbolic and hard links */
#define DEL_OP		(1 << 7)
#define CP_DEL_OP	(1 << 8)
#define MV_DIR		(1 << 9)

#define MKDIR_OP	(1 << 10)
#define RECUR_CP	(1 << 11)
#define RECUR_DEL	(1 << 12)
#define RECUR_CP_DEL	(1 << 13)
#define RMDIR_OP	(1 << 14)

/* If ERROR_OP is set, the error code is in the rest of the LSBs */
#define ERROR_OP	(1 << 15)

#define NUM_SRC_TYPE	5	/* none, dir, file, special, symlink */
#define NUM_OP		6	/* cp, mv, del, hlink, slink, cp_del */

/* Use this macros to look up the opcode_table */
#define OPCODES(SRC_TYPE, OP_TYPE)	opcode_table[SRC_TYPE][OP_TYPE - 1]

static const unsigned short opcode_table[NUM_SRC_TYPE][NUM_OP] = {
/* SRC   OP				TASKS				*/
/* NONE	COPY   */				ERROR_OP | ERR_NoSrc,
/* NONE	MOVE   */				ERROR_OP | ERR_NoSrc,
/* NONE	DELETE */				ERROR_OP | ERR_NoSrc,
/* NONE	HLINK  */				ERROR_OP | ERR_NoSrc,
/* NONE	SLINK  */				ERROR_OP | ERR_NoSrc,
/* NONE	CP_DEL */				ERROR_OP | ERR_NoSrc,

/* DIR	COPY   */ CHK_DESC|	    MKDIR_OP|	RECUR_CP,
/* DIR	MOVE   */ CHK_DESC|CHK_FS|  MV_DIR,	/* see DmDoFileOp */
/* DIR	DELETE */				RECUR_DEL|  RMDIR_OP,
/* DIR	HLINK  */				ERROR_OP | ERR_NoDirHardlink,
/* DIR	SLINK  */ CHK_SAME|	    LN_OP,
/* DIR	CP_DEL */ CHK_DESC|	    MKDIR_OP|	RECUR_CP_DEL|  RMDIR_OP,

/* FILE	COPY   */ CHK_SAME|	    CP_OP,
/* FILE	MOVE   */ CHK_SAME|CHK_FS|  MV_OP,
/* FILE	DELETE */		    DEL_OP,
/* FILE	HLINK  */ CHK_SAME|	    LN_OP,
/* FILE	SLINK  */ CHK_SAME|	    LN_OP,
/* FILE	CP_DEL */ CHK_SAME|	    CP_DEL_OP,

/* SPCL	COPY   */				ERROR_OP | ERR_CopySpecial,
/* SPCL	MOVE   */ CHK_SAME|CHK_FS|CHK_CP| MV_OP,
/* SPCL	DELETE */		    DEL_OP,
/* SPCL	HLINK  */ CHK_SAME|	    LN_OP,
/* SPCL	SLINK  */ CHK_SAME|	    LN_OP,
/* SPCL	CP_DEL */				ERROR_OP | ERR_CopySpecial,

/* SYML	COPY   */ CHK_SAME|	    CP_OP,
/* SYML	MOVE   */ CHK_SAME|CHK_FS|  MV_OP,
/* SYML	DELETE */		    DEL_OP,
/* SYML	HLINK  */ CHK_SAME|	    LN_OP,
/* SYML	SLINK  */ CHK_SAME|	    LN_OP,
/* SYML	CP_DEL */ CHK_SAME|	    CP_DEL_OP,
};

/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
    BeginFileOp- common code to begin file op: call client_proc and register
	work proc (if needed).

	Caller may want to take action before operation is actually started
	(busying windows, etc).
*/
static void
BeginFileOp(DmTaskInfoListPtr tip)
{
    if (tip->opr_info->options & OPRBEGIN)
	(*tip->client_proc)(DM_OPRBEGIN, tip->client_data,
			    (XtPointer)tip, NULL, NULL);

    /* register work proc with Xt, if one is not already registered */
    RegFileOpWorkProc(tip);
}

/****************************procedure*header*****************************
    BuildSubtasksForDir-  This routine recursively descend into subdirs and
    generate subtasks for each file. The "type" should be either DM_COPY or
    DM_DELETE.  'srcname' and 'dstname' are assume MALLOC'ed strings, and
    will be freed at the bottom of the function. The return code is 0 for
    success or err_code in case of error.
*/
static int
BuildSubtasksForDir(DmTaskInfoListPtr tip, DmFileOpType type,
		    char * srcname, char * dstname, DmTaskListPtr ** q)
{
    int			ret = 0;
    DIR *		dp;
    struct dirent *	directory;
    char *		fullsrcpath;
    int			source_type;
    int			op_vec;

    /* Recursively operate on contents */
    if ( (dp = opendir(srcname)) == NULL )
    {
	ret = ERR_OpenDir;
	goto done;
    }

    fullsrcpath = NULL;		/* in case there is an empty directory */

    while ( (directory = readdir(dp)) != NULL )
    {
	/* don't do anything with "." or ".." entries */
	if (!strcmp(directory->d_name, ".") ||
	    !strcmp(directory->d_name, ".."))
	    continue;

	/* Make full src path */
	fullsrcpath = strdup(DmMakePath(srcname, directory->d_name));

	source_type = Dm__FileType(fullsrcpath, NULL);
	op_vec = OPCODES(source_type, type);

	if (op_vec & ERROR_OP)
	{
	    ret = op_vec & ~ERROR_OP;
	    break;
	}

	/* There is no need to do CHK_FS here? */
	op_vec &= ~CHK_FS;

	if (op_vec & MKDIR_OP)
	    MakeTask(DM_MKDIR, strdup(fullsrcpath),
		     strdup(DmMakePath(dstname,directory->d_name)),
		     q);

	if (op_vec & CHK_CP)
	    if (op_vec & (CP_OP | CP_DEL_OP))
	    {
		ret = ERR_CopySpecial;
		break;
	    }

	if (op_vec & (CP_OP | CP_DEL_OP))
	    MakeTask((op_vec & CP_OP) ? DM_COPY : DM_COPY_DEL,
		     strdup(fullsrcpath),
		     strdup(DmMakePath(dstname,directory->d_name)),
		     q);

	else if (op_vec & DEL_OP)
	    MakeTask(DM_DELETE, strdup(fullsrcpath), NULL, q);

	else if (op_vec & (RECUR_CP | RECUR_DEL | RECUR_CP_DEL))
	{
	    ret =
		BuildSubtasksForDir(tip,
				    (op_vec & RECUR_CP) ? DM_COPY :
				    (op_vec & RECUR_DEL) ? DM_DELETE :
				    DM_COPY_DEL,
				    strdup(fullsrcpath),
				    strdup(DmMakePath(dstname,
						      directory->d_name)),
				    q);

	    if (ret != 0)
		break;
	}

	if (op_vec & RMDIR_OP)
	    MakeTask(DM_RMDIR, strdup(fullsrcpath),
		     strdup(DmMakePath(dstname,directory->d_name)),
		     q);

	FREE((void *)fullsrcpath);
	fullsrcpath = NULL;	/* so it's not freed again outside of loop */
    }				/* while */
    (void)closedir(dp);

    if (fullsrcpath != NULL)
	FREE((void *)fullsrcpath);

 done:
    if (dstname != NULL)
	FREE((void *)dstname);
    if (srcname != NULL)
	FREE((void *)srcname);
    return(ret);
}					/* end of BuildSubtasksForDir */

/* ExecuteTask() is the background work proc registered with Xt
 * by DmDoFileOp().  It gets invoked by Xt MainLoop 
 * whenever no X events are pending. The function pulls out the current task
 * to be executed (or thelast incomplete task from the Desktop/tip structure
 * and acts upon it. Except for Copy, all other sub-tasks are executed thru'
 * completion as copying one large file may take pretty long time which may
 * block X event processing during the time we are in work proc.
 * This work proc. is unregistered when no more pending task remain in any
 * of the window. Scheduling of next task and removal is done via a function
 * ScheduleNextTask() called towards the end of the function or overwirte/
 * error condition occurs.
 */

static Boolean
ExecuteTask(XtPointer client_data /* UNUSED */)
{
    char *		src;
    char *		dst;
    DmTaskInfoListPtr	tip = DESKTOP_CUR_TASK(Desktop);
    DmTaskListPtr	cur_task = tip->cur_task;
    DmFileOpType	type;
    int			err;
    DtAttrs		options;
    DmFileOpInfoPtr	opr_info;

    /* nothing to do, goto end and clean up */
    if (cur_task == NULL)
	goto done;

    type	= cur_task->type;
    src		= cur_task->source;
    dst		= cur_task->target;
    opr_info	= tip->opr_info;
    options	= opr_info->options;
    err		= 0;

    /* if caller asked to be notified when an overwrite occurs */
    if (type == DM_OVERWRITE)
    {
	if (options & OVERWRITE)
	{
	    opr_info->attrs |= OVERWRITE;
	    (*tip->client_proc)(DM_OVRWRITE, tip->client_data,
				(XtPointer)tip, src, dst);

	} else
	{
	    /* The next subtask will delete the target. */
	    tip->cur_task = tip->cur_task->next;
	}
	return (ScheduleNextTask());
    }


    /* if last copy operation was not finished, continue */
    if (tip->rfd != -1)
	err = CopyBlock(src, dst, &tip->rfd, &tip->wfd, 0);

    else
    {
	/* report progress of the operation */
	if ((options & REPORT_PROGRESS) &&
	    (type != DM_BEGIN) && (type != DM_ENDOVER))
	    (*tip->client_proc)(DM_REPORT_PROGRESS,
				tip->client_data, (XtPointer)tip, src, dst);

	/* and start actual file manipulation now, keep track of
	   any error that may occur.
	*/
	switch(type)
	{
	case DM_BEGIN:
	    opr_info->cur_src = (int)dst;
	    break;
	case DM_ENDOVER:
	    break;
	case DM_RMDIR:
	    err = DmRmDir(src);
	    break;
	case DM_MKDIR:
	    err = DmMakeDirectory(src, dst);
	    break;
	case DM_COPY:
	    err = DmCopyFile(src, dst, &tip->rfd, &tip->wfd);
	    break;
	case DM_COPY_DEL:
	    /* Delete will be processed after copy is done. See below */
	    err = DmCopyFile(src, dst, &tip->rfd, &tip->wfd);
	    break;
	case DM_MOVE:
	    err = DmMoveFile(src, dst);
	    break;
	case DM_HARDLINK:
	    err = DmLinkFile(src, dst);
	    break;
	case DM_SYMLINK:
	    err = DmSymLinkFile(src, dst);
	    break;
	case DM_DELETE:
	    err = DmDeleteFile(src);
	    break;
	default:
	    break;
	}
    }

    /* If error occured, notify caller and stop further processing.
       NOTE: ignore errors during undo
    */
    if (err != 0)
    {
  process_err:
	opr_info->error = err;
	opr_info->src_info[(opr_info->cur_src == 0) ? 0 :
			   opr_info->cur_src - 1] |= SRC_B_ERROR;
	(*tip->client_proc)(DM_ERROR, tip->client_data,
			    (XtPointer)tip, src, dst);

	if ( !(options & UNDO) )
	{
	    if (type == DM_COPY_DEL)
	    {
		switch(tip->rfd)
		{
		case -3:
		    /* Copy succeeded but delete failed (WB case):
		       Just delete dst from WB.  No need to be able to undo
		       later. "Reset" rfd so StopFileOp doesn't try to close
		       it.
		    */
		    (void)DmDeleteFile(dst);
		    tip->rfd = -1;
		    /*FALLTHROUGH*/
		case -1:
		    /* CopyBlock open() failed.  No need to undo this later */
		    break;

		case -2:
		    /* Copy succeeded but delete failed (non-WB case):
		       Change type from DM_COPY_DEL to DM_COPY.
		    */
		    /*FALLTHROUGH*/
		default:
		    /* CopyBlock failed in the middle:
		       change type from DM_COPY_DEL to DM_COPY. 
		    */
		    type = cur_task->type = DM_COPY;
		    break;
		}
	    }
	}
	DmStopFileOp(tip);		/* clean up tasks after error */

    } else if (tip->rfd == -1)		/* No error and no copy in progress */
    {
	/* For "copy-then-delete", do the delete part now */
	if ((type == DM_COPY_DEL) && ((err = DmDeleteFile(src)) != 0))
	{
	    /* Process error while deleting src.  Set "special" rfd values
	       for error processing above to indicate copy succeeded but
	       delete failed.
	    */
	    tip->rfd = DmSameOrDescendant(DM_WB_PATH(Desktop), dst, -1) ?
		-3 : -2;
	    goto process_err;
	}
	tip->cur_task = tip->cur_task->next;
	opr_info->task_cnt++;
    }

 done:   
    /* if done, call client_proc with DONE status. */
    if (tip->cur_task == NULL)
	(*tip->client_proc)(DM_DONE, tip->client_data,
			    (XtPointer)tip, NULL, NULL);

    /* schedule next task to execute or unregister itself */
    return(ScheduleNextTask());
}					/* end of ExecuteTask */

/****************************procedure*header*****************************
    Dm__TargetFileType-
*/
static int
Dm__TargetFileType(struct stat * st)
{
    return(((st->st_mode & S_IFMT) == S_IFDIR) ? DM_IS_DIR :
	   ((st->st_mode & S_IFMT) == S_IFREG) ? DM_IS_FILE : DM_IS_SPECIAL );

}				/* Dm__TargetFileType */	

/****************************procedure*header*****************************
    Dm__FileType- function checks the type of the file via stat() and returns
	one of internal file defines used in file manipulation routines.
*/
static int
Dm__FileType(char *filename, struct stat *st)
{
    struct stat local_stat;

    if (st == NULL)
	st = &local_stat;

    if (lstat(filename, st) != 0)
	return(DM_NO_FILE);

    if ((st->st_mode & S_IFMT) == S_IFLNK)
	return(DM_IS_SYMLINK);

    if (stat(filename, st) != 0)
	return(DM_NO_FILE);

    return(Dm__TargetFileType(st));
}

/****************************procedure*header*****************************
    FreeSrcList-
*/
static void
FreeSrcList(DmTaskInfoListPtr tip)
{
    char **	src_list = tip->opr_info->src_list;
    int		src_cnt = tip->opr_info->src_cnt;
    char **	src;

    if ((src_list == NULL) || (src_cnt == 0))
	return;

    for (src = src_list; src < src_list + src_cnt; src++)
	XtFree(*src);

    XtFree((char *)src_list);
}					/* end of FreeSrcList */

/****************************procedure*header*****************************
    FreeTasks- frees the tasklist when an error occurs, operation is done,
	user respond negatively to the overwrite etc.
*/
static void
FreeTasks(DmTaskListPtr current)
{
    while (current != NULL)
    {
	DmTaskListPtr save;

	FreeTaskContents(current);

	save = current;
	current = current->next;
	FREE((void *)save);
    }
}					/* end of FreeTasks */

/****************************procedure*header*****************************
    GetTaskInfoItem() is called from DmDoFileOp() to get new
	DmTaskInfoListPtr for use with file operation.
*/
static DmTaskInfoListPtr
GetTaskInfoItem(void)
{
    DmTaskInfoListPtr tip;

    tip = (DmTaskInfoListPtr)MALLOC(sizeof(DmTaskInfoListRec));

    /* if first operation */
    if (DESKTOP_CUR_TASK(Desktop) == NULL)
    {
	DESKTOP_CUR_TASK(Desktop) = tip;
	tip->prev = tip;

    } else
    {
	tip->prev = DESKTOP_CUR_TASK(Desktop)->prev;
	DESKTOP_CUR_TASK(Desktop)->prev->next = tip;
	DESKTOP_CUR_TASK(Desktop)->prev = tip;
    }

    tip->next = DESKTOP_CUR_TASK(Desktop);

    return(tip);
}				/* end of GetTaskInfoItem() */

/* MakeTask() function inserts each sub-task into the list of task */
static void
MakeTask(DmFileOpType type, char * src, char * dst, DmTaskListPtr ** q)
{
    DmTaskListPtr  p;

    /* set up task structure */
    p = (DmTaskListPtr) MALLOC(sizeof(DmTaskListRec));
    p->type	= type;
    p->source	= src;
    p->target	= dst;
    p->next	= NULL;

    /* and insert into the list */
    **q = p;
    *q = &p->next;
}				/* end of MakeTask */

/****************************procedure*header*****************************
    RegFileOpWorkProc(DmTaskInfoListPtr tip)-
*/
static void
RegFileOpWorkProc(DmTaskInfoListPtr tip)
{
    if (!Desktop->bg_not_regd)		/* ie., already registered */
	return;

    XtAddWorkProc(ExecuteTask, NULL);
    Desktop->bg_not_regd = False;
    DESKTOP_CUR_TASK(Desktop) = tip;
}

/****************************procedure*header*****************************
    ReturnError-
*/
static DmTaskInfoListPtr
ReturnError(DmTaskInfoListPtr tip, int error, char * name)
{
    tip->opr_info->error = error;
    (*tip->client_proc)(DM_ERROR, tip->client_data, (XtPointer)tip, name, NULL);
    DmFreeTaskInfo(tip);
    return(NULL);
}

/****************************procedure*header*****************************
    ScheduleNextTask - scheduler for the async file opr processing.
	"Round-robin" algorithm to find next file operation to perform.  Look
	for next task that is not processing an OVERWRITE notice.  Only
	consider CUR_TASK after all other opr's have been considered.  Return
	Boolean corresponding to use by Intrinsics: "True" means DON'T
	re-register background work proc.

	Note: CUR_TASK can be NULL as after caller frees task info when DONE.
*/
static Boolean
ScheduleNextTask(void)
{
    DmTaskInfoListPtr   tip;

    if ( (tip = DESKTOP_CUR_TASK(Desktop)) != NULL )
    {
	do
	{
	    tip = tip->next;
	    if ((tip->cur_task != NULL) && !(tip->opr_info->attrs & OVERWRITE))
	    {
		DESKTOP_CUR_TASK(Desktop) = tip;
		return(False);
	    }

	} while(tip != DESKTOP_CUR_TASK(Desktop));
    }

    DESKTOP_CUR_TASK(Desktop) = NULL;
    Desktop->bg_not_regd = True;
    return(True);
}					/* end of ScheduleNextTask() */

/****************************procedure*header*****************************
	FILE OPERATION ROUTINES

    Low-level routines which actually perform the file operations.
*/

/* CopyBlock() - a lowest level function to copy a 'src' to 'dst'. Note 
   that the function copies only COPY_BYTES at a time since this is during
   work proc (ie, minimize affect on user feedback).  Keep track of
   completion by keeping fildes open or "reset" (to -1).
*/
static int
CopyBlock(char * src, char * dst, int * rfd, int * wfd, mode_t mode)
{
#define COPY_BYTES	4096

    char	buf[COPY_BYTES];
    int		cnt;

    if (*rfd == -1)
    {
	if (strcmp(src, dst) == 0)
	    return(ERR_CopyToSelf);

	if ((*rfd = open(src, O_RDONLY)) == -1)
	    return(ERR_OpenSrc);

	if ((*wfd = open(dst, O_WRONLY|O_CREAT | O_EXCL, mode)) == -1)
	{
	    close(*rfd);
	    *rfd = -1;
	    return(ERR_OpenDst);
	}
	return(0);	/* First time thru, just open src & dst */
    }

    cnt = read(*rfd, buf, COPY_BYTES);

    if ((cnt > 0) && (write(*wfd, buf, cnt) == cnt))
	return(0);

    /* EOF reached during read or error occurred during read or write.
       Always close filedes but only "reset" them when EOF (ie, not error).
       Don't reset them when error so ExecuteTask knows to undo opr later.
    */
    close(*rfd);
    close(*wfd);

    if (cnt == 0)		/* ie, EOF */
    {
	*rfd = -1;
	*wfd = -1;
	return (0);
    }
				/* ie, a read or write error */
    (void)unlink(dst);

    return( (cnt == -1) ? ERR_Read : ERR_Write );

}					/* end of CopyBlock */

/* DmCopyFile()- low level function to copy 'src' to 'dst'.
	If src is a symbolic link, make a new copy of it.  Otherwise, copy
	the src to the dst a 'block' at a time and change mode of 'dst'
	based on 'src' mode.
*/
static int
DmCopyFile(char * src, char * dst, int * rfd, int * wfd)
{
    struct stat mystat;
    int retvalue;

    if (lstat(src, &mystat) != 0)
	return(ERR_NotAFile);

    if ((mystat.st_mode & S_IFLNK) == S_IFLNK)
	return( MakeLinkInstead(src, dst) );

    if (stat(src, &mystat) != 0)
	return(ERR_NotAFile);

    if (access(dst, F_OK) == 0)
	return(ERR_IsAFile);

    if ( (retvalue = CopyBlock(src, dst, rfd, wfd, mystat.st_mode)) != 0 )
	return(retvalue);

    return(0);
}				/* end of DmCopyFile */

/* DmDeleteFile() - low level function to delete a file specified in 'src' */
static int
DmDeleteFile(char * src)
{
    return((unlink(src) == 0) ? 0 : ERR_Rm);

}				/* end of DmDeleteFile */

/****************************procedure*header*****************************
   DmLinkFile - low level function to create a link from 'src' to 'dst'
*/
static int
DmLinkFile(char * src, char * dst)
{
    struct stat mystat;

    /* NOTE: is the next statement necessary? */
    if (stat(src, &mystat) != 0)
	return(ERR_NotAFile);

    if (access(dst, F_OK) == 0)
	return(ERR_IsAFile);

    errno = 0;
    return((link(src, dst) == 0) ? 0 :
	   (errno == EXDEV) ? ERR_ForeignLink : ERR_Link );

}					/* end of DmLinkFile */

/* DmMakeDirectory - low level function to create a new directory 
   specified in 'dst'. 'dst' inherits mode info. from the src.
*/
static int
DmMakeDirectory(char * src, char * dst)
{
    struct stat mystat;

    if (stat(src, &mystat) != 0)
	return(ERR_Stat);

    /* Don't create a directory to which the user does not have RWX access.
       Note, too, that parameter 'mode' is masked by user's umask. 
    */
    errno = 0;
    return(((mkdir(dst, mystat.st_mode | S_IRWXU) == 0) ||
	    (errno == EEXIST)) ? 0 : ERR_Mkdir);

}					/* end of DmMakeDirectory */

/****************************procedure*header*****************************
    MakeLinkInstead- if src is a symbolic link, rather than copy or move
	it (across file system), make a link instead.
*/
static int
MakeLinkInstead(char * src, char * dst)
{
    char	lname[FILENAME_MAX + 1];
    int		len;

    if ( (len = readlink(src, lname, FILENAME_MAX)) == -1 )
	return(ERR_ReadLink);

    lname[len] = 0;

    return( (symlink(lname, dst) == 0) ? 0 : ERR_Link );

}					/* end of MakeLinkInstead */

/* DmMoveFile - low level function to move/rename a 'src' to 'dst'.
   If move s across the file system, copy instead.
*/
static int
DmMoveFile(char * src, char * dst)
{
    if (access(dst, F_OK) == 0)
	return(ERR_IsAFile);


    return(rename(src, dst) == 0 ? 0 : ERR_Rename);

}					/* end of DmMoveFile */

/* DmRmDir() - low level function to delete a file specified in 'src' */
static int
DmRmDir(char * src)
{
    return((rmdir(src) == 0) ? 0 : ERR_Rm);

}					/* end of DmRmDir */

/* DmSymLinkFile - low level function to create a symbolic link 
   from 'src' to 'dst'.
*/
static int
DmSymLinkFile(char * src, char * dst)
{
    struct stat mystat;

    if (stat(src, &mystat) != 0)
	return(ERR_NotAFile);

    if (access(dst, F_OK) == 0)
	return(ERR_IsAFile);

    return((symlink(src, dst) == 0) ? 0 : ERR_Link );

}					/* end of DmSymLinkFile */

/***************************private*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
    DmDoFileOp- process file opr request.
	- set up TaskInfoList structure that contains necessary info.
	  about the operation (e.g. sorc_list to operate, target, error
	  conditions etc. Used by many internal function that implements one 
	  part of the file manipulation as well as functions that are 
	  responsible for Window update, Undo, Kill etc.

	- builds a task list: each big operation (e.g. moving a set of files
	  will break down to move of each file separately. etc).

	- register a background proc. with Xt so that whenever no events are
	   pending to be processed, file manipulation is done via the 
	  registered proc. Note, the background workproc. is the way how
	  an asynchrouns processing of file operation is implemented in dtm.
*/
DmTaskInfoListPtr
DmDoFileOp(DmFileOpInfoPtr opr_info,
	   DmClientProc client_proc, XtPointer client_data)
{
    DmTaskListPtr *	q;
    DmTaskInfoListPtr	tip;
    int			target_type;
    int			source_type;
    struct stat 	target_stat;
    struct stat 	source_stat;
    char **		src;
    DmFileOpType	type = opr_info->type;
    unsigned short	target_op_vec;
    unsigned short	op_vec;
    char		local_buffer[FILENAME_MAX];
    char *		target_path;
    int			target_offset;
    int			same_fs;
    int			ret;
    int			src_idx;

    tip = GetTaskInfoItem();

    tip->task_list	= NULL;
    tip->rfd		= -1;
    tip->wfd		= -1;
    tip->client_proc	= client_proc;
    tip->client_data	= client_data;
    tip->opr_info	= opr_info;
    opr_info->attrs	= 0;
    opr_info->cur_src	= 0;
    opr_info->task_cnt	= 0;
    opr_info->error	= 0;
    opr_info->src_info	= (DtAttrs *)CALLOC(opr_info->src_cnt,sizeof(DtAttrs));


    /* q points to the current end of the linked list */
    q = &tip->task_list;
	
    target_path = opr_info->target_path;

    /* get target type */
    if ((target_path == NULL) || (stat(target_path, &target_stat) != 0))
    	target_type = DM_NO_FILE;
    else
    {
    	target_type = Dm__TargetFileType(&target_stat);

	if (target_type == DM_IS_DIR)
	    opr_info->attrs = DM_B_DIR_EXISTS;
    }

    /* Look up target table first. */
    target_op_vec = target_table[target_type];

    /* Use RENAME as a hint.  It is exactly like MOVE (and gets re-mapped to
       MOVE) except that 'target_path' should be taken as is and not
       adjusted.  This is the case for "Rename" and when putting back a dir
       from WB, for instance.  Note that turning on OVER_WRITE here handles
       the dir case; overwriting a file is handled in the table.
    */
    if (type == DM_RENAME)
    {
	if (target_op_vec & ADJUST_TARGET)
	{
		target_op_vec &= ~ADJUST_TARGET;
		target_op_vec |= OVER_WRITE;
	}

	/* Re-map RENAME to MOVE for src/op processing below */
	type = opr_info->type = DM_MOVE;
    }

    if (target_op_vec & OVER_WRITE)
    {
	if (opr_info->options & DONT_OVERWRITE)
	    return( ReturnError(tip, ERR_IsAFile, target_path) );

	MakeTask(DM_OVERWRITE, NULL, strdup(opr_info->target_path), &q);
	if (target_type == DM_IS_DIR)
	{
	    ret = BuildSubtasksForDir(tip, DM_DELETE,
				      strdup(target_path), NULL, &q);
	    if (ret != 0)
		return( ReturnError(tip, ret, target_path) );

	    MakeTask(DM_RMDIR, strdup(target_path), NULL, &q);

	} else
	    MakeTask(DM_DELETE, strdup(target_path), NULL, &q);
    }

    if (target_op_vec & MSRC_MKDIR)
	if (opr_info->src_cnt > 1)
	{
	    if (type != DM_DELETE) {
	    	MakeTask(DM_MKDIR, strdup(opr_info->src_path),
			 strdup(opr_info->target_path), &q);

	        opr_info->attrs = DM_B_DIR_NEEDED_4FILES;

	        /* Adjust the target when multi-src's */
	        target_op_vec |= ADJUST_TARGET;
	    }
	}

    if (target_op_vec & ADJUST_TARGET)
    {
	/* need to append basename of each src to the end of target later */
	strcpy(target_path = local_buffer, opr_info->target_path);
	target_offset = strlen(target_path);
	if (strcmp(target_path, "/")) {
		target_path[target_offset++] = '/';
		target_path[target_offset] = '\0'; /* is this needed ? */
	}

	/* Indicate "adjusted" to caller */
	opr_info->attrs |= DM_B_TARGET_ADJUSTED;

    }

    /* go thru' each source item and build a "task list" that will be
       sequentially operated on by the background work proc. to execute user
       specified operation thru' completion
    */
    for (src_idx = 0, src = opr_info->src_list;
	 src_idx < opr_info->src_cnt; src_idx++, src++)
    {
	char * fullsrcpath = DmMakePath(opr_info->src_path, *src);

	/* Can't mv, rm, etc these special directories (even as root) */
	if ((type == DM_MOVE) || (type == DM_DELETE))
	{
	    if (strcmp(fullsrcpath, "/") == 0)
		return ( ReturnError(tip, ERR_MoveRoot, *src) );
	    if (strcmp(fullsrcpath, DESKTOP_DIR(Desktop)) == 0)
		return ( ReturnError(tip, ERR_MoveDesktop, *src) );
	    if (strcmp(fullsrcpath, DM_WB_PATH(Desktop)) == 0)
		return ( ReturnError(tip, ERR_MoveWastebasket, *src) );
	}

	/* DmMakePath used so make copy (freed below) */
	/* NOTE: memory leak when ReturnError below */
	fullsrcpath = strdup(fullsrcpath);

	/* Mark the beginning of tasks associated with a source item */
	MakeTask(DM_BEGIN, (char *)(src_idx + 1), (char *)(src_idx + 1), &q);

	source_type = Dm__FileType(fullsrcpath, &source_stat);
	opr_info->src_info[src_idx] = source_type;
	op_vec = OPCODES(source_type, opr_info->type);

	if (target_op_vec & ADJUST_TARGET)
	{
	    unsigned short tmp_op_vec;

	    /* add basename of source to target */
	    strcpy(target_path + target_offset, basename(fullsrcpath));

	    if (op_vec & (CHK_SAME | CHK_DESC))
	    {
		ret = DmSameOrDescendant(fullsrcpath, target_path, -1);

		if (ret < 0)		/* ie, same */
		    return(ReturnError(tip,
				       (type == DM_MOVE) ? ERR_MoveToSelf :
				       (type == DM_COPY) ? ERR_CopyToSelf :
				       ERR_LinkToSelf, *src) );

		else if ((ret > 0) && (op_vec & CHK_DESC))
		    return(ReturnError(tip,
				       (type == DM_MOVE) ? ERR_MoveToDesc :
				       ERR_CopyToDesc, *src) );
	    }

	    /* get the type of new target */
	    target_type = (stat(target_path, &target_stat) == 0) ?
		Dm__TargetFileType(&target_stat) : DM_NO_FILE;

	    tmp_op_vec = target_table[target_type];

	    if (tmp_op_vec & OVER_WRITE)
	    {
		MakeTask(DM_OVERWRITE, strdup(fullsrcpath),
			 strdup(target_path), &q);

		if (target_type == DM_IS_DIR)
		{
		    ret = BuildSubtasksForDir(tip, DM_DELETE,
					      strdup(target_path),
					      NULL, &q);
		    if (ret != 0)
			return( ReturnError(tip, ret, *src) );

		    MakeTask(DM_RMDIR, strdup(target_path), NULL, &q);

		} else
		    MakeTask(DM_DELETE, strdup(target_path), NULL, &q);
	    }

	    /* Special case if both source and target are dirs. */
	    if ((source_type != DM_NO_FILE) && (target_type == DM_IS_DIR))
	    {
		MakeTask(DM_OVERWRITE, strdup(fullsrcpath),
			 strdup(target_path), &q);

		ret = BuildSubtasksForDir(tip, DM_DELETE,
					  strdup(target_path), NULL, &q);

		if (ret != 0)
		    return( ReturnError(tip, ret, *src) );

		MakeTask(DM_RMDIR, strdup(target_path), NULL, &q);
	    }
	} else if (op_vec & (CHK_SAME | CHK_DESC))
	{
	    ret = DmSameOrDescendant(fullsrcpath, target_path, -1);

	    if (ret < 0)		/* ie, same */
		return(ReturnError(tip,
				   (type == DM_MOVE) ? ERR_MoveToSelf :
				   (type == DM_COPY) ? ERR_CopyToSelf :
				   ERR_LinkToSelf, *src) );

	    else if ((ret > 0) && (op_vec & CHK_DESC))
		return(ReturnError(tip,
				   (type == DM_MOVE) ? ERR_MoveToDesc :
				   ERR_CopyToDesc, *src) );
	}

	/* This check must be done before other bits are checked, because
	   the other LSB bits may the error number.
	*/
	if (op_vec & ERROR_OP)
	    return( ReturnError(tip, op_vec & ~ERROR_OP, *src) );

	if (op_vec & CHK_FS)
	{
	    if (target_type == DM_NO_FILE)
	    {
		char *free_this;
		char *dirpath;

		/* Use stat info of dirname(target) */

		dirpath = dirname(free_this = strdup(target_path));
		if (stat(dirpath, &target_stat) == -1)
		{
		    extern int errno;

		    /* TRY PARENT DIR
		     * This assumes that we are creating only one level of
		     * new dir, which should be the case.
		     */
		    if ((errno != ENOENT) ||
		        (dirpath = dirname(dirpath),
			 stat(dirpath, &target_stat) == -1)) {
		    	FREE(free_this);
		    	return( ReturnError(tip, ERR_TargetFS, *src) );
		    }
		}
		FREE(free_this);
	    }
	    same_fs = (target_stat.st_dev == source_stat.st_dev);

	} else
	    same_fs = 1;

	if (op_vec & MV_DIR)
	{
	    op_vec = same_fs ? MV_OP : MKDIR_OP | RECUR_CP_DEL | RMDIR_OP;

	} else if (op_vec & MV_OP)
	{
	    /* Special check for move operation.  Moving across file system
	       translates to copy and delete.  But should preserve the CHK_CP
	       bit in case it is a special file.
	    */
	    if (!same_fs)
		op_vec = (op_vec & CHK_CP) | (CP_DEL_OP);
	}

	if (op_vec & CHK_CP)
	    if (op_vec & (CP_OP | CP_DEL_OP))
		return( ReturnError(tip, ERR_CopySpecial, *src) );

	if (op_vec & MKDIR_OP)
	    MakeTask(DM_MKDIR, strdup(fullsrcpath), strdup(target_path), &q);

	/* Passing 'type' here is okay since link op's are not re-mapped */
	if (op_vec & LN_OP)
	{
	    MakeTask(type, strdup(fullsrcpath), strdup(target_path), &q);

	} else if (op_vec & (CP_OP | CP_DEL_OP | MV_OP))
	{
	    MakeTask((op_vec & CP_OP) ? DM_COPY :
		     (op_vec & CP_DEL_OP) ? DM_COPY_DEL : DM_MOVE,
		     strdup(fullsrcpath), strdup(target_path), &q);


	} else if (op_vec & DEL_OP)
	{
	    MakeTask(DM_DELETE, strdup(fullsrcpath), NULL, &q);

	} else if (op_vec & (RECUR_CP | RECUR_CP_DEL))
	{
	    /* descend into subdirectories */
	    ret = BuildSubtasksForDir(tip, (op_vec & RECUR_CP) ?
				      DM_COPY : DM_COPY_DEL,
				      strdup(fullsrcpath),
				      strdup(target_path), &q);
	    if (ret != 0)
		return( ReturnError(tip, ret, *src) );

	} else if (op_vec & RECUR_DEL)
	{
	    /* descend into subdirectories */
	    ret = BuildSubtasksForDir(tip, DM_DELETE,
				      strdup(fullsrcpath), NULL, &q);

	    if (ret != 0)
		return( ReturnError(tip, ret, *src) );
	}

	if (op_vec & RMDIR_OP)
	    MakeTask(DM_RMDIR, strdup(fullsrcpath),
		     (target_path == NULL) ? NULL : strdup(target_path), &q);

	/* Mark the end of tasks associated with a source item */
	MakeTask(DM_ENDOVER, (char *)(src_idx + 1), (char *)(src_idx + 1), &q);

	FREE(fullsrcpath);
    }					/* for each source */

    if (target_op_vec & OVER_WRITE)
	MakeTask(DM_ENDOVER, (char *)0, (char *)0, &q);

    tip->cur_task = tip->task_list;

    BeginFileOp(tip);
    return(tip);
}				/* end of DmDoFileOp */

/* DmFreeTaskInfo() - Remove task from list of file opr's and free all data
*/
void
DmFreeTaskInfo(DmTaskInfoListPtr tip)
{
    DmFileOpInfoPtr	opr_info;

    if (tip == NULL)
	return;

    /* Remove from list */
    if (tip->next == tip)			/* only task */
    {
	tip->next = NULL;			/* for CUR_TASK check below */

    } else
    {
	tip->prev->next	= tip->next;		/* adjust prev task */
	tip->next->prev	= tip->prev;		/* adjust next task */
    }

    if (DESKTOP_CUR_TASK(Desktop) == tip)
	DESKTOP_CUR_TASK(Desktop) = tip->next;	/* may be NULL */

    /* Free contents */
    opr_info = tip->opr_info;
    FreeTasks(tip->task_list);
    XtFree(opr_info->target_path);
    XtFree(opr_info->src_path);
    XtFree((char *)opr_info->src_info);
    FreeSrcList(tip);
    FREE((void *)opr_info);
    FREE((void *)tip);
}					/* end of DmFreeTaskInfo() */

/****************************procedure*header*****************************
    Dm__Overwrite- clean-up tasks due to overwriting target (or not).

	If overwriting ('overwrite' == True), just (re)register work proc so
	that overwrite tasks will be executed.

	If not, mark src item ('src_info') as "skipped" so it will not be
	processed during UpdateWindow and remove sub-tasks for this item.
*/
void 
Dm__Overwrite(DmTaskInfoListPtr tip, Boolean overwrite)
{
    DmFileOpInfoPtr	opr_info = tip->opr_info;

    /* NOTE: when cur_src is 0, don't do any of this (?).
       Free all tasks and set cur_task to NULL.
    */
    if (!overwrite)
    {
	DmTaskListPtr	task;
	int		match;
   
	/* Mark source item as skipped so that UNDO will work. */
	opr_info->src_info[(opr_info->cur_src == 0) ? 0 :
			  opr_info->cur_src - 1] |= SRC_B_SKIP_OVERWRITE;

	/* Remove any remaining tasks for this overwrite.  This is just like
	   FreeTasks except that only want to free up to the ENDOVER task.
	*/
	match = opr_info->cur_src;
	task = tip->cur_task->next;
	while ( !((task->type == DM_ENDOVER) &&
		  (match == (int)(task->source)) &&
		  (match == (int)(task->target))))
						     
	{
	    DmTaskListPtr save;

	    FreeTaskContents(task);

	    save = task;
	    task = task->next;
	    FREE((void *)save);
	}

	/* Overwrite tasks have been removed from task list so have cur_task
	   point to next task (actually this is the ENDOVER task but this is
	   needed to match the OVERWRITE task).  Add one to task_cnt for
	   ENDOVER.
	*/
	tip->cur_task->next = task;
	opr_info->task_cnt++;
    }

    opr_info->attrs &= ~OVERWRITE;	/* no longer in "OVERWRITE" state */
    tip->cur_task = tip->cur_task->next;
    RegFileOpWorkProc(tip);
}					/* end of Dm__Overwrite */

/****************************procedure*header*****************************
    DmStopFileOp- "stop" file op.  That is, clean up file opr tasks after
	user has stopped the opr or an error occurred. 
*/
void
DmStopFileOp(DmTaskInfoListPtr tip)
{
    DmFileOpInfoPtr	opr_info;

    if ((tip == NULL) || (tip->cur_task == NULL))
	return;

    opr_info = tip->opr_info;


    /* close any open file descriptors */
    if (tip->rfd >= 0)
	(void)close(tip->rfd);
    if (tip->wfd >= 0)
	(void)close(tip->wfd);

    if ((tip->cur_task->type == DM_COPY) && (tip->rfd != -1))
    {
	/* The copy operation that failed should be undone later */
	tip->cur_task = tip->cur_task->next;
	tip->rfd = -1;
	opr_info->task_cnt++;
    }

    if (opr_info->task_cnt == 0)
    {
	/* Nothing to undo. Failed on the first subtask. */
	FreeTasks(tip->task_list);
	tip->task_list = NULL;

    } else
    {
	/* Free all subtasks AFTER this subtask */
	FreeTasks(tip->cur_task->next);

	FreeTaskContents(tip->cur_task);

	/* Change the last subtask to a ENDOVER type (for UNDO) */
	tip->cur_task->type		= DM_ENDOVER;
	tip->cur_task->source		=
	    tip->cur_task->target	= (char *)(opr_info->cur_src);
	tip->cur_task->next		= NULL;

	/* add one for the ENDOVER subtask */
	opr_info->task_cnt++;
    }
 
    tip->cur_task = NULL;
}					/* end of DmStopFileOp */


static const DmFileOpType undo_table[] = {
    /* Original op		Undo op */
    /* DM_COPY		*/	DM_DELETE,
    /* DM_MOVE		*/	DM_MOVE,
    /* DM_DELETE	*/	DM_NO_OP,
    /* DM_HARDLINK	*/	DM_DELETE,
    /* DM_SYMLINK	*/	DM_DELETE,
    /* DM_COPY_DEL	*/	DM_COPY_DEL,
    /* DM_BEGIN		*/	DM_ENDOVER,
    /* DM_MKDIR		*/	DM_RMDIR,
    /* DM_RMDIR		*/	DM_MKDIR,	/* but see code */
    /* DM_OVERWRITE	*/	DM_NO_OP,
    /* DM_ENDOVER	*/	DM_BEGIN,
    /* DM_RENAME	*/	/* not used */
};

/****************************procedure*header*****************************
    DmUndoFileOp-
*/
int
DmUndoFileOp(DmTaskInfoListPtr tip)
{
    DmFileOpInfoPtr	opr_info = tip->opr_info;
    DmTaskListPtr *	task_ptrs;
    DmTaskListPtr *	tasks;
    DmTaskListPtr	orig_task;
    DmTaskListPtr	current;
    int			cur_src;

    /* build a vector of task list ptrs so list can be traversed in reverse */
    task_ptrs = (DmTaskListPtr *)
	MALLOC(opr_info->task_cnt * sizeof(DmTaskListPtr));

    if (task_ptrs == NULL)
	return(-1);

    orig_task = tip->task_list;
    for (tasks = task_ptrs; tasks < task_ptrs + opr_info->task_cnt; tasks++)
    {
	*tasks = orig_task;
	orig_task = orig_task->next;
    }

    /* Process the task list in reverse */

    tasks = task_ptrs + opr_info->task_cnt - 1;
    tip->task_list = tip->cur_task = *tasks;	/* tail becomes head */
    cur_src = opr_info->cur_src;

    for ( ; tasks >= task_ptrs; tasks--)
    {
	DmTaskListPtr task = *tasks;

	/* Map opr type to "undo" type */
	task->type = undo_table[task->type - 1];

	/* Special case for undo'ing RMDIR: don't always have original target
	   (following a recursive delete, for instance).  Therefore, this
	   cannot be undone.
	*/
	if ((task->type == DM_MKDIR) && (task->target == NULL))
	    task->type = DM_NO_OP;

	/* Clean up list by removing tasks that can't be undone */
	if (task->type == DM_NO_OP)
	{
	    /* Fix link from "previous" task.
	       Note: we know this is not the 1st but it may be the last task.
	    */
	    FreeTaskContents(task);
	    FREE((void *)task);

	} else
	{
	    if ((task->type == DM_BEGIN) || (task->type == DM_ENDOVER))
	    {
		/* Invert the src-item counter: n-->1, n-1-->2, etc. */
		task->source = task->target =
		    (char *)(cur_src - (int)task->source + 1);
	    
	    } else
	    {				/* switch source and target */
		char * tmp;
		tmp		= task->source;
		task->source= task->target;
		task->target= tmp;
	    }
	    current = task;
	}
	
	current->next = (tasks > task_ptrs) ? tasks[-1] : NULL;
    }
    FREE((void *)task_ptrs);

    opr_info->type	= undo_table[opr_info->type - 1];
    opr_info->task_cnt	= 0;			/* re-init num_subtasks */
    opr_info->cur_src	= 0;			/* re-init current src item */
    opr_info->error	= 0;

    /* NOTE: clear 'src_info' array?? */

    BeginFileOp(tip);
    return(0);
}					/* end of DmUndoFileOp */

