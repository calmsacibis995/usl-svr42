/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:f_layout.c	1.40"

/******************************file*header********************************

    Description:
	This file contains the source code related to laying out items in a folder.
*/
						/* #includes go here	*/
#include <grp.h>
#include <libgen.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>

#include "Dtm.h"
#include "extern.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Public  Procedures 
*/
					/* private procedures		*/
static char *	GetFilePermission(DmItemPtr itemp);

					/* public procedures		*/
void		DmComputeLayout(Widget, DmItemPtr, int, int, Dimension,
				DtAttrs, DtAttrs);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
    GetFilePermission-
*/
static char *
GetFilePermission(DmItemPtr itemp)
{
#define MAX_PERM_CHAR 10
    register int	x;
    DmFileInfoPtr	finfo = FILEINFO_PTR(itemp);
    static char		buf[MAX_PERM_CHAR];
    static const int	perms[] = {
	S_IRUSR,S_IWUSR,S_IXUSR,
	S_IRGRP,S_IWGRP,S_IXGRP,
	S_IROTH,S_IWOTH,S_IXOTH
    };
    static const char permc[] = {'r','w','x', 'r','w','x', 'r','w','x'};

    for (x = 0; x < MAX_PERM_CHAR - 1; x++)
	buf[x] = (finfo->mode & perms[x]) ? permc[x] : '-';

    return(buf);
}					/* end of GetFilePermission */

/***************************private*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
    DmComputeLayout-
*/
void
DmComputeLayout(Widget w, DmItemPtr itemp, int count, int type,
		Dimension width,
		DtAttrs geom_options,	/* size, icon position options */
		DtAttrs layout_options)	/* layout attributes */
{
    DmItemPtr	item;
    int		maxlen = 0;
    Dimension	margin = Ol_PointToPixel(OL_HORIZONTAL, ICON_MARGIN);
    Dimension	pad = Ol_PointToPixel(OL_HORIZONTAL, INTER_ICON_PAD);
    Position	x = margin;
    Position	y = margin;
    Position	center_x;
    Dimension	grid_width;
    Dimension	row_height;
    Dimension	item_width;

    if (layout_options & SAVE_ICON_POS)
	DmSaveXandY(itemp, count);

    if (width < GRID_WIDTH(Desktop))
	width = GRID_WIDTH(Desktop);

    switch(type)
    {
    case DM_NAME:	
        DmInitSmallIcons(DESKTOP_SHELL(Desktop));

	/* Compute row height and grid width outside of loop */
	row_height = (geom_options & DM_B_CALC_SIZE) ?
	    DM_NameRowHeight(Desktop) + pad : ITEM_HEIGHT(itemp) + pad;
	grid_width = GRID_WIDTH(Desktop);

	for (item = itemp; item < itemp + count; item++)
	{
	    if(!ITEM_MANAGED(item))
		continue;

	    if (layout_options & UPDATE_LABEL)
	    {
		XtFree(ITEM_LABEL(item));
		item->label = (XtArgVal)
		    strdup( Dm__MakeItemLabel(item, type, 0) );
	    }

	    if (geom_options & DM_B_CALC_SIZE)
		DmComputeItemSize(item, type,
				    (Dimension *)&item->icon_width,
				    (Dimension *)&item->icon_height);
	    item_width = ITEM_WIDTH(item);

	    /* Wrap now if item will extend beyond "wrap" width */
	    if ((x != margin) &&
		((Dimension)(x + item_width) > width))

	    {
		x = margin;
		y += row_height;
	    }

	    item->x = x;
	    item->y = y;

	    x += (item_width <= grid_width) ? grid_width :
		((Dimension)(item_width + grid_width - 1) / grid_width) *
		    grid_width;
	}
    break;

    case DM_ICONIC:
	/* Compute row height and grid width outside of loop */
	row_height = GRID_HEIGHT(Desktop);
	grid_width = GRID_WIDTH(Desktop);
	center_x = x + (grid_width / 2);

	for (item = itemp; item < itemp + count; item++)
	{
	    if(!ITEM_MANAGED(item))
		continue;

	    if (layout_options & UPDATE_LABEL)
	    {
		XtFree(ITEM_LABEL(item));
		item->label = (XtArgVal)
		    strdup( Dm__MakeItemLabel(item, type, 0) );
	    }

	    if (geom_options & DM_B_CALC_SIZE)
		DmComputeItemSize(item, type,
				    (Dimension *)&item->icon_width,
				    (Dimension *)&item->icon_height);

	    if (layout_options & RESTORE_ICON_POS) {
		if (((ITEM_OBJ(item))->x != 0) ||
		    ((ITEM_OBJ(item))->y != 0)) {
		    item->x = ITEM_OBJ(item)->x;
		    item->y = ITEM_OBJ(item)->y;
		}
	    }
	}

	for (item = itemp; item < itemp + count; item++)
	{
	    if(!ITEM_MANAGED(item))
		continue;

	    /* Since icons are moveable in iconic view, x & y are
	       saved/restored when switching views.
	    */
	    if (layout_options & RESTORE_ICON_POS)
	    {
		if (((ITEM_OBJ(item))->x == 0) &&
		    ((ITEM_OBJ(item))->y == 0))
		{
		    DmGetAvailIconPos(itemp, count,
				      ITEM_WIDTH(item), ITEM_HEIGHT(item),
				      width,
				      GRID_WIDTH(Desktop), GRID_HEIGHT(Desktop),
				      (Position *)&item->x,
				      (Position *)&item->y);
		} else
		{
		    item->x = ITEM_OBJ(item)->x;
		    item->y = ITEM_OBJ(item)->y;
		}

	    } else
	    {
		Dimension delta;
		Position next_x;

		item_width = ITEM_WIDTH(item);
again:
		/* Horiz: centered */
		center_x = x + (grid_width / 2);
		next_x = center_x - (item_width / 2);

		/* Wrap now if item will extend beyond "wrap" width */
		if ((x != margin) &&
		    ((Dimension)(x + grid_width) > width))
		{
		    x = margin;
		    y += row_height;
		    goto again;
		}
		/* Vert: bottom justified */
		item->y =
		    VertJustifyInGrid(y, ITEM_HEIGHT(item), row_height, delta);

		item->x = next_x;

		x += grid_width;
		center_x += grid_width / 2;		/* next center */
	    }
	}
	break;

    case DM_LONG:
        DmInitSmallIcons(DESKTOP_SHELL(Desktop));

	/* Compute row height and max label length outside of loop */
	row_height = DM_LongRowHeight(Desktop) + pad;
	maxlen = ItemLabelsMaxLen(itemp, count);

	for (item = itemp; item < itemp + count; item++)
	{
	    if (!ITEM_MANAGED(item))
		continue;

	    XtFree(ITEM_LABEL(item));

	    item->label = (XtArgVal)
		strdup( Dm__MakeItemLabel(item, DM_LONG, maxlen) );

	    DmComputeItemSize(item, DM_LONG,
				(Dimension *)&item->icon_width,
				(Dimension *)&item->icon_height);
	    item->x = x;
	    item->y = y;

	    /* we always assume one column in long format */
	    y += row_height;
	}
	break;

    default:
	break;
    }
}

/****************************procedure*header*****************************
    DmGetLongName-
*/
char *
DmGetLongName(DmItemPtr item, int len)
{
    static char		buffer[512];
    struct passwd *	pw;
    struct group *	gr;
    int			length;
    char		user_buf[512];
    char		group_buf[512];
    char		time_buf[256];
    char *		user_name;
    char *		group_name;

    buffer[0] = '\0';

    /* For user and group names, use the number if no group defined */
    if ( (pw = getpwuid(FILEINFO_PTR(item)->uid)) == NULL )
    {
	sprintf(user_buf, "%d", FILEINFO_PTR(item)->uid);
	user_name = user_buf;

    } else
	user_name = pw->pw_name;

    if ( (gr = getgrgid(FILEINFO_PTR(item)->gid)) == NULL )
    {
	sprintf(group_buf, "%d", FILEINFO_PTR(item)->gid);
	group_name = group_buf;

    } else
	group_name = gr->gr_name;

    (void)strftime(time_buf, sizeof(time_buf), TIME_FORMAT,
	     localtime(&FILEINFO_PTR(item)->mtime));
    length = sprintf(buffer, "%-*s%-10s%-10s%-10s%7ld %s",
		     len, DmGetObjectName(ITEM_OBJ(item)),
		     GetFilePermission(item),
		     user_name,
		     group_name,
		     FILEINFO_PTR(item)->size, time_buf);

    return(buffer);
}				/* end of DmGetLongName */

void
DmSaveXandY(DmItemPtr item, int count)
{

    DmItemPtr itemp;
    int	i;

    for (i=0, itemp = item; i < count; i++, itemp++)
	if (ITEM_MANAGED(itemp))
	{
	    ITEM_OBJ(itemp)->x = itemp->x;
	    ITEM_OBJ(itemp)->y = itemp->y;
	}
}

/****************************procedure*header*****************************
    DmComputeItemSize-
*/
void
DmComputeItemSize(DmItemPtr item, DmViewFormatType view_type,
		    Dimension * width, Dimension * height)
{
    DmFmodeKeyPtr fmkptr = DmFtypeToFmodeKey(ITEM_OBJ(item)->ftype);

    switch(view_type)
    {
    case DM_ICONIC :
	/* NOTE: DmSizeIcon will set the dimension in the item instead of
	   just returning the width and height.  There is currently no
	   interface to just get the dimension of an item in ICONIC view.
	*/
	DmSizeIcon(item, DESKTOP_FONTLIST(Desktop), DESKTOP_FONT(Desktop));
	*width = (Dimension)item->icon_width;
	*height = (Dimension)item->icon_height;
	break;

    case DM_NAME :
        DmInitSmallIcons(DESKTOP_SHELL(Desktop));
	*width = fmkptr->small_icon->width + ICON_LABEL_GAP + 2 * ICON_PADDING +
	    DmTextWidth(Desktop, ITEM_LABEL(item), strlen(ITEM_LABEL(item)));

	*height = DM_NameRowHeight(Desktop);
	break;

    case DM_LONG :
	/* Since LONG view uses a fixed-width font, the optimization for
	   calculating the label width is possible.
	*/
        DmInitSmallIcons(DESKTOP_SHELL(Desktop));
	*width = fmkptr->small_icon->width + ICON_LABEL_GAP + 2 * ICON_PADDING +
	    DM_TextWidth(DESKTOP_FIXED_FONT(Desktop), DESKTOP_FONTLIST(Desktop),
			 ITEM_LABEL(item), strlen(ITEM_LABEL(item)));
	*height = DM_LongRowHeight(Desktop);
	break;

    default:
	break;
    }
}					/* end of DmComputeItemSize */

int
ItemLabelsMaxLen(DmItemPtr item, int count)
{
    register int	i;
    register int	len;
    register int	max_len;

    max_len = 0;

    for (i = 0; i < count; i++, item++)
	if ((len = strlen(DmGetObjectName(ITEM_OBJ(item)))) > max_len)
	    max_len = len;

    return(max_len);
}				/* end of ItemLabelsMaxLen() */
