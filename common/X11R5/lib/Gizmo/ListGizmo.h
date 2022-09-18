/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:ListGizmo.h	1.10"
#endif

/*
 * ListGizmo.h
 *
 */

#ifndef _ListGizmo_h
#define _ListGizmo_h

/*
 * ListGizmo
 *
 * The \fIListGizmo\fP is used to construct a scrolled list interface
 * element consisting of a caption (optional), scrolled window, and list.
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <ListGizmo.h>
 * ... 
 */

typedef struct
   {
   XtArgVal        set;
   XtArgVal        fields;
   XtArgVal        clientData;
   } ListItem;

typedef struct
   {
   ListItem *      list;
   int             size;           /* # of items in the list   */
   int             numFields;      /* # of fields in each item */
   } ListHead;

typedef struct _ListGizmo
   {
   HelpInfo *      help;            /* help information          */
   char *          name;            /* name of the widget        */
   char *          caption;         /* caption label             */
   Setting *       settings;        /* settings                  */
   char *          format;          /* list format               */
   XtArgVal        exclusive;       /* what is this ?            */
   int             height;          /* item height               */
   char *          font;
   XtArgVal        copy_settings;
   void            (*executeCB)();
   void            (*selectCB)();
   void            (*unselectCB)();
   ArgList         args;
   Cardinal        num_args;
   Widget          flatList;
   } ListGizmo;

extern GizmoClassRec ListGizmoClass[];

/*
 * FIX: needed ?
 */

extern void   FreeList (ListHead * flist);
extern char * GetListField(ListGizmo *gizmo, int item);
extern Widget GetList(ListGizmo *gizmo);
extern void   InsertListItem(ListGizmo * gizmo, int item, char ** fields, int numfields);
extern void   DeleteListItem(ListGizmo * gizmo, int item);

#endif /* _ListGizmo_h */
