/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:AsciiSrcP.h	1.2"
/*
* $XConsortium: AsciiSrcP.h,v 1.8 91/02/20 17:58:15 converse Exp $
*/


/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

/*
 * AsciiSrcP.h - Private Header for Ascii Text Source.
 *
 * This is the private header file for the Ascii Text Source.
 * It is intended to be used with the Text widget, the simplest way to use
 * this text source is to use the AsciiText Object.
 *
 * Date:    June 29, 1989
 *
 * By:      Chris D. Peterson
 *          MIT X Consortium 
 *          kit@expo.lcs.mit.edu
 */

/* 
 * TextSrcP.h - Private definitions for AsciiSrc object
 * 
 */

#ifndef _XawAsciiSrcP_h
#define _XawAsciiSrcP_h

#include <X11/Xaw/TextSrcP.h>
#include <X11/Xaw/AsciiSrc.h>

/************************************************************
 *
 * Private declarations.
 *
 ************************************************************/

#ifdef L_tmpnam
#define TMPSIZ L_tmpnam
#else
#define TMPSIZ 32		/* bytes to allocate for tmpnam */
#endif

#define MAGIC_VALUE ((XawTextPosition) -1) /* Magic value. */

#define streq(a, b)        ( strcmp((a), (b)) == 0 )

typedef struct _Piece {		/* Piece of the text file of BUFSIZ allocated 
				   characters. */
  char * text;			/* The text in this buffer. */
  XawTextPosition used;		/* The number of characters of this buffer 
				   that have been used. */
  struct _Piece *prev, *next;	/* linked list pointers. */
} Piece;

/************************************************************
 *
 * New fields for the AsciiSrc object class record.
 *
 ************************************************************/

typedef struct _AsciiSrcClassPart { char foo; } AsciiSrcClassPart;

/* Full class record declaration */
typedef struct _AsciiSrcClassRec {
    ObjectClassPart     object_class;
    TextSrcClassPart	text_src_class;
    AsciiSrcClassPart	ascii_src_class;
} AsciiSrcClassRec;

extern AsciiSrcClassRec asciiSrcClassRec;

/* New fields for the AsciiSrc object record */

typedef struct _AsciiSrcPart {

  /* Resources. */

  char       *string;		/* either the string, or the
				   file name, depending upon the type. */
  XawAsciiType type;		/* either string or disk. */
  XawTextPosition piece_size;	/* Size of text buffer for each piece. */
  Boolean data_compression;	/* compress to minimum memory automatically
				   on save? */
  XtCallbackList callback;	/* A callback list to call when the source is
				   changed. */
  Boolean use_string_in_place;	/* Use the string passed in place. */
  int     ascii_length;		/* length field for ascii string emulation. */

#ifdef ASCII_DISK
  String filename;		/* name of file for Compatability. */
#endif /* ASCII_DISK */

/* Private data. */

  Boolean	is_tempfile;	  /* Is this a temporary file? */
  Boolean       changes;	  /* Has this file been edited? */
  Boolean       allocated_string; /* Have I allocated the
				     string in ascii_src->string? */
  XawTextPosition length; 	/* length of file */
  Piece * first_piece;		/* first piece of the text. */
} AsciiSrcPart;

/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _AsciiSrcRec {
  ObjectPart    object;
  TextSrcPart	text_src;
  AsciiSrcPart	ascii_src;
} AsciiSrcRec;

#endif /* _XawAsciiSrcP_h  --- Don't add anything after this line. */
