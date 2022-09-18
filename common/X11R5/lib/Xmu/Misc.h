/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:Misc.h	1.2"
/*
* $XConsortium: Misc.h,v 1.1 89/05/10 16:00:25 jim Exp $
*/

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

/* Various useful constant and macro definitions */

#ifndef _Xmu_Misc_h
#define _Xmu_Misc_h

#define MAXDIMENSION	((1 << 31)-1)

#define Max(x, y)	(((x) > (y)) ? (x) : (y))
#define Min(x, y)	(((x) < (y)) ? (x) : (y))
#define AssignMax(x, y)	{if ((y) > (x)) x = (y);}
#define AssignMin(x, y)	{if ((y) < (x)) x = (y);}

#endif /*_Xmu_Misc_h*/
