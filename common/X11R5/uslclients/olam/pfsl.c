/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:pfsl.c	1.10"
#endif

/*
** pfsl.c - This file contains routines to implement an ADT (Abstract Data
** Type) that merges a scrolling list widget and a linked list.  This is
** necessary to traverse the list.
** Most routines operate on a special item called the "current" item.  This
** is usually the item selected by the user, but it can also be set
** programatically.
*/


#ifndef MEMUTIL
#include <malloc.h>
#else
#include <X11/memutil.h>
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>

#include "errors.h"
#include "config.h"
#include "create.h"
#include "pfsl.h"
#include "util.h"


static void		Delete();
static OlListToken	FillListMain();
static void		Selected();


/*
** Deletes item from `pfsl' corresponding to `token'.  `token' shouldn't be
** the current token.  This is a helper function for PFSLdelete() and
** PFSLdeleteCurrent().
*/
static void
Delete(pfsl, token)
  PFScrollList	pfsl;
  OlListToken	token;
{
  /*
  ** Delete item from linked list
  */
  (void)ListDelete(pfsl->linked_list,
		   (ListPosition)(OlListItemPointer(token)->user_data));

  /*
  ** Delete item from scrolling list
  */
  (*(pfsl->delete))(pfsl->scrolling_list, token);

}	/* Delete() */


/*
** Change current item's label to `string'
*/
void
PFSLchange(pfsl, string)
  register PFScrollList	pfsl;
  String		string;
{

  if (pfsl->current_token != PFSL_NULL_TOKEN)
    {					/* Make sure there is a current item */
      /*
      ** Change label and then cause the scrolling list to update the visual
      */
      OlListItemPointer(pfsl->current_token)->label = string;
      (*(pfsl->touch))(pfsl->scrolling_list, pfsl->current_token);
    }

}	/* PFSLchange() */


/*
** Delete the item in `pfsl' corresponding to `token'.  Deleting the current
** token is a special case, since a new current item must be chosen.
*/
void
PFSLdelete(pfsl, token)
  PFScrollList	pfsl;
  OlListToken	token;
{

  if (token == pfsl->current_token)	/* `token' is the current token */
    PFSLdeleteCurrent(pfsl);
  else
    Delete(pfsl, token);

}	/* PFSLdelete() */


/*
** Delete all the items in `pfsl'.
** The current item is set to PFSL_NULL_TOKEN; the scrolling list is told
** not to update its visual; and the list is traversed, deleting each item.
*/
void
PFSLdeleteAll(pfsl)
  PFScrollList	pfsl;
{
  register List		linked_list;	/* Linked list part of `pfsl' */
  register ListPosition	pos;		/* Used to traverse `linked_list' */


  
  linked_list = pfsl->linked_list;
  pos = ListHead(linked_list);		/* Point at front of list */

  /*
  ** There is no current token
  */
  pfsl->current_token = PFSL_NULL_TOKEN;

  /*
  ** Tell the scrolling list not to update its visual
  */
  (*(pfsl->update_view))(pfsl->scrolling_list, FALSE);

  /*
  ** Traverse list, deleting each item
  */
  while (pos != LIST_NULL_POS)
    {
      PFSLdelete(pfsl, (OlListToken)ListGet(linked_list, pos));
      pos = ListNext(linked_list, pos);
    }

  /*
  ** Turn scrolling list updating back on
  */
  (*(pfsl->update_view))(pfsl->scrolling_list, TRUE);
  
}	/* PFSLdeleteAll() */


/*
** Delete the current item in `pfsl' and choose a new one.
** If there is a following item, it is made current, otherwise the preceding
** item is chosed (if it exists).  If neither (the list is now empty) there
** is no longer a current item.  The scrolling list's userMakeCurrent
** callbacks are then called with the new current item (NULL if none).
*/
void
PFSLdeleteCurrent(pfsl)
  register PFScrollList	pfsl;
{
  register ListPosition	deleted_pos;	/* Linked list position to be */
					/* deleted */
  OlListToken		deleted_token;	/* Scrolling list token to be */
					/* deleted */
  List			linked_list;	/* Linked list part of `pfsl' */
  ListPosition		new_pos;	/* Linked list positon of new */
					/* current item */
  OlListToken		selected_token;	/* Scrolling list token of new */
					/* current item */


  if (pfsl->current_token != PFSL_NULL_TOKEN)
    {					/* There is a current item */
      deleted_token = pfsl->current_token;
      /*
      ** Convert `deleted_token' into a linked list position
      */
      deleted_pos = (ListPosition)(OlListItemPointer(deleted_token)->user_data);

      linked_list = pfsl->linked_list;

      if (deleted_pos != ListTail(linked_list)) 
					/* There is a following item */
	new_pos = ListNext(linked_list, deleted_pos);
      else				/* No following item */
	if (deleted_pos != ListHead(linked_list))
					/* There is a preceding item */
	  new_pos = ListPrev(linked_list, deleted_pos);
	else				/* No item before or after */
	  {
	    /*
	    ** The list is now empty; there is no new current item
	    */
	    new_pos = LIST_NULL_POS;
	    selected_token = PFSL_NULL_TOKEN;
	  }
	
      /*
      ** If we found a new current item, convert the linked list position to
      ** a scrolling list token
      */
      if (new_pos != LIST_NULL_POS)
	selected_token = (OlListToken)ListGet(linked_list, new_pos);

      /*
      ** Call callbacks with the new current item (PFSL_NULL_TOKEN if none)
      ** Note that this makes `selected_token' current since Selected() is
      ** in the callback list.
      */
      XtCallCallbacks(pfsl->scrolling_list, XtNuserMakeCurrent,
		      (XtPointer)selected_token);

      /*
      ** Delete the thing already!
      */
      Delete(pfsl, deleted_token);
    }	    
	  
}	/* PFSLdeleteCurrent() */


/*
** Fills `pfsl' with the result of calling `get_string' with `stream' as an
** argument.
** `get_stream' is expected to return NULL on EOF.
** The first item is made the current item and the scrolling list's
** userMakeCurrent callbacks are called with it as an argument.  Then the
** rest of the list is filled.
** FillListMain() does the actual work of adding each item.
*/
void
PFSLfill(pfsl, get_string, stream)
  register PFScrollList	pfsl;
  register String	(*get_string)();
  register FILE		*stream;
{
  OlListToken		first_item;
  register String	string;


  if ((string = (*get_string)(stream)) != NULL)
    {					/* There is at least one string in */
					/* `stream' */
      /*
      ** Tell the scrolling list not to update its visual
      */
      (*(pfsl->update_view))(pfsl->scrolling_list, FALSE);

      /*
      ** Call callbacks with the the first item.
      ** Note that this makes `first_item' current since Selected() is
      ** in the callback list.
      */
      first_item = FillListMain(pfsl, string, PFSL_NULL_TOKEN,
				ListTail(pfsl->linked_list));
      XtCallCallbacks(pfsl->scrolling_list, XtNuserMakeCurrent,
		      (XtPointer)first_item);

      /*
      ** Fill the rest of the list
      */
      while ((string = (*get_string)(stream)) != NULL)
	(void)FillListMain(pfsl, string, PFSL_NULL_TOKEN,
			   ListTail(pfsl->linked_list));

      /*
      ** Turn scrolling list updating back on
      */
      (*(pfsl->update_view))(pfsl->scrolling_list, TRUE);
    }

}	/* PFSLfill() */


/*
** Helper for PFSLfill().  Adds an item with label `string' to `pfsl' before
** `token' in the scrolling list (PFSL_NULL_TOKEN to append) and after `pos'
** in the linked list.
** The scrolling list token for the new item is returned.
*/
static OlListToken
FillListMain(pfsl, string, token, pos)
  PFScrollList	pfsl;
  String	string;
  OlListToken	token;
  ListPosition	pos;
{
  OlListItem	item;			/* New scrolling list item */
  ListPosition	new_pos;		/* Pointer to new linked list entry */
  OlListToken	new_token;		/* New scrollgin list token */


  DEBUG_MSG( OlGetMessage(XtDisplay(shell), NULL,
             0,
             OleNfilepfsl,
             OleTmsg1,
             OleCOlClientOlamMsgs,
             OleMfilepfsl_msg1,
             (XrmDatabase)NULL),
             string);

  /*
  ** Set up scrolling list item
  */
  item.label = string;
  item.label_type = OL_STRING;
  item.attr = 0;
  item.mnemonic = '\0';
  /*
  ** Add item to scrolling list
  */
  new_token = (*(pfsl->add))(pfsl->scrolling_list, PFSL_NULL_TOKEN, token,
			     item);
  /*
  ** Insert token for the new item in the linked list
  */
  new_pos = ListInsert(pfsl->linked_list, pos, (ListData)new_token);

  /*
  ** Save the linked list position in the scrolling list's user_data field
  */
  OlListItemPointer(new_token)->user_data = (XtPointer)new_pos;

  return new_token;

}	/* FillListMain() */


/*
** Returns a new `pfsl' with `parent' as the parent for the widgets that are
** created.  The scrolling list is labeled by a caption with `label' as its
** string.  `select_callback' and `view_height' correspond to the scrolling
** list's XtNuserMakeCurrent and XtNviewHeight resources respectively.  The
** scrolling list's view is forced to `view_width' characters in width.
** A fixed font is used so that scrolling list items will line up.  This
** font is cached in a static variable.
** If a new pfsl cannot be created, PFSL_ERROR is returned.
*/
PFScrollList
PFSLnew(parent, label, select_callback, view_width, view_height)
  Widget		parent;
  String		label;
  XtCallbackProc	select_callback;
  Cardinal		view_width;
  Cardinal		view_height;
{
  Arg			arg[5];
  char			*blanks;	/* Used to force view width */
  Widget		caption;	/* Caption for scrolling list */
  static XFontStruct	*fixed_font;	/* Fixed width font struct */
  register int		i;		/* Used to force view width */
  Cardinal		n;		/* Number of `args' used */
  PFScrollList		pfsl;		/* New pfsl */


  /*
  ** Create a caption for the scrolling list
  */
  caption = CreateCaption(parent, label);

  /*
  ** Allocate a pfsl structure
  */
  if ((pfsl = (PFScrollList)malloc(sizeof(_PFScrollList))) == 
      (PFScrollList)NULL)		/* malloc(3) failed */
    return PFSL_ERROR;

  /*
  ** There is no current item
  */
  pfsl->current_token = PFSL_NULL_TOKEN;

  /*
  ** Create the linked list part of pfsl
  */
  if ((pfsl->linked_list = ListNew()) == LIST_NULL)
    return PFSL_ERROR;

  n = (Cardinal)0;
  /*
  ** Don't allow selection in scrolling list
  */
  XtSetArg(arg[n], XtNselectable, (XtArgVal)FALSE);			n++;
  XtSetArg(arg[n], XtNviewHeight, (XtArgVal)view_height);		n++;

  /*
  ** Get a fixed font if one isn't cached.  If the font we want isn't there,
  ** we'll have to use the default.
  */
  if (fixed_font == (XFontStruct *)NULL)
    fixed_font = XLoadQueryFont(toplevelDisplay, LISTFONT);

  if (fixed_font != (XFontStruct *)NULL) /* We've' got the font we want */
    {
      XtSetArg(arg[n], XtNfont, (XtArgVal)fixed_font);			n++;
    }

  /*
  ** Create the scrolling list part of pfsl
  */
  pfsl->scrolling_list = XtCreateManagedWidget("ScrollingList",
					       scrollingListWidgetClass,
					       caption,
					       arg,
					       n);

  /*
  ** Cache the scrolling list "method"s in the structure so we don't have to
  ** do an XtGetValues() every time we want to use one
  */
  XtSetArg(arg[0], XtNapplAddItem, &(pfsl->add));
  XtSetArg(arg[1], XtNapplDeleteItem, &(pfsl->delete));
  XtSetArg(arg[2], XtNapplTouchItem, &(pfsl->touch));
  XtSetArg(arg[3], XtNapplUpdateView, &(pfsl->update_view));
  XtSetArg(arg[4], XtNapplViewItem, &(pfsl->view));
  XtGetValues(pfsl->scrolling_list, arg, (Cardinal)5);

  /*
  ** Add the internal pfsl selection callback
  */
  XtAddCallback(pfsl->scrolling_list, XtNuserMakeCurrent, Selected,
		(XtPointer)pfsl);

  /*
  ** Hackery to force the scrolling list to a reasonable width when empty
  */
  if ((blanks = malloc((unsigned)view_width)) == NULL)
    return PFSL_ERROR;
  for (i = 0; i < view_width; ++i)
    blanks[i] = ' ';
  PFSLopen(pfsl, PFSL_AFTER);		/* Open a new item */
  PFSLchange(pfsl, blanks);		/* Change it to `view_width' blanks */
  PFSLdeleteCurrent(pfsl);		/* Delete it; scrolling list won't */
					/* shrink */

  /*
  ** Add user's `select_callback' @
  */
  XtAddCallback(pfsl->scrolling_list, XtNuserMakeCurrent, select_callback,
		(XtPointer)NULL);

  return pfsl;

}	/* PFSLnew() */


/*
** Open a new item before or after the current item.
** `where' can be PFSL_BEFORE or PFSL_AFTER.
** The new item is made current.
*/
void
PFSLopen(pfsl, where)
  PFScrollList	pfsl;
  int		where;
{
  OlListItem	*item;			/* Current item, if any */
  List		linked_list;		/* Linked list part of `pfsl' */
  OlListToken	new_token;		/* New scrolling list token */
  ListPosition	pos;			/* Linked list position to insert */
					/* after */
  OlListToken	token;			/* Scrolling list token to insert */
					/* before */
  

  /*
  ** Get linked list part of `pfsl'
  */
  linked_list = pfsl->linked_list;

  if (pfsl->current_token != NULL)	/* There is a current item */
    {
      /*
      ** Break the current item down into a token and position for use with
      ** FillListMain()
      */
      item = OlListItemPointer(pfsl->current_token);
      pos = (ListPosition)(item->user_data);
      token = pfsl->current_token;

      if (where == PFSL_AFTER)		/* Insert after the current item */
	if (pos == ListTail(linked_list))
	  token = PFSL_NULL_TOKEN;	/* Null token means append to */
					/* scrolling list */
        else
	  token = (OlListToken)ListGet(linked_list,
				       ListNext(linked_list, pos));
      else				/* Insert before the current item */
	if (pos == ListHead(linked_list))
	  pos = LIST_NULL_POS;		/* Null position means prepend to */
					/* the linked list */
        else
	  pos = ListPrev(linked_list, pos);
    }
  else					/* There is no current item; the */
    {					/* list must be empty */
      pos = LIST_NULL_POS;
      token = PFSL_NULL_TOKEN;
    }

  /*
  ** Use fill list main to create an empty entry
  */
  new_token = FillListMain(pfsl, "", token, pos);
  /*
  ** Call callbacks with the the new item
  ** Note that this makes `new_token' current since Selected() is
  ** in the callback list.
  */
  XtCallCallbacks(pfsl->scrolling_list, XtNuserMakeCurrent,
		  (XtPointer)new_token);

}	/* PFSLopen() */


/*
** Internal pfsl callback for scrolling list's userMakeCurrent resource.
** `client_data' is expected to be a pointer to a pfsl, and `call_data' is
** expected to be the selected token.
** This callback updates the scrolling list's visual and also sets `pfsl's
** internal record of the current item.
*/
static void
Selected(w, client_data, call_data)
  Widget	w;
  XtPointer	client_data;
  XtPointer	call_data;
{
  Arg			arg[1];
  register PFScrollList	pfsl;
  register OlListToken	selected_token;


  /*
  ** Cast arguments to appropriate types
  */
  pfsl = (PFScrollList)client_data;
  selected_token = (OlListToken)call_data;

  if (pfsl->current_token == selected_token)
    return;				/* No change; return */

  /*
  ** Make the current item no longer current
  */
  if (pfsl->current_token != PFSL_NULL_TOKEN)
    {
      OlListItemPointer(pfsl->current_token)->attr &= ~OL_LIST_ATTR_CURRENT;
      (*(pfsl->touch))(pfsl->scrolling_list, pfsl->current_token);
    }

  /*
  ** Make the selected token (if any) the current token
  */
  if (selected_token != PFSL_NULL_TOKEN)
    {
      DEBUG_MSG( OlGetMessage(XtDisplay(w), NULL,
             0,
             OleNfilepfsl,
             OleTmsg2,
             OleCOlClientOlamMsgs,
             OleMfilepfsl_msg2,
             (XrmDatabase)NULL),
       	OlListItemPointer(selected_token)->label);
      OlListItemPointer(selected_token)->attr |= OL_LIST_ATTR_CURRENT;
      (*(pfsl->touch))(pfsl->scrolling_list, selected_token);
      /*
      ** Bring the new current item into view
      */
      (*(pfsl->view))(pfsl->scrolling_list, selected_token);
    }

  /*
  ** Record the new current item
  */
  pfsl->current_token = selected_token;

}	/* Selected() */
