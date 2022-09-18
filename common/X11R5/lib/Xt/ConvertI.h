/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xt:ConvertI.h	1.1"
/* $XConsortium: ConvertI.h,v 1.14 91/05/11 14:53:10 converse Exp $ */

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

/* Representation types */

extern	XrmQuark  _XtQString;

/*
 * Resource conversions
 */

typedef struct _ConverterRec **ConverterTable;

extern void _XtSetDefaultConverterTable(
#if NeedFunctionPrototypes
    ConverterTable* 		/* table */
#endif
);

extern void _XtFreeConverterTable(
#if NeedFunctionPrototypes
    ConverterTable 		/* table */
#endif
);

extern void _XtTableAddConverter(
#if NeedFunctionPrototypes
    ConverterTable		/* table */,
    XrmRepresentation    	/* from_type */,
    XrmRepresentation    	/* to_type */,
    XtTypeConverter      	/* converter */,
    XtConvertArgList     	/* convert_args */,
    Cardinal             	/* num_args */,
    _XtBoolean              	/* new_style */,
    XtCacheType	    		/* cache_type */,
    XtDestructor         	/* destructor */
#endif
);

extern Boolean _XtConvert(
#if NeedFunctionPrototypes
    Widget			/* widget */,
    XrmRepresentation    	/* from_type */,
    XrmValuePtr			/* from */,
    XrmRepresentation		/* to_type */,
    XrmValuePtr			/* to */,
    XtCacheRef*			/* cache_ref_return */
#endif			  
);

