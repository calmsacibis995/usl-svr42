/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* ident	"@(#)dtm:CListGizmo.c	1.21" */

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/RectObj.h>
#include <Xol/OpenLook.h>
#include <Xol/ScrolledWi.h>
#include <Gizmo/Gizmos.h>
#include "Dtm.h"
#include "extern.h"
#include "CListGizmo.h"

static Widget		CreateCListGizmo();
static void		FreeCListGizmo();
static Gizmo		CopyCListGizmo();
static XtPointer	QueryCListGizmo();

GizmoClassRec CListGizmoClass[] = {
	"CListGizmo",
	CreateCListGizmo,	/* Create	*/
	CopyCListGizmo,		/* Copy		*/
	FreeCListGizmo,		/* Free		*/
	NULL,			/* Map		*/
	NULL,			/* Get		*/
	NULL,			/* Get Menu	*/
	NULL,			/* Build	*/
	NULL,			/* Manipulate	*/
	QueryCListGizmo		/* Query	*/
};

static Gizmo
CopyCListGizmo (g)
CListGizmo *g;
{
	CListGizmo * new = (CListGizmo *) MALLOC (
		sizeof (CListGizmo)
	);

	new->name       = g->name ? STRDUP(g->name) : NULL;
	new->width      = g->width;
	new->file  	= g->file;
	new->sys_class  = g->sys_class;
	new->xenix_class  = g->xenix_class;
	new->usr_class  = g->usr_class;
	new->req_prop   = g->req_prop ? STRDUP(g->req_prop) : NULL;
	new->overridden = g->overridden;
	new->exclusives = g->exclusives;
	new->noneset    = g->noneset;
	new->selectProc = g->selectProc;
	new->swinWidget = NULL;
	new->boxWidget  = NULL;

	return (Gizmo)new;
}

static void
FreeCListGizmo (gizmo)
CListGizmo *	gizmo;
{
	int	i;

	XtFree(gizmo->name);
	XtFree(gizmo->name);

	DmCloseContainer(gizmo->cp, DM_B_NO_FLUSH);
	FREE((void *)gizmo);
}

static void
DmAddClass(w, cp, name, fcp, viewable)
Widget w;
DmContainerPtr cp;
char *name;
DmFclassPtr fcp;
Boolean viewable;
{
	DmObjectPtr op;

	if (!name)
		name = DmClassName(fcp);

	op = Dm__NewObject(cp, name);
	if (viewable == False) {
		op->attrs |= DM_B_HIDDEN;
		op->fcp = fcp;
	}
	else {
		op->fcp = fcp;
		DmInitObjType(w, op);
	}
}

/*
 * Checks if a class should be included.
 */
static void
IncludeClass(parent, g, fmkp)
Widget parent;
CListGizmo *g;
DmFmodeKeyPtr fmkp; /* and DmFnameKeyPtr too */
{
	/* include overridden file classes */
	if (!(g->overridden) && (fmkp->attrs & DM_B_OVERRIDDEN))
			return;

	/* If no data file or folder, check req_prop.  If req_prop
	 * is set and the property is not found, exclude the file type.
	 */
	if (g->req_prop && strcmp(fmkp->name, "DIR") &&
	    strcmp(fmkp->name, "DATA") &&
	    (DtGetProperty(&(fmkp->fcp->plist), g->req_prop, NULL) == NULL))
				return;

	DmAddClass(parent, g->cp, NULL, fmkp->fcp, True);
}

static Widget
CreateCListGizmo (parent, g)
Widget parent;
CListGizmo *g;
{
	Dimension width, height, vpad, hpad;
	DmContainerPtr cp;
	DmObjectPtr op;
	DmItemPtr ip;
	int i;

	if ((cp = (DmContainerPtr)calloc(1, sizeof(DmContainerRec))) == NULL)
		return(NULL);

	cp->count	= 1;
	cp->num_objs	= 0;
	g->cp = cp;
	
	if (g->sys_class != False) {
		DmFmodeKeyPtr fmkp = DESKTOP_FMKP(Desktop);

		for (; fmkp->ftype != DM_FTYPE_SEM; fmkp++)
			IncludeClass(parent, g, fmkp);
	}
	if (g->sys_class == False && g->xenix_class != False) {
		DmFmodeKeyPtr fmkp = DESKTOP_FMKP(Desktop);

		for (; fmkp->ftype != DM_FTYPE_UNK; fmkp++)
			IncludeClass(parent, g, fmkp);
	}

	if (g->usr_class != False) {
		DmFnameKeyPtr fnkp = DESKTOP_FNKP(Desktop);

		for (; fnkp; fnkp = fnkp->next)
			if (fnkp->attrs & DM_B_CLASSFILE) {
			    if (g->file)
				/*
				 * Add fnkp entries as hidden objects.
				 * used by Icon Setup.
				 */
				DmAddClass(parent,cp, fnkp->name, fnkp, False);
			    else
				continue;

			}
			else
				IncludeClass(parent, g, (DmFmodeKeyPtr)fnkp);
	}

	vpad   = OlScreenMMToPixel(OL_VERTICAL, 2, DESKTOP_SCREEN(Desktop));
	hpad   = OlScreenMMToPixel(OL_HORIZONTAL,4, DESKTOP_SCREEN(Desktop));
	width  = (GRID_WIDTH(Desktop) + hpad) * g->width;
	if (g->cp->num_objs > g->width)
		/* reserve room for horizontal scrollbar */
		height = GRID_HEIGHT(Desktop) + vpad * 6;
	else
		height = GRID_HEIGHT(Desktop) + vpad * 2;

	/* create scrolled window */
	XtSetArg(Dm__arg[0], XtNhStepSize, GRID_WIDTH(Desktop) + hpad);
	XtSetArg(Dm__arg[1], XtNviewWidth, width);
	XtSetArg(Dm__arg[2], XtNviewHeight, height);
	XtSetArg(Dm__arg[3], XtNrecomputeHeight, False);
	g->swinWidget = XtCreateManagedWidget("ScrolledWin",
				scrolledWindowWidgetClass, parent,
				Dm__arg, 4);

	XtSetArg(Dm__arg[0], XtNdrawProc, DmDrawIcon);
	XtSetArg(Dm__arg[1], XtNfont,	  DESKTOP_FONT(Desktop));
	XtSetArg(Dm__arg[2], XtNmovableIcons,  False);
	XtSetArg(Dm__arg[3], XtNexclusives, g->exclusives);
	XtSetArg(Dm__arg[4], XtNnoneSet, g->noneset);
	XtSetArg(Dm__arg[5], XtNselectProc, g->selectProc);
	i = 6;

	g->boxWidget = DmCreateIconContainer(g->swinWidget,
					DM_B_CALC_SIZE | DM_B_NO_INIT,
					Dm__arg, i,
					cp->op,
					cp->num_objs,
					&(g->itp),
					cp->num_objs,
					NULL,
					DESKTOP_FONTLIST(Desktop),
					DESKTOP_FONT(Desktop),
					DM_FontHeight(Desktop));

	LayoutCListGizmo(g, False);

	return(g->swinWidget);
}

void
LayoutCListGizmo(CListGizmo *g, Boolean new_list)
{
	DmItemPtr ip;
	int i;
	Position x;
	Dimension height, vpad, hpad;

	if (!(g->boxWidget))
		return;

	vpad   = OlScreenMMToPixel(OL_VERTICAL, 2, DESKTOP_SCREEN(Desktop));
	hpad   = OlScreenMMToPixel(OL_HORIZONTAL, 4, DESKTOP_SCREEN(Desktop));
	height = GRID_HEIGHT(Desktop) + vpad;

	/* update each item's x,y so that they all line up as one row */
	for (ip=g->itp, i=g->cp->num_objs, x=vpad; i; i--, ip++)
		if (ITEM_MANAGED(ip) != False) {
			ip->x = (XtArgVal)x;
			ip->y = (XtArgVal)(height - ITEM_HEIGHT(ip));
			x += ITEM_WIDTH(ip) + hpad;
		}

	XtSetArg(Dm__arg[0], XtNitemsTouched, True);
	XtSetArg(Dm__arg[1], XtNitems, g->itp);
	XtSetArg(Dm__arg[2], XtNnumItems, g->cp->num_objs);
	XtSetValues(g->boxWidget, Dm__arg, 3);
}

void
ChangeCListItemLabel(g, idx, label)
CListGizmo *g;
int idx;
char *label;
{
	DmItemPtr ip = g->itp + idx;

	free(ITEM_LABEL(ip));
	ip->label = (XtArgVal)strdup(label);
	DmSizeIcon(ip, DESKTOP_FONTLIST(Desktop), DESKTOP_FONT(Desktop));

	LayoutCListGizmo(g, False);
}

void
ChangeCListItemGlyph(g, idx)
CListGizmo *g;
int idx;
{
	if (g->boxWidget) {
		DmItemPtr ip = g->itp + idx;
		DmObjectPtr op = ITEM_OBJ(ip);
		Dimension width, height;

		width = ITEM_WIDTH(ip);
		height = ITEM_HEIGHT(ip);
		if (op->fcp->glyph) {
			DmReleasePixmap(XtScreen(g->boxWidget),op->fcp->glyph);
			DmReleasePixmap(XtScreen(g->boxWidget),op->fcp->cursor);
		}

		/* reset attributes */
		op->fcp->attrs &= ~(DM_B_VAR | DM_B_FREE);
		DmInitObjType(g->boxWidget, op);
		DmSizeIcon(ip, DESKTOP_FONTLIST(Desktop),
			       DESKTOP_FONT(Desktop));

		if ((width != ITEM_WIDTH(ip)) || (height != ITEM_HEIGHT(ip)))
			LayoutCListGizmo(g, False);
		else
			OlFlatRefreshItem(g->boxWidget, idx, True);
	}
}

static XtPointer
QueryCListGizmo(CListGizmo *gizmo, int option, char * name)
{
   if (!name || !strcmp(name, gizmo->name)) {
      switch(option) {
         case GetGizmoSetting:
            return (XtPointer)(NULL);
            break;
         case GetGizmoWidget:
            return (XtPointer)(gizmo->swinWidget);
            break;
         case GetGizmoGizmo:
            return (XtPointer)(gizmo);
            break;
         default:
            return (XtPointer)(NULL);
            break;
      }
   }
   else
      return (XtPointer)(NULL);

} /* end of QuerySWinGizmo */

