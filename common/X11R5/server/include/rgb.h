/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:include/rgb.h	1.3"

/*copyright     "%c%"*/

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.
                        All Rights Reserved
******************************************************************/
/* $XConsortium: rgb.h,v 1.3 88/09/06 15:48:42 jim Exp $ */

#ifndef RGB_H
#define RGB_H
typedef struct _RGB {
	unsigned short red, green, blue;
	} RGB;

#define RGB_NSIZE 42

typedef struct _RGB_BASE {
        char name [RGB_NSIZE];
        unsigned short red, green, blue;
        } RGB_BASE;

#endif /* RGB_H */
