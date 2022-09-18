/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtm:tb_util.c	1.17"
#endif

#include <libgen.h>
#include <fcntl.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <Xol/OpenLook.h>
#include <FIconBox.h>

#include "Dtm.h"
#include "extern.h"

#define DUP_SUFFIX	":copy"

/*
 * Given a window id, find the toolbox window ptr if found.
 */
DmToolboxWinPtr
DmIsToolboxWin(win)
Window win;
{
	register DmToolboxWinPtr twp;

	for (twp=DESKTOP_TWP(Desktop); twp; twp=twp->next)
		if (XtWindow(twp->box) == win) {
			return(twp);
		}
	return(NULL);
}

/*
 * This function checks if the given name is part of the cp ancestor hierarchy.
 * Returns:	0 - no match.
 *		1 - name is the same as cp.
 *		2 - name is an ancestor of cp.
 */
int
DmTBIsAncestor(initial_cp, name)
DmContainerPtr initial_cp;
char *name;
{
	register DmContainerPtr cp = initial_cp;

	while (cp) {
		if (!strcmp(cp->path, name)) {
			if (cp == initial_cp)
				return(1);
			else
				return(2);
		}

		cp = TBINFO(cp)->parent;
	}
	return(0);
}

/*
 * This routine checks the name for duplicates in the container.
 * If duplicate is found, then ":copy" is appended to the name.
 * The above process will be repeated until a unique name is found.
 */
char *
DmUniqueLabel(cp, op, options)
DmContainerPtr cp;
DmObjectPtr op;
DtAttrs options;
{
	char *p = Dm__buffer;
	int dup = 0;

	strcpy(p, op->name);

	while (1) {
		if (DmGetObjectInContainer(cp, p))
			dup++;
		else if ((op->ftype == DM_FTYPE_TOOLBOX) &&
		    	 DmGetToolboxInfo(p)) {
				/*
				 * A special case is if the operation is a
				 * move and op is a toolbox, then p can be
				 * the same as the original op->name.
				 */
				if (!(options&DM_B_MOVE) && !strcmp(p,op->name))
					dup++;
			}

		if (dup) {
			dup = 0;
			strcat(p, DUP_SUFFIX);
		}
		else
			return(p);
	}
}

void
DmTBGetDefaultXY(twp, op, x, y)
DmToolboxWinPtr twp;
DmObjectPtr op;
int *x;
int *y;
{
	DmItemRec item;
	Dimension width;

	/* create a fake item to get size and position */
	item.label = (XtArgVal)(op->name);
	item.object_ptr = (XtArgVal)op;
	DmSizeIcon(&item, DESKTOP_FONTLIST(Desktop),
			  DESKTOP_FONT(Desktop),
			  DESKTOP_LABELHT(Desktop));

	/* get current width from icon box */
	XtSetArg(Dm__arg[0], XtNwidth, &width);
	XtGetValues(twp->box, Dm__arg, 1);

	/* calculate default x & y */
	DmGetDefaultXY(x, y, twp->itp,
        		twp->nitems,
			&item,
			(int)width,
			(int)ICON_GRID_WIDTH(Desktop),
			(int)ICON_GRID_HEIGHT(Desktop));
}

int
DmNewObjectInWindow(twp, op)
DmToolboxWinPtr twp;
DmObjectPtr op;
{
	int x, y;
	int ret;

	if ((op->x == 0) && (op->y == 0))
		DmTBGetDefaultXY(twp, op, &x, &y);
	else {
		x = op->x;
		y = op->y;
	}

	if (ret = DmAddObjectToIconContainer(twp->box, &(twp->itp),
			&(twp->nitems),
			twp->cp, op,
			x, y, DM_B_NO_INIT | DM_B_CALC_SIZE,
			DESKTOP_FONTLIST(Desktop),
			DESKTOP_FONT(Desktop),
			DESKTOP_LABELHT(Desktop)))
		_OlFlatLayoutIconBox(twp->box, 0, 0);
	return(ret);
}

int
DmAddShortcut(twp, op, x, y, options)
DmToolboxWinPtr twp;
DmObjectPtr op;
int x;
int y;
DtAttrs options;
{
	int ret;
	char *save_name = op->name;

	if (options & DM_B_DUPCHECK)
		op->name = strdup(DmUniqueLabel(twp->cp, op, options));
	if (ret = DmAddObjectToIconContainer(twp->box, &(twp->itp),
			&(twp->nitems),
			twp->cp, op,
			x, y, DM_B_NO_INIT | DM_B_CALC_SIZE,
			DESKTOP_FONTLIST(Desktop),
			DESKTOP_FONT(Desktop),
			DESKTOP_LABELHT(Desktop)) != OL_NO_ITEM) {
		if (op->ftype == DM_FTYPE_TOOLBOX) {
			DmContainerPtr cp;

			/* Just moved a toolbox. */

			/* if name has changed, update toolbox */
			if ((options & DM_B_DUPCHECK) &&
			    strcmp(save_name, op->name))
				DmChangeToolboxName(save_name, op->name);

			/*
			 * Need to update the toolbox container's parent.
			 */
			cp = DmGetToolboxInfo(op->name);
			TBINFO(cp)->parent = twp->cp;
		}
		if (options & DM_B_DUPCHECK)
			free(save_name);
		_OlFlatLayoutIconBox(twp->box, 0, 0);
	}
	else {
		if (options & DM_B_DUPCHECK) {
			/* restore op->name */
			free(op->name);
			op->name = save_name;
		}
	}
	return(ret);
}

DmContainerPtr
DmGetToolboxInfo(path)
char *path;
{
	register DmContainerPtr cp;

	for (cp=DESKTOP_TCP(Desktop); cp; cp=cp->next)
		if (!strcmp(cp->path, path))
			return(cp);
	return(NULL);
}

DmContainerPtr
DmCreateToolbox(parent, path, x, y)
DmContainerPtr parent;
char *path;
int x, y;
{
	DmContainerPtr new_cp = Dm__NewContainer(path);
	int fd;
	char prefix[6] = { 0 };
	char *toolbox_path;
	DmTBContainerInfoPtr tbcp;
	DmObjectPtr op;
	
	if (!new_cp)
		return(NULL);

	/** allocate a file name for the toolbox info file **/
	/* use the first 4 letters from the original box name */
	prefix[0] = '.';
	strncpy(prefix + 1, path, 4);
	toolbox_path = DmGetDTProperty(TOOLBOX_PATH, NULL);
	tbcp = (DmTBContainerInfoPtr)malloc(sizeof(DmTBContainerInfoRec));
	tbcp->path = tempnam(toolbox_path, prefix);
	tbcp->parent = parent;
	new_cp->data = (void *)tbcp;

	/* create a blank file */
	if ((fd = open(tbcp->path, O_WRONLY | O_CREAT | O_EXCL,
			DESKTOP_UMASK(Desktop))) == -1)
		goto err_2;
	close(fd);

	/* add it to the desktop linked list */
	new_cp->next = DESKTOP_TCP(Desktop);
	DESKTOP_TCP(Desktop) = new_cp;

	/* add it to the parent's list of objects */
	if (op = (DmObjectPtr)calloc(1, sizeof(DmObjectRec))) {
		DmToolboxWinPtr twp;

		op->name = strdup(path);
		op->fcp = DmFtypeToFmodeKey(DM_FTYPE_TOOLBOX)->fcp;
		op->ftype = DM_FTYPE_TOOLBOX;
		op->x = x;
		op->y = y;

		/* save TOOLBOX_FILE */
		DtSetProperty(&(op->plist), TOOLBOX_FILE,
				basename(tbcp->path), 0);
		DtSetProperty(&(op->plist), OBJECT_TYPE, "TOOLBOX", 0);

		if (twp = DmQueryToolboxWindow(parent->path)) {
			DmInitObjType(twp->box, op);
			if (DmNewObjectInWindow(twp, op) != OL_NO_ITEM)
				return(new_cp);
			else
				goto err_0;
		}
		else {
			if (DmAddObjectToContainer(parent, op, 0))
				goto err_0;
			else
				return(new_cp);
		}
	}
	else
		goto err_1;

err_0:
	Dm__FreeObject(op);
	free(op);

err_1:
	unlink(tbcp->path);

err_2:
	free(tbcp->path);
	free(tbcp);
	Dm__FreeContainer(new_cp);

	return(NULL);
}

int
DmTBCopyShortcut(dst_cp, src_cp, op, x, y)
DmContainerPtr dst_cp;
DmContainerPtr src_cp;
DmObjectPtr op;
int x, y;		/* position of icon */
{
	DmToolboxWinPtr dst_twp = DmQueryToolboxWindow(dst_cp->path);
	DmObjectPtr new_op;
	int ret;

	if (op->ftype == DM_FTYPE_TOOLBOX) {
		DmContainerPtr new_cp;
		DmContainerPtr sub_cp = DmGetToolboxInfo(op->name);
		char *new_name = DmUniqueLabel(dst_cp, op, 0);

		if (DmTBIsAncestor(dst_cp, op->name)) {
			/* copying toolbox into itself */
			return(0);
		}

		if (new_cp = DmCreateToolbox(dst_cp, new_name, x, y)) {
			for (op=sub_cp->op; op; op=op->next) {
				if (!DmTBCopyShortcut(new_cp, sub_cp, op, 0, 0))
					return(0);
			}
			return(1);
		}
		else
			return(0);
	}

	/* replicate op */
	new_op = DmDupObject(op);
	free(new_op->name); /* free old name */
	new_op->name = strdup(DmUniqueLabel(dst_cp, new_op, 0));

	if (dst_twp) {
		/** toolbox is currently open **/

		if (!x && !y)
			/* get default X Y */
			DmTBGetDefaultXY(dst_twp, new_op, &x, &y);

		/* for now, just copy everything from op */
		new_op->objectdata = op->objectdata;
		if (DmAddShortcut(dst_twp, new_op, x, y, 0)
				 != (int)OL_NO_ITEM) {
			/* bump the usage count for the real container */
			((DmObjectPtr)(op->objectdata))->container->count++;
			ret = 1;
		}
		else {
			new_op->objectdata = NULL;
			Dm__FreeObject(new_op);
			ret = 0;
		}
	}
	else {
		/* toolbox is NOT open */

		new_op->x = x;
		new_op->x = y;
		if (DmAddObjectToContainer(dst_cp, new_op, DM_B_CHECK_DUP)) {
			new_op->objectdata = NULL;
			Dm__FreeObject(new_op);
			ret = 0;
		}
		else
			ret = 1;
	} /* toolbox is NOT open */

	return(ret);
}

int
DmTBMoveShortcut(dst_cp, src_cp, op)
DmContainerPtr dst_cp;
DmContainerPtr src_cp;
DmObjectPtr op;
{
	DmToolboxWinPtr dst_twp = DmQueryToolboxWindow(dst_cp->path);
	char *save_name;
	int ret;

	if ((op->ftype==DM_FTYPE_TOOLBOX) && DmTBIsAncestor(dst_cp, op->name)) {
		/* moving toolbox icon into itself */
		return(0);
	}

	save_name = op->name;
	op->name = strdup(DmUniqueLabel(dst_cp, op, DM_B_MOVE));

	if (dst_twp) {
		int x, y;

		/** toolbox is currently open **/
		DmTBGetDefaultXY(dst_twp, op, &x, &y);

		DmDelObjectFromContainer(src_cp, op);
		if (DmAddShortcut(dst_twp, op, x, y, 0) != (int)OL_NO_ITEM) {
			ret = 1;
		}
		else {
			(void)DmAddObjectToContainer(src_cp, op, 0);
			ret = 0;
		}
	}
	else {
		/* toolbox is NOT open */
		int save_x, save_y;

		DmDelObjectFromContainer(src_cp, op);
		
		save_x = op->x;
		save_y = op->y;
		op->x = op->y = 0;
		if (DmAddObjectToContainer(dst_cp, op, DM_B_CHECK_DUP)) {
			/* put it back */
			op->x = save_x;
			op->y = save_y;
			(void)DmAddObjectToContainer(src_cp, op, 0);
			ret = 0;
		}
		else
			ret = 1;
	} /* toolbox is NOT open */

	if (ret) {
		/* success */
		free(save_name);
		if (op->ftype == DM_FTYPE_TOOLBOX) {
			DmContainerPtr cp;

			/* update parent info */
			cp = DmGetToolboxInfo(op->name);
			TBINFO(cp)->parent = dst_cp;
		}
	}
	else {
		/* failed! restore name */
		free(op->name);
		op->name = save_name;
	}
	return(ret);
}

void
DmUnmanageIcon(twp, idx)
DmToolboxWinPtr twp;
int idx;
{
	XtSetArg(Dm__arg[0], XtNmanaged, False);
	OlFlatSetValues(twp->box, idx, Dm__arg, 1);
	_OlFlatLayoutIconBox(twp->box, 0, 0);
}

/*
 * This function installs a shortcut icon in the specified toolbox.
 * possible return codes are:
 *
 *	0 - success!
 *	1 - specified toolbox doesn't exist.
 *	2 - path doesn't exist.
 *	3 - general failure. (memory...)
 *	4 - duplicate icon
 */
int
DmInstallShortcut(label, path, toolbox_name)
char *label;
char *path;
char *toolbox_name;
{
	DmContainerPtr cp;
	DmObjectPtr real_op, op;
	DmToolboxWinPtr twp;

	if ((cp = DmGetToolboxInfo(toolbox_name)) == NULL)
		return(1);

	/* check the path */
	if ((real_op = DmFileToObject(path)) == NULL)
		return(2);

	/* create a new object */
	if ((op = (DmObjectPtr)calloc(1, sizeof(DmObjectRec))) == NULL) {
		DmCloseContainer(real_op->container);
		return(3);
	}

	op->objectdata = real_op;
	op->name = strdup(label);
	op->x = op->y = 0;
	op->attrs |= DM_B_SHORTCUT;
	(void)DtSetProperty(&(op->plist), REAL_PATH, path, 0);
	(void)DtSetProperty(&(op->plist), OBJECT_TYPE, "FILE", 0);

	if (twp = DmQueryToolboxWindow(cp->path)) {
		DmInitObjType(twp->shell, real_op);
		op->fcp = real_op->fcp;
		op->ftype = real_op->ftype;
		if (DmNewObjectInWindow(twp, op) != OL_NO_ITEM)
			return(0);
		else
			return(4);
	}
	else {
		/* just add it to the toolbox container list */
		if (DmAddObjectToContainer(cp, op, DM_B_CHECK_DUP)) {
			/* failed */
			return(4);
		}
		else
			return(0);
	}
}

void
DmDelToolbox(name)
char *name;
{
	DmContainerPtr del_cp = DmGetToolboxInfo(name);
	DmContainerPtr parent_cp;
	DmContainerPtr cp;
	DmObjectPtr op;
	DmToolboxWinPtr twp;

	/* close the window first */
	if (twp = DmQueryToolboxWindow(name))
		DmCloseToolboxWindow(twp);

	/* save the ptr to parent */
	parent_cp = TBINFO(del_cp)->parent;

	/* remove cp from the list first */
	cp = DESKTOP_TCP(Desktop);
	if (del_cp == cp)
		DESKTOP_TCP(Desktop) = del_cp->next;
	else {
		while (cp->next != del_cp)
			cp = cp->next;
		cp->next = del_cp->next;
	}

	/* remove object from the parent container */
	if (twp = DmContainerToToolboxWindow(parent_cp)) {
		int idx;

		if ((idx = Dm__ItemNameToIndex(twp->itp, twp->nitems,
				 del_cp->path)) != OL_NO_ITEM)
			/* delete item first */
			DmUnmanageIcon(twp, idx);
	}

	if (op = DmGetObjectInContainer(parent_cp, del_cp->path)) {
		DmDelObjectFromContainer(parent_cp, op);
		op->objectdata = NULL;
		Dm__FreeObject(op);
	}


	/* delete TB info file */
	(void)unlink(TBINFO(del_cp)->path);

	/* free del_cp */
	free(del_cp->path);
	free(TBINFO(del_cp)->path);
	free(TBINFO(del_cp));
	free(del_cp);
}

int
DmDelShortcut(cp, op)
DmContainerPtr cp;
DmObjectPtr op;
{
	if (op->ftype == DM_FTYPE_TOOLBOX) {
		/* delete toolbox */
		DmContainerPtr dst_cp;

		dst_cp = DmGetToolboxInfo(op->name);

		/* make sure it is empty first */
		if (dst_cp->num_objs) {
			int i = dst_cp->num_objs;
			DmObjectPtr op2 = dst_cp->op;

			for (; i; i--, op2 = op2->next)
				DmDelShortcut(dst_cp, op2);
		}

		if (TBINFO(dst_cp)->parent == NULL)
			return(2);

		DmDelToolbox(op->name);
	}
	else {
		DmObjectPtr real_op = (DmObjectPtr)(op->objectdata);
		DmToolboxWinPtr twp = DmQueryToolboxWindow(cp->path);

		/* deleting shortcut */
		DmDelObjectFromContainer(cp, op);

		if (twp) {
			/* find item given op */
			int item_index = DmObjectToIndex((DmWinPtr)twp, op);

			if (item_index != OL_NO_ITEM)
				DmUnmanageIcon(twp, item_index);
		}

		op->fcp = NULL; /* not real, just a copy from real_op */
		op->objectdata = NULL;
		Dm__FreeObject(op);

		/* close container containing the real op */
		if (real_op)
			DmCloseContainer(real_op->container, DM_B_NO_FLUSH);
	}

	return(0);
}

int
DmChangeToolboxName(old_name, new_name)
char *old_name;
char *new_name;
{
	DmContainerPtr cp;
	DmToolboxWinPtr twp;

	/* check if there is a collision with new_name */
	if (DmGetToolboxInfo(new_name))
		return(1);

	/* update container name */
	cp = DmGetToolboxInfo(old_name);
	free(cp->path);
	cp->path = strdup(new_name);

	if (twp = DmQueryToolboxWindow(new_name)) {
		/* change the title */
		XtSetArg(Dm__arg[0], XtNtitle, new_name);
		XtSetValues(twp->shell, Dm__arg, 1);
	}

	return(0);
}

