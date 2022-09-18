/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xt:RectObj.c	1.2"
/* $XConsortium: RectObj.c,v 1.14 91/06/11 20:11:45 converse Exp $ */

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#define RECTOBJ
#include "IntrinsicI.h"
#include "StringDefs.h"
/******************************************************************
 *
 * Rectangle Object Resources
 *
 ******************************************************************/

#ifndef MOTIFBC
static void XtCopyAncestorSensitive();
#else
extern void XtCopyAncestorSensitive();
#endif

static XtResource resources[] = {

    {XtNancestorSensitive, XtCSensitive, XtRBoolean, sizeof(Boolean),
      XtOffsetOf(RectObjRec,rectangle.ancestor_sensitive),XtRCallProc,
      (XtPointer)XtCopyAncestorSensitive},
    {XtNx, XtCPosition, XtRPosition, sizeof(Position),
         XtOffsetOf(RectObjRec,rectangle.x), XtRImmediate, (XtPointer)0},
    {XtNy, XtCPosition, XtRPosition, sizeof(Position),
         XtOffsetOf(RectObjRec,rectangle.y), XtRImmediate, (XtPointer)0},
    {XtNwidth, XtCWidth, XtRDimension, sizeof(Dimension),
         XtOffsetOf(RectObjRec,rectangle.width), XtRImmediate, (XtPointer)0},
    {XtNheight, XtCHeight, XtRDimension, sizeof(Dimension),
         XtOffsetOf(RectObjRec,rectangle.height), XtRImmediate, (XtPointer)0},
    {XtNborderWidth, XtCBorderWidth, XtRDimension, sizeof(Dimension),
         XtOffsetOf(RectObjRec,rectangle.border_width), XtRImmediate,
	 (XtPointer)1},
    {XtNsensitive, XtCSensitive, XtRBoolean, sizeof(Boolean),
         XtOffsetOf(RectObjRec,rectangle.sensitive), XtRImmediate,
	 (XtPointer)True}
    };

static void RectObjInitialize();
static void RectClassPartInitialize();
static void RectSetValuesAlmost();

externaldef(rectobjclassrec) RectObjClassRec rectObjClassRec = {
  {
    /* superclass	  */	(WidgetClass)&objectClassRec,
    /* class_name	  */	"Rect",
    /* widget_size	  */	sizeof(RectObjRec),
    /* class_initialize   */    NULL,
    /* class_part_initialize*/	RectClassPartInitialize,
    /* class_inited       */	FALSE,
    /* initialize	  */	RectObjInitialize,
    /* initialize_hook    */	NULL,		
    /* realize		  */	NULL,
    /* actions		  */	NULL,
    /* num_actions	  */	0,
    /* resources	  */	resources,
    /* num_resources	  */	XtNumber(resources),
    /* xrm_class	  */	NULLQUARK,
    /* compress_motion	  */	FALSE,
    /* compress_exposure  */	TRUE,
    /* compress_enterleave*/ 	FALSE,
    /* visible_interest	  */	FALSE,
    /* destroy		  */	NULL,
    /* resize		  */	NULL,
    /* expose		  */	NULL,
    /* set_values	  */	NULL,
    /* set_values_hook    */	NULL,			
    /* set_values_almost  */	RectSetValuesAlmost,  
    /* get_values_hook    */	NULL,			
    /* accept_focus	  */	NULL,
    /* version		  */	XtVersion,
    /* callback_offsets   */    NULL,
    /* tm_table           */    NULL,
    /* query_geometry	    */  NULL,
    /* display_accelerator  */	NULL,
    /* extension	    */  NULL
  }
};

externaldef(rectObjClass)
WidgetClass rectObjClass = (WidgetClass)&rectObjClassRec;

/*ARGSUSED*/
#ifndef MOTIFBC
static void XtCopyAncestorSensitive(widget, offset, value)
#else
extern void XtCopyAncestorSensitive(widget, offset, value)
#endif
    Widget      widget;
    int		offset;
    XrmValue    *value;
{
    static Boolean  sensitive;
    Widget parent = widget->core.parent;

    sensitive = (parent->core.ancestor_sensitive & parent->core.sensitive);
    value->addr = (XPointer)(&sensitive);
}


/*
 * Start of rectangle object methods
 */


static void RectClassPartInitialize(wc)
    register WidgetClass wc;
{
    register RectObjClass roc = (RectObjClass)wc;
    register RectObjClass super = ((RectObjClass)roc->rect_class.superclass);

    /* We don't need to check for null super since we'll get to object
       eventually, and it had better define them!  */


    if (roc->rect_class.resize == XtInheritResize) {
	roc->rect_class.resize = super->rect_class.resize;
    }

    if (roc->rect_class.expose == XtInheritExpose) {
	roc->rect_class.expose = super->rect_class.expose;
    }

    if (roc->rect_class.set_values_almost == XtInheritSetValuesAlmost) {
	roc->rect_class.set_values_almost = super->rect_class.set_values_almost;
    }


    if (roc->rect_class.query_geometry == XtInheritQueryGeometry) {
	roc->rect_class.query_geometry = super->rect_class.query_geometry;
    }
}

/* ARGSUSED */
static void RectObjInitialize(requested_widget, new_widget, args, num_args)
    Widget   requested_widget;
    register Widget new_widget;
    ArgList args;
    Cardinal *num_args;
{
    ((RectObj)new_widget)->rectangle.managed = FALSE;
}

/*ARGSUSED*/
static void RectSetValuesAlmost(old, new, request, reply)
    Widget		old;
    Widget		new;
    XtWidgetGeometry    *request;
    XtWidgetGeometry    *reply;
{
    *request = *reply;
}
