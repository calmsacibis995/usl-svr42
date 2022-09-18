/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtadmin:fontmgr/font_view.c	1.15"

/*
 * Module:     dtadmin:fontmgr   Graphical Administration of Fonts
 * File:       font_view.c
 */

#include <IntrinsicP.h>
#include <Xatom.h>
#include <Caption.h>
#include <OpenLookP.h>
#include <Intrinsic.h>
#include <StringDefs.h>
#include <FExclusive.h>
#include <FNonexclus.h>
#include <StaticText.h>
#include <TextEdit.h>
#include <ControlAre.h>
#include <TextField.h>
#include <IntegerFie.h>
#include <FList.h>

#include <fontmgr.h>

extern Widget       app_shellW;		  /* application shell widget       */
extern Boolean C_locale;

static void UpdatePS();
extern String GetFontName( String);

Boolean show_xlfd;


void
DisplayFont( view_type *view)
{
    XFontStruct * font;
    XFontStruct * new_font;
    Arg args[5];
    Cardinal n;
    char buf[MAX_PATH_STRING];
    static Boolean first_time=TRUE;

    /*  Load the font and set it in the text widget.  */
#ifdef SERVER_CANT_CACL_RESOL
    new_font = _OlGetDefaultFont(app_shellW, view->cur_xlfd);
#else
    new_font = XLoadQueryFont(XtDisplay(app_shellW), view->cur_xlfd);
#endif

    if (new_font == NULL)  {
	if (show_xlfd)
	    sprintf(buf, "Can't display %s", view->cur_xlfd);
	else
	    sprintf(buf, "Can't display %s", GetFontName(view->cur_xlfd));
	InformUser(buf);
	if (!view->bitmap)
	    StandardCursor(0);
	strcpy(view->cur_xlfd, view->prev_xlfd);
	return;
    }

/***********
 * don't free any fonts, let the X-server cache it
    if (first_time)
	first_time = FALSE;
    else {
	n = 0;
	XtSetArg(args[n], XtNfont, &font);	n++;
	XtGetValues(view->sample_text, args, n);
	XFreeFont(XtDisplay(app_shellW), font);
    }
************/

    /*  Change the sample_text to show the current font.  */
    n = 0;
    XtSetArg(args[n], XtNfont, new_font);	n++;
    XtSetValues(view->sample_text, args, n);
	  
    if (show_xlfd)
	InformUser(view->cur_xlfd);
    else
	InformUser(GetFontName(view->cur_xlfd));

    if (!view->bitmap)
	StandardCursor(0);

} /* end of DisplayFont */


static void
UpdateSample (view_type *view, int item_index)
{
    static char buf[MAX_PATH_STRING];
    String p;
    int i=0;
    font_type *font_data = _OlArenaElement(view->ps_arena, item_index).l;
    String xlfd_name;

    xlfd_name = font_data->xlfd_name;
    strcpy(view->prev_xlfd, view->cur_xlfd);
    
    if (view->bitmap) {
	/* search for the x-resolution field */
	for(p=xlfd_name; *p && (i < 9); p++) {
	    if (*p == DELIM)
		i++;
	}
	strncpy(view->cur_xlfd, xlfd_name, p-xlfd_name);
	view->cur_xlfd[p-xlfd_name] = 0; /* terminator */
	
	/* wild-card the resolution fields */
	strcat(view->cur_xlfd, "*-*-");
	
	/* skip the resolution fields */
	for(i=0; *p && (i < 2); p++) {
	    if (*p == DELIM)
		i++;
	}
	strcat(view->cur_xlfd, p);
    }
    else {
	BusyCursor(0);

	/* search for the pointsize field */
	for(p=xlfd_name; *p && (i < 8); p++) {
	    if (*p == DELIM)
		i++;
	}
	strncpy(buf, xlfd_name, p-xlfd_name);
	buf[p-xlfd_name] = 0; /* terminator */
	
	/* skip the resolution fields */
	for(i=0; *p && (i < 3); p++) {
	    if (*p == DELIM)
		i++;
	}

	/* fill in the pointsize and wild-card the resolution fields */
	sprintf(view->cur_xlfd, "%s%d0-0-0-%s", buf, view->cur_size, p);
    }

    XSync(XtDisplay(app_shellW), FALSE);
    ScheduleWork(DisplayFont, view, 10);

}  /* end of UpdateSample() */


static void
UpdateLook (view_type *view, int family_index)
{
    int i;
    Widget look_exclusive = view->look_exclusive;
    _OlArenaType(PSArena) * ps_arena;
    look_info tmp;
    _OlArenaType(LookArena) * look_arena;

    look_arena = _OlArenaElement(view->family_arena, family_index).l;
    tmp.look_name = view->cur_look;
    _OlArenaFindHint(look_arena, &i, tmp);
    if (i >= _OlArenaSize(look_arena)) {
	i = _OlArenaSize(look_arena) - 1;
    }

    /* update the look list */
    XtVaSetValues(look_exclusive, 
		  XtNitems, look_arena->arena,
		  XtNnumItems, _OlArenaSize(look_arena),
		  0);
    OlVaFlatSetValues(look_exclusive, i, XtNset, True,(String) 0);
    /* can't set XtNviewItemIndex with other resources */
    XtVaSetValues(look_exclusive, XtNviewItemIndex, i, 0);

    /*  Display the selected font.  */
    ps_arena = _OlArenaElement(look_arena, i).l;
    UpdatePS(view, ps_arena, 0);

}  /* end of UpdateLook() */


static void
UpdatePS (view, ps_arena, delta)
    view_type *view;
    _OlArenaType(PSArena) * ps_arena;
    int delta;
{
    Cardinal org_item_index, item_index;
    char cur_size_str[MAX_STRING];
    Arg args[1];
    Widget size_exclusive = view->size_exclusive;
    int hint;
    font_type *font_data;
    ps_info tmp;

    view->ps_arena = ps_arena;
    XtSetArg(args[0], XtNuserData, &org_item_index);
    XtGetValues(size_exclusive, args, 1);

    /*  Find the current point size in the new ps_arena, so that the
	current point size is maintianed.  The hinted search returns
	the position that the current point size would have been
	inserted.  If the current size is not found, use the element
	at that index as the closest choice.  */
    sprintf(cur_size_str, "%d", view->cur_size);
    tmp.ps = cur_size_str;
    _OlArenaFindHint(ps_arena, &hint, tmp);
    hint += delta;
    if (hint >= (int) _OlArenaSize(ps_arena))
	item_index = _OlArenaSize(ps_arena)-1;
    else if (hint < 0)
	item_index = 0;
    else
	item_index = hint;
    
    /* if needed, update the index */
    if (org_item_index != item_index) {
	/*  Make sure that the userData contains the point size.  */
	XtVaSetValues(size_exclusive, XtNuserData, item_index, 0);
    }

    /*  Update the size_exclusive to have the valid point sizes in
	the ps_arena.  */
    XtVaSetValues (size_exclusive,
		   XtNitems, (XtArgVal)ps_arena->arena,
		   XtNnumItems, (XtArgVal)_OlArenaSize(ps_arena),
		   (String)0);
    OlVaFlatSetValues(size_exclusive, item_index, XtNset, True, (String)0);
    /* can't set XtNviewItemIndex with other resources */
    XtVaSetValues(size_exclusive, XtNviewItemIndex, item_index, 0);

    /* outline always have index of zero */
    font_data = _OlArenaElement(ps_arena, 0).l;
    view->bitmap = font_data->bitmap;
    if (view->bitmap) {
	XtUnmanageChild(view->ps_text);
	XtManageChild(view->size_window);
    }
    else {
	XtUnmanageChild(view->size_window);
	XtManageChild(view->ps_text);
	XtVaSetValues( view->ps_text, XtNvalue, view->cur_size, NULL);
	item_index = 0;
    }

    /*  Display the selected font.  */
    UpdateSample(view, item_index);
}  /* end of UpdatePS() */


static void
UpdateFamily (view_type *view, int family_index)
{
    XtVaSetValues(view->family_exclusive, 
		      XtNitems, view->family_arena->arena,
		      XtNnumItems, _OlArenaSize(view->family_arena),
		      (String) 0);
    OlVaFlatSetValues(view->family_exclusive, family_index, XtNset, True, 0);
    /* can't set XtNviewItemIndex with other resources */
    XtVaSetValues(view->family_exclusive, XtNviewItemIndex, family_index, 0);

    /*  Update the look and size exclusives */
    UpdateLook(view, family_index);

}  /* end of UpdateFamily() */


void
FamilySelectCB (w, client_data, call_data)
	Widget w;
	XtPointer client_data;
	XtPointer call_data;
{
        OlFlatCallData *        fd      = (OlFlatCallData *)call_data;
	view_type *view = (view_type *) client_data;
	int family_index = fd->item_index;

	UpdateLook(view, family_index);
	strcpy(view->cur_family, 
	       _OlArenaElement(view->family_arena, family_index).n);

}  /* end of FamilySelectCB() */


void
LookSelectCB (w, client_data, call_data)
	Widget w;
	XtPointer client_data;
	XtPointer call_data;
{
    OlFlatCallData *        fd      = (OlFlatCallData *)call_data;
    view_type *view = (view_type *) client_data;
    _OlArenaType(PSArena) * ps_arena;
    String look_str;

    OlVaFlatGetValues (w, fd->item_index,
		       XtNuserData, (XtArgVal) &ps_arena,
		       XtNlabel, &look_str,
		       (String)0);
    strcpy( view->cur_look, look_str);

    UpdatePS(view, ps_arena, 0);
}  /* end of LookSelectCB() */


void
PSSelectCB (w, client_data, call_data)
	Widget w;
	XtPointer client_data;
	XtPointer call_data;
{
    view_type *view = (view_type *) client_data;
    OlFlatCallData *        fd      = (OlFlatCallData *)call_data;
    char *cur_size_str;

    OlVaFlatGetValues (w, fd->item_index,
		       XtNlabel, &cur_size_str,
		       (String)0);
    view->cur_size = atoi( cur_size_str);

    /*  Make sure that the userData contains the set index.  */
    XtVaSetValues(w,
		XtNuserData, fd->item_index,
		(String)0);

    /*  Display the selected font.  */
    UpdateSample(view, fd->item_index);

}  /* end of PSSelectCB() */


static void
TimeOutHandler(view_type *view)
{
    /*  Display the selected font.  */
    UpdateSample(view, 0);

    /* mark timer as off */
    view->timer_id = 0;
} /* end of TimeOutHandler */


/*
 * This routine is used to handle multi-click to the integerField widget.
 * It will update the pointsize only to the last click and not all the
 * clicks in between
 */
void
OutlinePSCB(w, client_data, call_data)
    Widget w;
    XtPointer client_data;
    XtPointer call_data;
{
    OlIntegerFieldChanged *cbP = (OlIntegerFieldChanged *) call_data;
    int			value = cbP->value;
    view_type *view = (view_type *) client_data;

    view->cur_size = value;

    if (view->timer_id) 
	XtRemoveTimeOut(view->timer_id);
    view->timer_id = XtAddTimeOut(500, 
				  (XtTimerCallbackProc)TimeOutHandler, view);
} /* end of OutlinePSCB */


void
ResetFont (view)
	view_type *view;
{
    Arg args[1];
    _OlArenaType(LookArena) * look_arena;
    _OlArenaType(PSArena) * ps_arena;
    family_info tmp;
    int family_index;
    int spot;

    tmp.n = view->cur_family;
    if ((spot = _OlArenaFindHint(view->family_arena, &family_index, tmp)) ==
	_OL_NULL_ARENA_INDEX)  {
	/* reset the view */
	strcpy(view->cur_family, DEFAULT_FAMILY);
	strcpy(view->cur_look, DEFAULT_LOOK);
	view->cur_size = atoi(DEFAULT_POINT_SIZE);
	tmp.n = view->cur_family;
	if (_OlArenaFindHint(view->family_arena, &family_index, tmp) ==
	    _OL_NULL_ARENA_INDEX)
	    family_index = 0;
    }
    UpdateFamily(view, family_index);

}  /* end of ResetFont() */


