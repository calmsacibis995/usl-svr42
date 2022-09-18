/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:AsciiTextP.h	1.2"
/*
 * $XConsortium: AsciiTextP.h,v 1.15 89/07/17 18:09:37 kit Exp $ 
 */

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

/***********************************************************************
 *
 * AsciiText Widget
 *
 ***********************************************************************/

/*
 * AsciiText.c - Private header file for AsciiText Widget.
 *
 * This Widget is intended to be used as a simple front end to the 
 * text widget with an ascii source and ascii sink attached to it.
 *
 * Date:    June 29, 1989
 *
 * By:      Chris D. Peterson
 *          MIT X Consortium 
 *          kit@expo.lcs.mit.edu
 */

#ifndef _AsciiTextP_h
#define _AsciiTextP_h

#include <X11/Xaw/TextP.h>
#include <X11/Xaw/AsciiSrc.h> /* no need to get private header. */
#include <X11/Xaw/AsciiText.h>

typedef struct {int empty;} AsciiClassPart;

typedef struct _AsciiTextClassRec {
    CoreClassPart	core_class;
    SimpleClassPart	simple_class;
    TextClassPart	text_class;
    AsciiClassPart	ascii_class;
} AsciiTextClassRec;

extern AsciiTextClassRec asciiTextClassRec;

typedef struct { char foo; /* keep compiler happy. */ } AsciiPart;

typedef struct _AsciiRec {
    CorePart		core;
    SimplePart		simple;
    TextPart		text;
    AsciiPart           ascii;
} AsciiRec;

/************************************************************
 *
 * Ascii String Emulation widget.
 *
 ************************************************************/ 

#ifdef ASCII_STRING

typedef struct {int empty;} AsciiStringClassPart;

typedef struct _AsciiStringClassRec {
    CoreClassPart	core_class;
    SimpleClassPart	simple_class;
    TextClassPart	text_class;
    AsciiClassPart	ascii_class;
    AsciiStringClassPart string_class;
} AsciiStringClassRec;

extern AsciiStringClassRec asciiStringClassRec;

typedef struct { char foo; /* keep compiler happy. */ } AsciiStringPart;

typedef struct _AsciiStringRec {
    CorePart		core;
    SimplePart		simple;
    TextPart		text;
    AsciiPart           ascii;
    AsciiStringPart     ascii_str;
} AsciiStringRec;

#endif /* ASCII_STRING */

#ifdef ASCII_DISK

/************************************************************
 *
 * Ascii Disk Emulation widget.
 *
 ************************************************************/ 

typedef struct {int empty;} AsciiDiskClassPart;

typedef struct _AsciiDiskClassRec {
    CoreClassPart	core_class;
    SimpleClassPart	simple_class;
    TextClassPart	text_class;
    AsciiClassPart	ascii_class;
    AsciiDiskClassPart	disk_class;
} AsciiDiskClassRec;

extern AsciiDiskClassRec asciiDiskClassRec;

typedef struct { char foo; /* keep compiler happy. */ } AsciiDiskPart;

typedef struct _AsciiDiskRec {
    CorePart		core;
    SimplePart		simple;
    TextPart		text;
    AsciiPart           ascii;
    AsciiDiskPart       ascii_disk;
} AsciiDiskRec;
#endif /* ASCII_DISK */

#endif /* _AsciiTextP_h */
