/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:AsciiSrc.h	1.2"
/*
 * $XConsortium: AsciiSrc.h,v 1.13 91/07/22 19:02:47 converse Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 */


/*
 * AsciiSrc.h - Public Header file for Ascii Text Source.
 *
 * This is the public header file for the Ascii Text Source.
 * It is intended to be used with the Text widget, the simplest way to use
 * this text source is to use the AsciiText Object.
 *
 * Date:    June 29, 1989
 *
 * By:      Chris D. Peterson
 *          MIT X Consortium 
 *          kit@expo.lcs.mit.edu
 */


#ifndef _XawAsciiSrc_h
#define _XawAsciiSrc_h

#include <X11/Xaw/TextSrc.h>
#include <X11/Xfuncproto.h>

/* Resources:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------
 callback	     Callback		Callback	(none)
 dataCompression     DataCompression	Boolean		True
 length		     Length		int		(internal)
 pieceSize	     PieceSize		int		BUFSIZ
 string		     String		String		NULL
 type		     Type		XawAsciiType	XawAsciiString
 useStringInPlace    UseStringInPlace	Boolean		False

*/
 
/* Class record constants */

extern WidgetClass asciiSrcObjectClass;

typedef struct _AsciiSrcClassRec *AsciiSrcObjectClass;
typedef struct _AsciiSrcRec      *AsciiSrcObject;

/*
 * Just to make people's lives a bit easier.
 */

#define AsciiSourceObjectClass AsciiSrcObjectClass
#define AsciiSourceObject      AsciiSrcObject

/*
 * Resource Definitions.
 */

#define XtCDataCompression "DataCompression"
#define XtCPieceSize "PieceSize"
#define XtCType "Type"
#define XtCUseStringInPlace "UseStringInPlace"

#define XtNdataCompression "dataCompression"
#define XtNpieceSize "pieceSize"
#define XtNtype "type"
#define XtNuseStringInPlace "useStringInPlace"

#define XtRAsciiType "AsciiType"

#define XtEstring "string"
#define XtEfile "file"

typedef enum {XawAsciiFile, XawAsciiString} XawAsciiType;

/************************************************************
 *
 * Public routines 
 *
 ************************************************************/

_XFUNCPROTOBEGIN

/*	Function Name: XawAsciiSourceFreeString
 *	Description: Frees the string returned by a get values call
 *                   on the string when the source is of type string.
 *	Arguments: w - the AsciiSrc object.
 *	Returns: none.
 */

extern void XawAsciiSourceFreeString(
#if NeedFunctionPrototypes
    Widget		/* w */
#endif
);

/*	Function Name: XawAsciiSave
 *	Description: Saves all the pieces into a file or string as required.
 *	Arguments: w - the asciiSrc Object.
 *	Returns: TRUE if the save was successful.
 */

extern Boolean XawAsciiSave(
#if NeedFunctionPrototypes
    Widget		/* w */
#endif
);

/*	Function Name: XawAsciiSaveAsFile
 *	Description: Save the current buffer as a file.
 *	Arguments: w - the asciiSrc object.
 *                 name - name of the file to save this file into.
 *	Returns: True if the save was successful.
 */

extern Boolean XawAsciiSaveAsFile(
#if NeedFunctionPrototypes
    Widget		/* w */,
    _Xconst char*	/* name */
#endif 
);

/*	Function Name: XawAsciiSourceChanged
 *	Description: Returns true if the source has changed since last saved.
 *	Arguments: w - the asciiSource object.
 *	Returns: a Boolean (see description).
 */

extern Boolean XawAsciiSourceChanged(
#if NeedFunctionPrototypes
    Widget		/* w */
#endif
);

_XFUNCPROTOEND

#endif /* _XawAsciiSrc_h  - Don't add anything after this line. */

