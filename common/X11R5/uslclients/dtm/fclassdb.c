/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* ident	"@(#)dtm:fclassdb.c	1.14" */

#include <libgen.h>
#include <X11/Intrinsic.h>
#include "Dtm.h"
#include "extern.h"

static char *
ResolveSymLink(op, real_path, real_name)
DmObjectPtr op;
char **real_path;
char **real_name;
{
	char *p = DmObjPath(op);
	char *p2;

	/* Make a copy, because Dm__buffer will be used in the next stmt. */
	p = strdup(p);

	if (realpath(p, Dm__buffer) == NULL) {
		free(p);
		*real_name = NULL;
		return(*real_path = NULL);
	}
	else {
		free(p);
		p = strdup(Dm__buffer);
		*real_name = basename(p);
		*real_path = dirname(p);
		return(p);
	}
}

void
Dm__SetFileClass(op)
DmObjectPtr op;
{
	register DmFnameKeyPtr fnkp = DESKTOP_FNKP(Desktop);
	register char *p;
	char *path;
	char *name;
	char *free_this = NULL;
	char *real_path;
	char *real_name;

#ifdef TOOLBOX
	if (op->attrs & DM_B_SHORTCUT) {
		p = DtGetProperty(&(op->plist), REAL_PATH, NULL);
		path = strdup(p);
		name = strrchr(path, '/');
		*name = '\0';
		name++;
	}
	else {
#endif
		path = op->container->path;
		name = op->name;
#ifdef TOOLBOX
	}
#endif

	for (; fnkp; fnkp = fnkp->next) {
		/*
		 * Skip new entries that haven't been applied yet.
		 * Note that deleted entries that haven't been applied are
		 * still being used.
		 */
		if (fnkp->attrs & (DM_B_NEW | DM_B_CLASSFILE | DM_B_OVERRIDDEN))
			continue;

		if ((fnkp->attrs & DM_B_FILETYPE) && (op->ftype != fnkp->ftype))
				continue;

		if (fnkp->attrs & DM_B_FILEPATH) {
		    p = DtGetProperty(&(fnkp->fcp->plist), FILEPATH, NULL);
		    p = Dm__expand_sh(p, DmObjProp, (XtPointer)op);

		    if (strcmp((p[0] == '/' && p[1] == '/') ? p + 1 : p,
			       path)) {
			free(p);
			continue;
		    }
		    free(p);
		}

		if (fnkp->attrs & DM_B_LFILEPATH)
		{
		    if ( !(op->attrs & DM_B_SYMLINK) )
			continue;

		    if (free_this == NULL) {
			free_this = ResolveSymLink(op, &real_path, &real_name);

			if (free_this == NULL)	/* can't resolve path */
			    continue;
		    }

		    p = DtGetProperty(&(fnkp->fcp->plist), LFILEPATH, NULL);
		    p = Dm__expand_sh(p, DmObjProp, (XtPointer)op);

		    if (strcmp((p[0] == '/' && p[1] == '/') ? p + 1 : p,
			       real_path)) {
			free(p);
			continue;
		    }
		    free(p);
		}

		if (fnkp->attrs & DM_B_REGEXP) {
			char *p2;

			for (p2=fnkp->re; *p2; p2=p2+strlen(p2)+1) {
				if (gmatch(name, p2))
					/* found it */
					break;
			}

			if (*p2 == '\0')
				continue;
		}

		if (fnkp->attrs & DM_B_LREGEXP) {
		    char *p2;

		    if ( !(op->attrs & DM_B_SYMLINK) )
			continue;

		    if (free_this == NULL) {
			free_this = ResolveSymLink(op, &real_path, &real_name);
			if (free_this = NULL)	/* can't resolve path */
			    continue;
		    }

		    for (p2=fnkp->lre; *p2; p2=p2+strlen(p2)+1) {
			if (gmatch(real_name, p2))
			    /* found it */
			    break;
		    }

		    if (*p2 == '\0')
			continue;
		}

		/* found it! */
		op->fcp = fnkp->fcp;
		break;
	}

#ifdef TOOLBOX
	if (op->attrs & DM_B_SHORTCUT)
		free(path);
#endif
	if (free_this)
		free(free_this);
}

void
DmFreeFileClass(fnkp)
DmFnameKeyPtr fnkp;
{
#ifdef NOT_USE
	XtFree(fnkp->name);
	XtFree(fnkp->re);

	if (fnkp->fcp) {
		DtFreePropertyList(&(fnkp->fcp->plist));
		if (fnkp->fcp->glyph) {
			DmReleasePixmap(DESKTOP_SCREEN(Desktop),
					fnkp->fcp->glyph);
			DmReleasePixmap(DESKTOP_SCREEN(Desktop),
					fnkp->fcp->cursor);
		}
		free(fnkp->fcp);
	}

	free(fnkp);
#endif
}

