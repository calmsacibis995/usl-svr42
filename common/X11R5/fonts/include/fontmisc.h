/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5fontinc:include/fontmisc.h	1.1"
/*copyright     "%c%"*/

/*

 * $XConsortium: fontmisc.h,v 1.5 91/05/12 16:15:48 rws Exp $
 *
 * Copyright 1991 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Keith Packard, MIT X Consortium
 */

#ifndef _FONTMISC_H_
#define _FONTMISC_H_


#ifndef OS_H
typedef unsigned char	*pointer;
typedef int		Bool;
#define xalloc(n)   Xalloc ((unsigned) n)
#define xfree(p)    Xfree ((pointer) p)
#define xrealloc(p,n)	Xrealloc ((pointer)p,n)
#define lowbit(x) ((x) & (~(x) + 1))
#endif 

#ifndef X_PROTOCOL
typedef unsigned long	Atom;
typedef unsigned long	XID;
#endif
#ifndef LSBFirst
#define LSBFirst	0
#define MSBFirst	1
#endif

#ifndef None
#define None	0l
#endif

#ifndef Success
#define Success 1
#endif 
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

extern char	    *NameForAtom ();

#define assert(x)

#endif /* _FONTMISC_H_ */
