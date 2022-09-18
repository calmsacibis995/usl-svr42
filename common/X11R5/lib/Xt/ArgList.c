/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xt:ArgList.c	1.1"
/* $XConsortium: ArgList.c,v 1.20 91/02/05 16:57:51 gildea Exp $ */

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#include	"IntrinsicI.h"
#include	<stdio.h>

/*
 * This routine merges two arglists. It does NOT check for duplicate entries.
 */

ArgList XtMergeArgLists(args1, num_args1, args2, num_args2)
    ArgList args1;
    Cardinal num_args1;
    ArgList args2;
    Cardinal num_args2;
{
    ArgList result, args;

    result = (ArgList) XtCalloc((unsigned) num_args1 + num_args2,
				(unsigned) sizeof(Arg));

    for (args = result; num_args1 != 0; num_args1--)
    	*args++ = *args1++;
    for (             ; num_args2 != 0; num_args2--)
    	*args++ = *args2++;

    return result;
}

 
