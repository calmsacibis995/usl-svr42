/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtm:tb_init.c	1.4"
#endif

#include <limits.h>
#include <X11/Intrinsic.h>
#include "Dtm.h"
#include "extern.h"

static char *sep;

static void
GetTBInfo(parent, path, tbname)
DmContainerPtr parent;
char *path;	/* path of toolbox info file */
char *tbname;	/* name of toolbox */
{
	DmContainerPtr cp;
	DmObjectPtr op;
	char *p;
	DmTBContainerInfoPtr tbcp;

	cp = Dm__NewContainer(tbname);
	DmReadDtInfo(cp, path, 0);
	tbcp = (DmTBContainerInfoPtr)malloc(sizeof(DmTBContainerInfoRec));
	tbcp->path = strdup(path);
	tbcp->parent = parent;
	cp->data = (void *)tbcp;
	cp->count = 1;
	
	cp->next = DESKTOP_TCP(Desktop);
	DESKTOP_TCP(Desktop) = cp;

	/* check for toolboxes */
	for (op=cp->op; op; op=op->next) {
		if (p = DtGetProperty(&(op->plist), TOOLBOX_FILE, NULL)) {
			/* a toolbox container */
			op->ftype = DM_FTYPE_TOOLBOX;
			strcpy(sep, p);

			GetTBInfo(cp, path, op->name);
		}
	}
}

/*
 * This routine will
 *	- read the ".toolbox" file and create a container description for it.
 *	- then recursively read the entire hierarchy.
 */

void
DmInitToolboxInfo(path, name)
char *path;
char *name;
{
	char buffer[PATH_MAX];
	int len;

	strcpy(buffer, path);
	len = strlen(buffer);
	sep = buffer + len;
	*sep++ = '/';
	strcpy(sep, name);
	GetTBInfo(NULL, buffer, TOOLBOXROOT);
}

