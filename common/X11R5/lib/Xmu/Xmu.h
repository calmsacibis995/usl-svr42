/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:Xmu.h	1.2"
/*
 * $XConsortium: Xmu.h,v 1.26 89/07/16 14:12:37 jim Exp $
 *
 * Copyright 1988 by the Massachusetts Institute of Technology
 *
 *
 * The X Window System is a Trademark of MIT.
 *
 * The interfaces described by this header file are for miscellaneous utilities
 * and are not part of the Xlib standard.
 */

#ifndef _XMU_H_
#define _XMU_H_

/*
 * This include file is obsolete and is provided only for compatibility with
 * MIT Release 3 clients.  Callers should use the appropriate include file.  
 *
 * DO NOT ADD ANY NEW INCLUDES OR DEFINITIONS TO THIS FILE!
 */
#include <X11/Intrinsic.h>
#include <X11/Xmu/Atoms.h>		/* _XA_... */
#include <X11/Xmu/CharSet.h>		/* CopyISOLatin1Lowered */
#include <X11/Xmu/Converters.h>		/* CvtStringTo... */
#include <X11/Xmu/Drawing.h>		/* DrawRoundedRect, DrawLogo */
#include <X11/Xmu/Error.h>		/* PrintDefaultError */
#include <X11/Xmu/StdSel.h>		/* ConvertStandardSelection */

#endif /* _XMU_H_ */

