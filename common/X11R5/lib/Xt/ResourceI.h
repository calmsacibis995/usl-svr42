/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xt:ResourceI.h	1.1"
/* $XConsortium: ResourceI.h,v 1.8 91/06/11 20:06:59 converse Exp $ */

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

/****************************************************************
 *
 * Resources
 *
 ****************************************************************/

#define StringToQuark(string) XrmStringToQuark(string)
#define StringToName(string) XrmStringToName(string)
#define StringToClass(string) XrmStringToClass(string)

typedef struct _XtTypedArg* XtTypedArgList;

extern void _XtResourceDependencies(
#if NeedFunctionPrototypes
    WidgetClass  /* wc */
#endif
);

extern void _XtConstraintResDependencies(
#if NeedFunctionPrototypes
    ConstraintWidgetClass  /* wc */
#endif
);

extern XtCacheRef* _XtGetResources(
#if NeedFunctionPrototypes
    Widget	    /* w */,
    ArgList	    /* args */,
    Cardinal	    /* num_args */,
    XtTypedArgList  /* typed_args */,
    Cardinal*	    /* num_typed_args */
#endif
);

extern void _XtCopyFromParent(
#if NeedFunctionPrototypes
    Widget		/* widget */,
    int			/* offset */,
    XrmValue*		/* value */
#endif
);
