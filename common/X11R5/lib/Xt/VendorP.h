/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xt:VendorP.h	1.1"
/*
* $XConsortium: VendorP.h,v 1.20 89/10/04 12:22:55 swick Exp $
* $oHeader: VendorP.h,v 1.2 88/08/18 15:56:48 asente Exp $
*/

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

/* 
 * VendorP.h - Private definitions for VendorShell widget
 * 
 * Author:	Paul Asente
 * 		Digital Equipment Corporation
 * 		Western Software Laboratory
 * Date:	Thu Dec 3, 1987
 */

/***********************************************************************
 *
 * VendorShell Widget Private Data
 *
 ***********************************************************************/

#ifndef  _XtVendorPrivate_h
#define _XtVendorPrivate_h

#include <X11/Vendor.h>

/* New fields for the VendorShell widget class record */

typedef struct {
    XtPointer       extension;          /* pointer to extension record      */
} VendorShellClassPart;

typedef struct _VendorShellClassRec {
  	CoreClassPart      core_class;
	CompositeClassPart composite_class;
	ShellClassPart  shell_class;
	WMShellClassPart   wm_shell_class;
	VendorShellClassPart vendor_shell_class;
} VendorShellClassRec;

externalref VendorShellClassRec vendorShellClassRec;

/* New fields for the vendor shell widget. */

typedef struct {
	int		vendor_specific;
} VendorShellPart;

typedef  struct {
	CorePart 	core;
	CompositePart 	composite;
	ShellPart 	shell;
	WMShellPart	wm;
	VendorShellPart	vendor;
} VendorShellRec, *VendorShellWidget;

#endif  /* _XtVendorPrivate_h */
